//----------------------------------------------------------------------------
//  EDGE Win32 Misc System Interface Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2001  The EDGE Team.
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

#include "..\con_main.h"
#include "..\dm_defs.h"
#include "..\e_main.h"
#include "..\e_net.h"
#include "..\g_game.h"
#include "..\m_argv.h"
#include "..\m_menu.h"
#include "..\m_misc.h"
#include "..\r_draw1.h"
#include "..\r_draw2.h"
#include "..\s_sound.h"
#include "..\w_wad.h"
#include "..\z_zone.h"

#include "i_sysinc.h"

// Application active?
boolean_t appactive;

// has system been setup?
boolean_t systemup = false;

// output string buffer
#define MSGBUFSIZE 4096
static char msgbuf[MSGBUFSIZE];

// This annoys me....
extern boolean_t demorecording;

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
// SysTicker
//
// System Ticker Routine
//
void CALLBACK SysTicker(UINT id, UINT msg, DWORD user, DWORD dw1, DWORD dw2)
{
	if (appactive)
	{
		actualticcount++;

		if (!(actualticcount%ENGINE_TIMER_TICS))
			ticcount++;
	}

	I_MP3Ticker();         // Called to handle MP3 Code
	I_MUSTicker();         // Called to handle MUS Code
	I_ControlTicker();     // Called to buffer input for reading

	return;
}

//
// HandleFocusChange
//
static void HandleFocusChange(HWND window, HWND otherwin, boolean_t gotfocus)
{
	if (window == mainwindow && otherwin != mainwindow)
	{
		if (gotfocus)
		{
			while (ShowCursor(FALSE) >= 0) {}; // Remove the cursor
			appactive = true;
			I_HandleKeypress(KEYD_PAUSE, false);
		}
		else
		{
			I_HandleKeypress(KEYD_PAUSE, false);
			InvalidateRect(conwinhandle, NULL, FALSE);
			SendMessage(conwinhandle, WM_PAINT, 0, 0);
			ShowCursor(TRUE);
			appactive = false;
		}
	}
}

// ============ END OF INTERNALS ==============

//
// I_SystemStartup
//
// -ACB- 1998/07/11 Reformatted the code.
//
boolean_t I_SystemStartup(void)
{
	HRESULT result;
	int clockspeed;

	systemup = true;

	// Startup system clock
	timeBeginPeriod(TIMER_RES);
	clockspeed = 1000 / ACTUAL_TIMER_HZ;
	timerID = timeSetEvent(clockspeed, TIMER_RES, SysTicker, 0, TIME_PERIODIC);

	I_StartupControl();

	// Startup Sound
	if (!M_CheckParm("-nosound"))
	{
		nosound = !(I_StartupSound((void*)&mainwindow));
		if (nosound)
			I_Warning("%s\n", I_SoundReturnError());
	}

	// Startup Music System
	if (!I_StartupMusic((void*)&mainwindow))
		I_Warning("%s\n", I_MusicReturnError());

	I_StartupGraphics();

#ifndef DEVELOPERS

#ifndef INTOLERANT_MATH
	// -ACB- 2000/06/05 This switches off x87 signalling error
	_control87(EM_ZERODIVIDE | EM_INVALID, EM_ZERODIVIDE | EM_INVALID);
#endif /* INTOLERANT_MATH */

#endif /* DEVELOPERS */

	return true;
}

//
// I_WaitVBL
//
void I_WaitVBL(int count)
{
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

	if (debugfile)
	{
		fprintf(debugfile, "ERROR: %s\n", msgbuf);
		fflush(debugfile);
	}

	ShowCursor(TRUE);
	MessageBox(mainwindow, msgbuf, TITLE, MB_OK);

	I_SystemShutdown();
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

	// And the text screen if in text mode
	if (!graphicsmode)
		I_WinConPrintf(printbuf);

	va_end(argptr);
}

//
// I_PutTitle
//
// This basically inits the Win32 System Console for init an sets
// the main window's title (which isn't yet viewable).
//
void I_PutTitle(const char *title)
{
	I_StartWinConsole();
	SetWindowText(mainwindow, title); // Set EDGE Engine Window with this title
	I_SetConsoleTitle(NULL);
}

//
// I_PathIsAbsolute
//
// Returns true if the path should be treated as an absolute path.
// -ES- 2000/01/01 Written.
//
boolean_t I_PathIsAbsolute(const char *path)
{
	if (path[0] == '\\' || path[0] == '.' || (path[0] <= 'z' && path[0] >= 'A' && path[1] == ':'))
		return true;
	else
		return false;
}

//
// I_PathIsDirectory
//
boolean_t I_PathIsDirectory(const char *path)
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
	int len = strlen(path);
	char *s;

	if (len == 0)
	{
		// empty string means ".\"
		return Z_StrDup(".");
	}

	if (path[0] >= 'A' && path[0] <= 'z' && path[1] == ':' && len == 2)
	{
		// special case: "c:" turns into "c:."
		s = Z_Malloc(4);
		s[0] = path[0];
		s[1] = path[1];
		s[2] = '.';
		s[3] = 0;
		return s;
	}

	if (path[len - 1] == '\\')
	{
		// cut off the last separator
		s = Z_Malloc(len);
		memcpy(s, path, len - 1);
		s[len - 1] = 0;

		return s;
	}

	return Z_StrDup(path);
}

//
// I_Access
//
boolean_t I_Access(const char *filename)
{
	int handle = open(filename, O_RDONLY | O_BINARY);

	if (handle == -1)
		return false;

	close(handle);

	return true;
}

//
// I_WindowProc
//
// The Main Window Message Handling Procedure
//
long FAR PASCAL I_WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//  DWORD actflag;

	switch (message)
	{
		case WM_CLOSE:
			return 1L;

		case WM_KEYDOWN:
			// We don't want to know about a key still being pressed.
			if ((lParam & 0x40000000) != 0) 
				break;                        

			if (wParam == VK_PAUSE)
				I_HandleKeypress(KEYD_PAUSE, true);

			return 0L;

		case WM_KEYUP:
			if (wParam == VK_PAUSE)
				I_HandleKeypress(KEYD_PAUSE, false);

			return 0L;

		case WM_KILLFOCUS:
			HandleFocusChange(hWnd, (HWND)(wParam), false);
			return 0L;

		case WM_SETFOCUS:
			HandleFocusChange(hWnd, (HWND)(wParam), true);
			return 0L;

		case WM_SIZE:
			if (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED) 
			{
				I_SizeWindow();
				return 0L;
			}
			break;

		default:
			break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

//
// I_EDGELoop
//
// Win32 needs to handle messages during its loop...
//
void I_EDGELoop(void)
{
	MSG msg;
	static boolean_t gameon = true;

	while (gameon)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				gameon = false;
				appactive = false;
				break;
			}

			if (!TranslateAccelerator(msg.hwnd, accelerator, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		if (appactive)
			E_EDGELoopRoutine();
	}

	return;
}

//
// I_PureRandom
//
long I_PureRandom(void)
{
	return time(NULL);
}

//
// I_ReadMicroSeconds
//
unsigned long I_ReadMicroSeconds(void)
{
	return timeGetTime()*1000;
}

//
// I_GetModifiedTime
//
// Fills in 'i_time_t' to match the modified time of 'filename'. Returns
// true on success.
//
// -ACB- 2001/06/14
//
boolean_t I_GetModifiedTime(const char *filename, i_time_t *t)
{
	struct _stat buf;
	struct tm *timeinf;

	// Check the sanity of the coders...
	if (!filename)
		return false;

	if (!t)
		return false;

	memset(t,0,sizeof(i_time_t));

#if 1  // FIXME: -AJA- don't know why, but it ain't working
	return true;
#endif

	// Check the file is invalid
	if (_stat(filename, &buf))
		return false;

	// Convert the 'time_t' of the modified time into something more human
	timeinf = localtime(&buf.st_mtime);
	if (!timeinf)
		return false;

	t->secs    = timeinf->tm_sec;
	t->minutes = timeinf->tm_min;
	t->hours   = timeinf->tm_hour;
	t->day     = timeinf->tm_mday;
	t->month   = timeinf->tm_mon+1;
	t->year    = timeinf->tm_year+1900;

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
	{
		I_ShutdownWinConsole();
		return;
	}

	// Pause to allow sounds to finish
	waittime = timeGetTime() + 2000;
	while (waittime >= (int)timeGetTime());

	I_ShutdownGraphics();
	I_ShutdownMusic();
	I_ShutdownSound();
	I_ShutdownControl();

	// Kill timer
	timeKillEvent(timerID);
	timeEndPeriod(TIMER_RES);

	// -KM- 1999/01/31 Close the debugfile
#ifdef DEVELOPERS
	if (debugfile != NULL)
		fclose(debugfile);
#endif

	I_ShutdownWinConsole();
}







