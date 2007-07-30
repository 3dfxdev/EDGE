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

/// #include "v_ctx.h"

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

#define EMU_LIGHT(level,dist)  ((level) * 2 - 256 + 80*256.0f / MAX(128.0f, (dist)))

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

extern int doom_fading;
extern int var_nearclip;
extern int var_farclip;

#define APPROX_DIST2(dx,dy)  \
	((dx) + (dy) - 0.5f * MIN((dx),(dy)))

#define APPROX_DIST3(dx,dy,dz)  \
	APPROX_DIST2(APPROX_DIST2(dx,dy),dz)


//----------------------------------------------------------------------------

struct drawfloor_s;


//
//  R2_BSP
//

//
// DrawThing
//
// Stores the info about a single visible sprite in a subsector.
//
typedef struct drawthing_s
{
	// link for list
	struct drawthing_s *next;
	struct drawthing_s *prev;

	// actual map object
	mobj_t *mo;

	// vertical extent of sprite (world coords)
	float top;
	float bottom;

	// these record whether this piece of a sprite has been clipped on
	// the left or right side.  We can skip certain clipsegs when one of
	// these is true (and stop when they both become true).
	// 
	bool clipped_left, clipped_right;

	// +1 if this sprites should be vertically clipped at a solid
	// floor or ceiling, 0 if just clip at translucent planes, or -1 if
	// shouldn't be vertically clipped at all.
	//
	int clip_vert;

	// sprite image to use
	const image_t *image;
	bool flip;

	// scaling
	float xfrac;
	float xscale;
	float yscale;
	float ixscale;
	float iyscale;

	float y_offset;

	// distance
	float dist_scale;

	// translated coords
	float tx, tz;
	float tx1, tx2;

	// colourmap/lighting
	region_properties_t *props;
	bool bright;

	// dynamic lighting
	int extra_light;

	//...

	// world offsets for GL
	float left_dx,  left_dy;
	float right_dx, right_dy;
	float orig_top, orig_bottom;

	// EXPERIMENTAL
	bool is_shadow;
	bool is_halo;
	
	// Rendering order
	struct drawthing_s *rd_l, *rd_r, *rd_prev, *rd_next; 
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

#if 0
	// list of walls (includes midmasked textures)
	drawwall_t *walls;

	// list of planes (including translucent ones).
	drawplane_t *planes;
#endif

	// list of things
	// (not sorted until R2_DrawFloor is called).
	drawthing_t *things;

	// list of dynamic lights
	drawthing_t *dlights;
}
drawfloor_t;

// -ACB- 2004/08/04: I could of done some funky inheritence 
// for the following classes, but felt the duplication was the clear start
// and we can cleanup later if needs be.
//
// In here we use an commited var to determine how many items
// are actually used and its means we don't spend time reallocated
// items, when we can reuse previously generated ones. The
// array_entries reflects how many items are allocated.
//

// --> Draw thing container class
class drawthingarray_c : public epi::array_c
{
public:
	drawthingarray_c() : epi::array_c(sizeof(drawthing_t*)) 
	{ 
		active_trans = false; 
	}
	
	~drawthingarray_c() { Clear(); }

private:
	bool active_trans;
	int commited;

	void CleanupObject(void *obj) { delete *(drawthing_t**)obj; }

public:
	void Commit(void);
	drawthing_t* GetNew(void);
	void Init(void) { commited = 0; }
	void Rollback(void);
};

// --> Draw floor container class
class drawfloorarray_c : public epi::array_c
{
public:
	drawfloorarray_c() : epi::array_c(sizeof(drawfloor_t*)) 
	{ 
		active_trans = false; 
	}
	
	~drawfloorarray_c() { Clear(); }

private:
	bool active_trans;
	int commited;
	
	void CleanupObject(void *obj) { delete *(drawfloor_t**)obj; }

public:
	void Commit(void);
	drawfloor_t* GetNew(void);
	void Init(void) { commited = 0; }
	void Rollback(void);
};

extern int detail_level;
extern int use_dlights;  // 2 means compat_mode (FIXME: remove for EDGE 1.30)
extern int sprite_kludge;

const image_t * R2_GetThingSprite(mobj_t *mo, bool *flip);
const image_t * R2_GetOtherSprite(int sprite, int frame, bool *flip);
void R2_ClipSpriteVertically(subsector_t *dsub, drawthing_t *dthing);

void R2_AddDLights(int num, int *level, 
    float *x, float *y, float *z, mobj_t *mo);
void R2_AddColourDLights(int num, int *r, int *g, int *b, 
    float *x, float *y, float *z, mobj_t *mo);


//
//  R2_UTIL
//

void R2_InitUtil(void);
void R2_ClearBSP(void);

extern drawthingarray_c drawthings;
extern drawfloorarray_c drawfloors;

//
//  R2_DRAW
//

void R2_Init(void);


#endif  // __RGL_DEFS__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
