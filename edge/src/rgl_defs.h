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

#include "wp_main.h"  // -AJA- 2003/10/11: ouch, needed for wipetype_e

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
void RGL_SetupMatrices2D(void);
void RGL_SetupMatrices3D(void);
void RGL_DrawPlayerSprites(player_t * p);

void RGL_RainbowEffect(player_t *player);
void RGL_ColourmapEffect(player_t *player);
void RGL_PaletteEffect(player_t *player);


//
//  RGL_SKY
//
//
void RGL_SetupSkyMatrices(void);
void RGL_RevertSkyMatrices(void);
void RGL_BeginSky(void);
void RGL_FinishSky(void);

void RGL_DrawSkyBox(void);
void RGL_DrawSkyBackground(void);
void RGL_DrawSkyPlane(subsector_t *sub, float h);
void RGL_DrawSkyWall(seg_t *seg, float h1, float h2);


//
//  RGL_BSP
//

#define M_ROOT2  1.414213562f

// save a little room for lighting effects
#define TOP_LIGHT  (0.9f)

#define MAX_PLVERT  64

// extra lighting on the player weapon
extern int rgl_weapon_r;
extern int rgl_weapon_g;
extern int rgl_weapon_b;

void RGL_RenderTrueBSP(void);


//
// RGL_UNIT
//

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
	GLfloat n_x, n_y, n_z;
	GLboolean edge;
}
local_gl_vert_t;

void RGL_InitUnits(void);
void RGL_SoftInitUnits(void);

void RGL_StartUnits(bool solid);
void RGL_FinishUnits(void);

local_gl_vert_t *RGL_BeginUnit(GLuint mode, int max_vert, 
							   GLuint tex_id, int pass,
							   bool masked, bool blended);
void RGL_EndUnit(int actual_vert);
void RGL_DrawUnits(void);

void RGL_SendRawVector(const local_gl_vert_t *V);

// utility macros
#define SET_COLOR(R,G,B,A)  \
	do { vert->col[0] = (R); vert->col[1] = (G); vert->col[2] = (B);  \
	vert->col[3] = (A); } while(0)

#define SET_TEXCOORD(X,Y)  \
	do { vert->t_x = (X); vert->t_y = (Y); } while(0)

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
						GLuint tex_id, int pass, bool masked, bool blended);


//
//  1D OCCLUSION STUFF
//
void RGL_1DOcclusionClear(void);
void RGL_1DOcclusionSet(angle_t low, angle_t high);
bool RGL_1DOcclusionTest(angle_t low, angle_t high);

void RGL_1DOcclusionPush(void);
void RGL_1DOcclusionPop(void);


//
// RGL_WIPE
//

void RGL_InitWipe(int reverse, wipetype_e effect);
void RGL_StopWipe(void);
bool RGL_DoWipe(void);


#endif  // __RGL_DEFS__
