//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering : Polygonizer
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2009  Andrew Apted
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

#include "i_defs.h"

#include "r_defs.h"
#include "r_state.h"
#include "p_local.h"


static int max_segs;
static int max_subsecs;

static void Poly_Setup(void)
{
	max_segs    = numsides * 4;
	max_subsecs = numlines * 2;
}

// Nasty allocation shit....  Fixme

static inline seg_t *NewSeg(void)
{
	if (numsegs >= max_segs)
		I_Error("R_PolygonizeMap: too many segs !\n");

	seg_t *seg = segs + numsegs;

	memset(seg, 0, sizeof(seg_t));

	numsegs++;

	return seg;
}

static inline subsector_t *NewSubsector(void)
{
	if (numsubsectors >= max_segs)
		I_Error("WF_BuildBSP: too many subsectors !\n");

	subsector_t *sub = subsectors + numsubsectors;

	memset(sub, 0, sizeof(subsector_t));

	numsubsectors++;

	return sub;
}


static subsector_t *CreateSubsector(sector_t *sec)
{
	subsector_t *sub = NewSubsector();

	sub->sector = sec;

	// bbox is computed at the very end
	
	return sub;
}

static seg_t * CreateOneSeg(line_t *ld, sector_t *sec, int side,
                            vertex_t *v1, vertex_t *v2)
{
	seg_t *g = NewSeg();

	g->v1 = v1;
	g->v2 = v2;
	g->angle = R_PointToAngle(v1->x, v1->y, v2->x, v2->y);
	g->length = R_PointToDist(v1->x, v1->y, v2->x, v2->y);

	// Note: these fields are setup by GroupLines:
	//          front_sub,
	//          back_sub
	//          backsector

	g->miniseg = false;

	g->offset  = 0;
	g->sidedef = ld->side[side];
	g->linedef = ld;
	g->side    = side;

	g->frontsector = sec;

	if (! sec->subsectors)
		sec->subsectors = CreateSubsector(sec);

	subsector_t *sub = sec->subsectors;

	// add seg to subsector
	g->sub_next = sub->segs;
	sub->segs = g;

	return g;
}

// FIXME: SplitSeg


static void Poly_CreateSegs(void)
{
	for (int i=0; i < numlines; i++)
	{
		line_t *ld = &lines[i];

		// ignore zero-length lines		
		if (ld->length < 0.1)
			continue;

		// TODO: handle overlapping lines

		// ignore self-referencing lines
		if (ld->frontsector == ld->backsector)
			continue;

		seg_t *right = CreateOneSeg(ld, ld->frontsector, 0, ld->v1, ld->v2);
		seg_t *left  = NULL;

		if (ld->backsector)
		{
			left = CreateOneSeg(ld, ld->backsector, 1, ld->v2, ld->v1 );

			right->partner = left;
			left ->partner = right;
		}
	}
}


static seg_t * ChoosePartition(subsector_t *sub)
{
	return NULL;
}


static void TrySplitSubsector(subsector_t *sub)
{
	seg_t * partition = ChoosePartition(sub);

	if (! partition)
	{
		sub->convex = true;
		return;
	}

	// blah...
}


void Poly_SplitSector(sector_t *sec)
{
	// skip sectors which reference no linedefs
	if (! sec->subsectors)
		return;

	for (;;)
	{
		int changes = 0;

		for (subsector_t *sub = sec->subsectors; sub; sub = sub->sec_next)
		{
			if (! sub->convex)
			{
				TrySplitSubsector(sub);
				changes++;
			}
		}

		if (changes == 0)
			return;
	}
}


void R_PolygonizeMap(void)
{
	Poly_Setup();

	Poly_CreateSegs();

	for (int i = 0; i < numsectors; i++)
		Poly_SplitSector(&sectors[i]);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
