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

#include "i_defs.h"
#include "z_zone.h"

#include <stdlib.h>
#include <string.h>

static cache_flusher_f **cache_flushers = NULL;
static int num_flushers = 0;

#ifdef __cplusplus
void operator++ (z_urgency_e& urg, int blah)
{
  urg = (z_urgency_e)(urg + 1);
}
#endif

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

#define CHECK_PTR(h) DEV_ASSERT((*(int *)((char *)(h) + (h)->size + sizeof(mallocheader_t)) == ZONEID), ("Block without ZONEID"))
#endif

//
// Z_Free
//
void Z_Free(void *ptr)
{
	// -ES- FIXME: Decide whether we should allow Z_Free(NULL) and Z_Malloc(0)
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

//
// Z_RegisterCacheFlusher
//
// Tells the memory system that f can be called to free up some memory.
//
void Z_RegisterCacheFlusher(cache_flusher_f *f)
{
	Z_Resize(cache_flushers, cache_flusher_f *, num_flushers + 1);
	cache_flushers[num_flushers] = f;
	num_flushers++;
}

//
// FlushCaches
//
// Calls other parts of the code to trim any "cached" data, like
// unused structures stored on an quick-alloc list.  It gets called
// when the zone cannot fulfill a ZMalloc request, in the hope that
// after flushing the memory will be available.
//
// `urge' is one of the Z_Urgency* values, and the more urgent the
// request is, the harder that the cache flushing code should try to
// free memory.  For example, sprites & textures can be reconstructed
// (albeit slowly) from info in the WAD file(s), and in the worst case
// scenario (urge == Z_UrgencyExtreme) the memory could be freed.
//
// -AJA- 1999/09/16: written.
// -ES- 1999/12/18 Written.

static void FlushCaches(z_urgency_e urge, int size)
{
	int i;

	// Call all cache flusher at the current urgency level
	for (i = 0; i < num_flushers; i++)
		(*cache_flushers[i])(urge);

#ifdef DEVELOPERS
	{
		// Output flush count.
		static int count = 0;
		L_WriteDebug("Flush %d", count++);
	}
#endif
}

//
// Z_Calloc
//
// Just allocates memory and clears it.
//
void *Z_Calloc2(int size)
{
	byte *data = (byte *) Z_Malloc(size);
	Z_Clear(data, byte, size);
	return (void *) data;
}

//
// Z_ReMalloc
//
// Zone reallocation.
//
void *Z_ReMalloc2(void *ptr, int size)
{
#if defined INTOLERANT_MATH && defined DEVELOPERS
	// The intolerant remalloc will always trash the old memory block with -1:s,
	// and also trash anything undefined in the new block.
	void *newp;
	mallocheader_t *h;

	if (size == 0)
	{
		if (ptr)
			Z_Free(ptr);
		return NULL;
	}

	if (ptr == NULL)
		return Z_Malloc(size);

	// All trashing of undefined data will be done by Z_Malloc/Z_Free.
	newp = Z_Malloc(size);
	h = (mallocheader_t *)ptr - 1;
	CHECK_PTR(h);
	I_MoveData(newp, ptr, MIN(size, h->size));
	Z_Free(ptr);

	return newp;
#else

#ifdef DEVELOPERS
	mallocheader_t *h, *newp;
#else
	void *h, *newp;
#endif
	int allocsize;
	z_urgency_e flush_urge = Z_UrgencyNone;

	if (size == 0)
	{
		if (ptr)
			Z_Free(ptr);

		return NULL;
	}

#ifdef DEVELOPERS
	if (ptr == NULL)
	{
		h = NULL;
	}
	else
	{
		h = (mallocheader_t *)ptr - 1;
		CHECK_PTR(h);
	}
	allocsize = size + sizeof(mallocheader_t) + sizeof(int);
#else
	h = ptr;
	allocsize = size;
#endif

#ifdef DEVELOPERS
	while (NULL == (newp = (mallocheader_t*)realloc(h, allocsize)))
#else
	while (NULL == (newp = realloc(h, allocsize)))
#endif
	{
		if (flush_urge == Z_UrgencyExtreme)
			I_Error("Z_ReMalloc: failed on allocation of %i bytes", size);

		flush_urge++;
		FlushCaches((z_urgency_e)flush_urge, allocsize);
	}

#ifdef DEVELOPERS
	newp->size = size;
	newp++;
	*(int *)((char *)newp + size) = ZONEID;
#endif
	return newp;
#endif
}

//
// Z_Malloc
//
// Zone allocation.

void *Z_Malloc2(int size)
{
#ifdef DEVELOPERS
	mallocheader_t *p;
#else
	void *p;
#endif
	int allocsize;
	z_urgency_e flush_urge = Z_UrgencyNone;

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
		if (flush_urge == Z_UrgencyExtreme)
			I_Error("Z_Malloc: failed on allocation of %i bytes", size);

		flush_urge++;
		FlushCaches((z_urgency_e)flush_urge, allocsize);
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

#ifdef LEAK_HUNT
//
// Z_RegisterMalloc
//
// Registers the allocater of Z_Malloced memory
void *Z_RegisterMalloc(void *ptr, const char *file, int line)
{
	memblock_t *b = ptr;

	b[-1].file = file;
	b[-1].line = line;
	return ptr;
}

//
// Z_UnRegisterTmpFree
//
void *Z_UnRegisterTmpMalloc(void *ptr, const char *function)
{
	memblock_t *b = ptr;

	if (function != b[-1].file)
		I_Error("I_TmpFree: Allocation from %s (line %d) freed in %s!", b[-1].file, b[-1].line, function);

	return ptr;
}


typedef struct
{
	const char *file;
	int line;
	int count;
}
leak_info_item;

//
// Z_DumpLeakInfo
//
// Scans through & analyses all the memory blocks.
// This routine uses malloc to allocate mem, to avoid trouble. Therefore, the
// heapsize must be slightly smaller than maximum.

void Z_DumpLeakInfo(int level)
{
	int i,j;
	int numblocks = 0;
	int pos;
	memblock_t *block;
	leak_info_item *list = NULL;
	int listsize = 0;
	bool done;
	FILE *f;

	f = fopen("leakinfo", "wt");
	if (!f)
		I_Error("Z_DumpLeakInfo: Couldn't open file!");

	for (block = mainzone->blocklist.next; block != &mainzone->blocklist; block = block->next)
	{
		if (block->user == 0)
			continue;
		if (level != -1 && block->tag >= level)
			continue;

		done = false;
		for (i = 0; i < numblocks; i++)
		{
			// we can actually compare the string pointers directly here.
			if (list[i].file == block->file && list[i].line == block->line)
			{
				list[i].count++;
				done = true;
				break;
			}
		}

		if (done)
			continue;

		numblocks++;
		if (numblocks > listsize)
		{
			listsize += 64;
			list = realloc(list, listsize * sizeof(leak_info_item));
			if (list == NULL)
				I_Error("Z_DumpLeakInfo: Out of memory!\n");
		}

		list[numblocks - 1].file = block->file;
		list[numblocks - 1].line = block->line;
		list[numblocks - 1].count = 0;
	}

	// ignore all calls that only alloced 1 block.
	pos = 0;
	for (i = 0; i < numblocks; i++)
	{
		if (list[i].count)
		{
			list[pos] = list[i];
			pos++;
		}
	}
	numblocks = pos;

	for (i = 0; i < numblocks; i++)
	{
		fprintf(f, "%3d %s:%d\n", list[i].count+1, list[i].file, list[i].line);
	}

	numblocks = 0;
	// Save the memory usage of each malloc call
	for (block = mainzone->blocklist.next; block != &mainzone->blocklist; block = block->next)
	{
		if (block->user == 0)
			continue;
		if (level != -1 && block->tag >= level)
			continue;

		done = false;
		for (i = 0; i < numblocks; i++)
		{
			// we can actually compare the string pointers directly here.
			if (list[i].file == block->file && list[i].line == block->line)
			{
				list[i].count += block->size;
				done = true;
				break;
			}
		}

		if (done)
			continue;

		numblocks++;
		if (numblocks > listsize)
		{
			listsize += 64;
			list = realloc(list, listsize * sizeof(leak_info_item));
			if (list == NULL)
				I_Error("Z_DumpLeakInfo: Out of memory!\n");
		}

		list[numblocks - 1].file = block->file;
		list[numblocks - 1].line = block->line;
		list[numblocks - 1].count = block->size;
	}

	// output from one source file at a time.
	fprintf(f, "\nMem usage:\n");
	for (i = 0; i < numblocks; i++)
	{
		if (list[i].count)
		{
			j = 0;
			fprintf(f, "%s:\n", list[i].file);
			for (pos = i; pos < numblocks; pos++)
			{
				if (list[i].file == list[pos].file)
				{
					j += list[pos].count;
					fprintf(f, " %d: %d\n", list[pos].line, list[pos].count);
					list[pos].count = 0;
				}
			}
			fprintf(f, "%s total: %d bytes\n\n", list[i].file, j);
		}
	}

	fclose(f);
	free(list);
}
#endif

//
// Z_Init
//
void Z_Init(void)
{
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
