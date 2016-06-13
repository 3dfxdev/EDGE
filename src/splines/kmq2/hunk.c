//----------------------------------------------------------------------------
//  EDGE2 Hunk Allocator
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2015 Isotope SoftWorks and Contributors.
//  Based on the Quake 3: Arena source, (C) id Software.
//
//  Uses code from KMQuake II, (C) Mark "KnightMare" Shan.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int		hunkcount;
char		*membase;
int		hunkmaxsize;
int		cursize;

void *Hunk_Begin (int maxsize)
{
	cursize = 0;
	hunkmaxsize = maxsize;
	membase = malloc (maxsize);
	if (!membase) {
		printf("VirtualAlloc reserve failed");
		fflush(stdout);
	}
	memset (membase, 0, maxsize);	// bitshifter- moved here where pointer is known

	return (void *)membase;
}

void *Hunk_Alloc_Aligned (int size, int alignment)
{
	if ((alignment-1)&alignment) {
		printf("Hunk_Alloc_Aligned: alignment isn't power of 2 (asked for %i)\n",alignment);
		fflush(stdout);
	}
	alignment--;
	// round to cacheline
	//size = (size+alignment) & ~alignment;
	cursize = (cursize+alignment) & ~alignment;

	cursize += size;
	if (cursize > hunkmaxsize) {
		printf("Hunk_Alloc overflow Cur: %i, Max: %i, Req: %i\n",cursize, hunkmaxsize, size);
		fflush(stdout);
	}

	return (void *)(membase+cursize-size);
}

void *Hunk_Alloc (int size)
{
	return Hunk_Alloc_Aligned(size,32);
}

int Hunk_End (void)
{
	hunkcount++;
	return cursize;
}

void Hunk_Free (void *base)
{
	if (base)
		free (base);
	hunkcount--;
}
