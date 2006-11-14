//----------------------------------------------------------------------------
//  EDGE Zone Memory Allocation Code 
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
//
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#ifndef __Z_ZONE__
#define __Z_ZONE__

#include "dm_type.h"

#ifndef EPI_MEM_USAGE

#define ZONEID  0x1d4a11f1

typedef enum
{
	Z_UrgencyNone    = 0,
	Z_UrgencyLow     = 1,
	Z_UrgencyMedium  = 2,
	Z_UrgencyHigh    = 3,
	Z_UrgencyExtreme = 4
}
z_urgency_e;

// A cache flusher is a function that can find and free unused memory.
typedef void cache_flusher_f(z_urgency_e urge);

// Generic helper functions.
char *Z_StrDup(const char *s);

// Memory handling functions.
void Z_RegisterCacheFlusher(cache_flusher_f *f);
void Z_Init(void);
void *Z_Calloc2(int size);
void *Z_Malloc2(int size);
void *Z_ReMalloc2(void *ptr, int size);
void Z_Free(void *ptr);

// -ES- 1999/12/16 Leak Hunt
#ifdef LEAK_HUNT
void *Z_RegisterMalloc(void *ptr, const char *file, int row);
void *Z_UnRegisterTmpMalloc(void *ptr, const char *func);
#define Z_Malloc(size) Z_RegisterMalloc(Z_Malloc2(size),__FILE__,__LINE__)
#define Z_Calloc(size) Z_RegisterMalloc(Z_Calloc2(size),__FILE__,__LINE__)
#define Z_ReMalloc(ptr,size) Z_RegisterMalloc(Z_ReMalloc2(ptr,size),__FILE__,__LINE__)
#else
#define Z_Malloc Z_Malloc2
#define Z_Calloc Z_Calloc2
#define Z_ReMalloc Z_ReMalloc2
#endif

//
// Z_New
//
// Allocates num elements of type. Use this instead of Z_Malloc whenever
// possible.
//
#define Z_New(type, num) ((type *) Z_Malloc((num) * sizeof(type)))

//
// Z_Resize
//
// Reallocates a block. Use instead of Z_ReMalloc wherever possible.
// Unlike normal Z_ReMalloc, the pointer parameter is assigned the new
// value, and there is no return value.
//
#define Z_Resize(ptr,type,n)  \
	(void)((ptr) = (type *) Z_ReMalloc((void *)(ptr), (n) * sizeof(type)))

//
// Z_ClearNew
//
// The Z_Calloc version of Z_New. Allocates mem and clears it to zero.
//
#define Z_ClearNew(type, num) ((type *) Z_Calloc((num) * sizeof(type)))

//
// Z_Clear
//
// Clears memory to zero.
//
#define Z_Clear(ptr, type, num)  \
	memset((void *)(ptr), ((ptr) - ((type *)(ptr))), (num) * sizeof(type))

//
// Z_MoveData
//
// moves data from src to dest.
//
#define Z_MoveData(dest, src, type, num)  \
	I_MoveData((void *)(dest), (void *)(src), (num) * sizeof(type) + ((src) - (type *)(src)) + ((dest) - (type *)(dest)))

//
// StrNCpy
//
// Copies up to max characters of src into dest, and then applies a
// terminating zero (so dest must hold at least max+1 characters).
// The terminating zero is always applied (there is no reason not to)
//
#define Z_StrNCpy(dest, src, max) \
	(void)(strncpy((dest), (src), (max)), (dest)[(max)] = 0)

// -AJA- 2001/07/24: New lightweight "Bunches"

#define Z_Bunch(type)  \
	struct { type *arr; int max; int num; }

#define Z_BunchNewSize(var, type)  \
do \
{  \
	if ((var).num > (var).max)  \
	{								\
		(var).max = (var).num + 16;				\
		Z_Resize((var).arr, type, (var).max);	\
	} \
} \
while(0) \

#endif	/* EPI_MEM_USAGE */

#endif  /* __Z_ZONE__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
