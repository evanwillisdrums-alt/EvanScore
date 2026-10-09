param(
    [Parameter(Mandatory = $true)][string]$InstallRoot,
    [string]$OutputDirectory = 'build.artifacts/gui',
    [string]$ScorePath = '',
    [ValidateSet('default', 'software')][string]$Renderer = 'default',
    [ValidateRange(0, 60)][int]$MinimumOpenSeconds = 0
)

$ErrorActionPreference = 'Stop'
$app = Get-ChildItem $InstallRoot -Filter '*.exe' -Recurse |
    Where-Object { $_.BaseName -like 'MuseScore*' } | Select-Object -First 1
if (-not $app) { throw 'No MuseScore executable found' }
New-Item -ItemType Directory -Force $OutputDirectory | Out-Null
$output = (Resolve-Path $OutputDirectory).Path

Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
public static class EvanScoreWindows {
    public delegate bool EnumCallback(IntPtr handle, IntPtr data);
    [StructLayout(LayoutKind.Sequential)] public struct Rect { public int Left, Top, Right, Bottom; }
    public class Window {
        public long Handle; public string Title; public string Class;
        public int Left, Top, Width, Height;
    }
    [DllImport("user32.dll")] static extern bool EnumWindows(EnumCallback callback, IntPtr data);
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr handle, out uint pid);
    [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr handle);
    [DllImport("user32.dll")] static extern bool GetWindowRect(IntPtr handle, out Rect rect);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] static extern int GetWindowText(IntPtr handle, StringBuilder title, int count);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] static extern int GetClassName(IntPtr handle, StringBuilder name, int count);
    [DllImport("user32.dll", SetLastError=true)] static extern IntPtr SendMessageTimeout(
        IntPtr handle, uint message, UIntPtr wParam, IntPtr lParam, uint flags, uint timeout, out UIntPtr result);
    public static bool Responsive(long handle) {
        UIntPtr result;
        return SendMessageTimeout(new IntPtr(handle), 0, UIntPtr.Zero, IntPtr.Zero, 2, 1000, out result) != IntPtr.Zero;
    }
    public static Window[] Visible(uint processId) {
        var windows = new List<Window>();
        EnumWindows((handle, data) => {
            uint pid; GetWindowThreadProcessId(handle, out pid);
            if (pid != processId || !IsWindowVisible(handle)) return true;
            Rect rect; GetWindowRect(handle, out rect);
            var title = new StringBuilder(1024); GetWindowText(handle, title, title.Capacity);
            var name = new StringBuilder(256); GetClassName(handle, name, name.Capacity);
            windows.Add(new Window { Handle=handle.ToInt64(), Title=title.ToString(), Class=name.ToString(),
                Left=rect.Left, Top=rect.Top, Width=rect.Right-rect.Left, Height=rect.Bottom-rect.Top });
            return true;
        }, IntPtr.Zero);
        return windows.ToArray();
    }
}
'@

$oldBackend = $env:QT_QUICK_BACKEND
$oldPlatform = $env:QT_QPA_PLATFORM
$started = Get-Date
$process = $null
$main = $null
$ready = $false
$closedCleanly = $false
try {
    $scoreToOpen = ''
    if ($ScorePath) {
        # Engraving reference XML is not a complete application project: absent
        # audio/view metadata can mark it changed and legitimately prompt on quit.
        # Round-trip it through this exact app before testing normal GUI closure.
        $scoreToOpen = Join-Path $output ([System.IO.Path]::GetFileNameWithoutExtension($ScorePath) + '.mscz')
        $env:QT_QPA_PLATFORM = 'windows'
        $prepare = Start-Process -FilePath $app.FullName `
            -ArgumentList @('-o', ('"' + $scoreToOpen + '"'), ('"' + (Resolve-Path $ScorePath).Path + '"')) `
            -WorkingDirectory $app.DirectoryName -PassThru `
            -RedirectStandardOutput (Join-Path $output 'prepare-score-stdout.log') `
            -RedirectStandardError (Join-Path $output 'prepare-score-stderr.log')
        if (-not $prepare.WaitForExit(120000)) {
            $prepare.Kill()
            throw 'The app did not finish saving the GUI test score'
        }
        $prepare.Refresh()
        if ($prepare.ExitCode -ne 0 -or -not (Test-Path $scoreToOpen)) {
            throw 'The app could not save a native GUI test score'
        }
        $savedScore = [System.IO.File]::ReadAllBytes($scoreToOpen)
        if ($savedScore.Length -lt 1024 -or $savedScore[0] -ne 0x50 -or $savedScore[1] -ne 0x4B) {
            throw 'The saved GUI test score is not a native MSCZ archive'
        }
    }
    # Use the real Windows desktop; a PDF export never loads the main QML window.
    $env:QT_QPA_PLATFORM = 'windows'
    if ($Renderer -eq 'software') { $env:QT_QUICK_BACKEND = 'software' }
    else { Remove-Item Env:QT_QUICK_BACKEND -ErrorAction SilentlyContinue }
    $launchArguments = @('--debug')
    if ($scoreToOpen) {
        $launchArguments += ('"' + $scoreToOpen + '"')
    } else {
        $launchArguments += @('--session-type', 'start-empty')
    }
    $process = Start-Process -FilePath $app.FullName -ArgumentList $launchArguments `
        -WorkingDirectory $app.DirectoryName -PassThru `
        -RedirectStandardOutput (Join-Path $output 'stdout.log') `
        -RedirectStandardError (Join-Path $output 'stderr.log')
    $deadline = (Get-Date).AddSeconds(90)
    $ready = $false
    $readyChecks = 0
    $expectedScoreName = if ($ScorePath) { [System.IO.Path]::GetFileNameWithoutExtension($ScorePath) } else { '' }
    do {
        Start-Sleep -Milliseconds 1000
        $process.Refresh()
        if ($process.HasExited) { throw "GUI exited with code $($process.ExitCode)" }
        $windows = @([EvanScoreWindows]::Visible($process.Id))
        $windows | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $output 'windows.json')
        $main = $windows | Where-Object { $_.Class -like '*QWindow*' -and $_.Title -like '*EvanScore*' -and $_.Width -ge 600 -and $_.Height -ge 450 } | Select-Object -First 1
        # The startup splash is an 800x380 QWidget. It must disappear as well.
        $splash = $windows | Where-Object { $_.Class -like '*QWidget*' -and $_.Width -eq 800 -and $_.Height -eq 380 }
        $windowResponding = $main -and [EvanScoreWindows]::Responsive($main.Handle)
        $scoreLoaded = -not $ScorePath -or ($main -and $main.Title.Contains($expectedScoreName))
        if ($main -and -not $splash -and $process.Responding -and $windowResponding -and $scoreLoaded) {
            $readyChecks++
        } else {
            $readyChecks = 0
        }
        @{
            elapsedSeconds = [math]::Round(((Get-Date) - $started).TotalSeconds)
            processResponding = $process.Responding
            windowResponding = $windowResponding
            splashVisible = [bool]$splash
            scoreLoaded = [bool]$scoreLoaded
            consecutiveResponsiveChecks = $readyChecks
        } | ConvertTo-Json | Set-Content (Join-Path $output 'startup-state.json')
        # Require the requested score title and sustained response, not a fleeting
        # initial window that can still freeze while startup tasks finish.
        if ($readyChecks -ge 10 -and ((Get-Date) - $started).TotalSeconds -ge $MinimumOpenSeconds) {
            if ($main.Title -notlike '*EvanScore*') { throw "Unexpected main window title: $($main.Title)" }
            $interruptions = $windows | Where-Object {
                $_.Title -match 'Welcome|First.?launch|MuseScore Studio Development' -or
                ($_.Class -match '^Qt[0-9]+QWindow' -and $_.Handle -ne $main.Handle)
            }
            if ($interruptions) { throw 'An unexpected startup dialog covers the score workspace' }
            $ready = $true
            Add-Type -AssemblyName System.Drawing
            $bitmap = [System.Drawing.Bitmap]::new($main.Width, $main.Height)
            $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
            try {
                $graphics.CopyFromScreen($main.Left, $main.Top, 0, 0, $bitmap.Size)
                $bitmap.Save((Join-Path $output 'desktop.png'))
            } finally { $graphics.Dispose(); $bitmap.Dispose() }
            Write-Host "GUI stayed responsive beyond splash for 10 checks: $($main.Title), $($main.Width)x$($main.Height), renderer=$Renderer"
            break
        }
    } while ((Get-Date) -lt $deadline)
    if (-not $ready) { throw 'Desktop startup did not complete within 90 seconds' }
} finally {
    if ($process -and -not $process.HasExited) {
        if ($main -and -not (Test-Path (Join-Path $output 'desktop.png'))) {
            Add-Type -AssemblyName System.Drawing
            $bitmap = [System.Drawing.Bitmap]::new($main.Width, $main.Height)
            $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
            try {
                $graphics.CopyFromScreen($main.Left, $main.Top, 0, 0, $bitmap.Size)
                $bitmap.Save((Join-Path $output 'desktop.png'))
            } finally { $graphics.Dispose(); $bitmap.Dispose() }
        }
        # A successful score launch must close cleanly so the next renderer's
        # check does not inherit an intentional crash-recovery session.
        if ($ready) {
            $closeRequested = $process.CloseMainWindow()
            $closedCleanly = $process.WaitForExit(10000)
            $process.Refresh()
            $closedCleanly = $closeRequested -and $closedCleanly -and $process.ExitCode -eq 0
            @{
                closeRequested = $closeRequested
                exitedNormally = $closedCleanly
                exitCode = if ($process.HasExited) { $process.ExitCode } else { $null }
            } | ConvertTo-Json | Set-Content (Join-Path $output 'shutdown-state.json')
        }
        if (-not $process.HasExited) {
            Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
        }
    }
    $env:QT_QUICK_BACKEND = $oldBackend
    $env:QT_QPA_PLATFORM = $oldPlatform
    $logRoot = Join-Path $env:LOCALAPPDATA 'MuseScore'
    if (Test-Path $logRoot) {
        Get-ChildItem $logRoot -Filter '*.log' -Recurse |
            Where-Object { $_.LastWriteTime -ge $started } |
            ForEach-Object { Copy-Item $_.FullName (Join-Path $output $_.Name) }
    }
    foreach ($name in @('stdout.log', 'stderr.log')) {
        if (Test-Path (Join-Path $output $name)) { Get-Content (Join-Path $output $name) -Tail 100 }
    }
}
if ($ready -and -not $closedCleanly) {
    throw 'The responsive desktop app did not close normally within 10 seconds'
}
