//----------------------------------------------------------------------------
//  EDGE <-> Wolf3D (BSP Build)
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
// -AJA- 2000/07/29: wrote this file.
//
// TODO HERE:
//   + improve the wlf_seg_matrix stuff
//   + optimise
//

#include "i_defs.h"
#include "wf_local.h"

#include "dm_state.h"
#include "e_player.h"
#include "e_search.h"
#include "m_bbox.h"
#include "p_local.h"
#include "r_defs.h"



extern int max_segs;

extern short *wlf_seg_matrix;

#define MP1_INDEX(x,y)  ((y) * (WLFMAP_H + 1) + (x))


//
// NewSeg
//
// Ugh, this ain't very pretty, but it works
//
static inline seg_t *NewSeg(void)
{
	if (numsegs >= max_segs)
		I_Error("WF_BuildBSP: too many segs !\n");

  seg_t *seg = segs + numsegs;

	memset(seg, 0, sizeof(seg_t));

	numsegs++;

	return seg;
}

//
// NewSubsector
//
// NOTE WELL: don't keep any pointers to subsectors, always use []
// indexing since the array can move.
//
static inline int NewSubsector(void)
{
	if (numsubsectors >= max_segs)
		I_Error("WF_BuildBSP: too many subsectors !\n");

  subsector_t *sub = subsectors + numsubsectors;

	memset(sub, 0, sizeof(subsector_t));

	numsubsectors++;

	return numsubsectors-1;
}

//
// NewNode
//
// NOTE WELL: don't keep any pointers to the nodes, always use []
// indexing since the array can move.
//
static inline int NewNode(void)
{
	if (numnodes >= max_segs)
		I_Error("WF_BuildBSP: too many nodes !\n");

  node_t *nd = nodes + numnodes;

	memset(nd, 0, sizeof(node_t));

	numnodes++;

	return numnodes-1;
}

//
// CreateInitialSeg
//
static seg_t *CreateInitialSeg(line_t *line, vertex_t *start,
		vertex_t *end, side_t *side)
{
	seg_t *result = NewSeg();

	result->v1 = start;
	result->v2 = end;

	result->angle = R_PointToAngle(start->x, start->y, end->x, end->y);
	result->length = R_PointToDist(start->x, start->y, end->x, end->y);
	result->offset = 0.0;

	result->partner = NULL;
	result->sub_next = NULL;
	result->miniseg = false;

	result->sidedef = side;
	result->linedef = line;
	result->frontsector = side->sector;

	// determine front_sub, back_sub and backsector later
	result->front_sub = result->back_sub = NULL;
	result->backsector = NULL;

	return result;
}

//
// CreateSegs
//
// Creates one or two segs for each linedef (depending on whether it
// is one-sided or two-sided), and link them into one big list.
// Returns the list.
//
static seg_t *CreateSegs(void)
{
	int i;

	seg_t *left;
	seg_t *right;

	for (i=0; i < numlines; i++)
	{
		line_t *ld = &lines[i];

		right = CreateInitialSeg(ld, ld->v1, ld->v2, ld->side[0]);

		if (ld->side[1])
		{
			left = CreateInitialSeg(ld, ld->v2, ld->v1, ld->side[1]);

			// setup partner info (important !)
			left->partner = right;
			right->partner = left;
		}
	}

	// link all segs into a list
	for (i=0; i < numsegs - 1; i++)
		segs[i].sub_next = &segs[i+1];

	return &segs[0];
}

//
// FindLimits
//
static void FindLimits(float *bbox, seg_t *seglist)
{
	M_ClearBox(bbox);

	for (; seglist; seglist = seglist->sub_next)
	{
		M_AddToBox(bbox, seglist->v1->x, seglist->v1->y);
		M_AddToBox(bbox, seglist->v2->x, seglist->v2->y);
	}
}


//----------------------------------------------------------------------------

//
// SegOnRight
//
// Determine which side of a partition line the given seg is.  Returns
// true if on the right, false if on the left.  NOTE: we assume that a
// seg is never longer than one Wolf tile (64 units).
//
static bool SegOnRight(seg_t *seg, seg_t *part)
{
	bool neg;

	if (part->v1->y == part->v2->y)
	{
		// horizontal partition
		SYS_ASSERT(part->v1->x != part->v2->x);

		neg = (part->v2->x < part->v1->x) ? true : false;

		if (MIN(seg->v1->y, seg->v2->y) < part->v1->y)
			return true ^ neg;

		if (MAX(seg->v1->y, seg->v2->y) > part->v1->y)
			return false ^ neg;

		// seg must lie along partition line
		return (seg->v2->x > seg->v1->x) ^ neg;
	}
	else
	{
		// vertical partition
		SYS_ASSERT(part->v1->x == part->v2->x);
		SYS_ASSERT(part->v1->y != part->v2->y);

		neg = (part->v2->y < part->v1->y) ? true : false;

		if (MAX(seg->v1->x, seg->v2->x) > part->v1->x)
			return true ^ neg;

		if (MIN(seg->v1->x, seg->v2->x) < part->v1->x)
			return false ^ neg;

		// seg must lie along partition line
		return (seg->v2->y > seg->v1->y) ^ neg;
	}
}

//
// CoversOneTile
//
// 
static bool CoversOneTile(seg_t *seglist)
{
	float bbox[4];

	FindLimits(bbox, seglist);

	if (bbox[BOXRIGHT] - bbox[BOXLEFT] > 64*(1))
		return false;

	if (bbox[BOXTOP] - bbox[BOXBOTTOM] > 64*(1))
		return false;

	return true;
}

//
// EvalPartition
//
// Evaluate the partition seg and determine the cost.  Returns -1 if
// the seg should be skipped altogether.  We ignore minisegs here
// since we _need_ a real seg on each side.
//
static int EvalPartition(seg_t *seglist, seg_t *part)
{
	int on_left  = 0;
	int on_right = 0;

	for (; seglist; seglist = seglist->sub_next)
	{
		if (seglist->miniseg)
			continue;

		if (SegOnRight(seglist, part))
			on_right++;
		else
			on_left++;
	}

	if (on_left == 0 || on_right == 0)
		return -1;

	return ABS(on_left - on_right);
}

//
// FindPartition
//
// Find the best partition seg to use, or NULL if the seg list is
// already convex.
//
static seg_t *FindPartition(seg_t *seglist)
{
	bool single_tile = CoversOneTile(seglist);

	seg_t *part;
	seg_t *best = NULL;

	int cost;
	int best_cost = INT_MAX;

	for (part=seglist; part; part=part->sub_next)
	{
		// ignore minisegs
		if (part->miniseg)
			continue;

		// doorsegs are the only segs that ever lie on non-tile boundary
		// coordinates.  Therefore, only consider them for partition lines
		// when we have reached a single tile.  The linedef code ensures
		// that we don't need to split anything here.

		if (! single_tile &&
				((fabs(fmod(part->v1->x, 64*(1))) > 0.001 &&
				  fabs(fmod(part->v2->x, 64*(1))) > 0.001)  ||
				 (fabs(fmod(part->v1->y, 64*(1))) > 0.001 &&
				  fabs(fmod(part->v2->y, 64*(1))) > 0.001)))
		{
			continue;
		}

		// optimisation: if this seg is on the same line as the next seg,
		// then we can skip one of them.

		if (part->sub_next && !part->sub_next->miniseg &&
				((part->v1->y == part->v2->y &&
				  part->sub_next->v1->y == part->sub_next->v2->y &&
				  part->v1->y == part->sub_next->v1->y)  ||
				 (part->v1->x == part->v2->x &&
				  part->sub_next->v1->x == part->sub_next->v2->x &&
				  part->v1->x == part->sub_next->v1->x)))
		{
			continue;
		}

		cost = EvalPartition(seglist, part);

		// seg unsuitable or too costly ?
		if (cost < 0 || cost >= best_cost)
			continue;

		// we have found a better one
		best = part;
		best_cost = cost;
	}

	return best;
}

//
// CreateOneMiniseg
//
static void CreateOneMiniseg(seg_t ** seglist, int vx1, int vy1,
		int vx2, int vy2)
{
	seg_t *A = NewSeg();
	seg_t *B = NewSeg();

	A->v1 = WF_GetVertex(vx1, vy1);
	A->v2 = WF_GetVertex(vx2, vy2);

	B->v1 = A->v2;
	B->v2 = A->v1;

	A->angle = R_PointToAngle(A->v1->x, A->v1->y, A->v2->x, A->v2->y);
	A->length = R_PointToDist(A->v1->x, A->v1->y, A->v2->x, A->v2->y);

	B->angle  = A->angle + ANG180;
	B->length = A->length;

	A->partner = B;
	B->partner = A;

	A->miniseg = B->miniseg = true;

	// determine front_sub & back_sub later
	A->front_sub = A->back_sub = NULL;
	B->front_sub = B->back_sub = NULL;

	// link them in
	B->sub_next = (*seglist);
	A->sub_next = B;
	(*seglist)  = A;
}

//
// CreateMinisegs
//
static void CreateMinisegs(seg_t ** seglist, seg_t *part, float *bbox)
{
	int x, y;

	if (part->v1->y == part->v2->y)
	{
		// horizontal partition
		y = (int) (part->v1->y / 64*(1));
		SYS_ASSERT(0 <= y && y <= WLFMAP_H);

		for (x=0; x < WLFMAP_W; x++)
		{
			// don't include segs outside of our bbox
			if (64*(x) < bbox[BOXLEFT] || 64*(x) >= bbox[BOXRIGHT])
				continue;

			if (wlf_seg_matrix[MP1_INDEX(x,y) * 2 + 0] < 0)
			{
				// either void space or pre-existing seg
				continue;
			}

			CreateOneMiniseg(seglist, x, y, x+1, y);
		}
	}
	else
	{
		// vertical partition
		SYS_ASSERT(part->v1->x == part->v2->x);

		x = (int) (part->v1->x / 64*(1));
		SYS_ASSERT(0 <= x && x <= WLFMAP_W);

		for (y=0; y < WLFMAP_H; y++)
		{
			if (64*(y) < bbox[BOXBOTTOM] || 64*(y) >= bbox[BOXTOP])
				continue;

			if (wlf_seg_matrix[MP1_INDEX(x,y) * 2 + 1] < 0)
				continue;

			CreateOneMiniseg(seglist, x, y+1, x, y);
		}
	}
}

//
// DivideSegs
//
// Divide the seg list into left and right halves.
//
static void DivideSegs(seg_t *seglist, seg_t *part,
		seg_t ** left, seg_t ** right, float *bbox)
{
	seg_t *next;

	(*left)  = NULL;
	(*right) = NULL;

	CreateMinisegs(&seglist, part, bbox);

	for (; seglist; seglist = next)
	{
		next = seglist->sub_next;

		if (SegOnRight(seglist, part))
		{
			// move to right side
			seglist->sub_next = (*right);
			(*right) = seglist;
		}
		else
		{
			// move to left side
			seglist->sub_next = (*left);
			(*left) = seglist;
		}
	}
}


//----------------------------------------------------------------------------

//
// SortClockwise
//
static seg_t *SortClockwise(seg_t *seglist)
{
	int i;
	int total;
	float mid_x, mid_y;

	seg_t *cur;
	seg_t ** array;

	// compute middle point
	mid_x = mid_y = 0;

	for (cur=seglist, total=0; cur; cur=cur->sub_next, total++)
	{
		// only use start vertex (we assume closedness)
		mid_x += cur->v1->x;
		mid_y += cur->v1->y;
	}

	SYS_ASSERT(total > 0);

	mid_x /= (float)total;
	mid_y /= (float)total;

	// create array to sort on
	array = new seg_t* [total];

	for (i=0, cur=seglist; i < total; i++, cur=cur->sub_next)
		array[i] = cur;

#define GET_ANG(seg)  \
	R_PointToAngle(mid_x, mid_y, (seg)->v1->x, (seg)->v1->y)
#define CMP(a,b)  (GET_ANG(a) > GET_ANG(b))

	QSORT(seg_t *, array, total, CUTOFF);

#undef CMP
#undef GET_ANG

	// setup links
	for (i=0; i < total; i++)
		array[i]->sub_next = (i+1 < total) ? array[i+1] : NULL;

	cur = array[0];

  delete[] array;

	return cur;
}

//
// BuildSubsector
//
// Builds a single subsector from a list of segs, known to be convex.
// Returns the subsector number, or'd with the NF_SUBSECTOR flag.
//
static int BuildSubsector(seg_t *seglist)
{
	SYS_ASSERT(seglist);

	int sub = NewSubsector();
	seg_t *cur;

	// determine sector
	for (cur=seglist; cur && cur->miniseg; cur=cur->sub_next)
	{ /* nothing here */ }

	// subsectors *must* contain at least one non-miniseg
	SYS_ASSERT(cur);

	subsectors[sub].sector = cur->sidedef->sector;

	// sort seglist into clockwise order
	subsectors[sub].segs = SortClockwise(seglist);

  subsectors[sub].bbox = new float[4];

  FindLimits(subsectors[sub].bbox, seglist);

	return sub | NF_V5_SUBSECTOR;
}

//
// BuildNodes
//
// Recursively build nodes, by looking for a seg that can act as a
// partition line.  Returns the child number, which is either a node
// or a subsector depending on the NF_SUBSECTOR flag.
//
static int BuildNodes(seg_t *seglist, float *bbox)
{
	seg_t *part;
	seg_t *left, *right;
	int nd;

	SYS_ASSERT(seglist);

	// find partition seg.  None indicates subsector is convex.
	part = FindPartition(seglist);

	if (! part)
		return BuildSubsector(seglist);

	// divide the segs into left and right halves, taking care to add
	// minisegs where needed.

	DivideSegs(seglist, part, &left, &right, bbox);

	SYS_ASSERT(left);
	SYS_ASSERT(right);

	// create a new node

	nd = NewNode();

	nodes[nd].div.x  = part->v1->x;
	nodes[nd].div.y  = part->v1->y;
	nodes[nd].div.dx = part->v2->x - part->v1->x;
	nodes[nd].div.dy = part->v2->y - part->v1->y;

	nodes[nd].div_len = R_PointToDist(0, 0, nodes[nd].div.dx, nodes[nd].div.dy);

	FindLimits(nodes[nd].bbox[0], right);
	FindLimits(nodes[nd].bbox[1], left);

	// recursively divide left & right halves

	nodes[nd].children[0] = BuildNodes(right, nodes[nd].bbox[0]);
	nodes[nd].children[1] = BuildNodes(left,  nodes[nd].bbox[1]);

	return nd;
}

//
// FixupSubsectors
//
static void FixupSubsectors(void)
{
	int i;
	seg_t *cur;

	for (i=0; i < numsubsectors; i++)
	{
		subsector_t *sub = subsectors + i;

		SYS_ASSERT(sub->sector);

		// link subsector into sector
		sub->sec_next = sub->sector->subsectors;
		sub->sector->subsectors = sub;

		// fix up the segs
		for (cur=sub->segs; cur; cur=cur->sub_next)
			cur->front_sub = sub;
	}

	// fix up the BACKSUB of the segs.  We cannot do this in the above
	// loop, as we require _all_ the front pointers to be valid.

	for (i=0; i < numsegs; i++)
	{
		cur = segs + i;

		if (! cur->partner)
			continue;

		cur->back_sub = cur->partner->front_sub;
		cur->partner->back_sub = cur->front_sub;
	}
}

//
// WF_BuildBSP
//
// Use the current vertices, lines and sectors to build the BSP segs,
// subsectors and nodes.  Due to the simplicity of the Wolf world,
// this should be pretty straightforward (for example, seg-splits
// never occur).
//
void WF_BuildBSP(void)
{
	seg_t *seglist;
	float bbox[4];

	// firstly, create normal segs from each linedef
	seglist = CreateSegs();

	SYS_ASSERT(numsegs > 0);

	// now build the root node

	bbox[BOXLEFT]   = 64*(0);
	bbox[BOXBOTTOM] = 64*(0);
	bbox[BOXRIGHT]  = 64*(WLFMAP_W);
	bbox[BOXTOP]    = 64*(WLFMAP_H);

	root_node = BuildNodes(seglist, bbox);

	I_Printf("WOLF: Seg count: %d\n", numsegs);
	I_Printf("WOLF: Subsector count: %d\n", numsubsectors);
	I_Printf("WOLF: Node count: %d\n", numnodes);
	I_Printf("WOLF: Root Node: %d\n", root_node);

	// fix up various things
	FixupSubsectors();
}

