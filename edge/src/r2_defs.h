//----------------------------------------------------------------------------
//  EDGE True BSP Rendering (Definitions)
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
//
// -AJA- 1999/08/31: Wrote this file.
//

#ifndef __R2_DEFS__
#define __R2_DEFS__

#include "v_ctx.h"


struct drawfloor_s;


//
//  R2_BSP
//

typedef struct Y_range_s
{
	// range is inclusive.  y1 > y2 means the column is empty.
	short y1, y2;
}
Y_range_t;

//
// ScreenLine
//
// Stores the info for one on-screen area of a wall, plane or thing.
//
typedef struct screenline_s
{
	// horizontal range (inclusive)
	short x1, x2;

	// vertical columns over x1..x2.
	Y_range_t *ranges;

	// top line (higher on screen), used only for tex coords.  When the
	// ranges are unclipped, this should correspond to the top pixel
	// positions in the ranges.
	float y, step;

	// vertical offset (in WORLD coordinates, but positive goes down)
	float y_offset;
}
screenline_t;


//
// DrawWall
//
// Stores the info about a single visible section of a wall of a
// subsector.  Not used in GL mode.
//
typedef struct drawwall_s
{
	// link for list
	struct drawwall_s *next;

	screenline_t area;

	// seg this belongs to
	seg_t *seg;

	// texture to use
	surface_t *part;

	// texture scaling
	float scale1;
	float scale_step;

	// colourmap & lighting
	region_properties_t *props;

	// dynamic lighting
	int extra_light[2];

	// info for texture mapper
	float distance;
	float x_offset;
	angle_t angle;

	// contains transparent parts ?
	int is_masked;

	// horizontal slider ?
	slidetype_e slide_type;
	float opening, line_len;
	int side;
}
drawwall_t;


//
// DrawPlane
//
// Stores the info about a single visible plane (either floor or
// ceiling) of a subsector.  Not used in GL mode.
//
typedef struct drawplane_s
{
	// link for list
	struct drawplane_s *next;

	screenline_t area;

	// when true, this drawplane only marks where a plane ends
	// (where it starts will be determined by a future drawplane).
	bool marker_only;

	int face_dir;

	// height
	float h;

	// texture & offsets to use
	surface_t *info;

	// colourmap & lighting
	region_properties_t *props;

	// dynamic lighting
	int extra_light[2];
	int min_y, max_y;
}
drawplane_t;


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

	// Note: the area.ranges field isn't used here, instead the x1..x2
	// range is looked-up in the the containing subsector, which stores
	// *empty* areas to clip against.
	//
	screenline_t area;

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

	// distance
	float dist_scale;

	// translated coords
	float tx, tz;
	float tx1, tx2;

	// colourmap/lighting
	region_properties_t *props;
	bool bright;
	const byte *trans_table;

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

	// list of walls (includes midmasked textures)
	drawwall_t *walls;

	// list of planes (including translucent ones).
	drawplane_t *planes;

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

// --> Draw wall container class
class drawwallarray_c : public epi::array_c
{
public:
	drawwallarray_c() : epi::array_c(sizeof(drawwall_t*)) 
	{ 
		active_trans = false; 
	}
	
	~drawwallarray_c() { Clear(); }

private:
	bool active_trans;
	int commited;
	
	void CleanupObject(void *obj) { delete *(drawwall_t**)obj; }

public:
	void Commit(void);
	drawwall_t* GetNew(void);
	void Init(void) { commited = 0; }
	void Rollback(void);
};

// --> Draw plane container class
class drawplanearray_c : public epi::array_c
{
public:
	drawplanearray_c() : epi::array_c(sizeof(drawplane_t*)) 
	{ 
		active_trans = false; 
	}
	
	~drawplanearray_c() { Clear(); }

private:
	bool active_trans;
	int commited;
	
	void CleanupObject(void *obj) { delete *(drawplane_t**)obj; }
	
public:
	void Commit(void);
	drawplane_t* GetNew(void);
	void Init(void) { commited = 0; }
	void Rollback(void);
};

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
extern bool use_dlights;
extern int sprite_kludge;

bool R2_CheckBBox(float *bspcoord);
void R2_RenderTrueBSP(void);

const image_t * R2_GetThingSprite(mobj_t *mo, bool *flip);
const image_t * R2_GetOtherSprite(int sprite, int frame, bool *flip);
void R2_ClipSpriteVertically(subsector_t *dsub, drawthing_t *dthing);

void R2_AddDLights(int num, int *level, 
    float *x, float *y, float *z, mobj_t *mo);
void R2_AddColourDLights(int num, int *r, int *g, int *b, 
    float *x, float *y, float *z, mobj_t *mo);
void R2_FindDLights(subsector_t *sub, drawfloor_t *dfloor);


//
//  R2_UTIL
//

extern byte *subsectors_seen;
extern Y_range_t Screen_clip[2048];

extern drawwallarray_c  drawwalls;
extern drawplanearray_c drawplanes;
extern drawthingarray_c drawthings;
extern drawfloorarray_c drawfloors;

void R2_InitUtil(void);
void R2_ClearBSP(void);

//Y_range_t *R2_GetOpenings(int width);
//void R2_CommitOpenings(int width);

void R2_1DOcclusionClear(int x1, int x2);
void R2_1DOcclusionSet(int x1, int x2);
bool R2_1DOcclusionTest(int x1, int x2);
bool R2_1DOcclusionTestShrink(int *x1, int *x2);
void R2_1DOcclusionClose(int x1, int x2, Y_range_t *ranges);

void R2_2DOcclusionClear(int x1, int x2);
void R2_2DOcclusionClose(int x1, int x2, Y_range_t *ranges,
    bool connect_low, bool connect_high, bool solid);
void R2_2DOcclusionCopy(int x1, int x2, Y_range_t *ranges);
void R2_2DUpdate1D(int x1, int x2);

int R2_GetPointLOD(float x, float y, float z);
int R2_GetBBoxLOD(float x1, float y1, float z1,
    float x2, float y2, float z2);
int R2_GetWallLOD(float x1, float y1, float z1,
    float x2, float y2, float z2);
int R2_GetPlaneLOD(subsector_t *sub, float h);


//
//  R2_DRAW
//

extern video_context_t vctx;

void R2_DrawWall (subsector_t *dsub, drawwall_t  *wall);
void R2_DrawPlane(subsector_t *dsub, drawplane_t *plane);
void R2_DrawThing(subsector_t *dsub, drawthing_t *thing);
void R2_DrawFloor(subsector_t *dsub, drawfloor_t *dfloor);
void R2_DrawSubsector(subsector_t *dsub);
void R2_DrawPlayerSprites(player_t * p);

void BOGUS_Clear(void);
void BOGUS_Line(float x1, float y1, float x2, float y2, int col);

void R2_Init(void);


#endif  // __R2_DEFS__

