//----------------------------------------------------------------------------
//  EDGE Creature Action Code
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
//
// -KM- 1998/09/27 Sounds.ddf stuff
//
// -AJA- 1999/07/21: Replaced some non-critical P_Randoms with M_Random,
//       and removed some X_Random()-X_Random() things.
//

#include "i_defs.h"

#include "dm_data.h"
#include "dm_state.h"
#include "g_game.h"
#include "m_random.h"
#include "p_action.h"
#include "p_local.h"
#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"

#include <float.h>

dirtype_e opposite[] =
{
	DI_WEST,
	DI_SOUTHWEST,
	DI_SOUTH,
	DI_SOUTHEAST,
	DI_EAST,
	DI_NORTHEAST,
	DI_NORTH,
	DI_NORTHWEST,
	DI_NODIR
};

dirtype_e diags[] =
{
	DI_NORTHWEST,
	DI_NORTHEAST,
	DI_SOUTHWEST,
	DI_SOUTHEAST
};

// sqrt(2) / 2: The diagonal speed of creatures
#define SQ2 0.7071067812f

float xspeed[8] = {1.0f, SQ2, 0.0f, -SQ2, -1.0f, -SQ2, 0.0f, SQ2};
float yspeed[8] = {0.0f, SQ2, 1.0f, SQ2, 0.0f, -SQ2, -1.0f, -SQ2};

#undef SQ2

//
//  ENEMY THINKING
//
// Enemies are allways spawned
// with targetplayer = -1, threshold = 0
// Most monsters are spawned unaware of all players,
// but some can be made preaware
//

//
// Called by P_NoiseAlert.
// Recursively traverse adjacent sectors,
// sound blocking lines cut off traversal.
//
static void RecursiveSound(sector_t * sec, int soundblocks, int player)
{
	int i;
	line_t *check;
	sector_t *other;

	// has the sound flooded this sector
	if (sec->validcount == validcount && sec->soundtraversed <= soundblocks + 1)
		return;

	// wake up all monsters in this sector
	sec->validcount = validcount;
	sec->soundtraversed = soundblocks + 1;
	sec->sound_player = player;

	for (i = 0; i < sec->linecount; i++)
	{
		check = sec->lines[i];

		if (!(check->flags & MLF_TwoSided))
			continue;

		// -AJA- 1999/07/19: Gaps are now stored in line_t.
		if (check->gap_num == 0)
			continue;  // closed door

		// -AJA- 2001/11/11: handle closed Sliding doors
		if (check->slide_door && ! check->slide_door->s.see_through &&
			! check->slider_move)
		{
			continue;
		}

		if (check->frontsector == sec)
			other = check->backsector;
		else
			other = check->frontsector;

		if (check->flags & MLF_SoundBlock)
		{
			if (!soundblocks)
				RecursiveSound(other, 1, player);
		}
		else
		{
			RecursiveSound(other, soundblocks, player);
		}
	}
}

void P_NoiseAlert(player_t *p)
{
	validcount++;

	RecursiveSound(p->mo->subsector->sector, 0, p->pnum);
}

//
// Move in the current direction,
// returns false if the move is blocked.
//
bool P_Move(mobj_t * mo, bool path)
{
	vec3_t orig_pos;
	
	orig_pos.Set(mo->x, mo->y, mo->z);

	float tryx;
	float tryy;

	if (path)
	{
		tryx = mo->x + mo->speed * M_Cos(mo->angle);
		tryy = mo->y + mo->speed * M_Sin(mo->angle);
	}
	else
	{
		if (mo->movedir == DI_NODIR)
			return false;

		if ((unsigned)mo->movedir >= 8)
			I_Error("Weird mo->movedir!");

		tryx = mo->x + mo->speed * xspeed[mo->movedir];
		tryy = mo->y + mo->speed * yspeed[mo->movedir];
	}

	if (! P_TryMove(mo, tryx, tryy))
	{
		epi::array_iterator_c it;
		line_t* ld;
		
		// open any specials
		if (mo->flags & MF_FLOAT && floatok)
		{
			// must adjust height
			if (mo->z < float_destz)
				mo->z += mo->info->float_speed;
			else
				mo->z -= mo->info->float_speed;

			mo->flags |= MF_INFLOAT;
			// FIXME: position interpolation
			return true;
		}

		if (spechit.GetSize() == 0)
			return false;

		mo->movedir = DI_NODIR;

		// -AJA- 1999/09/10: As Lee Killough points out, this is where
		//       monsters can get stuck in doortracks.  We follow Lee's
		//       method: return true 90% of the time if the blocking line
		//       was the one activated, or false 90% of the time if there
		//       was some other line activated.

		bool any_used = false;
		bool block_used = false;
					
		for (it=spechit.GetTailIterator(); it.IsValid(); it--)
		{
			ld = ITERATOR_TO_TYPE(it, line_t*);
			if (P_UseSpecialLine(mo, ld, 0, -FLT_MAX, FLT_MAX))
			{
				any_used = true;

				if (ld == blockline)
					block_used = true;
			}
		}
		
		return any_used && (P_Random() < 230 ? block_used : !block_used);
	}

	mo->flags &= ~MF_INFLOAT;

	if (!(mo->flags & MF_FLOAT) && !(mo->extendedflags & EF_GRAVFALL))
		mo->z = mo->floorz;

	// -AJA- 2008/01/16: position interpolation
	const state_t *st = &mo->info->states[mo->state];

	if ((st->flags & SFF_Model) || (mo->flags & MF_FLOAT))
	{
		mo->lerp_num = CLAMP(2, st->tics, 10);
		mo->lerp_pos = 1;

		mo->lerp_from = orig_pos;
	}

	return true;
}

//
// Attempts to move actor on
// in its current (ob->moveangle) direction.
// If blocked by either a wall or an actor
// returns FALSE
// If move is either clear or blocked only by a door,
// returns TRUE and sets...
// If a door is in the way,
// an OpenDoor call is made to start it opening.
//
static bool TryWalk(mobj_t * mo)
{
	if (!P_Move(mo, false))
		return false;

	mo->movecount = P_Random() & 15;
	return true;
}

// -ACB- 1998/09/06 actor is now an object; different movement choices.
void P_NewChaseDir(mobj_t * mo)
{
	float deltax;
	float deltay;
	dirtype_e tdir;

	dirtype_e d[3];
	dirtype_e olddir;
	dirtype_e turnaround;

	olddir = mo->movedir;
	turnaround = opposite[olddir];

	//
	// Movement choice: Previously this was calculation to find
	// the distance between object and target: if the object had
	// no target, a fatal error was returned. However it is now
	// possible to have movement without a target. if the object
	// has a target, go for that; else if it has a supporting
	// object aim to go within supporting distance of that; the
	// remaining option is to walk aimlessly: the target destination
	// is always 128 in the old movement direction, think
	// of it like the donkey and the carrot sketch: the donkey will
	// move towards the carrot, but since the carrot is always a
	// set distance away from the donkey, the rather stupid mammal
	// will spend eternity trying to get the carrot and will walk
	// forever.
	//
	// -ACB- 1998/09/06

	if (mo->target)
	{
		deltax = mo->target->x - mo->x;
		deltay = mo->target->y - mo->y;
	}
	else if (mo->supportobj)
	{
		// not too close
		deltax = (mo->supportobj->x - mo->x) - (mo->supportobj->radius * 4);
		deltay = (mo->supportobj->y - mo->y) - (mo->supportobj->radius * 4);
	}
	else
	{
		deltax = 128 * xspeed[olddir];
		deltay = 128 * yspeed[olddir];
	}

	if (deltax > 10)
		d[1] = DI_EAST;
	else if (deltax < -10)
		d[1] = DI_WEST;
	else
		d[1] = DI_NODIR;

	if (deltay < -10)
		d[2] = DI_SOUTH;
	else if (deltay > 10)
		d[2] = DI_NORTH;
	else
		d[2] = DI_NODIR;

	// try direct route
	if (d[1] != DI_NODIR && d[2] != DI_NODIR)
	{
		mo->movedir = diags[((deltay < 0) << 1) + (deltax > 0)];
		if (mo->movedir != turnaround && TryWalk(mo))
			return;
	}

	// try other directions
	if (P_Random() > 200 || fabs(deltay) > fabs(deltax))
	{
		tdir = d[1];
		d[1] = d[2];
		d[2] = tdir;
	}

	if (d[1] == turnaround)
		d[1] = DI_NODIR;

	if (d[2] == turnaround)
		d[2] = DI_NODIR;

	if (d[1] != DI_NODIR)
	{
		mo->movedir = d[1];
		if (TryWalk(mo))
		{
			// either moved forward or attacked
			return;
		}
	}

	if (d[2] != DI_NODIR)
	{
		mo->movedir = d[2];

		if (TryWalk(mo))
			return;
	}

	// there is no direct path to the player,
	// so pick another direction.
	if (olddir != DI_NODIR)
	{
		mo->movedir = olddir;

		if (TryWalk(mo))
			return;
	}

	// randomly determine direction of search
	if (P_Random() & 1)
	{
		for (tdir = DI_EAST; tdir <= DI_SOUTHEAST; tdir = (dirtype_e)((int)tdir+1))
		{
			if (tdir != turnaround)
			{
				mo->movedir = tdir;

				if (TryWalk(mo))
					return;
			}
		}
	}
	else
	{
		for (tdir = DI_SOUTHEAST; tdir != (dirtype_e)(DI_EAST - 1); tdir = (dirtype_e)((int)tdir-1))
		{
			if (tdir != turnaround)
			{
				mo->movedir = tdir;

				if (TryWalk(mo))
					return;
			}
		}
	}

	if (turnaround != DI_NODIR)
	{
		mo->movedir = turnaround;
		if (TryWalk(mo))
			return;
	}

	// cannot move
	mo->movedir = DI_NODIR;
}

//
// Range is angle range on either side of eyes, 90 degrees for normal
// view, 180 degrees for total sight in all dirs.
//
// Returns true if a player is targeted.
//
bool P_LookForPlayers(mobj_t * mo, angle_t range)
{
	int c;
	int stop;
	player_t *player;
	angle_t an;
	float dist;

	c = 0;
	stop = (mo->lastlook - 1 + MAXPLAYERS) % MAXPLAYERS;

	for (; mo->lastlook != stop; mo->lastlook = (mo->lastlook + 1) % MAXPLAYERS)
	{
		player = players[mo->lastlook];

		if (!player)
			continue;

		SYS_ASSERT(player->mo);

		// done looking ?
		if (c++ >= 2)
			break;

		// dead ?
		if (player->health <= 0)
			continue;

		// on the same team ?
		if ((mo->side & player->mo->side) != 0)
			continue;

		if (range < ANG180)
		{
			an = R_PointToAngle(mo->x, mo->y, player->mo->x,
				player->mo->y) - mo->angle;

			if (range <= an && an <= (range * -1))
			{
				// behind back.
				// if real close, react anyway
				dist = P_ApproxDistance(player->mo->x - mo->x,
					player->mo->y - mo->y);

				if (dist > MELEERANGE)
					continue;
			}
		}

		// out of sight ?
		if (!P_CheckSight(mo, player->mo))
			continue;

		mo->SetTarget(player->mo);
		return true;
	}

	return false;
}

//
//   BOSS-BRAIN HANDLING
//

//
// -AJA- Savegames: we assume that the spit-spot objects never
//       disappear, or new ones appear.  After all, they have to be
//       there to be the target of the cubes.  This means we don't
//       need to save anything: the set of shoot-spots will be
//       regenerated after the loadgame when the BrainShooter next
//       tries to shoot a cube.

shoot_spot_info_t brain_spots = { -1, NULL };

void P_LookForShootSpots(const mobjtype_c *spot_type)
{
	int i;
	mobj_t *cur;

	brain_spots.number = 0;

	// count them
	for (cur=mobjlisthead; cur != NULL; cur=cur->next)
	{
		if (cur->info == spot_type)
			brain_spots.number++;
	}

	if (brain_spots.number == 0)
	{
		I_Warning("No [%s] objects found for BossBrain shooter.\n",
			spot_type->ddf.name.c_str());
		return;
	}

	// create the spots
	brain_spots.targets = new mobj_t* [brain_spots.number];

	for (cur=mobjlisthead, i=0; cur != NULL; cur=cur->next)
	{
		if (cur->info == spot_type)
			brain_spots.targets[i++] = cur;
	}

	SYS_ASSERT(i == brain_spots.number);
}

void P_FreeShootSpots(void)
{
	if (brain_spots.number < 0)
		return;

	if (brain_spots.targets > 0)
	{
		SYS_ASSERT(brain_spots.targets);

		delete[] brain_spots.targets;
	}

	brain_spots.number = -1;
	brain_spots.targets = NULL;
}

static void SpawnDeathMissile(mobj_t *source, float x, float y, float z)
{
	const mobjtype_c *info;
	mobj_t *th;

	info = mobjtypes.Lookup("BRAIN_DEATH_MISSILE");

	th = P_MobjCreateObject(x, y, z, info);
	if (th->info->seesound)
		S_StartFX(th->info->seesound, P_MobjGetSfxCategory(th), th);

	th->SetRealSource(source);

	th->mom.x = (x - source->x) / 50.0f;
	th->mom.y = -0.25f;
	th->mom.z = (z - source->z) / 50.0f;

	th->tics -= M_Random() & 7;

	if (th->tics < 1)
		th->tics = 1;
}

void A_BrainScream(mobj_t * bossbrain, void *data)
{
	// The brain and his pain...

	float x, y, z;
	float min_x, max_x;

	min_x = bossbrain->x - 280.0f;
	max_x = bossbrain->x + 280.0f;

	for (x = min_x; x < max_x; x += 4)
	{
		y = bossbrain->y - 320.0f;
		z = bossbrain->z + (P_Random() - 180.0f) * 2.0f;

		SpawnDeathMissile(bossbrain, x, y, z);
	}

	if (bossbrain->info->deathsound)
		S_StartFX(bossbrain->info->deathsound, P_MobjGetSfxCategory(bossbrain), bossbrain);
}

void A_BrainMissileExplode(mobj_t * mo, void *data)
{
	float x, y, z;

	if (! mo->source)
		return;

	x = mo->source->x + (P_Random() - 128.0f) * 4.0f;
	y = mo->source->y - 320.0f;
	z = mo->source->z + (P_Random() - 180.0f) * 2.0f;

	SpawnDeathMissile(mo->source, x, y, z);
}

void A_BrainDie(mobj_t * bossbrain, void *data)
{
	G_ExitLevel(TICRATE);
}

void A_BrainSpit(mobj_t * shooter, void *data)
{
	static int easy = 0;

	// when skill is easy, only fire every second cube.

	easy ^= 1;

	if (gameskill <= sk_easy && (!easy))
		return;

	// shoot out a cube
	A_RangeAttack(shooter, NULL);
}


void A_CubeSpawn(mobj_t * cube, void *data)
{
	mobj_t *targ;
	mobj_t *newmobj;
	const mobjtype_c *type;
	int r;

	targ = cube->target;

	// -AJA- 2007/07/28: workaround for DeHackEd patches using S_SPAWNFIRE
	if (!targ || !cube->currentattack ||
		cube->currentattack->attackstyle != ATK_SHOOTTOSPOT)
		return;
	
	// Randomly select monster to spawn.
	r = P_Random();

	// Probability distribution (kind of :)),
	// decreasing likelihood.
	if (r < 50)
		type = mobjtypes.Lookup("IMP");
	else if (r < 90)
		type = mobjtypes.Lookup("DEMON");
	else if (r < 120)
		type = mobjtypes.Lookup("SPECTRE");
	else if (r < 130)
		type = mobjtypes.Lookup("PAIN_ELEMENTAL");
	else if (r < 160)
		type = mobjtypes.Lookup("CACODEMON");
	else if (r < 162)
		type = mobjtypes.Lookup("ARCHVILE");
	else if (r < 172)
		type = mobjtypes.Lookup("REVENANT");
	else if (r < 192)
		type = mobjtypes.Lookup("ARACHNOTRON");
	else if (r < 222)
		type = mobjtypes.Lookup("MANCUBUS");
	else if (r < 246)
		type = mobjtypes.Lookup("HELL_KNIGHT");
	else
		type = mobjtypes.Lookup("BARON_OF_HELL");

	newmobj = P_MobjCreateObject(targ->x, targ->y, targ->z, type);

	if (P_LookForPlayers(newmobj, ANG180))
	{
		if (newmobj->info->chase_state)
			P_SetMobjState(newmobj, newmobj->info->chase_state);
		else
			P_SetMobjState(newmobj, newmobj->info->spawn_state);
	}

	// telefrag anything in this spot
	P_TeleportMove(newmobj, newmobj->x, newmobj->y, newmobj->z);
}

void A_PlayerScream(mobj_t * mo, void *data)
{
	sfx_t *sound;

	sound = mo->info->deathsound;

	if ((mo->health < -50) && (W_CheckNumForName("DSPDIEHI") >= 0))
	{
		// if the player dies and unclipped health is < -50%...

		sound = sfxdefs.GetEffect("PDIEHI");
	}

	S_StartFX(sound, P_MobjGetSfxCategory(mo), mo);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
