//----------------------------------------------------------------------------
//  EDGE Play Simulation Action routines
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
// Notes:
//  All Procedures here are never called directly, except possibly
//  by another P_Act* Routine. Otherwise the procedure is called
//  by referencing an code pointer from the states[] table. The only
//  exception to these rules are P_ActMissileContact and
//  P_SlammedIntoObject that requiring "acting" on the part
//  of an obj.
//
// This file was created for all action code by DDF.
//
// -KM- 1998/09/27 Added sounds.ddf capability
// -KM- 1998/12/21 New smooth visibility.
// -AJA- 1999/07/21: Replaced some non-critical P_Randoms with M_Random.
// -AJA- 1999/08/08: Replaced some P_Random()-P_Random() stuff.
//

#include "i_defs.h"
#include "p_action.h"

#include "con_main.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "g_game.h"
#include "m_inline.h"
#include "m_misc.h"
#include "m_random.h"
#include "p_local.h"
#include "p_weapon.h"
#include "r_state.h"
#include "rad_trig.h"
#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"


static int P_AttackGetSfxCategory(const mobj_t *mo)
{
	int category = P_MobjGetSfxCategory(mo);

	if (category == SNCAT_Player)
		return SNCAT_Weapon;

	return category;
}

static int SfxFlags(const mobjtype_c *info)
{
	int flags = 0;

	if (info->extendedflags & EF_ALWAYSLOUD)
		flags |= FX_Boss;

	return flags;
}

//-----------------------------------------
//--------------MISCELLANOUS---------------
//-----------------------------------------

//
// P_ActActivateLineType
//
// Allows things to also activate linetypes, bringing them into the
// fold with radius triggers, which can also do it.  There's only two
// parameters needed: linetype number & tag number, which are stored
// in the state's `action_par' field as a pointer to two integers.
// 
void P_ActActivateLineType(mobj_t * object)
{
	int *values;
  
	if (!object->state || !object->state->action_par)
		return;

	values = (int *) object->state->action_par;
  
	// Note the `NULL' here: this prevents the activation from failing
	// because the object isn't a PLAYER, for example.
	P_RemoteActivation(NULL, values[0], values[1], 0, line_Any);
}

//
// P_ActEnableRadTrig
// P_ActDisableRadTrig
//
// Allows things to enable or disable radius triggers (by tag number),
// like linetypes can do already.
//
void P_ActEnableRadTrig(mobj_t * object)
{
	int *value;
  
	if (!object->state || !object->state->action_par)
		return;

	value = (int *) object->state->action_par;
	RAD_EnableByTag(object, value[0], false);
}

void P_ActDisableRadTrig(mobj_t * object)
{
	int *value;
  
	if (!object->state || !object->state->action_par)
		return;

	value = (int *) object->state->action_par;
	RAD_EnableByTag(object, value[0], true);
}

//
// P_ActLookForTargets
//
// Looks for targets: used in the same way as enemy things look
// for players
//
// TODO: Write a decent procedure.
// -KM- 1999/01/31 Added sides. Still has to search every mobj on the
//  map to find a target.  There must be a better way...
// -AJA- 2004/04/28: Rewritten. Mobjs on same side are never targeted.
//
// NOTE: a better way might be: do a mini "BSP render", use a small 1D
//       occlusion buffer (e.g. 64 bits).
//
bool P_ActLookForTargets(mobj_t *we)
{
	mobj_t *them;

	// Optimisation: nobody to support when side is zero
	if (we->side == 0)
		return P_LookForPlayers(we, we->info->sight_angle);

	for (them = mobjlisthead; them != NULL; them = them->next)
	{
		if (them == we)
			continue;

		bool same_side = ((them->side & we->side) != 0);

		// only target monsters or players (not barrels)
		if (! (them->extendedflags & EF_MONSTER) && ! them->player)
			continue;

		if (! (them->flags & MF_SHOOTABLE))
			continue;

		if (same_side && !we->supportobj && them->supportobj != we)
		{
			if (them->supportobj && P_CheckSight(we, them->supportobj))
				them = them->supportobj;
			else if (! P_CheckSight(we, them))
				continue;  // OK since same side

			if (them)
			{
				P_MobjSetSupportObj(we, them);
				if (we->info->meander_state)
					P_SetMobjStateDeferred(we, we->info->meander_state, 0);
				return true;
			}
		}

		if (same_side)
			continue;

		/// if (them == we->supportobj || we == them->supportobj ||
		///	(them->supportobj && them->supportobj == we->supportobj))
		///	continue;

		if ((we->info == them->info) && ! (we->extendedflags & EF_DISLOYALTYPE))
			continue;

        /// -AJA- IDEALLY: use this to prioritize possible targets.
		/// if (! (them->target &&
		///	  (them->target == we || them->target == we->supportobj)))
		///	continue;

		if (P_CheckSight(we, them))
		{
			P_MobjSetTarget(we, them);
			if (we->info->chase_state)
				P_SetMobjStateDeferred(we, we->info->chase_state, 0);
			return true;
		}
	}

	return false;
}

//
// DecideMeleeAttack
//
// This is based on P_CheckMeleeRange, except that it relys upon
// info from the objects close combat attack, the original code
// used a set value for all objects which was MELEERANGE + 20,
// this code allows different melee ranges for different objects.
//
// -ACB- 1998/08/15
// -KM- 1998/11/25 Added attack parameter.
//
static bool DecideMeleeAttack(mobj_t * object, const atkdef_c * attack)
{
	mobj_t *target;
	float distance;
	float meleedist;

	target = object->target;

	if (!target)
		return false;

	distance = P_ApproxDistance(target->x - object->x, target->y - object->y);

	if (level_flags.true3dgameplay)
		distance = P_ApproxDistance(target->z - object->z, distance);

	meleedist = attack ? attack->range : MELEERANGE;
	meleedist += target->radius - 20.0f;	// Check the thing's actual radius		

	if (distance >= meleedist)
		return false;

	return P_CheckSight(object, target);
}

//
// DecideRangeAttack
//
// This is based on P_CheckMissileRange, contrary the name it does more
// than check the missile range, it makes a decision of whether or not an
// attack should be made or not depending on the object with the option
// to attack. A return of false is mandatory if the object cannot see its
// target (makes sense, doesn't it?), after this the distance is calculated,
// it will eventually be check to see if it is greater than a number from
// the Random Number Generator; if so the procedure returns true. Essentially
// the closer the object is to its target, the more chance an attack will
// be made (another logical decision).
//
// -ACB- 1998/08/15
//
static bool DecideRangeAttack(mobj_t * object)
{
	percent_t chance;
	float distance;
	const atkdef_c *attack;

	if (! object->target)
		return false;

	if (object->info->rangeattack)
		attack = object->info->rangeattack;
	else
		return false;  // cannot evaluate range with no attack range

	// Just been hit (and have felt pain), so in true tit-for-tat
	// style, the object - without regard to anything else - hits back.
	if (object->flags & MF_JUSTHIT)
	{
		if (! P_CheckSight(object, object->target))
                	return false;

		object->flags &= ~MF_JUSTHIT;
		return true;
	}

	// Bit slow on the up-take: the object hasn't had time to
	// react his target.
	if (object->reactiontime)
		return false;

	// Get the distance, a basis for our decision making from now on
	distance = P_ApproxDistance(object->x - object->target->x,
								object->y - object->target->y);

	// If no close-combat attack, increase the chance of a missile attack
	if (!object->info->melee_state)
		distance -= 192;
	else
		distance -= 64;

	// Object is too far away to attack?
	if (attack->range && distance >= attack->range)
		return false;

	// Object is too close to target
	if (attack->tooclose && attack->tooclose >= distance)
		return false;

	// Object likes to fire? if so, double the chance of it happening
	if (object->extendedflags & EF_TRIGGERHAPPY)
		distance /= 2;

	// The chance in the object is one given that the attack will happen, so
	// we inverse the result (since its one in 255) to get the chance that
	// the attack will not happen.
	chance = 1.0f - object->info->minatkchance;
	chance = MIN(distance / 255.0f, chance);

	// now after modifing distance where applicable, we get the random number and
	// check if it is less than distance, if so no attack is made.
	if (P_RandomTest(chance))
		return false;

	return P_CheckSight(object, object->target);
}

//
// P_ActFaceTarget
//
// Look at the prey......
//
void P_ActFaceTarget(mobj_t * object)
{
	mobj_t *target = object->target;

	if (!target)
		return;

	if (object->flags & MF_STEALTH)
		object->vis_target = VISIBLE;

	object->flags &= ~MF_AMBUSH;

	object->angle = R_PointToAngle(object->x, object->y, target->x, target->y);

	float dist = R_PointToDist(object->x, object->y, target->x, target->y);

	if (dist >= 0.1f)
	{
		float dz = MO_MIDZ(target) - MO_MIDZ(object);

		object->vertangle = M_ATan(dz / dist);
	}

	if (target->flags & MF_FUZZY)
	{
		object->angle += P_RandomNegPos() << (ANGLEBITS - 11);
		object->vertangle += M_ATan(P_RandomNegPos() / 1024.0f);
	}

	if (target->visibility < VISIBLE)
	{
		float amount = (VISIBLE - target->visibility);

		object->angle += (angle_t)(P_RandomNegPos() * (ANGLEBITS - 12) * amount);
		object->vertangle += M_ATan(P_RandomNegPos() * amount / 2048.0f);
	}

	// don't look up/down too far...
	if (object->vertangle > LOOKUPLIMIT && object->vertangle < LOOKDOWNLIMIT)
	{
		if (object->vertangle <= ANG180)
			object->vertangle = LOOKUPLIMIT;
		else
			object->vertangle = LOOKDOWNLIMIT;
	}
}

//
// P_ActMakeIntoCorpse
//
// Gives the effect of the object being a corpse....
//
void P_ActMakeIntoCorpse(mobj_t * mo)
{
	if (mo->flags & MF_STEALTH)
		mo->vis_target = VISIBLE;  // dead and very visible

	// object is on ground, it can be walked over
	mo->flags &= ~MF_SOLID;

  mo->tag = 0;
}

//
// P_BringCorpseToLife
//
// Bring a corpse back to life (the opposite of the above routine).
// Handles players too !
//
void P_BringCorpseToLife(mobj_t * corpse)
{
	const mobjtype_c *info = corpse->info;

	corpse->flags = info->flags;
	corpse->health = info->spawnhealth;
	corpse->radius = info->radius;
	corpse->height = info->height;
	corpse->extendedflags = info->extendedflags;
	corpse->hyperflags = info->hyperflags;
	corpse->vis_target = PERCENT_2_FLOAT(info->translucency);
	corpse->tag = corpse->spawnpoint.tag;

	if (corpse->player)
	{
		corpse->player->playerstate = PST_LIVE;
		corpse->player->health = corpse->health;
		corpse->player->std_viewheight = corpse->height * 
			PERCENT_2_FLOAT(info->viewheight);
	}

	if (info->overkill_sound)
		S_StartFX(info->overkill_sound, P_MobjGetSfxCategory(corpse), corpse);

	if (info->raise_state)
		P_SetMobjState(corpse, info->raise_state);
	else if (info->meander_state)
		P_SetMobjState(corpse, info->meander_state);
	else if (info->idle_state)
		P_SetMobjState(corpse, info->idle_state);
	else
		I_Error("Object %s has no RESURRECT states.\n", info->ddf.name.GetString());
}

//
// P_ActResetSpreadCount
//
// Resets the spreader count for fixed-order spreaders, normally used at the
// beginning of a set of missile states to ensure that an object fires in
// the same object each time.
//
void P_ActResetSpreadCount(mobj_t * object)
{
	object->spreadcount = 0;
}

//-------------------------------------------------------------------
//-------------------VISIBILITY HANDLING ROUTINES--------------------
//-------------------------------------------------------------------

//
// P_ActTransSet
//
void P_ActTransSet(mobj_t * object)
{
	const state_t *st;
	float value = VISIBLE;

	st = object->state;

	if (st && st->action_par)
	{
		value = ((percent_t *)st->action_par)[0];
		value = MAX(0.0f, MIN(1.0f, value));
	}

	object->visibility = object->vis_target = value;
}

//
// P_ActTransFade
//
void P_ActTransFade(mobj_t * object)
{
	const state_t *st;
	float value = INVISIBLE;

	st = object->state;

	if (st && st->action_par)
	{
		value = ((percent_t *)st->action_par)[0];
		value = MAX(0.0f, MIN(1.0f, value));
	}

	object->vis_target = value;
}

//
// P_ActTransLess
//
void P_ActTransLess(mobj_t * object)
{
	const state_t *st;
	float value = 0.05f;

	st = object->state;

	if (st && st->action_par)
	{
		value = ((percent_t *)st->action_par)[0];
		value = MAX(0.0f, MIN(1.0f, value));
	}

	object->vis_target -= value;

	if (object->vis_target < INVISIBLE)
		object->vis_target = INVISIBLE;
}

//
// P_ActTransMore
//
void P_ActTransMore(mobj_t * object)
{
	const state_t *st;
	float value = 0.05f;

	st = object->state;

	if (st && st->action_par)
	{
		value = ((percent_t *)st->action_par)[0];
		value = MAX(0.0f, MIN(1.0f, value));
	}

	object->vis_target += value;

	if (object->vis_target > VISIBLE)
		object->vis_target = VISIBLE;
}

//
// P_ActTransAlternate
//
// Alters the translucency of an item, EF_LESSVIS is used
// internally to tell the object if it should be getting
// more visible or less visible; EF_LESSVIS is set when an
// object is to get less visible (because it has become
// to a level of lowest translucency) and the flag is unset
// if the object has become as highly translucent as possible.
//
void P_ActTransAlternate(mobj_t * object)
{
	const state_t *st;
	float value = 0.05f;

	st = object->state;

	if (st && st->action_par)
	{
		value = ((percent_t *)st->action_par)[0];
		value = MAX(0.0f, MIN(1.0f, value));
	}

	if (object->extendedflags & EF_LESSVIS)
	{
		object->vis_target -= value;
		if (object->vis_target <= INVISIBLE)
		{
			object->vis_target = INVISIBLE;
			object->extendedflags &= ~EF_LESSVIS;
		}
	}
	else
	{
		object->vis_target += value;
		if (object->vis_target >= VISIBLE)
		{
			object->vis_target = VISIBLE;
			object->extendedflags |= EF_LESSVIS;
		}
	}
}

//
// P_ActDLightSet
//
void P_ActDLightSet(mobj_t * mo)
{
	const state_t *st = mo->state;

	if (st && st->action_par)
	{
		mo->dlight_qty = MAX(0.0f, ((int *)st->action_par)[0]);
		mo->dlight_target = mo->dlight_qty;
	}
}

//
// P_ActDLightFade
//
void P_ActDLightFade(mobj_t * mo)
{
	const state_t *st = mo->state;

	if (st && st->action_par)
	{
		mo->dlight_target = MAX(0.0f, ((int *)st->action_par)[0]);
	}
}

//
// P_ActDLightRandom
//
void P_ActDLightRandom(mobj_t * mo)
{
	const state_t *st = mo->state;

	if (st && st->action_par)
	{
		int low  = ((int *)st->action_par)[0];
		int high = ((int *)st->action_par)[1];

		// Note: using M_Random so that gameplay is unaffected
		int qty = low + (high - low) * M_Random() / 255;
    
		mo->dlight_qty = MAX(0.0f, qty);
		mo->dlight_target = mo->dlight_qty;
	}
}


//-------------------------------------------------------------------
//------------------- MOVEMENT ROUTINES -----------------------------
//-------------------------------------------------------------------

void P_ActFaceDir(mobj_t * object)
{
	const state_t *st = object->state;

	if (st && st->action_par)
		object->angle = *(angle_t *)st->action_par;
	else
		object->angle = 0;
}

void P_ActTurnDir(mobj_t * object)
{
	const state_t *st = object->state;
	angle_t turn = ANG180;

	if (st && st->action_par)
    turn = *(angle_t *)st->action_par;

	object->angle += turn;
}

void P_ActTurnRandom(mobj_t * object)
{
	const state_t *st = object->state;
	int turn = 359;

	if (st && st->action_par)
	{
        turn = (int)ANG_2_FLOAT(*(angle_t *)st->action_par);
	}

	turn = turn * P_Random() / 90;  // 10 bits of angle
   
	if (turn < 0)
		object->angle -= (angle_t)((-turn) << (ANGLEBITS - 10));
	else
		object->angle += (angle_t)(turn << (ANGLEBITS - 10));
}

void P_ActMlookFace(mobj_t * object)
{
	const state_t *st = object->state;

	if (st && st->action_par)
		object->vertangle = M_ATan(*(float *)st->action_par);
	else
		object->vertangle = 0;
}

void P_ActMlookTurn(mobj_t * object)
{
	const state_t *st = object->state;

	if (st && st->action_par)
		object->vertangle = M_ATan(*(float *)st->action_par);
}

void P_ActMoveFwd(mobj_t * object)
{
	const state_t *st = object->state;

	if (st && st->action_par)
	{
		float amount = *(float *)st->action_par;
    
		float dx = M_Cos(object->angle);
		float dy = M_Sin(object->angle);

		object->mom.x += dx * amount;
		object->mom.y += dy * amount;
	}
}

void P_ActMoveRight(mobj_t * object)
{
	const state_t *st = object->state;

	if (st && st->action_par)
	{
		float amount = *(float *)st->action_par;
    
		float dx = M_Cos(object->angle - ANG90);
		float dy = M_Sin(object->angle - ANG90);

		object->mom.x += dx * amount;
		object->mom.y += dy * amount;
	}
}

void P_ActMoveUp(mobj_t * object)
{
	const state_t *st = object->state;

	if (st && st->action_par)
		object->mom.z += *(float *)st->action_par;
}

void P_ActStopMoving(mobj_t * object)
{
	object->mom.x = object->mom.y = object->mom.z = 0;
}


//-------------------------------------------------------------------
//-------------------SOUND CAUSING ROUTINES--------------------------
//-------------------------------------------------------------------

//
// P_ActPlaySound
//
// Generate an arbitrary sound.
//
void P_ActPlaySound(mobj_t * mo)
{
	sfx_t *sound = NULL;

	if (mo->state && mo->state->action_par)
		sound = (sfx_t *) mo->state->action_par;

	if (! sound)
	{
		M_WarnError("P_ActPlaySound: missing sound name in %s.\n", 
					mo->info->ddf.name.GetString());
		return;
	}

	S_StartFX(sound, P_MobjGetSfxCategory(mo), mo);
}

//
// P_ActKillSound
//
// Kill any current sounds from this thing.
//
void P_ActKillSound(mobj_t * mo)
{
	S_StopFX(mo);
}

//
// P_ActMakeAmbientSound
//
// Just a sound generating procedure that cause the sound ref
// in seesound to be generated.
//
void P_ActMakeAmbientSound(mobj_t * object)
{
	if (object->info->seesound)
		S_StartFX(object->info->seesound, P_MobjGetSfxCategory(object), object);

#ifdef DEVELOPERS
	else
		L_WriteDebug("%s has no ambient sound\n", object->info->ddf.name.GetString());
#endif
}

//
// P_ActMakeAmbientSoundRandom
//
// Give a small "random" chance that this object will make its
// ambient sound. Currently this is a set value of 50, however
// the code that drives this, should allow for the user to set
// the value, note for further DDF Development.
//
void P_ActMakeAmbientSoundRandom(mobj_t * object)
{
	if (object->info->seesound)
	{
		if (M_Random() < 50)
			S_StartFX(object->info->seesound, P_MobjGetSfxCategory(object), object);
	}
#ifdef DEVELOPERS
	else
	{
		L_WriteDebug("%s has no ambient sound\n", object->info->ddf.name.GetString());
		return;
	}
#endif

}

//
// P_ActMakeActiveSound
//
// Just a sound generating procedure that cause the sound ref
// in activesound to be generated.
//
// -KM- 1999/01/31
//
void P_ActMakeActiveSound(mobj_t * object)
{
	if (object->info->activesound)
		S_StartFX(object->info->activesound, P_MobjGetSfxCategory(object), object);

#ifdef DEVELOPERS
	else
		L_WriteDebug("%s has no ambient sound\n", object->info->ddf.name.GetString());
#endif
}

//
// P_ActMakeDyingSound
//
// This procedure is like everyother sound generating
// procedure with the exception that if the object is
// a boss (EF_ALWAYSLOUD extended flag) then the sound is
// generated at full volume (source = NULL).
//
void P_ActMakeDyingSound(mobj_t * object)
{
	sfx_t *sound;

	sound = object->info->deathsound;

	if (sound)
	{
		S_StartFX(sound, P_MobjGetSfxCategory(object),
				       object, SfxFlags(object->info));
		return;
	}

#ifdef DEVELOPERS
	L_WriteDebug("%s has no death sound\n", object->info->ddf.name.GetString());
#endif
}

//
// P_ActMakePainSound (Ow!! it hurts!)
//
void P_ActMakePainSound(mobj_t * object)
{
	if (object->info->painsound)
	{
		S_StartFX(object->info->painsound, P_MobjGetSfxCategory(object),
				       object, SfxFlags(object->info));
	}
#ifdef DEVELOPERS
	else
	{
		L_WriteDebug("%s has no pain sound\n", object->info->ddf.name.GetString());
	}
#endif
}

//
// P_ActMakeOverKillSound
//
// -AJA- 1999/12/01: made user definable.
//
void P_ActMakeOverKillSound(mobj_t * object)
{
	if (object->info->overkill_sound)
	{
		S_StartFX(object->info->overkill_sound, P_MobjGetSfxCategory(object),
					   object, SfxFlags(object->info));
	}
#ifdef DEVELOPERS
	else
		L_WriteDebug("%s has no overkill sound\n", object->info->ddf.name.GetString());
#endif
}

//
// P_ActMakeCloseAttemptSound
//
// Attempting close combat sound
//
void P_ActMakeCloseAttemptSound(mobj_t * object)
{
	sfx_t *sound;

	if (! object->info->closecombat)
		I_Error("Object [%s] used CLOSEATTEMPTSND action, "
				"but has no CLOSE_ATTACK\n", object->info->ddf.name.GetString());
   
	sound = object->info->closecombat->initsound;

	if (sound)
	{
		S_StartFX(sound, P_MobjGetSfxCategory(object), object);
	}
#ifdef DEVELOPERS
	else
		L_WriteDebug("%s has no close combat attempt sound\n", object->info->ddf.name.GetString());
#endif
}

//
// P_ActMakeRangeAttemptSound
//
// Attempting attack at range sound
//
void P_ActMakeRangeAttemptSound(mobj_t * object)
{
	sfx_t *sound;

	if (! object->info->rangeattack)
		I_Error("Object [%s] used RANGEATTEMPTSND action, "
				"but has no RANGE_ATTACK\n", object->info->ddf.name.GetString());     

	sound = object->info->rangeattack->initsound;

	if (sound)
		S_StartFX(sound, P_MobjGetSfxCategory(object), object);
#ifdef DEVELOPERS
	else
		L_WriteDebug("%s has no range attack attempt sound\n", object->info->ddf.name.GetString());
#endif
}

//-------------------------------------------------------------------
//-------------------EXPLOSION DAMAGE ROUTINES-----------------------
//-------------------------------------------------------------------

//
// P_ActDamageExplosion
//
// Radius Attack damage set by info->damage. Used for the original Barrels
//
void P_ActDamageExplosion(mobj_t * object)
{
	float damage;
  
	DAMAGE_COMPUTE(damage, &object->info->explode_damage);

#ifdef DEVELOPERS
	if (!damage)
	{
		L_WriteDebug("%s caused no explosion damage\n", object->info->ddf.name.GetString());
		return;
	}
#endif

	// -AJA- 2004/09/27: new EXPLODE_RADIUS command (overrides normal calc)
	float radius = object->info->explode_radius;
	if (radius == 0) radius = damage;

	P_RadiusAttack(object, object->source, radius, damage,
				   &object->info->explode_damage, false);
}

//
// P_ActThrust
//
// Thrust set by info->damage.
//
void P_ActThrust(mobj_t * object)
{
	float damage;
  
	DAMAGE_COMPUTE(damage, &object->info->explode_damage);

#ifdef DEVELOPERS
	if (!damage)
	{
		L_WriteDebug("%s caused no thrust\n", object->info->ddf.name.GetString());
		return;
	}
#endif

	float radius = object->info->explode_radius;
	if (radius == 0) radius = damage;

	P_RadiusAttack(object, object->source, radius, damage,
				   &object->info->explode_damage, true);
}

//-------------------------------------------------------------------
//-------------------MISSILE HANDLING ROUTINES-----------------------
//-------------------------------------------------------------------

//
// P_ActExplode
//
// The object blows up, like a missile.
//
// -AJA- 1999/08/21: Replaced P_ActExplodeMissile (which was identical
//       to p_mobj's P_ExplodeMissile) with this.
//
void P_ActExplode(mobj_t * object)
{
	P_MobjExplodeMissile(object);
}

//
// P_ActCheckMissileSpawn
//
// This procedure handles a newly spawned missile, it moved
// by half the amount of momentum and then checked to see
// if the move is possible, if not the projectile is
// exploded. Also the number of initial tics on its
// current state is taken away from by a random number
// between 0 and 3, although the number of tics will never
// go below 1.
//
// -ACB- 1998/08/04
//
// -AJA- 1999/08/22: Fixed a bug that occasionally caused the game to
//       go into an infinite loop.  NOTE WELL: don't fiddle with the
//       object's x & y directly, use P_TryMove instead, or
//       P_ChangeThingPosition.
//
static void CheckMissileSpawn(mobj_t * projectile)
{
	projectile->tics -= P_Random() & 3;

	if (projectile->tics < 1)
		projectile->tics = 1;

	projectile->z += projectile->mom.z / 2;

	if (!P_TryMove(projectile,
				   projectile->x + projectile->mom.x / 2,
				   projectile->y + projectile->mom.y / 2))
	{
		P_MobjExplodeMissile(projectile);
	}
}

//
// LaunchProjectile
//
// This procedure launches a project the direction of the target mobj.
// * source - the source of the projectile
// * target - the target of the projectile
// * type   - the mobj type of the projectile
//
// For all sense and purposes it is possible for the target to be a dummy
// mobj, just to act as a carrier for a set of target co-ordinates.
//
// Missiles can be spawned at different locations on and around
// the mobj. Traditionally an mobj would fire a projectile
// at a height of 32 from the centerpoint of that
// mobj, this was true for all creatures from the Cyberdemon to
// the Imp. The currentattack holds the height and x & y
// offsets that dictates the spawning location of a projectile.
//
// Traditionally: Height   = 4*8
//                x-offset = 0
//                y-offset = 0
//
// The exception to this rule is the revenant, which did increase
// its z value by 16 before firing: This was a hack
// to launch a missile at a height of 48. The revenants
// height was reduced to normal after firing, this new code
// makes that an unnecesary procedure.
//
// projx, projy & projz are the projectiles spawn location
//
// NOTE: may return NULL.
//
static mobj_t *DoLaunchProjectile(mobj_t * source, float tx, float ty, float tz,
								mobj_t * target, const mobjtype_c * type)
{
	const atkdef_c *attack = source->currentattack;

	if (! attack)
		return NULL;

	// -AJA- projz now handles crouching
	float projx = source->x;
	float projy = source->y;
	float projz = source->z + attack->height * source->height / source->info->height;

	angle_t angle = source->angle;

	projx += attack->xoffset * M_Cos(angle + ANG90);
	projy += attack->xoffset * M_Sin(angle + ANG90);

	float yoffset;

	if (attack->yoffset)
		yoffset = attack->yoffset;
	else
		yoffset = source->radius - 0.5f;

	projx += yoffset * M_Cos(angle) * M_Cos(source->vertangle);
	projy += yoffset * M_Sin(angle) * M_Cos(source->vertangle);
	projz += yoffset * M_Sin(source->vertangle);

	mobj_t *projectile = P_MobjCreateObject(projx, projy, projz, type);

	// currentattack is held so that when a collision takes place
	// with another object, we know whether or not the object hit
	// can shake off the attack or is damaged by it.
	//
	projectile->currentattack = attack;
	P_MobjSetRealSource(projectile, source);

	// check for blocking lines between source and projectile
	if (P_MapCheckBlockingLine(source, projectile))
	{
		P_MobjExplodeMissile(projectile);
		return NULL;
	}

	// launch sound
	if (projectile->info && projectile->info->seesound)
	{
		int category = P_AttackGetSfxCategory(source);
		int flags = SfxFlags(projectile->info);

		mobj_t *sfx_source = projectile;
		if (category == SNCAT_Player || category == SNCAT_Weapon)
			sfx_source = source;

		S_StartFX(projectile->info->seesound, category, sfx_source, flags);
	}

	angle = R_PointToAngle(projx, projy, tx, ty);

	// Now add the fact that the target may be difficult to spot and
	// make the projectile's target the same as the sources. Only
	// do these if the object is not a dummy object, otherwise just
	// flag the missile not to trace: you cannot track a target that
	// does not exist...

	P_MobjSetTarget(projectile, target);

	if (! target)
	{
		tz += attack->height;
	}
	else
	{
		projectile->extendedflags |= EF_FIRSTCHECK;

		if (! (attack->flags & AF_Player))
		{
			if (target->flags & MF_FUZZY)
				angle += P_RandomNegPos() << (ANGLEBITS - 12);

			if (target->visibility < VISIBLE)
				angle += (angle_t)(P_RandomNegPos() * 64 * (VISIBLE - target->visibility));
		}
	}

	// Calculate slope
	float slope = P_ApproxSlope(tx - projx, ty - projy, tz - projz);

	// -AJA- 1999/09/11: add in attack's angle & slope offsets.
	angle -= attack->angle_offset;
	slope += attack->slope_offset;
  
	// is the attack not accurate?
	if (!source->player || source->player->refire > 0)
	{
		if (attack->accuracy_angle > 0.0f)
			angle += (attack->accuracy_angle >> 8) * P_RandomNegPos();

		if (attack->accuracy_slope > 0.0f)
			slope += attack->accuracy_slope * (P_RandomNegPos() / 255.0f);
	}

	P_SetMobjDirAndSpeed(projectile, angle, slope, projectile->speed);
	CheckMissileSpawn(projectile);

	return projectile;
}

static mobj_t *LaunchProjectile(mobj_t * source, mobj_t * target,
								const mobjtype_c * type)
{
	float tx, ty, tz;

	if (source->currentattack && (source->currentattack->flags & AF_NoTarget))
		target = NULL;

	P_TargetTheory(source, target, &tx, &ty, &tz);

	return DoLaunchProjectile(source, tx, ty, tz, target, type);
}

//
// LaunchSmartProjectile
//
// This procedure has the same effect as
// LaunchProjectile, but it calculates a point where the target
// and missile will intersect.  This comes from the fact that to shoot
// something, you have to aim slightly ahead of it.  It will also put
// an end to circle-strafing.  :-)
//
// -KM- 1998/10/29
// -KM- 1998/12/16 Fixed it up.  Works quite well :-)
//
static void LaunchSmartProjectile(mobj_t * source, mobj_t * target, 
								  const mobjtype_c * type)
{
	float t = -1;
	float mx = 0, my = 0;

	if (target)
	{
		mx = target->mom.x;
		my = target->mom.y;

		float dx = source->x - target->x;
		float dy = source->y - target->y;

		float s = type->speed;
		if (level_flags.fastparm)
			s *= type->fast;

		float a = mx * mx + my * my - s * s;
		float b = 2 * (dx * mx + dy * my);
		float c = dx * dx + dy * dy;

		float t1 = -1, t2 = -1;

		// find solution to the quadratic equation
		if (a && ((b * b - 4 * a * c) >= 0))
		{
			t1 = -b + (float)sqrt(b * b - 4.0f * a * c);
			t1 /= 2.0f * a;

			t2 = -b - (float)sqrt(b * b - 4.0f * a * c);
			t2 /= 2.0f * a;
		}

		if (t1 < 0)
			t = t2;
		else if (t2 < 0)
			t = t1;
		else
			t = (t1 < t2) ? t1 : t2;
	}

	if (t <= 0)
	{
		// -AJA- when no target, fall back to "dumb mode"
		LaunchProjectile(source, target, type);
	}
	else
	{
		// -AJA- 2005/02/07: assumes target doesn't move up or down

		float tx = target->x + mx * t;
		float ty = target->y + my * t;
		float tz = MO_MIDZ(target);

		DoLaunchProjectile(source, tx, ty, tz, target, type);

#if 0  // -AJA- this doesn't seem correct / consistent
		if (projectile)
			source->angle = projectile->angle;
#endif
	}
}

//
// P_MissileContact
//
// Called by PIT_CheckRelThing when a missile comes into
// contact with another object. Placed here with
// the other missile code for cleaner code.
//
// Returns: -1 if missile should pass through.
//           0 if hit but no damage was done.
//          +1 if hit and damage was done.
//
int P_MissileContact(mobj_t * object, mobj_t * objecthit)
{
	mobj_t *source = object->source;

	if (source)
	{
		// check for ghosts (attack passes through)
		if (object->currentattack && BITSET_EMPTY ==
			(object->currentattack->attack_class & ~objecthit->info->ghost))
			return -1;

		if ((objecthit->side & source->side) != 0)
		{
			if (objecthit->hyperflags & HF_SIDEGHOST)
				return -1;

			if (objecthit->hyperflags & HF_SIDEIMMUNE)
				return 0;
		}

		if (source->info == objecthit->info)
		{
			if (!(objecthit->extendedflags & EF_DISLOYALTYPE))
				return 0;
		}

		if (object->currentattack != NULL &&
			! (objecthit->extendedflags & EF_OWNATTACKHURTS))
		{
			if (object->currentattack == objecthit->info->rangeattack)
				return 0;
			if (object->currentattack == objecthit->info->closecombat)
				return 0;
		}
	}

	// check for immunity against the attack
	if (object->currentattack && BITSET_EMPTY ==
		(object->currentattack->attack_class & ~objecthit->info->immunity))
	{
		return 0;
	}

	// support for "tunnelling" missiles, which should only do damage at
	// the first impact.
	if (object->extendedflags & EF_TUNNEL)
	{
		// this hash is very basic, but should work OK
		u32_t hash = (u32_t)(long)objecthit;

		if (object->tunnel_hash[0] == hash || object->tunnel_hash[1] == hash)
			return -1;

		object->tunnel_hash[0] = object->tunnel_hash[1];
		object->tunnel_hash[1] = hash;
	}

	const damage_c *damtype;
	float damage;

	// transitional hack
	if (object->currentattack)
		damtype = &object->currentattack->damage;
	else
		damtype = &object->info->explode_damage;

	DAMAGE_COMPUTE(damage, damtype);

	// Berserk handling
	if (object->currentattack && object->player &&
		object->player->powers[PW_Berserk] != 0.0f)
	{
		damage *= object->currentattack->berserk_mul;
	}

	if (!damage)
	{
#ifdef DEVELOPERS
		L_WriteDebug("%s missile did zero damage.\n", 
					 object->info->ddf.name.GetString());
#endif
		return 0;
	}

	P_DamageMobj(objecthit, object, object->source, damage, damtype);
	return 1;
}

//
// P_BulletContact
//
// Called by PTR_ShootTraverse when a bullet comes into contact with
// another object.  Needed so that the "DISLOYAL" special will behave
// in the same manner for bullets as for missiles.  Note: also used
// for close combat attacks.
//
// Returns: -1 if bullet should pass through.
//           0 if hit but no damage was done.
//          +1 if hit and damage was done.
//
int P_BulletContact(mobj_t * source, mobj_t * objecthit, 
							 float damage, const damage_c *damtype)
{
	// check for ghosts (attack passes through)
	if (source->currentattack && BITSET_EMPTY ==
		(source->currentattack->attack_class & ~objecthit->info->ghost))
		return -1;

	if ((objecthit->side & source->side) != 0)
	{
		if (objecthit->hyperflags & HF_SIDEGHOST)
			return -1;

		if (objecthit->hyperflags & HF_SIDEIMMUNE)
			return 0;
	}

	if (source->info == objecthit->info)
	{
		if (! (objecthit->extendedflags & EF_DISLOYALTYPE))
			return 0;
	}

	if (source->currentattack != NULL &&
		!(objecthit->extendedflags & EF_OWNATTACKHURTS))
	{
		if (source->currentattack == objecthit->info->rangeattack)
			return 0;
		if (source->currentattack == objecthit->info->closecombat)
			return 0;
	}

	// check for immunity against the attack
	if (source->currentattack && BITSET_EMPTY ==
		(source->currentattack->attack_class & ~objecthit->info->immunity))
	{
		return 0;
	}

	if (!damage)
	{
#ifdef DEVELOPERS
		L_WriteDebug("%s's shoot/combat attack did zero damage.\n", 
					 source->info->ddf.name.GetString());
#endif
		return 0;
	}

	P_DamageMobj(objecthit, source, source, damage, damtype);
	return 1;
}

//
// P_ActCreateSmokeTrail
//
// Just spawns smoke behind an mobj: the smoke is
// risen by giving it z momentum, in order to
// prevent the smoke appearing uniform (which obviously
// does not happen), the number of tics that the smoke
// mobj has is "randomly" reduced, although the number
// of tics never gets to zero or below.
//
// -ACB- 1998/08/10 Written
// -ACB- 1999/10/01 Check thing's current attack has a smoke projectile
//
void P_ActCreateSmokeTrail(mobj_t * projectile)
{
	const atkdef_c *attack = projectile->currentattack;

	if (attack == NULL)
		return;

	if (attack->puff == NULL)
	{
		M_WarnError("P_ActCreateSmokeTrail: attack %s has no PUFF object\n",
					attack->ddf.name.GetString());
		return;
	}
  
	// spawn a puff of smoke behind the rocket
	mobj_t *smoke = P_MobjCreateObject(
		projectile->x - projectile->mom.x / 2.0f,
		projectile->y - projectile->mom.y / 2.0f,
		projectile->z, attack->puff);

	smoke->mom.z = smoke->info->float_speed;
	smoke->tics -= M_Random() & 3;

	if (smoke->tics < 1)
		smoke->tics = 1;
}

//
// P_ActHomingProjectile
//
// This projectile will alter its course to intercept its
// target, if is possible for this procedure to be called
// and nothing results because of a chance that the
// projectile will not chase its target.
//
// As this code is based on the revenant tracer, it did use
// a bit check on the current gametic - which was why every so
// often a revenant fires a missile straight and not one that
// homes in on its target: If the gametic has bits 1+2 on
// (which boils down to 1 in every 4 tics), the trick in this
// is that - in conjuntion with the tic count for the
// tracing object's states - the tracing will always fail or
// pass the check: if it passes first time, it will always
// pass and vice versa. The problem with this was two fold:
// demos will go out of sync if the starting gametic if different
// from when the demo was recorded (which admittly is easily
// fixable), the second is that for someone designing a new
// tracing projectile it would be more than a bit confusing to
// joe "dooming" public.
//
// The new system that affects the original gameplay slightly is
// to get a random chance of the projectile not homing in on its
// target and working this out first time round, the test result
// is recorded (by clearing the 'target' field).
//
// -ACB- 1998/08/10
//
void P_ActHomingProjectile(mobj_t * projectile)
{
	const atkdef_c *attack = projectile->currentattack;

	if (attack == NULL)
		return;

	if (attack->flags & AF_TraceSmoke)
		P_ActCreateSmokeTrail(projectile);

	if (projectile->extendedflags & EF_FIRSTCHECK)
	{
		projectile->extendedflags &= ~EF_FIRSTCHECK;

		if (P_RandomTest(attack->notracechance))
		{
			P_MobjSetTarget(projectile, NULL);
			return;
		}
	}

	mobj_t *destination = projectile->target;

	if (!destination || destination->health <= 0)
		return;

	// change angle
	angle_t exact = R_PointToAngle(projectile->x, projectile->y,
								   destination->x, destination->y);

	if (exact != projectile->angle)
	{
		if (exact - projectile->angle > ANG180)
		{
			projectile->angle -= attack->trace_angle;

			if (exact - projectile->angle < ANG180)
				projectile->angle = exact;
		}
		else
		{
			projectile->angle += attack->trace_angle;

			if (exact - projectile->angle > ANG180)
				projectile->angle = exact;
		}
	}

	projectile->mom.x = projectile->speed * M_Cos(projectile->angle);
	projectile->mom.y = projectile->speed * M_Sin(projectile->angle);

	// change slope
	float slope = P_ApproxSlope(destination->x - projectile->x,
						  destination->y - projectile->y,
						  MO_MIDZ(destination) - projectile->z);

	slope *= projectile->speed;

	if (slope < projectile->mom.z)
		projectile->mom.z -= 0.125f;
	else
		projectile->mom.z += 0.125f;
}

//
// P_ActHomeToSpot
//
// This projectile will alter its course to intercept its target,
// or explode if it has reached it.  Used by the bossbrain cube.
//
void P_ActHomeToSpot(mobj_t * projectile)
{
	mobj_t *target = projectile->target;
  
	if (!target)
	{
		P_MobjExplodeMissile(projectile);
		return;
	}

	float dx = target->x - projectile->x;
	float dy = target->y - projectile->y;
	float dz = target->z - projectile->z;

	float ck_radius = target->radius + projectile->radius + 2;
	float ck_height = target->height + projectile->height + 2;
  
	// reached target ?
	if (fabs(dx) <= ck_radius && fabs(dy) <= ck_radius && fabs(dz) <= ck_height)
	{
		P_MobjExplodeMissile(projectile);
		return;
	}

	// calculate new angles
	angle_t angle = R_PointToAngle(0, 0, dx, dy);
	float slope = P_ApproxSlope(dx, dy, dz);

	P_SetMobjDirAndSpeed(projectile, angle, slope, projectile->speed);
}

//
// LaunchOrderedSpread
//
// Due to the unique way of handling that the mancubus fires, it is necessary
// to write a single procedure to handle the firing. In real terms it amounts
// to a glorified hack; The table holds the angle modifier and the choice of
// whether the firing object or the projectile is affected. This procedure
// should NOT be used for players as it will alter the player's mobj, bypassing
// the normal player controls; The only reason for its existance is to maintain
// the original mancubus behaviour. Although it is possible to make this generic,
// the benefits of doing so are minimal. Purist function....
//
// -ACB- 1998/08/15
//
static void LaunchOrderedSpread(mobj_t * object)
{
	// left side = angle modifier
	// right side = object or projectile (true for object).
	static int spreadorder[] =
	{
		(ANG90 / 8), true,
		(ANG90 / 8), false,
		-(ANG90 / 8), true,
		-(ANG90 / 4), false,
		-(ANG90 / 16), false,
		(ANG90 / 16), false
	};

	const atkdef_c *attack = object->currentattack;

	if (attack == NULL)
		return;

	int count = object->spreadcount;

	if (count < 0 || count > 12)
		count = object->spreadcount = 0;

  // object or projectile? - if true is the object, else it is the projectile
	if (spreadorder[count + 1])
	{
		object->angle += spreadorder[count];

		LaunchProjectile(object, object->target, attack->atk_mobj);
	}
	else
	{
		mobj_t *projectile = LaunchProjectile(object, object->target,
											  attack->atk_mobj);
		if (projectile == NULL)
			return;

		projectile->angle += spreadorder[count];

		projectile->mom.x = projectile->speed * M_Cos(projectile->angle);
		projectile->mom.y = projectile->speed * M_Sin(projectile->angle);
	}

	object->spreadcount += 2;
}

//
// LaunchRandomSpread
//
// This is a the generic function that should be used for a spreader like
// mancubus, although its random nature would certainly be a change to the
// ordered method used now. The random number is bit shifted to the right
// and then the ANG90 is divided by it, the first bit of the RN is checked
// to detemine if the angle is change is negative or not (approx 50% chance).
// The result is the modifier for the projectile's angle.
//
// -ACB- 1998/08/15
//
static void LaunchRandomSpread(mobj_t * object)
{
	if (object->currentattack == NULL)
		return;

	mobj_t *projectile = LaunchProjectile(object, object->target,
								  object->currentattack->atk_mobj);
	if (projectile == NULL)
		return;

	int i = P_Random() & 127;

	if (i >> 1)
	{
		angle_t spreadangle = (ANG90 / (i >> 1));

		if (i & 1)
			spreadangle -= spreadangle << 1;

		projectile->angle += spreadangle;
	}

	projectile->mom.x = projectile->speed * M_Cos(projectile->angle);
	projectile->mom.y = projectile->speed * M_Sin(projectile->angle);
}

//-------------------------------------------------------------------
//-------------------LINEATTACK ATTACK ROUTINES-----------------------
//-------------------------------------------------------------------

// -KM- 1998/11/25 Added uncertainty to the z component of the line.
static void ShotAttack(mobj_t * object)
{
	const atkdef_c *attack = object->currentattack;

	if (! attack)
		return;

	float range = (attack->range > 0) ? attack->range : MISSILERANGE;

	// -ACB- 1998/09/05 Remember to use the object angle, fool!
	angle_t objangle = object->angle;
	float objslope;

	if ((object->player && !object->target) || (attack->flags & AF_NoTarget))
		objslope = M_Tan(object->vertangle);
	else
		P_AimLineAttack(object, objangle, range, &objslope);

	if (attack->sound)
		S_StartFX(attack->sound, P_AttackGetSfxCategory(object), object);

	// -AJA- 1999/09/10: apply the attack's angle offsets.
	objangle -= attack->angle_offset;
	objslope += attack->slope_offset;
  
	for (int i = 0; i < attack->count; i++)
	{
		angle_t angle = objangle;
		float slope = objslope;

		// is the attack not accurate?
		if (!object->player || object->player->refire > 0)
		{
			if (attack->accuracy_angle > 0)
				angle += (attack->accuracy_angle >> 8) * P_RandomNegPos();

			if (attack->accuracy_slope > 0)
				slope += attack->accuracy_slope * (P_RandomNegPos() / 255.0f);
		}

		float damage;
		DAMAGE_COMPUTE(damage, &attack->damage);

		if (object->player && object->player->powers[PW_Berserk] != 0.0f)
			damage *= attack->berserk_mul;

		P_LineAttack(object, angle, range, slope, damage,
					 &attack->damage, attack->puff);
	}
}

// -KM- 1998/11/25 BFG Spray attack.  Must be used from missiles.
//   Will do a BFG spray on every monster in sight.
static void SprayAttack(mobj_t * mo)
{
	const atkdef_c *attack = mo->currentattack;

	if (! attack)
		return;

	float range = (attack->range > 0) ? attack->range : MISSILERANGE;

	// offset angles from its attack angle
	for (int i = 0; i < 40; i++)
	{
		angle_t an = mo->angle - ANG90 / 2 + (ANG90 / 40) * i;

		// mo->source is the originator (player) of the missile
		mobj_t *target = P_AimLineAttack(mo->source ? mo->source : mo, an, range, NULL);

		if (! target)
			continue;

		mobj_t *ball = P_MobjCreateObject(target->x, target->y,
							   target->z + target->height / 4,
							   attack->atk_mobj);

		P_MobjSetTarget(ball, mo->target);

		// check for immunity against the attack
		if (BITSET_EMPTY == (attack->attack_class & ~target->info->immunity))
		{
			return;
		}

		float damage;
		DAMAGE_COMPUTE(damage, &attack->damage);

		if (mo->player && mo->player->powers[PW_Berserk] != 0.0f)
			damage *= attack->berserk_mul;

		if (damage)
			P_DamageMobj(target, NULL, mo->source, damage, &attack->damage);
	}
}

static void DoMeleeAttack(mobj_t * mo)
{
	const atkdef_c *attack = mo->currentattack;

	float range = (attack->range > 0) ? attack->range : MISSILERANGE;

	float damage;
	DAMAGE_COMPUTE(damage, &attack->damage);

	// -KM- 1998/11/25 Berserk ability
	// -ACB- 2004/02/04 Only zero is off
	if (mo->player && mo->player->powers[PW_Berserk] != 0.0f)
		damage *= attack->berserk_mul;

	// -KM- 1998/12/21 Use Line attack so bullet puffs are spawned.

	if (! DecideMeleeAttack(mo, attack))
	{
		P_LineAttack(mo, mo->angle, range, M_Tan(mo->vertangle), 
					 damage, &attack->damage, attack->puff);
		return;
	}

	if (attack->sound)
		S_StartFX(attack->sound, P_AttackGetSfxCategory(mo), mo);

	float slope;

	P_AimLineAttack(mo, mo->angle, range, &slope);

	P_LineAttack(mo, mo->angle, range, slope, damage,
				 &attack->damage, attack->puff);
}

//-------------------------------------------------------------------
//--------------------TRACKER HANDLING ROUTINES----------------------
//-------------------------------------------------------------------

//
// A Tracker is an object that follows its target, by being on top of
// it. This is the attack style used by an Arch-Vile. The first routines
// handle the tracker itself, the last two are called by the source of
// the tracker.
//

//
// P_ActTrackerFollow
//
// Called by the tracker to follow its target.
//
// -ACB- 1998/08/22
//
void P_ActTrackerFollow(mobj_t * tracker)
{
	mobj_t *destination = tracker->target;

	if (!destination || !tracker->source)
		return;

	// Can the parent of the tracker see the target?
	if (!P_CheckSight(tracker->source, destination))
		return;

	angle_t angle = destination->angle;

	P_ChangeThingPosition(tracker,
						  destination->x + 24 * M_Cos(angle),
						  destination->y + 24 * M_Sin(angle),
						  destination->z);
}

//
// P_ActTrackerActive
//
// Called by the tracker to make its active sound: also tracks
//
// -ACB- 1998/08/22
//
void P_ActTrackerActive(mobj_t * tracker)
{
	if (tracker->info->activesound)
		S_StartFX(tracker->info->activesound, P_MobjGetSfxCategory(tracker), tracker);

	P_ActTrackerFollow(tracker);
}

//
// P_ActTrackerStart
//
// Called by the tracker to make its launch (see) sound: also tracks
//
// -ACB- 1998/08/22
//
void P_ActTrackerStart(mobj_t * tracker)
{
	if (tracker->info->seesound)
		S_StartFX(tracker->info->seesound, P_MobjGetSfxCategory(tracker), tracker);

	P_ActTrackerFollow(tracker);
}

//
// LaunchTracker
//
// This procedure starts a tracking object off and links
// the tracker and the monster together.
//
// -ACB- 1998/08/22
//
static void LaunchTracker(mobj_t * object)
{
	const atkdef_c *attack = object->currentattack;
	mobj_t *target = object->target;

	if (!attack || !target)
		return;

	mobj_t *tracker = P_MobjCreateObject(target->x, target->y, target->z,
								 attack->atk_mobj);

	// link the tracker to the object
	P_MobjSetTracer(object, tracker);

	// tracker source is the object
	P_MobjSetRealSource(tracker, object);

	// tracker's target is the object's target
	P_MobjSetTarget(tracker, target);

	P_ActTrackerFollow(tracker);
}

//
// P_ActEffectTracker
//
// Called by the object that launched the tracker to
// cause damage to its target and a radius attack
// (explosion) at the location of the tracker.
//
// -ACB- 1998/08/22
//
void P_ActEffectTracker(mobj_t * object)
{
	mobj_t *tracker;
	mobj_t *target;
	const atkdef_c *attack;
	angle_t angle;
	float damage;

	if (!object->target || !object->currentattack)
		return;

	attack = object->currentattack;
	target = object->target;

	if (attack->flags & AF_FaceTarget)
		P_ActFaceTarget(object);

	if (attack->flags & AF_NeedSight)
	{
		if (!P_CheckSight(object, target))
			return;
	}

	if (attack->sound)
		S_StartFX(attack->sound, P_MobjGetSfxCategory(object), object);

	angle = object->angle;
	tracker = object->tracer;

	DAMAGE_COMPUTE(damage, &attack->damage);

	if (damage)
		P_DamageMobj(target, object, object, damage, &attack->damage);
#ifdef DEVELOPERS
	else
		L_WriteDebug("%s + %s attack has zero damage\n",
					 object->info->ddf.name.GetString(), 
					 tracker->info->ddf.name.GetString());
#endif

	// -ACB- 2000/03/11 Check for zero mass
	if (target->info->mass)
		target->mom.z = 1000 / target->info->mass;
	else
		target->mom.z = 2000;

	if (!tracker)
		return;

	// move the tracker between the object and the object's target

	P_ChangeThingPosition(tracker,
						  target->x - 24 * M_Cos(angle),
						  target->y - 24 * M_Sin(angle),
						  target->z);

#ifdef DEVELOPERS
	if (!tracker->info->explode_damage.nominal)
		L_WriteDebug("%s + %s explosion has zero damage\n",
					 object->info->ddf.name.GetString(), 
					 tracker->info->ddf.name.GetString());
#endif

	DAMAGE_COMPUTE(damage, &tracker->info->explode_damage);

	float radius = object->info->explode_radius;
	if (radius == 0) radius = damage;

	P_RadiusAttack(tracker, object, radius, damage,
				   &tracker->info->explode_damage, false);
}

//-----------------------------------------------------------------
//--------------------BOSS HANDLING PROCEDURES---------------------
//-----------------------------------------------------------------

static void ShootToSpot(mobj_t * object)
{
	// Note: using a static int here for better randomness.
	// -AJA- FIXME: should be global to allow synchronisation in Netgames
	static int current_spot = 0;

	if (! object->currentattack)
		return;

	if (brain_spots.number < 0)
	{
		if (! object->info->spitspot)
		{
			M_WarnError("Thing [%s] used SHOOT_TO_SPOT attack, but has no "
						"SPIT_SPOT\n", object->info->ddf.name.GetString());
			return;
		}

		P_LookForShootSpots(object->info->spitspot);
	}

	if (brain_spots.number == 0)
		return;

	DEV_ASSERT2(brain_spots.targets);
	DEV_ASSERT2(brain_spots.number > 0);

	current_spot += P_Random();
	current_spot %= brain_spots.number;
  
	LaunchProjectile(object, brain_spots.targets[current_spot],
					 object->currentattack->atk_mobj);
}

//-------------------------------------------------------------------
//-------------------OBJECT-SPAWN-OBJECT HANDLING--------------------
//-------------------------------------------------------------------

//
// P_ActObjectSpawning
//
// An Object spawns another object and is spawned in the state specificed
// by attack->objinitstate. The procedure is based on the A_PainShootSkull
// which is the routine for shooting skulls from a pain elemental. In
// this the object being created is decided in the attack. This
// procedure also used the new blocking line check to see if
// the object is spawned across a blocking line, if so the procedure
// terminates.
//
// -ACB- 1998/08/23
//
static void ObjectSpawning(mobj_t * parent, angle_t angle)
{
	float slope;
	const atkdef_c *attack;

	attack = parent->currentattack;
	if (! attack)
		return;

	const mobjtype_c *shoottype = attack->spawnedobj;

	if (! shoottype)
	{
		I_Error("Object [%s] uses spawning attack [%s], but no object "
				"specified.\n", 
				parent->info->ddf.name.GetString(), 
				attack->ddf.name.GetString());
	}

	if (attack->spawn_limit > 0)
	{
		int count = 0;
		for (mobj_t *mo = mobjlisthead; mo; mo = mo->next)
			if (mo->info == shoottype)
				if (++count >= attack->spawn_limit)
					return;
	}

	// -AJA- 1999/09/10: apply the angle offset of the attack.
	angle -= attack->angle_offset;
	slope = M_Tan(parent->vertangle) + attack->slope_offset;
  
	float spawnx = parent->x;
	float spawny = parent->y;
	float spawnz = parent->z + attack->height;

	if (attack->flags & AF_PrestepSpawn)
	{
		float prestep = 4.0f + 1.5f * parent->radius + shoottype->radius;

		spawnx += prestep * M_Cos(angle);
		spawny += prestep * M_Sin(angle);
	}

	mobj_t *child = P_MobjCreateObject(spawnx, spawny, spawnz, shoottype);

	// Blocking line detected between object and spawnpoint?
	if (P_MapCheckBlockingLine(parent, child))
	{
		// -KM- 1999/01/31 Explode objects over remove them.
		// -AJA- 2000/02/01: Remove now the default.
		if (attack->flags & AF_KillFailedSpawn)
			P_KillMobj(parent, child, NULL);
		else
			P_RemoveMobj(child);

		return;
	}

	if (attack->sound)
		S_StartFX(attack->sound, P_AttackGetSfxCategory(parent), parent);

	// If the object cannot move from its position, remove it or kill it.
	if (!P_TryMove(child, child->x, child->y))
	{
		if (attack->flags & AF_KillFailedSpawn)
			P_KillMobj(parent, child, NULL);
		else
			P_RemoveMobj(child);

		return;
	}

	if (! (attack->flags & AF_NoTarget))
		P_MobjSetTarget(child, parent->target);

	P_MobjSetSupportObj(child, parent);

	child->side = parent->side;

	// -AJA- 2004/09/27: keep ambush status of parent
	child->flags |= (parent->flags & MF_AMBUSH);

	// -AJA- 1999/09/25: Set the initial direction & momentum when
	//       the ANGLED_SPAWN attack special is used.
	if (attack->flags & AF_AngledSpawn)
		P_SetMobjDirAndSpeed(child, angle, slope, attack->assault_speed);

	P_SetMobjStateDeferred(child, attack->objinitstate, 0);
}

//
// P_ActObjectTripleSpawn
//
// Spawns three objects at 90, 180 and 270 degrees. This is essentially
// another purist function to support the death sequence of the Pain
// elemental. However it could be used as in conjunction with radius
// triggers to generate a nice teleport spawn invasion.
//
// -ACB- 1998/08/23 (I think....)
//

static void ObjectTripleSpawn(mobj_t * object)
{
	ObjectSpawning(object, object->angle + ANG90);
	ObjectSpawning(object, object->angle + ANG180);
	ObjectSpawning(object, object->angle + ANG270);
}

//-------------------------------------------------------------------
//-------------------SKULLFLY HANDLING ROUTINES----------------------
//-------------------------------------------------------------------

//
// SkullFlyAssault
//
// This is the attack procedure for objects that launch themselves
// at their target like a missile.
//
// -ACB- 1998/08/16
//
static void SkullFlyAssault(mobj_t * object)
{
	if (!object->currentattack)
		return;

	if (!object->target && !object->player)
	{
		// -AJA- 2000/09/29: fix for the zombie lost soul bug
		// -AJA- 2000/10/22: monsters only !  Don't stuff up gibs/missiles.
		if (object->extendedflags & EF_MONSTER)
			object->flags |= MF_SKULLFLY;
		return;
	}

	float speed = object->currentattack->assault_speed;

	// -KM- 1999/01/31 Fix skulls in nightmare mode
	if (level_flags.fastparm)
		speed *= object->info->fast;

	sfx_t *sound = object->currentattack->initsound;

	if (sound)
		S_StartFX(sound, P_MobjGetSfxCategory(object), object);

	object->flags |= MF_SKULLFLY;

	// determine destination
	float tx, ty, tz;

	P_TargetTheory(object, object->target, &tx, &ty, &tz);

	float slope = P_ApproxSlope(tx - object->x, ty - object->y, tz - object->z);

	P_SetMobjDirAndSpeed(object, object->angle, slope, speed);
}

//
// P_SlammedIntoObject
//
// Used when a flying object hammers into another object when on the
// attack. Replaces the code in PIT_Checkthing.
//
// -ACB- 1998/07/29: Written
//
// -AJA- 1999/09/12: Now uses P_SetMobjStateDeferred, since this
//                   routine can be called by TryMove/PIT_CheckRelThing.
//
void P_SlammedIntoObject(mobj_t * object, mobj_t * objecthit)
{
	sfx_t *sound;
	float damage;

	if (object->currentattack)
	{
		if (objecthit != NULL)
		{
			// -KM- 1999/01/31 Only hurt shootable objects...
			if (objecthit->flags & MF_SHOOTABLE)
			{
				DAMAGE_COMPUTE(damage, &object->currentattack->damage);

				P_DamageMobj(objecthit, object, object, damage,
							 &object->currentattack->damage);
			}
		}

		sound = object->currentattack->sound;
		if (sound)
			S_StartFX(sound, P_MobjGetSfxCategory(object), object);
	}

	object->flags &= ~MF_SKULLFLY;
	object->mom.x = object->mom.y = object->mom.z = 0;

	P_SetMobjStateDeferred(object, object->info->idle_state, 0);
}

//
// P_UseThing
//
// Called when this thing is attempted to be used (i.e. by pressing
// the spacebar near it) by the player.  Returns true if successfully
// used, or false if other things should be checked.
//
bool P_UseThing(mobj_t * user, mobj_t * thing, float open_bottom,
				float open_top)
{
	// item is disarmed ?
	if (!(thing->flags & MF_TOUCHY))
		return false;
  
  // can be reached ?
	open_top    = MIN(open_top, thing->z + thing->height);
	open_bottom = MAX(open_bottom, thing->z);
  
	if (user->z >= open_top || 
		(user->z + user->height + USE_Z_RANGE < open_bottom))
		return false;
  
  // OK, disarm and put into touch states  
	DEV_ASSERT2(thing->info->touch_state > 0);

	thing->flags &= ~MF_TOUCHY;
	P_SetMobjStateDeferred(thing, thing->info->touch_state, 0);

	return true;
}

//
// P_TouchyContact
//
// Used whenever a thing comes into contact with a TOUCHY object.
//
// -AJA- 1999/09/12: Now uses P_SetMobjStateDeferred, since this
//       routine can be called by TryMove/PIT_CheckRelThing.
//
void P_TouchyContact(mobj_t *touchy, mobj_t *victim)
{
	// dead thing touching. Can happen with a sliding player corpse.
	if (victim->health <= 0)
		return;

  // don't harm the grenadier...
	if (touchy->source == victim)
		return;

	P_MobjSetTarget(touchy, victim);
	touchy->flags &= ~MF_TOUCHY;  // disarm

	if (touchy->info->touch_state)
		P_SetMobjStateDeferred(touchy, touchy->info->touch_state, 0);
	else
		P_MobjExplodeMissile(touchy);
}

//
// P_ActTouchyRearm    
// P_ActTouchyDisarm
//
void P_ActTouchyRearm(mobj_t * touchy)
{
	touchy->flags |= MF_TOUCHY;
}

void P_ActTouchyDisarm(mobj_t * touchy)
{
	touchy->flags &= ~MF_TOUCHY;
}

//
// P_ActBounceRearm
// P_ActBounceDisarm
//
void P_ActBounceRearm(mobj_t * mo)
{
	mo->extendedflags &= ~EF_JUSTBOUNCED;
}

void P_ActBounceDisarm(mobj_t * mo)
{
	mo->extendedflags |= EF_JUSTBOUNCED;
}

//
// P_ActDropItem
//
// -AJA- 2000/10/20: added.
//
void P_ActDropItem(mobj_t * mo)
{
	const mobjtype_c *info = mo->info->dropitem;
	mobj_t *item;

	float dx, dy;

	if (mo->state && mo->state->action_par)
	{
		mobj_strref_c *ref = (mobj_strref_c *) mo->state->action_par;

		info = ref->GetRef();
	}

	if (! info)
	{
		M_WarnError("P_ActDropItem: %s specifies no item to drop.\n", 
					mo->info->ddf.name.GetString());
		return;
	}

	// unlike normal drops, these ones are displaced randomly

	dx = P_RandomNegPos() * mo->info->radius / 255.0f;
	dy = P_RandomNegPos() * mo->info->radius / 255.0f;

	item = P_MobjCreateObject(mo->x + dx, mo->y + dy, mo->floorz, info);

	// -ES- 1998/07/18 NULL check to prevent crashing
	if (item)
	{
		item->flags |= MF_DROPPED;
		item->flags &= ~MF_SOLID;

		item->angle = mo->angle;

		// allow respawning
		item->spawnpoint.x = item->x;
		item->spawnpoint.y = item->y;
		item->spawnpoint.z = item->z;
		item->spawnpoint.angle = item->angle;
		item->spawnpoint.vertangle = item->vertangle;
		item->spawnpoint.info  = info;
		item->spawnpoint.flags = 0;
	}
}

//
// P_ActPathCheck
//
// Checks if the creature is a path follower, and if so enters the
// meander states.
//
// -AJA- 2000/02/17: wrote this & PathFollow.
//
void P_ActPathCheck(mobj_t * mo)
{
	if (! mo->path_trigger || ! mo->info->meander_state)
		return;
 
	P_SetMobjStateDeferred(mo, mo->info->meander_state, 0);

	mo->movedir = DI_SLOWTURN;
	mo->movecount = 0;
}

//
// P_ActPathFollow
//
// For path-following creatures (spawned via RTS), makes the creature
// follow the path by trying to get to the next node.
//
void P_ActPathFollow(mobj_t * mo)
{
	float dx, dy;
	angle_t diff;

	if (!mo->path_trigger)
		return;

	if (RAD_CheckReachedTrigger(mo))
	{
		// reached the very last one ?
		if (!mo->path_trigger)
		{
			mo->movedir = DI_NODIR;
			return;
		}

		mo->movedir = DI_SLOWTURN;
		return;
	}

	dx = mo->path_trigger->x - mo->x;
	dy = mo->path_trigger->y - mo->y;

	diff = mo->angle - R_PointToAngle(0, 0, dx, dy);

	// movedir value: 
	//   0 for slow turning.
	//   1 for fast turning.
	//   2 for walking.
	//   3 for evasive maneouvres.

	// if (mo->movedir < 2)

	if (mo->movedir == DI_SLOWTURN || mo->movedir == DI_FASTTURN)
	{
		angle_t step = (mo->movedir == DI_FASTTURN) ? ANG1*30 : ANG1*7;
    
		// if not facing the path node, turn towards it
		if (diff > ANG1*15 && diff < ANG180)
		{
			mo->angle -= P_Random() * (step >> 8);
			return;
		}
		else if (diff >= ANG180 && diff < ANG1*345U)
		{
			mo->angle += P_Random() * (step >> 8);
			return;
		}

		mo->movedir = DI_WALKING;
	}

	if (mo->movedir == DI_WALKING)
	{
		if (diff < ANG1*30)
			mo->angle -= ANG1;
		else if (diff >= ANG1*330U)
			mo->angle += ANG1;
		else
			mo->movedir = DI_SLOWTURN;

		if (! P_Move(mo, true))
		{
			mo->movedir = DI_EVASIVE;
			mo->angle = P_Random() << (ANGLEBITS - 8);
			mo->movecount = 1 + (P_Random() & 7);
		}
		return;
	}

	// make evasive maneouvres
	mo->movecount--;

	if (mo->movecount <= DI_SLOWTURN)
	{
		mo->movedir = DI_FASTTURN;
		return;
	}

	P_Move(mo, true);
}

//-------------------------------------------------------------------
//--------------------ATTACK HANDLING PROCEDURES---------------------
//-------------------------------------------------------------------

//
// P_DoAttack
//
// When an object goes on the attack, it current attack is handled here;
// the attack type is discerned and the assault is launched.
//
// -ACB- 1998/08/07
//
static void P_DoAttack(mobj_t * object)
{
	const atkdef_c *attack = object->currentattack;

	DEV_ASSERT2(attack);

	switch (attack->attackstyle)
	{
		case ATK_CLOSECOMBAT:
		{
			DoMeleeAttack(object);
			break;
		}

		case ATK_PROJECTILE:
		{
			LaunchProjectile(object, object->target, attack->atk_mobj);
			break;
		}

		case ATK_SMARTPROJECTILE:
		{
			LaunchSmartProjectile(object, object->target, attack->atk_mobj);
			break;
		}

		case ATK_RANDOMSPREAD:
		{
			LaunchRandomSpread(object);
			break;
		}

		case ATK_SHOOTTOSPOT:
		{
			ShootToSpot(object);
			break;
		}

		case ATK_SHOT:
		{
			ShotAttack(object);
			break;
		}

		case ATK_SKULLFLY:
		{
			SkullFlyAssault(object);
			break;
		}

		case ATK_SPAWNER:
		{
			ObjectSpawning(object, object->angle);
			break;
		}

		case ATK_SPREADER:
		{
			LaunchOrderedSpread(object);
			break;
		}

		case ATK_TRACKER:
		{
			LaunchTracker(object);
			break;
		}

		case ATK_TRIPLESPAWNER:
		{
			ObjectTripleSpawn(object);
			break;
		}

		// -KM- 1998/11/25 Added spray attack
		case ATK_SPRAY:
		{
			SprayAttack(object);
			break;
		}

		default:  // THIS SHOULD NOT HAPPEN
		{
#ifdef DEVELOPERS
			I_Error("P_DoAttack: %s has an unknown attack type.\n", 
				object->info->ddf.name.GetString());
#endif
			break;
		}
	}
}

//
// P_ActComboAttack
//
// This is called at end of a set of states that can result in
// either a closecombat or ranged attack. The procedure checks
// to see if the target is within melee range and picks the
// approiate attack.
//
// -ACB- 1998/08/07
//
void P_ActComboAttack(mobj_t * object)
{
	const atkdef_c *attack;

	if (!object->target)
		return;

	if (DecideMeleeAttack(object, object->info->closecombat))
		attack = object->info->closecombat;
	else
		attack = object->info->rangeattack;

	if (attack)
	{
		if (attack->flags & AF_FaceTarget)
			P_ActFaceTarget(object);

		if (attack->flags & AF_NeedSight)
		{
			if (!P_CheckSight(object, object->target))
				return;
		}

		object->currentattack = attack;
		P_DoAttack(object);
	}
#ifdef DEVELOPERS
	else
	{
		if (!object->info->closecombat)
			M_WarnError("%s hasn't got a close combat attack\n", object->info->ddf.name.GetString());
		else
			M_WarnError("%s hasn't got a range attack\n", object->info->ddf.name.GetString());
	}
#endif

}

//
// P_ActMeleeAttack
//
// Setup a close combat assault
//
// -ACB- 1998/08/07
//
void P_ActMeleeAttack(mobj_t * object)
{
	const atkdef_c *attack;

	attack = object->info->closecombat;

	// -AJA- 1999/08/10: Multiple attack support.
	if (object->state && object->state->action_par)
		attack = (const atkdef_c *) object->state->action_par;

	if (!attack)
	{
		M_WarnError("P_ActMeleeAttack: %s has no close combat attack.\n", 
					object->info->ddf.name.GetString());
		return;
	}

	if (attack->flags & AF_FaceTarget)
		P_ActFaceTarget(object);

	if (attack->flags & AF_NeedSight)
	{
		if (!object->target || !P_CheckSight(object, object->target))
			return;
	}

	object->currentattack = attack;
	P_DoAttack(object);
}

//
// P_ActRangeAttack
//
// Setup an attack at range
//
// -ACB- 1998/08/07
//
void P_ActRangeAttack(mobj_t * object)
{
	const atkdef_c *attack;

	attack = object->info->rangeattack;

	// -AJA- 1999/08/10: Multiple attack support.
	if (object->state && object->state->action_par)
		attack = (const atkdef_c *) object->state->action_par;

	if (!attack)
	{
		M_WarnError("P_ActRangeAttack: %s hasn't got a range attack.\n", 
					object->info->ddf.name.GetString());
		return;
	}

	if (attack->flags & AF_FaceTarget)
		P_ActFaceTarget(object);

	if (attack->flags & AF_NeedSight)
	{
		if (!object->target || !P_CheckSight(object, object->target))
			return;
	}

	object->currentattack = attack;
	P_DoAttack(object);
}

//
// P_ActSpareAttack
//
// Setup an attack that is not defined as close or range. can be
// used to act as a follow attack for close or range, if you want one to
// add to the others.
//
// -ACB- 1998/08/24
//
void P_ActSpareAttack(mobj_t *object)
{
	const atkdef_c *attack;

	attack = object->info->spareattack;

	// -AJA- 1999/08/10: Multiple attack support.
	if (object->state && object->state->action_par)
		attack = (const atkdef_c *) object->state->action_par;

	if (attack)
	{
		if ((attack->flags & AF_FaceTarget) && object->target)
			P_ActFaceTarget(object);

		if ((attack->flags & AF_NeedSight) && object->target)
		{
			if (!P_CheckSight(object, object->target))
				return;
		}

		object->currentattack = attack;
		P_DoAttack(object);
	}
#ifdef DEVELOPERS
	else
	{
		M_WarnError("P_ActSpareAttack: %s hasn't got a spare attack\n", object->info->ddf.name.GetString());
		return;
	}
#endif

}

//
// P_ActRefireCheck
//
// This procedure will be called inbetween firing on an object
// that will fire repeatly (Chaingunner/Arachontron etc...), the
// purpose of this is to see if the object should refire and
// performs checks to that effect, first there is a check to see
// if the object will keep firing regardless and the others
// check if the the target exists, is alive and within view. The
// only other code here is a stealth check: a object with stealth
// capabilitys will lose the ability while firing.
//
// -ACB- 1998/08/10
//
void P_ActRefireCheck(mobj_t * object)
{
	mobj_t *target;
	const atkdef_c *attack;

	attack = object->currentattack;

	if (! attack)
		return;

	if (attack->flags & AF_FaceTarget)
		P_ActFaceTarget(object);

	// Random chance that object will keep firing regardless
	if (P_RandomTest(attack->keepfirechance))
		return;

	target = object->target;

	if (!target || (target->health <= 0) || !P_CheckSight(object, target))
	{
		if (object->info->chase_state)
			P_SetMobjStateDeferred(object, object->info->chase_state, 0);
	}
	else if (object->flags & MF_STEALTH)
	{
		object->vis_target = VISIBLE;
	}
}

//
// P_ActReloadCheck
//
// Enter reload states if the monster has shot a certain number of
// shots (given by RELOAD_SHOTS command).
//
// -AJA- 2004/11/15: added this.
//
void P_ActReloadCheck(mobj_t * object)
{
	object->shot_count++;

	if (object->shot_count >= object->info->reload_shots)
	{
		object->shot_count = 0;

		if (object->info->reload_state)
			P_SetMobjStateDeferred(object, object->info->reload_state, 0);
	}
}

void P_ActReloadReset(mobj_t * object)
{
	object->shot_count = 0;
}

//---------------------------------------------
//-----------LOOKING AND CHASING---------------
//---------------------------------------------

//
// SelectTarget
//
// Search the things list for a target
//
// -ACB- 2000/06/20 Re-written and Simplified
//
static mobj_t *SelectTarget(bool newlev)
{
	static mobj_t *targetobj;
	int count;

	// Setup target object
	if (newlev)
		targetobj = mobjlisthead;

	// Nothing?
	if (!targetobj)
		return NULL;

	// Find mobj in list 
	count = P_Random();
	while (count)
	{
		if (targetobj)
			targetobj = targetobj->next;
		else
			targetobj = mobjlisthead;

		count--;
	}

	// Found end of list
	if (!targetobj)
		return NULL;

	// Not a valid obj?
	if (!(targetobj->info->extendedflags & EF_MONSTER) || targetobj->health<=0)
		return NULL;

	return targetobj;
}

//
// CreateAggression
//
// Sets an object up to target a previously stored object.
//
// -ACB- 2000/06/20 Re-written and Simplified
//
static bool CreateAggression(mobj_t * object)
{
	static const mapdef_c *mapcheck = NULL;	// FIXME!!! Lose this static sh*te!
	static mobj_t *target = NULL;
	static int count = 0;
	const mobjtype_c *targinfo;
	const mobjtype_c *objinfo;

	count++;

	// New map of no map - setup the procedure for next time
	if (mapcheck == NULL || mapcheck != currmap)
	{
		mapcheck = currmap;
		target = SelectTarget(true);
		count = 0;
		return false;
	}

	// No target or target dead
	if (target == NULL)
	{
		target = SelectTarget(false);
		count = 0;
		return false;
	}

	if (!(target->info->extendedflags & EF_MONSTER) || target->health <= 0)
	{
		target = SelectTarget(false);
		count = 0;
		return false;
	}

	// Don't target self...
	if (object == target)
		return false;

	// This object has been checked too many times, try a another one.
	if (count > 127)
	{
		target = SelectTarget(false);
		count = 0;
		return false;
	}

	objinfo = object->info;
	targinfo = target->info;

	if (!P_CheckSight(object, target))
		return false;

	if (targinfo == objinfo)
	{
		if (! (objinfo->extendedflags & EF_DISLOYALTYPE))
			return false;

		// Type the same and it can't hurt own kind - not good.
		if (! (objinfo->extendedflags & EF_OWNATTACKHURTS))
			return false;
	}

	// don't attack a friend if we cannot hurt them.
	// -AJA- I'm assuming that even friends will 'infight'.
	if ((targinfo->side & objinfo->side) != 0 && 
		(objinfo->hyperflags & (HF_SIDEIMMUNE | HF_ULTRALOYAL)))
	{
		return false;
	}

	P_MobjSetTarget(object, target);

	if (object->info->chase_state)
		P_SetMobjStateDeferred(object, object->info->chase_state, 0);

	return true;
}


//
// P_ActStandardLook
//
// Standard Lookout procedure
//
// -ACB- 1998/08/22
//
void P_ActStandardLook(mobj_t * object)
{
	int targ_pnum;
	mobj_t *targ = NULL;

	object->threshold = 0;  // any shot will wake up

	targ_pnum = object->subsector->sector->sound_player;

	if (targ_pnum >= 0 && targ_pnum < MAXPLAYERS && 
		players[targ_pnum])
	{
		targ = players[targ_pnum]->mo;
	}

	// -AJA- 2004/09/02: ignore the sound of a friend
	// FIXME: maybe wake up and support that player ??
	if (targ && (targ->side & object->side) != 0)
		targ = NULL;

	if (object->flags & MF_STEALTH)
		object->vis_target = VISIBLE;

	if (infight)
	{
		if (CreateAggression(object))
			return;
	}

	if (targ && (targ->flags & MF_SHOOTABLE))
	{
		P_MobjSetTarget(object, targ);

		if (object->flags & MF_AMBUSH)
		{
			if (!P_CheckSight(object, object->target) && 
				!P_LookForPlayers(object, object->info->sight_angle))
				return;
		}
	}
	else
	{
		if (!P_LookForPlayers(object, object->info->sight_angle))
			return;
	}

	if (object->info->seesound)
	{
		S_StartFX(object->info->seesound, P_MobjGetSfxCategory(object),
					   object, SfxFlags(object->info));
	}

	// -AJA- this will remove objects which have no chase states.
        // For compatibility with original DOOM.
	P_SetMobjStateDeferred(object, object->info->chase_state, 0);
}

//
// P_ActPlayerSupportLook
//
// Player Support Lookout procedure
//
// -ACB- 1998/09/05
//
void P_ActPlayerSupportLook(mobj_t * object)
{
	object->threshold = 0;  // any shot will wake up

	if (object->flags & MF_STEALTH)
		object->vis_target = VISIBLE;

	if (!object->supportobj)
	{
		if (! P_ActLookForTargets(object))
			return;

		// -AJA- 2004/09/02: join the player's side
		if (object->side == 0)
			object->side = object->target->side;

		if (object->info->seesound)
		{
			S_StartFX(object->info->seesound, P_MobjGetSfxCategory(object),
						   object, SfxFlags(object->info));
		}
	}

	if (object->info->meander_state)
		P_SetMobjStateDeferred(object, object->info->meander_state, 0);
}

//
// P_ActStandardMeander
//
void P_ActStandardMeander(mobj_t * object)
{
	int delta;

	object->threshold = 0;  // any shot will wake up

	// move within supporting distance of player
	if (--object->movecount < 0 || !P_Move(object, false))
		P_NewChaseDir(object);

	// turn towards movement direction if not there yet
	if (object->movedir < 8)
	{
		object->angle &= (7 << 29);
		delta = object->angle - (object->movedir << 29);

		if (delta > 0)
			object->angle -= ANG45;
		else if (delta < 0)
			object->angle += ANG45;
	}
}

//
// P_ActPlayerSupportMeander
//
void P_ActPlayerSupportMeander(mobj_t * object)
{
	int delta;

	object->threshold = 0;  // any shot will wake up

	// move within supporting distance of player
	if (--object->movecount < 0 || !P_Move(object, false))
		P_NewChaseDir(object);

	// turn towards movement direction if not there yet
	if (object->movedir < 8)
	{
		object->angle &= (7 << 29);
		delta = object->angle - (object->movedir << 29);

		if (delta > 0)
			object->angle -= ANG45;
		else if (delta < 0)
			object->angle += ANG45;
	}

	//
	// we have now meandered, now check for a support object, if we don't
	// look for one and return; else look for targets to take out, if we
	// find one, go for the chase.
	//
	/*  if (!object->supportobj)
		{
		P_ActPlayerSupportLook(object);
		return;
		} */

	P_ActLookForTargets(object);
}

//
// P_ActStandardChase
//
// Standard AI Chase Procedure
//
// -ACB- 1998/08/22 Procedure Written
// -ACB- 1998/09/05 Added Support Object Check
//
void P_ActStandardChase(mobj_t * object)
{
	int delta;
	sfx_t *sound;

	if (object->reactiontime)
		object->reactiontime--;

	// object has a pain threshold, while this is true, reduce it. while
	// the threshold is true, the object will remain intent on its target.
	if (object->threshold)
	{
		if (!object->target || object->target->health <= 0)
			object->threshold = 0;
		else
			object->threshold--;
	}

	// A Chasing Stealth Creature becomes less visible
	if (object->flags & MF_STEALTH)
		object->vis_target = INVISIBLE;

	// turn towards movement direction if not there yet
	if (object->movedir < 8)
	{
		object->angle &= (7 << 29);
		delta = object->angle - (object->movedir << 29);

		if (delta > 0)
			object->angle -= ANG45;
		else if (delta < 0)
			object->angle += ANG45;
	}

	if (!object->target || !(object->target->flags & MF_SHOOTABLE))
	{
		if (P_ActLookForTargets(object))
			return;

		// -ACB- 1998/09/06 Target is not relevant: NULLify.
		P_MobjSetTarget(object, NULL);

		P_SetMobjStateDeferred(object, object->info->idle_state, 0);
		return;
	}

	// do not attack twice in a row
	if (object->flags & MF_JUSTATTACKED)
	{
		object->flags &= ~MF_JUSTATTACKED;

		// -KM- 1998/12/16 Nightmare mode set the fast parm.
		if (!level_flags.fastparm)
			P_NewChaseDir(object);

		return;
	}

	sound = object->info->attacksound;

	// check for melee attack
	if (object->info->melee_state && DecideMeleeAttack(object, object->info->closecombat))
	{
		if (sound)
			S_StartFX(sound, P_MobjGetSfxCategory(object), object);

		if (object->info->melee_state)
			P_SetMobjStateDeferred(object, object->info->melee_state, 0);
		return;
	}

	// check for missile attack
	if (object->info->missile_state)
	{
		// -KM- 1998/12/16 Nightmare set the fastparm.
		if (!(!level_flags.fastparm && object->movecount))
		{
			if (DecideRangeAttack(object))
			{
				if (object->info->missile_state)
					P_SetMobjStateDeferred(object, object->info->missile_state, 0);
				object->flags |= MF_JUSTATTACKED;
				return;
			}
		}
	}

	// possibly choose another target
	// -ACB- 1998/09/05 Object->support->object check, go for new targets
	if (!P_CheckSight(object, object->target) && !object->threshold)
	{
		if (P_ActLookForTargets(object))
			return;
	}

	// chase towards player
	if (--object->movecount < 0 || !P_Move(object, false))
		P_NewChaseDir(object);

	// make active sound
	if (object->info->activesound && M_Random() < 3)
		S_StartFX(object->info->activesound, P_MobjGetSfxCategory(object), object);
}

//
// P_ActResurrectChase
//
// Before undertaking the standard chase procedure, the object
// will check for a nearby corpse and raises one if it exists.
//
// -ACB- 1998/09/05 Support Check: Raised object supports raiser's supportobj
//
void P_ActResurrectChase(mobj_t * object)
{
	mobj_t *corpse;

	corpse = P_MapFindCorpse(object);

	if (corpse)
	{
		object->angle = R_PointToAngle(object->x, object->y, corpse->x, corpse->y);
		if (object->info->res_state)
			P_SetMobjStateDeferred(object, object->info->res_state, 0);

		// corpses without raise states should be skipped
		DEV_ASSERT2(corpse->info->raise_state);

		P_BringCorpseToLife(corpse);

		// -ACB- 1998/09/05 Support Check: Res creatures to support that object
		if (object->supportobj)
		{
			P_MobjSetSupportObj(corpse, object->supportobj);
			P_MobjSetTarget(corpse, object->target);
		}
		else
		{
			P_MobjSetSupportObj(corpse, NULL);
			P_MobjSetTarget(corpse, NULL);
		}

		// -AJA- Resurrected creatures are on Archvile's side (like MBF)
		corpse->side = object->side;
		return;
	}

	P_ActStandardChase(object);
}

//
// P_ActWalkSoundChase
//
// Make a sound and then chase...
//
void P_ActWalkSoundChase(mobj_t * object)
{
	if (!object->info->walksound)
	{
		M_WarnError("WALKSOUND_CHASE: %s hasn't got a walksound.\n", 
					object->info->ddf.name.GetString());
		return;
	}

	S_StartFX(object->info->walksound, P_MobjGetSfxCategory(object), object);
	P_ActStandardChase(object);
}

//
// P_ActDie
//
// Boom/MBF compatibility.
//
void P_ActDie(mobj_t * mo)
{
	P_DamageMobj(mo, NULL, NULL, mo->health, NULL);
}

//
// P_ActKeenDie
//
void P_ActKeenDie(mobj_t * mo)
{
	P_ActMakeIntoCorpse(mo);

	// see if all other Keens are dead
	for (mobj_t *cur = mobjlisthead; cur != NULL; cur = cur->next)
	{
		if (cur == mo)
			continue;

		if (cur->info != mo->info)
			continue;

		if (cur->health > 0)
			return; // other Keen not dead
	}

  L_WriteDebug("P_ActKeenDie: ALL DEAD, activating...\n");

	P_RemoteActivation(NULL, 2 /* door type */, 666 /* tag */, 0, line_Any);
}

//
// P_ActCheckMoving
//
// -KM- 1999/01/31 Returns a player to spawnstate when not moving.
//
void P_ActCheckMoving(mobj_t * mo)
{
	player_t *p = mo->player;

	if (fabs(mo->mom.x) < STOPSPEED && fabs(mo->mom.y) < STOPSPEED)
	{
		if (p)
		{
			if (p->cmd.forwardmove || p->cmd.sidemove)
				return;
		}
		mo->mom.x = mo->mom.y = 0;

		P_SetMobjStateDeferred(mo, mo->info->idle_state, 0);
	}
}

//
// P_ActCheckBlood
//
// -KM- 1999/01/31 Part of the extra blood option, makes blood stick around...
// -AJA- 1999/10/02: ...but not indefinitely.
//
void P_ActCheckBlood(mobj_t * mo)
{
	if (level_flags.more_blood && mo->tics >= 0)
	{
		int val = P_Random();

		// exponential formula
		mo->tics = ((val * val * val) >> 18) * TICRATE + TICRATE;
	}
}

//
// P_ActJump
//
// Jumps to the given label, possibly randomly.  Note: nothing to do
// with monsters physically jumping.
//
void P_ActJump(mobj_t * object)
{
	act_jump_info_t *jump;
  
	if (!object->state || !object->state->action_par)
	{
		M_WarnError("JUMP action used in [%s] without a label !\n",
					object->info->ddf.name.GetString());
		return;
	}

	jump = (act_jump_info_t *) object->state->action_par;

	DEV_ASSERT2(jump->chance >= 0);
	DEV_ASSERT2(jump->chance <= 1);

	if (P_RandomTest(jump->chance))
	{
		object->next_state = (object->state->jumpstate == S_NULL) ?
			NULL : (states + object->state->jumpstate);
	}
}

//
// P_PlayerAttack
//
// -AJA- 1999/08/08: New attack flag FORCEAIM, which fixes chainsaw.
//
void P_PlayerAttack(mobj_t * p_obj, const atkdef_c * attack)
{
	DEV_ASSERT2(attack);

	p_obj->currentattack = attack;

	float range = (attack->range > 0) ? attack->range : MISSILERANGE;

	// see which target is to be aimed at
	mobj_t *target = P_MapTargetAutoAim(p_obj, p_obj->angle, range, 
				 (attack->flags & AF_ForceAim) ? true : false);

	P_MobjSetTarget(p_obj, target);

	if (attack->flags & AF_FaceTarget)
		P_ActFaceTarget(p_obj);

	P_DoAttack(p_obj);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
