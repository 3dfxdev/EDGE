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
// (NOTE: rgl_tex prolly completely redundant by new w_image code).

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

void RGL_Texture(int texture, int is_masked);
void RGL_Flat(int flat);
void RGL_Sprite(int sprite);


void RGL_1DOcclusionClear(angle_t low, angle_t high);
void RGL_1DOcclusionSet(angle_t low, angle_t high);
boolean_t RGL_1DOcclusionTest(angle_t low, angle_t high);


//
//  RGL_MAIN
//

extern int glmax_lights;
extern int glmax_clip_planes;
extern int glmax_tex_size;

extern angle_t oned_side_angle;

extern float_t light_to_gl[256];

void RGL_Init(void);
void RGL_SetupMatrices2D(void);
void RGL_SetupMatrices3D(void);
void RGL_MarkSky(int x1, int x2);
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

void RGL_RenderScene(int x1, int y1, int x2, int y2, vid_view_t *view);

// FIXME: this will be redundant with layer system...
void RGL_RenderTrueBSP(void);


#endif  // __R2_DEFS__
#endif  // USE_GL

