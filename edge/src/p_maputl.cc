//----------------------------------------------------------------------------
//  EDGE Movement, Collision & Blockmap utility functions
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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
// DESCRIPTION:
//   Movement/collision utility functions,
//   as used by function in p_map.c. 
//   BLOCKMAP Iterator functions,
//   and some PIT_* functions to use for iteration.
//   Gap/extrafloor utility functions.
//   Touch Node code.
//
// TODO HERE:
//   + make gap routines I_Error if overflow limit.
//

#include "i_defs.h"

#include "m_bbox.h"
#include "dm_defs.h"
#include "p_local.h"
#include "p_spec.h"

#include "z_zone.h"

// State.
#include "r_state.h"

#include <float.h>
#include <math.h>
#include <string.h>

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

//
// P_ApproxDistance
//
// Gives an estimation of distance (not exact)
//
float P_ApproxDistance(float dx, float dy)
{
	dx = fabs(dx);
	dy = fabs(dy);

	return (dy > dx) ? dy + dx/2 : dx + dy/2;
}

float P_ApproxDistance(float dx, float dy, float dz)
{
	dx = fabs(dx);
	dy = fabs(dy);
	dz = fabs(dz);

	float dxy = (dy > dx) ? dy + dx/2 : dx + dy/2;

	return (dz > dxy) ? dz + dxy/2 : dxy + dz/2;
}

//
// P_ApproxSlope
//
// Gives an estimation of slope (not exact)
//
// -AJA- 1999/09/11: written.
//
float P_ApproxSlope(float dx, float dy, float dz)
{
	float dist = P_ApproxDistance(dx, dy);

	// kludge to prevent overflow or division by zero.
	if (dist < 1.0f / 32.0f)
		dist = 1.0f / 32.0f;

	return dz / dist;
}

//
// P_PointOnDivlineSide
//
// Tests which side of the line the given point lies on.
// Returns 0 (front/right) or 1 (back/left).  If the point lies
// directly on the line, result is undefined (either 0 or 1).
//
int P_PointOnDivlineSide(float x, float y, divline_t *div)
{
	float dx, dy;
	float left, right;

	if (div->dx == 0)
		return ((x <= div->x) ^ (div->dy > 0)) ? 0 : 1;

	if (div->dy == 0)
		return ((y <= div->y) ^ (div->dx < 0)) ? 0 : 1;

	dx = x - div->x;
	dy = y - div->y;

	// try to quickly decide by looking at sign bits
	if ((div->dy < 0) ^ (div->dx < 0) ^ (dx < 0) ^ (dy < 0))
	{
		// left is negative
		if ((div->dy < 0) ^ (dx < 0))
			return 1;

		return 0;
	}

	left  = dx * div->dy;
	right = dy * div->dx;

	return (right < left) ? 0 : 1;
}

//
// P_PointOnDivlineThick
//
// Tests which side of the line the given point is on.   The thickness
// parameter determines when the point is considered "on" the line.
// Returns 0 (front/right), 1 (back/left), or 2 (on).
//
int P_PointOnDivlineThick(float x, float y, divline_t *div,
						  float div_len, float thickness)
{
	float dx, dy;
	float left, right;

	if (div->dx == 0)
	{
		if (fabs(x - div->x) <= thickness)
			return 2;

		return ((x < div->x) ^ (div->dy > 0)) ? 0 : 1;
	}

	if (div->dy == 0)
	{
		if (fabs(y - div->y) <= thickness)
			return 2;

		return ((y < div->y) ^ (div->dx < 0)) ? 0 : 1;
	}

	dx = x - div->x;
	dy = y - div->y;

	// need divline's length here to compute proper distances
	left  = (dx * div->dy) / div_len;
	right = (dy * div->dx) / div_len;

	if (fabs(left - right) < thickness)
		return 2;

	return (right < left) ? 0 : 1;
}

//
// P_BoxOnLineSide
//
// Considers the line to be infinite
// Returns side 0 or 1, -1 if box crosses the line.
//
int P_BoxOnLineSide(const float * tmbox, line_t * ld)
{
	int p1 = 0;
	int p2 = 0;

	divline_t div;

	div.x = ld->v1->x;
	div.y = ld->v1->y;
	div.dx = ld->dx;
	div.dy = ld->dy;

	switch (ld->slopetype)
	{
	case ST_HORIZONTAL:
		p1 = tmbox[BOXTOP] > ld->v1->y;
		p2 = tmbox[BOXBOTTOM] > ld->v1->y;
		if (ld->dx < 0)
		{
			p1 ^= 1;
			p2 ^= 1;
		}
		break;

	case ST_VERTICAL:
		p1 = tmbox[BOXRIGHT] < ld->v1->x;
		p2 = tmbox[BOXLEFT] < ld->v1->x;
		if (ld->dy < 0)
		{
			p1 ^= 1;
			p2 ^= 1;
		}
		break;

	case ST_POSITIVE:
		p1 = P_PointOnDivlineSide(tmbox[BOXLEFT], tmbox[BOXTOP], &div);
		p2 = P_PointOnDivlineSide(tmbox[BOXRIGHT], tmbox[BOXBOTTOM], &div);
		break;

	case ST_NEGATIVE:
		p1 = P_PointOnDivlineSide(tmbox[BOXRIGHT], tmbox[BOXTOP], &div);
		p2 = P_PointOnDivlineSide(tmbox[BOXLEFT], tmbox[BOXBOTTOM], &div);
		break;
	}

	if (p1 == p2)
		return p1;

	return -1;
}

//
// P_BoxOnDivLineSide
//
// Considers the line to be infinite
// Returns side 0 or 1, -1 if box crosses the line.
//
int P_BoxOnDivLineSide(const float * tmbox, divline_t *div)
{
	int p1 = 0;
	int p2 = 0;

	if (div->dy == 0)
	{
		p1 = tmbox[BOXTOP] > div->y;
		p2 = tmbox[BOXBOTTOM] > div->y;

		if (div->dx < 0)
		{
			p1 ^= 1;
			p2 ^= 1;
		}
	}
	else if (div->dx == 0)
	{
		p1 = tmbox[BOXRIGHT] < div->x;
		p2 = tmbox[BOXLEFT] < div->x;

		if (div->dy < 0)
		{
			p1 ^= 1;
			p2 ^= 1;
		}
	}
	else if (div->dy / div->dx > 0)  // optimise ?
	{
		p1 = P_PointOnDivlineSide(tmbox[BOXLEFT], tmbox[BOXTOP], div);
		p2 = P_PointOnDivlineSide(tmbox[BOXRIGHT], tmbox[BOXBOTTOM], div);
	}
	else
	{
		p1 = P_PointOnDivlineSide(tmbox[BOXRIGHT], tmbox[BOXTOP], div);
		p2 = P_PointOnDivlineSide(tmbox[BOXLEFT], tmbox[BOXBOTTOM], div);
	}

	if (p1 == p2)
		return p1;

	return -1;
}

//
// P_InterceptVector
//
// Returns the fractional intercept point along the first divline.
// This is only called by the addthings and addlines traversers.
//
float P_InterceptVector(divline_t * v2, divline_t * v1)
{
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

//
//  GAP UTILITY FUNCTIONS
//

static int GAP_RemoveSolid(vgap_t * dest, int d_num, float z1, float z2)
{
	int d;
	int new_num = 0;
	vgap_t new_gaps[100];

#ifdef DEVELOPERS
	if (z1 > z2)
		I_Error("RemoveSolid: z1 > z2");
#endif

	for (d = 0; d < d_num; d++)
	{
		if (dest[d].c <= dest[d].f)
			continue;  // ignore empty gaps.

		if (z1 <= dest[d].f && z2 >= dest[d].c)
			continue;  // completely blocks it.

		if (z1 >= dest[d].c || z2 <= dest[d].f)
		{
			// no intersection.

			new_gaps[new_num].f = dest[d].f;
			new_gaps[new_num].c = dest[d].c;
			new_num++;
			continue;
		}

		// partial intersections.

		if (z1 > dest[d].f)
		{
			new_gaps[new_num].f = dest[d].f;
			new_gaps[new_num].c = z1;
			new_num++;
		}

		if (z2 < dest[d].c)
		{
			new_gaps[new_num].f = z2;
			new_gaps[new_num].c = dest[d].c;
			new_num++;
		}
	}

	Z_MoveData(dest, new_gaps, vgap_t, new_num);

	return new_num;
}

static int GAP_Construct(vgap_t * gaps, sector_t * sec, mobj_t * thing)
{
	extrafloor_t * ef;

	int num = 1;

	// early out for FUBAR sectors
	if (sec->f_h >= sec->c_h)
		return 0;

	gaps[0].f = sec->f_h;
	gaps[0].c = sec->c_h;

	for (ef = sec->bottom_ef; ef; ef = ef->higher)
	{
		num = GAP_RemoveSolid(gaps, num, ef->bottom_h, ef->top_h);
	}

	// -- handle WATER WALKERS --

	if (!thing || !(thing->extendedflags & EF_WATERWALKER))
		return num;

	for (ef = sec->bottom_liq; ef; ef = ef->higher)
	{
		if (ef->ef_info && (ef->ef_info->type & EXFL_Water))
		{
			num = GAP_RemoveSolid(gaps, num, ef->bottom_h, ef->top_h);
		}
	}

	return num;
}

static int GAP_SightConstruct(vgap_t * gaps, sector_t * sec)
{
	extrafloor_t * ef;

	int num = 1;

	// early out for closed or FUBAR sectors
	if (sec->c_h <= sec->f_h)
		return 0;

	gaps[0].f = sec->f_h;
	gaps[0].c = sec->c_h;

	for (ef = sec->bottom_ef; ef; ef = ef->higher)
	{
		if (!ef->ef_info || !(ef->ef_info->type & EXFL_SeeThrough))
			num = GAP_RemoveSolid(gaps, num, ef->bottom_h, ef->top_h);
	}

	for (ef = sec->bottom_liq; ef; ef = ef->higher)
	{
		if (!ef->ef_info || !(ef->ef_info->type & EXFL_SeeThrough))
			num = GAP_RemoveSolid(gaps, num, ef->bottom_h, ef->top_h);
	}

	return num;
}

static int GAP_Restrict(vgap_t * dest, int d_num, vgap_t * src, int s_num)
{
	int d, s;
	float f, c;

	int new_num = 0;
	vgap_t new_gaps[32];

	for (s = 0; s < s_num; s++)
	{
		// ignore empty gaps.
		if (src[s].c <= src[s].f)
			continue;

		for (d = 0; d < d_num; d++)
		{
			// ignore empty gaps.
			if (dest[d].c <= dest[d].f)
				continue;

			f = MAX(src[s].f, dest[d].f);
			c = MIN(src[s].c, dest[d].c);

			if (f < c)
			{
				new_gaps[new_num].c = c;
				new_gaps[new_num].f = f;
				new_num++;
			}
		}
	}

	Z_MoveData(dest, new_gaps, vgap_t, new_num);

	return new_num;
}

#if 0  // DEBUG ONLY
static void GAP_Dump(vgap_t *gaps, int num)
{
	int j;

	for (j=0; j < num; j++)
		L_WriteDebug("  GAP %d/%d  %1.1f .. %1.1f\n", j+1, num,
		gaps[j].f, gaps[j].c);
}
#endif

//
// P_FindThingGap
//
// Find the best gap that the thing could fit in, given a certain Z
// position (z1 is foot, z2 is head).  Assuming at least two gaps exist,
// the best gap is chosen as follows:
//
// 1. if the thing fits in one of the gaps without moving vertically,
//    then choose that gap.
//
// 2. if there is only *one* gap which the thing could fit in, then
//    choose that gap.
//
// 3. if there is multiple gaps which the thing could fit in, choose
//    the gap whose floor is closest to the thing's current Z.
//
// 4. if there is no gaps which the thing could fit in, do the same.
//
// Returns the gap number, or -1 if there are no gaps at all.
//
int P_FindThingGap(vgap_t * gaps, int gap_num, float z1, float z2)
{
	int i;
	float dist;

	int fit_num = 0;
	int fit_last = -1;

	int fit_closest = -1;
	float fit_mindist = FLT_MAX;

	int nofit_closest = -1;
	float nofit_mindist = FLT_MAX;

	// check for trivial gaps...

	if (gap_num == 0)
	{
		return -1;
	}
	else if (gap_num == 1)
	{
		return 0;
	}

	// There are 2 or more gaps.  Now it gets interesting :-)

	for (i = 0; i < gap_num; i++)
	{
		if (z1 >= gaps[i].f && z2 <= gaps[i].c)
		{ // [1]
			return i;
		}

		dist = ABS(z1 - gaps[i].f);

		if (z2 - z1 <= gaps[i].c - gaps[i].f)
		{ // [2]
			fit_num++;

			fit_last = i;
			if (dist < fit_mindist)
			{ // [3]
				fit_mindist = dist;
				fit_closest = i;
			}
		}
		else
		{
			if (dist < nofit_mindist)
			{ // [4]
				nofit_mindist = dist;
				nofit_closest = i;
			}
		}
	}

	if (fit_num == 1)
		return fit_last;
	else if (fit_num > 1)
		return fit_closest;
	else
		return nofit_closest;
}

//
// P_ComputeThingGap
//
// Determine the initial floorz and ceilingz that a thing placed at a
// particular Z would have.  Returns the nominal Z height.  Some special
// values of Z are recognised: ONFLOORZ & ONCEILINGZ.
//
float P_ComputeThingGap(mobj_t * thing, sector_t * sec, float z,
						float * f, float * c)
{
	vgap_t temp_gaps[100];
	int temp_num;

	temp_num = GAP_Construct(temp_gaps, sec, thing);

	if (z == ONFLOORZ)
		z = sec->f_h;

	if (z == ONCEILINGZ)
		z = sec->c_h - thing->height;

	temp_num = P_FindThingGap(temp_gaps, temp_num, z, z + thing->height);

	if (temp_num < 0)
	{
		// thing is stuck in a closed door.
		*f = *c = sec->f_h;
		return *f;
	}

	*f = temp_gaps[temp_num].f;
	*c = temp_gaps[temp_num].c;

	return z;
}

//
// P_ComputeGaps
//
// Determine the gaps between the front & back sectors of the line,
// taking into account any extra floors.
//
// -AJA- 1999/07/19: This replaces P_LineOpening.
//
void P_ComputeGaps(line_t * ld)
{
	sector_t *front = ld->frontsector;
	sector_t *back  = ld->backsector;

	int temp_num;
	vgap_t temp_gaps[100];

	ld->blocked = true;
	ld->gap_num = 0;

	if (!front || !back)
	{
		// single sided line
		return;
	}

	// NOTE: this check is rather lax.  It mirrors the check in original
	// Doom r_bsp.c, in order for transparent doors to work properly.
	// In particular, the blocked flag can be clear even when one of the
	// sectors is closed (has ceiling <= floor).

	if (back->c_h <= front->f_h || front->c_h <= back->f_h)
	{
		// closed door.
		return;
	}

	// FIXME: strictly speaking this is not correct, as the front or
	// back sector may be filled up with thick opaque extrafloors.
	//
	ld->blocked = false;

	// handle horizontal sliders
	if (ld->special && ld->special->s.type != SLIDE_None)
	{
		slider_move_t *smov = ld->slider_move;

		if (! smov)
			return;

		// these semantics copied from XDoom
		if (smov->direction > 0 && smov->opening < smov->target * 0.5f)
			return;

		if (smov->direction < 0 && smov->opening < smov->target * 0.75f)
			return;
	}

	// handle normal gaps ("movement" gaps)

	ld->gap_num = GAP_Construct(ld->gaps, front, NULL);
	temp_num = GAP_Construct(temp_gaps, back, NULL);

	ld->gap_num = GAP_Restrict(ld->gaps, ld->gap_num, temp_gaps, temp_num);
}

//
// P_DumpExtraFloors
//
#ifdef DEVELOPERS
void P_DumpExtraFloors(const sector_t *sec)
{
	const extrafloor_t *ef;

	L_WriteDebug("EXTRAFLOORS IN Sector %d  (%d used, %d max)\n",
		sec - sectors, sec->exfloor_used, sec->exfloor_max);

	L_WriteDebug("  Basic height: %1.1f .. %1.1f\n", sec->f_h, sec->c_h);

	for (ef = sec->bottom_ef; ef; ef = ef->higher)
	{
		L_WriteDebug("  Solid %s: %1.1f .. %1.1f\n",
			(ef->ef_info->type & EXFL_Thick) ? "Thick" : "Thin",
			ef->bottom_h, ef->top_h);
	}

	for (ef = sec->bottom_liq; ef; ef = ef->higher)
	{
		L_WriteDebug("  Liquid %s: %1.1f .. %1.1f\n",
			(ef->ef_info->type & EXFL_Thick) ? "Thick" : "Thin",
			ef->bottom_h, ef->top_h);
	}
}
#endif

//
// P_ExtraFloorFits
//
// Check if a solid extrafloor fits.
//
exfloor_fit_e P_ExtraFloorFits(sector_t *sec, float z1, float z2)
{
	extrafloor_t *ef;

	if (z2 > sec->c_h)
		return EXFIT_StuckInCeiling;

	if (z1 < sec->f_h)
		return EXFIT_StuckInFloor;

	for (ef=sec->bottom_ef; ef && ef->higher; ef=ef->higher)
	{
		float bottom = ef->bottom_h;
		float top = ef->top_h;

		SYS_ASSERT(top >= bottom);

		// here is another solid extrafloor, check for overlap
		if (z2 > bottom && z1 < top)
			return EXFIT_StuckInExtraFloor;
	}

	return EXFIT_Ok;
}

//
// P_AddExtraFloor
//
void P_AddExtraFloor(sector_t *sec, line_t *line)
{
	sector_t *ctrl = line->frontsector;
	const extrafloordef_c *ef_info;

	surface_t *top, *bottom;
	extrafloor_t *newbie, *cur;

	bool liquid;
	exfloor_fit_e errcode;

	SYS_ASSERT(line->special);
	SYS_ASSERT(line->special->ef.type & EXFL_Present);

	ef_info = &line->special->ef;

	//
	// -- create new extrafloor --
	// 

	SYS_ASSERT(sec->exfloor_used <= sec->exfloor_max);

	if (sec->exfloor_used == sec->exfloor_max)
		I_Error("INTERNAL ERROR: extrafloor overflow in sector %d\n",
		(int)(sec - sectors));

	newbie = sec->exfloor_first + sec->exfloor_used;
	sec->exfloor_used++;

	Z_Clear(newbie, extrafloor_t, 1);

	bottom = &ctrl->floor;
	top = (ef_info->type & EXFL_Thick) ? &ctrl->ceil : bottom;

	// Handle the BOOMTEX flag (Boom compatibility)
	if (ef_info->type & EXFL_BoomTex)
	{
		bottom = &ctrl->ceil;
		top = &sec->floor;
	}

	newbie->bottom_h = ctrl->f_h;
	newbie->top_h = (ef_info->type & EXFL_Thick) ? ctrl->c_h : newbie->bottom_h;

	if (newbie->top_h < newbie->bottom_h)
		I_Error("Bad Extrafloor in sector #%d: "
		"z range is %1.0f / %1.0f\n", (int)(sec - sectors),
		newbie->bottom_h, newbie->top_h);

	newbie->sector = sec;
	newbie->top = top;
	newbie->bottom = bottom;

	newbie->p = &ctrl->props;
	newbie->ef_info = ef_info;
	newbie->ef_line = line;

	// Insert into the dummy's linked list
	newbie->ctrl_next = ctrl->control_floors;
	ctrl->control_floors = newbie;

	//
	// -- handle liquid extrafloors --
	//

	liquid = (ef_info->type & EXFL_Liquid)?true:false;

	if (liquid)
	{
		// find place to link into.  cur will be the next higher liquid,
		// or NULL if this is the highest.

		for (cur=sec->bottom_liq; cur; cur=cur->higher)
		{
			if (cur->bottom_h > newbie->bottom_h)
				break;
		}

		newbie->higher = cur;
		newbie->lower = cur ? cur->lower : sec->top_liq;

		if (newbie->higher)
			newbie->higher->lower = newbie;
		else
			sec->top_liq = newbie;

		if (newbie->lower)
			newbie->lower->higher = newbie;
		else
			sec->bottom_liq = newbie;

		return;
	}

	//
	// -- handle solid extrafloors --
	//

	// check if fits
	errcode = P_ExtraFloorFits(sec, newbie->bottom_h, newbie->top_h);

	switch (errcode)
	{
		case EXFIT_Ok:
			break;

		case EXFIT_StuckInCeiling:
			I_Error("Extrafloor with z range of %1.0f / %1.0f is stuck "
				"in sector #%d's ceiling.\n",
				newbie->bottom_h, newbie->top_h, (int)(sec - sectors));

		case EXFIT_StuckInFloor:
			I_Error("Extrafloor with z range of %1.0f / %1.0f is stuck "
				"in sector #%d's floor.\n",
				newbie->bottom_h, newbie->top_h, (int)(sec - sectors));

		default:
			I_Error("Extrafloor with z range of %1.0f / %1.0f is stuck "
				"in sector #%d in another extrafloor.\n",
				newbie->bottom_h, newbie->top_h, (int)(sec - sectors));
	}

	// find place to link into.  cur will be the next higher extrafloor,
	// or NULL if this is the highest.

	for (cur=sec->bottom_ef; cur; cur=cur->higher)
	{
		if (cur->bottom_h > newbie->bottom_h)
			break;
	}

	newbie->higher = cur;
	newbie->lower = cur ? cur->lower : sec->top_ef;

	if (newbie->higher)
		newbie->higher->lower = newbie;
	else
		sec->top_ef = newbie;

	if (newbie->lower)
		newbie->lower->higher = newbie;
	else
		sec->bottom_ef = newbie;
}

//
// P_FloodExtraFloors
//
void P_FloodExtraFloors(sector_t *sector)
{
	extrafloor_t *S, *L, *C;

	region_properties_t *props;
	region_properties_t *flood_p = NULL, *last_p = NULL;

	sector->p = &sector->props;

	// traverse downwards, stagger both lists
	S = sector->top_ef;
	L = sector->top_liq;

	while (S || L)
	{
		if (!L || (S && S->bottom_h > L->bottom_h))
		{
			C = S;  S = S->lower;
		}
		else
		{
			C = L;  L = L->lower;
		}

		SYS_ASSERT(C);

		props = &C->ef_line->frontsector->props;

		if (C->ef_info->type & EXFL_Flooder)
		{
			C->p = last_p = flood_p = props;

			if ((C->ef_info->type & EXFL_Liquid) && C->bottom_h >= sector->c_h)
				sector->p = flood_p;

			continue;
		}

		if (C->ef_info->type & EXFL_NoShade)
		{
			if (! last_p)
				last_p = props;

			C->p = last_p;
			continue;
		}

		C->p = last_p = flood_p ? flood_p : props;
	}
}


static INLINE void AddWallTile(side_t *sd, float z1, float z2,
							   float tex_z, surface_t *surface, int flags)
{
	wall_tile_t *wt;

	if (! surface->image)
		return;

	if (sd->tile_used >= sd->tile_max)
		return;

	wt = sd->tiles + sd->tile_used;
	sd->tile_used++;

	wt->z1 = z1;
	wt->z2 = z2;
	wt->tex_z = tex_z;
	wt->surface = surface;
	wt->flags = flags;
}

//
// P_ComputeWallTiles
//
void P_ComputeWallTiles(line_t *ld, int sidenum)
{
	side_t *sd = ld->side[sidenum];
	sector_t *sec, *other;
	surface_t *surf;

	extrafloor_t *S, *L, *C;
	float floor_h;
	float tex_z;

	bool lower_invis = false;
	bool upper_invis = false;


	if (! sd)
		return;

	sec = sd->sector;
	other = sidenum ? ld->frontsector : ld->backsector;

	// clear existing tiles
	sd->tile_used = 0;

	if (! other)
	{
		if (! sd->middle.image)
			return;

		AddWallTile(sd, sec->f_h, sec->c_h, 
			(ld->flags & ML_LowerUnpegged) ? 
			sec->f_h + IM_HEIGHT(sd->middle.image) : sec->c_h,
			&sd->middle, 0);
		return;
	}

	// handle lower, upper and mid-masker

	if (sec->f_h < other->f_h)
	{
		if (sd->bottom.image)
			AddWallTile(sd, sec->f_h, other->f_h, 
			(ld->flags & ML_LowerUnpegged) ? sec->c_h : other->f_h,
			&sd->bottom, 0);
		else
			lower_invis = true;
	}

	if (sec->c_h > other->c_h &&
		! (IS_SKY(sec->ceil) && IS_SKY(other->ceil)))
	{
		if (sd->top.image)
			AddWallTile(sd, other->c_h, sec->c_h, 
			(ld->flags & ML_UpperUnpegged) ? sec->c_h : 
		other->c_h + IM_HEIGHT(sd->top.image), &sd->top, 0);
		else
			upper_invis = true;
	}

	if (sd->middle.image)
	{
		float f1 = MAX(sec->f_h, other->f_h);
		float c1 = MIN(sec->c_h, other->c_h);

		float f2, c2;

		if (ld->flags & ML_LowerUnpegged)
		{
			f2 = f1 + sd->midmask_offset;
			c2 = f2 + IM_HEIGHT(sd->middle.image);
		}
		else
		{
			c2 = c1 + sd->midmask_offset;
			f2 = c2 - IM_HEIGHT(sd->middle.image);
		}

		tex_z = c2;

		// hack for transparent doors
		{
			if (lower_invis) f1 = sec->f_h;
			if (upper_invis) c1 = sec->c_h;
		}

		// hack for "see-through" lines (same sector on both sides)
		if (sec != other)
		{
			f2 = MAX(f2, f1);
			c2 = MIN(c2, c1);
		}

		if (c2 > f2)
		{
			AddWallTile(sd, f2, c2, tex_z, &sd->middle, WTILF_MidMask);
		}
	}

	// -- thick extrafloor sides --

	// -AJA- Don't bother drawing extrafloor sides if the front/back
	//       sectors have the same tag (and thus the same extrafloors).
	//
	if (other->tag == sec->tag)
		return;

	floor_h = other->f_h;

	S = other->bottom_ef;
	L = other->bottom_liq;

	while (S || L)
	{
		if (!L || (S && S->bottom_h < L->bottom_h))
		{
			C = S;  S = S->higher;
		}
		else
		{
			C = L;  L = L->higher;
		}

		SYS_ASSERT(C);

		// ignore liquids in the middle of THICK solids, or below real
		// floor or above real ceiling
		//
		if (C->bottom_h < floor_h || C->bottom_h > other->c_h)
			continue;

		if (C->ef_info->type & EXFL_Thick)
		{
			int flags = 0;

			// -AJA- 1999/09/25: Better DDF control of side texture.
			if (C->ef_info->type & EXFL_SideUpper)
				surf = &sd->top;
			else if (C->ef_info->type & EXFL_SideLower)
				surf = &sd->bottom;
			else
			{
				surf = &C->ef_line->side[0]->middle;

				flags |= WTILF_ExtraX;

				if (C->ef_info->type & EXFL_SideMidY)
					flags |= WTILF_ExtraY;
			}

			tex_z = (C->ef_line->flags & ML_LowerUnpegged) ?
				C->bottom_h + IM_HEIGHT(surf->image) : C->top_h;

			AddWallTile(sd, C->bottom_h, C->top_h, tex_z, surf, flags);
		}

		floor_h = C->top_h;
	}
}

//
// P_RecomputeGapsAroundSector
// 
void P_RecomputeGapsAroundSector(sector_t *sec)
{
	int i;

	for (i=0; i < sec->linecount; i++)
	{
		P_ComputeGaps(sec->lines[i]);
	}

	// now do the sight gaps...

	if (sec->c_h <= sec->f_h)
	{
		sec->sight_gap_num = 0;
		return;
	}

	sec->sight_gap_num = GAP_SightConstruct(sec->sight_gaps, sec);
}

void P_RecomputeTilesInSector(sector_t *sec)
{
	int i;

	for (i=0; i < sec->linecount; i++)
	{
		P_ComputeWallTiles(sec->lines[i], 0);
		P_ComputeWallTiles(sec->lines[i], 1);
	}
}


//--------------------------------------------------------------------------

//
//  THING POSITION SETTING
//

static INLINE touch_node_t *TouchNodeAlloc(void)
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

static INLINE void TouchNodeFree(touch_node_t *tn)
{
#ifdef DEVELOPERS
	touchstat_free++;
#endif

	// PREV field is ignored in quick-alloc list
	tn->mo_next = free_touch_nodes;
	free_touch_nodes = tn;
}

static INLINE void TouchNodeLinkIntoSector(touch_node_t *tn, sector_t *sec)
{
	tn->sec = sec;

	tn->sec_next = sec->touch_things;
	tn->sec_prev = NULL;

	if (tn->sec_next)
		tn->sec_next->sec_prev = tn;

	sec->touch_things = tn;
}

static INLINE void TouchNodeLinkIntoThing(touch_node_t *tn, mobj_t *mo)
{
	tn->mo = mo;

	tn->mo_next = mo->touch_sectors;
	tn->mo_prev = NULL;

	if (tn->mo_next)
		tn->mo_next->mo_prev = tn;

	mo->touch_sectors = tn;
}

static INLINE void TouchNodeUnlinkFromSector(touch_node_t *tn)
{
	if (tn->sec_next)
		tn->sec_next->sec_prev = tn->sec_prev;

	if (tn->sec_prev)
		tn->sec_prev->sec_next = tn->sec_next;
	else
		tn->sec->touch_things = tn->sec_next;
}

static INLINE void TouchNodeUnlinkFromThing(touch_node_t *tn)
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
static void P_UnsetThingPosition(mobj_t * thing)
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
	if (thing->info && thing->info->dlight.type != DLITE_None)
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
	if (thing->info && thing->info->dlight.type != DLITE_None)
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
void P_ChangeThingPosition(mobj_t * thing, float x, float y, float z)
{
	P_UnsetThingPosition(thing);

	thing->x = x;
	thing->y = y;
	thing->z = z;

	P_SetThingPosition(thing);
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


//
// INTERCEPT ROUTINES
//

// --> Intercept list class
class interceptarray_c : public epi::array_c
{
public:
	interceptarray_c() : epi::array_c(sizeof(intercept_t)) {}
	~interceptarray_c() { Clear(); }

private:
	void CleanupObject(void *obj) { /* ... */ }

public:
	int GetSize() { return array_entries; } 
	int Insert(intercept_t *in) { return InsertObject((void*)in); }
	intercept_t* operator[](int idx) { return (intercept_t*)FetchObject(idx); } 
};

interceptarray_c intercepts;

divline_t trace;
bool earlyout;
int ptflags;

//
// PIT_AddLineIntercepts.
//
// Looks for lines in the given block
// that intercept the given trace
// to add to the intercepts list.
//
// A line is crossed if its endpoints
// are on opposite sides of the trace.
// Returns true if earlyout and a solid line hit.
//
static bool PIT_AddLineIntercepts(line_t * ld)
{
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

	if (frac < 0)
		return true;  // behind source

	// try to early out the check
	if (earlyout && frac < 1.0f && !ld->backsector)
		return false;  // stop checking

	// Intercept is a simple struct that can be memcpy()'d: Load
	// up a structure and get into the array
	intercept_t in;
	
	in.frac = frac;
	in.type = INCPT_Line;
	in.d.line = ld;
	
	intercepts.Insert(&in);
	return true;  // continue

}

//
// PIT_AddThingIntercepts
//
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

	// behind source ?
	if (frac < 0)
		return true;

	// Intercept is a simple struct that can be memcpy()'d: Load
	// up a structure and get into the array
	intercept_t in;
	
	in.frac = frac;
	in.type = INCPT_Thing;
	in.d.thing = thing;
	
	intercepts.Insert(&in);
	return true;
}

//
// TraverseIntercepts
//
// Returns true if the traverser function returns true
// for all lines.
//
static bool TraverseIntercepts(traverser_t func, float maxfrac)
{
	epi::array_iterator_c it;
	int count;
	float dist;
	intercept_t *in = NULL;
	intercept_t *scan_in = NULL;

	count = intercepts.GetSize();
	while (count--)
	{
		dist = FLT_MAX;

		for (it = intercepts.GetBaseIterator(); it.IsValid(); it++)
		{
			scan_in = ITERATOR_TO_PTR(it, intercept_t);
			if (scan_in->frac < dist)
			{
				dist = scan_in->frac;
				in = scan_in;
			}
		}

		if (dist > maxfrac)
		{
			// checked everything in range  
			return true;
		}

		if (!func(in))
		{
			// don't bother going farther
			return false;
		}

		in->frac = FLT_MAX;
	}

	// everything was traversed
	return true;
}

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
	int xt1;
	int yt1;
	int xt2;
	int yt2;

	float xstep;
	float ystep;

	float partial;

	float xintercept;
	float yintercept;

	double tmp;

	int mapx;
	int mapy;

	int mapxstep;
	int mapystep;

	int count;

	earlyout = (flags & PT_EARLYOUT)?true:false;

	validcount++;

	//
	// -ACB- 2004/08/02 We don't clear the array since there is no need
	// to free the memory
	//
	intercepts.ZeroiseCount();

	if (fmod(x1 - bmaporgx, MAPBLOCKUNITS) == 0)
	{
		// don't side exactly on a line
		x1 += 1.0f;
	}

	if (fmod(y1 - bmaporgy, MAPBLOCKUNITS) == 0)
	{
		// don't side exactly on a line
		y1 += 1.0f;
	}

	trace.x = x1;
	trace.y = y1;
	trace.dx = x2 - x1;
	trace.dy = y2 - y1;

	x1 -= bmaporgx;
	y1 -= bmaporgy;
	x2 -= bmaporgx;
	y2 -= bmaporgy;

	xt1 = (int)(x1 / MAPBLOCKUNITS);
	yt1 = (int)(y1 / MAPBLOCKUNITS);
	xt2 = (int)(x2 / MAPBLOCKUNITS);
	yt2 = (int)(y2 / MAPBLOCKUNITS);

	if (xt2 > xt1)
	{
		mapxstep = 1;
		partial = 1.0f - modf(x1 / MAPBLOCKUNITS, &tmp);

		// -ACB- 2000/03/11 Div-by-zero check...
		CHECKVAL(x2-x1);

		ystep = (y2 - y1) / fabs(x2 - x1);
	}
	else if (xt2 < xt1)
	{
		mapxstep = -1;
		partial = modf(x1 / MAPBLOCKUNITS, &tmp);

		// -ACB- 2000/03/11 Div-by-zero check...
		CHECKVAL(x2-x1);

		ystep = (y2 - y1) / fabs(x2 - x1);
	}
	else
	{
		mapxstep = 0;
		partial = 1.0f;
		ystep = 256.0f;
	}

	yintercept = y1 / MAPBLOCKUNITS + partial * ystep;

	if (yt2 > yt1)
	{
		mapystep = 1;
		partial = 1.0f - modf(y1 / MAPBLOCKUNITS, &tmp);

		// -ACB- 2000/03/11 Div-by-zero check...
		CHECKVAL(y2-y1);

		xstep = (x2 - x1) / fabs(y2 - y1);
	}
	else if (yt2 < yt1)
	{
		mapystep = -1;
		partial = modf(y1 / MAPBLOCKUNITS, &tmp);

		// -ACB- 2000/03/11 Div-by-zero check...
		CHECKVAL(y2-y1);

		xstep = (x2 - x1) / fabs(y2 - y1);
	}
	else
	{
		mapystep = 0;
		partial = 1.0f;
		xstep = 256.0f;
	}
	xintercept = x1 / MAPBLOCKUNITS + partial * xstep;

	// Step through map blocks.
	// Count is present to prevent a round off error
	// from skipping the break.
	mapx = xt1;
	mapy = yt1;

	for (count = 0; count < 64; count++)
	{
		if (flags & PT_ADDLINES)
		{
			if (!P_BlockLinesIterator(mapx, mapy, PIT_AddLineIntercepts))
				return false;
		}

		if (flags & PT_ADDTHINGS)
		{
			if (!P_BlockThingsIterator(mapx, mapy, PIT_AddThingIntercepts))
				return false;
		}

		if (mapx == xt2 && mapy == yt2)
			break;

		if (mapy == (int)yintercept)
		{
			yintercept += ystep;
			mapx += mapxstep;
		}
		else if (mapx == (int)xintercept)
		{
			xintercept += xstep;
			mapy += mapystep;
		}
	}

	// go through the sorted list
	return TraverseIntercepts(trav, 1.0f);
}

static INLINE bool PST_CheckBBox(float *bspcoord, float *test)
{
	return (test[BOXRIGHT]  < bspcoord[BOXLEFT] ||
		test[BOXLEFT]   > bspcoord[BOXRIGHT] ||
		test[BOXTOP]    < bspcoord[BOXBOTTOM] ||
		test[BOXBOTTOM] > bspcoord[BOXTOP]) ? false : true;
}

static bool TraverseSubsec(unsigned int bspnum, float *bbox,
								bool (*func)(mobj_t *mo))
{
	subsector_t *sub;
	node_t *node;
	mobj_t *obj;

	// just a normal node ?
	if (! (bspnum & NF_V5_SUBSECTOR))
	{
		node = nodes + bspnum;

		// recursively check the children nodes
		// OPTIMISE: check against partition lines instead of bboxes.

		if (PST_CheckBBox(node->bbox[0], bbox))
		{
			if (! TraverseSubsec(node->children[0], bbox, func))
				return false;
		}

		if (PST_CheckBBox(node->bbox[1], bbox))
		{
			if (! TraverseSubsec(node->children[1], bbox, func))
				return false;
		}

		return true;
	}

	// the sharp end: check all things in the subsector

	sub = subsectors + (bspnum & ~NF_V5_SUBSECTOR);

	for (obj=sub->thinglist; obj; obj=obj->snext)
	{
		if (! (* func)(obj))
			return false;
	}

	return true;
}

//
// P_SubsecThingIterator
//
// Iterate over all things that touch a certain rectangle on the map,
// using the BSP tree.
//
// If any function returns false, then this routine returns false and
// nothing else is checked.  Otherwise true is returned.
//
bool P_SubsecThingIterator(float *bbox,
								bool (*func)(mobj_t *mo))
{
	return TraverseSubsec(root_node, bbox, func);
}

static float checkempty_bbox[4];
static line_t *checkempty_line;

static bool PST_CheckThingArea(mobj_t *mo)
{
	if (mo->x + mo->radius < checkempty_bbox[BOXLEFT] ||
		mo->x - mo->radius > checkempty_bbox[BOXRIGHT] ||
		mo->y + mo->radius < checkempty_bbox[BOXBOTTOM] ||
		mo->y - mo->radius > checkempty_bbox[BOXTOP])
	{
		// keep looking
		return true;
	}

	// ignore corpses and pickup items
	if (! (mo->flags & MF_SOLID) && (mo->flags & MF_CORPSE))
		return true;

	if (mo->flags & MF_SPECIAL)
		return true;

	// we've found a thing in that area: we can stop now
	return false;
}

static bool PST_CheckThingLine(mobj_t *mo)
{
	float bbox[4];
	int side;

	bbox[BOXLEFT]   = mo->x - mo->radius;
	bbox[BOXRIGHT]  = mo->x + mo->radius;
	bbox[BOXBOTTOM] = mo->y - mo->radius;
	bbox[BOXTOP]    = mo->y + mo->radius;

	// found a thing on the line ?
	side = P_BoxOnLineSide(bbox, checkempty_line);

	if (side != -1)
		return true;

	// ignore corpses and pickup items
	if (! (mo->flags & MF_SOLID) && (mo->flags & MF_CORPSE))
		return true;

	if (mo->flags & MF_SPECIAL)
		return true;

	return false;
}

//
// P_ThingsInArea
//
// Checks if there are any things contained within the given rectangle
// on the 2D map.
//
bool P_ThingsInArea(float *bbox)
{
	checkempty_bbox[BOXLEFT]   = bbox[BOXLEFT];
	checkempty_bbox[BOXRIGHT]  = bbox[BOXRIGHT];
	checkempty_bbox[BOXBOTTOM] = bbox[BOXBOTTOM];
	checkempty_bbox[BOXTOP]    = bbox[BOXTOP];

	return ! P_SubsecThingIterator(bbox, PST_CheckThingArea);
}

//
// P_ThingsOnLine
//
// Checks if there are any things touching the given line.
//
bool P_ThingsOnLine(line_t *ld)
{
	checkempty_line = ld;

	return ! P_SubsecThingIterator(ld->bbox, PST_CheckThingLine);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
