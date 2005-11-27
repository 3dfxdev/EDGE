//----------------------------------------------------------------------------
//  EDGE Win32 Main Interface Functions
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
// -ACB- 1999/04/02
//
#include "..\i_defs.h"

#include "..\m_argv.h"
#include "..\e_main.h"

#include "i_sysinc.h"

#include <epi/strings.h>
#include <SDL.h>

// -ACB- 2003/10/05 We need these outside the function, so we can delete them on exit
static const char **edgeargv = NULL;
static int edgeargc = 0;

//
// strtok_quote
//
// -AJA- 2003/10/11: like strtok() but handles double quotes (").
//
// This is to allow filenames with spaces.  Examples:
//
//      "C:\Program Files\Foo"
//      @"\Temp Stuff\RESPONSE.TXT"
//      My" "Wads\Good" "Time.wad
//
static char *strtok_quote(char *str, const char *sep)
{
	static char *cur_pos = NULL;

	if (str)
	{
		cur_pos = str;

		// ignore leading spaces
		while (*cur_pos && strchr(sep, *cur_pos) != NULL)
			cur_pos++;
	}

	if (*cur_pos == 0)
		return NULL;

	char *result = cur_pos;
	char *dest   = cur_pos;

	bool quoting = false;

	// look for an unquoted space, removing quotes as we go
	while (*cur_pos && (quoting || strchr(sep, *cur_pos) == NULL))
	{
		if (*cur_pos == '"')
		{
			quoting = ! quoting;
			cur_pos++;
			continue;
		}

		*dest++ = *cur_pos++;
	}

	if (dest < cur_pos)
		*dest = 0;

	// remove trailing spaces from argument
	while (*cur_pos && strchr(sep, *cur_pos) != NULL)
		*cur_pos++ = 0;
	
	return result;
}

//
// ParseParameters
//
// Parse the command line for parameters and
// give them in a form M_InitArguments
//
static void ParseParameters(void)
{
	char *s;
	char *p;
	char *cmdline;

	cmdline = GetCommandLine();

	// allow for 32 parameters
	edgeargv = new const char*[32];
	if (!edgeargv)
		I_Error("ParseParameters: FAILED ON PARAMETER POINTER MALLOC");

	edgeargc = 0;
	s = strtok_quote(cmdline, " ");
	while (s != NULL)
	{
		p = new char[strlen(s)+1];
		strcpy(p, s);
		edgeargv[edgeargc] = p;
		edgeargc++;
		s = strtok_quote(NULL, " ");

		// grow 32 elements at a time
		if ((edgeargc & 31) == 0)
		{
			const char **oldargv;

			oldargv = edgeargv;

			edgeargv = new const char*[edgeargc+32];
			if (!edgeargv)
					I_Error("ParseParameters: FAILED ON PARAMETER POINTER REALLOC");

			memcpy(edgeargv, oldargv, sizeof(char*)*edgeargc-1);
			delete [] oldargv;
		}
	}
}

//
// CleanupParameters
//
static void CleanupParameters(void)
{
	int i;

	if (edgeargc && edgeargv)
	{
		// Cleanup the remaining elements
		for (i=0; i<edgeargc; i++)
		{
			delete [] edgeargv[i];
		}
		delete [] edgeargv;
	}
}

//
// ChangeToExeDir
//
void ChangeToExeDir(const char *full_path)
{
	const char *r = strrchr(full_path, '\\');

	if (r == NULL || r == full_path)
		return;

	int length = (r - full_path);

	epi::string_c str;
	str.AddChars(full_path, 0, length);

	SetCurrentDirectory(str.GetString());
}

//
// WinMain
//
// Life starts here....
//
int PASCAL WinMain (HINSTANCE curr, HINSTANCE prev, LPSTR cmdline, int show)
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
			return -1;
		}
	}

	// sort command line
	ParseParameters();

	//
	// -AJA- workaround for drag-and-drop (Windows sets our current
	//       directory to the windows directory C:\WINDOWS).
	//
	if (edgeargc >= 1)
		ChangeToExeDir(edgeargv[0]);

	// Load SDL dynamic link library
	if ( SDL_Init(SDL_INIT_NOPARACHUTE) < 0 ) {
		MessageBox(NULL, SDL_GetError(), TITLE, MB_ICONSTOP|MB_OK);
		return -1;
	}

	SDL_SetModuleHandle(GetModuleHandle(NULL));

	// Run Game....
	engine::Main(edgeargc, edgeargv);

	// Cleanup on exit
	CleanupParameters();

	return 0;
}
