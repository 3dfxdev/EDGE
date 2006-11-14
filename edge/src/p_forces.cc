//----------------------------------------------------------------------------
//  EDGE Sector Forces (wind / current / points)
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
//  Based on code from PrBoom:
//
//  PrBoom a Doom port merged with LxDoom and LSDLDoom
//  based on BOOM, a modified and improved DOOM engine
//  Copyright (C) 1999 by
//  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
//  Copyright (C) 1999-2000 by
//  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
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

#include "dm_defs.h"
#include "dm_state.h"
#include "m_random.h"
#include "p_local.h"
#include "r_state.h"
#include "z_zone.h"

#define PUSH_FACTOR  128.0f

typedef struct force_s
{
	bool is_point;
	bool is_wind;

	vec3_t point;

	float radius;
	float magnitude;

	vec2_t mag;  // wind/current

	sector_t *sector;  // the affected sector

	struct force_s *next, *prev;
}
force_t;

force_t *forces = NULL;

static force_t *tm_force;  // for PIT_PushThing


static void WindCurrentForce(force_t *f, mobj_t *mo)
{
	float qty = 1.0f;

	if (f->is_wind)
	{
		if (mo->z > f->sector->f_h + 1.0f)
			qty = 0.5f;
	}
	else // current
	{
		if (mo->z > f->sector->f_h + 1.0f)
			return;

		qty = 0.5f;
	}

	mo->mom.x += qty * f->mag.x;
	mo->mom.y += qty * f->mag.y;
}

static bool PIT_PushThing(mobj_t *mo)
{
	if (! (mo->hyperflags & HF_PUSHABLE))
	{
		// FIXME FIXME @@@
	}

	return true;
}

//
// GENERALISED FORCE
//
static void DoForce(force_t * f)
{
	sector_t *sec = f->sector;

    if (sec->props.type & MSF_Push)
	{
		if (f->is_point)
		{
			// FIXME: point forces : BlockThingIterator
		}
		else // wind/current
		{
			touch_node_t *nd;

			for (nd = sec->touch_things; nd; nd = nd->sec_next)
				if (nd->mo->hyperflags & HF_PUSHABLE)
					WindCurrentForce(f, nd->mo);
		}
	}
}


//
// P_DestroyAllForces
//
void P_DestroyAllForces(void)
{
	force_t *f, *next;

	for (f = forces; f; f = next)
	{
		next = f->next;
		Z_Free(f);
	}
	forces = NULL;
}

//
// Allocate and link in the force.
//
static force_t *P_NewForce(void)
{
	force_t *f = Z_ClearNew(force_t, 1);

	f->next = forces;;
	f->prev = NULL;

	if (forces)
		forces->prev = f;

	forces = f;

	return f;
}

void P_AddPointForce(sector_t *sec, float length)
{
	// search for the point objects
	for (subsector_t *sub = sec->subsectors; sub; sub = sub->sec_next)
		for (mobj_t *mo = sub->thinglist; mo; mo = mo->snext)
			if (mo->hyperflags & HF_POINT_FORCE)
			{
				force_t *f = P_NewForce();

				f->is_point = true;
				f->point.x = mo->x;
				f->point.y = mo->y;
				f->point.z = MO_MIDZ(mo);
				f->radius = length * 2.0f;
				f->magnitude = length * mo->info->speed;
				f->sector = sec;
			}
}

void P_AddSectorForce(sector_t *sec, bool is_wind, float x_mag, float y_mag)
{
	force_t *f = P_NewForce();

	f->is_point = false;
	f->is_wind  = is_wind;
	f->mag.x  = x_mag * PUSH_FACTOR;
	f->mag.y  = y_mag * PUSH_FACTOR;
	f->sector = sec;
}

//
// P_RunForces
//
// Executes all force effects for this tic.
//
void P_RunForces(void)
{
	for (force_t *f = forces; f; f = f->next)
		DoForce(f);
}

