//----------------------------------------------------------------------------
//  EDGE Moving Object Handling Code
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
//
// -MH- 1998/07/02  "shootupdown" --> "true3dgameplay"
//
// -ACB- 1998/07/30 Took an axe to the item respawn code: now uses a
//                  double-linked list to store to individual items;
//                  limit removed; P_MobjItemRespawn replaces P_RespawnSpecials
//                  as the procedure that handles respawning of items.
//
//                  P_NightmareRespawnOld -> P_TeleportRespawn
//                  P_NightmareRespawnNew -> P_ResurrectRespawn
//
// -ACB- 1998/07/31 Use new procedure to handle flying missiles that hammer
//                  into sky-hack walls & ceilings. Also don't explode the
//                  missile if it hits sky-hack ceiling or floor.
//
// -ACB- 1998/08/06 Implemented limitless mobjdef list, altered/removed all
//                  mobjdef[] references.
//
// -AJA- 1999/07/21: Replaced some non-critical P_Randoms with M_Random.
//
// -AJA- 1999/07/30: Removed redundant code from P_SpawnMobj (it was
//                   virtually identical to P_MobjCreateObject).
//
// -AJA- 1999/09/15: Removed P_SpawnMobj itself :-).
//

#include "i_defs.h"
#include "p_mobj.h"

#include "con_main.h"
#include "con_cvar.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "m_argv.h"
#include "m_random.h"
#include "p_local.h"
#include "r_misc.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "z_zone.h"

#include "epi/arrays.h"

#include <list>

#define DEBUG_MOBJ  0

#if 1  // DEBUGGING
void P_DumpMobjs(void)
{
	mobj_t *mo;

	int index = 0;

	L_WriteDebug("MOBJs:\n");

	for (mo=mobjlisthead; mo; mo=mo->next, index++)
	{
		L_WriteDebug(" %4d: %p next:%p prev:%p [%s] at (%1.0f,%1.0f,%1.0f) states=%d > %d tics=%d\n",
			index,
			mo, mo->next, mo->prev,
			mo->info->ddf.name.GetString(),
			mo->x, mo->y, mo->z,
			mo->state ? mo->state - states : -1,
			mo->next_state ? mo->next_state - states : -1,
			mo->tics);
	}

	L_WriteDebug("END OF MOBJs\n");
}
#endif


// The Object Removal Que
class mobjlist_c : public epi::array_c
{
public:
	mobjlist_c() : epi::array_c(sizeof(mobj_t*)) {}
	~mobjlist_c() { Clear(); } 

private:
	void CleanupObject(void *obj) { /* Do Nothing */ }

public:
	int Insert(mobj_t *mo) 
	{ 
		epi::array_iterator_c it;
		
		for (it=GetBaseIterator(); it.IsValid(); it++)
		{
			if (mo == ITERATOR_TO_TYPE(it, mobj_t*))
				return -1;
		}
	
		return InsertObject((void*)&mo); 
	} 
};

// List of all objects in map.
mobj_t *mobjlisthead;

// Where objects go to die...
static std::list<mobj_t *> remove_queue;

iteminque_t *itemquehead;

// =========================== INTERNALS =========================== 

// convenience function
// -AJA- FIXME: duplicate code from p_map.c
static INLINE int PointOnLineSide(float x, float y, line_t *ld)
{
	divline_t div;

	div.x = ld->v1->x;
	div.y = ld->v1->y;
	div.dx = ld->dx;
	div.dy = ld->dy;

	return P_PointOnDivlineSide(x, y, &div);
}

//
// EnterBounceStates
//
// -AJA- 1999/10/18: written.
//
static void EnterBounceStates(mobj_t * mo)
{
	if (! mo->info->bounce_state)
		return;

	// ignore if disarmed
	if (mo->extendedflags & EF_JUSTBOUNCED)
		return;

	// give deferred states a higher priority
	if (!mo->state || !mo->next_state ||
		(mo->next_state - states) != mo->state->nextstate)
	{
		return;
	}

	mo->extendedflags |= EF_JUSTBOUNCED;

	P_SetMobjState(mo, mo->info->bounce_state);
}

//
// BounceOffWall
//
// -AJA- 1999/08/22: written.
//
static void BounceOffWall(mobj_t * mo, line_t * wall)
{
	angle_t angle;
	angle_t wall_angle;
	angle_t diff;

	divline_t div;
	float dest_x, dest_y;

	angle = R_PointToAngle(0, 0, mo->mom.x, mo->mom.y);
	wall_angle = R_PointToAngle(0, 0, wall->dx, wall->dy);

	diff = wall_angle - angle;

	if (diff > ANG90 && diff < ANG270)
		diff -= ANG180;

	// -AJA- Prevent getting stuck at some walls...

	dest_x = mo->x + M_Cos(angle) * (mo->speed + mo->info->radius) * 4.0f;
	dest_y = mo->y + M_Sin(angle) * (mo->speed + mo->info->radius) * 4.0f;

	div.x = wall->v1->x;
	div.y = wall->v1->y;
	div.dx = wall->dx;
	div.dy = wall->dy;

	if (P_PointOnDivlineSide(mo->x, mo->y, &div) ==
		P_PointOnDivlineSide(dest_x, dest_y, &div))
	{
		// Result is the same, thus we haven't crossed the line.  Choose a
		// random angle to bounce away.  And don't attenuate the speed (so
		// we can get far enough away).

		angle = P_Random() << (ANGLEBITS - 8);
	}
	else
	{
		angle += diff << 1;
	}

	// calculate new momentum

	mo->speed *= mo->info->bounce_speed;

	mo->mom.x = M_Cos(angle) * mo->speed;
	mo->mom.y = M_Sin(angle) * mo->speed;
	mo->angle = angle;

	EnterBounceStates(mo);
}

//
// BounceOffPlane
//
// -AJA- 1999/10/18: written.
//
static void BounceOffPlane(mobj_t * mo, float dir)
{
	// calculate new momentum

	mo->speed *= mo->info->bounce_speed;

	mo->mom.x = (float)(M_Cos(mo->angle) * mo->speed);
	mo->mom.y = (float)(M_Sin(mo->angle) * mo->speed);
	mo->mom.z = (float)(dir * mo->speed * mo->info->bounce_up);

	EnterBounceStates(mo);
}

//
// CorpseShouldSlide
//
// -AJA- 1999/09/25: written.
//
static bool CorpseShouldSlide(mobj_t * mo)
{
	float floor, ceil;

	if (-0.25f < mo->mom.x && mo->mom.x < 0.25f &&
		-0.25f < mo->mom.y && mo->mom.y < 0.25f)
	{
		return false;
	}

	P_ComputeThingGap(mo, mo->subsector->sector, mo->z, &floor, &ceil);

	return (mo->floorz != floor);
}

//
// TeleportRespawn
//
static void TeleportRespawn(mobj_t * mobj)
{
	float x, y, z, oldradius, oldheight;
	const mobjtype_c *info = mobj->spawnpoint.info;
	mobj_t *new_mo;
	int oldflags;

	if (!info)
		return;

	x = mobj->spawnpoint.x;
	y = mobj->spawnpoint.y;
	z = mobj->spawnpoint.z;

	// something is occupying it's position?

	//
	// -ACB- 2004/02/01 Check if the object can respawn in this position with
	// its correct radius. Should this check fail restore the old values back
	//
	oldradius = mobj->radius;
	oldheight = mobj->height;
	oldflags = mobj->flags;

	mobj->radius = mobj->spawnpoint.info->radius;
	mobj->height = mobj->spawnpoint.info->height;

	if (info->flags & MF_SOLID)						// Should it be solid?
		mobj->flags |= MF_SOLID;

	if (!P_CheckAbsPosition(mobj, x, y, z))
	{
		mobj->radius = oldradius;
		mobj->height = oldheight;
		mobj->flags = oldflags;
		return;
	}

	// spawn a teleport fog at old spot
	// because of removal of the body...

	// temp fix for teleport flash...
	if (info->respawneffect)
		P_MobjCreateObject(mobj->x, mobj->y, mobj->z, info->respawneffect);

	// spawn a teleport fog at the new spot...

	// temp fix for teleport flash...
	if (info->respawneffect)
		P_MobjCreateObject(x, y, z, info->respawneffect);

	// spawn it, inheriting attributes from deceased one
	// -ACB- 1998/08/06 Create Object
	new_mo = P_MobjCreateObject(x, y, z, info);

	new_mo->spawnpoint = mobj->spawnpoint;
	new_mo->angle = mobj->spawnpoint.angle;
	new_mo->vertangle = mobj->spawnpoint.vertangle;
	new_mo->tag = mobj->spawnpoint.tag;

	if (mobj->spawnpoint.flags & MF_AMBUSH)
		new_mo->flags |= MF_AMBUSH;

	new_mo->reactiontime = RESPAWN_DELAY;

	// remove the old monster.
	P_RemoveMobj(mobj);
}

//
// ResurrectRespawn
//
// -ACB- 1998/07/29 Prevented respawning of ghosts
//                  Make monster deaf, if originally deaf
//                  Given a reaction time, delays monster starting up immediately.
//                  Doesn't try to raise an object with no raisestate
//
static void ResurrectRespawn(mobj_t * mobj)
{
	float x, y, z, oldradius, oldheight;
	const mobjtype_c *info;
	int oldflags;

	x = mobj->x;
	y = mobj->y;
	z = mobj->z;

	info = mobj->info;

	// cannot raise the unraisable
	if (!info->raise_state)
		return;

	// don't respawn gibs
	if (mobj->extendedflags & EF_GIBBED)
		return;

	//
	// -ACB- 2004/02/01 Check if the object can respawn in this position with
	// its correct radius. Should this check fail restore the old values back
	//
	oldradius = mobj->radius;
	oldheight = mobj->height;
	oldflags = mobj->flags;

	mobj->radius = info->radius;
	mobj->height = info->height;

	if (info->flags & MF_SOLID)					// Should it be solid?
		mobj->flags |= MF_SOLID;

	if (!P_CheckAbsPosition(mobj, x, y, z))
	{
		mobj->radius = oldradius;
		mobj->height = oldheight;
		mobj->flags = oldflags;
		return;
	}

	// Resurrect monster
	if (info->overkill_sound)
		S_StartFX(info->overkill_sound, P_MobjGetSfxCategory(mobj), mobj);

	P_SetMobjState(mobj, info->raise_state);

	SYS_ASSERT(! mobj->isRemoved());

	mobj->flags = info->flags;
	mobj->extendedflags = info->extendedflags;
	mobj->hyperflags = info->hyperflags;
	mobj->health = info->spawnhealth;

	mobj->visibility = PERCENT_2_FLOAT(info->translucency);
	mobj->movecount = 0;  // -ACB- 1998/08/03 Don't head off in any direction

	mobj->SetSource(NULL);
	mobj->SetTarget(NULL);

	mobj->tag = mobj->spawnpoint.tag;

	if (mobj->spawnpoint.flags & MF_AMBUSH)
		mobj->flags |= MF_AMBUSH;

	mobj->reactiontime = RESPAWN_DELAY;
	return;
}

void mobj_t::ClearStaleRefs()
{
	if (target && target->isRemoved()) SetTarget(NULL);
	if (source && source->isRemoved()) SetSource(NULL);
	if (tracer && tracer->isRemoved()) SetTracer(NULL);

	if (supportobj && supportobj->isRemoved()) SetSupportObj(NULL);
	if (above_mo   && above_mo->isRemoved())   SetAboveMo(NULL);
	if (below_mo   && below_mo->isRemoved())   SetBelowMo(NULL);
}

//
// Finally destroy the map object.
//
static void DeleteMobj(mobj_t * mo)
{
#if (DEBUG_MOBJ > 0)
	L_WriteDebug("tics=%05d  DELETE %p [%s]\n", leveltime, mo, 
		mo->info ? mo->info->ddf.name.GetString() : "???");
#endif

	// Sound might still be playing, so use remove the
    // link between object and effect

    S_StopFX(mo);

	if (mo->refcount != 0)
	{
		I_Error("INTERNAL ERROR: Reference count %d", mo->refcount);
		return;
	}

	Z_Free(mo);
}

// ======================== END OF INTERNALS ======================== 

// Use these methods to set mobj entries.
// NEVER EVER modify the entries directly.

#define FUNCTION_BODY(field) \
{ \
	if (field) field->refcount--; \
	field = ref; \
	if (field) field->refcount++; \
}

void mobj_t::SetTarget(mobj_t *ref)  FUNCTION_BODY(target)
void mobj_t::SetSource(mobj_t *ref)  FUNCTION_BODY(source)
void mobj_t::SetTracer(mobj_t *ref)  FUNCTION_BODY(tracer)

void mobj_t::SetSupportObj(mobj_t *ref)  FUNCTION_BODY(supportobj)
void mobj_t::SetAboveMo(mobj_t *ref)     FUNCTION_BODY(above_mo)
void mobj_t::SetBelowMo(mobj_t *ref)     FUNCTION_BODY(below_mo)

#undef FUNCTION_BODY

//
// P_MobjSetRealSource
//
// -AJA- This is for missiles that spawn other missiles -- what we
//       really want to know is who spawned the original missile
//       (the "instigator" of all the mayhem :-).
//
void mobj_t::SetRealSource(mobj_t *ref)
{
	while (ref && ref->source && (ref->flags & MF_MISSILE))
		ref = ref->source;

	SetSource(ref);
}

//
// P_SetMobjState
//
// Returns true if the mobj is still present.
//
bool P_SetMobjState(mobj_t * mobj, statenum_t state)
{
	state_t *st;

	// ignore removed objects
	if (mobj->isRemoved())
		return false;

	if (state == S_NULL)
	{
		P_RemoveMobj(mobj);
		return false;
	}

	st = &states[state];

	mobj->state  = st;
	mobj->tics   = st->tics;
	mobj->sprite = st->sprite;
	mobj->frame  = st->frame;
	mobj->bright = st->bright;
	mobj->next_state = (st->nextstate == S_NULL) ? NULL :
		(states + st->nextstate);

	if (st->action)
		(* st->action)(mobj);

	return true;
}

//
// P_SetMobjStateDeferred
//
// Similiar to P_SetMobjState, but no actions are performed yet.
// The new state will entered when the P_MobjThinker code reaches it,
// which may happen in the current tick, or at worst the next tick.
//
// Prevents re-entrancy into code like P_CheckRelPosition which is
// inherently non re-entrant.
//
// -AJA- 1999/09/12: written.
//
bool P_SetMobjStateDeferred(mobj_t * mo, statenum_t stnum, int tic_skip)
{
	// ignore removed objects
	if (mo->isRemoved() || !mo->next_state)
		return false;

///???	if (stnum == S_NULL)
///???	{
///???		P_RemoveMobj(mo);
///???		return false;
///???	}

	mo->next_state = (stnum == S_NULL) ? NULL : (states + stnum);

	mo->tics = 0;
	mo->tic_skip = tic_skip;

	return true;
}

//
// P_MobjFindLabel
//
// Look for the given label in the mobj's states.  Returns the state
// number if found, otherwise S_NULL.
//
statenum_t P_MobjFindLabel(mobj_t * mobj, const char *label)
{
	int i;

	for (i=mobj->info->first_state; i <= mobj->info->last_state; i++)
	{
		if (! states[i].label)
			continue;

		if (DDF_CompareName(states[i].label, label) == 0)
			return i;
	}

	return S_NULL;
}

//
// P_SetMobjDirAndSpeed
//
void P_SetMobjDirAndSpeed(mobj_t * mo, angle_t angle, float slope, float speed)
{
	mo->angle = angle;
	mo->vertangle = M_ATan(slope);

	mo->mom.z = M_Sin(mo->vertangle) * speed;
	speed    *= M_Cos(mo->vertangle);

	mo->mom.x = M_Cos(angle) * speed;
	mo->mom.y = M_Sin(angle) * speed;
}

//
// P_MobjExplodeMissile  
//
// -AJA- 1999/09/12: Now uses P_SetMobjStateDeferred, since this
//       routine can be called by TryMove/PIT_CheckRelThing.
//
void P_MobjExplodeMissile(mobj_t * mo)
{
	mo->mom.x = mo->mom.y = mo->mom.z = 0;

	mo->flags &= ~(MF_MISSILE | MF_TOUCHY);
	mo->extendedflags &= ~(EF_BOUNCE | EF_USABLE);

	if (mo->info->deathsound)
		S_StartFX(mo->info->deathsound, SNCAT_Object, mo);

	// mobjdef used -ACB- 1998/08/06
	P_SetMobjStateDeferred(mo, mo->info->death_state, P_Random() & 3);
}


static INLINE void AddRegionProperties(const mobj_t *mo,
									   float bz, float tz, region_properties_t *new_p, 
									   float f_h, float c_h, const region_properties_t *p)
{
	int flags = p->special ? p->special->special_flags : SECSP_PushConstant;

	float factor = 1.0f;
	float push_mul;

	SYS_ASSERT(tz > bz);

	if (tz > c_h)
		factor -= factor * (tz - c_h) / (tz-bz);

	if (bz < f_h)
		factor -= factor * (f_h - bz) / (tz-bz);

	if (factor <= 0)
		return;

	new_p->gravity   += factor * p->gravity;
	new_p->viscosity += factor * p->viscosity;
	new_p->drag      += factor * p->drag;

	// handle push sectors

	if (! (flags & SECSP_WholeRegion) && bz > f_h + 1)
		return;

	push_mul = 1.0f;

	if (! (flags & SECSP_PushConstant))
	{
		SYS_ASSERT(mo->info->mass > 0);
		push_mul = 100.0f / mo->info->mass;
	}

	if (flags & SECSP_Proportional)
		push_mul *= factor;

	new_p->push.x += push_mul * p->push.x;
	new_p->push.y += push_mul * p->push.y;
	new_p->push.z += push_mul * p->push.z;
}

//
// P_CalcFullProperties
//
// Calculates the properties (gravity etc..) acting on an object,
// especially when the object is in multiple extrafloors with
// different props.
//
// Only used for players for now (too expensive to be used by
// everything).
//
void P_CalcFullProperties(const mobj_t *mo, region_properties_t *new_p)
{
	sector_t *sector = mo->subsector->sector;

	extrafloor_t *S, *L, *C;
	float floor_h;

	float bz = mo->z;
	float tz = bz + mo->height;


	new_p->gravity = 0;
	new_p->viscosity = 0;
	new_p->drag = 0;

	new_p->push.x = new_p->push.y = new_p->push.z = 0;

	new_p->type = 0;  // these shouldn't be used
	new_p->special = NULL;

	// Note: friction not averaged: comes from region foot is in
	new_p->friction = sector->p->friction;

	floor_h = sector->f_h;

	S = sector->bottom_ef;
	L = sector->bottom_liq;

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

		// ignore "hidden" liquids
		if (C->bottom_h < floor_h || C->bottom_h > sector->c_h)
			continue;

		if (bz < C->bottom_h)
			new_p->friction = C->p->friction;

		AddRegionProperties(mo, bz, tz, new_p, floor_h, C->top_h, C->p);

		floor_h = C->top_h;
	}

	AddRegionProperties(mo, bz, tz, new_p, floor_h, sector->c_h, sector->p);
}

//
// P_XYMovement  
//
static void P_XYMovement(mobj_t * mo, const region_properties_t *props)
{
	player_t *player;

	float ptryx;
	float ptryy;
	float xmove;
	float ymove;
	float xstep;
	float ystep;
	float absx,absy;
	float maxstep;

	int did_move;

	player = mo->player;

	if (mo->mom.x > MAXMOVE)
		mo->mom.x = MAXMOVE;
	else if (mo->mom.x < -MAXMOVE)
		mo->mom.x = -MAXMOVE;

	if (mo->mom.y > MAXMOVE)
		mo->mom.y = MAXMOVE;
	else if (mo->mom.y < -MAXMOVE)
		mo->mom.y = -MAXMOVE;

	xmove = mo->mom.x;
	ymove = mo->mom.y;

	// -AJA- 1999/07/31: Ride that rawhide :->
#if 1
	if (mo->above_mo && !(mo->above_mo->flags & MF_FLOAT) &&
		mo->above_mo->floorz < (mo->z + mo->height + 1))
	{
		mo->above_mo->mom.x += xmove * mo->info->ride_friction;
		mo->above_mo->mom.y += ymove * mo->info->ride_friction;
	}
#else
	if (mo->ride_em && (mo->z == mo->floorz))
	{
		xmove += (mo->ride_em->x + mo->ride_dx - mo->x) *
			mo->ride_em->info->ride_friction;
		ymove += (mo->ride_em->y + mo->ride_dy - mo->y) *
			mo->ride_em->info->ride_friction;
	}
#endif

	// -AJA- 1999/10/09: Reworked viscosity.
	xmove *= 1.0f - props->viscosity;
	ymove *= 1.0f - props->viscosity;

	// -ES- 1999/10/16 For fast mobjs, break down
	//  the move into steps of max half radius for collision purposes.

	// Use half radius as max step, if not exceptionally small.
	if (mo->radius > STEPMOVE)
		maxstep = mo->radius / 2;
	else
		maxstep = STEPMOVE / 2;

	// precalculate these two, they are used frequently
	absx = (float)fabs(xmove);
	absy = (float)fabs(ymove);

	if (absx > maxstep || absy > maxstep)
	{
		// Do it in the most number of steps.
		if (absx > absy)
		{
			xstep = (xmove > 0) ? maxstep : -maxstep;

			// almost orthogonal movements are rounded to orthogonal, to prevent
			// an infinite loop in some extreme cases.
			if (absy * 256 < absx)
				ystep = ymove = 0;
			else
				ystep = ymove * xstep / xmove;
		}
		else
		{
			ystep = (ymove > 0) ? maxstep : -maxstep;

			if (absx * 256 < absy)
				xstep = xmove = 0;
			else
				xstep = xmove * ystep / ymove;
		}
	}
	else
	{
		// Step is less than half radius, so one iteration is enough.
		xstep = xmove;
		ystep = ymove;
	}

	// Keep attempting moves until object has lost all momentum.
	do
	{
		// if movement is more than half that of the maximum, attempt the move
		// in two halves or move.
		if (fabs(xmove) > fabs(xstep))
		{
			ptryx = mo->x + xstep;
			xmove -= xstep;
		}
		else
		{
			ptryx = mo->x + xmove;
			xmove = 0;
		}

		if (fabs(ymove) > fabs(ystep))
		{
			ptryy = mo->y + ystep;
			ymove -= ystep;
		}
		else
		{
			ptryy = mo->y + ymove;
			ymove = 0;
		}

		did_move = P_TryMove(mo, ptryx, ptryy);

		// unable to complete desired move ?
		if (!did_move)
		{ 
			// check for missiles hitting shootable lines
			// NOTE: this is for solid lines.  The "pass over" case is
			// handled in P_TryMove().

			if ((mo->flags & MF_MISSILE) && 
				(! mo->currentattack ||
				! (mo->currentattack->flags & AF_NoTriggerLines)))
			{
				//
				// -AJA- Seems this is called to handle this situation: 
				// P_TryMove is called, but fails because missile would hit 
				// solid line.  BUT missile did pass over some special lines.  
				// These special lines were not activated in P_TryMove since it 
				// failed.  Ugh !
				//
				if (spechit.GetSize() > 0)
				{
					epi::array_iterator_c it;
					line_t* ld;
					
					for (it=spechit.GetTailIterator(); it.IsValid(); it--)
					{
						ld = ITERATOR_TO_TYPE(it, line_t*);
						
						P_ShootSpecialLine(ld, PointOnLineSide(mo->x, mo->y, ld), 
											mo->source);
					}	
				}
				
				if (blockline && blockline->special)
				{
					P_ShootSpecialLine(blockline, 
						PointOnLineSide(mo->x, mo->y, blockline), mo->source);
				}
			}

			if (mo->info->flags & MF_SLIDE)
			{
				P_SlideMove(mo, ptryx, ptryy);
			}
			else if (mo->extendedflags & EF_BOUNCE)
			{
				// -KM- 1999/01/31 Bouncy objects (grenades)
				// -AJA- 1999/07/30: Moved up here.

				if (! blockline)
				{
					if (mobj_hit_sky)
						P_MobjRemoveMissile(mo);
					else
						P_MobjExplodeMissile(mo);

					return;
				}

				BounceOffWall(mo, blockline);
				xmove = ymove = 0;
			}
			else if (mo->flags & MF_MISSILE)
			{
				if (mobj_hit_sky)
					P_MobjRemoveMissile(mo);  // New Procedure -ACB- 1998/07/30
				else
					P_MobjExplodeMissile(mo);

				return;
			}
			else
			{
				xmove = ymove = 0;
				mo->mom.x = mo->mom.y = 0;
			}
		}
	}
	while (xmove || ymove);

	if ((mo->extendedflags & EF_NOFRICTION) || (mo->flags & MF_SKULLFLY))
		return;

	if (mo->flags & MF_CORPSE)
	{
		// do not stop sliding if halfway off a step with some momentum
		if (CorpseShouldSlide(mo))
			return;
	}

	//
	// -MH- 1998/08/18 - make mid-air movement normal when using the jetpack
	//      When in mid-air there's no friction so you slide about
	//      uncontrollably. This is realistic but makes the game
	//      difficult to control to the extent that for normal people,
	//      it's not worth playing - a bit like having auto-aim
	//      permanently off (as most real people are not crack-shots!)
	//
	if ((mo->z > mo->floorz) &&
		!(level_flags.true3dgameplay && mo->player && 
		mo->player->powers[PW_Jetpack]) &&
		!(mo->on_ladder >= 0))
	{
		// apply drag when airborne
		mo->mom.x *= props->drag;
		mo->mom.y *= props->drag;
	}
	else
	{
		mo->mom.x *= props->friction;
		mo->mom.y *= props->friction;
	}

	if (mo->player)
	{
		if (fabs(mo->mom.x) < STOPSPEED && fabs(mo->mom.y) < STOPSPEED &&
			mo->player->cmd.forwardmove == 0 && 
			mo->player->cmd.sidemove == 0)
		{
			mo->mom.x = mo->mom.y = 0;
		}
	}
}

//
// P_ZMovement
//
static void P_ZMovement(mobj_t * mo, const region_properties_t *props)
{
	float dist;
	float delta;
	float zmove;

	// -KM- 1998/11/25 Gravity is now not precalculated so that
	//  menu changes affect instantly.
	float gravity = props->gravity / 8.0f * 
		(float)level_flags.menu_grav / (float)MENU_GRAV_NORMAL;

	// check for smooth step up
	if (mo->player && mo->player->mo == mo && mo->z < mo->floorz)
	{
		mo->player->viewheight -= mo->floorz - mo->z;
		mo->player->deltaviewheight = (mo->player->std_viewheight - 
			mo->player->viewheight) / 8.0f;
	}

	zmove = mo->mom.z * (1.0f - props->viscosity);

	// adjust height
	mo->z += zmove;

	if (mo->flags & MF_FLOAT && mo->target)
	{
		// float down towards target if too close
		if (!(mo->flags & MF_SKULLFLY) && !(mo->flags & MF_INFLOAT))
		{
			dist = P_ApproxDistance(mo->x - mo->target->x, mo->y - mo->target->y);
			delta = mo->target->z + (mo->height / 2) - mo->z;

			if (delta < 0 && dist < -(delta * 3))
				mo->z -= mo->info->float_speed;
			else if (delta > 0 && dist < (delta * 3))
				mo->z += mo->info->float_speed;
		}
	}

	//
	//  HIT FLOOR ?
	//

	if (mo->z <= mo->floorz)
	{
		if (mo->flags & MF_SKULLFLY)
			mo->mom.z = -mo->mom.z;

		if (mo->mom.z < 0)
		{
			float hurt_momz = gravity * mo->info->maxfall;
			bool fly_or_swim = mo->player && (mo->player->swimming ||
				mo->player->powers[PW_Jetpack] || mo->on_ladder >= 0);

			if (mo->player && gravity > 0 && -zmove > OOF_SPEED && ! fly_or_swim)
			{
				// Squat down. Decrease viewheight for a moment after hitting the
				// ground (hard), and utter appropriate sound.
				mo->player->deltaviewheight = zmove / 8.0f;
				S_StartFX(mo->info->oof_sound, P_MobjGetSfxCategory(mo), mo);
			}
			// -KM- 1998/12/16 If bigger than max fall, take damage.
			if (mo->info->maxfall && gravity > 0 && -mo->mom.z > hurt_momz &&
				(! mo->player || ! fly_or_swim))
			{
				P_DamageMobj(mo, NULL, NULL, (-mo->mom.z - hurt_momz), NULL);
			}

			// -KM- 1999/01/31 Bouncy bouncy...
			if (mo->extendedflags & EF_BOUNCE)
			{
				BounceOffPlane(mo, +1.0f);

				// don't bounce forever on the floor
				if (! (mo->flags & MF_NOGRAVITY) &&
					fabs(mo->mom.z) < STOPSPEED + fabs(gravity))
				{
					mo->mom.x = mo->mom.y = mo->mom.z = 0;
				}
			}
			else
				mo->mom.z = 0;
		}

		mo->z = mo->floorz;

		if ((mo->flags & MF_MISSILE) && !(mo->flags & MF_NOCLIP))
		{
			// -AJA- 2003/10/09: handle missiles that hit a monster on
			//       the head from a sharp downward angle (such a case
			//       is missed by PIT_CheckRelThing).  FIXME: more kludge.

			if (mo->below_mo && (int)mo->floorz ==
				(int)(mo->below_mo->z + mo->below_mo->info->height) &&
				(mo->below_mo->flags & MF_SHOOTABLE) &&
				(mo->source != mo->below_mo))
			{
				if (P_MissileContact(mo, mo->below_mo) < 0 ||
					(mo->extendedflags & EF_TUNNEL))
					return;
			}

			// if the floor is sky, don't explode missile -ACB- 1998/07/31
			if (IS_SKY(mo->subsector->sector->floor) &&
				mo->subsector->sector->f_h >= mo->floorz)
			{
				P_MobjRemoveMissile(mo);
			}
			else
			{
				if (! (mo->extendedflags & EF_BOUNCE))
					P_MobjExplodeMissile(mo);
			}
			return;
		}
	}
	else if (gravity > 0.0f)
	{
		// thing is above the ground, therefore apply gravity

		// -MH- 1998/08/18 - Disable gravity while player has jetpack
		//                   (nearly forgot this one:-)

		if (!(mo->flags & MF_NOGRAVITY) &&
			!(level_flags.true3dgameplay && mo->player && 
			mo->player->powers[PW_Jetpack]) &&
			!(mo->on_ladder >= 0))
		{
			mo->mom.z -= gravity;
		}
	}

	//
	//  HIT CEILING ?
	//

	if (mo->z + mo->height > mo->ceilingz)
	{
		if (mo->flags & MF_SKULLFLY)
			mo->mom.z = -mo->mom.z;  // the skull slammed into something

		// hit the ceiling
		if (mo->mom.z > 0)
		{
			float hurt_momz = gravity * mo->info->maxfall;
			bool fly_or_swim = mo->player && (mo->player->swimming ||
				mo->player->powers[PW_Jetpack] || mo->on_ladder >= 0);

			if (mo->player && gravity < 0 && zmove > OOF_SPEED && ! fly_or_swim)
			{
				mo->player->deltaviewheight = zmove / 8.0f;
				S_StartFX(mo->info->oof_sound, P_MobjGetSfxCategory(mo), mo);
			}
			if (mo->info->maxfall && gravity < 0 && mo->mom.z > hurt_momz &&
				(! mo->player || ! fly_or_swim))
			{
				P_DamageMobj(mo, NULL, NULL, (mo->mom.z - hurt_momz), NULL);
			}

			// -KM- 1999/01/31 More bouncing.
			if (mo->extendedflags & EF_BOUNCE)
			{
				BounceOffPlane(mo, -1.0f);

				// don't bounce forever on the ceiling
				if (! (mo->flags & MF_NOGRAVITY) &&
					fabs(mo->mom.z) < STOPSPEED + fabs(gravity))
				{
					mo->mom.x = mo->mom.y = mo->mom.z = 0;
				}
			}
			else
				mo->mom.z = 0;
		}

		mo->z = mo->ceilingz - mo->height;

		if ((mo->flags & MF_MISSILE) && !(mo->flags & MF_NOCLIP))
		{
			if (mo->above_mo && (int)mo->ceilingz == (int)(mo->above_mo->z) &&
				(mo->above_mo->flags & MF_SHOOTABLE) &&
				(mo->source != mo->above_mo))
			{
				if (P_MissileContact(mo, mo->above_mo) < 0 ||
					(mo->extendedflags & EF_TUNNEL))
					return;
			}

			// if the ceiling is sky, don't explode missile -ACB- 1998/07/31
			if (IS_SKY(mo->subsector->sector->ceil) &&
				mo->subsector->sector->c_h <= mo->ceilingz)
			{
				P_MobjRemoveMissile(mo);
			}
			else
			{
				if (! (mo->extendedflags & EF_BOUNCE))
					P_MobjExplodeMissile(mo);
			}
			return;
		}
	}
	else if (gravity < 0.0f)
	{
		// thing is below ceiling, therefore apply any negative gravity

		// -MH- 1998/08/18 - Disable gravity while player has jetpack
		//                   (nearly forgot this one:-)

		if (!(mo->flags & MF_NOGRAVITY) &&
			!(level_flags.true3dgameplay && mo->player && 
			mo->player->powers[PW_Jetpack]) &&
			!(mo->on_ladder >= 0))
		{
			mo->mom.z += -gravity;
		}
	}

	// update the object's vertical region
	P_TryMove(mo, mo->x, mo->y);

	// ladders have friction
	if (mo->on_ladder >= 0)
		mo->mom.z *= LADDER_FRICTION;

	// apply drag -- but not to frictionless things
	if ((mo->extendedflags & EF_NOFRICTION) || (mo->flags & MF_SKULLFLY))
		return;

	mo->mom.z *= props->drag;

	if (mo->player)
	{
		if (fabs(mo->mom.z) < STOPSPEED &&
			mo->player->cmd.upwardmove == 0)
		{
			mo->mom.z = 0;
		}
	}
}

//
// P_MobjThinker
//
#define MAX_THINK_LOOP  8

static void P_MobjThinker(mobj_t * mobj)
{
	const region_properties_t *props;
	region_properties_t player_props;

	SYS_ASSERT_MSG(mobj->next != (mobj_t *)-1,
		("P_MobjThinker INTERNAL ERROR: mobj has been Z_Freed"));

	SYS_ASSERT(mobj->state);
	SYS_ASSERT(mobj->refcount >= 0);

	mobj->ClearStaleRefs();

	mobj->visibility = (15 * mobj->visibility + mobj->vis_target) / 16;

	for (int DL=0; DL < 2; DL++)
		mobj->dlight[DL].r = (15 * mobj->dlight[DL].r + mobj->dlight[DL].target) / 16;

	// handle SKULLFLY attacks
	if ((mobj->flags & MF_SKULLFLY) && mobj->mom.x == 0 && mobj->mom.y == 0)
	{
		// the skull slammed into something
		mobj->flags &= ~MF_SKULLFLY;
		mobj->mom.x = mobj->mom.y = mobj->mom.z = 0;

		P_SetMobjState(mobj, mobj->info->idle_state);

		if (mobj->isRemoved()) return;
	}

	// determine properties, & handle push sectors

	SYS_ASSERT(mobj->props);

	if (mobj->player)
	{
		P_CalcFullProperties(mobj, &player_props);

		mobj->mom.x += player_props.push.x;
		mobj->mom.y += player_props.push.y;
		mobj->mom.z += player_props.push.z;

		props = &player_props;
	}
	else
	{
		props = mobj->props;

		if (props->push.x || props->push.y || props->push.z)
		{
			sector_flag_e flags = props->special ?
				props->special->special_flags : SECSP_PushConstant;

			if (!((mobj->flags & MF_NOGRAVITY) || (flags & SECSP_PushAll))  &&
				(mobj->z <= mobj->floorz + 1.0f || (flags & SECSP_WholeRegion)))
			{
				float push_mul = 1.0f;

				SYS_ASSERT(mobj->info->mass > 0);
				if (! (flags & SECSP_PushConstant))
					push_mul = 100.0f / mobj->info->mass;

				mobj->mom.x += push_mul * props->push.x;
				mobj->mom.y += push_mul * props->push.y;
				mobj->mom.z += push_mul * props->push.z;
			}
		}
	}

	// momentum movement
	if (mobj->mom.x != 0 || mobj->mom.y != 0) //  || mobj->ride_em)
	{
		P_XYMovement(mobj, props);

		if (mobj->isRemoved()) return;
	}

	if ((mobj->z != mobj->floorz) || mobj->mom.z != 0) //  || mobj->ride_em)
	{
		P_ZMovement(mobj, props);

		if (mobj->isRemoved()) return;
	}

	if (mobj->fuse >= 0)
	{
		if (!--mobj->fuse)
			P_MobjExplodeMissile(mobj);

		if (mobj->isRemoved()) return;
	}

	if (mobj->tics < 0)
	{
		// check for nightmare respawn
		if (!(mobj->extendedflags & EF_MONSTER))
			return;

		// replaced respawnmonsters & newnmrespawn with respawnsetting
		// -ACB- 1998/07/30
		if (!level_flags.respawn)
			return;

		mobj->movecount++;

		//
		// Uses movecount as a timer, when movecount hits 12*TICRATE the
		// object will try to respawn. So after 12 seconds the object will
		// try to respawn.
		//
		if (mobj->movecount < mobj->info->respawntime)
			return;

		// if the first 5 bits of leveltime are on, don't respawn now...ok?
		if (leveltime & 31)
			return;

		// give a limited "random" chance that respawn don't respawn now
		if (P_Random() > 32)
			return;

		// replaced respawnmonsters & newnmrespawn with respawnsetting
		// -ACB- 1998/07/30
		if (level_flags.res_respawn)
			ResurrectRespawn(mobj);
		else
			TeleportRespawn(mobj);

		return;
	}

	// Cycle through states, calling action functions at transitions.
	// -AJA- 1999/09/12: reworked for deferred states.
	// -AJA- 2000/10/17: reworked again.

	for (int loop_count=0; loop_count < MAX_THINK_LOOP; loop_count++)
	{
		mobj->tics -= (1 + mobj->tic_skip);
		mobj->tic_skip = 0;

		if (mobj->tics >= 1)
			break;

		// You can cycle through multiple states in a tic.
		// NOTE: returns false if object freed itself.

		P_SetMobjState(mobj, mobj->next_state ?
			(mobj->next_state - states) : S_NULL);

		if (mobj->isRemoved())
			return;

		if (mobj->tics != 0)
			break;
	}
}

//
// P_RunMobjThinkers
//
// Cycle through all mobjs and let them think.
//
void P_RunMobjThinkers(void)
{
	mobj_t *mo;
	mobj_t *next;

	for (mo = mobjlisthead; mo; mo = next)
	{
		next = mo->next;

		P_MobjThinker(mo);
	}

	P_RemoveQueuedMobjs(false);
}

//
// P_RemoveQueuedMobjs
//
// Removes all the mobjs in the remove_queue list.
//
// -ES- 1999/10/24 Written.
//
void P_RemoveQueuedMobjs(bool force_all)
{
	std::list<mobj_t *>::iterator M;

	bool did_remove = false;

	for (M = remove_queue.begin(); M != remove_queue.end(); M++)
	{
		mobj_t *mo = *M;

		mo->fuse--;

		if (!force_all && mo->fuse == 1 && mo->refcount != 0)
			I_Warning("Bad ref count for %s = %d.\n", 
						mo->info->ddf.name.GetString(), mo->refcount);

		if (force_all || (mo->fuse <= 0 && mo->refcount == 0))
		{
			DeleteMobj(mo);

			// cannot mess with the list while traversing it,
			// hence set the current entry to NULL and remove
			// those nodes afterwards.
			*M = NULL; did_remove = true;
		}
	}

	if (did_remove)
		remove_queue.remove(NULL);
}

static void AddMobjToList(mobj_t *mo)
{
	mo->prev = NULL;
	mo->next = mobjlisthead;

	if (mo->next != NULL)
	{
		SYS_ASSERT(mo->next->prev == NULL);
		mo->next->prev = mo;
	}

	mobjlisthead = mo;

#if (DEBUG_MOBJ > 0)
	L_WriteDebug("tics=%05d  ADD %p [%s]\n", leveltime, mo, 
		mo->info ? mo->info->ddf.name.GetString() : "???");
#endif
}

static void RemoveMobjFromList(mobj_t *mo)
{
	if (mo->prev != NULL)
	{
		SYS_ASSERT(mo->prev->next == mo);
		mo->prev->next = mo->next;
	}
	else // no previous, must be first item
	{
		SYS_ASSERT(mobjlisthead == mo);
		mobjlisthead = mo->next;
	}

	if (mo->next != NULL)
	{
		SYS_ASSERT(mo->next->prev == mo);
		mo->next->prev = mo->prev;
	}

	mo->next = (mobj_t *) -1;
	mo->prev = (mobj_t *) -1;
}

//
// P_RemoveMobj
//
// Removes the object from the play simulation: no longer thinks, if
// the mobj is MF_SPECIAL: i.e. item can be picked up, it is added to
// the item-respawn-que, so it gets respawned if needed; The respawning
// only happens if itemrespawn is set or the deathmatch mode is
// version 2.0: altdeath.
//
void P_RemoveMobj(mobj_t *mo)
{
#if (DEBUG_MOBJ > 0)
	L_WriteDebug("tics=%05d  REMOVE %p [%s]\n", leveltime, mo, 
		mo->info ? mo->info->ddf.name.GetString() : "???");
#endif

	if (mo->isRemoved())
	{
		L_WriteDebug("Warning: Object %p (%s) REMOVED TWICE\n",
		     mo, mo->info ? mo->info->ddf.name.GetString() : "???");
		return;
	}

	if (! (mo->flags & MF_MISSILE) &&
		(deathmatch >= 2 || level_flags.itemrespawn) &&
		(mo->info->flags & MF_SPECIAL) && 
		!(mo->extendedflags & EF_NORESPAWN) &&
		!(mo->flags & MF_DROPPED))
	{
		iteminque_t *newbie = Z_New(iteminque_t, 1);

		newbie->spawnpoint = mo->spawnpoint;
		newbie->time = mo->info->respawntime;

		if (itemquehead == NULL)
		{
			newbie->next = newbie->prev = NULL;
			itemquehead = newbie;
		}
		else
		{
			iteminque_t *tail;

			for (tail = itemquehead; tail->next; tail = tail->next)
			{ /* nothing */ }

			newbie->next = NULL;
			newbie->prev = tail;

			tail->next = newbie;
		}
	}

	// mark as REMOVED
	mo->state = NULL;
	mo->next_state = NULL;

	// Clear all references to other mobjs
	mo->SetTarget(NULL);
	mo->SetSource(NULL);
	mo->SetTracer(NULL);

	mo->SetSupportObj(NULL);
	mo->SetAboveMo(NULL);
	mo->SetBelowMo(NULL);

	// unlink from sector and block lists
	P_UnsetThingFinally(mo);

	// remove from global list
	RemoveMobjFromList(mo);

	// set timer to keep in the remove queue ("IN LIMBO")
	mo->fuse = TICRATE * 3;

	remove_queue.push_back(mo);
}

void P_RemoveAllMobjs(void)
{
	while (mobjlisthead)
	{
		mobj_t *mo = mobjlisthead;

		P_UnsetThingFinally(mo);

		RemoveMobjFromList(mo);

		mo->refcount = 0;
		DeleteMobj(mo);
	}
}

//
// P_RemoveItemsInQue
//
void P_RemoveItemsInQue(void)
{
	while (itemquehead)
	{
		iteminque_t *tmp = itemquehead;
		itemquehead = itemquehead->next;
		Z_Free(tmp);
	}
}

//
// GAME SPAWN FUNCTIONS
//

//
// P_SpawnPuff
//
void P_SpawnPuff(float x, float y, float z, const mobjtype_c * puff)
{
	mobj_t *th;

	z += (float) P_RandomNegPos() / 80.0f;

	// -ACB- 1998/08/06 Specials table for non-negotiables....
	th = P_MobjCreateObject(x, y, z, puff);

	// -AJA- 1999/07/14: DDF-itised.
	th->mom.z = puff->float_speed;

	th->tics -= P_Random() & 3;

	if (th->tics < 1)
		th->tics = 1;
}

//
// P_SpawnBlood
//
// -KM- 1998/11/25 Made more violent. :-)
// -KM- 1999/01/31 Different blood objects for different mobjs.
//
void P_SpawnBlood(float x, float y, float z, float damage,
				  angle_t angle, const mobjtype_c * blood)
{
	int num;
	mobj_t *th;

	angle += ANG180;

	num = (int) (!level_flags.more_blood ? 1.0f : (M_Random() % 7) + 
		(float)((MAX(damage / 4.0f, 7.0f))));

	while (num--)
	{
		z += (float)(P_RandomNegPos() / 64.0f);

		angle += (angle_t) (P_RandomNegPos() * (int)(ANG1 / 2));

		th = P_MobjCreateObject(x, y, z, blood);

		P_SetMobjDirAndSpeed(th, angle, ((float)num + 12.0f) / 6.0f, 
			(float)num / 4.0f);

		th->tics -= P_Random() & 3;

		if (th->tics < 1)
			th->tics = 1;

		if (damage <= 12 && th->state && th->next_state)
			P_SetMobjState(th, th->next_state - states);

		if (damage <= 8 && th->state && th->next_state)
			P_SetMobjState(th, th->next_state - states);
	}
}

//
// P_MobjItemRespawn
//
// Replacement procedure for P_RespawnSpecials, uses a linked list to go through
// the item-respawn-que. The time until respawn (in tics) is decremented every tic,
// when the item-in-the-que has a time of zero is it respawned.
//
// -ACB- 1998/07/30 Procedure written.
// -KM- 1999/01/31 Custom respawn fog.
//
void P_MobjItemRespawn(void)
{
	float x, y, z;
	mobj_t *mo;
	const mobjtype_c *objtype;

	iteminque_t *cur, *next;

	// only respawn items in deathmatch or if itemrespawn
	if (! (deathmatch >= 2 || level_flags.itemrespawn))
		return;

	// No item-respawn-que exists, so nothing to process.
	if (itemquehead == NULL)
		return;

	// lets start from the beginning....
	for (cur = itemquehead; cur; cur = next)
	{
		next = cur->next;

		cur->time--;

		if (cur->time > 0)
			continue;

		// no time left, so respawn object

		x = cur->spawnpoint.x;
		y = cur->spawnpoint.y;
		z = cur->spawnpoint.z;

		objtype = cur->spawnpoint.info;

		if (objtype == NULL)
		{
			I_Error("P_MobjItemRespawn: No such item type!");
			return;  // shouldn't happen.
		}

		// spawn a teleport fog at the new spot
		SYS_ASSERT(objtype->respawneffect);
		P_MobjCreateObject(x, y, z, objtype->respawneffect);

		// -ACB- 1998/08/06 Use MobjCreateObject
		mo = P_MobjCreateObject(x, y, z, objtype);

		mo->angle = cur->spawnpoint.angle;
		mo->vertangle = cur->spawnpoint.vertangle;
		mo->spawnpoint = cur->spawnpoint;

		// Taking this item-in-que out of the que, remove
		// any references by the previous and next items to
		// the current one.....

		if (cur->next)
			cur->next->prev = cur->prev;

		if (cur->prev)
			cur->prev->next = next;
		else
			itemquehead = next;

		Z_Free(cur);
	}
}

//
// P_MobjRemoveMissile
//
// This procedure only is used when a flying missile is removed because
// it "hit" a wall or ceiling that in the simulation acts as a sky. The
// only major differences with P_RemoveMobj are that now item respawn check
// is not done (not needed) and any sound will continue playing despite
// the fact the missile has been removed: This is only done due to the
// fact that a missile in reality would continue flying through a sky and
// you should still be able to hear it.
//
// -ACB- 1998/07/31 Procedure written.
// -AJA- 1999/09/15: Functionality subsumed by DoRemoveMobj.
// -ES- 1999/10/24 Removal Queue.
//
void P_MobjRemoveMissile(mobj_t * missile)
{
	P_RemoveMobj(missile);

	missile->mom.x = missile->mom.y = missile->mom.z = 0;

	missile->flags &= ~(MF_MISSILE | MF_TOUCHY);
	missile->extendedflags &= ~(EF_BOUNCE);
}

//
// P_MobjCreateObject
//
// Creates a Map Object (MOBJ) at the specified location, with the
// specified type (given by DDF).  The special z values ONFLOORZ and
// ONCEILINGZ are recognised and handled appropriately.
//
// -ACB- 1998/08/02 Procedure written.
//
mobj_t *P_MobjCreateObject(float x, float y, float z, const mobjtype_c *type)
{
	mobj_t *mobj;
	state_t *st;
	sector_t *sec;

	mobj = Z_ClearNew(mobj_t, 1);

#if (DEBUG_MOBJ > 0)
	L_WriteDebug("tics=%05d  CREATE %p [%s]  AT %1.0f,%1.0f,%1.0f\n", 
		leveltime, mobj, type->ddf.name.GetString(), x, y, z);
#endif

	mobj->info = type;
	mobj->x = x;
	mobj->y = y;
	mobj->radius = type->radius;
	mobj->height = type->height;
	mobj->flags = type->flags;
	mobj->health = type->spawnhealth;
	mobj->speed = type->speed;
	mobj->fuse = type->fuse;
	mobj->side = type->side;
	mobj->on_ladder = -1;

	if (level_flags.fastparm)
		mobj->speed *= type->fast;

	// -ACB- 1998/06/25 new mobj Stuff (1998/07/11 - invisibility added)
	mobj->extendedflags = type->extendedflags;
	mobj->hyperflags = type->hyperflags;
	mobj->vis_target = mobj->visibility = PERCENT_2_FLOAT(type->translucency);
	mobj->currentattack = NULL;

	if (gameskill != sk_nightmare)
		mobj->reactiontime = type->reactiontime;

	mobj->lastlook = P_Random() % MAXPLAYERS;

	//
	// Do not set the state with P_SetMobjState,
	// because action routines can not be called yet
	//
	// if we have a spawnstate use that; else try the meanderstate
	// -ACB- 1998/09/06
	//
	// -AJA- So that the first action gets executed, the `next_state'
	//       is set to the first state and `tics' set to 0.
	//
	if (type->spawn_state)
		st = &states[type->spawn_state];
	else if (type->meander_state)
		st = &states[type->meander_state];
	else
		st = &states[type->idle_state];

	mobj->state  = st;
	mobj->tics   = 0;
	mobj->sprite = st->sprite;
	mobj->frame  = st->frame;
	mobj->bright = st->bright;
	mobj->next_state = st;

	SYS_ASSERT(! mobj->isRemoved());

	// enable usable items
	if (mobj->extendedflags & EF_USABLE)
		mobj->flags |= MF_TOUCHY;

	// handle dynamic lights
	for (int DL=0; DL < 2; DL++)
	{
		const dlight_info_c *info = (DL == 0) ? &type->dlight0 : &type->dlight1;

		if (info->type != DLITE_None)
		{
			mobj->dlight[DL].r = mobj->dlight[DL].target = info->radius;
			mobj->dlight[DL].image = W_ImageLookup(info->shape, INS_Graphic, ILF_Null);
		}
	}

	// set subsector and/or block links
	P_SetThingPosition(mobj);

	// -AJA- 1999/07/30: Updated for extra floors.

	sec = mobj->subsector->sector;

	mobj->z = P_ComputeThingGap(mobj, sec, z, &mobj->floorz, &mobj->ceilingz);

	// Find the real players height (TELEPORT WEAPONS).
	mobj->origheight = z;

	// update totals for countable items.  Doing it here means that
	// things spawned dynamically can be counted as well.  Whilst this
	// has its dangers, at least it is consistent (more than can be said
	// when RTS comes into play -- trying to second guess which
	// spawnthings should not be counted just doesn't work).

	if (mobj->flags & MF_COUNTKILL)
		totalkills++;

	if (mobj->flags & MF_COUNTITEM)
		totalitems++;

	//
	// -ACB- 1998/08/27 Mobj Linked-List Addition
	//
	// A useful way of cycling through the current things without
	// having to deref everything using thinkers.
	//
	// -AJA- 1999/09/15: now adds to _head_ of list (for speed).
	//
	AddMobjToList(mobj);

	return mobj;
}

//
// P_MobjGetSfxCategory
//
// Returns the sound category for an object.
//
int P_MobjGetSfxCategory(const mobj_t *mo)
{
    if (mo->player)
    {
        if (mo->player == players[displayplayer])
            return SNCAT_Player;
        
		return SNCAT_Opponent;
    }
    else
    {
        if (mo->extendedflags & EF_MONSTER) 
            return SNCAT_Monster;

		return SNCAT_Object;
    }
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
