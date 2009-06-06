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


#define DIST_EPSILON  0.2f


static int max_segs;
static int max_subsecs;
static int max_glvert;

static void Poly_Setup(void)
{
	numsegs = 0;
	numsubsectors = 0;
	num_gl_vertexes = 0;

	max_segs    = numsides * 4;
	max_subsecs = numlines * 2;
	max_glvert  = numsides * 3;

	segs = new seg_t[numsegs];
	subsectors = new subsector_t[numsubsectors];
	gl_vertexes = new vertex_t[num_gl_vertexes];

	Z_Clear(segs, seg_t, numsegs);
	Z_Clear(subsectors, subsector_t, numsubsectors);
	Z_Clear(gl_vertexes, vertex_t, num_gl_vertexes);
}

// Nasty allocation shit....  Fixme

static seg_t *NewSeg(void)
{
	if (numsegs >= max_segs)
		I_Error("R_PolygonizeMap: ran out of segs !\n");

	seg_t *seg = segs + numsegs;

	memset(seg, 0, sizeof(seg_t));

	numsegs++;

	return seg;
}

static subsector_t *NewSubsector(void)
{
	if (numsubsectors >= max_segs)
		I_Error("WF_BuildBSP: ran out of subsectors !\n");

	subsector_t *sub = subsectors + numsubsectors;

	memset(sub, 0, sizeof(subsector_t));

	numsubsectors++;

	return sub;
}

static vertex_t *NewVertex(void)
{
	if (num_gl_vertexes >= max_glvert)
		I_Error("R_PolygonizeMap: ran out of vertices !\n");

	vertex_t *vert = gl_vertexes + num_gl_vertexes;

	memset(vert, 0, sizeof(vertex_t));

	num_gl_vertexes++;

	return vert;
}


static subsector_t *CreateSubsector(sector_t *sec)
{
	subsector_t *sub = NewSubsector();

	sub->sector = sec;

	// bbox is computed at the very end
	
	return sub;
}

static void AddSeg(subsector_t *sub, seg_t *seg)
{
	// add seg to subsector
	seg->sub_next = sub->segs;
	sub->segs = seg;
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

	AddSeg(sub, g);

	return g;
}


static inline void ComputeIntersection(seg_t *cur, divline_t& party,
  float perp_c, float perp_d, float *x, float *y)
{
	// horizontal partition against vertical seg
	if (party.dy == 0 && cur->v1->x == cur->v2->x)
	{
		*x = cur->v1->x;
		*y = party.y;
		return;
	}

	// vertical partition against horizontal seg
	if (party.dx == 0 && cur->v1->y == cur->v2->y)
	{
		*x = party.x;
		*y = cur->v1->y;
		return;
	}

	// 0 = start, 1 = end
	double ds = perp_c / (perp_c - perp_d);

	if (cur->pdx == 0)
		*x = cur->psx;
	else
		*x = cur->psx + (cur->pdx * ds);

	if (cur->pdy == 0)
		*y = cur->psy;
	else
		*y = cur->psy + (cur->pdy * ds);
}

//
// SplitSeg
//
// -AJA- Splits the given seg at the point (x,y).  The new seg is
//       returned.  The old seg is shortened (the original start
//       vertex is unchanged), whereas the new seg becomes the cut-off
//       tail (keeping the original end vertex).
//
//       If the seg has a partner, than that partner is also split.
//       NOTE WELL: the new piece of the partner seg is inserted into
//       the same list as the partner seg (and after it) -- thus ALL
//       segs (except the one we are currently splitting) must exist
//       on a singly-linked list somewhere. 
//
static seg_t *SplitSeg(seg_t *old_seg, float x, float y)
{
# if DEBUG_SPLIT
	if (old_seg->linedef)
		PrintDebug("Splitting Linedef %d (%p) at (%1.1f,%1.1f)\n",
				old_seg->linedef->index, old_seg, x, y);
	else
		PrintDebug("Splitting Miniseg %p at (%1.1f,%1.1f)\n", old_seg, x, y);
# endif

	seg_t *new_seg = NewSeg();

	vertex_t *new_vert = NewVertex(x, y);

	// copy seg info
	new_seg[0] = old_seg[0];
	new_seg->next = NULL;

	old_seg->v2 = new_vert;
	new_seg->v1 = new_vert;

	old_seg->length = R_PointToDist(old_seg->v1->x, old_seg->v1->y, x, y);
	new_seg->length = R_PointToDist(new_seg->v2->x, old_seg->v2->y, x, y);

# if DEBUG_SPLIT
	PrintDebug("Splitting Vertex is %04X at (%1.1f,%1.1f)\n",
			new_vert->index, new_vert->x, new_vert->y);
# endif

	// handle partners

	if (old_seg->partner)
	{
#   if DEBUG_SPLIT
		PrintDebug("Splitting Partner %p\n", old_seg->partner);
#   endif

		new_seg->partner = NewSeg();

		// copy seg info
		new_seg->partner[0] = old_seg->partner[0];

		// IMPORTANT: keep partner relationship valid
		new_seg->partner->partner = new_seg;

		old_seg->partner->v1 = new_vert;
		new_seg->partner->v2 = new_vert;

		old_seg->partner->length = old_seg->length;
		new_seg->partner->length = new_seg->length;

		// link it into list
		old_seg->partner->next = new_seg->partner;
	}

	return new_seg;
}


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


static inline bool SameDir(divline_t& party, seg_t *seg)
{
	float sdx = seg->v2->x - seg->v1->x;
	float sdy = seg->v2->y - seg->v1->y;

	return (sdx * party.dx + sdy * party.dy) >= 0;
}

static inline float PerpDist(divline_t& party, float x, float y)
{
	x -= party.x;
	y -= party.y;

	float len = sqrt(party.dx * party.dx + party.dy * party.dy);

	return (x * party.dy - y * party.dx) / len;
}

static inline float AlongDist(divline_t& party, float x, float y)
{
	x -= party.x;
	y -= party.y;

	float len = sqrt(party.dx * party.dx + party.dy * party.dy);

	return (x * party.dx + y * party.dy) / len;
}


static void AddIntersection(intersection_t ** cut_list,
    vertex_t *vert, seg_t *part, boolean_g self_ref)
{
  intersection_t *cut;
  intersection_t *after;

  /* check if vertex already present */
  for (cut=(*cut_list); cut; cut=cut->next)
  {
    if (vert == cut->vertex)
      return;
  }

  /* create new intersection */
  cut = NewIntersection();

  cut->vertex = vert;
  cut->along_dist = UtilParallelDist(part, vert->x, vert->y);
  cut->self_ref = self_ref;
 
  cut->before = VertexCheckOpen(vert, -part->pdx, -part->pdy);
  cut->after  = VertexCheckOpen(vert,  part->pdx,  part->pdy);
 
  /* enqueue the new intersection into the list */

  for (after=(*cut_list); after && after->next; after=after->next)
  { }

  while (after && cut->along_dist < after->along_dist) 
    after = after->prev;
  
  /* link it in */
  cut->next = after ? after->next : (*cut_list);
  cut->prev = after;

  if (after)
  {
    if (after->next)
      after->next->prev = cut;
    
    after->next = cut;
  }
  else
  {
    if (*cut_list)
      (*cut_list)->prev = cut;
    
    (*cut_list) = cut;
  }
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

static void DivideOneSeg(seg_t *cur, divline_t& party,
	subsector_t *left, subsector_t *right,
    intersection_t ** cut_list)
{
	seg_t *new_seg;

	/* get state of lines' relation to each other */
	float a = PerpDist(party, cur->v1->x, cur->v1->y);
	float b = PerpDist(party, cur->v2->x, cur->v2->y);

	int a_side = (a < -DIST_EPSILON) ? -1 : (a > DIST_EPSILON) ? +1 : 0;
	int b_side = (b < -DIST_EPSILON) ? -1 : (b > DIST_EPSILON) ? +1 : 0;

	/* check for being on the same line */
	if (a_side == 0 && b_side == 0)
	{
		// this seg runs along the same line as the partition.  check
		// whether it goes in the same direction or the opposite.

		if (SameDir(party, cur))
		{
			AddSeg(right, cur);
		}
		else
		{
			AddSeg(left, cur);
		}

		// don't need to add any intersections, since there should
		// be another seg that comes away from the line.

		return;
	}

	/* check for right side */
	if (a_side >= 0 && b_side >= 0)
	{
		if (a_side == 0)
			AddIntersection(cut_list, party, cur->v1, -1);
		else if (b_side == 0)
			AddIntersection(cut_list, party, cur->v2, +1);

		AddSeg(right, cur);
		return;
	}

	/* check for left side */
	if (a_side <= 0 && b_side <= 0)
	{
		if (a_side == 0)
			AddIntersection(cut_list, party, cur->v1, +1);
		else if (b_side == 0)
			AddIntersection(cut_list, party, cur->v2, -1);

		AddSeg(left, cur);
		return;
	}

	// when we reach here, we have a and b non-zero and opposite sign,
	// hence this seg will be split by the partition line.

	SYS_ASSERT(a_side == -b_side);

	float x, y;

	ComputeIntersection(cur, party, a, b, &x, &y);

	new_seg = SplitSeg(cur, x, y);

	AddIntersection(cut_list, party, new_seg->v1, a_side);

	if (a_side < 0)
	{
		AddSeg(left,  cur);
		AddSeg(right, new_seg);
	}
	else
	{
		AddSeg(right, cur);
		AddSeg(left,  new_seg);
	}
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

	// link new subsector into the Sector
	left->sec_next = sub->sector->subsectors;
	sub->sector->subsectors = left;

	seg_t *seg_list = sub->segs;
	sub->segs = NULL;

	intersection_t *cut_list;

	while (seg_list)
	{
		seg_t *cur = seg_list;
		seg_list = seg_list->next;

		DivideOneSeg(cur, party, left, sub, cut_list);
	}

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
