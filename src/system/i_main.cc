//----------------------------------------------------------------------------
//  EDGE Main
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2018  The EDGE Team.
// 
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//----------------------------------------------------------------------------

#include "i_defs.h"
#include "i_sdlinc.h"  // needed for proper SDL main linkage

#include "../../epi/exe_path.h"

#include "../dm_defs.h"
#include "../m_argv.h"
#include "../e_main.h"
#include "../version.h"

#ifdef WIN32
#include <windows.h>
#include "win32/w32_sysinc.h"
#ifdef _MSC_VER
extern int __cdecl I_W32ExceptionHandler(PEXCEPTION_POINTERS ep);
#endif
#endif

const char *win32_exe_path = ".";

//#undef HYPERTENSION

#ifdef WIN32
// (C) James Haley (From Eternity Engine)
// I_TweakConsole 
//
// Disable the Win32 console window's close button and set its title.
//
static void I_TweakConsole()
{
#if _WIN32_WINNT > 0x500
   HWND hwnd = GetConsoleWindow();

   if(hwnd)
   {
      HMENU hMenu = GetSystemMenu(hwnd, FALSE);
      DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
   }
   SetConsoleTitle("EDGE Engine System Console");
#endif
}
#endif

int main(int argc, char *argv[])
{
	// FIXME: setup argument handler NOW
	bool allow_coredump = false;
	for (int i = 1; i < argc; i++)
		if (strcmp(argv[i], "-core") == 0)
			allow_coredump = true;

    I_SetupSignalHandlers(allow_coredump);

#ifdef WIN32
	// -AJA- give us a proper name in the Task Manager
	SDL_RegisterApp(TITLE, 0, 0);
	I_TweakConsole();
#endif

    I_CheckAlreadyRunning();

#ifdef WIN32
    // -AJA- change current dir to match executable
	win32_exe_path = epi::GetExecutablePath(argv[0]);

    ::SetCurrentDirectory(win32_exe_path);
#else
    // -ACB- 2005/11/26 We don't do on LINUX since we assume the 
    //                  executable is globally installed
#endif

#ifdef DREAMCAST
	const char *a[]={"dreamedge","-width","640","-height","480","-bpp","16","-fullscreen","-smoothing","-nomipmap","-nomusic",
#ifdef NO_SOUND
		"-nosound",
#endif
	};

#ifdef DEBUG_DREAMEDGE
	I_Printf("GDB_INIT:\n");
	gdb_init();
#endif
	
printf("DREAMCAST STARTUP\n");
fflush(stdout);
	E_Main(12,a);
#else

#if defined(HYPERTENSION)
	system("compile_hod_pk3.bat");
#endif  

#if defined(DAIKATANA)
	system("compile_daikatana2_pk3.bat");
#endif 

	fflush(stdout);

#if defined(_DEBUG) && defined(_MSC_VER)
	// Uncomment this line to make the Visual C++ CRT check the heap before
	// every allocation and deallocation. This will be slow, but it can be a
	// great help in finding problem areas.

	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF);

	// Enable leak checking at exit.
	//_CrtSetDbgFlag(_CrtSetDbgFlag(0) | _CRTDBG_LEAK_CHECK_DF);

	// Use this to break at a specific allocation number.
	//_crtBreakAlloc = 53039;
#endif  
#endif // 0

#ifdef _MSC_VER
	// Run EDGE. it never returns
	_try
	{
		//I_TweakConsole();
		E_Main(argc, (const char**)argv);
	}
	__except (I_W32ExceptionHandler(GetExceptionInformation()))
	{
		I_Error("Exception caught in main: see CRASHLOG.TXT for info\n");
	}
#else
	try
	{
		E_Main(argc, (const char **)argv);
		//common_main(argc, argv);
	}
	catch (int e) 
	{
		I_Error("Exception caught in main: see CRASHLOG.TXT for info\n");
	}
#endif


	return 0;

}

#if 0

#if !defined(_DEBUG) && defined (WIN32)
int main(int argc, char **argv)
{
#ifdef WIN32
	// -AJA- give us a proper name in the Task Manager
	SDL_RegisterApp(TITLE, 0, 0);
	I_TweakConsole();
#endif

	I_CheckAlreadyRunning();

	win32_exe_path = epi::GetExecutablePath(argv[0]);

	::SetCurrentDirectory(win32_exe_path);

	__try
	{
		I_TweakConsole();
		E_Main(argc, (const char **)argv);
		//common_main(argc, argv);
	}
	__except (I_W32ExceptionHandler(GetExceptionInformation()))
	{
		I_Error("Exception caught in main: see CRASHLOG.TXT for info\n");
	}

	return 0;
}
#endif    
#endif // 0







//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
