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

// DOS GCC
#ifdef DJGPP

typedef long long Int64;
#define FLOAT_IEEE_754
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
#include <assert.h>
#include <errno.h>
#include <sys\stat.h>
#include <dos.h>

#endif

// Borland C++ V5.5 for Win32
#ifdef WIN32
#ifdef __BORLANDC__

#define STRICT
#define _WINDOWS
#define WIN32_LEAN_AND_MEAN

typedef __int64 Int64;
#define FLOAT_IEEE_754

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
#include <assert.h>
#include <errno.h>
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
#define FLOAT_IEEE_754
typedef unsigned char byte;

#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <values.h>
#include <sys/stat.h>

#endif

#endif /*__SYSTEM_SPECIFIC_DEFS__*/
