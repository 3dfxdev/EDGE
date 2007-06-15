//----------------------------------------------------------------------------
//  EDGE Bounding Box Code
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
#include "m_bbox.h"

#include <float.h>
#include <string.h>

void M_ClearBox(float * box)
{
	box[BOXTOP] = box[BOXRIGHT] = -FLT_MAX;
	box[BOXBOTTOM] = box[BOXLEFT] = FLT_MAX;
}

void M_AddToBox(float * box, float x, float y)
{
	if (x < box[BOXLEFT])
		box[BOXLEFT] = x;

	if (x > box[BOXRIGHT])
		box[BOXRIGHT] = x;

	if (y < box[BOXBOTTOM])
		box[BOXBOTTOM] = y;

	if (y > box[BOXTOP])
		box[BOXTOP] = y;
}

void M_CopyBox(float * box, float * other)
{
	box[BOXLEFT]   = other[BOXLEFT];
	box[BOXRIGHT]  = other[BOXRIGHT];
	box[BOXTOP]    = other[BOXTOP];
	box[BOXBOTTOM] = other[BOXBOTTOM];
}

void M_UnionBox(float * box, float * other)
{
	if (other[BOXLEFT] < box[BOXLEFT])
		box[BOXLEFT] = other[BOXLEFT];

	if (other[BOXRIGHT] > box[BOXRIGHT])
		box[BOXRIGHT] = other[BOXRIGHT];

	if (other[BOXBOTTOM] < box[BOXBOTTOM])
		box[BOXBOTTOM] = other[BOXBOTTOM];

	if (other[BOXTOP] > box[BOXTOP])
		box[BOXTOP] = other[BOXTOP];
}

//
//  DIRTY REGION HANDLING
//

byte dirty_region[DIRT_REG_H][DIRT_REG_W];

bool dirty_region_whole = true;
bool dirty_region_always = false;

//
// M_CleanMatrix
//
// Make the dirty matrix totally clean (spick and span).

void M_CleanMatrix(void)
{
	dirty_region_whole = false;

	memset(dirty_region, 0, sizeof(dirty_region));
}

//
// M_DirtyMatrix
//
// Make the dirty matrix totally dirty.

void M_DirtyMatrix(void)
{
	if (dirty_region_whole)
		return;

	dirty_region_whole = true;

	memset(dirty_region, 1, sizeof(dirty_region));
}

//
// M_DirtyRegion
//
// Coordinates are in screen pixels (inclusive).

void M_DirtyRegion(int x1, int y1, int x2, int y2)
{
	int x;

	if (dirty_region_whole)
		return;

	SYS_ASSERT(x1 >= 0);  SYS_ASSERT(y1 >= 0);
	SYS_ASSERT(x1 <= x2); SYS_ASSERT(y1 <= y2);

	x1 /= DIRT_X; y1 /= DIRT_Y;
	x2 /= DIRT_X; y2 /= DIRT_Y;

	SYS_ASSERT(x2 < DIRT_REG_W); 
	SYS_ASSERT(y2 < DIRT_REG_H);

	for (; y1 <= y2; y1++)
	{
		for (x=x1; x <= x2; x++)
			dirty_region[y1][x] = 1;
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
