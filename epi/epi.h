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

// 
#endif /*__SYSTEM_SPECIFIC_DEFS__*/

#ifdef LINUX
#include "epi_linux.h"
#endif

#ifdef WIN32
#include "epi_win32.h"
#endif

#ifdef MACOSX
#include "epi_macosx.h"
#endif

#ifdef BSD
#include "epi_macosx.h"
#endif

#ifdef DREAMCAST
#include "epi_dreamcast.h"
#endif

// if we can't use C++11 or aren't using VS2015, resort to gross hacks
#if __cplusplus < 201103L && (!defined(_MSC_VER) || _MSC_VER < 1900)
#define nullptr NULL
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

/// `CA- 6.5.2016: quick hacks to change these in Visual Studio (less warnings). 
#ifdef _MSC_VER
#define strdup _strdup
#define stricmp _stricmp
#define strnicmp _strnicmp
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#define uint unsigned int
#define ALIGN_8(x)							__declspec(align(8)) x
#define ALIGN_16(x)							__declspec(align(16)) x
#define ALIGN_32(x)							__declspec(align(32)) x
#pragma warning( disable : 4099)
#elif defined MACOSX
typedef unsigned int uint;
#endif
// Template for LINUX like MSVC 
#elif defined LINUX 
#define ALIGN_STRUCT(x)    __attribute__((aligned(x)))
//#endif



#endif /* __EDGE_PLATFORM_INTERFACE__ */
//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
