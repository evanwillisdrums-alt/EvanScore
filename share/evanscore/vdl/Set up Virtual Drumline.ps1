[CmdletBinding()]
param(
    [switch]$CheckOnly,
    [switch]$DownloadOnly,
    [string]$LibraryPath,
    [string]$OutputDirectory
)

$ErrorActionPreference = 'Stop'
$installerUrl = 'https://assets.native-instruments.com/downloads/Native-Access_2.exe'
$downloadPage = 'https://www.native-instruments.com/en/specials/native-access-2/'

function Find-NativeAccess {
    foreach ($root in @($env:ProgramFiles, ${env:ProgramFiles(x86)})) {
        if (-not $root) { continue }
        foreach ($name in @('Native Access.exe', 'NativeAccess.exe')) {
            $candidate = Join-Path $root "Native Instruments\Native Access\$name"
            if (Test-Path -LiteralPath $candidate -PathType Leaf) { return $candidate }
        }
    }
    return $null
}

function Get-SetupStatus {
    $plugins = @()
    foreach ($root in @($env:CommonProgramFiles, ${env:CommonProgramFiles(x86)})) {
        if (-not $root) { continue }
        $vstRoot = Join-Path $root 'VST3'
        if (Test-Path -LiteralPath $vstRoot) {
            $plugins += @(Get-ChildItem -LiteralPath $vstRoot -Filter '*Kontakt*.vst3' -Recurse -ErrorAction SilentlyContinue |
                Select-Object -ExpandProperty FullName)
        }
    }
    $libraryStatus = 'Not selected'
    if ($LibraryPath) {
        $samples = @(Get-ChildItem -LiteralPath $LibraryPath -Filter '*.nkx' -ErrorAction SilentlyContinue)
        $instruments = @(Get-ChildItem -LiteralPath (Join-Path $LibraryPath 'Instruments') -Filter '*.nki' -Recurse -ErrorAction SilentlyContinue)
        $libraryStatus = if ($samples.Count -and $instruments.Count) {
            'Library files found; activation and playback still require verification'
        } else { 'Library folder missing or incomplete' }
    }
    [pscustomobject]@{
        NativeAccess = (Find-NativeAccess)
        KontaktVst3Candidates = @($plugins | Select-Object -Unique)
        VdlLibrary = $libraryStatus
        ActivationVerified = $false
        PlaybackVerified = $false
    }
}

function Get-VerifiedInstaller {
    if (-not $OutputDirectory) {
        $OutputDirectory = Join-Path $env:LOCALAPPDATA 'EvanScore\Setup\NativeAccess'
    }
    New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
    $installer = Join-Path $OutputDirectory 'Native-Access_2.exe'
    $partial = Join-Path $OutputDirectory ('download-' + [guid]::NewGuid().ToString('N') + '.download.exe')
    try {
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        Write-Host 'Downloading Native Access from Native Instruments...'
        Invoke-WebRequest -Uri $installerUrl -OutFile $partial -UseBasicParsing
        $signature = Get-AuthenticodeSignature -LiteralPath $partial
        if ($signature.Status -ne 'Valid' -or
            $signature.SignerCertificate.Subject -notmatch 'Native Instruments') {
            throw 'The download did not have a valid Native Instruments signature. It will not be launched.'
        }
        Move-Item -LiteralPath $partial -Destination $installer -Force
        return $installer
    } finally {
        if (Test-Path -LiteralPath $partial) { Remove-Item -LiteralPath $partial -Force }
    }
}

try {
    if ($CheckOnly -and $DownloadOnly) { throw 'Choose CheckOnly or DownloadOnly, not both.' }
    if ($CheckOnly) { Get-SetupStatus | ConvertTo-Json -Depth 3; exit 0 }
    if ($DownloadOnly) {
        $installer = Get-VerifiedInstaller
        [pscustomobject]@{ Installer = $installer; SignatureVerified = $true; Launched = $false } | ConvertTo-Json
        exit 0
    }

    Start-Process -FilePath (Join-Path $PSScriptRoot 'Virtual Drumline setup.html')
    $nativeAccess = Find-NativeAccess
    if ($nativeAccess) {
        Start-Process -FilePath $nativeAccess
    } else {
        $installer = Get-VerifiedInstaller
        Start-Process -FilePath $installer -Wait
        $nativeAccess = Find-NativeAccess
        if ($nativeAccess) { Start-Process -FilePath $nativeAccess }
    }
    Write-Host 'Follow the setup page: install Kontakt Player with VST3, activate your VDL library, then choose Kontakt in the EvanScore Mixer.'
    Write-Host 'VDL samples, account sign-in, and activation are managed by Native Instruments and Tapspace.'
} catch {
    Write-Host $_.Exception.Message
    Write-Host "Official download page: $downloadPage"
    Write-Host 'You can install Native Access from that page and follow the included setup guide.'
    exit 1
}
