//----------------------------------------------------------------------------
//  System Specific Header
//----------------------------------------------------------------------------
// 
//  DEH_EDGE  Copyright (C) 2004  The EDGE Team
// 
//  This program is under the GNU General Public License.
//  It comes WITHOUT ANY WARRANTY of any kind.
//  See COPYING.txt for the full details.
//
//------------------------------------------------------------------------

#ifndef __SYSTEM_SPECIFIC_DEFS__
#define __SYSTEM_SPECIFIC_DEFS__

// COMMON STUFF...
#define FLOAT_IEEE_754
typedef unsigned char byte;

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <limits.h>
#include <values.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>

// DOS GCC
#if defined(DJGPP)

typedef long long Int64;

#include <unistd.h>
#include <dos.h>

// Windows
#elif defined(WIN32)

#define STRICT
#define _WINDOWS
#define WIN32_LEAN_AND_MEAN

#ifdef __GNUC__
typedef long long Int64;
#else
typedef __int64 Int64;
#endif

#include <windows.h>

#include <io.h>
#include <malloc.h>
#include <mem.h>

// Access() define values. Nicked from DJGPP's <unistd.h>
#ifndef R_OK
#define R_OK    0x02
#define W_OK    0x04
#endif

// LINUX
#elif defined(LINUX)

#include <unistd.h>

typedef long long Int64;

// MacOS X
#elif defined (MACOSX)

#include <unistd.h>

typedef long long Int64;

#else
#error Unknown System (not DJGPP, WIN32, LINUX or MACOSX)
#endif

#endif /*__SYSTEM_SPECIFIC_DEFS__*/
