param([Parameter(Mandatory=$true)][string]$InstallRoot)
$ErrorActionPreference = 'Stop'
$app = Get-ChildItem $InstallRoot -Filter '*.exe' -Recurse | Where-Object { $_.BaseName -like 'MuseScore*' } | Select-Object -First 1
$env:QT_QPA_PLATFORM = 'windows'
$env:QT_QUICK_BACKEND = 'software'
Add-Type -TypeDefinition @'
using System;
using System.Text;
using System.Runtime.InteropServices;
public static class EvanScoreExceptionObserver {
    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)] struct StartupInfo {
        public int cb; public string reserved, desktop, title; public uint x,y,xSize,ySize,xChars,yChars,fill,flags;
        public ushort show,reserved2; public IntPtr reservedPtr,stdin,stdout,stderr;
    }
    [StructLayout(LayoutKind.Sequential)] struct ProcessInfo { public IntPtr process,thread; public uint pid,tid; }
    [DllImport("kernel32.dll",CharSet=CharSet.Unicode,SetLastError=true)] static extern bool CreateProcess(string app,StringBuilder command,IntPtr pa,IntPtr ta,bool inherit,uint flags,IntPtr environment,string cwd,ref StartupInfo si,out ProcessInfo pi);
    [DllImport("kernel32.dll",SetLastError=true)] static extern bool WaitForDebugEventEx(IntPtr ev,uint timeout);
    [DllImport("kernel32.dll")] static extern bool ContinueDebugEvent(uint pid,uint tid,uint status);
    [DllImport("kernel32.dll")] static extern bool ReadProcessMemory(IntPtr process,IntPtr address,byte[] data,IntPtr size,out IntPtr read);
    [DllImport("kernel32.dll")] static extern bool TerminateProcess(IntPtr process,uint code);
    [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr handle);
    [DllImport("kernel32.dll")] static extern IntPtr OpenThread(uint access,bool inherit,uint tid);
    [DllImport("kernel32.dll")] static extern bool GetThreadContext(IntPtr thread,IntPtr context);
    [DllImport("dbghelp.dll",CharSet=CharSet.Unicode,SetLastError=true)] static extern bool SymInitializeW(IntPtr process,string path,bool invade);
    [DllImport("dbghelp.dll",SetLastError=true)] static extern bool SymFromAddr(IntPtr process,ulong address,out ulong displacement,IntPtr symbol);
    static byte[] Read(IntPtr process,ulong address,int size) {
        var data=new byte[size]; IntPtr read;
        if (!ReadProcessMemory(process,new IntPtr(unchecked((long)address)),data,new IntPtr(size),out read)) throw new Exception("Cannot read exception metadata");
        return data;
    }
    static uint U32(IntPtr process,ulong address) { return BitConverter.ToUInt32(Read(process,address,4),0); }
    static ulong U64(IntPtr process,ulong address) { return BitConverter.ToUInt64(Read(process,address,8),0); }
    static string CString(IntPtr process,ulong address) {
        var data=Read(process,address,256); int end=Array.IndexOf(data,(byte)0);
        return Encoding.UTF8.GetString(data,0,end<0?data.Length:end);
    }
    static string Symbol(IntPtr process,ulong address) {
        var buffer=Marshal.AllocHGlobal(512);
        try {
            for(int i=0;i<512;i++)Marshal.WriteByte(buffer,i,0);
            Marshal.WriteInt32(buffer,0,88); Marshal.WriteInt32(buffer,80,400);
            ulong offset;
            if(!SymFromAddr(process,address,out offset,buffer))return null;
            return Marshal.PtrToStringAnsi(IntPtr.Add(buffer,84))+"+0x"+offset.ToString("x");
        } finally { Marshal.FreeHGlobal(buffer); }
    }
    static void Stack(IntPtr process,uint tid) {
        var thread=OpenThread(0x48,false,tid); var context=Marshal.AllocHGlobal(1248);
        try {
            long aligned=(context.ToInt64()+15)&~15L; var ptr=new IntPtr(aligned);
            for(int i=0;i<1232;i++)Marshal.WriteByte(ptr,i,0);
            Marshal.WriteInt32(ptr,48,0x100001);
            if(!GetThreadContext(thread,ptr))return;
            ulong rsp=unchecked((ulong)Marshal.ReadInt64(ptr,152));
            SymInitializeW(process,null,true);
            var bytes=Read(process,rsp,1024);
            Console.WriteLine("Symbolized stack candidates (not an unwound trace):");
            int shown=0;
            for(int i=0;i<bytes.Length && shown<30;i+=8) {
                ulong address=BitConverter.ToUInt64(bytes,i);
                if(address<0x10000)continue;
                var name=Symbol(process,address);
                if(name!=null) {Console.WriteLine("  0x"+address.ToString("x")+" "+name);shown++;}
            }
        } catch(Exception e){Console.WriteLine("Stack observation: "+e.Message);}
        finally {if(thread!=IntPtr.Zero)CloseHandle(thread);Marshal.FreeHGlobal(context);}
    }
    public static void Observe(string app,string cwd) {
        var si=new StartupInfo();si.cb=Marshal.SizeOf(si); ProcessInfo pi;
        if(!CreateProcess(app,new StringBuilder("\""+app+"\" --debug --session-type start-empty"),IntPtr.Zero,IntPtr.Zero,false,2,IntPtr.Zero,cwd,ref si,out pi))throw new Exception("CreateProcess failed: "+Marshal.GetLastWin32Error());
        var ev=Marshal.AllocHGlobal(176);bool breakpointHandled=false;var deadline=DateTime.UtcNow.AddSeconds(45);
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
                        Console.WriteLine("Exception 0x"+exception.ToString("x")+" firstChance="+chance);
                        if(exception==0xe06d7363) {
                            try {
                                ulong obj=unchecked((ulong)Marshal.ReadInt64(ev,56)),info=unchecked((ulong)Marshal.ReadInt64(ev,64)),module=unchecked((ulong)Marshal.ReadInt64(ev,72));
                                ulong types=module+U32(pi.process,info+12),catchType=module+U32(pi.process,types+4),descriptor=module+U32(pi.process,catchType+4);
                                Console.WriteLine("MSVC thrown type: "+CString(pi.process,descriptor+16));
                                try{Console.WriteLine("Exception message: "+CString(pi.process,U64(pi.process,obj+8)));}catch{}
                            } catch(Exception e){Console.WriteLine("Type observation: "+e.Message);}
                            Stack(pi.process,tid);
                        }
                    }
                }
                ContinueDebugEvent(pid,tid,status);
                if(code==5)break;
            }
        } finally {TerminateProcess(pi.process,0);CloseHandle(pi.thread);CloseHandle(pi.process);Marshal.FreeHGlobal(ev);}
    }
}
'@
[EvanScoreExceptionObserver]::Observe($app.FullName,$app.DirectoryName)
