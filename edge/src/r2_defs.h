//----------------------------------------------------------------------------
//  EDGE True BSP Rendering (Definitions)
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
//
// -AJA- 1999/08/31: Wrote this file.
//

#ifndef __R2_DEFS__
#define __R2_DEFS__

#include "v_ctx.h"


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
  float_t y, step;

  // vertical offset (in texture pixels, not screen pixels)
  float_t y_offset;
}
screenline_t;


// forward decls.
struct drawfloor_s;


//
// DrawWall
//
// Stores the info about a single visible section of a wall of a
// subsector.
//
typedef struct drawwall_s
{
  // link for list
  struct drawwall_s *next;

  screenline_t area;

  // seg this belongs to
  seg_t *seg;

  // texture to use
  sidepart_t *part;
  
  // texture scaling
  float_t scale1;
  float_t scale_step;

  // colourmap & lighting
  region_properties_t *props;

  // dynamic lighting
  int extra_light[2];

  // info for texture mapper
  float_t distance;
  float_t x_offset;
  angle_t angle;

  // contains transparent parts ?
  int is_masked;
  
  // horizontal slider ?
  slidetype_e slide_type;
  float_t opening, target;
}
drawwall_t;


//
// DrawPlane
//
// Stores the info about a single visible plane (either floor or
// ceiling) of a subsector.
//
typedef struct drawplane_s
{
  // link for list
  struct drawplane_s *next;

  screenline_t area;

  // when true, this drawplane only marks where a plane ends
  // (where it starts will be determined by a future drawplane).
  boolean_t marker_only;

  int face_dir;

  // texture & offsets to use
  plane_info_t *info;

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
  screenline_t area;

  // actual map object
  mobj_t *mo;

  // vertical extent of sprite (world coords)
  float_t top;
  float_t bottom;

  // these record whether this piece of a sprite has been clipped on
  // the left or right side.  We can skip certain clipsegs when one of
  // these is true (and stop when the both become true).
  boolean_t clipped_left, clipped_right;
  
  // +1 if this sprites should be vertically clipped at a solid
  // floor or ceiling, 0 if just clip at translucent planes, or -1 if
  // shouldn't be vertically clipped at all.
  int clip_vert;
  
  // sprite image to use
  const image_t *image;
  boolean_t flip;

  // scaling
  float_t xfrac;
  float_t xscale;
  float_t yscale;
  float_t ixscale;
  float_t iyscale;
  
  // distance
  float_t dist_scale;

  // translated coords
  float_t tx, tz;
  float_t tx1, tx2;
  
  // colourmap/lighting
  region_properties_t *props;
  boolean_t bright;
  const byte *trans_table;

  // dynamic lighting
  int extra_light;

  //...

  // world offsets for GL
  float_t left_dx,  left_dy;
  float_t right_dx, right_dy;
  
  // EXPERIMENTAL
  boolean_t is_shadow;
  boolean_t is_halo;
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
  struct drawfloor_s *z_next, *z_prev;

  // region
  vert_region_t *reg;

  // list of walls (includes midmasked textures)
  drawwall_t *walls;

  // list of planes (including translucent ones).
  drawplane_t *planes;

  // list of things
  // (not sorted until R2_DrawFloor is called).
  drawthing_t *things;

  // list of extra walls
  drawwall_t *extras;

  // list of dynamic lights
  drawthing_t *dlights;
}
drawfloor_t;


extern boolean_t use_dlights;
extern int sprite_kludge;

boolean_t R2_CheckBBox(float_t *bspcoord);
void R2_RenderTrueBSP(void);

const image_t * R2_GetThingSprite(mobj_t *mo, boolean_t *flip);
const image_t * R2_GetOtherSprite(int sprite, int frame, boolean_t *flip);
void R2_ClipSpriteVertically(subsector_t *dsub, drawthing_t *dthing);

void R2_AddDLights(int num, int *level, 
    float_t *x, float_t *y, float_t *z, mobj_t *mo);
void R2_AddColourDLights(int num, int *r, int *g, int *b, 
    float_t *x, float_t *y, float_t *z, mobj_t *mo);
void R2_FindDLights(subsector_t *sub, drawfloor_t *dfloor);


//
//  R2_UTIL
//

typedef struct tilesky_s
{
  boolean_t active;
  
  const tilesky_info_t *info;

  // linedef that info comes from
  struct line_s *line;
}
tilesky_t;

extern byte *subsectors_seen;
extern Y_range_t Screen_clip[2048];

extern tilesky_t sky_tiles[4];
extern int sky_tiles_active;

void R2_InitUtil(void);
void R2_ClearBSP(void);

drawwall_t  *R2_GetDrawWall(void);
drawplane_t *R2_GetDrawPlane(void);
drawthing_t *R2_GetDrawThing(void);
drawfloor_t *R2_GetDrawFloor(void);

void R2_CommitDrawWall(int used);
void R2_CommitDrawPlane(int used);
void R2_CommitDrawThing(int used);
void R2_CommitDrawFloor(int used);

Y_range_t *R2_GetOpenings(int width);
void R2_CommitOpenings(int width);

void R2_1DOcclusionClear(int x1, int x2);
void R2_1DOcclusionSet(int x1, int x2);
boolean_t R2_1DOcclusionTest(int x1, int x2);
boolean_t R2_1DOcclusionTestShrink(int *x1, int *x2);
void R2_1DOcclusionClose(int x1, int x2, Y_range_t *ranges);

void R2_2DOcclusionClear(int x1, int x2);
void R2_2DOcclusionClose(int x1, int x2, Y_range_t *ranges,
    boolean_t connect_low, boolean_t connect_high, boolean_t solid);
void R2_2DOcclusionCopy(int x1, int x2, Y_range_t *ranges);
void R2_2DUpdate1D(int x1, int x2);

void R2_TileSkyClear(void);
void R2_TileSkyAdd(const tilesky_info_t *info, struct line_s *line);


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

