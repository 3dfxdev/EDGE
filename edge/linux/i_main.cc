//----------------------------------------------------------------------------
//  EDGE Linux Main
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
//
// DESCRIPTION:
//      Main program, simply calls E_Main high level loop.
//

#ifdef MACOSX
#include <SDL.h>  // needed for proper SDL main linkage
#endif

#include "i_defs.h"

#include "dm_defs.h"
#include "m_argv.h"
#include "e_main.h"
#include "z_zone.h"

extern const char system_string[];

void I_PreInitGraphics (void);

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef USE_GLUT
#include <GL/glut.h>
#endif

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

#ifdef MACOSX
int main(int argc, char *argv[])
#else
int main(int argc, const char **argv)
#endif
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

#ifdef USE_GLUT
	// Intentional Const Override
	glutInit(&argc, (char **) argv);
#endif

	// Run EDGE. it never returns
	engine::Main(argc, (const char **) argv);

	return 0;
}
