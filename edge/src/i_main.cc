//----------------------------------------------------------------------------
//  EDGE Main
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
#include "i_sdlinc.h"  // needed for proper SDL main linkage

#ifdef WIN32
#include "w32_sysinc.h"  // for <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dm_defs.h"
#include "m_argv.h"
#include "e_main.h"

#include "epi/strings.h"


#ifndef WIN32
#include <unistd.h>
#include <signal.h>

// cleanup handling -- killough:

static void I_SignalHandler(int s)
{
	// CPhipps - report but don't crash on SIGPIPE
	if (s == SIGPIPE)
	{
		// -AJA- linux signals reset when raised.
		signal(SIGPIPE, I_SignalHandler);

		fprintf(stderr, "EDGE: Broken pipe\n");
		return;
	}

	signal(s, SIG_IGN);    // Ignore future instances of this signal.

	switch (s)
	{
		case SIGSEGV: I_Error("EDGE: Segmentation Violation"); break;
		case SIGINT:  I_Error("EDGE: Interrupted by User"); break;
		case SIGILL:  I_Error("EDGE: Illegal Instruction"); break;
		case SIGFPE:  I_Error("EDGE: Floating Point Exception"); break;
		case SIGTERM: I_Error("EDGE: Killed"); break;
	}

	I_Error("EDGE: Terminated by signal %d", s);
}

void ChangeToExeDir(const char *full_path)
{
	const char *r = strrchr(full_path, '/');

	if (r == NULL || r == full_path)
		return;

#ifdef MACOSX
        // -AJA- It seems argv[0] points directly to the "gledge" binary
        //       inside of the Edge.app folder (when run from the Finder).
        //       Hence we need to strip the extra bits off.
        const char *app = r - 4;

        for (; app > full_path; app--)
        {
            if (app[0] == '.' && app[1] == 'a' && app[2] == 'p' &&
                app[3] == 'p' && app[4] == '/')
              break;
        }
        if (app > full_path)
        {
          for (; app > full_path; app--)
            if (app[0] == '/')
              break;
        }
        if (app > full_path)
          r = app;
#endif
	int length = (r - full_path) + 1;

	epi::string_c str;
	str.AddChars(full_path, 0, length);

	chdir(str.GetString());
}

static void SetupSignalHandlers()
{
	signal(SIGPIPE, I_SignalHandler); // CPhipps - add SIGPIPE, as this is fatal

#ifdef DEVELOPERS

	// -AJA- Disable signal handlers, otherwise we don't get core dumps
	//       and core dumps are _DAMN_ useful for debugging.

#else
	signal(SIGSEGV, I_SignalHandler);
	signal(SIGTERM, I_SignalHandler);
	signal(SIGILL,  I_SignalHandler);
	signal(SIGFPE,  I_SignalHandler);
	signal(SIGILL,  I_SignalHandler);
	signal(SIGINT,  I_SignalHandler);  // killough 3/6/98: allow CTRL-BRK during init
	signal(SIGABRT, I_SignalHandler);
#endif
}

static void CheckAlreadyRunning()
{ }

#else // WIN32

static void SetupSignalHandlers()
{ }

static void CheckAlreadyRunning()
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

void ChangeToExeDir(const char *full_path)
{
  // FIXME !!!!!: use either _pgmptr OR GetModuleFileName(NULL)

	const char *r = strrchr(full_path, '\\');

	if (r == NULL || r == full_path)
		return;

	int length = (r - full_path);

	epi::string_c str;
	str.AddChars(full_path, 0, length);

	SetCurrentDirectory(str.GetString());
}

#endif

extern "C" {

#ifdef MACOSX
int main(int argc, char *argv[])
#else
int main(int argc, char **argv)
#endif
{
    SetupSignalHandlers(); 
    CheckAlreadyRunning();

    // -ACB- 2005/11/26 We don't do on LINUX since we assume the 
    //                  executable is globally installed
#ifndef LINUX
	// -AJA- change current dir to match executable (just like Win32)
    ChangeToExeDir(argv[0]);
#endif

	// Run EDGE. it never returns
	engine::Main(argc, (const char **) argv);

	return 0;
}

} // extern "C"


// Application active?
int app_state = APP_STATE_ACTIVE;

//
// I_Loop
//
void I_Loop(void)
{
	bool gameon = true;
	while (gameon)
	{
		// We always do this once here, although the engine may makes in own
		// calls to keep on top of the event processing
		I_ControlGetEvents(); 
		
		if (app_state & APP_STATE_ACTIVE)
			engine::Tick();
			
		if (app_state & APP_STATE_PENDING_QUIT) // Engine may have set this 
			gameon = false; 
	}

	return;
}
