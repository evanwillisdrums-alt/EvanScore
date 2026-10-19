# SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies
# Private, per-user CPU runtime. No administrator rights, registry edits or login.
param([Parameter(Mandatory=$true)][string]$Root, [int]$Port = 11435)
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
Set-StrictMode -Version Latest
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
if ($Port -lt 1024 -or $Port -gt 65535) { throw 'Invalid local AI port' }
$Root = [IO.Path]::GetFullPath($Root)
[IO.Directory]::CreateDirectory($Root) | Out-Null
$setupLock = [IO.File]::Open((Join-Path $Root 'setup.lock'), 'OpenOrCreate', 'ReadWrite', 'None')
$server = $null
function Progress([string]$Text) { Write-Output "EVAN_AI_PROGRESS|$Text" }
function Sha256([string]$Path) {
    $hash = [Security.Cryptography.SHA256]::Create()
    $stream = [IO.File]::OpenRead($Path)
    try { return ([BitConverter]::ToString($hash.ComputeHash($stream))).Replace('-','').ToLowerInvariant() }
    finally { $stream.Dispose(); $hash.Dispose() }
}
try {
    $runtime = Join-Path $Root 'runtime'
    $exe = Join-Path $runtime 'ollama.exe'
    if (-not (Test-Path $exe) -or -not (Test-Path (Join-Path $Root 'runtime-ready.json'))) {
        $drive = New-Object IO.DriveInfo ([IO.Path]::GetPathRoot($Root))
        if ($drive.AvailableFreeSpace -lt 6GB) { throw 'Local AI setup needs at least 6 GB of free disk space.' }
        # Pinned official release and GitHub-published SHA256. Never execute a
        # partially downloaded archive or weaken TLS/hash checks on failure.
        $url = 'https://github.com/ollama/ollama/releases/download/v0.40.6/ollama-windows-amd64.zip'
        $expected = 'e359637575302f288d76c0007c408a4be9c7376d59c4f28684d63b232a44b427'
        $archive = Join-Path $Root 'runtime.zip'
        if (-not (Test-Path $archive)) {
            Progress 'Downloading the private CPU runtime (official archive, about 1.5 GB)…'
            $partial = "$archive.partial"
            $request = [Net.HttpWebRequest]::Create($url)
            $request.UserAgent = 'EvanScore local AI setup'
            $request.Timeout = 30000
            $request.ReadWriteTimeout = 120000
            $response = $request.GetResponse()
            try {
                $inputStream = $response.GetResponseStream()
                $outputStream = [IO.File]::Create($partial)
                try {
                    $buffer = New-Object byte[] 1048576
                    $received = 0L; $last = -1
                    while (($read = $inputStream.Read($buffer, 0, $buffer.Length)) -gt 0) {
                        $outputStream.Write($buffer, 0, $read); $received += $read
                        $percent = if ($response.ContentLength -gt 0) { [int](100 * $received / $response.ContentLength) } else { -1 }
                        if ($percent -ge 0 -and $percent -ne $last) { Progress "Downloading runtime: $percent%"; $last = $percent }
                    }
                } finally { $outputStream.Dispose(); $inputStream.Dispose() }
            } finally { $response.Dispose() }
            if ((Sha256 $partial) -ne $expected) { throw 'Runtime checksum failed; nothing was installed.' }
            Move-Item $partial $archive -Force
        }
        if ((Sha256 $archive) -ne $expected) { throw 'Cached runtime checksum failed.' }
        Progress 'Verified official runtime. Extracting CPU components…'
        Add-Type -AssemblyName System.IO.Compression.FileSystem
        $zip = [IO.Compression.ZipFile]::OpenRead($archive)
        try {
            [IO.Directory]::CreateDirectory($runtime) | Out-Null
            $prefix = [IO.Path]::GetFullPath($runtime) + [IO.Path]::DirectorySeparatorChar
            foreach ($entry in $zip.Entries) {
                $name = $entry.FullName.Replace('\','/')
                if ($name.EndsWith('/')) { continue }
                if ($name -ne 'ollama.exe' -and -not $name.StartsWith('lib/ollama/')) { continue }
                if ($name -match '(?i)cuda|cublas|cudart|nvrtc|nvjit|nvidia|vulkan|rocm|rocblas|hipblas|hiprtc|amdhip|mlx') { continue }
                $destination = [IO.Path]::GetFullPath((Join-Path $runtime $name))
                if (-not $destination.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe archive path' }
                [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destination)) | Out-Null
                [IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $destination, $true)
            }
        } finally { $zip.Dispose() }
        if (-not (Test-Path $exe)) { throw 'Official archive did not contain the expected runtime.' }
        @{version='0.40.6'; archiveSha256=$expected} | ConvertTo-Json | Set-Content (Join-Path $Root 'runtime-ready.json') -Encoding UTF8
        Remove-Item $archive
    }
    $env:OLLAMA_HOST = "127.0.0.1:$Port"
    $env:OLLAMA_MODELS = Join-Path $Root 'models'
    $env:OLLAMA_NO_CLOUD = '1'
    $env:OLLAMA_KEEP_ALIVE = '0'
    $env:OLLAMA_NUM_PARALLEL = '1'
    $env:OLLAMA_MAX_LOADED_MODELS = '1'
    $env:OLLAMA_CONTEXT_LENGTH = '4096'
    Progress 'Starting private local service to install the model…'
    $occupied = $false
    try { $null = Invoke-RestMethod "http://127.0.0.1:$Port/api/version" -TimeoutSec 2; $occupied = $true } catch {}
    if ($occupied) { throw "Private AI port $Port is already in use. Close the other EvanScore instance and retry." }
    $server = Start-Process -FilePath $exe -ArgumentList 'serve' -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $Root 'setup-service.log') -RedirectStandardError (Join-Path $Root 'setup-service-errors.log')
    $ready = $false
    for ($n=0; $n -lt 100; $n++) {
        if ($server.HasExited) { throw 'Private local AI service could not start; check its setup log or port 11435.' }
        try { $null = Invoke-RestMethod "http://127.0.0.1:$Port/api/version" -TimeoutSec 2; $ready = $true; break } catch { Start-Sleep -Milliseconds 200 }
    }
    if (-not $ready) { throw 'Private local AI service did not become ready.' }
    Progress 'Downloading Qwen 2.5 1.5B local weights (about 1 GB); Ollama verifies model digests…'
    $ErrorActionPreference = 'Continue'
    & $exe pull 'qwen2.5:1.5b' 2>&1 | ForEach-Object {
        $line = ([string]$_) -replace '\x1b\[[0-9;?]*[a-zA-Z]', ''
        if ($line.Trim()) { Progress $line.Trim() }
    }
    $pullExit = $LASTEXITCODE
    $ErrorActionPreference = 'Stop'
    if ($pullExit -ne 0) { throw "Local model download failed ($pullExit). Re-run setup to resume." }
    & $exe show 'qwen2.5:1.5b' --license | Set-Content (Join-Path $Root 'MODEL-LICENSE.txt') -Encoding UTF8
    if ($LASTEXITCODE -ne 0) { throw 'Could not retain the model license.' }
    $models = Invoke-RestMethod "http://127.0.0.1:$Port/api/tags" -TimeoutSec 5
    $found = @($models.models | Where-Object { $_.name -eq 'qwen2.5:1.5b' -and $_.size -gt 64MB -and $_.size -lt 2GB })
    if ($found.Count -ne 1) { throw 'The expected small local model was not installed.' }
    @{runtimeVersion='0.40.6'; model='qwen2.5:1.5b'; port=$Port} | ConvertTo-Json | Set-Content (Join-Path $Root 'ready.json') -Encoding UTF8
    Progress 'Local AI setup complete. The model will load only when you ask.'
} finally {
    if ($server -and -not $server.HasExited) { Stop-Process -Id $server.Id -Force -ErrorAction SilentlyContinue }
    $setupLock.Dispose()
}
