//----------------------------------------------------------------------------
//  Win32 EDGE System Specifics
//----------------------------------------------------------------------------
//
//  Copyright (c) 2003-2007  The EDGE Team.
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


// GNU C
#ifdef __GNUC__

#ifndef STRICT
#define STRICT
#endif 

#define _WINDOWS
#define WIN32_LEAN_AND_MEAN


#define DIRSEPARATOR '\\'

#define GCCATTR(xyz) __attribute__ (xyz)

#endif /* __GNUC__ */


// Microsoft Visual C++ for Win32
#ifdef _GATESY_ 

#define WIN32_LEAN_AND_MEAN


#include <windows.h>

///---#include <ctype.h>
///---#include <direct.h>
///---#include <fcntl.h>
///---#include <float.h>
///---#include <io.h>
///---#include <limits.h>
///---#include <math.h>
///---#include <malloc.h>
///---#include <stdarg.h>
///---#include <stdio.h>
///---#include <stdlib.h>
///---#include <string.h>
///---#include <time.h>
///---#include <sys\stat.h>
///---#include <time.h>
///---
///---#include <gl/gl.h>

#define DIRSEPARATOR '\\'

#define GCCATTR(xyz)  /* nothing */

#define MAXPATH _MAX_PATH


#define M_PI 3.14159265358979323846

#pragma warning( disable : 4290 )

#endif /* __GATESY__ */

#endif /* __WIN32_EPI_HEADER__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
