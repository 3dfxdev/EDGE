//----------------------------------------------------------------------------
//  Windows EDGE System Specifics
//----------------------------------------------------------------------------
//
//  Copyright (c) 2003-2018  The EDGE Team.
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
#ifndef __WIN32_EPI_HEADER__
#define __WIN32_EPI_HEADER__

// Sanity checking...
#ifdef __EPI_HEADER_SYSTEM_SPECIFIC__
#error "Two different system specific EPI headers included"
#else
#define __EPI_HEADER_SYSTEM_SPECIFIC__
#endif

#if 0 // needed??
#define R_OK    0x02
#define W_OK    0x04
#endif

#ifndef MAXPATH
#define MAXPATH  _MAX_PATH
#endif

#define DIRSEPARATOR  '\\'

#ifdef __GNUC__
#define GCCATTR(xyz) __attribute__ (xyz)
#else
#define GCCATTR(xyz)  /* nothing */
#endif

#pragma warning( disable : 4290 )
#pragma warning( disable : 4005 ) // This is to disable the macro redefinition of _WINDOWS under Visual Studio
#pragma warning( disable : 4099)

// Win-specific definition stuff:
#define strdup _strdup
#define stricmp _stricmp
#define strnicmp _strnicmp
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#define strupr _strupr
#define uint unsigned int
#define ALIGN_8(x)							__declspec(align(8)) x
#define ALIGN_16(x)							__declspec(align(16)) x
#define ALIGN_32(x)							__declspec(align(32)) x

#define _WINDOWS
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#ifndef WIN32
#define WIN32
#endif

#ifdef _MSC_VER
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

#include <windows.h>


#endif /*__WIN32_EPI_HEADER__*/

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
