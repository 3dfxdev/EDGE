//----------------------------------------------------------------------------
//  EDGE Win32 Misc System Interface Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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
#include "i_sdlinc.h"

#include "con_main.h"
#include "dm_defs.h"
#include "e_main.h"
#include "g_game.h"
#include "m_argv.h"
#include "m_menu.h"
#include "m_misc.h"
#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"

#include "w32_sysinc.h"

#include "epi/strings.h"
#include "epi/timestamp.h"

#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#define INTOLERANT_MATH 1  // -AJA- FIXME: temp fix to get to compile

extern FILE* debugfile;
extern FILE* logfile;

// has system been setup?
bool systemup = false;

// output string buffer
#define MSGBUFSIZE 4096
static char msgbuf[MSGBUFSIZE];

// ================ INTERNALS =================

//
// FlushMessageQueue
//
// Hacktastic work around for SDL_Quit() 
//
void FlushMessageQueue()
{
	MSG msg;

	while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
	{
		if ( msg.message == WM_QUIT ) break;
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}
}

// ============ END OF INTERNALS ==============

void I_SetupSignalHandlers(bool allow_coredump)
{
	/* nothing needed */
}

void I_CheckAlreadyRunning(void)
{
	HANDLE edgemap;

	// Check we've not already running
	edgemap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READONLY, 0, 8, TITLE);
	if (edgemap)
	{
		DWORD lasterror = GetLastError();
		if (lasterror == ERROR_ALREADY_EXISTS)
		{
			MessageBox(NULL, "EDGE is already running!", TITLE, MB_ICONSTOP|MB_OK);
      exit(9);
		}
	}
}

void I_ChangeToExeDir(const char *argv0)
{
#if 1
	argv0 = _pgmptr;

#elif 1
  // alternate method: assume argv0 is OK

#else
  // yet another way...

  char path[MAX_PATH];

  DWORD path_len = GetModuleFileName(NULL, path, MAX_PATH);
  
  if (path_len <= 0 && path_len >= MAX_PATH)
    return; // FAILED

  argv0 = path;
#endif

	const char *r = strrchr(argv0, '/');

	if (r == NULL || r == argv0)
		return;

	int length = (r - argv0) + 1;

	epi::string_c str;
	str.AddChars(argv0, 0, length);

	chdir(str.GetString());
}

//
// I_SystemStartup
//
// -ACB- 1998/07/11 Reformatted the code.
//
bool I_SystemStartup(void)
{
	systemup = true;

	I_StartupNetwork();

	I_StartupGraphics(); // SDL requires this to be called first
	I_StartupControl();

	// Startup Sound
	if (!M_CheckParm("-nosound"))
	{
		nosound = !(I_StartupSound((void*)NULL));
	}

	// Startup Music System
	if (!I_StartupMusic((void*)NULL))
		I_Warning("%s\n", I_MusicReturnError());

#ifndef INTOLERANT_MATH
	// -ACB- 2000/06/05 This switches off x87 signalling error
	_control87(EM_ZERODIVIDE | EM_INVALID, EM_ZERODIVIDE | EM_INVALID);
#endif /* INTOLERANT_MATH */

	return true;
}

//
// I_DisplayExitScreen
//
void I_DisplayExitScreen(void)
{
}

//
// I_CloseProgram
//
void I_CloseProgram(int exitnum)
{
	exit(exitnum);
}

//
// I_TraceBack
//
// Like I_CloseProgram, but may display some sort of debugging information
// on some systems (typically the function call stack).
//
void I_TraceBack(void)
{
	I_CloseProgram(-1);
}


///---//
///---// returns time in 1/TICRATE second tics
///---//
///---int I_GetTime(void)
///---{
///---	return ticcount;
///---}

//
// I_Warning
//
// -AJA- 1999/09/10: added this.
//
void I_Warning(const char *warning,...)
{
	va_list argptr;

	va_start(argptr, warning);
	vsprintf(msgbuf, warning, argptr);

	I_Printf("WARNING: %s", msgbuf);
	va_end(argptr);
}

//
// I_Error
//
void I_Error(const char *error,...)
{
	va_list argptr;

	va_start(argptr, error);
	vsprintf(msgbuf, error, argptr);
	va_end(argptr);

	ChangeDisplaySettings(0, 0);

	if (logfile)
	{
		fprintf(logfile, "ERROR: %s\n", msgbuf);
		fflush(logfile);
	}

	if (debugfile)
	{
		fprintf(debugfile, "ERROR: %s\n", msgbuf);
		fflush(debugfile);
	}

	I_SystemShutdown();

	//ShowCursor(TRUE);
	FlushMessageQueue();
	MessageBox(NULL, msgbuf, TITLE, MB_OK);

	I_CloseProgram(-1);
	return;
}

//
// I_Printf
//
void I_Printf(const char *message,...)
{
	va_list argptr;
	char *string;
	char printbuf[MSGBUFSIZE];

	// clear the buffer
	memset (printbuf, 0, MSGBUFSIZE);

	string = printbuf;

	va_start(argptr, message);

	// Print the message into a text string
	vsprintf(printbuf, message, argptr);

	L_WriteLog("%s", printbuf);

	// If debuging enabled, print to the debugfile
	L_WriteDebug("%s", printbuf);

	// Clean up \n\r combinations
	while (*string)
	{
		if (*string == '\n')
		{
			memmove(string + 2, string + 1, strlen(string));
			string[1] = '\r';
			string++;
		}
		string++;
	}

	// Send the message to the console.
	CON_Printf(printbuf);

	va_end(argptr);
}

//
// I_PureRandom
//
int I_PureRandom(void)
{
	return ((int)time(NULL) ^ (int)I_ReadMicroSeconds()) & 0x7FFFFFFF;
}

//
// I_ReadMicroSeconds
//
u32_t I_ReadMicroSeconds(void)
{
	return (u32_t) (timeGetTime() * 1000);
}

//
// I_Sleep
//
void I_Sleep(int millisecs)
{
	::Sleep(millisecs);
}

//
// I_GetModifiedTime
//
// Fills in 'timestamp_c' to match the modified time of 'filename'. Returns
// true on success.
//
// -ACB- 2001/06/14
// -ACB- 2004/02/15 Use native win32 functions: they should work!
//
bool I_GetModifiedTime(const char *filename, epi::timestamp_c *t)
{
	SYSTEMTIME timeinf;
	HANDLE handle;
	WIN32_FIND_DATA fdata;

	// Check the sanity of the coders...
	if (!filename || !t)
		return false;

	// Get the file info...
	handle = FindFirstFile(filename, &fdata);
	if (handle == INVALID_HANDLE_VALUE)
		return false;

	// Convert to a time we can actually use...
	if (!FileTimeToSystemTime(&fdata.ftLastWriteTime, &timeinf))
		return false;

	FindClose(handle);

	t->Set( (byte)timeinf.wDay,    (byte)timeinf.wMonth,
			(short)timeinf.wYear,  (byte)timeinf.wHour,
			(byte)timeinf.wMinute, (byte)timeinf.wSecond );

	return true;
}

//
// I_SystemShutdown
//
// -ACB- 1998/07/11 Tidying the code
//
void I_SystemShutdown(void)
{
	int waittime;

	E_EngineShutdown();

	if (!systemup)
		return;

	// Pause to allow sounds to finish
	waittime = timeGetTime() + 2000;
	while (waittime >= (int)timeGetTime());

	I_ShutdownMusic();
	I_ShutdownSound();
	I_ShutdownControl();
	I_ShutdownGraphics();
	I_ShutdownNetwork();

	if (logfile)
	{
		fclose(logfile);
		logfile = NULL;
	}

	// -KM- 1999/01/31 Close the debugfile
#ifdef DEVELOPERS
	if (debugfile != NULL)
		fclose(debugfile);
#endif
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
