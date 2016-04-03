//----------------------------------------------------------------------------
//  EDGE2 Zone Memory Allocation Code 
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2015 Isotope SoftWorks
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
//  Note: refactored to use standard C library functions.
//----------------------------------------------------------------------------

#include "i_defs.h"

#include "../epi/utility.h"

#include "z_zone.h"

//
// Z_StrDup
//
// Duplicates the string.
char *Z_StrDup(const char *s)
{        
	int size;
	char *ret;

	if (s == NULL)
		return NULL;

	size = strlen(s) + 1;

	ret = Z_New(char, size);

	Z_MoveData(ret, s, char, size);

	return ret;
}

//
// Z_Free
//
void Z_Free(void *ptr)
{
    free(ptr);
}

//
// Z_ReMalloc
//
// Zone reallocation.
//

void *Z_ReMalloc2(void *ptr, int size)
{
	printf("Z_Realloc(%i)\n", size);

    return realloc(ptr, size);
}

//
// Z_Malloc
//
// Zone allocation.
void *Z_Malloc2(int size)
{
	printf("Z_Malloc(%i)\n", size);
    
    return malloc(size);                          
}

//
// Z_Init
//
void Z_Init(void)
{
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
