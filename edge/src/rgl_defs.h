//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Definitions)
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

#ifndef __RGL_DEFS__
#define __RGL_DEFS__

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
void RGL_DrawProgress(int perc, int glbsp_perc);
void RGL_DrawBeta(void);
void RGL_SetupMatrices2D(void);
void RGL_SetupMatrices3D(void);

#define LT_RED(light)  (MIN(255,light) * ren_red_mul / 255.0f)
#define LT_GRN(light)  (MIN(255,light) * ren_grn_mul / 255.0f)
#define LT_BLU(light)  (MIN(255,light) * ren_blu_mul / 255.0f)


//
// RGL_TEX
//
const byte *RGL_LogoImage(int *w, int *h);
const byte *RGL_InitImage(int *w, int *h);
const byte *RGL_GlbspImage(int *w, int *h);
const byte *RGL_BuildImage(int *w, int *h);
const byte *RGL_BetaImage(int *w, int *h);


//
//  RGL_BSP
//

#define M_ROOT2  1.414213562f  // FIXME: move into m_math.h ?

#define EMU_LIGHT(level,dist)  ((level) * 2 - 256 + int(80*256.0f / MAX(128.0f, (dist))))

// extra lighting on the player weapon
extern int rgl_weapon_r;
extern int rgl_weapon_g;
extern int rgl_weapon_b;

int RGL_Light(int nominal);
int RGL_LightEmu(int nominal);
void RGL_LoadLights(void);
void RGL_RenderTrueBSP(void);

extern int ren_extralight;

extern float ren_red_mul;
extern float ren_grn_mul;
extern float ren_blu_mul;

extern bool doom_fading;
extern int var_nearclip;
extern int var_farclip;

#define APPROX_DIST2(dx,dy)  \
	((dx) + (dy) - 0.5f * MIN((dx),(dy)))

#define APPROX_DIST3(dx,dy,dz)  \
	APPROX_DIST2(APPROX_DIST2(dx,dy),dz)


#endif  // __RGL_DEFS__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
