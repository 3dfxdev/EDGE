<html>
<body>
<pre>
<h1>Build Log</h1>
<h3>
--------------------Configuration: HawkNL_WCE - Win32 (WCE x86em) Debug--------------------
</h3>
<h3>Command Lines</h3>
Creating temporary file "c:\temp\RSPBC.tmp" with contents
[
/nologo /W3 /Zi /Od /I "include" /D "DEBUG" /D _WIN32_WCE=211 /D "WIN32" /D "STRICT" /D "_WIN32_WCE_EMULATION" /D "INTERNATIONAL" /D "USA" /D "INTLMSG_CODEPAGE" /D "WIN32_PLATFORM_HPCPRO" /D "i486" /D UNDER_CE=211 /D "UNICODE" /D "_UNICODE" /D "_X86_" /D "x86" /D "_USRDLL" /D "HAWKNL_WCE_EXPORTS" /Fp"X86EMDbg/HawkNL_WCE.pch" /YX /Fo"X86EMDbg/" /Fd"X86EMDbg/" /Gz /c 
"D:\MyProjects\HawkNL1.68\src\condition.c"
"D:\MyProjects\HawkNL1.68\src\crc.c"
"D:\MyProjects\HawkNL1.68\src\err.c"
"D:\MyProjects\HawkNL1.68\src\errorstr.c"
"D:\MyProjects\HawkNL1.68\src\group.c"
"D:\MyProjects\HawkNL1.68\src\loopback.c"
"D:\MyProjects\HawkNL1.68\src\mutex.c"
"D:\MyProjects\HawkNL1.68\src\nl.c"
"D:\MyProjects\HawkNL1.68\src\nltime.c"
"D:\MyProjects\HawkNL1.68\src\sock.c"
"D:\MyProjects\HawkNL1.68\src\thread.c"
]
Creating command line "cl.exe @c:\temp\RSPBC.tmp" 
Creating temporary file "c:\temp\RSPBD.tmp" with contents
[
corelibc.lib commctrl.lib coredll.lib winsock.lib /nologo /stack:0x10000,0x1000 /dll /incremental:yes /pdb:"X86EMDbg/HawkNL_WCE.pdb" /debug /nodefaultlib:"OLDNAMES.lib" /nodefaultlib:libc.lib /nodefaultlib:libcd.lib /nodefaultlib:libcmt.lib /nodefaultlib:libcmtd.lib /nodefaultlib:msvcrt.lib /nodefaultlib:msvcrtd.lib /nodefaultlib:oldnames.lib /out:"X86EMDbg/HawkNL_WCE.dll" /implib:"X86EMDbg/HawkNL_WCE.lib" /windowsce:emulation /MACHINE:IX86 
.\X86EMDbg\condition.obj
.\X86EMDbg\crc.obj
.\X86EMDbg\err.obj
.\X86EMDbg\errorstr.obj
.\X86EMDbg\group.obj
.\X86EMDbg\loopback.obj
.\X86EMDbg\mutex.obj
.\X86EMDbg\nl.obj
.\X86EMDbg\nltime.obj
.\X86EMDbg\sock.obj
.\X86EMDbg\thread.obj
]
Creating command line "link.exe @c:\temp\RSPBD.tmp"
<h3>Output Window</h3>
Compiling...
condition.c
crc.c
err.c
errorstr.c
group.c
loopback.c
mutex.c
nl.c
nltime.c
sock.c
thread.c
Linking...
   Creating library X86EMDbg/HawkNL_WCE.lib and object X86EMDbg/HawkNL_WCE.exp



<h3>Results</h3>
HawkNL_WCE.dll - 0 error(s), 0 warning(s)
</pre>
</body>
</html>
