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
//

#include "..\i_defs.h"
#include "..\SDL\i_sdlinc.h"

#include "..\con_main.h"
#include "..\dm_defs.h"
#include "..\e_main.h"
#include "..\g_game.h"
#include "..\m_argv.h"
#include "..\m_menu.h"
#include "..\m_misc.h"
#include "..\s_sound.h"
#include "..\w_wad.h"
#include "..\z_zone.h"

#include "i_sysinc.h"

// has system been setup?
bool systemup = false;

// output string buffer
#define MSGBUFSIZE 4096
static char msgbuf[MSGBUFSIZE];

// MicroTimer
unsigned long microtimer_granularity = 1000;

// Timer Control
#define ACTUAL_TIMER_HZ   140
#define ENGINE_TIMER_TICS 4             // In comparision with about timer...
#define TIMER_RES 7                     // Timer resolution.
static int timerID;                     // Timer ID
static int ticcount = 0;                // Engine tick count
static int actualticcount = 0;          // Actual tick count

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

//
// SysTicker
//
// System Ticker Routine
//
void CALLBACK SysTicker(UINT id, UINT msg, DWORD user, DWORD dw1, DWORD dw2)
{
	if (app_state & APP_STATE_ACTIVE)
	{
		actualticcount++;

		if (!(actualticcount%ENGINE_TIMER_TICS))
			ticcount++;
	}

	I_MUSTicker();         // Called to handle MUS Code
	return;
}

// ============ END OF INTERNALS ==============

//
// I_SystemStartup
//
// -ACB- 1998/07/11 Reformatted the code.
//
bool I_SystemStartup(void)
{
	int clockspeed;

	systemup = true;

	// Startup system clock
	timeBeginPeriod(TIMER_RES);
	clockspeed = 1000 / ACTUAL_TIMER_HZ;
	timerID = timeSetEvent(clockspeed, TIMER_RES, SysTicker, 0, TIME_PERIODIC);

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

#ifndef DEVELOPERS

#ifndef INTOLERANT_MATH
	// -ACB- 2000/06/05 This switches off x87 signalling error
	_control87(EM_ZERODIVIDE | EM_INVALID, EM_ZERODIVIDE | EM_INVALID);
#endif /* INTOLERANT_MATH */

#endif /* DEVELOPERS */

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

//
// I_GetTime
//
// returns time in 1/TICRATE second tics
//
int I_GetTime(void)
{
	return ticcount;
}

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
// I_PathIsAbsolute
//
// Returns true if the path should be treated as an absolute path.
// -ES- 2000/01/01 Written.
//
bool I_PathIsAbsolute(const char *path)
{
	if (path[0] == '\\' || path[0] == '.' || (path[0] <= 'z' && path[0] >= 'A' && path[1] == ':'))
		return true;
	else
		return false;
}

//
// I_PathIsDirectory
//
bool I_PathIsDirectory(const char *path)
{
	char curpath[MAXPATH];
	int result;

	getcwd(curpath, MAXPATH);

	result = chdir(path);

	chdir(curpath);

	if (result == -1)
		return false;

	return true;
}

//
// I_PreparePath
//
// Prepares the end of the path name, so it will be possible to concatenate
// a DIRSEPARATOR and a file name to it.
//
// Allocates and returns the new string.
//
char *I_PreparePath(const char *path)
{
	int len = (int)strlen(path);
	char *s;

	if (len == 0)
	{
		// empty string means ".\"
		return Z_StrDup(".");
	}

	if (path[0] >= 'A' && path[0] <= 'z' && path[1] == ':' && len == 2)
	{
		// special case: "c:" turns into "c:."
		s = (char*)Z_Malloc(4);
		s[0] = path[0];
		s[1] = path[1];
		s[2] = '.';
		s[3] = 0;
		return s;
	}

	if (path[len - 1] == '\\')
	{
		// cut off the last separator
		s = (char*)Z_Malloc(len);
		memcpy(s, path, len - 1);
		s[len - 1] = 0;

		return s;
	}

	return Z_StrDup(path);
}

//
// I_Access
//
bool I_Access(const char *filename)
{
	int handle = open(filename, O_RDONLY | O_BINARY);

	if (handle == -1)
		return false;

	close(handle);

	return true;
}

//
// I_PureRandom
//
long I_PureRandom(void)
{
	return (long)time(NULL) ^ (long)I_ReadMicroSeconds();
}

//
// I_ReadMicroSeconds
//
unsigned long I_ReadMicroSeconds(void)
{
	return timeGetTime()*1000;
}

//
// I_Sleep
//
void I_Sleep(unsigned long millisecs)
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

	// Kill timer
	timeKillEvent(timerID);
	timeEndPeriod(TIMER_RES);

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






