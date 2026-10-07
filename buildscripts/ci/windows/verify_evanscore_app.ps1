$ErrorActionPreference = 'Stop'

$app = Get-ChildItem build.install -Filter '*.exe' -Recurse |
    Where-Object { $_.BaseName -like 'MuseScore*' } |
    Select-Object -First 1
if (-not $app) { throw 'No MuseScore executable was installed' }

$bytes = [System.IO.File]::ReadAllBytes($app.FullName)
if ($bytes.Length -lt 64 -or $bytes[0] -ne 0x4D -or $bytes[1] -ne 0x5A) {
    throw 'Installed app does not have a Windows executable header'
}
$peOffset = [BitConverter]::ToInt32($bytes, 0x3C)
if ($peOffset -lt 0 -or $peOffset + 6 -gt $bytes.Length -or
    [BitConverter]::ToUInt32($bytes, $peOffset) -ne 0x00004550 -or
    [BitConverter]::ToUInt16($bytes, $peOffset + 4) -ne 0x8664) {
    throw 'Installed app is not a Windows x64 PE executable'
}

New-Item -ItemType Directory -Force build.artifacts | Out-Null
$artifacts = (Resolve-Path build.artifacts).Path
$score = (Resolve-Path src/engraving/tests/tuplet_data/tuplet1.mscx).Path
$pdf = Join-Path $artifacts 'Windows-score-export.pdf'
$stdout = Join-Path $artifacts 'Windows-app-stdout.log'
$stderr = Join-Path $artifacts 'Windows-app-stderr.log'
$oldPlatform = $env:QT_QPA_PLATFORM

try {
    $env:QT_QPA_PLATFORM = 'offscreen'
    $process = Start-Process -FilePath $app.FullName `
        -ArgumentList @('-o', ('"' + $pdf + '"'), ('"' + $score + '"')) `
        -WorkingDirectory $app.DirectoryName -PassThru `
        -RedirectStandardOutput $stdout -RedirectStandardError $stderr
    if (-not $process.WaitForExit(120000)) {
        $process.Kill()
        throw 'Score export did not finish within two minutes'
    }
    $process.Refresh()
    if ($process.ExitCode -ne 0) {
        throw "Native score export exited with code $($process.ExitCode)"
    }
    if (-not (Test-Path $pdf)) { throw 'The app did not produce a PDF' }
    $pdfBytes = [System.IO.File]::ReadAllBytes($pdf)
    if ($pdfBytes.Length -lt 1024 -or
        [System.Text.Encoding]::ASCII.GetString($pdfBytes, 0, 4) -ne '%PDF') {
        throw 'The score export is not a valid PDF document'
    }
} finally {
    $env:QT_QPA_PLATFORM = $oldPlatform
    foreach ($log in @($stdout, $stderr)) {
        if (Test-Path $log) { Get-Content $log }
    }
}

$relativeApp = [System.IO.Path]::GetRelativePath((Resolve-Path build.install).Path, $app.FullName)
$source = "https://github.com/$env:GITHUB_REPOSITORY/tree/$env:GITHUB_SHA"
@"
EvanScore Windows development app

Extract the entire downloaded ZIP into a folder, then run:
$relativeApp

Keep the DLLs, plugins, fonts, and other supplied folders together.
This is the native MuseScore fork with the implemented gray styling and rounded controls.
The marimba visualization and floating keypad shown in the HTML preview are still preview concepts.
The internal executable and application name remain MuseScore.

Build checks passed: Windows x64 executable header, native process exit, and score-to-PDF export.
Interactive playback and the visible desktop UI have not been checked by this automated test.

Corresponding source and its license information:
$source
"@ | Set-Content (Join-Path (Resolve-Path build.install).Path 'EvanScore-README.txt')

Get-FileHash $app.FullName -Algorithm SHA256 |
    Format-List | Out-File (Join-Path $artifacts 'Windows-executable-sha256.txt')
Write-Host "Verified native Windows app: $relativeApp"
