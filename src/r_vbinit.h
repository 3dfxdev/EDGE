//----------------------------------------------------------------------------
//  EDGE View Bitmap Initialisation code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
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

#ifndef __R_VBINIT_H__
#define __R_VBINIT_H__

#include "r_view.h"
#include "con_cvar.h"

void InitVB_Classic(viewbitmap_t * vb);

void R_InitCamera_StdObject(camera_t * c, mobj_t * mo);
void R_InitCamera_StdPlayer(camera_t * c);
void R_InitCamera_ViewOffs(camera_t * c, angle_t offs);

#endif
