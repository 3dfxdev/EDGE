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


static void DetermineMiddle(subsector_t *sub, float *x, float *y)
{
	int total=0;

	*x = 0;
	*y = 0;

	for (seg_t *cur = sub->segs; cur; cur = cur->sub_next)
	{
		*x += cur->v1->x + cur->v2->x;
		*y += cur->v1->y + cur->v2->y;

		total += 2;
	}

	*x /= total;
	*y /= total;
}


static void ClockwiseOrder(subsector_t *sub)
{
	float mid_x, mid_y;

	DetermineMiddle(sub, &mid_x, &mid_y);

	seg_t *cur;
	seg_t ** array;
	seg_t *seg_buffer[32];

	int i;
	int total = 0;

	int first = 0;
	int score = -1;

#if DEBUG_SUBSEC
	PrintDebug("Subsec: Clockwising %d\n", sub->index);
#endif

	// count segs and create an array to manipulate them
	for (cur=sub->seg_list; cur; cur=cur->next)
		total++;

	// use local array if small enough
	if (total <= 32)
		array = seg_buffer;
	else
		array = UtilCalloc(total * sizeof(seg_t *));

	for (cur=sub->seg_list, i=0; cur; cur=cur->next, i++)
		array[i] = cur;

	if (i != total)
		InternalError("ClockwiseOrder miscounted.");

	// sort segs by angle (from the middle point to the start vertex).
	// The desired order (clockwise) means descending angles.

	i = 0;

	while (i+1 < total)
	{
		seg_t *A = array[i];
		seg_t *B = array[i+1];

		angle_t angle1, angle2;

		angle1 = UtilComputeAngle(A->start->x - sub->mid_x, A->start->y - sub->mid_y);
		angle2 = UtilComputeAngle(B->start->x - sub->mid_x, B->start->y - sub->mid_y);

		if (angle1 < angle2)
		{
			// swap 'em
			array[i] = B;
			array[i+1] = A;

			// bubble down
			if (i > 0)
				i--;
		}
		else
		{
			// bubble up
			i++;
		}
	}

	// transfer sorted array back into sub
	sub->seg_list = NULL;

	for (i=total-1; i >= 0; i--)
	{
		int j = (i + first) % total;

		array[j]->next = sub->seg_list;
		sub->seg_list  = array[j];
	}

	if (total > 32)
		UtilFree(array);

#if DEBUG_SORTER
	PrintDebug("Sorted SEGS around (%1.1f,%1.1f)\n", sub->mid_x, sub->mid_y);

	for (cur=sub->seg_list; cur; cur=cur->next)
	{
		angle_g angle = UtilComputeAngle(cur->start->x - sub->mid_x,
				cur->start->y - sub->mid_y);

		PrintDebug("  Seg %p: Angle %1.6f  (%1.1f,%1.1f) -> (%1.1f,%1.1f)\n",
				cur, angle, cur->start->x, cur->start->y, cur->end->x, cur->end->y);
	}
#endif
}


static bool ChoosePartition(subsector_t *sub, divline_t *div)
{
	int total = 0;
	for (seg_t *Z = sub->segs; Z; Z=Z->sub_next)
		total++;

	if (total <= 3)
		return false;

	// FIXME: if total > ### median shit

	return false;
}


static void TrySplitSubsector(subsector_t *sub)
{
	divline_t party;

	if (! ChoosePartition(sub, &party))
	{
		sub->convex = true;
		return;
	}

	subsector_t *left = CreateSubsector(sub->sector);

	left->sec_next = sub->sector->subsectors;
	sub->sector->subsectors = left;

	seg_t *seg_list = sub->segs;
	sub->segs = NULL;

	intersection_t *cut_list;

	SeparateSegs(sub, seg_list, &party, left, sub, &cut_list);

	AddMinisegs( cut_list ) ;
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
