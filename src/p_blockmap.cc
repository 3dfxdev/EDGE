//----------------------------------------------------------------------------
//  EDGE Blockmap utility functions
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

#include "i_defs.h"

#include <float.h>

#include <vector>
#include <algorithm>

#include "dm_data.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "m_bbox.h"
#include "p_local.h"
#include "p_spec.h"
#include "r_state.h"
#include "z_zone.h"

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
int bmapwidth;
int bmapheight;  // size in mapblocks

unsigned short *  bmap_lines = NULL; 
unsigned short ** bmap_pointers = NULL;

// offsets in blockmap are from here
int *blockmaplump = NULL;

// origin of block map
float bmaporgx;
float bmaporgy;

// for thing chains
mobj_t **blocklinks = NULL;

// for dynamic lights
mobj_t **blocklights = NULL;


void P_CreateThingBlockMap(void)
{
	blocklinks  = new mobj_t* [bmapwidth * bmapheight];

	Z_Clear(blocklinks,  mobj_t*, bmapwidth * bmapheight);

	blocklights = new mobj_t* [bmapwidth * bmapheight];

	Z_Clear(blocklights, mobj_t*, bmapwidth * bmapheight);
}

void P_DestroyBlockMap(void)
{
	delete[] bmap_lines;    bmap_lines  = NULL;
	delete[] bmap_pointers; bmap_pointers = NULL;

	delete[] blocklinks;    blocklinks  = NULL;
	delete[] blocklights;   blocklights = NULL;
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

static inline touch_node_t *TouchNodeAlloc(void)
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

static inline void TouchNodeFree(touch_node_t *tn)
{
#ifdef DEVELOPERS
	touchstat_free++;
#endif

	// PREV field is ignored in quick-alloc list
	tn->mo_next = free_touch_nodes;
	free_touch_nodes = tn;
}

static inline void TouchNodeLinkIntoSector(touch_node_t *tn, sector_t *sec)
{
	tn->sec = sec;

	tn->sec_next = sec->touch_things;
	tn->sec_prev = NULL;

	if (tn->sec_next)
		tn->sec_next->sec_prev = tn;

	sec->touch_things = tn;
}

static inline void TouchNodeLinkIntoThing(touch_node_t *tn, mobj_t *mo)
{
	tn->mo = mo;

	tn->mo_next = mo->touch_sectors;
	tn->mo_prev = NULL;

	if (tn->mo_next)
		tn->mo_next->mo_prev = tn;

	mo->touch_sectors = tn;
}

static inline void TouchNodeUnlinkFromSector(touch_node_t *tn)
{
	if (tn->sec_next)
		tn->sec_next->sec_prev = tn->sec_prev;

	if (tn->sec_prev)
		tn->sec_prev->sec_next = tn->sec_next;
	else
		tn->sec->touch_things = tn->sec_next;
}

static inline void TouchNodeUnlinkFromThing(touch_node_t *tn)
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
	mobj_t *thing;

	float bbox[4];
}
setposbsp_t;

//
// SetPositionBSP
//
static void SetPositionBSP(setposbsp_t *info, int nodenum)
{
	touch_node_t *tn;
	sector_t *sec;
	subsector_t *sub;
	seg_t *seg;

	while (! (nodenum & NF_V5_SUBSECTOR))
	{
		node_t *nd = nodes + nodenum;

		int side = P_BoxOnDivLineSide(info->bbox, &nd->div);

		// if box touches partition line, we must traverse both sides
		if (side == -1)
		{
			SetPositionBSP(info, nd->children[0]);
			side = 1;
		}

		SYS_ASSERT(side == 0 || side == 1);

		nodenum = nd->children[side];
	}

	// reached a leaf of the BSP.  Need to check BBOX against all
	// linedef segs.  This is because we can get false positives, since
	// we don't actually split the thing's BBOX when it intersects with
	// a partition line.

	sub = subsectors + (nodenum & ~NF_V5_SUBSECTOR);

	for (seg=sub->segs; seg; seg=seg->sub_next)
	{
		divline_t div;

		if (seg->miniseg)
			continue;

		div.x = seg->v1->x;
		div.y = seg->v1->y;
		div.dx = seg->v2->x - div.x;
		div.dy = seg->v2->y - div.y;

		if (P_BoxOnDivLineSide(info->bbox, &div) == 1)
			return;
	}

	// Perform linkage...

	sec = sub->sector;

#ifdef DEVELOPERS
	touchstat_miss++;
#endif

	for (tn = info->thing->touch_sectors; tn; tn = tn->mo_next)
	{
		if (! tn->mo)
		{
			// found unused touch node.  We reuse it.
			tn->mo = info->thing;

			if (tn->sec != sec)
			{
				TouchNodeUnlinkFromSector(tn);
				TouchNodeLinkIntoSector(tn, sec);
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

		SYS_ASSERT(tn->mo == info->thing);

		// sector already present ?
		if (tn->sec == sec)
			return;
	}

	// need to allocate a new touch node
	tn = TouchNodeAlloc();

	TouchNodeLinkIntoThing(tn, info->thing);
	TouchNodeLinkIntoSector(tn, sec);
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
void P_UnsetThingPosition(mobj_t * thing)
{
	int blockx;
	int blocky;
	int bnum;

	touch_node_t *tn;

	// unlink from subsector
	if (!(thing->flags & MF_NOSECTOR))
	{
		// (inert things don't need to be in subsector list)

		if (thing->snext)
		{
#ifdef DEVELOPERS
			if (thing->snext->sprev != thing)
				I_Error("INTERNAL ERROR: Bad subsector NEXT link in thing.\n");
#endif
			thing->snext->sprev = thing->sprev;
		}

		if (thing->sprev)
		{
#ifdef DEVELOPERS
			if (thing->sprev->snext != thing)
				I_Error("INTERNAL ERROR: Bad subsector PREV link in thing.\n");
#endif
			thing->sprev->snext = thing->snext;
		}
		else
		{
#ifdef DEVELOPERS
			if (thing->subsector->thinglist != thing)
				I_Error("INTERNAL ERROR: Bad subsector link (HEAD) in thing, possibly double P_UnsetThingPosition call.\n");
#endif
			thing->subsector->thinglist = thing->snext;
		}

		thing->snext = NULL;
		thing->sprev = NULL;
	}

	// unlink from touching list.
	// NOTE: lazy unlinking -- see notes in r_defs.h
	//
	for (tn = thing->touch_sectors; tn; tn = tn->mo_next)
	{
		tn->mo = NULL;
	}

	// unlink from blockmap
	if (!(thing->flags & MF_NOBLOCKMAP))
	{
		// inert things don't need to be in blockmap
		if (thing->bnext)
		{
#ifdef DEVELOPERS
			if (thing->bnext->bprev != thing)
				I_Error("INTERNAL ERROR: Bad block NEXT link in thing.\n");
#endif
			thing->bnext->bprev = thing->bprev;
		}

		if (thing->bprev)
		{
#ifdef DEVELOPERS
			if (thing->bprev->bnext != thing)
				I_Error("INTERNAL ERROR: Bad block PREV link in thing.\n");
#endif
			thing->bprev->bnext = thing->bnext;
		}
		else
		{
			blockx = BLOCKMAP_GET_X(thing->x);
			blocky = BLOCKMAP_GET_Y(thing->y);

			if (blockx >= 0 && blockx < bmapwidth &&
				blocky >= 0 && blocky < bmapheight)
			{
				bnum = blocky * bmapwidth + blockx;
#ifdef DEVELOPERS
				if (blocklinks[bnum] != thing)
					I_Error("INTERNAL ERROR: Bad block link (HEAD) in thing.\n");
#endif
				blocklinks[bnum] = thing->bnext;
			}
		}

		thing->bprev = NULL;
		thing->bnext = NULL;
	}

	// unlink from dynamic light blockmap
	if (thing->info && (thing->info->dlight[0].type != DLITE_None))
	{
		if (thing->dlnext)
		{
			SYS_ASSERT(thing->dlnext->dlprev == thing);
			thing->dlnext->dlprev = thing->dlprev;
		}

		if (thing->dlprev)
		{
			SYS_ASSERT(thing->dlprev->dlnext == thing);
			thing->dlprev->dlnext = thing->dlnext;
		}
		else
		{
			blockx = BLOCKMAP_GET_X(thing->x);
			blocky = BLOCKMAP_GET_Y(thing->y);

			if (blockx >= 0 && blockx < bmapwidth &&
				blocky >= 0 && blocky < bmapheight)
			{
				bnum = blocky * bmapwidth + blockx;

				SYS_ASSERT(blocklights[bnum] == thing);
				blocklights[bnum] = thing->dlnext;
			}
		}

		thing->dlprev = thing->dlnext = NULL;
	}
}

//
// P_UnsetThingFinally
// 
// Call when the thing is about to be removed for good.
// 
void P_UnsetThingFinally(mobj_t * thing)
{
	touch_node_t *tn;

	P_UnsetThingPosition(thing);

	// clear out touch nodes

	while (thing->touch_sectors)
	{
		tn = thing->touch_sectors;
		thing->touch_sectors = tn->mo_next;

		TouchNodeUnlinkFromSector(tn);
		TouchNodeFree(tn);
	}
}

//
// P_SetThingPosition
//
// Links a thing into both a block and a subsector
// based on it's x y.
//
void P_SetThingPosition(mobj_t * thing)
{
	subsector_t *ss;
	int blockx;
	int blocky;
	int bnum;

	setposbsp_t pos;
	touch_node_t *tn;

	// -ES- 1999/12/04 The position must be unset before it's set again.
#ifdef DEVELOPERS
	if (thing->snext || thing->sprev || thing->bnext || thing->bprev)
		I_Error("INTERNAL ERROR: Double P_SetThingPosition call.");

	SYS_ASSERT(! (thing->dlnext || thing->dlprev));
#endif  // DEVELOPERS

	// link into subsector
	ss = R_PointInSubsector(thing->x, thing->y);
	thing->subsector = ss;

	// determine properties
	thing->props = R_PointGetProps(ss, thing->z + thing->height/2);

	if (! (thing->flags & MF_NOSECTOR))
	{
		thing->snext = ss->thinglist;
		thing->sprev = NULL;

		if (ss->thinglist)
			ss->thinglist->sprev = thing;

		ss->thinglist = thing;
	}

	// link into touching list

#ifdef DEVELOPERS
	touchstat_moves++;
#endif

	pos.thing = thing;
	pos.bbox[BOXLEFT]   = thing->x - thing->radius;
	pos.bbox[BOXRIGHT]  = thing->x + thing->radius;
	pos.bbox[BOXBOTTOM] = thing->y - thing->radius;
	pos.bbox[BOXTOP]    = thing->y + thing->radius;

	SetPositionBSP(&pos, root_node);

	// handle any left-over unused touch nodes

	for (tn = thing->touch_sectors; tn && tn->mo; tn = tn->mo_next)
	{ /* nothing here */ }

	if (tn)
	{
		if (tn->mo_prev)
			tn->mo_prev->mo_next = NULL;
		else
			thing->touch_sectors = NULL;

		while (tn)
		{
			touch_node_t *cur = tn;
			tn = tn->mo_next;

			SYS_ASSERT(! cur->mo);

			TouchNodeUnlinkFromSector(cur);
			TouchNodeFree(cur);
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
	if (!(thing->flags & MF_NOBLOCKMAP))
	{
		blockx = BLOCKMAP_GET_X(thing->x);
		blocky = BLOCKMAP_GET_Y(thing->y);

		if (blockx >= 0 && blockx < bmapwidth &&
			blocky >= 0 && blocky < bmapheight)
		{
			bnum = blocky * bmapwidth + blockx;

			thing->bprev = NULL;
			thing->bnext = blocklinks[bnum];

			if (blocklinks[bnum])
				(blocklinks[bnum])->bprev = thing;

			blocklinks[bnum] = thing;
		}
		else
		{
			// thing is off the map
			thing->bnext = thing->bprev = NULL;
		}
	}

	// link into dynamic light blockmap
	if (thing->info && (thing->info->dlight[0].type != DLITE_None))
	{
		blockx = BLOCKMAP_GET_X(thing->x);
		blocky = BLOCKMAP_GET_Y(thing->y);

		if (blockx >= 0 && blockx < bmapwidth &&
			blocky >= 0 && blocky < bmapheight)
		{
			bnum = blocky * bmapwidth + blockx;

			thing->dlprev = NULL;
			thing->dlnext = blocklights[bnum];

			if (blocklights[bnum])
				(blocklights[bnum])->dlprev = thing;

			blocklights[bnum] = thing;
		}
		else
		{
			// thing is off the map
			thing->dlnext = thing->dlprev = NULL;
		}
	}
}

//
// P_ChangeThingPosition
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

//
// P_FreeSectorTouchNodes
// 
void P_FreeSectorTouchNodes(sector_t *sec)
{
	touch_node_t *tn;

	for (tn = sec->touch_things; tn; tn = tn->sec_next)
		TouchNodeFree(tn);
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
// 23-6-98 KM Changed to reflect blockmap is now int* not short*
// -AJA- 2000/07/31: line data changed back to shorts.
//
bool P_BlockLinesIterator (int x, int y, bool(*func) (line_t *))
{
	unsigned short *list;
	line_t *ld;

	if (x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight)
		return true;

	list = bmap_pointers[y * bmapwidth + x];

	for (; *list != BMAP_END; list++)
	{
		ld = &lines[*list];

		// has line already been checked ?
		if (ld->validcount == validcount)
			continue;

		ld->validcount = validcount;

		if (!func(ld))
			return false;
	}

	// everything was checked
	return true;
}


bool P_BlockThingsIterator(int x, int y, bool(*func) (mobj_t *))
{
	mobj_t *mobj;

	if (x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight)
		return true;

	for (mobj = blocklinks[y * bmapwidth + x]; mobj; mobj = mobj->bnext)
	{
		if (!func(mobj))
			return false;
	}

	return true;
}

bool P_RadiusThingsIterator(float x, float y, float r, bool(*func) (mobj_t *))
{
	int bx, by;
	int xl, xh, yl, yh;

	// The bounding box is extended by MAXRADIUS
	// because mobj_ts are grouped into mapblocks
	// based on their origin point, and can overlap
	// into adjacent blocks by up to MAXRADIUS units.
	//
	// -AJA- 2006/11/15: restored this (was broken for a long time!).
	r += MAXRADIUS;

	xl = BLOCKMAP_GET_X(x - r);
	xh = BLOCKMAP_GET_X(x + r);
	yl = BLOCKMAP_GET_Y(y - r);
	yh = BLOCKMAP_GET_Y(y + r);

	for (by = yl; by <= yh; by++)
	for (bx = xl; bx <= xh; bx++)
	{
		if (! P_BlockThingsIterator(bx, by, func))
			return false;
	}

	return true;
}


//--------------------------------------------------------------------------
//
// INTERCEPT ROUTINES
//

static std::vector<intercept_t> intercepts;

divline_t trace;
bool earlyout;
int ptflags;


float P_InterceptVector(divline_t * v2, divline_t * v1)
{
	// Returns the fractional intercept point along the first divline.
	// This is only called by the addthings and addlines traversers.
	//
	float frac;
	float num;
	float den;
	float v1x;
	float v1y;
	float v1dx;
	float v1dy;
	float v2x;
	float v2y;
	float v2dx;
	float v2dy;

	v1x = v1->x;
	v1y = v1->y;
	v1dx = v1->dx;
	v1dy = v1->dy;
	v2x = v2->x;
	v2y = v2->y;
	v2dx = v2->dx;
	v2dy = v2->dy;

	den = v1dy * v2dx - v1dx * v2dy;

	if (den == 0)
		return 0;  // parallel

	num = (v1x - v2x) * v1dy + (v2y - v1y) * v1dx;
	frac = num / den;

	return frac;
}


static bool PIT_AddLineIntercepts(line_t * ld)
{
	// Looks for lines in the given block
	// that intercept the given trace
	// to add to the intercepts list.
	//
	// A line is crossed if its endpoints
	// are on opposite sides of the trace.
	// Returns true if earlyout and a solid line hit.

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

	if (s1 == s2)
		return true;  // line isn't crossed

	// hit the line

	frac = P_InterceptVector(&trace, &div);

	// out of range?
	if (frac < 0 || frac > 1)
		return true;

	// try to early out the check
	if (earlyout && frac < 1.0f && !ld->backsector)
		return false;  // stop checking

	// Intercept is a simple struct that can be memcpy()'d: Load
	// up a structure and get into the array
	intercept_t in;

	in.frac  = frac;
	in.thing = NULL;
	in.line  = ld;

	intercepts.push_back(in);
	return true;  // continue

}


static bool PIT_AddThingIntercepts(mobj_t * thing)
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
		return true;

	div.x = x1;
	div.y = y1;
	div.dx = x2 - x1;
	div.dy = y2 - y1;

	frac = P_InterceptVector(&trace, &div);

	// out of range?
	if (frac < 0 || frac > 1)
		return true;

	// Intercept is a simple struct that can be memcpy()'d: Load
	// up a structure and get into the array
	intercept_t in;
	
	in.frac  = frac;
	in.thing = thing;
	in.line  = NULL;

	intercepts.push_back(in);
	return true;
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
bool P_PathTraverse(float x1, float y1, float x2, float y2, 
						 int flags, traverser_t trav)
{
	earlyout = (flags & PT_EARLYOUT)?true:false;

	validcount++;

	intercepts.clear();

	// don't side exactly on a line
	if (fmod(x1 - bmaporgx, BLOCKMAP_UNIT) == 0)
		x1 += 0.1f;

	if (fmod(y1 - bmaporgy, BLOCKMAP_UNIT) == 0)
		y1 += 0.1f;

	trace.x = x1;
	trace.y = y1;
	trace.dx = x2 - x1;
	trace.dy = y2 - y1;

	x1 -= bmaporgx;
	y1 -= bmaporgy;
	x2 -= bmaporgx;
	y2 -= bmaporgy;

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
		if (flags & PT_ADDLINES)
		{
			if (!P_BlockLinesIterator(bx, by, PIT_AddLineIntercepts))
				return false;
		}

		if (flags & PT_ADDTHINGS)
		{
			if (!P_BlockThingsIterator(bx, by, PIT_AddThingIntercepts))
				return false;
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
		if (! trav(& *I))
		{
			// don't bother going farther
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
	SYS_ASSERT(bnum < (bmapwidth * bmapheight));

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

	x0 = (int)(ld->v1->x - bmaporgx);
	y0 = (int)(ld->v1->y - bmaporgy);
	x1 = (int)(ld->v2->x - bmaporgx);
	y1 = (int)(ld->v2->y - bmaporgy);

	// swap endpoints if horizontally backward
	if (x1 < x0)
	{
		int temp;

		temp = x0; x0 = x1; x1 = temp;
		temp = y0; y0 = y1; y1 = temp;
	}

	SYS_ASSERT(0 <= x0 && (x0 / BLOCKMAP_UNIT) < bmapwidth);
	SYS_ASSERT(0 <= y0 && (y0 / BLOCKMAP_UNIT) < bmapheight);
	SYS_ASSERT(0 <= x1 && (x1 / BLOCKMAP_UNIT) < bmapwidth);
	SYS_ASSERT(0 <= y1 && (y1 / BLOCKMAP_UNIT) < bmapheight);

	// check if this line spans multiple blocks.

	x_dist = ABS((x1 / BLOCKMAP_UNIT) - (x0 / BLOCKMAP_UNIT));
	y_dist = ABS((y1 / BLOCKMAP_UNIT) - (y0 / BLOCKMAP_UNIT));

	y_sign = (y1 >= y0) ? 1 : -1;

	// handle the simple cases: same column or same row

	blocknum = (y0 / BLOCKMAP_UNIT) * bmapwidth + (x0 / BLOCKMAP_UNIT);

	if (y_dist == 0)
	{
		for (i=0; i <= x_dist; i++, blocknum++)
			BlockAdd(blocknum, line_num);

		return;
	}

	if (x_dist == 0)
	{
		for (i=0; i <= y_dist; i++, blocknum += y_sign * bmapwidth)
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
			blocknum = (sy / 128 + j * y_sign) * bmapwidth + (sx / 128);

			BlockAdd(blocknum, line_num);
		}
	}
}


void P_GenerateBlockMap(int min_x, int min_y, int max_x, int max_y)
{
	int i;
	int bnum, btotal;
	unsigned short *b_pos;

	bmaporgx = min_x - 8;
	bmaporgy = min_y - 8;
	bmapwidth  = BLOCKMAP_GET_X(max_x) + 1;
	bmapheight = BLOCKMAP_GET_Y(max_y) + 1;

	btotal = bmapwidth * bmapheight;

	L_WriteDebug("GenerateBlockmap: MAP (%d,%d) -> (%d,%d)\n",
		min_x, min_y, max_x, max_y);
	L_WriteDebug("GenerateBlockmap: BLOCKS %d x %d  TOTAL %d\n",
		bmapwidth, bmapheight, btotal);

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
