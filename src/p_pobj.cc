//----------------------------------------------------------------------------
//  EDGE Poly-Object Code
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2017  The EDGE Team.
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

#include "system/i_defs.h"
#include "r_defs.h"
#include "r_misc.h"
#include "m_bbox.h"
#include "p_mobj.h"
#include "p_pobj.h"

#include "../ddf/main.h"



static polyobj_t *polyobjects = (polyobj_t*)NULL;
static int numpolyobjs = 0;

extern int numlines;
extern line_t *lines;

extern int numsegs;
extern seg_t *segs;


static int PO_FindLines(line_t *start, polyobj_t *po)
{
	int count = 0;
	vec2_t *v1s = start->v1;
	seg_t *seg = po ? po->segs : (seg_t*)NULL;
	line_t **pline = po ? po->lines : (line_t**)NULL;

	line_t *ld = start; // starting line of PO
	while (ld)
	{
		int i, j;

		if (pline)
			pline[count] = ld;

		if (seg)
		{
			// find any segment on the line
			seg_t *ls = segs;
			for (j=0; j<numsegs; j++, ls++)
				if (ls->linedef == ld)
					break;
			SYS_ASSERT(j != numsegs);
			// make fake segment from real one
			memcpy(seg, ls, sizeof(seg_t));
			seg->v1 = ld->v1;
			seg->v2 = ld->v2;
			seg->offset = 0.0f;
			seg->angle  = R_PointToAngle(ld->v1->x, ld->v1->y, ld->v2->x, ld->v2->y);
			seg->length = R_PointToDist(ld->v1->x, ld->v1->y, ld->v2->x, ld->v2->y);
			seg++;
		}

		count++;

		// check if done
		if (ld->v2 == v1s)
			break;

		// next line
		for (i=0; i<numlines; i++)
		{
			line_t *ln = lines + i;
			if (ln->v1 == ld->v2)
			{
				ld = ln;
				break;
			}
		}
	}

	return count;
}

static inline void PO_RecomputeLinedefData(line_t *ld)
{
	vec2_t *v1 = ld->v1;
	vec2_t *v2 = ld->v2;

	ld->dx = v2->x - v1->x;
	ld->dy = v2->y - v1->y;

	if (ld->dx == 0)
		ld->slopetype = ST_VERTICAL;
	else if (ld->dy == 0)
		ld->slopetype = ST_HORIZONTAL;
	else if (ld->dy / ld->dx > 0)
		ld->slopetype = ST_POSITIVE;
	else
		ld->slopetype = ST_NEGATIVE;

	ld->length = R_PointToDist(0, 0, ld->dx, ld->dy);

	if (v1->x < v2->x)
	{
		ld->bbox[BOXLEFT] = v1->x;
		ld->bbox[BOXRIGHT] = v2->x;
	}
	else
	{
		ld->bbox[BOXLEFT] = v2->x;
		ld->bbox[BOXRIGHT] = v1->x;
	}

	if (v1->y < v2->y)
	{
		ld->bbox[BOXBOTTOM] = v1->y;
		ld->bbox[BOXTOP] = v2->y;
	}
	else
	{
		ld->bbox[BOXBOTTOM] = v2->y;
		ld->bbox[BOXTOP] = v1->y;
	}
}

void P_ClearPolyobjects(void)
{
	if (polyobjects)
	{
		for (int i=0; i<numpolyobjs; i++)
		{
			if (polyobjects[i].lines)
				free(polyobjects[i].lines);
			if (polyobjects[i].segs)
				free(polyobjects[i].segs);
		}

		free(polyobjects);
		polyobjects = (polyobj_t*)NULL;
		numpolyobjs = 0;
	}
}

void P_AddPolyobject(int ix, line_t *start)
{
	if (numpolyobjs)
		polyobjects = (polyobj_t*)realloc(polyobjects, (numpolyobjs + 1) * sizeof(polyobj_t)); //TODO: V701 https://www.viva64.com/en/w/v701/ realloc() possible leak: when realloc() fails in allocating memory, original pointer 'polyobjects' is lost. Consider assigning realloc() to a temporary pointer.
	else
		polyobjects = (polyobj_t*)malloc((numpolyobjs + 1) * sizeof(polyobj_t));

	polyobj_t *po = polyobjects + numpolyobjs; //TODO: V769 https://www.viva64.com/en/w/v769/ The 'polyobjects' pointer in the 'polyobjects + numpolyobjs' expression could be nullptr. In such case, resulting value will be senseless and it should not be used.
	numpolyobjs++;
	memset((void*)po, 0, sizeof(polyobj_t));

	po->index = ix;
	po->start = start;

	//I_Printf("P_AddPolyobject: Added PO %d for line %p\n", ix, start);
} //TODO: V773 https://www.viva64.com/en/w/v773/ Visibility scope of the 'po' pointer was exited without releasing the memory. A memory leak is possible.

// called once after level data is loaded to alter PO vertexes to where they should appear
void P_PostProcessPolyObjs(void)
{
	for (int i=0; i<numpolyobjs; i++)
	{
		polyobj_t *po = polyobjects + i;
		seg_t *seg;

		I_Printf("P_PostProcessPolyobjs: Finding all lines for PO %d\n", po->index);
		po->count = PO_FindLines(po->start, (polyobj_t*)NULL);
		if (po->count)
		{
			po->lines = (line_t**)malloc(po->count * sizeof(line_t*));
			po->segs = (seg_t*)malloc(po->count * sizeof(seg_t));
			I_Printf("  Found %d lines\n", po->count);
			PO_FindLines(po->start, po);
		}
		else
		{
			I_Debugf("  No lines found for PO %d\n", po->index);
			continue;
		}

		mobj_t *mo = po->mobj;
		if (!mo)
		{
			I_Printf("  No Mobj found for PO %d\n", po->index);
			continue;
		}

		mobj_t *ao = po->anchor;
		if (!ao)
		{
			I_Printf("  No anchor found for PO %d\n", po->index);
			continue;
		}

		I_Printf("  Processing vertexes for PO %d\n", po->index);
		seg = po->segs;
		for (int j=0; j<po->count; j++, seg++)
		{
			//I_Printf("P_PostProcessPolyobjs: Processing seg %p\n", seg);
			//I_Printf(" (%f, %f), (%f, %f)\n", seg->v1->x, seg->v1->y, seg->v2->x, seg->v2->y);
			if (seg->v1->x < 1000000.0f)
			{
				seg->v1->x -= ao->x;
				seg->v1->x += mo->x + 2000000.0f;
				seg->v1->y -= ao->y;
				seg->v1->y += mo->y;
			}
			if (seg->v2->x < 1000000.0f)
			{
				seg->v2->x -= ao->x;
				seg->v2->x += mo->x + 2000000.0f;
				seg->v2->y -= ao->y;
				seg->v2->y += mo->y;
			}
		}

		po->last_x = mo->x;
		po->last_y = mo->y;

		// now get rid of markers
		seg = po->segs;
		for (int j=0; j<po->count; j++, seg++)
		{
			if (seg->v1->x > 1000000.0f)
				seg->v1->x -= 2000000.0f;
			if (seg->v2->x > 1000000.0f)
				seg->v2->x -= 2000000.0f;
		}

		// recompute line data
		for (int j=0; j<po->count; j++)
			PO_RecomputeLinedefData(po->lines[j]);

		I_Printf("  Done processing for PO %d\n", po->index);
	}
}

void P_UpdatePolyObj(mobj_t *mo)
{
	polyobj_t *po = P_GetPolyobject(mo->po_ix);
	seg_t *seg;

	// anchor doesn't need processing
	if (mo->typenum == 9300)
		return;

	if (mo->radius <= 20)
	{
		// compute radius for polyobject
		seg = po->segs;
		float minx = seg->v1->x;
		float maxx = seg->v2->x;
		float miny = seg->v1->y;
		float maxy = seg->v2->y;
		for (int j=0; j<po->count; j++, seg++)
		{
			if (seg->v1->x < minx)
				minx = seg->v1->x;
			if (seg->v2->x > maxx)
				maxx = seg->v2->x;
			if (seg->v1->y < miny)
				miny = seg->v1->y;
			if (seg->v2->y > maxy)
				maxy = seg->v2->y;
		}
		mo->radius = fabsf(maxx - minx) > fabsf(maxy - miny) ?
			fabsf(maxx - minx) / 2.0f : fabsf(maxy - miny) / 2.0f;
		I_Printf("P_UpdatePolyObj: mobj radius = %f\n", mo->radius);
	}

	if (po->last_x != mo->x || po->last_y != mo->y)
	{
		float dx = mo->x - po->last_x;
		float dy = mo->y - po->last_y;

		seg = po->segs;
		for (int j=0; j<po->count; j++, seg++)
		{
			if (seg->v1->x < 1000000.0f)
			{
				seg->v1->x += dx + 2000000.0f;
				seg->v1->y += dy;
			}
			if (seg->v2->x < 1000000.0f)
			{
				seg->v2->x += dx + 2000000.0f;
				seg->v2->y += dy;
			}
		}

		po->last_x = mo->x;
		po->last_y = mo->y;

		// now get rid of markers
		seg = po->segs;
		for (int j=0; j<po->count; j++, seg++)
		{
			if (seg->v1->x > 1000000.0f)
				seg->v1->x -= 2000000.0f;
			if (seg->v2->x > 1000000.0f)
				seg->v2->x -= 2000000.0f;
		}

		if (po->state)
		{
			// action in progress
			if (po->action == 2)
			{
				// rotate left
				int new_angle = (po->last_angle + po->args[1]) & 0x0000FFFF;
				if (new_angle >= (po->args[2] << 8))
					po->state = 0; // done
				// do rotation



				po->last_angle = new_angle;
			}


		}

		// recompute line data
		for (int j=0; j<po->count; j++)
			PO_RecomputeLinedefData(po->lines[j]);
	}
}

polyobj_t *P_GetPolyobject(int ix)
{
	for (int i=0; i<numpolyobjs; i++)
		if (polyobjects[i].index == ix)
			return polyobjects + i;

	I_Debugf("P_GetPolyobject: couldn't find PO %d\n", ix);
	return (polyobj_t*)NULL;
}


void PO_RotateLeft(int pobj, int speed, int angle)
{
	polyobj_t *po = P_GetPolyobject(pobj);
	if (po)
	{
		po->action = 2;
		po->args[0] = pobj;
		po->args[1] = speed;
		po->args[2] = angle;

		po->state = 1;
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
