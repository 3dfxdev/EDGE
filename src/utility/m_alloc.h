//----------------------------------------------------------------------------
//  EDGE Malloc Wrapper
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2020  The EDGE Team.
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
//  Based on the ZDoom source code:
//
//    Copyright 2005-2008 Randy Heit
//
//----------------------------------------------------------------------------


#ifndef __M_ALLOC_H__
#define __M_ALLOC_H__

#include <stdlib.h>
#include "zdoomsupport.h"

// These are the same as the same stdlib functions,
// except they bomb out with a fatal error
// when they can't get the memory.

#if defined(_DEBUG)
#define M_Malloc(s)		M_Malloc_Dbg(s, __FILE__, __LINE__)
#define M_Realloc(p,s)	M_Realloc_Dbg(p, s, __FILE__, __LINE__)

void *M_Malloc_Dbg (size_t size, const char *file, int lineno);
void *M_Realloc_Dbg (void *memblock, size_t size, const char *file, int lineno);
#else
void *M_Malloc (size_t size);
void *M_Realloc (void *memblock, size_t size);
#endif

void M_Free (void *memblock);

#endif //__M_ALLOC_H__
