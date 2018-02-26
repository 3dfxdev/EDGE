//----------------------------------------------------------------------------
//  EDGE Zone Memory Allocation Code 
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2008  The EDGE Team.
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

#include "i_defs.h"

#include "epi/utility.h"

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
#ifdef DEVELOPERS
typedef struct mallocheader_s
{
	int size;
} mallocheader_t;

#define CHECK_PTR(h) SYS_ASSERT_MSG((*(int *)((char *)(h) + (h)->size + sizeof(mallocheader_t)) == ZONEID), ("Block without ZONEID"))
#endif


void Z_Free(void *ptr)
{
	if (ptr == NULL)
		return;
#ifdef DEVELOPERS
	{
		mallocheader_t *h = (mallocheader_t *)ptr - 1;
		CHECK_PTR(h);
#ifdef INTOLERANT_MATH
		// -ES- 2000/04/08 Trash all data.
		memset(h, -1, h->size + sizeof(mallocheader_t) + sizeof(int));
#endif
		free(h);
	}
#else
	free(ptr);
#endif
}



void *Z_Malloc(int size)
{
#ifdef DEVELOPERS
	mallocheader_t *p;
#else
	void *p;
#endif
	int allocsize;

	if (size == 0)
		return NULL;

#ifdef DEVELOPERS
	allocsize = sizeof(mallocheader_t) + size + sizeof(int);
#else
	allocsize = size;
#endif

#ifdef DEVELOPERS
	while (NULL == (p = (mallocheader_t*)malloc(allocsize)))
#else
	while (NULL == (p = malloc(allocsize)))
#endif
	{
		I_Error("Z_Malloc: failed on allocation of %i bytes", size);
	}
#ifdef DEVELOPERS
	p->size = size;
	p++;
	*(int *)((char *)p + size) = ZONEID;
#ifdef INTOLERANT_MATH
	// -ES- 2000/03/28 Will turn all floats into -NaN, making uninitialised
	// elements easier to find.
	memset(p, -1, size);
#endif
#endif
	return p;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
