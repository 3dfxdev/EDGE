//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Things)
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

#ifndef __RGL_THING__
#define __RGL_THING__

#include "r_defs.h"
#include "r2_defs.h"

void RGL_WalkThing(mobj_t *mo, subsector_t *cur_sub);
void RGL_DrawSortThings(drawfloor_t *dfloor);
void RGL_UpdateTheFuzz(void);

void RGL_DrawPlayerSprites(player_t * p);
void RGL_LightUpPlayerWeapon(player_t *p, drawfloor_t *dfloor);


#endif  // __RGL_THING__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
