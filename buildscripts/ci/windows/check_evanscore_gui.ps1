param(
    [Parameter(Mandatory = $true)][string]$InstallRoot,
    [string]$OutputDirectory = 'build.artifacts/gui',
    [ValidateSet('default', 'software')][string]$Renderer = 'default'
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
try {
    # Use the real Windows desktop; a PDF export never loads the main QML window.
    $env:QT_QPA_PLATFORM = 'windows'
    if ($Renderer -eq 'software') { $env:QT_QUICK_BACKEND = 'software' }
    else { Remove-Item Env:QT_QUICK_BACKEND -ErrorAction SilentlyContinue }
    $process = Start-Process -FilePath $app.FullName -ArgumentList @('--debug', '--session-type', 'start-empty') `
        -WorkingDirectory $app.DirectoryName -PassThru `
        -RedirectStandardOutput (Join-Path $output 'stdout.log') `
        -RedirectStandardError (Join-Path $output 'stderr.log')
    $deadline = (Get-Date).AddSeconds(90)
    $ready = $false
    do {
        Start-Sleep -Milliseconds 1000
        $process.Refresh()
        if ($process.HasExited) { throw "GUI exited with code $($process.ExitCode)" }
        $windows = @([EvanScoreWindows]::Visible($process.Id))
        $windows | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $output 'windows.json')
        $main = $windows | Where-Object { $_.Class -like '*QWindow*' -and $_.Width -ge 600 -and $_.Height -ge 450 } | Select-Object -First 1
        # The startup splash is an 800x380 QWidget. It must disappear as well.
        $splash = $windows | Where-Object { $_.Class -like '*QWidget*' -and $_.Width -eq 800 -and $_.Height -eq 380 }
        if ($main -and -not $splash -and $process.Responding) {
            $ready = $true
            Add-Type -AssemblyName System.Drawing
            $bitmap = [System.Drawing.Bitmap]::new($main.Width, $main.Height)
            $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
            try {
                $graphics.CopyFromScreen($main.Left, $main.Top, 0, 0, $bitmap.Size)
                $bitmap.Save((Join-Path $output 'desktop.png'))
            } finally { $graphics.Dispose(); $bitmap.Dispose() }
            Write-Host "GUI opened beyond splash: $($main.Title), $($main.Width)x$($main.Height), renderer=$Renderer"
            break
        }
    } while ((Get-Date) -lt $deadline)
    if (-not $ready) { throw 'Desktop startup did not complete within 90 seconds' }
} finally {
    if ($process -and -not $process.HasExited) {
        Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
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
