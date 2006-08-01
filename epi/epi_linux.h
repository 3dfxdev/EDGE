//----------------------------------------------------------------------------
//  Linux EPI System Specifics
//----------------------------------------------------------------------------
//
//  Copyright (c) 2003-2005  The EDGE Team.
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
#ifndef __LINUX_EPI_HEADER__
#define __LINUX_EPI_HEADER__

// Sanity checking...
#ifdef __EPI_HEADER_SYSTEM_SPECIFIC__
#error "Two different system specific EPI headers included"
#else
#define __EPI_HEADER_SYSTEM_SPECIFIC__
#endif

#ifndef NULL
#define NULL 0
#endif

#define FLOAT_IEEE_754

typedef long long i64_t;

/*
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <values.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fnmatch.h>
*/

#include <GL/gl.h>

//#define DIRSEPARATOR '/'

#ifndef INLINE
#define INLINE inline
#endif

#define GCCATTR(a) __attribute__((a))
#define EDGE_INLINE(decl, body) extern inline decl body

#endif /* __LINUX_EPI_HEADER__ */
