//----------------------------------------------------------------------------
//  EDGE BSP Handling Code
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
//
// -KM- 1998/09/27 Special sector colourmap changing
//

#include "i_defs.h"
#include "r_bsp.h"

#include "dm_defs.h"
#include "dm_state.h"
#include "m_bbox.h"
#include "p_local.h"
#include "r_main.h"
#include "r_state.h"
#include "r_things.h"

side_t *sidedef;
line_t *linedef;
sector_t *frontsector;
sector_t *backsector;

unsigned int root_node;


//
// R_CheckBBox
//
// Checks BSP node/subtree bounding box.
// Returns true if some part of the bbox might be visible.

int checkcoord[12][4] =
{
  {BOXRIGHT, BOXTOP, BOXLEFT, BOXBOTTOM},
  {BOXRIGHT, BOXTOP, BOXLEFT, BOXTOP},
  {BOXRIGHT, BOXBOTTOM, BOXLEFT, BOXTOP},
  {0},
  {BOXLEFT, BOXTOP, BOXLEFT, BOXBOTTOM},
  {0},
  {BOXRIGHT, BOXBOTTOM, BOXRIGHT, BOXTOP},
  {0},
  {BOXLEFT, BOXTOP, BOXRIGHT, BOXBOTTOM},
  {BOXLEFT, BOXBOTTOM, BOXRIGHT, BOXBOTTOM},
  {BOXLEFT, BOXBOTTOM, BOXRIGHT, BOXTOP}
};


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
