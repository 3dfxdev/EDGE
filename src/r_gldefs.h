//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Definitions)
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

#ifndef __RGL_DEFS__
#define __RGL_DEFS__

#include "r_defs.h"

#include <list>
#include <vector>


extern bool use_lighting;
extern bool use_color_material;

extern bool dumb_sky;
extern bool dumb_multi;
extern bool dumb_combine;
extern bool dumb_clamp;


//
//  RGL_MAIN
//

#define FUZZY_TRANS  0.30f

extern int glmax_lights;
extern int glmax_clip_planes;
extern int glmax_tex_size;
extern int glmax_tex_units;

extern int rgl_light_map[256];

void RGL_Init(void);
void RGL_SoftInit(void);
void RGL_DrawProgress(int perc, int glbsp_perc);
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


int RGL_Light(int nominal);

void RGL_LoadLights(void);
void RGL_RenderTrueBSP(void);

extern int ren_extralight;

extern float ren_red_mul;
extern float ren_grn_mul;
extern float ren_blu_mul;

extern int doom_fading;
extern int var_nearclip;
extern int var_farclip;

#define APPROX_DIST2(dx,dy)  \
	((dx) + (dy) - 0.5f * MIN((dx),(dy)))

#define APPROX_DIST3(dx,dy,dz)  \
	APPROX_DIST2(APPROX_DIST2(dx,dy),dz)


//----------------------------------------------------------------------------

struct drawfloor_s;

class drawsub_c;


typedef enum
{
	YCLIP_Never = 0,
	YCLIP_Soft  = 1, // only clip at translucent water
	YCLIP_Hard  = 2, // vertically clip sprites at all solid surfaces
}
y_clip_mode_e;


//
// DrawThing
//
// Stores the info about a single visible sprite in a subsector.
//
typedef struct drawthing_s
{
public:
	// link for list
	struct drawthing_s *next;
	struct drawthing_s *prev;

	// actual map object
	mobj_t *mo;

	bool is_model;

	float mx, my;

	// vertical extent of sprite (world coords)
	float top;
	float bottom;

	int y_clipping;

	// sprite image to use
	const image_c *image;
	bool flip;

	// translated coords
	float tx, tz;
	float tx1, tx2;

	// colourmap/lighting
	region_properties_t *props;

	// world offsets for GL
	float left_dx,  left_dy;
	float right_dx, right_dy;
	float orig_top, orig_bottom;

	// Rendering order
	struct drawthing_s *rd_l, *rd_r, *rd_prev, *rd_next; 

public:
	void Clear()
	{
		next = prev = NULL;
		mo = NULL;
		image = NULL;
		props = NULL;
		rd_l = rd_r = rd_prev = rd_next = NULL;
	}
}
drawthing_t;


//
// DrawFloor
//
// Stores all the information needed to draw a single on-screen
// floor of a subsector.
//
typedef struct drawfloor_s
{
public:
	// link for list, drawing order
	struct drawfloor_s *next, *prev;

	// link for height order list
	struct drawfloor_s *higher, *lower;

	// heights for this floor
	float f_h, c_h, top_h;

	surface_t *floor, *ceil;

	extrafloor_t *ef;

	// properties used herein
	region_properties_t *props;

	// list of things
	// (not sorted until R2_DrawFloor is called).
	drawthing_t *things;

	// list of dynamic lights
	drawthing_t *dlights;

public:
	void Clear()
	{
		next = prev = NULL;
		higher = lower = NULL;
		floor = ceil = NULL;
		ef = NULL;
		props = NULL;
		things = dlights = NULL;
	}
}
drawfloor_t;


class drawmirror_c
{
public:
	seg_t *seg;

	angle_t left, right;

	std::list<drawsub_c *> drawsubs;

public:
	drawmirror_c() : seg(NULL), drawsubs()
	{ }

	~drawmirror_c()
	{ /* FIXME !!!! */ }

	void Clear(seg_t *ss)
	{
		seg  = ss;

		drawsubs.clear();
	}
};


class drawseg_c   // HOPEFULLY this can go away
{
public:
	seg_t *seg;
};


class drawsub_c
{
public:
	subsector_t *sub;

    // floors.  During the WALK phase they are sorted in
	// height order (lowest to highest).
	// For the DRAW phase they get sorted into rendering order
	// (furthest to closest).
	std::vector<drawfloor_t *> floors;

	std::list<drawseg_c *> segs;

	std::list<drawmirror_c *> mirrors;

	bool visible;
	bool sorted;

public:
	drawsub_c() : sub(NULL), floors(), segs(), mirrors()
	{ }

	~drawsub_c()
	{ /* !!!! FIXME */ }

	void Clear(subsector_t *ss)
	{
		sub = ss;
		visible = false;
		sorted  = false;

		floors.clear();
		segs.clear();
		mirrors.clear();
	}
};


extern int detail_level;
extern int use_dlights;
extern int sprite_kludge;

const image_c * R2_GetThingSprite(mobj_t *mo, bool *flip);
const image_c * R2_GetOtherSprite(int sprite, int frame, bool *flip);


//
//  R2_UTIL
//

void R2_InitUtil(void);
void R2_ClearBSP(void);

drawthing_t  *R_GetDrawThing();
drawfloor_t  *R_GetDrawFloor();
drawseg_c    *R_GetDrawSeg();
drawsub_c    *R_GetDrawSub();
drawmirror_c *R_GetDrawMirror();


//
//  R2_DRAW
//

void R2_Init(void);


#endif /* __RGL_DEFS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
