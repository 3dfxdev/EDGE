//----------------------------------------------------------------------------
//  EDGE Bounding-box Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2003  The EDGE Team.
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

#ifndef __M_BBOX__
#define __M_BBOX__

#include <limits.h>

#include "m_fixed.h"

// Bounding box coordinate storage.
enum
{
  BOXTOP,
  BOXBOTTOM,
  BOXLEFT,
  BOXRIGHT
};  // bbox coordinates

// Bounding box functions.
void M_ClearBox(float * box);
void M_AddToBox(float * box, float x, float y);
void M_CopyBox(float * box, float * other);
void M_UnionBox(float * box, float * other);

//
//  DIRTY REGION HANDLING
// 
// Each byte in dirty_region[][] represents a block (DIRT_X x DIRT_Y)
// on the screen.  When `dirty_region_whole' is true, then the whole
// screen is considered dirty (simplifying some operations).  When
// `dirty_region_always' is true, the whole screen is ALWAYS
// considered to be dirty (e.g. double buffering & drawing into video
// memory).

#define DIRT_X  16
#define DIRT_Y  16

#define DIRT_REG_W  ((2048+DIRT_X-1) / DIRT_X)
#define DIRT_REG_H  ((1536+DIRT_Y-1) / DIRT_X)

extern byte dirty_region[DIRT_REG_H][DIRT_REG_W];
extern bool dirty_region_whole;
extern bool dirty_region_always;

void M_CleanMatrix(void);
void M_DirtyMatrix(void);
void M_DirtyRegion(int x1, int y1, int x2, int y2);


#endif
