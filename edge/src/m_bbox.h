//----------------------------------------------------------------------------
//  EDGE Bounding-box Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2000  The EDGE Team.
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
void M_ClearBox(float_t * box);
void M_AddToBox(float_t * box, float_t x, float_t y);
void M_CopyBox(float_t * box, float_t * other);
void M_UnionBox(float_t * box, float_t * other);

//
//  DIRTY REGION HANDLING
// 
// Each byte represents a 64x64 block of the screen.
extern byte dirty_region[32][32];

void M_CleanRegion(int x1, int y1, int x2, int y2);
void M_DirtyRegion(int x1, int y1, int x2, int y2);
boolean_t M_TestAndClean(int x, int y);


#endif
