param([Parameter(Mandatory = $true)][string]$InstallRoot)
$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Force build.artifacts | Out-Null
$root = (Resolve-Path $InstallRoot).Path
$app = Get-ChildItem $root -Filter 'MuseScoreStudio5.exe' -Recurse | Select-Object -First 1
if (-not $app) { throw 'The native MuseScoreStudio5.exe is missing' }
$files = Get-ChildItem $root -Recurse -File | Where-Object {
    $_.Extension -eq '.exe' -or $_.Name -like 'Qt6*.dll' -or $_.Name -eq 'qt.conf' -or
    $_.Name -match '^(vcruntime|msvcp|qwindows|qoffscreen|sndfile|libssl|libcrypto)'
} | ForEach-Object {
    [ordered]@{path=[IO.Path]::GetRelativePath($root, $_.FullName); bytes=$_.Length; version=$_.VersionInfo.FileVersion}
}
$files | ConvertTo-Json | Set-Content build.artifacts/package-files.json
Get-ChildItem $root -Filter qt.conf -Recurse | ForEach-Object {
    Write-Host "Qt configuration: $($_.FullName)"
    Get-Content $_.FullName
}
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$vsInstall = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$dumpbin = Get-ChildItem (Join-Path $vsInstall 'VC/Tools/MSVC') -Filter dumpbin.exe -Recurse |
    Where-Object { $_.FullName -like '*Hostx64\x64\*' } | Select-Object -First 1
& $dumpbin.FullName /dependents $app.FullName | Set-Content build.artifacts/package-imports.log
Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
public static class EvanScoreLoaderWindows {
    delegate bool Callback(IntPtr window, IntPtr data);
    [DllImport("user32.dll")] static extern bool EnumWindows(Callback callback, IntPtr data);
    [DllImport("user32.dll")] static extern bool EnumChildWindows(IntPtr parent, Callback callback, IntPtr data);
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr window, out uint pid);
    [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr window);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] static extern int GetWindowText(IntPtr window, StringBuilder text, int size);
    static string Text(IntPtr window) { var text=new StringBuilder(4096); GetWindowText(window,text,text.Capacity); return text.ToString(); }
    public static string[] Inspect(uint processId) {
        var texts=new List<string>();
        EnumWindows((window,data) => {
            uint pid; GetWindowThreadProcessId(window,out pid);
            if(pid!=processId || !IsWindowVisible(window)) return true;
            texts.Add("Window: "+Text(window));
            EnumChildWindows(window,(child,unused) => { texts.Add("Control: "+Text(child)); return true; },IntPtr.Zero);
            return true;
        },IntPtr.Zero);
        return texts.ToArray();
    }
}
'@
$process = Start-Process $app.FullName -ArgumentList '--version' -WorkingDirectory $app.DirectoryName -PassThru `
    -RedirectStandardOutput build.artifacts/package-version-stdout.log -RedirectStandardError build.artifacts/package-version-stderr.log
try {
    $exited = $process.WaitForExit(10000)
    $process.Refresh()
    $windows = if ($exited) { @() } else { @([EvanScoreLoaderWindows]::Inspect($process.Id)) }
    [ordered]@{executable=$app.FullName; exited=$exited; exitCode=if ($exited) {$process.ExitCode} else {$null}; windows=$windows} |
        ConvertTo-Json -Depth 4 | Set-Content build.artifacts/package-loader.json
} finally {
    if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force }
}
foreach ($file in @('package-imports.log', 'package-loader.json', 'package-version-stdout.log', 'package-version-stderr.log')) {
    $content = Get-Content (Join-Path build.artifacts $file) -Raw
    if ($content) {
        $message = ($file + "`n" + $content).Replace('%','%25').Replace("`r",'%0D').Replace("`n",'%0A')
        Write-Host "::notice title=Portable package inspection::$message"
    }
}
