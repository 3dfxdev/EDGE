//----------------------------------------------------------------------------
//  EDGE Video Code
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
//
// Original Author: Chi Hoang
//

#ifndef __MULTIRES_H__
#define __MULTIRES_H__

#include "dm_type.h"
#include "dm_defs.h"
#include "r_data.h"

bool V_MultiResInit(void);
void V_InitResolution(void);
void V_AddAvailableResolution(screenmode_t *mode);
int V_FindClosestResolution(screenmode_t *mode,
    bool samesize, bool samedepth);
int V_CompareModes(screenmode_t *A, screenmode_t *B);

//
//start with v_video.h
//
#define CENTERY                 (SCREENHEIGHT/2)

#define FROM_320(x)  ((x) * SCREENWIDTH  / 320)
#define FROM_200(y)  ((y) * SCREENHEIGHT / 200)

//
// now with r_draw.h
//

void R_FillBackScreen(void);

// Screen Modes
extern screenmode_t *scrmode;
extern int numscrmodes;

#endif // __MULTIRES_H__
