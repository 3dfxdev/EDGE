//----------------------------------------------------------------------------
//  EDGE True BSP Rendering (Utility routines)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2003  The EDGE Team.
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
// TODO HERE:
//   + Implement better 1D buffer code.
//

#include "i_defs.h"

#include "dm_state.h"
#include "m_bbox.h"
#include "m_fixed.h"

#include "r_data.h"
#include "r_defs.h"
#include "r_state.h"
#include "r_main.h"
#include "v_res.h"
#include "z_zone.h"

#include "r2_defs.h"


// arrays of stuff

typedef struct commit_array_s
{
  stack_array_t a;

  // position of current free entry.
  int pos;

  // whether an item has been gotten, but not yet committed.
  bool active;
}
commit_array_t;

static drawwall_t  ** arr_walls  = NULL;
static drawplane_t ** arr_planes = NULL;
static drawthing_t ** arr_things = NULL;
static drawfloor_t ** arr_floors = NULL;

static commit_array_t cmt_walls;
static commit_array_t cmt_planes;
static commit_array_t cmt_things;
static commit_array_t cmt_floors;


byte *subsectors_seen = NULL;
static int subsectors_seen_size = -1;

void R2_InitOpenings(void);
void R2_StartOpenings(void);


//
// R2_InitUtil
//
// One-time initialisation routine.
//
void R2_InitUtil(void)
{
  Z_InitStackArray(&cmt_walls.a, (void ***)&arr_walls, 
      sizeof(drawwall_t),  64);
  cmt_walls.pos = 0;
  cmt_walls.active = false;

  Z_InitStackArray(&cmt_planes.a, (void ***)&arr_planes, 
      sizeof(drawplane_t), 64);
  cmt_planes.pos = 0;
  cmt_planes.active = false;
      
  Z_InitStackArray(&cmt_things.a, (void ***)&arr_things, 
      sizeof(drawthing_t), 64);
  cmt_things.pos = 0;
  cmt_things.active = false;

  Z_InitStackArray(&cmt_floors.a, (void ***)&arr_floors, 
      sizeof(drawfloor_t), 64);
  cmt_floors.pos = 0;
  cmt_floors.active = false;

  R2_InitOpenings();
}

// bsp clear function

void R2_ClearBSP(void)
{
  Z_SetArraySize(&cmt_walls.a,  0);
  Z_SetArraySize(&cmt_planes.a, 0);
  Z_SetArraySize(&cmt_things.a, 0);
  Z_SetArraySize(&cmt_floors.a, 0);

  cmt_walls.pos  = 0;
  cmt_planes.pos = 0;
  cmt_things.pos = 0;
  cmt_floors.pos = 0;

  if (subsectors_seen_size != numsubsectors)
  {
    subsectors_seen_size = numsubsectors;

    Z_Resize(subsectors_seen, byte, subsectors_seen_size);
  }

  Z_Clear(subsectors_seen, byte, subsectors_seen_size);

  R2_StartOpenings();
}


// allocate functions

drawwall_t *R2_GetDrawWall(void)
{
  DEV_ASSERT(!cmt_walls.active, ("R2_GetDrawWall: called twice"));
  
  if (cmt_walls.pos >= cmt_walls.a.num)
    Z_SetArraySize(&cmt_walls.a, cmt_walls.pos + 1);
  
  cmt_walls.active = true;
  return arr_walls[cmt_walls.pos];
}

drawplane_t *R2_GetDrawPlane(void)
{
  DEV_ASSERT(!cmt_planes.active, ("R2_GetDrawPlane: called twice"));
  
  if (cmt_planes.pos >= cmt_planes.a.num)
    Z_SetArraySize(&cmt_planes.a, cmt_planes.pos + 1);
  
  cmt_planes.active = true;
  return arr_planes[cmt_planes.pos];
}

drawthing_t *R2_GetDrawThing(void)
{
  DEV_ASSERT(!cmt_things.active, ("R2_GetDrawThing: called twice"));
  
  if (cmt_things.pos >= cmt_things.a.num)
    Z_SetArraySize(&cmt_things.a, cmt_things.pos + 1);
    
  cmt_things.active = true;
  return arr_things[cmt_things.pos];
}

drawfloor_t *R2_GetDrawFloor(void)
{
  DEV_ASSERT(!cmt_floors.active, ("R2_GetDrawFloor: called twice"));

  if (cmt_floors.pos >= cmt_floors.a.num)
    Z_SetArraySize(&cmt_floors.a, cmt_floors.pos + 1);
  
  cmt_floors.active = true;
  return arr_floors[cmt_floors.pos];
}

void R2_CommitDrawWall(int used)
{
  DEV_ASSERT(cmt_walls.active, ("R2_CommitDrawWall: not active"));
  DEV_ASSERT2(0 <= used && used <= 1);
  
  cmt_walls.pos += used;
  cmt_walls.active = false;
}

void R2_CommitDrawPlane(int used)
{
  DEV_ASSERT(cmt_planes.active, ("R2_CommitDrawPlane: not active"));
  DEV_ASSERT2(0 <= used && used <= 1);
  
  cmt_planes.pos += used;
  cmt_planes.active = false;
}

void R2_CommitDrawThing(int used)
{
  DEV_ASSERT(cmt_things.active, ("R2_CommitDrawThing: not active"));
  DEV_ASSERT2(0 <= used && used <= 1);
  
  cmt_things.pos += used;
  cmt_things.active = false;
}

void R2_CommitDrawFloor(int used)
{
  DEV_ASSERT(cmt_floors.active, ("R2_CommitDrawFloor: not active"));
  DEV_ASSERT2(0 <= used && used <= 1);
  
  cmt_floors.pos += used;
  cmt_floors.active = false;
}


void R2_FreeupBSP(void)
{
  Z_SetArraySize(&cmt_walls.a,  0);
  Z_SetArraySize(&cmt_planes.a, 0);
  Z_SetArraySize(&cmt_things.a, 0);
  Z_SetArraySize(&cmt_floors.a, 0);
}


//----------------------------------------------------------------------------

//
//  OPENING CODE
//

//
// The openings are kept in blocks of 64K, allocated when needed (and
// by using "Stack Arrays" they get freed when no longer needed).
//
#define OPENING_CHUNK  16384

typedef struct range_array_s
{
  Y_range_t ranges[OPENING_CHUNK];
}
range_array_t;


static stack_array_t range_arrays_a;
static range_array_t ** range_arrays;
static int num_range_arrays;

static range_array_t *free_R_array;
static int free_RA_pos;

static bool RA_active;

//
// R2_InitOpenings
//
// Once-only call to initialise opening system.
//
void R2_InitOpenings(void)
{
  Z_InitStackArray(&range_arrays_a, (void ***)&range_arrays, 
      sizeof(range_array_t), 0);
}

//
// R2_StartOpenings
//
// Called once per frame, before rendering begins.
//
void R2_StartOpenings(void)
{
  num_range_arrays = 1;
  Z_SetArraySize(&range_arrays_a, num_range_arrays);

  // setup the free pointer
  free_R_array = range_arrays[0];
  free_RA_pos  = 0;

  RA_active = false;
}

//
// R2_GetOpenings
//
// Get an array of openings for a new wall/plane/thing or whatever,
// with at most `width' elements.  The returned array is provisional,
// you must call R2_CommitOpenings() sometime later to ensure that
// nothing else uses it.  This routine cannot be called again until
// R2_CommitOpenings is called.  R2_CommitOpenings can be called with
// a lower width (including 0, if no openings were needed).
//
Y_range_t *R2_GetOpenings(int width)
{
  DEV_ASSERT(!RA_active, ("R2_GetOpenings: called twice"));
  DEV_ASSERT2(free_R_array);
  DEV_ASSERT2(0 < width && width <= OPENING_CHUNK);

  // no more room in current chunk ?
  if (width > (OPENING_CHUNK - free_RA_pos))
  {
    num_range_arrays++;
    Z_SetArraySize(&range_arrays_a, num_range_arrays);

    free_R_array = range_arrays[num_range_arrays-1];
    free_RA_pos  = 0;
  }

  RA_active = true;

  return free_R_array->ranges + free_RA_pos;
}

//
// R2_CommitOpenings
//
// (see the comment above).
//
void R2_CommitOpenings(int width)
{
  DEV_ASSERT(RA_active, ("R2_CommitOpenings: not active"));
  DEV_ASSERT2(free_R_array);
  DEV_ASSERT2(0 <= width && width <= OPENING_CHUNK);
  DEV_ASSERT2(width <= (OPENING_CHUNK - free_RA_pos));

  free_RA_pos += width;

  RA_active = false;
}


//----------------------------------------------------------------------------
//
//  1D OCCLUSION BUFFER CODE
//

// NOTE!  temporary code to test new 1D buffer concept

static byte cruddy_1D_buf[2048];

//
// R2_1DOcclusionClear
//
void R2_1DOcclusionClear(int x1, int x2)
{
  DEV_ASSERT2(0 <= x1 && x1 <= x2);

  for (; x1 <= x2; x1++)
    cruddy_1D_buf[x1] = 1;
}

//
// R2_1DOcclusionSet
//
void R2_1DOcclusionSet(int x1, int x2)
{
  DEV_ASSERT2(0 <= x1 && x1 <= x2);

  for (; x1 <= x2; x1++)
    cruddy_1D_buf[x1] = 0;
}

//
// R2_1DOcclusionTest
//
// Test if the range from x1..x2 is completely blocked, returning true
// if it is, otherwise false.
//
bool R2_1DOcclusionTest(int x1, int x2)
{
  DEV_ASSERT2(0 <= x1 && x1 <= x2);

  for (; x1 <= x2; x1++)
    if (cruddy_1D_buf[x1])
      return false;
  
  return true;
}

//
// R2_1DOcclusionTestShrink
//
// Similiar to the above routine, but shrinks the given range as much
// as possible.  Returns true if totally occluded.
//
bool R2_1DOcclusionTestShrink(int *x1, int *x2)
{
  DEV_ASSERT2(0 <= (*x1) && (*x1) <= (*x2));

  for (; (*x1) <= (*x2); (*x1)++)
    if (cruddy_1D_buf[*x1])
      break;

  for (; (*x1) <= (*x2); (*x2)--)
    if (cruddy_1D_buf[*x2])
      break;
  
  return (*x1) > (*x2) ? true : false;
}

//
// R2_1DOcclusionClose
//
void R2_1DOcclusionClose(int x1, int x2, Y_range_t *ranges)
{
  int i;

  DEV_ASSERT2(0 <= x1 && x1 <= x2);

  for (i=x1; i <= x2; i++)
  {
    if (cruddy_1D_buf[i] == 0)
    {
      ranges[i - x1].y1 = 1;
      ranges[i - x1].y2 = 0;
    }
  }
}


//----------------------------------------------------------------------------
//
//  2D OCCLUSION BUFFER CODE
//

Y_range_t Screen_clip[2048];

//
// R2_2DOcclusionClear
//
void R2_2DOcclusionClear(int x1, int x2)
{
  DEV_ASSERT2(0 <= x1 && x1 <= x2);

  for (; x1 <= x2; x1++)
  {
    Screen_clip[x1].y1 = 0;
    Screen_clip[x1].y2 = viewheight-1;
  }
}

//
// R2_2DOcclusionClose
//
void R2_2DOcclusionClose(int x1, int x2, Y_range_t *ranges,
    bool connect_low, bool connect_high, bool solid)
{
  int i;

  DEV_ASSERT2(0 <= x1 && x1 <= x2);

  for (i=x1; i <= x2; i++)
  {
    short y1 = ranges[i - x1].y1;
    short y2 = ranges[i - x1].y2;

    if (y1 > y2)
      continue;

    ranges[i - x1].y1 = MAX(y1, Screen_clip[i].y1);
    ranges[i - x1].y2 = MIN(y2, Screen_clip[i].y2);
  
    if (connect_low || (solid && y2 >= Screen_clip[i].y2))
      Screen_clip[i].y2 = MIN(Screen_clip[i].y2, y1-1);

    if (connect_high || (solid && y1 <= Screen_clip[i].y1))
      Screen_clip[i].y1 = MAX(Screen_clip[i].y1, y2+1);
  }
}

//
// R2_2DOcclusionCopy
//
// Copy the 1D/2D occlusion buffer.
//
void R2_2DOcclusionCopy(int x1, int x2, Y_range_t *ranges)
{
  int i;

  DEV_ASSERT2(0 <= x1 && x1 <= x2);

  for (i=x1; i <= x2; i++)
  {
    if (cruddy_1D_buf[i] == 0)
    {
      ranges[i - x1].y1 = 1;
      ranges[i - x1].y2 = 0;
    }
    else
    {
      ranges[i - x1].y1 = Screen_clip[i].y1;
      ranges[i - x1].y2 = Screen_clip[i].y2;
    }
  }
}

//
// R2_2DUpdate1D
//
// Check the 2D occlusion buffer for complete closure, updating the 1D
// occlusion buffer where necessary.
//
void R2_2DUpdate1D(int x1, int x2)
{
  int i;

  DEV_ASSERT2(0 <= x1 && x1 <= x2);

  for (i=x1; i <= x2; i++)
  {
    if (Screen_clip[i].y1 > Screen_clip[i].y2)
      cruddy_1D_buf[i] = 0;
  }
}


//----------------------------------------------------------------------------
//
//  TILE SKY UTILITIES
//

tilesky_t sky_tiles[4];
int sky_tiles_active = 0;

void R2_TileSkyClear(void)
{
  memset(sky_tiles, 0, sizeof(sky_tiles));
  sky_tiles_active = 0;
}

void R2_TileSkyAdd(const tilesky_info_t *info, struct line_s *line)
{
  int layer = info->layer - 1;

  if (layer < 0 || layer >= MAX_TILESKY)
    return;

  // update count of active sky tiles
  if (! sky_tiles[layer].active)
    sky_tiles_active++;

  sky_tiles[layer].info = info;
  sky_tiles[layer].line = line;
  sky_tiles[layer].active = true;

  L_WriteDebug("SKY TILE %d ACTIVE (total active %d)\n",
      layer, sky_tiles_active);
}


//----------------------------------------------------------------------------
//
//  LEVEL-OF-DETAIL STUFF
//
//  These functions return a "LOD" number.  1 means the maximum
//  detail, 2 is half the detail, 4 is a quarter, etc...
//

int lod_base_cube = 256;


//
// R2_GetPointLOD
//
int R2_GetPointLOD(float x, float y, float z)
{
  x = fabs(x - viewx);
  y = fabs(y - viewy);
  z = fabs(z - viewz);

  x = MAX(x, MAX(y, z));

  return 1 + (int)floor(x / lod_base_cube);
}

//
// R2_GetBBoxLOD
//
int R2_GetBBoxLOD(float x1, float y1, float z1,
    float x2, float y2, float z2)
{
  x1 -= viewx;  x2 -= viewx;
  y1 -= viewy;  y2 -= viewy;
  z1 -= viewz;  z2 -= viewz;

  // check signs to handle BBOX crossing the axis
  x1 = ((x1<0) != (x2<0)) ? 0 : MIN(fabs(x1), fabs(x2));
  y1 = ((y1<0) != (y2<0)) ? 0 : MIN(fabs(y1), fabs(y2));
  z1 = ((z1<0) != (z2<0)) ? 0 : MIN(fabs(z1), fabs(z2));
  
  x1 = MAX(x1, MAX(y1, z1));

  return 1 + (int)floor(x1 / lod_base_cube);
}

//
// R2_GetWallLOD
//
// Note: code is currently the same as in R2_GetBBoxLOD above, though
// this could be improved to handle diagonal lines better (at the
// moment we just use the line's bbox).
//
int R2_GetWallLOD(float x1, float y1, float z1,
    float x2, float y2, float z2)
{
  x1 -= viewx;  x2 -= viewx;
  y1 -= viewy;  y2 -= viewy;
  z1 -= viewz;  z2 -= viewz;

  // check signs to handle BBOX crossing the axis
  x1 = ((x1<0) != (x2<0)) ? 0 : MIN(fabs(x1), fabs(x2));
  y1 = ((y1<0) != (y2<0)) ? 0 : MIN(fabs(y1), fabs(y2));
  z1 = ((z1<0) != (z2<0)) ? 0 : MIN(fabs(z1), fabs(z2));
  
  x1 = MAX(x1, MAX(y1, z1));

  return 1 + (int)floor(x1 / lod_base_cube);
}

//
// R2_GetPlaneLOD
//
int R2_GetPlaneLOD(subsector_t *sub, float h)
{
  // get BBOX of plane
  float x1 = sub->bbox[BOXLEFT]   - viewx;
  float x2 = sub->bbox[BOXRIGHT]  - viewx;
  float y1 = sub->bbox[BOXBOTTOM] - viewy;
  float y2 = sub->bbox[BOXTOP]    - viewy;

  x1 = ((x1<0) != (x2<0)) ? 0 : MIN(fabs(x1), fabs(x2));
  y1 = ((y1<0) != (y2<0)) ? 0 : MIN(fabs(y1), fabs(y2));

  h = MAX(fabs(h - viewz), MAX(x1, y1));

  return 1 + (int)floor(h / lod_base_cube);
}

