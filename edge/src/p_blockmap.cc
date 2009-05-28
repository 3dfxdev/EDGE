//----------------------------------------------------------------------------
//  EDGE Blockmap utility functions
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2008  The EDGE Team.
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

#include "i_defs.h"
#include "i_defs_gl.h"  // needed for r_shader.h

#include <float.h>

#include <vector>
#include <algorithm>

#include "dm_data.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "m_bbox.h"
#include "p_local.h"
#include "p_spec.h"
#include "r_shader.h"
#include "r_state.h"
#include "z_zone.h"


// FIXME: have a proper API
extern abstract_shader_c *MakeDLightShader(mobj_t *mo);
extern abstract_shader_c *MakePlaneGlow(mobj_t *mo);


#define MAXRADIUS  128.0

// BLOCKMAP
//
// Created from axis aligned bounding box
// of the map, a rectangular array of
// blocks of size ...
// Used to speed up collision detection
// by spatial subdivision in 2D.
//
// Blockmap size.
// 23-6-98 KM Promotion of short * to int *
int bmap_width;
int bmap_height;  // size in mapblocks

unsigned short *  bmap_lines = NULL; 
unsigned short ** bmap_pointers = NULL;

// offsets in blockmap are from here
int *blockmaplump = NULL;

// origin of block map
float bmap_orgx;
float bmap_orgy;

// for thing chains
mobj_t **bmap_things = NULL;

// for dynamic lights
int dlmap_width;
int dlmap_height;

mobj_t **dlmap_things = NULL;


void P_CreateThingBlockMap(void)
{
	bmap_things  = new mobj_t* [bmap_width * bmap_height];

	Z_Clear(bmap_things,  mobj_t*, bmap_width * bmap_height);

	// compute size of dynamic light blockmap
	dlmap_width  = (bmap_width  * BLOCKMAP_UNIT + LIGHTMAP_UNIT-1) / LIGHTMAP_UNIT;
	dlmap_height = (bmap_height * BLOCKMAP_UNIT + LIGHTMAP_UNIT-1) / LIGHTMAP_UNIT;

	I_Debugf("Blockmap size: %dx%d --> Lightmap size: %dx%x\n",
			 bmap_width, bmap_height, dlmap_width, dlmap_height);

	dlmap_things = new mobj_t* [dlmap_width * dlmap_height];

	Z_Clear(dlmap_things, mobj_t*, dlmap_width * dlmap_height);
}

void P_DestroyBlockMap(void)
{
	delete[] bmap_lines;    bmap_lines  = NULL;
	delete[] bmap_pointers; bmap_pointers = NULL;
	delete[] bmap_things;   bmap_things = NULL;

	delete[] dlmap_things;  dlmap_things = NULL;
}


//--------------------------------------------------------------------------
//
//  THING POSITION SETTING
//

// stats
#ifdef DEVELOPERS
int touchstat_moves;
int touchstat_hit;
int touchstat_miss;
int touchstat_alloc;
int touchstat_free;
#endif

// quick-alloc list
// FIXME: incorporate into FlushCaches
touch_node_t *free_touch_nodes;

static inline touch_node_t *TN_Alloc(void)
{
	touch_node_t *tn;

#ifdef DEVELOPERS
	touchstat_alloc++;
#endif

	if (free_touch_nodes)
	{
		tn = free_touch_nodes;
		free_touch_nodes = tn->mo_next;
	}
	else
	{
		tn = Z_New(touch_node_t, 1);
	}

	return tn;
}

static inline void TN_Free(touch_node_t *tn)
{
#ifdef DEVELOPERS
	touchstat_free++;
#endif

	// PREV field is ignored in quick-alloc list
	tn->mo_next = free_touch_nodes;
	free_touch_nodes = tn;
}

static inline void TN_LinkIntoSector(touch_node_t *tn, sector_t *sec)
{
	tn->sec = sec;

	tn->sec_next = sec->touch_things;
	tn->sec_prev = NULL;

	if (tn->sec_next)
		tn->sec_next->sec_prev = tn;

	sec->touch_things = tn;
}

static inline void TN_LinkIntoThing(touch_node_t *tn, mobj_t *mo)
{
	tn->mo = mo;

	tn->mo_next = mo->touch_sectors;
	tn->mo_prev = NULL;

	if (tn->mo_next)
		tn->mo_next->mo_prev = tn;

	mo->touch_sectors = tn;
}

static inline void TN_UnlinkFromSector(touch_node_t *tn)
{
	if (tn->sec_next)
		tn->sec_next->sec_prev = tn->sec_prev;

	if (tn->sec_prev)
		tn->sec_prev->sec_next = tn->sec_next;
	else
		tn->sec->touch_things = tn->sec_next;
}

static inline void TN_UnlinkFromThing(touch_node_t *tn)
{
	if (tn->mo_next)
		tn->mo_next->mo_prev = tn->mo_prev;

	if (tn->mo_prev)
		tn->mo_prev->mo_next = tn->mo_next;
	else
		tn->mo->touch_sectors = tn->mo_next;
}

typedef struct setposbsp_s
{
	mobj_t *mo;

	float bbox[4];
}
setposbsp_t;


static void PerformSectorLinkage(mobj_t *mo, sector_t *sec)
{
#ifdef DEVELOPERS
	touchstat_miss++;
#endif

	touch_node_t *tn;

	for (tn = mo->touch_sectors; tn; tn = tn->mo_next)
	{
		if (! tn->mo)
		{
			// found unused touch node.  We reuse it.
			tn->mo = mo;

			if (tn->sec != sec)
			{
				TN_UnlinkFromSector(tn);
				TN_LinkIntoSector(tn, sec);
			}
#ifdef DEVELOPERS
			else
			{
				touchstat_miss--;
				touchstat_hit++;
			}
#endif

			return;
		}

		SYS_ASSERT(tn->mo == mo);

		// sector already present ?
		if (tn->sec == sec)
			return;
	}

	// need to allocate a new touch node
	tn = TN_Alloc();

	TN_LinkIntoThing(tn,  mo);
	TN_LinkIntoSector(tn, sec);
}

static bool PIT_GetSectors(line_t *ld, void *data)
{
	setposbsp_t *pos = (setposbsp_t *) data;

	if (P_BoxOnLineSide(pos->bbox, ld) == -1)
	{
		PerformSectorLinkage(pos->mo, ld->frontsector);

		if (ld->backsector && ld->backsector != ld->frontsector)
			PerformSectorLinkage(pos->mo, ld->backsector);
	}

	return true;
}

//
// P_UnsetThingPosition
//
// Unlinks a thing from block map and subsector.
// On each position change, BLOCKMAP and other
// lookups maintaining lists ot things inside
// these structures need to be updated.
//
// -ES- 1999/12/04 Better error checking: Clear prev/next fields.
// This catches errors which can occur if the position is unset twice.
//
void P_UnsetThingPosition(mobj_t * mo)
{
	int blockx;
	int blocky;
	int bnum;

	touch_node_t *tn;

	// unlink from subsector
	if (!(mo->flags & MF_NOSECTOR))
	{
		// (inert things don't need to be in subsector list)

	}

	// unlink from touching list.
	// NOTE: lazy unlinking -- see notes in r_defs.h
	//
	for (tn = mo->touch_sectors; tn; tn = tn->mo_next)
	{
		tn->mo = NULL;
	}

	// unlink from blockmap
	if (!(mo->flags & MF_NOBLOCKMAP))
	{
		// inert things don't need to be in blockmap
		if (mo->bnext)
		{
			SYS_ASSERT(mo->bnext->bprev == mo);

			mo->bnext->bprev = mo->bprev;
		}

		if (mo->bprev)
		{
			SYS_ASSERT(mo->bprev->bnext == mo);

			mo->bprev->bnext = mo->bnext;
		}
		else
		{
			blockx = BLOCKMAP_GET_X(mo->x);
			blocky = BLOCKMAP_GET_Y(mo->y);

			if (blockx >= 0 && blockx < bmap_width &&
				blocky >= 0 && blocky < bmap_height)
			{
				bnum = blocky * bmap_width + blockx;

				SYS_ASSERT(bmap_things[bnum] == mo);

				bmap_things[bnum] = mo->bnext;
			}
		}

		mo->bprev = NULL;
		mo->bnext = NULL;
	}

	// unlink from dynamic light blockmap
	if (mo->info && (mo->info->dlight[0].type != DLITE_None) &&
		(mo->info->glow_type == GLOW_None))
	{
		if (mo->dlnext)
		{
			SYS_ASSERT(mo->dlnext->dlprev == mo);

			mo->dlnext->dlprev = mo->dlprev;
		}

		if (mo->dlprev)
		{
			SYS_ASSERT(mo->dlprev->dlnext == mo);

			mo->dlprev->dlnext = mo->dlnext;
		}
		else
		{
			blockx = LIGHTMAP_GET_X(mo->x);
			blocky = LIGHTMAP_GET_Y(mo->y);

			if (blockx >= 0 && blockx < dlmap_width &&
				blocky >= 0 && blocky < dlmap_height)
			{
				bnum = blocky * dlmap_width + blockx;

				SYS_ASSERT(dlmap_things[bnum] == mo);
				dlmap_things[bnum] = mo->dlnext;
			}
		}

		mo->dlprev = NULL;
		mo->dlnext = NULL;
	}

	// unlink from sector glow list
	if (mo->info && (mo->info->dlight[0].type != DLITE_None) &&
		(mo->info->glow_type != GLOW_None))
	{
		sector_t *sec = mo->subsector->sector;

		if (mo->dlnext)
		{
			SYS_ASSERT(mo->dlnext->dlprev == mo);

			mo->dlnext->dlprev = mo->dlprev;
		}

		if (mo->dlprev)
		{
			SYS_ASSERT(mo->dlprev->dlnext == mo);

			mo->dlprev->dlnext = mo->dlnext;
		}
		else
		{
			SYS_ASSERT(sec->glow_things == mo);

			sec->glow_things = mo->dlnext;
		}

		mo->dlprev = NULL;
		mo->dlnext = NULL;
	}
}

//
// P_UnsetThingFinally
// 
// Call when the thing is about to be removed for good.
// 
void P_UnsetThingFinally(mobj_t * mo)
{
	P_UnsetThingPosition(mo);

	// clear out touch nodes

	while (mo->touch_sectors)
	{
		touch_node_t *tn = mo->touch_sectors;
		mo->touch_sectors = tn->mo_next;

		TN_UnlinkFromSector(tn);
		TN_Free(tn);
	}
}

//
// P_SetThingPosition
//
// Links a thing into both a block and a subsector
// based on it's x y.
//
void P_SetThingPosition(mobj_t * mo)
{
	int blockx;
	int blocky;
	int bnum;

	setposbsp_t pos;
	touch_node_t *tn;

	// -ES- 1999/12/04 The position must be unset before it's set again.
	if (mo->bnext || mo->bprev)
		I_Error("INTERNAL ERROR: Double P_SetThingPosition call.");

	SYS_ASSERT(! (mo->dlnext || mo->dlprev));

	// link into subsector
	subsector_t *ss = R_PointInSubsector(mo->x, mo->y);
	mo->subsector = ss;

	// determine properties
	mo->props = R_PointGetProps(ss, mo->z + mo->height/2);

	///  if (! (mo->flags & MF_NOSECTOR)) add_to_subsector

	// link into touching list

#ifdef DEVELOPERS
	touchstat_moves++;
#endif

	pos.mo = mo;
	pos.bbox[BOXLEFT]   = mo->x - mo->radius;
	pos.bbox[BOXRIGHT]  = mo->x + mo->radius;
	pos.bbox[BOXBOTTOM] = mo->y - mo->radius;
	pos.bbox[BOXTOP]    = mo->y + mo->radius;

	P_BlockLinesIterator(mo->x - mo->radius, mo->y - mo->radius,
	                     mo->x + mo->radius, mo->y + mo->radius,
						 PIT_GetSectors, &pos),

	PerformSectorLinkage(mo, mo->subsector->sector);

	// handle any left-over unused touch nodes

	for (tn = mo->touch_sectors; tn && tn->mo; tn = tn->mo_next)
	{ /* nothing here */ }

	if (tn)
	{
		if (tn->mo_prev)
			tn->mo_prev->mo_next = NULL;
		else
			mo->touch_sectors = NULL;

		while (tn)
		{
			touch_node_t *cur = tn;
			tn = tn->mo_next;

			SYS_ASSERT(! cur->mo);

			TN_UnlinkFromSector(cur);
			TN_Free(cur);
		}
	}

#if 0  // PROFILING
	{
		static int last_time = 0;

		if ((leveltime - last_time) > 5*TICRATE)
		{
			L_WriteDebug("TOUCHSTATS: Mv=%d Ht=%d Ms=%d Al=%d Fr=%d\n",
				touchstat_moves, touchstat_hit, touchstat_miss,
				touchstat_alloc, touchstat_free);

			touchstat_moves = touchstat_hit = touchstat_miss =
				touchstat_alloc = touchstat_free = 0;

			last_time = leveltime;
		}
	}
#endif

	// link into blockmap
	if (!(mo->flags & MF_NOBLOCKMAP))
	{
		blockx = BLOCKMAP_GET_X(mo->x);
		blocky = BLOCKMAP_GET_Y(mo->y);

		if (blockx >= 0 && blockx < bmap_width &&
			blocky >= 0 && blocky < bmap_height)
		{
			bnum = blocky * bmap_width + blockx;

			mo->bprev = NULL;
			mo->bnext = bmap_things[bnum];

			if (bmap_things[bnum])
				(bmap_things[bnum])->bprev = mo;

			bmap_things[bnum] = mo;
		}
		else
		{
			// thing is off the map
			mo->bnext = mo->bprev = NULL;
		}
	}

	// link into dynamic light blockmap
	if (mo->info && (mo->info->dlight[0].type != DLITE_None) &&
		(mo->info->glow_type == GLOW_None))
	{
		blockx = LIGHTMAP_GET_X(mo->x);
		blocky = LIGHTMAP_GET_Y(mo->y);

		if (blockx >= 0 && blockx < dlmap_width &&
			blocky >= 0 && blocky < dlmap_height)
		{
			bnum = blocky * dlmap_width + blockx;

			mo->dlprev = NULL;
			mo->dlnext = dlmap_things[bnum];

			if (dlmap_things[bnum])
				(dlmap_things[bnum])->dlprev = mo;

			dlmap_things[bnum] = mo;
		}
		else
		{
			// thing is off the map
			mo->dlnext = mo->dlprev = NULL;
		}
	}

	// link into sector glow list
	if (mo->info && (mo->info->dlight[0].type != DLITE_None) &&
		(mo->info->glow_type != GLOW_None))
	{
		sector_t *sec = mo->subsector->sector;

		mo->dlprev = NULL;
		mo->dlnext = sec->glow_things;

		sec->glow_things = mo;
	}
}

//
//
// New routine to "atomicly" move a thing.  Apart from object
// construction and destruction, this routine should always be called
// when moving a thing, rather than fiddling with the coordinates
// directly (or even P_UnsetThingPos/P_SetThingPos pairs).
// 
void P_ChangeThingPosition(mobj_t * mo, float x, float y, float z)
{
	P_UnsetThingPosition(mo);
	{
		mo->x = x;
		mo->y = y;
		mo->z = z;
	}
	P_SetThingPosition(mo);
}


void P_FreeSectorTouchNodes(sector_t *sec)
{
	touch_node_t *tn;

	for (tn = sec->touch_things; tn; tn = tn->sec_next)
		TN_Free(tn);
}



//--------------------------------------------------------------------------
//
// BLOCK MAP ITERATORS
//
// For each line/thing in the given mapblock,
// call the passed PIT_* function.
// If the function returns false,
// exit with false without checking anything else.
//

//
// P_BlockLinesIterator
//
// The validcount flags are used to avoid checking lines
// that are marked in multiple mapblocks,
// so increment validcount before the first call
// to P_BlockLinesIterator, then make one or more calls
// to it.
//
bool P_BlockLinesIterator(float x1, float y1, float x2, float y2,
		                  bool(* func)(line_t *, void *), void *data)
{
	validcount++;

	int lx = BLOCKMAP_GET_X(x1);
	int ly = BLOCKMAP_GET_Y(y1);
	int hx = BLOCKMAP_GET_X(x2);
	int hy = BLOCKMAP_GET_Y(y2);

	lx = MAX(0, lx);  hx = MIN(bmap_width-1,  hx);
	ly = MAX(0, ly);  hy = MIN(bmap_height-1, hy);

	for (int by = ly; by <= hy; by++)
	for (int bx = lx; bx <= hx; bx++)
	{
		unsigned short *list = bmap_pointers[by * bmap_width + bx];

		for (; *list != BMAP_END; list++)
		{
			line_t *ld = &lines[*list];

			// has line already been checked ?
			if (ld->validcount == validcount)
				continue;

			ld->validcount = validcount;

			// check whether line touches the given bbox
			if (ld->bbox[BOXRIGHT] <= x1 || ld->bbox[BOXLEFT]   >= x2 ||
				ld->bbox[BOXTOP]   <= y1 || ld->bbox[BOXBOTTOM] >= y2)
			{
				continue;
			}

			if (! func(ld, data))
				return false;
		}
	}

	// everything was checked
	return true;
}


bool P_BlockThingsIterator(float x1, float y1, float x2, float y2,
		                   bool (*func)(mobj_t *, void*), void *data)
{
	// need to expand the source by one block because large
	// things (radius limited to BLOCKMAP_UNIT) can overlap
	// into adjacent blocks.

	int lx = BLOCKMAP_GET_X(x1) - 1;
	int ly = BLOCKMAP_GET_Y(y1) - 1;
	int hx = BLOCKMAP_GET_X(x2) + 1;
	int hy = BLOCKMAP_GET_Y(y2) + 1;

	lx = MAX(0, lx);  hx = MIN(bmap_width-1,  hx);
	ly = MAX(0, ly);  hy = MIN(bmap_height-1, hy);

	for (int by = ly; by <= hy; by++)
	for (int bx = lx; bx <= hx; bx++)
	{
		for (mobj_t *mo = bmap_things[by * bmap_width + bx]; mo; mo = mo->bnext)
		{
			// check whether thing touches the given bbox
			float r = mo->radius;

			if (mo->x + r <= x1 || mo->x - r >= x2 ||
			    mo->y + r <= y1 || mo->y - r >= y2)
				continue;

			if (! func(mo, data))
				return false;
		}
	}

	return true;
}


void P_DynamicLightIterator(float x1, float y1, float z1,
		                    float x2, float y2, float z2,
		                    void (*func)(mobj_t *, void *), void *data)
{
	int lx = LIGHTMAP_GET_X(x1) - 1;
	int ly = LIGHTMAP_GET_Y(y1) - 1;
	int hx = LIGHTMAP_GET_X(x2) + 1;
	int hy = LIGHTMAP_GET_Y(y2) + 1;

	lx = MAX(0, lx);  hx = MIN(dlmap_width-1,  hx);
	ly = MAX(0, ly);  hy = MIN(dlmap_height-1, hy);

	for (int by = ly; by <= hy; by++)
	for (int bx = lx; bx <= hx; bx++)
	{
		for (mobj_t *mo = dlmap_things[by * dlmap_width + bx]; mo; mo = mo->dlnext)
		{
			SYS_ASSERT(mo->state);

			// skip "off" lights
			if (mo->state->bright <= 0 || mo->dlight.r <= 0)
				continue;

			// check whether radius touches the given bbox
			float r = mo->dlight.r;

			if (mo->x + r <= x1 || mo->x - r >= x2 ||
			    mo->y + r <= y1 || mo->y - r >= y2 ||
				mo->z + r <= z1 || mo->z - r >= z2)
				continue;
			
			// create shader if necessary
			if (! mo->dlight.shader)
				  mo->dlight.shader = MakeDLightShader(mo);

//			mo->dlight.shader->CheckReset();

			func(mo, data);
		}
	}
}


void P_SectorGlowIterator(sector_t *sec,
	                   	  float x1, float y1, float z1,
	                   	  float x2, float y2, float z2,
		                  void (*func)(mobj_t *, void *), void *data)
{
	for (mobj_t *mo = sec->glow_things; mo; mo = mo->dlnext)
	{
		SYS_ASSERT(mo->state);

		// skip "off" lights
		if (mo->state->bright <= 0 || mo->dlight.r <= 0)
			continue;

		// check whether radius touches the given bbox
		float r = mo->dlight.r;

		if (mo->info->glow_type == GLOW_Floor && sec->f_h + r <= z1)
			continue;
		
		if (mo->info->glow_type == GLOW_Ceiling && sec->c_h - r >= z1)
			continue;
		
		// create shader if necessary
		if (! mo->dlight.shader)
			  mo->dlight.shader = MakePlaneGlow(mo);

//		mo->dlight.shader->CheckReset();

		func(mo, data);
	}
}


//--------------------------------------------------------------------------
//
// INTERCEPT ROUTINES
//

static std::vector<intercept_t> intercepts;

divline_t trace;


float P_InterceptVector(divline_t * v2, divline_t * v1)
{
	// Returns the fractional intercept point along the first divline.
	// This is only called by the addthings and addlines traversers.

	float den = (v1->dy * v2->dx) - (v1->dx * v2->dy);

	if (den == 0)
		return 0;  // parallel

	float num = (v1->x - v2->x) * v1->dy + (v2->y - v1->y) * v1->dx;

	return num / den;
}


static inline void PIT_AddLineIntercept(line_t * ld)
{
	// Looks for lines in the given block
	// that intercept the given trace
	// to add to the intercepts list.
	//
	// A line is crossed if its endpoints
	// are on opposite sides of the trace.
	// Returns true if earlyout and a solid line hit.

	// has line already been checked ?
	if (ld->validcount == validcount)
		return;

	ld->validcount = validcount;

	int s1;
	int s2;
	float frac;
	divline_t div;

	div.x = ld->v1->x;
	div.y = ld->v1->y;
	div.dx = ld->dx;
	div.dy = ld->dy;

	// avoid precision problems with two routines
	if (trace.dx > 16 || trace.dy > 16 || trace.dx < -16 || trace.dy < -16)
	{
		s1 = P_PointOnDivlineSide(ld->v1->x, ld->v1->y, &trace);
		s2 = P_PointOnDivlineSide(ld->v2->x, ld->v2->y, &trace);
	}
	else
	{
		s1 = P_PointOnDivlineSide(trace.x, trace.y, &div);
		s2 = P_PointOnDivlineSide(trace.x + trace.dx, trace.y + trace.dy, &div);
	}

	// line isn't crossed ?
	if (s1 == s2)
		return;

	// hit the line

	frac = P_InterceptVector(&trace, &div);

	// out of range?
	if (frac < 0 || frac > 1)
		return;

	// Intercept is a simple struct that can be memcpy()'d: Load
	// up a structure and get into the array
	intercept_t in;

	in.frac  = frac;
	in.thing = NULL;
	in.line  = ld;

	intercepts.push_back(in);
}

static inline void PIT_AddThingIntercept(mobj_t * thing)
{
	float x1;
	float y1;
	float x2;
	float y2;

	int s1;
	int s2;

	bool tracepositive;

	divline_t div;

	float frac;

	tracepositive = (trace.dx >= 0) == (trace.dy >= 0);

	// check a corner to corner crossection for hit
	if (tracepositive)
	{
		x1 = thing->x - thing->radius;
		y1 = thing->y + thing->radius;

		x2 = thing->x + thing->radius;
		y2 = thing->y - thing->radius;
	}
	else
	{
		x1 = thing->x - thing->radius;
		y1 = thing->y - thing->radius;

		x2 = thing->x + thing->radius;
		y2 = thing->y + thing->radius;
	}

	s1 = P_PointOnDivlineSide(x1, y1, &trace);
	s2 = P_PointOnDivlineSide(x2, y2, &trace);

	// line isn't crossed ?
	if (s1 == s2)
		return;

	div.x = x1;
	div.y = y1;
	div.dx = x2 - x1;
	div.dy = y2 - y1;

	frac = P_InterceptVector(&trace, &div);

	// out of range?
	if (frac < 0 || frac > 1)
		return;

	// Intercept is a simple struct that can be memcpy()'d: Load
	// up a structure and get into the array
	intercept_t in;

	in.frac  = frac;
	in.thing = thing;
	in.line  = NULL;

	intercepts.push_back(in);
}


struct Compare_Intercept_pred
{
	inline bool operator() (const intercept_t& A, const intercept_t& B) const
	{
		return A.frac < B.frac;
	}
};

//
// P_PathTraverse
//
// Traces a line from x1,y1 to x2,y2,
// calling the traverser function for each.
// Returns true if the traverser function returns true
// for all lines.
//
bool P_PathTraverse(float x1, float y1, float x2, float y2, int flags,
		            bool (* func)(intercept_t *, void *), void *data)
{
	validcount++;

	intercepts.clear();

	// don't side exactly on a line
	if (fmod(x1 - bmap_orgx, BLOCKMAP_UNIT) == 0)
		x1 += 0.1f;

	if (fmod(y1 - bmap_orgy, BLOCKMAP_UNIT) == 0)
		y1 += 0.1f;

	trace.x = x1;
	trace.y = y1;
	trace.dx = x2 - x1;
	trace.dy = y2 - y1;

	x1 -= bmap_orgx;
	y1 -= bmap_orgy;
	x2 -= bmap_orgx;
	y2 -= bmap_orgy;

	int bx1 = (int)(x1 / BLOCKMAP_UNIT);
	int by1 = (int)(y1 / BLOCKMAP_UNIT);
	int bx2 = (int)(x2 / BLOCKMAP_UNIT);
	int by2 = (int)(y2 / BLOCKMAP_UNIT);

	int bx_step;
	int by_step;

	float xstep;
	float ystep;

	float partial;

	double tmp;


	if (bx2 > bx1 && (x2 - x1) > 0.001)
	{
		bx_step = 1;
		partial = 1.0f - modf(x1 / BLOCKMAP_UNIT, &tmp);

		ystep = (y2 - y1) / fabs(x2 - x1);
	}
	else if (bx2 < bx1 && (x2 - x1) < -0.001)
	{
		bx_step = -1;
		partial = modf(x1 / BLOCKMAP_UNIT, &tmp);

		ystep = (y2 - y1) / fabs(x2 - x1);
	}
	else
	{
		bx_step = 0;
		partial = 1.0f;
		ystep = 256.0f;
	}

	float yintercept = y1 / BLOCKMAP_UNIT + partial * ystep;

	if (by2 > by1 && (y2 - y1) > 0.001)
	{
		by_step = 1;
		partial = 1.0f - modf(y1 / BLOCKMAP_UNIT, &tmp);

		xstep = (x2 - x1) / fabs(y2 - y1);
	}
	else if (by2 < by1 && (y2 - y1) < -0.001)
	{
		by_step = -1;
		partial = modf(y1 / BLOCKMAP_UNIT, &tmp);

		xstep = (x2 - x1) / fabs(y2 - y1);
	}
	else
	{
		by_step = 0;
		partial = 1.0f;
		xstep = 256.0f;
	}

	float xintercept = x1 / BLOCKMAP_UNIT + partial * xstep;

	// Step through map blocks.
	// Count is present to prevent a round off error
	// from skipping the break.
	int bx = bx1;
	int by = by1;

	for (int count = 0; count < 64; count++)
	{
		if (0 <= bx && bx < bmap_width &&
			0 <= by && by < bmap_height)
		{
			if (flags & PT_ADDLINES)
			{
				unsigned short *list = bmap_pointers[by * bmap_width + bx];

				for (; *list != BMAP_END; list++)
				{
					PIT_AddLineIntercept(&lines[*list]);
				}
			}

			if (flags & PT_ADDTHINGS)
			{
				for (mobj_t *mo = bmap_things[by * bmap_width + bx]; mo; mo = mo->bnext)
				{
					PIT_AddThingIntercept(mo);
				}
			}
		}

		if (bx == bx2 && by == by2)
			break;

		if (by == (int)yintercept)
		{
			yintercept += ystep;
			bx += bx_step;
		}
		else if (bx == (int)xintercept)
		{
			xintercept += xstep;
			by += by_step;
		}
	}

	// go through the sorted list

	if (intercepts.size() == 0)
		return true;

	std::sort(intercepts.begin(), intercepts.end(),
			  Compare_Intercept_pred());

	std::vector<intercept_t>::iterator I;

	for (I = intercepts.begin(); I != intercepts.end(); I++)
	{
		if (! func(& *I, data))
		{
			// don't bother going further
			return false;
		}
	}

	// everything was traversed
	return true;
}


//--------------------------------------------------------------------------
//
//  BLOCKMAP GENERATION
//

//  Variables for GenerateBlockMap

// fixed array of dynamic array
static unsigned short ** blk_cur_lines = NULL;
static int blk_total_lines;

#define BCUR_SIZE   0
#define BCUR_MAX    1
#define BCUR_FIRST  2

static void BlockAdd(int bnum, unsigned short line_num)
{
	unsigned short *cur = blk_cur_lines[bnum];

	SYS_ASSERT(bnum >= 0);
	SYS_ASSERT(bnum < (bmap_width * bmap_height));

	if (! cur)
	{
		// create initial block

		blk_cur_lines[bnum] = cur = Z_New(unsigned short, BCUR_FIRST + 16);
		cur[BCUR_SIZE] = 0;
		cur[BCUR_MAX]  = 16;
	}

	if (cur[BCUR_SIZE] == cur[BCUR_MAX])
	{
		// need to grow this block

		cur = blk_cur_lines[bnum];
		cur[BCUR_MAX] += 16;
		Z_Resize(blk_cur_lines[bnum], unsigned short,
			BCUR_FIRST + cur[BCUR_MAX]);
		cur = blk_cur_lines[bnum];
	}

	SYS_ASSERT(cur);
	SYS_ASSERT(cur[BCUR_SIZE] < cur[BCUR_MAX]);

	cur[BCUR_FIRST + cur[BCUR_SIZE]] = line_num;
	cur[BCUR_SIZE] += 1;

	blk_total_lines++;
}

static void BlockAddLine(int line_num)
{
	int i, j;
	int x0, y0;
	int x1, y1;

	line_t *ld = lines + line_num;

	int blocknum;

	int y_sign;
	int x_dist, y_dist;

	float slope;

	x0 = (int)(ld->v1->x - bmap_orgx);
	y0 = (int)(ld->v1->y - bmap_orgy);
	x1 = (int)(ld->v2->x - bmap_orgx);
	y1 = (int)(ld->v2->y - bmap_orgy);

	// swap endpoints if horizontally backward
	if (x1 < x0)
	{
		int temp;

		temp = x0; x0 = x1; x1 = temp;
		temp = y0; y0 = y1; y1 = temp;
	}

	SYS_ASSERT(0 <= x0 && (x0 / BLOCKMAP_UNIT) < bmap_width);
	SYS_ASSERT(0 <= y0 && (y0 / BLOCKMAP_UNIT) < bmap_height);
	SYS_ASSERT(0 <= x1 && (x1 / BLOCKMAP_UNIT) < bmap_width);
	SYS_ASSERT(0 <= y1 && (y1 / BLOCKMAP_UNIT) < bmap_height);

	// check if this line spans multiple blocks.

	x_dist = ABS((x1 / BLOCKMAP_UNIT) - (x0 / BLOCKMAP_UNIT));
	y_dist = ABS((y1 / BLOCKMAP_UNIT) - (y0 / BLOCKMAP_UNIT));

	y_sign = (y1 >= y0) ? 1 : -1;

	// handle the simple cases: same column or same row

	blocknum = (y0 / BLOCKMAP_UNIT) * bmap_width + (x0 / BLOCKMAP_UNIT);

	if (y_dist == 0)
	{
		for (i=0; i <= x_dist; i++, blocknum++)
			BlockAdd(blocknum, line_num);

		return;
	}

	if (x_dist == 0)
	{
		for (i=0; i <= y_dist; i++, blocknum += y_sign * bmap_width)
			BlockAdd(blocknum, line_num);

		return;
	}

	// -AJA- 2000/12/09: rewrote the general case

	SYS_ASSERT(x1 > x0);

	slope = (float)(y1 - y0) / (float)(x1 - x0);

	// handle each column of blocks in turn
	for (i=0; i <= x_dist; i++)
	{
		// compute intersection of column with line
		int sx = (i==0)      ? x0 : (128 * (x0 / 128 + i));
		int ex = (i==x_dist) ? x1 : (128 * (x0 / 128 + i) + 127);

		int sy = y0 + (int)(slope * (sx - x0));
		int ey = y0 + (int)(slope * (ex - x0));

		SYS_ASSERT(sx <= ex);

		y_dist = ABS((ey / 128) - (sy / 128));

		for (j=0; j <= y_dist; j++)
		{
			blocknum = (sy / 128 + j * y_sign) * bmap_width + (sx / 128);

			BlockAdd(blocknum, line_num);
		}
	}
}


void P_GenerateBlockMap(int min_x, int min_y, int max_x, int max_y)
{
	int i;
	int bnum, btotal;
	unsigned short *b_pos;

	bmap_orgx = min_x - 8;
	bmap_orgy = min_y - 8;
	bmap_width  = BLOCKMAP_GET_X(max_x) + 1;
	bmap_height = BLOCKMAP_GET_Y(max_y) + 1;

	btotal = bmap_width * bmap_height;

	L_WriteDebug("GenerateBlockmap: MAP (%d,%d) -> (%d,%d)\n",
		min_x, min_y, max_x, max_y);
	L_WriteDebug("GenerateBlockmap: BLOCKS %d x %d  TOTAL %d\n",
		bmap_width, bmap_height, btotal);

	// setup blk_cur_lines array.  Initially all pointers are NULL, when
	// any lines get added then the dynamic array is created.

	blk_cur_lines = Z_New(unsigned short *, btotal);

	Z_Clear(blk_cur_lines, unsigned short *, btotal);

	// initial # of line values ("0, -1" for each block)
	blk_total_lines = 2 * btotal;

	// process each linedef of the map
	for (i=0; i < numlines; i++)
		BlockAddLine(i);

	L_WriteDebug("GenerateBlockmap: TOTAL DATA=%d\n", blk_total_lines);

	// convert dynamic arrays to single array and free memory

	bmap_pointers = new unsigned short * [btotal];
	bmap_lines    = new unsigned short   [blk_total_lines];

	b_pos = bmap_lines;

	for (bnum=0; bnum < btotal; bnum++)
	{
		SYS_ASSERT(b_pos - bmap_lines < blk_total_lines);

		bmap_pointers[bnum] = b_pos;

		*b_pos++ = 0;

		if (blk_cur_lines[bnum])
		{
			// transfer the line values
			for (i=blk_cur_lines[bnum][BCUR_SIZE] - 1; i >= 0; i--)
				*b_pos++ = blk_cur_lines[bnum][BCUR_FIRST + i];

			Z_Free(blk_cur_lines[bnum]);
		}

		*b_pos++ = BMAP_END;
	}

	if (b_pos - bmap_lines != blk_total_lines)
		I_Error("GenerateBlockMap: miscounted !\n");

	Z_Free(blk_cur_lines);
	blk_cur_lines = NULL;
}


//--------------------------------------------------------------------------
//
// SUBSECTOR BLOCKMAP
//

typedef std::list<subsector_t *> subsec_set_t;

//## class subsec_set_c
//## {
//## public:
//## 	std::list<subsector_t *> subs;
//## 
//## public:
//## 	 subsec_set_c() : subs() { }
//## 	~subsec_set_c() { } 
//## };

static subsec_set_t **subsec_map;
static int subsec_map_total;

static void SubsecAdd(subsector_t *sub)
{
	int lx = BLOCKMAP_GET_X(sub->bbox[BOXLEFT]);
	int ly = BLOCKMAP_GET_Y(sub->bbox[BOXBOTTOM]);
	int hx = BLOCKMAP_GET_X(sub->bbox[BOXRIGHT]);
	int hy = BLOCKMAP_GET_Y(sub->bbox[BOXTOP]);

	SYS_ASSERT(0 <= lx && lx <= hx && hx < bmap_width);
	SYS_ASSERT(0 <= ly && ly <= hy && hy < bmap_height);
	
	for (int by = ly; by <= hy; by++)
	for (int bx = lx; bx <= hx; bx++)
	{
		int bnum = by * bmap_width + bx;	

		if (! subsec_map[bnum])
			subsec_map[bnum] = new subsec_set_t;

		subsec_map[bnum]->push_back(sub);
		subsec_map_total++;
	}
}

static int SubsecFillHoles(void)
{
	int holes  = 0;
	int filled = 0;

	for (int y = 0; y < bmap_height; y++)
	for (int x = 0; x < bmap_width;  x++)
	{
		int bnum = y * bmap_width + x;

		if (subsec_map[bnum])
			continue;

		holes++;

		for (int side = 0; side < 4; side++)
		{
			int nx = x + ((side==0) ? 1 : (side==2) ? -1 : 0);
			int ny = y + ((side==1) ? 1 : (side==3) ? -1 : 0);

			if (0 <= nx && nx < bmap_width  &&
			    0 <= ny && ny < bmap_height &&
				subsec_map[ny * bmap_width + nx])
			{
				subsec_set_t *other = subsec_map[ny * bmap_width + nx];
				SYS_ASSERT(other->size() > 0);
				
				// we only need a single subsector for our hole
				if (other->size() == 1)
					subsec_map[bnum] = other;
				else
				{
					subsec_map[bnum] = new subsec_set_t;
					subsec_map[bnum]->push_back(other->front());
				}
				
				filled++;
				break;
			}
		}
	}

	I_Debugf("SubsecFillHoles: %d holes, %d filled\n", holes, filled);

	return holes;
}

void P_GenerateSubsecMap(void)
{
	subsec_map = new subsec_set_t * [bmap_width * bmap_height];
	subsec_map_total = 0;

	Z_Clear(subsec_map, subsec_set_t *, bmap_width * bmap_height);

	for (int i = 0; i < numsubsectors; i++)
		SubsecAdd(&subsectors[i]);

	if (subsec_map_total < 1)
		I_Error("No subsectors ????\n");

	while (SubsecFillHoles() > 0)
	{
		/* repeat until all holes filled */
	}
}

bool R_SubsectorContainsPoint(subsector_t *sub, float x, float y)
{
	for (seg_t *seg = sub->segs; seg; seg = seg->sub_next)
	{
		divline_t div;

		div.x  = seg->v1->x;
		div.y  = seg->v1->y;
		div.dx = seg->v2->x - div.x;
		div.dy = seg->v2->y - div.y;

		if (P_PointOnDivlineSide(x, y, &div) == 1)
			return false;
	}

	return true;
}

subsector_t *R_PointInSubsector(float x, float y)
{
	int bx = BLOCKMAP_GET_X(x);
	int by = BLOCKMAP_GET_Y(y);

	if (bx < 0) bx = 0;
	if (by < 0) by = 0;

	if (bx >= bmap_width)  bx = bmap_width-1;
	if (by >= bmap_height) by = bmap_height-1;

	subsec_set_t *list = subsec_map[by * bmap_width + bx];
	SYS_ASSERT(list);
	SYS_ASSERT(list->size() > 0);

	// Start search at second entry.  If search fails then
	// we assume the point is in the very first entry.
	//
	// The reason for this logic is to handle points which
	// lie in void space or outside of the map.  The old BSP
	// method would handle such points gracefully, since the
	// traversal always lead to a valid subsector. 

	subsec_set_t::iterator LI = list->begin();

	for (LI++; LI != list->end(); LI++)
	{
		subsector_t *sub = *LI;
		SYS_ASSERT(sub);

		if (R_SubsectorContainsPoint(sub, x, y))
			return sub;
	}

	return list->front();
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
