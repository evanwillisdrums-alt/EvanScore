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

foreach ($renderer in @('default', 'software')) {
    & "$PSScriptRoot/check_evanscore_gui.ps1" -InstallRoot build.install -Renderer $renderer -OutputDirectory "build.artifacts/gui-$renderer-empty"
    & "$PSScriptRoot/check_evanscore_gui.ps1" -InstallRoot build.install -Renderer $renderer -OutputDirectory "build.artifacts/gui-$renderer-score" -ScorePath $score
}

$relativeApp = [System.IO.Path]::GetRelativePath((Resolve-Path build.install).Path, $app.FullName)
$source = "https://github.com/$env:GITHUB_REPOSITORY/tree/$env:GITHUB_SHA"
if (-not (Get-ChildItem build.install -Filter 'EvanScoreNoteInput.qml' -Recurse)) {
    throw 'The floating note-input plugin was not installed'
}
@"
EvanScore Windows development app

Extract the entire downloaded ZIP into a folder, then run:
$relativeApp

Keep the DLLs, plugins, fonts, and other supplied folders together.
This is the native MuseScore fork with the implemented gray styling and rounded controls.
Use the Regular / Percussion button in the top toolbar to select a saved workspace mode.
Percussion uses the supplied style until you save your own default, and places accents above.
In Preferences > Shortcuts, search Flam, Diddle, or Roll, or record side/middle mouse-button bindings.
Percussion input uses one compact notation strip that fits the panel without sideways scrolling.
The reference-style translucent floating keypad has working Windows minimize/close buttons.
The keypad’s top-left open notehead button changes appearance without changing duration or playback.
In Format > Style, Make Default Style saves the current style for new scores and persists after restart.
Existing scores retain their styles; Apply Default Style applies the saved default to the current score.
Saving a default is a permanent preference change even if you later cancel score edits in the dialog.
The optional floating keypad is bundled as the EvanScore Note Input plugin: enable it in Extensions > Manage plugins, then run it from the Extensions menu.
Open View > Dynamics for the native Dynamics sidebar; close it like other panels without disabling saved playback settings.
Full score edits change default mappings; selecting notes, dynamic markings, or hairpins exposes local controls.
Dynamics use 0-127 MIDI velocities (0 is silent), with independent normal, tap, tenuto, accent, marcato, ghost, soft-accent, stress, and unstress mappings and visual draggable curves.
Hairpin endpoints can independently reference note categories, including ff accents to mp taps.
Selection edits support exact values, add/subtract, scaling, and relative percentages, with articulation/category filters.
Save Preset / Load Preset transfer playback profiles as .evands files.
Make Default Dynamics persists for new scores, including templates and after restarting.
Apply Default Dynamics explicitly applies that profile to an existing score without restyling it.
Mappings and curves save with the score; local edits support undo/redo and reset to inherited settings.
Sound libraries determine the audible response to these values; patch-specific Virtual Drumline techniques still use the host's playback setup.
The marimba visualization is still a preview concept.
The internal executable and application name remain MuseScore.

Build checks passed: Windows x64 executable header, native process exit, score-to-PDF export, and desktop startup beyond the splash screen.
Interactive playback has not been checked by this automated test.

Corresponding source and its license information:
$source
"@ | Set-Content (Join-Path (Resolve-Path build.install).Path 'EvanScore-README.txt')

Get-FileHash $app.FullName -Algorithm SHA256 |
    Format-List | Out-File (Join-Path $artifacts 'Windows-executable-sha256.txt')
Write-Host "Verified native Windows app: $relativeApp"
