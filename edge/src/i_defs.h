//----------------------------------------------------------------------------
//  EDGE System Specific Header
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2001  The EDGE Team.
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

// NULL port
// Template for new ports. It is as close to ANSI C as possible, and comments
// have been added to show what code you have to add to make it work.
#if 0

//typedef long long Int64;

//
// Define USE_INT64 if the system supports 64 bit integers.
//#define USE_INT64
//
typedef float float_t;

//
// define FLOAT_IEEE_754 if the float is IEEE_754 compliant, i.e. 1 bit
// sign + 8 bits exponent + 23 bits mantissa.
//
//#define FLOAT_IEEE_754
typedef enum { false, true } boolean_t;

//
// Add any system includes here, e.g.
// #include <stdio.h>
//

#define EDGECONFIGFILE "EDGE.CFG"
#define EDGEHOMEDIR    "EDGE"
#define EDGEWADDIR     "EDGEWADDIR"
#define EDGEWADEXT     "wad"
#define EDGEGWAEXT     "gwa"
#define REQUIREDWAD    "EDGE"

//
// This is the default directory separator. It is '\\' in DOS, and '/' on
// most other systems.
//
#define DIRSEPARATOR '/'

// If GCC is used, this one is defined as __attribute__((a)).
#define GCCATTR(a)

//
// INLINE can be defined to a keyword which hints the compiler to inline a
// function.
//
#define INLINE

// See m_inline.h for usage. This is the ANSI C compliant definition.
#define EDGE_INLINE(decl, body) extern decl;

//
// TmpMalloc is a memory allocater which can be used for temporary allocations.
// Memory allocated with I_TmpMalloc has to be freed with I_TmpFree before the
// procedure containing the I_TmpMalloc call exits. Its advantage is that it
// gets much faster on all systems that support alloca.
//
#define I_TmpMalloc(size) Z_Malloc(size)
#define I_TmpFree(ptr) Z_Free(ptr)

// If memmove is not optimal on your system, you can use your own I_MoveData,
// which should have exactly the same type as memcpy, and should declared here.
#define I_MoveData memmove

// include headers to compensate for missing standard functions.
#include ".\null\i_compen.h"
#include "i_system.h"

#endif // NULL port

// DOS GCC
#ifdef DJGPP

typedef long long Int64;
#define USE_INT64
typedef float float_t;
#define FLOAT_IEEE_754
typedef enum { false, true } boolean_t;

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
#include <sys\stat.h>

#define EDGECONFIGFILE "EDGE.CFG"
#define EDGEHOMEDIR    "EDGE"
#define EDGEWADDIR     "EDGEWADDIR"
#define EDGEWADEXT     "wad"
#define EDGEGWAEXT     "gwa"
#define REQUIREDWAD    "EDGE"

#define DIRSEPARATOR '\\'

#define GCCATTR(a) __attribute__((a))
#define INLINE inline
#define EDGE_INLINE(decl, body) extern decl ; extern inline decl body

//
// TmpMalloc is a memory allocater which can be used for temporary allocations.
// Memory allocated with TmpMalloc has to be freed with TmpFree before the
// procedure containing the TmpMalloc call exits. Its advantage is that it
// gets much faster on all systems that support alloca.
//
#define I_TmpMalloc(size) alloca(size)
#define I_TmpFree(ptr) do { } while (0)

#define I_MoveData memmove

#include ".\djgpp\i_compen.h"
#include "i_system.h"

#endif

// Microsoft Visual C++ V6.0 for Win32
#ifdef WIN32 
#ifdef _GATESY_

#define WIN32_LEAN_AND_MEAN

typedef __int64 Int64;
typedef float float_t;
#define FLOAT_IEEE_754

typedef enum { false, true } boolean_t;

#include <windows.h>

#include <ctype.h>
#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include <limits.h>
#include <math.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys\stat.h>
#include <time.h>

#include <gl/gl.h>

#define EDGECONFIGFILE "EDGE.CFG"
#define EDGEHOMEDIR    "EDGE"
#define EDGEWADDIR     "EDGEWADDIR"
#define EDGEWADEXT     "wad"
#define EDGEGWAEXT     "gwa"
#define REQUIREDWAD    "EDGE"

#define DIRSEPARATOR '\\'

#define NAME        "The EDGE Engine"
#define OUTPUTNAME  "EDGECONSOLE"
#define TITLE       "EDGE Engine"
#define OUTPUTTITLE "EDGE System Console"

// Access() define values. Nicked from DJGPP's <unistd.h>
#define R_OK    0x02
#define W_OK    0x04

// PI define. Nicked from DJGPP's <math.h>
#define M_PI 3.14159265358979323846

#define GCCATTR(a)
#define INLINE _inline 
#define EDGE_INLINE(decl, body) extern decl;

#define I_TmpMalloc(size) alloca(size)
#define I_TmpFree(ptr) do { } while (0)

#define MAXPATH _MAX_PATH

#define I_MoveData memmove

#include ".\win32\i_compen.h"
#include "i_system.h"

#endif
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

#include <gl/gl.h>

#define EDGECONFIGFILE "EDGE.CFG"
#define EDGEHOMEDIR    "EDGE"
#define EDGEWADDIR     "EDGEWADDIR"
#define EDGEWADEXT     "wad"
#define EDGEGWAEXT     "gwa"
#define REQUIREDWAD    "EDGE"

#define DIRSEPARATOR '\\'

#define NAME        "EDGE"
#define OUTPUTNAME  "EDGECONSOLE"
#define TITLE       "EDGE Engine"
#define OUTPUTTITLE "EDGE System Console"

// Access() define values. Nicked from DJGPP's <unistd.h>
#define R_OK    0x02
#define W_OK    0x04

// PI define. Nicked from DJGPP's <math.h>
#define M_PI 3.14159265358979323846

#define GCCATTR(a)
#define INLINE 
#define EDGE_INLINE(decl, body) extern decl;

#define I_TmpMalloc(size) alloca(size)
#define I_TmpFree(ptr) do { } while (0)

#define I_MoveData memmove

#include ".\win32\i_compen.h"
#include "i_system.h"

#endif
#endif

// LINUX GCC
#ifdef LINUX

typedef long long Int64;
typedef float float_t;
#define FLOAT_IEEE_754
typedef enum { false, true } boolean_t;

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


#ifdef USE_GL
#include <GL/gl.h>
#endif

#define EDGECONFIGFILE "edge.cfg"
#define EDGEHOMEDIR    "HOME"
#define EDGEWADDIR     "EDGEWADDIR"
#define EDGEWADEXT     "wad"
#define EDGEGWAEXT     "gwa"
#define REQUIREDWAD    "edge"

#define DIRSEPARATOR '/'

#define NAME        "EDGE"
#define OUTPUTNAME  "EDGECONSOLE"
#define TITLE       "EDGE Engine"
#define OUTPUTTITLE "EDGE Engine console"

#define GCCATTR(a) __attribute__((a))
#define INLINE inline
#define EDGE_INLINE(decl, body) extern inline decl body

#define I_TmpMalloc(size) alloca(size)
#define I_TmpFree(ptr) do { } while (0)

#define I_MoveData memmove

#include "i_system.h"
#include "linux/i_compen.h"

#endif // LINUX GCC

// BeOS GCC
#ifdef BeOS

typedef long long Int64;
typedef float float_t;
#define FLOAT_IEEE_754
typedef enum { false, true } boolean_t;

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
#include <sys/stat.h>


#ifdef USE_GL
#include <GL/gl.h>
#endif

#define EDGECONFIGFILE "edge.cfg"
#define EDGEHOMEDIR    "HOME"
#define EDGEWADDIR     "EDGEWADDIR"
#define EDGEWADEXT     "wad"
#define EDGEGWAEXT     "gwa"
#define REQUIREDWAD    "edge"

#define DIRSEPARATOR '/'

#define NAME        "EDGE"
#define OUTPUTNAME  "EDGECONSOLE"
#define TITLE       "EDGE Engine"
#define OUTPUTTITLE "EDGE Engine console"

#define GCCATTR(a) __attribute__((a))
#define INLINE inline
#define EDGE_INLINE(decl, body) extern inline decl body

#define I_TmpMalloc(size) alloca(size)
#define I_TmpFree(ptr) do { } while (0)

#define I_MoveData memmove

#include "i_system.h"
#include "beos/b_compen.h"

#endif // BeOS GCC

#ifdef LEAK_HUNT

#undef I_TmpMalloc
#undef I_TmpFree

#ifdef __GNUC__

//
// use __FUNCTION__: This will check that the corresponding Z_TmpFree is done
// within the function.
//
#define I_TmpMalloc(size) Z_RegisterMalloc(Z_Malloc2(size), __FUNCTION__, __LINE__)
#define I_TmpFree(ptr) Z_Free(Z_UnRegisterTmpMalloc(ptr, __FUNCTION__))

#else

//
// Can not use __FUNCTION__, so we can only check whether it is freed within
// the same file
//
#define I_TmpMalloc(size) Z_RegisterMalloc(Z_Malloc2(size), __FILE__, __LINE__)
#define I_TmpFree(ptr) Z_Free(Z_UnRegisterTmpMalloc(ptr, __FILE__))

#endif

#endif

#endif /*__SYSTEM_SPECIFIC_DEFS__*/


