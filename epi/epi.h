//----------------------------------------------------------------------------
//  EDGE Platform Interface Header
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2008  The EDGE Team.
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
#include "asserts.h"
//#include "epi/physfs/physfs.h"

#endif /*__SYSTEM_SPECIFIC_DEFS__*/

#ifdef LINUX
#define HAVE_PHYSFS 1
#include <physfs.h>
#include "epi_linux.h"
#endif

#if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
#define HAVE_PHYSFS 1
#include <physfs.h>
#include "epi_win32.h"
#endif

#ifdef __APPLE__
#define HAVE_PHYSFS 1
#include <physfs.h>
#include "epi_macosx.h"
#endif

#ifdef BSD
#define HAVE_PHYSFS 1
#include <physfs.h>
#include "epi_macosx.h"
#endif

#ifdef DREAMCAST
#include "epi_dreamcast.h"
#endif

#ifdef VITA
#define HAVE_PHYSFS 1
#include "epi_linux.h"
#include "epi_vita.h"
#endif



// if we can't use C++11 or aren't using VS2015, resort to gross hacks
#if __cplusplus < 201103L && (!defined(_MSC_VER) || _MSC_VER < 1900)
#define nullptr NULL
#endif

#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#else
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop) )
#endif


namespace epi
{
	// Base Functions
	bool Init(void);
	void Shutdown(void);
};

/* Important functions provided by Engine code */

void I_Error(const char *error,...) GCCATTR((format(printf, 1, 2)));
void I_Warning(const char *warning,...) GCCATTR((format(printf, 1, 2)));
void I_Printf(const char *message,...) GCCATTR((format(printf, 1, 2)));
void I_Debugf(const char *message,...) GCCATTR((format(printf, 1, 2)));

/* __EDGE_PLATFORM_INTERFACE__ */
//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
