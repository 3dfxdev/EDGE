//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Definitions)
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

#ifndef __RGL_DEFS__
#define __RGL_DEFS__

#include "v_ctx.h"

//
//  RGL_MAIN
//

#define FUZZY_TRANS  0.30f

extern int glmax_lights;
extern int glmax_clip_planes;
extern int glmax_tex_size;
extern int glmax_tex_units;

extern bool glcap_multitex;
extern bool glcap_edgeclamp;

extern int rgl_light_map[256];
extern angle_t oned_side_angle;  // ANG180 disables polar clipping

void RGL_Init(void);
void RGL_SoftInit(void);
void RGL_DrawProgress(int perc);
void RGL_SetupMatrices2D(void);
void RGL_SetupMatrices3D(void);
void RGL_DrawPlayerSprites(player_t * p);
void RGL_UpdateTheFuzz(void);

//
// RGL_TEX
//
const byte *RGL_LogoImage(int *w, int *h);
const byte *RGL_InitImage(int *w, int *h);


//
//  RGL_BSP
//

#define M_ROOT2  1.414213562f  // FIXME: move into m_math.h ?

// save a little room for lighting effects
#define TOP_LIGHT  (0.9f)

// extra lighting on the player weapon
extern int rgl_weapon_r;
extern int rgl_weapon_g;
extern int rgl_weapon_b;

void RGL_RenderTrueBSP(void);


// FIXME: temp hack
#include "rgl_fx.h"
#include "rgl_occ.h"
#include "rgl_sky.h"
#include "rgl_unit.h"
#include "rgl_wipe.h"


#endif  // __RGL_DEFS__
