//----------------------------------------------------------------------
//  COAL MEMORY BLOCKS
//----------------------------------------------------------------------
// 
//  Copyright (C) 2009  Andrew Apted
//
//  Coal is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as
//  published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  Coal is distributed in the hope that it will be useful, but
//  WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
//  the GNU General Public License for more details.
//
//----------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include <vector>

#include "coal.h"


namespace coal
{

#include "c_local.h"
#include "c_memory.h"


bgroup_c::~bgroup_c()
{
	while (used > 0)
	{
		used--;
		delete[] blocks[used];
	}
}

int bgroup_c::try_alloc(int len)
{
	if (len > 2040)
	{
		// Special handling for "big" blocks:
		//
		// We allocate a number of contiguous block_c structures,
		// returning the address for the first one (allowing the
		// rest to be overwritten).

		if (used == 256)
			return -1;

		int big_num = 1 + (len >> 12);

		blocks[used] = new block_c[big_num];
		blocks[used]->used = len;

		used++;

		return (used - 1) << 12;
	}

	if (used == 0)
		blocks[used++] = new block_c[1];

	if (blocks[used-1]->used + len > 4096)
	{
		// need a new block
		if (used == 256)
			return -1;

		blocks[used++] = new block_c[1];
	}

	int result = ((used-1) << 12) | blocks[used-1]->used;

	blocks[used-1]->used += len;

	return result;
}

int bgroup_c::usedMemory() const
{
	int result = 0;

	for (int i = 0; i < used; i++)
		result += blocks[i]->used;

	return result;
}

int bgroup_c::totalMemory() const
{
	int result = (int)sizeof(bgroup_c);

	for (int i = 0; i < used; i++)
	{
		int big_num = 1;
		if (blocks[i]->used > 4096)
			big_num = 1 + (blocks[i]->used >> 12);

		result += big_num * (int)sizeof(block_c);
	}

	return result;
}


//----------------------------------------------------------------------

bmaster_c::~bmaster_c()
{
	while (used > 0)
	{
		used--;
		delete[] groups[used];
	}
}



int bmaster_c::alloc(int len)
{
	if (len == 0)
		return 0;

	if (used == 0)
		groups[used++] = new bgroup_c[1];

	int attempt = groups[used-1]->try_alloc(len);

	if (attempt < 0)
	{
		// need a new group
		assert(used < 256);

		groups[used++] = new bgroup_c[1];

		attempt = groups[used-1]->try_alloc(len);
		assert(attempt >= 0);
	}

	return ((used - 1) << 20) | attempt;
}


int bmaster_c::usedMemory() const
{
	int result = 0;

	for (int k = 0; k < used; k++)
		result += groups[k]->usedMemory();

	return result;
}

int bmaster_c::totalMemory() const
{
	int result = (int)sizeof(bmaster_c);

	for (int k = 0; k < used; k++)
		result += groups[k]->totalMemory();

	return result;
}


}  // namespace coal

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
