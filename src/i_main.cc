//----------------------------------------------------------------------------
//  EDGE Main
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2006  The EDGE Team.
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

#include "epi/strings.h"
#include "epi/exe_path.h"

#include "dm_defs.h"
#include "m_argv.h"
#include "e_main.h"
#include "version.h"

const char *exe_path;

extern "C" {

int I_Main(int argc, char *argv[])
{
	// FIXME: setup argument handler NOW
	bool allow_coredump = false;
	for (int i = 1; i < argc; i++)
		if (strcmp(argv[i], "-core") == 0)
			allow_coredump = true;

    I_SetupSignalHandlers(allow_coredump);

#ifndef LINUX
	// -AJA- give us a proper name in the Task Manager
	SDL_RegisterApp(TITLE, 0, 0);
#endif

    I_CheckAlreadyRunning();

    // -AJA- change current dir to match executable
	exe_path = epi::GetExecutablePath(argv[0]);

#ifdef WIN32
    ::SetCurrentDirectory(exe_path);
#else
    // -ACB- 2005/11/26 We don't do on LINUX since we assume the 
    //                  executable is globally installed
#endif

	// Run EDGE. it never returns
	E_Main(argc, (const char **) argv);

	return 0;
}

} // extern "C"


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
