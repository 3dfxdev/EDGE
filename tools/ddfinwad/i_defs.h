//----------------------------------------------------------------------------
//  EDGE System Specific Header
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2000  The EDGE Team.
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
// -ACB- 1999/09/19 Written
//

#ifndef __SYSTEM_SPECIFIC_DEFS__
#define __SYSTEM_SPECIFIC_DEFS__

// DOS GCC
#ifdef DJGPP

typedef long long Int64;
typedef float float_t;
#define FLOAT_IEEE_754
typedef enum { false, true } boolean_t;
typedef unsigned char byte;

#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <values.h>
#include <sys\stat.h>

#endif

// Borland C++ V5.5 for Win32
#ifdef WIN32
#ifdef __BORLANDC__

#define STRICT
#define _WINDOWS
#define WIN32_LEAN_AND_MEAN

typedef __int64 Int64;
typedef float float_t;
#define FLOAT_IEEE_754

typedef enum { false, true } boolean_t;
typedef unsigned char byte;

#include <windows.h>

#include <ctype.h>
#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include <limits.h>
#include <math.h>
#include <malloc.h>
#include <mem.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys\stat.h>
#include <time.h>

// Access() define values. Nicked from DJGPP's <unistd.h>
#define R_OK    0x02
#define W_OK    0x04

#endif
#endif

// LINUX GCC
#ifdef LINUX

typedef long long Int64;
typedef float float_t;
#define FLOAT_IEEE_754
typedef enum { false, true } boolean_t;
typedef unsigned char byte;

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

#endif

#endif /*__SYSTEM_SPECIFIC_DEFS__*/


