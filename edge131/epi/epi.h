//----------------------------------------------------------------------------
//  EDGE Platform Interface Header
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

#ifndef __EDGE_PLATFORM_INTERFACE__
#define __EDGE_PLATFORM_INTERFACE__

#include "headers.h"
#include "types.h"
#include "macros.h"

#ifdef LINUX
#include "epi_linux.h"
#endif

#ifdef WIN32
#include "epi_win32.h"
#endif

#ifdef MACOSX
#include "epi_macosx.h"
#endif

namespace epi
{
	// Forward Declarations
	class filesystem_c;
	class mem_manager_c;

	// Base Functions
	bool Init(void);
	void Shutdown(void);

	// External declarations
	extern filesystem_c* the_filesystem;
	extern mem_manager_c* the_mem_manager;
};

#endif /* __EDGE_PLATFORM_INTERFACE__ */
