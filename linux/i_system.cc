//----------------------------------------------------------------------------
//  EDGE Linux Misc System Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
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

#include <stdio.h>

#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <ctype.h>

#include "./i_sysinc.h"
#include "i_defs.h"

#include "version.h"
#include "con_main.h"
#include "dm_state.h"
#include "e_main.h"
#include "g_game.h"
#include "m_argv.h"
#include "m_misc.h"
#include "m_random.h"
#include "r_draw1.h"
#include "r_draw2.h"
#include "w_wad.h"
#include "z_zone.h"

#ifdef USE_FLTK

// remove some problematic #defines
#undef VISIBLE
#undef INVISIBLE

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/fl_ask.H>

#endif // USE_FLTK

static char cp437_to_ascii[160] =
{ 
	'.', '.', '.', '.', '.', '.', '.', '.',   // 0x00 - 0x07
	'.', '.', '.', '.', '.', '.', '.', '.',   // 0x08 - 0x0F
	'>', '<', '.', '.', '.', '.', '.', '.',   // 0x10 - 0x17
	'.', '.', '.', '<', '.', '.', 'A', 'V',   // 0x18 - 0x1F

	'.', '.', '.', '.', '.', '.', '.', '.',   // 0x80 - 0x87
	'.', '.', '.', '.', '.', '.', '.', '.',   // 0x88 - 0x8F
	'.', '.', '.', '.', '.', '.', '.', '.',   // 0x90 - 0x97
	'.', '.', '.', '.', '.', '.', '.', '.',   // 0x98 - 0x9F
	'.', '.', '.', '.', '.', '.', '.', '.',   // 0xA0 - 0xA7
	'.', '.', '.', '.', '.', '.', '.', '.',   // 0xA8 - 0xAF

	'.', '%', '.', '|', '+', '+', '+', '.',   // 0xB0 - 0xB7
	'.', '+', '|', '+', '+', '.', '.', '+',   // 0xB8 - 0xBF
	'+', '+', '+', '+', '-', '+', '+', '|',   // 0xC0 - 0xC7
	'+', '+', '+', '+', '+', '-', '+', '-',   // 0xC8 - 0xCF
	'+', '-', '.', '.', '.', '.', '.', '+',   // 0xD0 - 0xD7
	'+', '+', '+', '.', '.', '.', '.', '.',   // 0xD8 - 0xDF

	'.', '.', '.', '.', '.', '.', '.', '.',   // 0xE0 - 0xE7
	'.', '.', '.', '.', '.', '.', '.', '.',   // 0xE8 - 0xEF
	'.', '.', '>', '<', '.', '.', '.', '.',   // 0xF0 - 0xF7
	'.', '.', '.', '.', '.', '.', '.', '.'    // 0xF8 - 0xFF
};

unsigned long microtimer_granularity = 1000000;


void I_RemoveGrab(void);  // in SDL/i_video.cpp

void I_WaitVBL (int count)
{
}

// Most of the following has been rewritten by Lee Killough
// and then by CPhipps
//
// I_GetTime
//

// CPhipps - believe it or not, it is possible with consecutive calls to 
// gettimeofday to receive times out of order, e.g you query the time twice and 
// the second time is earlier than the first. Cheap'n'cheerful fix here.
// NOTE: only occurs with bad kernel drivers loaded, e.g. pc speaker drv

unsigned long I_GetMicroSec (void)
{
	struct timeval tv;
	struct timezone tz;
	gettimeofday (&tv, &tz);
	return (tv.tv_sec * 1000000 + tv.tv_usec);
}

static unsigned long lasttimereply;

int I_GetTime (void)
{
	struct timeval tv;
	struct timezone tz;
	static unsigned long basetime = 0;
	unsigned long thistimereply;

	gettimeofday (&tv, &tz);

	// Fix for time problem
	thistimereply = (tv.tv_sec * TICRATE + (tv.tv_usec * TICRATE) / 1000000);

	if (!basetime)
		basetime = thistimereply;
	thistimereply -= basetime;

	if (thistimereply < lasttimereply)
		thistimereply = lasttimereply;

	return (lasttimereply = thistimereply);
}


extern int autorun;  // Autorun state


bool microtimer_installed = 1;


static char errmsg[4096];  // buffer of error message -- killough

// killough 2/22/98: Add support for ENDBOOM, which is PC-specific

// this converts BIOS color codes to ANSI codes.  Its not pretty, but it
// does the job - rain
// CPhipps - made static

static inline int convert_colour(int colour, int *bold)
{
	*bold = 0;

	if (colour > 7)
	{
		colour &= 7;
		*bold = 1;
	}

	switch (colour)
	{
		case 1: return 4;
		case 3: return 6;
		case 4: return 1;
		case 6: return 3;
	}

	return colour;
}

// CPhipps - flags controlling ENDOOM behaviour
enum
{
	endoom_colours = 1,
	endoom_nonasciichars = 2,
	endoom_droplastline = 4
};
unsigned int endoom_mode;

//
// I_Warning
//
void I_Warning(const char *warning,...)
{
	va_list argptr;

	va_start (argptr, warning);
	vsprintf (errmsg, warning, argptr);
	va_end (argptr);

	I_Printf ("WARNING: %s", errmsg);
}

//
// I_Error
//
void I_Error (const char *error, ...)
{
	va_list argptr;

	va_start (argptr, error);
	vsprintf (errmsg, error, argptr);
	va_end (argptr);

#ifndef USE_FLTK
	fprintf (stderr, "%s\n", errmsg);
#endif

	if (logfile)
	{
		fprintf(logfile, "ERROR: %s\n", errmsg);
		fflush(logfile);
	}

	if (debugfile)
	{
		fprintf(debugfile, "ERROR: %s\n", errmsg);
		fflush(debugfile);
	}

#ifdef USE_FLTK
	I_SystemShutdown();
	I_MessageBox(errmsg, "EDGE Error", 0);

	exit(-1);
#endif

	//
	// -AJA- Commit suicide, thereby producing a core dump which may
	//       well come in handy for debugging the code that called
	//       I_Error().
	//
#ifdef DEVELOPERS
	I_RemoveGrab();
	raise(11);
	/* NOTREACHED */
#endif

	I_SystemShutdown();
	exit (-1);
}

// -AJA- Routine which emulates IBM charset.
static void PrintString(char *str)
{
	for (; *str; str++)
	{
		int ch = (unsigned char) *str;

		if (ch == 0x7F || ch == '\r')
			continue;

		if ((0x20 <= ch && ch <= 0x7E) ||
				ch == '\n' || ch == '\t')
		{
			putchar(ch);
			continue;
		}

		if (ch >= 0x80)
			ch -= 0x60;

		putchar(cp437_to_ascii[ch]);
	}

	fflush(stdout);
}

void I_Printf (const char *message,...)
{
	va_list argptr;
	char printbuf[2048];
	char *string = printbuf;

	va_start (argptr, message);

	// Print the message into a text string
	vsprintf (printbuf, message, argptr);

	L_WriteLog("%s", printbuf);

	// If debuging enabled, print to the debugfile
	L_WriteDebug("%s", printbuf);

	// Clean up \n\r combinations
	while (*string)
	{
		if (*string == '\n')
		{
			memmove (string + 2, string + 1, strlen (string));
			string[1] = '\r';
			string++;
		}
		string++;
	}

	// Send the message to the console.
	CON_Printf (printbuf);

	// And the text screen if in text mode
#ifndef USE_FLTK
	if (!graphicsmode)
	{
		PrintString(printbuf);
	}
#endif

	va_end(argptr);
}

void TextAttr (int attr)
{
	// Not supported in Linux without low-level termios manipulation
	// or ncurses, which I'd rather not link
	// textattr(attr);
}

void ClearScreen (void)
{  
	I_Printf("\n");
}

// -KM- 1998/10/29 Use all of screen, not just first 25 rows.
void I_PutTitle(const char *title)
{
	char string[81];
	int centre;

	memset(string, ' ', 80);
	string[80] = 0;

	// Build the title
	centre = (80 - strlen(title)) / 2;
	memcpy(&string[centre], title, strlen(title));

	// Print the title
	TextAttr(0x07);
	ClearScreen();
	I_Printf("%s\n\n", string);
	TextAttr(0x07);
}

//
// I_DisplayExitScreen
//
void I_DisplayExitScreen(void)
{
	/* not implemented */
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
void I_TraceBack(void)
{
	I_CloseProgram(-1);
}

//
// I_SystemStartup
//
// -ACB- 1998/07/11 Reformatted the code.
//
bool I_SystemStartup(void)
{
	I_StartupControl();
	I_StartupGraphics();
	I_StartupSound(NULL);    // -ACB- 1999/09/20 Sets nosound directly
	I_StartupMusic(NULL);

	return true;
}

//
// I_SystemShutdown
//
// -ACB- 1998/07/11 Tidying the code
//
void I_SystemShutdown(void)
{
	E_EngineShutdown();

	I_ShutdownMusic();
	I_ShutdownSound();
	I_ShutdownGraphics();
	I_ShutdownControl();

	if (logfile)
	{
		fclose(logfile);
		logfile = NULL;
	}

	// -KM- 1999/01/31 Close the debugfile
#ifdef DEVELOPERS
	if (debugfile)
		fclose(debugfile);
#endif
}

//
// I_PathIsAbsolute
//
// Returns true if the path should be treated as an absolute path.
//
// -ES- 2000/01/01 Written.
//
bool I_PathIsAbsolute(const char *path)
{
	if (path[0] == '/' || path[0] == '.')
		return true;
	else
		return false;
}

//
// I_PreparePath
//
// Prepares the end of the path name, so it will be possible to concatenate
// a DIRSEPARATOR and a file name to it.
// Allocates and returns the new string.
//
char *I_PreparePath(const char *path)
{
	int len = strlen(path);
	char *s;

	if (len == 0)
	{
		// empty string means "./"
		return Z_StrDup(".");
	}

	if (path[len - 1] == '/')
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
// I_PureRandom
//
// Returns as-random-as-possible 32 bit values.
//
long I_PureRandom(void)
{
	// FIXME: use /dev/random

	return time(NULL);
}

//
// I_ReadMicroSeconds
//
unsigned long I_ReadMicroSeconds(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	// FIXME: this is probably wrong...

	return tv.tv_usec;
}

//
// I_PathIsDirectory
//
bool I_PathIsDirectory(const char *path)
{
	struct stat buf;

	if (stat(path, &buf) != 0)
		return false;

	return S_ISDIR(buf.st_mode);
}

//
// I_Access
//
bool I_Access(const char *filename)
{
	return (access(filename, R_OK) == 0) ? true : false;
}


//
// I_GetModifiedTime
//
// -ACB- 2001/06/14
//
bool I_GetModifiedTime(const char *filename, i_time_t *t)
{
	struct stat buf;
	struct tm timeinf;

	// Check the sanity of the coders...
	if (!filename)
		return false;

	if (!t)
		return false;

	memset(t,0,sizeof(i_time_t));

	// Check the file is invalid
	if (stat(filename, &buf))			
		return false;

	// Convert the 'time_t' of the modified time into something more human
	if(!localtime_r(&buf.st_mtime, &timeinf))
		return false;

	t->secs    = timeinf.tm_sec;
	t->minutes = timeinf.tm_min;
	t->hours   = timeinf.tm_hour;
	t->day     = timeinf.tm_mday;
	t->month   = timeinf.tm_mon+1;
	t->year    = timeinf.tm_year+1900;

	return true;
}

//
// I_Loop
//
void I_Loop(void)
{
	while (1)
		engine::Tick();
}

//
// I_MessageBox
//
void I_MessageBox(const char *message, const char *title, int mode)
{
#ifdef USE_FLTK
	Fl::scheme(NULL);
	fl_message_font(FL_HELVETICA_BOLD, 18);	
	fl_message("%s", message);
#else // USE_FLTK
	fprintf(stderr, "%s", message);
#endif // USE_FLTK
}

