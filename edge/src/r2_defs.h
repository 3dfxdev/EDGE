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


//
//  R2_POLY
//

//
// Halfplane
//
// These come from nodes or segs.  Points on the right side of the
// line are considered "in", points on the left are "out".

typedef struct halfplane_s
{
  struct halfplane_s *next;

  float_t x1, y1;
  float_t x2, y2;
} 
halfplane_t;

//
// Cut Point
//
// Remembers the point where two halfplanes intersect.

typedef struct cut_point_s
{
  struct cut_point_s *next;

  float_t x, y;
  float_t x2, y2; 

  angle_t angle;
  angle_t angle2;

  seg_t *what_seg;  // NULL if not a seg
}
cut_point_t;


void R2_Polygonise(void);
void R2_FreeAllocatedStuff(void);


//
//  R2_BSP
//

//
// ScreenLine
//
// Stores the info for one trapezoid shaped area (with vertical sides)
// on the screen.  For walls and planes, it indicates a solid area.
// For the empty list, it indicates an area not yet filled in by walls
// or planes.  Note: yt & yb are inclusive.
//

typedef struct screenline_s
{
  // links for list
  struct screenline_s *next;
  struct screenline_s *prev;

  // inclusive range
  int x1;
  int x2;

  // top line (higher on screen)
  float_t yt;
  float_t yt_step;

  // bottom line (lower on screen)
  float_t yb;
  float_t yb_step;

  // offset for top (in texture pixels)
  float_t yp;
  float_t yp_step;
}
screenline_t;


// forward decls.
struct drawfloor_s;
struct drawsub_s;


//
// DrawWall
//
// Stores the info about a single visible section of a wall of a
// subsector.

typedef struct drawwall_s
{
  // link for list
  struct drawwall_s *next;

  screenline_t *area;

  // texture to use
  sidepart_t *part;
  
  // texture scaling
  float_t scale1;
  float_t scale_step;

  // colourmap & lighting
  region_properties_t *props;

  // info for texture mapper
  float_t distance;
  float_t x_offset;
  angle_t angle;

  // contains transparent parts ?
  int is_masked;
  
  // TEMP HACK
  int picnum;
}
drawwall_t;


//
// ClipSeg
//
// Used to clip sprites to subsector boundaries.  Every time we visit
// a subsector, we must compute a list of clipsegs for every seg of
// the subsector (even ones that are totally off-screen).

typedef struct clipseg_s
{
  // link for list
  struct clipseg_s *next;

  seg_t *seg;
  
  // coordinates translated and rotated about viewpoint
  float_t tx1, tz1;
  float_t tx2, tz2;

  // orientation:
  //   +1 : right side (on screen) faces containing subsector
  //   -1 : left  side (on screen) faces containing subsector
  //    0 : seg lies in same direction as viewplane
  int orientation;
}
clipseg_t;


//
// DrawPlane
//
// Stores the info about a single visible plane (either floor or
// ceiling) of a subsector.

typedef struct drawplane_s
{
  // link for list
  struct drawplane_s *next;

  screenline_t *area;

  // when true, this drawplane only marks where a plane ends
  // (where it starts will be determined by a future drawplane).
  boolean_t marker_only;

  int face_dir;

  plane_info_t *info;
  region_properties_t *props;

  //...

  // TEMP HACK
  int picnum;
}
drawplane_t;


//
// DrawThing
//
// Stores the info about a single visible sprite in a subsector.

typedef struct drawthing_s
{
  // link for list
  struct drawthing_s *next;
  struct drawthing_s *prev;

  screenline_t *area;
  
  // actual map object
  mobj_t *mo;

  // vertical extent of sprite (world coords)
  int top;
  int bottom;

  // for non-cut-off pieces of a sprite, this is NULL, otherwise it is
  // the subsector that the previous (whole, non-split) piece was in.
  // When clipping the sprite against clipsegs, we can skip clipsegs
  // bordering on subsectors that the previous piece was in.
  subsector_t *cut_sub;

  // attempt II at preventing infinite move cycles.  This value is 0
  // if this drawthing has not moved, -1 if it moved left, +1 if it
  // moved right.
  int cut_move_dir;
  
  // sprite image to use
  int patch;
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

  //...

  // TEMP for GL
  float_t left_x,  left_y;
  float_t right_x, right_y;
  
  // TEMP HACK
  int picnum;
}
drawthing_t;


//
// DrawFloor
//
// Stores all the information needed to draw a single on-screen
// floor of a subsector.

typedef struct drawfloor_s
{
  // link for list, drawing order
  struct drawfloor_s *next;

  // link for height order list
  struct drawfloor_s *z_next;
  struct drawfloor_s *z_prev;

  // region
  vert_region_t *reg;

  // list of walls (includes midmasked textures)
  drawwall_t *walls;

  // list of planes (including translucent ones).
  drawplane_t *planes;

  // list of things
  // (not sorted until R2_DrawFloor is called).
  drawthing_t *things;
}
drawfloor_t;


//
// DrawSub
//
// Stores all the information needed to draw a single on-screen
// subsector.

typedef struct drawsub_s
{
  // link in list
  // (sorted from furthest to closest)
  struct drawsub_s *next;

  subsector_t *subsec;

  // list of floors
  // these are sorted into drawing order.
  drawfloor_t *floors;

  // list of floors, sorted in height order.
  drawfloor_t *z_floors;

  // list of clip segs
  clipseg_t *clippers;

  // lists of sprites.  Sprites are initially handled on a
  // per-subsector basis, only later are they vertically moved (or
  // split) into the correct drawfloors.  "Good" things are ones that
  // are known to lie completely within the subsector.  "Raw" things
  // need to be checked (and maybe clipped) against the clipsegs.
  drawthing_t *good_things;
  drawthing_t *raw_things;
}
drawsub_t;


//
// PlaneBack
//
// Describes an onscreen (but back-facing) seg which is used to mark
// the edges of planes.

typedef struct planeback_s
{
  // link for list
  struct planeback_s *next;

  // inclusive range on screen
  int x1;
  int x2;

  // scaling info
  float_t scale1;
  float_t scale2;
}
planeback_t;


extern boolean_t use_true_bsp;
extern boolean_t force_classic;

boolean_t R2_CheckBBox(float_t *bspcoord);
void R2_RenderTrueBSP(void);

void R2_GetThingSprite(mobj_t *thing, spritedef_t ** sprite,
    spriteframe_t ** frame, int *lump, boolean_t *flip, boolean_t *bright);
void R2_ClipSpriteVertically(drawsub_t *dsub, drawthing_t *dthing);


//
//  R2_UTIL
//

extern drawsub_t **arr_drawsubs;
extern int num_drawsubs;

extern screenline_t *empty_list;
extern float_t *sky_clip_top;
extern float_t *sky_clip_bottom;

void R2_ClearPoly(void);
void R2_FreeupPoly(void);

halfplane_t *R2_NewHalfPlane(void);
cut_point_t *R2_NewCutPoint(void);

void R2_ClearBSP(void);
void R2_AddInitialLines(void);

screenline_t *R2_NewScreenLine(void);
drawwall_t   *R2_NewDrawWall(void);
drawplane_t  *R2_NewDrawPlane(void);
drawthing_t  *R2_NewDrawThing(void);
drawfloor_t  *R2_NewDrawFloor(void);
drawsub_t    *R2_NewDrawSub(void);
clipseg_t    *R2_NewClipSeg(void);

drawsub_t *R2_GetDrawsubFromSubsec(subsector_t *subsec);
void R2_ClearPlanes(void);
planeback_t * R2_NewPlaneBack(void);

boolean_t R2_CheckOccluded(int x1, int x2);
boolean_t R2_CheckOccludedShrink(int *x1, int *x2);

screenline_t * R2_ClipEmptyHoriz(screenline_t *empty, int x1, int x2);
boolean_t R2_ClipEmptyArea(screenline_t *empty, screenline_t *solid);
boolean_t R2_ClipSolidArea(screenline_t *solid, screenline_t *empty);
void R2_RemoveEmptyArea(screenline_t *empty);

void R2_AddSkyTop(int x1, int x2, float_t y, float_t y_step);
void R2_AddSkyBottom(int x1, int x2, float_t y, float_t y_step);

//
//  R2_DRAW
//

void R2_DrawWall (drawsub_t *sub, drawwall_t  *wall);
void R2_DrawPlane(drawsub_t *sub, drawplane_t *plane);
void R2_DrawThing(drawsub_t *sub, drawthing_t *thing);
void R2_DrawFloor(drawsub_t *sub, drawfloor_t *floor);
void R2_DrawSubsector(drawsub_t *sub);
void R2_DrawSky(void);

void BOGUS_Clear(void);
void BOGUS_Line(float x1, float y1, float x2, float y2, int col);


#endif  // __R2_DEFS__

