//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Sky defs)
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

#ifndef __RGL_SKY__
#define __RGL_SKY__

#include "v_ctx.h"


void RGL_SetupSkyMatrices(void);
void RGL_RevertSkyMatrices(void);
void RGL_BeginSky(void);
void RGL_FinishSky(void);

void RGL_DrawSkyBox(void);
void RGL_DrawSkyPlane(subsector_t *sub, float h);
void RGL_DrawSkyWall(seg_t *seg, float h1, float h2);

void RGL_UpdateSkyBoxTextures(void);
void RGL_PreCacheSky(void);
void RGL_CalcSkyCoord(float sx, float sy, float sz, int tw, float *tx, float *ty);



#endif  // __RGL_SKY__
