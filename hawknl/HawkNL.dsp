# Microsoft Developer Studio Project File - Name="HawkNL" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=HawkNL - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "HawkNL.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "HawkNL.mak" CFG="HawkNL - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "HawkNL - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "HawkNL - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "HawkNL - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "HawkNL__"
# PROP BASE Intermediate_Dir "HawkNL__"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "src\Release"
# PROP Intermediate_Dir "src\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W4 /GX /O2 /I "include\\" /I "src\win32\\" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 pthreadVCE.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /version:1.6 /subsystem:windows /dll /map /machine:I386 /out:"lib\HawkNL.dll" /libpath:"src\win32\\"
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE=$(InputPath)
PostBuild_Cmds=copy                                        src\Release\*.lib \
                                                          lib\                                 	copy \
                                                      src\win32\*.dll                                                 lib\ 
# End Special Build Tool

!ELSEIF  "$(CFG)" == "HawkNL - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "HawkNL_0"
# PROP BASE Intermediate_Dir "HawkNL_0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "src\Debug"
# PROP Intermediate_Dir "src\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W4 /Gm /GX /Zi /Od /I "include\\" /I "src\win32\\" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 pthreadVCE.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /version:0.5 /subsystem:windows /dll /map /debug /machine:I386 /out:"lib\HawkNL.dll" /pdbtype:sept /libpath:"src\win32\\"
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE=$(InputPath)
PostBuild_Cmds=copy                                        src\Debug\*.lib \
                                                         lib\                                  	copy \
                                                      src\win32\*.dll                                                lib\ 
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "HawkNL - Win32 Release"
# Name "HawkNL - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter ".c"
# Begin Source File

SOURCE=.\src\condition.c
# End Source File
# Begin Source File

SOURCE=.\src\crc.c
# ADD CPP /Za
# End Source File
# Begin Source File

SOURCE=.\src\err.c
# End Source File
# Begin Source File

SOURCE=.\src\errorstr.c

!IF  "$(CFG)" == "HawkNL - Win32 Release"

# ADD CPP /Ze

!ELSEIF  "$(CFG)" == "HawkNL - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\group.c
# End Source File
# Begin Source File

SOURCE=.\src\ipx.c
# End Source File
# Begin Source File

SOURCE=.\src\loopback.c
# End Source File
# Begin Source File

SOURCE=.\src\mutex.c
# End Source File
# Begin Source File

SOURCE=.\src\nl.c

!IF  "$(CFG)" == "HawkNL - Win32 Release"

# ADD CPP /Ze

!ELSEIF  "$(CFG)" == "HawkNL - Win32 Debug"

# PROP Intermediate_Dir "src\Debug"
# ADD CPP /Ze

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\nltime.c
# End Source File
# Begin Source File

SOURCE=.\src\sock.c
# End Source File
# Begin Source File

SOURCE=.\src\thread.c
# End Source File
# End Group
# Begin Group "Include Files"

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=.\src\ipx.h
# End Source File
# Begin Source File

SOURCE=.\src\loopback.h
# End Source File
# Begin Source File

SOURCE=.\include\nl.h
# End Source File
# Begin Source File

SOURCE=.\src\nlinternal.h
# End Source File
# Begin Source File

SOURCE=.\src\parallel.h
# End Source File
# Begin Source File

SOURCE=.\src\serial.h
# End Source File
# Begin Source File

SOURCE=.\src\sock.h
# End Source File
# Begin Source File

SOURCE=.\src\wsock.h
# End Source File
# End Group
# Begin Group "Doc Files"

# PROP Default_Filter ".txt"
# Begin Source File

SOURCE=.\src\NLchanges.txt
# End Source File
# Begin Source File

SOURCE=.\src\readme.txt
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Source File

SOURCE=.\src\hawknl.def
# End Source File
# End Target
# End Project
