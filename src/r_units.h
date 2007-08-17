//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Unit system)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2007  The EDGE Team.
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

#ifndef __R_UNITS_H__
#define __R_UNITS_H__

#define MAX_PLVERT  64

extern bool use_lighting;
extern bool use_color_material;
extern bool dumb_sky;

// a single vertex to pass to the GL 
typedef struct local_gl_vert_s
{
	GLfloat rgba[4];
	vec3_t pos;
	vec2_t texc[2];
	vec3_t normal;
	GLboolean edge;
}
local_gl_vert_t;

void RGL_InitUnits(void);
void RGL_SoftInitUnits(void);

void RGL_StartUnits(bool solid);
void RGL_FinishUnits(void);
void RGL_DrawUnits(void);

typedef enum
{
	BL_NONE = 0,

	BL_Masked  = (1<<0),  // drop fragments when alpha == 0
	BL_Alpha   = (1<<1),  // alpha-blend with the framebuffer
	BL_Add     = (1<<2),  // additive-blend with the framebuffer
	BL_NoZBuf  = (1<<3),  // don't update the Z buffer
	BL_ClampY  = (1<<4),
}
blending_mode_e;


local_gl_vert_t *RGL_BeginUnit(GLuint shape, int max_vert, 
		                       GLuint env1, GLuint tex1,
							   GLuint env2, GLuint tex2,
							   int pass, int blending);
void RGL_EndUnit(int actual_vert);


//----------------------------------------------------------------------------


typedef enum
{
	PIPEF_NONE = 0,

	PIPEF_NoLight = (1 << 0),
	PIPEF_Shadows = (1 << 1),
}
pipeline_flags_e;

typedef void (* pipeline_coord_func_t)(void *data, int v_idx,
	vec3_t *pos, float *rgb, vec2_t *texc,
	vec3_t *normal, vec3_t *lit_pos);

void R_InitPipeline(void);

// FIXME Colormap setup stuff

// FIXME Shadow setup stuff

// FIXME Glow setup stuff

// FIXME Dlight setup stuff

void R_RunPipeline(GLuint shape, int num_vert,
		           GLuint tex, float alpha, int blending, int flags,
				   void *func_data, pipeline_coord_func_t func);

#endif /* __R_UNITS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
