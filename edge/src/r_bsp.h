//----------------------------------------------------------------------------
//  EDGE BSP Handling Code
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

#ifndef __R_BSP__
#define __R_BSP__

#include "r_defs.h"
#include "z_zone.h"

extern seg_t *curline;
extern side_t *sidedef;
extern line_t *linedef;
extern sector_t *frontsector;
extern sector_t *backsector;
extern region_properties_t *front_properties;
extern int root_node;

extern int rw_x;
extern int rw_stopx;

extern boolean_t segtextured;

// false if the back side is the same plane
extern boolean_t markfloor;
extern boolean_t markceiling;

extern boolean_t skymap;

extern stack_array_t drawsegs_a;
extern drawseg_t **drawsegs;
extern int ds_p;

// -AJA- 1999/07/25: Need this for moving deep water.
extern float_t extrafl_offset;

typedef void (*drawfunc_t) (int start, int stop);

// BSP?
void R_ClearClipSegs(void);
void R_ClearDrawSegs(void);

void R_RenderBSPNode(int bspnum);

#endif
