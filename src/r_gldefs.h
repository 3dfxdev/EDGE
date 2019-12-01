//----------------------------------------------------------------------------
//  EDGE2 OpenGL Rendering (Definitions)
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2009  The EDGE2 Team.
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

#include "../ddf/main.h"
#include "../ddf/image.h"

#include "r_defs.h"

#include <list>
#include <vector>

extern int r_anisotropy;
extern int r_colorlighting;
extern int r_colormaterial;

extern int r_dumbsky;
extern int r_dumbmulti;
extern int r_dumbcombine;
extern int r_dumbclamp;

extern int r_gl3_path;

//
//  RGL_MAIN
//

extern int glmax_lights;
extern int glmax_clip_planes;
extern int glmax_tex_size;
extern int glmax_tex_units;

void RGL_Init(void);
void RGL_ReInit(void);
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

void RGL_LoadLights(void);

extern int ren_extralight;

extern float ren_red_mul;
extern float ren_grn_mul;
extern float ren_blu_mul;

extern const colourmap_c *ren_fx_colmap;

extern int doom_fading;

extern float r_aspect;

extern float r_nearclip;
extern float r_farclip;

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

	float mx, my, mz;  // mz only used for models

	// vertical extent of sprite (world coords)
	float top;
	float bottom;

	int y_clipping;

	// sprite image to use
	const image_c *image;
	bool flip;

	// translated coords
	float tx, tz;

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
	short is_lowest;
	short is_highest;

	// link for list, rendering order
	struct drawfloor_s *next_R, *prev_R;

	// heights for this floor
	float f_h, c_h, top_h;

	surface_t *floor, *ceil;

	extrafloor_t *f_ef, *c_ef;

	// properties used herein
	region_properties_t *props;

	// list of things
	// (not sorted until R2_DrawFloor is called).
	drawthing_t *things;

public:
	void Clear()
	{
		is_highest = is_lowest = false;
		next_R = prev_R = NULL;
		floor = ceil = NULL;
		f_ef = c_ef = NULL;
		props = NULL;
		things = NULL;
	}
}
drawfloor_t;


class drawmirror_c
{
public:
	seg_t *seg;

	angle_t left, right;

	bool is_portal;

	std::list<drawsub_c *> drawsubs;

public:
	drawmirror_c() : seg(NULL), is_portal(false), drawsubs() //TODO: V730 https://www.viva64.com/en/w/v730/ Not all members of a class are initialized inside the constructor. Consider inspecting: left, right.
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

    // floors, sorted in height order (lowest to highest).
	std::vector<drawfloor_t *> floors;

	// link list of floors, render order (furthest to closest)
	drawfloor_t *floors_R;

	std::list<drawseg_c *> segs;

	std::list<drawmirror_c *> mirrors;

	bool visible;
	bool sorted;

public:
	drawsub_c() : sub(NULL), floors(), segs(), mirrors() //TODO: V730 https://www.viva64.com/en/w/v730/ Not all members of a class are initialized inside the constructor. Consider inspecting: floors_R, visible, sorted.
	{ }

	~drawsub_c()
	{ /* !!!! FIXME */ }

	void Clear(subsector_t *ss)
	{
		sub = ss;
		visible = false;
		sorted  = false;
		floors_R = NULL;

		floors.clear();
		segs.clear();
		mirrors.clear();
	}
};


extern int detail_level;
extern int use_dlights;
extern int simple_shadows;
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


//
//  MIRRORS
//

extern int num_active_mirrors;

void MIR_Coordinate(float& x, float& y);
void MIR_Height(float& z);
void MIR_Angle(angle_t& ang);

bool MIR_Reflective(void);
float MIR_XYScale(void);
float MIR_ZScale(void);

//
//   SHADOWS (simple)
//

/* extern simple_shadow_params_t simple_shadows;
extern int gl_shadows_maxdist;
extern int gl_shadows_factor;

void RGL_InitShadows(void);
void RGL_ProcessThingShadow(mobj_t *mo);
void RGL_RenderShadows(void); */

#endif /* __RGL_DEFS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
