//----------------------------------------------------------------------------
//  Wrappers for the malloc family of functions that count used bytes.
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
//
// TODO here: Move into EPI code, ./src/utility -> ./epi/zdoom
//

#include "system/i_defs.h"

#include <malloc.h>


size_t AllocBytes;

#ifndef _MSC_VER
#define _NORMAL_BLOCK			0
#define _malloc_dbg(s,b,f,l)	malloc(s)
#define _realloc_dbg(p,s,b,f,l)	realloc(p,s)
#endif
#ifndef _WIN32
#define _msize(p)				malloc_usable_size(p)	// from glibc/FreeBSD
#endif

#ifndef _DEBUG
void* M_Malloc(size_t size)
{
	void* block = malloc(size);

	if (block == NULL)
		I_Error("Could not malloc %u bytes", size);

	AllocBytes += _msize(block);
	return block;
}

void* M_Realloc(void* memblock, size_t size)
{
	if (memblock != NULL)
	{
		AllocBytes -= _msize(memblock);
	}
	void* block = realloc(memblock, size);
	if (block == NULL)
	{
		I_Error("Could not realloc %u bytes", size);
	}
	AllocBytes += _msize(block);
	return block;
}
#else
#ifdef _MSC_VER
#include <crtdbg.h>
#endif

void* M_Malloc_Dbg(size_t size, const char* file, int lineno)
{
	void* block = _malloc_dbg(size, _NORMAL_BLOCK, file, lineno);

	if (block == NULL)
		I_Error("Could not malloc %u bytes", size);

	AllocBytes += _msize(block);
	return block;
}

void* M_Realloc_Dbg(void* memblock, size_t size, const char* file, int lineno)
{
	if (memblock != NULL)
	{
		AllocBytes -= _msize(memblock);
	}
	void* block = _realloc_dbg(memblock, size, _NORMAL_BLOCK, file, lineno);
	if (block == NULL)
	{
		I_Error("Could not realloc %u bytes", size);
	}
	AllocBytes += _msize(block);
	return block;
}
#endif

void M_Free(void* block)
{
	if (block != NULL)
	{
		AllocBytes -= _msize(block);
		free(block);
	}
}