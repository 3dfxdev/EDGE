//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Definitions)
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

#ifdef USE_GL

#ifndef __RGL_DEFS__
#define __RGL_DEFS__

#include "v_ctx.h"


//
//  RGL_TEX
//
// (NOTE: rgl_tex is completely redundant by new w_image code).
//

typedef struct stored_gl_tex_s
{
  // current status
  enum
  { 
    TEX_EMPTY=0, TEX_OK, TEX_DUMMY
  } 
  status;

  // GL identifier,  when TEX_OK
  // Dummy color (5:5:5), when TEX_DUMMY
  GLuint id;

  // width & height (in pixels)
  short width;
  short height;

  // right & bottom parts
  float_t right;
  float_t bottom;
}
stored_gl_tex_t;

void RGL_InitTextures(void);
void RGL_ResetTextures(void);

void RGL_Sprite(int sprite);


// EXPERIMENTAL!!
#define SPR_HALO  9998


//
//  RGL_MAIN
//

extern int glmax_lights;
extern int glmax_clip_planes;
extern int glmax_tex_size;

extern angle_t oned_side_angle;

void RGL_Init(void);
void RGL_SetupMatrices2D(void);
void RGL_SetupMatrices3D(void);
void RGL_MarkSky(void);
void RGL_DrawSky(void);
void RGL_DrawPlayerSprites(player_t * p);

void RGL_RainbowEffect(player_t *player);
void RGL_ColourmapEffect(player_t *player);
void RGL_PaletteEffect(player_t *player);

// FIXME: these three will be redundant with layer system...
void RGL_MapClear(void);
void RGL_DrawLine(int x1, int y1, int x2, int y2, int colour);
void RGL_DrawPatch(int patch, int sx, int sy);


//
//  RGL_BSP
//

#define M_ROOT2  1.414213562

// save a little room for lighting effects
#define TOP_LIGHT  (0.9)

void RGL_RenderScene(int x1, int y1, int x2, int y2, vid_view_t *view);

// FIXME: this will be redundant with layer system...
void RGL_RenderTrueBSP(void);


//
// RGL_UNIT
//

// a single vertex to pass to the GL 
typedef struct local_gl_vert_s
{
  GLfloat x, y, z;
  GLfloat r, g, b, a;
  GLfloat t_x, t_y;
  GLfloat n_x, n_y, n_z;
  GLboolean edge;
}
local_gl_vert_t;

void RGL_InitUnits(void);
void RGL_StartUnits(boolean_t solid);
void RGL_FinishUnits(void);

local_gl_vert_t *RGL_BeginUnit(GLuint mode, int max_vert, 
    GLuint tex_id, boolean_t masked, boolean_t blended);
void RGL_EndUnit(int actual_vert);
void RGL_DrawUnits(void);

// utility macros
#define SET_VERTEX(X,Y,Z)  \
    do { vert->x = (X); vert->y = (Y); vert->z = (Z); } while(0)

#define SET_COLOR(R,G,B,A)  \
    do { vert->r = (R); vert->g = (G); vert->b = (B);  \
         vert->a = (A); } while(0)

#define SET_TEXCOORD(X,Y)  \
    do { vert->t_x = (X); vert->t_y = (Y); } while(0)

#define SET_NORMAL(X,Y,Z)  \
    do { vert->n_x = (X); vert->n_y = (Y); vert->n_z = (Z); } while(0)

#define SET_EDGE_FLAG(E)  \
    do { vert->edge = (E); } while(0)
 
//
//  1D OCCLUSION STUFF
//
void RGL_1DOcclusionClear(angle_t low, angle_t high);
void RGL_1DOcclusionSet(angle_t low, angle_t high);
boolean_t RGL_1DOcclusionTest(angle_t low, angle_t high);


#endif  // __RGL_DEFS__
#endif  // USE_GL

