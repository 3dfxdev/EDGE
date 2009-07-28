//------------------------------------------------------------------------
//  MEMORY CRUD
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
//  Copyright (C) 1997-2003 Andr� Majorel et al
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
//------------------------------------------------------------------------
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Rapha�l Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#include "main.h"


/*
   allocate memory with error checking
*/

void *GetMemory (unsigned long size)
{
	void *ret = malloc ((size_t) size);

	if (!ret)
		FatalError("out of memory (cannot allocate %u bytes)", size);

	return ret;
}


/*
   reallocate memory with error checking
   */

void *ResizeMemory (void *old, unsigned long size)
{
	void *ret = realloc (old, (size_t) size);

	if (!ret)
		FatalError("out of memory (cannot reallocate %lu bytes)", size);

	return ret;
}


/*
   free memory
   */

void FreeMemory (void *ptr)
{
	/* just a wrapper around free(), but provide an entry point */
	/* for memory debugging routines... */
	free (ptr);
}



/* end of file */
//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
