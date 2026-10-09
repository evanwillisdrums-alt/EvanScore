param([string]$AppPath = '', [string]$OutputDirectory = '')
$ErrorActionPreference = 'Stop'
if (-not [Environment]::Is64BitProcess) { throw 'Use 64-bit PowerShell to capture this 64-bit app.' }
if (-not $AppPath) {
    $AppPath = @((Join-Path $PSScriptRoot 'bin/MuseScoreStudio5.exe'), (Join-Path $PSScriptRoot 'MuseScoreStudio5.exe')) | Where-Object { Test-Path $_ } | Select-Object -First 1
}
if (-not $AppPath -or -not (Test-Path $AppPath)) { throw 'Put these two capture files in the extracted EvanScore folder, next to bin.' }
$app = Get-Item $AppPath
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $PSScriptRoot ('crash-report-' + (Get-Date -Format 'yyyyMMdd-HHmmss')) }
New-Item -ItemType Directory -Force $OutputDirectory | Out-Null
$report = (Resolve-Path $OutputDirectory).Path
Start-Transcript -Path (Join-Path $report 'capture.txt') | Out-Null
Write-Host 'EvanScore will open. Reproduce the repeated edits and crash, then send the crash-report folder.'
Write-Host 'Reports stay on this computer; this tool does not upload them or change settings.'
Add-Type -TypeDefinition @'
using System;
using System.Text;
using System.Runtime.InteropServices;
public static class EvanScoreCrashCapture {
    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)] struct StartupInfo {
        public int cb; public string reserved, desktop, title; public uint x,y,xSize,ySize,xChars,yChars,fill,flags;
        public ushort show,reserved2; public IntPtr reservedPtr,stdin,stdout,stderr;
    }
    [StructLayout(LayoutKind.Sequential)] struct ProcessInfo { public IntPtr process,thread; public uint pid,tid; }
    [DllImport("kernel32.dll",CharSet=CharSet.Unicode,SetLastError=true)] static extern bool CreateProcess(string app,StringBuilder command,IntPtr pa,IntPtr ta,bool inherit,uint flags,IntPtr environment,string cwd,ref StartupInfo si,out ProcessInfo pi);
    [DllImport("kernel32.dll",SetLastError=true)] static extern bool WaitForDebugEventEx(IntPtr ev,uint timeout);
    [DllImport("kernel32.dll")] static extern bool ContinueDebugEvent(uint pid,uint tid,uint status);
    [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr handle);
    [DllImport("kernel32.dll")] static extern IntPtr OpenThread(uint access,bool inherit,uint tid);
    [DllImport("kernel32.dll")] static extern bool GetThreadContext(IntPtr thread,IntPtr context);
    [StructLayout(LayoutKind.Sequential)] struct DumpExceptionInfo {
        public uint threadId; public IntPtr pointers; [MarshalAs(UnmanagedType.Bool)] public bool clientPointers;
    }
    [DllImport("dbghelp.dll",SetLastError=true)] static extern bool MiniDumpWriteDump(IntPtr process,uint pid,IntPtr file,uint type,ref DumpExceptionInfo exception,IntPtr user,IntPtr callbacks);
    [DllImport("kernel32.dll")] static extern bool DebugSetProcessKillOnExit(bool kill);
    [DllImport("kernel32.dll")] static extern bool DebugActiveProcessStop(uint pid);
    static void Dump(IntPtr process,uint pid,uint tid,IntPtr ev,string folder,int number) {
        var thread=OpenThread(0x48,false,tid);var allocation=Marshal.AllocHGlobal(1248);var pointers=Marshal.AllocHGlobal(16);
        try {
            var context=new IntPtr((allocation.ToInt64()+15)&~15L);
            for(int i=0;i<1232;i++)Marshal.WriteByte(context,i,0);
            Marshal.WriteInt32(context,48,0x0010001f);
            if(thread==IntPtr.Zero || !GetThreadContext(thread,context))throw new Exception("Could not read faulting thread context");
            Marshal.WriteIntPtr(pointers,0,IntPtr.Add(ev,16));Marshal.WriteIntPtr(pointers,8,context);
            var info=new DumpExceptionInfo {threadId=tid,pointers=pointers,clientPointers=false};
            var path=System.IO.Path.Combine(folder,"evanscore-fault-"+number+".dmp");
            using(var file=new System.IO.FileStream(path,System.IO.FileMode.Create,System.IO.FileAccess.Write,System.IO.FileShare.Read)) {
                if(!MiniDumpWriteDump(process,pid,file.SafeFileHandle.DangerousGetHandle(),0x1020,ref info,IntPtr.Zero,IntPtr.Zero))
                    throw new Exception("MiniDumpWriteDump failed: "+Marshal.GetLastWin32Error());
            }
            Console.WriteLine("Saved fault dump: "+path);
        } catch(Exception e) {Console.WriteLine("Dump capture failed: "+e.Message);}
        finally {if(thread!=IntPtr.Zero)CloseHandle(thread);Marshal.FreeHGlobal(pointers);Marshal.FreeHGlobal(allocation);}
    }
    public static void Observe(string app,string cwd,string folder) {
        var si=new StartupInfo();si.cb=Marshal.SizeOf(si); ProcessInfo pi;
        if(!CreateProcess(app,new StringBuilder("\""+app+"\" --debug --session-type start-empty"),IntPtr.Zero,IntPtr.Zero,false,2,IntPtr.Zero,cwd,ref si,out pi))throw new Exception("CreateProcess failed: "+Marshal.GetLastWin32Error());
        DebugSetProcessKillOnExit(false);
        var ev=Marshal.AllocHGlobal(176);bool breakpointHandled=false;bool exited=false;int faults=0;var deadline=DateTime.UtcNow.AddMinutes(30);
        try {
            while(DateTime.UtcNow<deadline) {
                if(!WaitForDebugEventEx(ev,1000))continue;
                uint code=unchecked((uint)Marshal.ReadInt32(ev,0)),pid=unchecked((uint)Marshal.ReadInt32(ev,4)),tid=unchecked((uint)Marshal.ReadInt32(ev,8));uint status=0x10002;
                if(code==1) {
                    uint exception=unchecked((uint)Marshal.ReadInt32(ev,16));
                    uint chance=unchecked((uint)Marshal.ReadInt32(ev,168));
                    status=0x80010001;
                    if(exception==0x80000003 && !breakpointHandled){breakpointHandled=true;status=0x10002;}
                    else {
                        if(exception==0xc0000005 || exception==0xc0000409 || exception==0xc000001d || exception==0xc0000094) {
                            if(faults<3)Dump(pi.process,pid,tid,ev,folder,++faults);
                            // Early crashes can occur before the app creates its
                            // logger or main window; retain their faulting stack.
                            Console.WriteLine("Fault 0x"+exception.ToString("x")+" thread="+tid+" firstChance="+chance);
                        }
                    }
                }
                // Close debugger-owned DLL/create-process file handles.
                if(code==3 || code==6) { var file=Marshal.ReadIntPtr(ev,16); if(file!=IntPtr.Zero && file!=new IntPtr(-1))CloseHandle(file); }
                ContinueDebugEvent(pid,tid,status);
                if(code==5){exited=true;Console.WriteLine("App exit code: "+unchecked((uint)Marshal.ReadInt32(ev,16)));break;}
            }
        } finally {if(!exited)DebugActiveProcessStop(pi.pid);CloseHandle(pi.thread);CloseHandle(pi.process);Marshal.FreeHGlobal(ev);}
    }
}
'@
try { [EvanScoreCrashCapture]::Observe($app.FullName,$app.DirectoryName,$report) }
finally {
    $logs = Join-Path $env:LOCALAPPDATA 'MuseScore/MuseScoreStudio5Development/logs'
    Get-ChildItem $logs -Filter 'MuseScoreStudio*.log' -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending | Select-Object -First 1 | Copy-Item -Destination $report
    Stop-Transcript | Out-Null
    Write-Host "Capture finished: $report"
}
