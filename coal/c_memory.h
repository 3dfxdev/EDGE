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

#ifndef __COAL_MEMORY_STUFF_H__
#define __COAL_MEMORY_STUFF_H__

struct block_c
{
	int used;

	char data[4096];

public:
	 block_c() : used(0) { }
	~block_c() { }
};

struct bgroup_c
{
	int used;

	block_c *blocks[256];

public:
	 bgroup_c() : used(0) { }
	~bgroup_c();

	int try_alloc(int len);

	int totalUsed() const;
	int totalMemory() const;
};


struct bmaster_c
{
	int used;

	bgroup_c *groups[256];

public:
	 bmaster_c() : used(0) { }
	~bmaster_c();

	int alloc(int len);

	inline void *deref(int index) const
	{
		bgroup_c *grp = groups[index >> 20];
		index &= ((1 << 20) - 1);

		block_c *blk = grp->blocks[index >> 12];
		index &= ((1 << 12) - 1);

		return blk->data + index;
	}

	// compute the total amount of memory used.  The second form
	// includes all the extra/free/wasted space.
	int totalUsed() const;
	int totalMemory() const;
};

#endif /* __COAL_MEMORY_STUFF_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
