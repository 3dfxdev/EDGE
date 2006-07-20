//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Unit system)
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

#ifndef __RGL_UNIT__
#define __RGL_UNIT__

//#include "v_ctx.h"


#define MAX_PLVERT  64

extern bool use_lighting;
extern bool use_color_material;
extern bool use_vertex_array;
extern bool dumb_sky;

// a single vertex to pass to the GL 
typedef struct local_gl_vert_s
{
	GLfloat x, y, z;
	GLfloat col[4];
	GLfloat t_x, t_y;
	GLfloat t2_x, t2_y;
	GLfloat n_x, n_y, n_z;
	GLboolean edge;
}
local_gl_vert_t;

void RGL_InitUnits(void);
void RGL_SoftInitUnits(void);

void RGL_StartUnits(bool solid);
void RGL_FinishUnits(void);

typedef enum
{
	BL_Masked  = 0x0001,  // drop fragments when (alpha <= 8)
	BL_Alpha   = 0x0002,  // alpha-blend with the framebuffer
	BL_Add     = 0x0004,  // additive-blend with the framebuffer
	BL_Multi   = 0x0010,  // use multi-texturing (second unit MODULATEs)
	BL_NoZBuf  = 0x0020,  // don't update the Z buffer
	BL_ClampY  = 0x0040,
}
blending_mode_e;

local_gl_vert_t *RGL_BeginUnit(GLuint mode, int max_vert, 
							   GLuint tex_id, GLuint tex2_id,
							   int pass, int blending);
void RGL_EndUnit(int actual_vert);
void RGL_DrawUnits(void);

void RGL_SendRawVector(const local_gl_vert_t *V);

// utility macros
#define SET_COLOR(R,G,B,A)  \
	do { vert->col[0] = (R); vert->col[1] = (G); vert->col[2] = (B);  \
	vert->col[3] = (A); } while(0)

#define SET_TEXCOORD(X,Y)  \
	do { vert->t_x = (X); vert->t_y = (Y); } while(0)

#define SET_TEX2COORD(X,Y)  \
	do { vert->t2_x = (X); vert->t2_y = (Y); } while(0)

#define SET_NORMAL(X,Y,Z)  \
	do { vert->n_x = (X); vert->n_y = (Y); vert->n_z = (Z); } while(0)

#define SET_EDGE_FLAG(E)  \
	do { vert->edge = (E); } while(0)

#define SET_VERTEX(X,Y,Z)  \
	do { vert->x = (X); vert->y = (Y); vert->z = (Z); } while(0)


// RAW STUFF

typedef struct raw_polyquad_s
{
	// Quad ?  When true, the number of vertices must be even, and the
	// order must be the same as for glBegin(GL_QUADSTRIP).
	// 
	bool quad;

	vec3_t *verts;
	int num_verts;
	int max_verts;

	// When a polyquad is split, the other pieces are linked from the
	// original through this pointer.
	//
	struct raw_polyquad_s *sisters;

	// 3D bounding box
	vec3_t min, max;
}
raw_polyquad_t;

#define PQ_ADD_VERT(P,X,Y,Z)  do {  \
	(P)->verts[(P)->num_verts].x = (X);  \
	(P)->verts[(P)->num_verts].y = (Y);  \
	(P)->verts[(P)->num_verts].z = (Z);  \
	(P)->num_verts++; } while(0)

raw_polyquad_t *RGL_NewPolyQuad(int maxvert, bool quad);
void RGL_FreePolyQuad(raw_polyquad_t *poly);
void RGL_BoundPolyQuad(raw_polyquad_t *poly);

void RGL_SplitPolyQuad(raw_polyquad_t *poly, int division, 
					   bool separate);
void RGL_SplitPolyQuadLOD(raw_polyquad_t *poly, int max_lod, int base_div);

void RGL_RenderPolyQuad(raw_polyquad_t *poly, void *data,
						void (* CoordFunc)(vec3_t *src, local_gl_vert_t *vert, void *data),
						GLuint tex_id, GLuint tex2_id, int pass, int blending);


#endif  // __RGL_UNIT__
