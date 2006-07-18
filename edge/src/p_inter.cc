//----------------------------------------------------------------------------
//  EDGE Interactions (picking up items etc..) Code
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

#include "i_defs.h"

#include "am_map.h"
#include "con_main.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "dstrings.h"
#include "m_random.h"
#include "p_local.h"
#include "s_sound.h"
#include "st_stuff.h"

#define BONUS_ADD    6
#define BONUS_LIMIT  100

#define DAMAGE_ADD_MIN  3
#define DAMAGE_LIMIT  100


//
// P_GiveAmmo
//
// Returns false if the ammo can't be picked up at all
//
// -ACB- 1998/06/19 DDF Change: Number passed is the exact amount of ammo given.
// -KM- 1998/11/25 Handles weapon change from priority.
//
static bool GiveAmmo(player_t * player, mobj_t * special,
					 benefit_t *be, bool lose_em, int *new_weap, int *new_ammo)
{
	int ammo  = be->sub.type;  
	int num   = (int)(be->amount + 0.5f);

	if (ammo == AM_NoAmmo || num <= 0)
		return false;

	if (ammo < 0 || ammo >= NUMAMMO)
		I_Error("GiveAmmo: bad type %i", ammo);

	if (lose_em)
	{
		if (player->ammo[ammo].num == 0)
			return false;

		player->ammo[ammo].num -= num;

		if (player->ammo[ammo].num < 0)
			player->ammo[ammo].num = 0;

		return true;
	}

	// In Nightmare you need the extra ammo, in "baby" you are given double
	if (special)
	{
		if ((gameskill == sk_baby) || (gameskill == sk_nightmare))
			num <<= 1;
	}

	bool pickup = false;

	// for newly acquired weapons (in the same benefit list) which have
	// a clip, try to "bundle" this ammo inside that clip.  
	if (new_weap != NULL && *new_weap >= 0)
	{
		pickup = pickup || P_TryFillNewWeapon(player, *new_weap,
			(ammotype_e)ammo, &num);

		if (num == 0)
			return true;
	}

	// divide by two here, means that the ammo for filling a clip
	// weapons is not affected by the MF_DROPPED flag.
	if (num > 1 && special && (special->flags & MF_DROPPED))
		num /= 2;

	if (player->ammo[ammo].num == player->ammo[ammo].max)
		return pickup;

	// if there is some fresh ammo, we should change weapons 
	if (new_ammo != NULL && player->ammo[ammo].num == 0)
		(*new_ammo) = ammo;

	player->ammo[ammo].num += num;

	if (player->ammo[ammo].num > player->ammo[ammo].max)
		player->ammo[ammo].num = player->ammo[ammo].max;

	return true;
}

//
// GiveAmmoLimit
//
static bool GiveAmmoLimit(player_t * player, mobj_t * special,
						  benefit_t *be, bool lose_em)
{
	int ammo  = be->sub.type;  
	int limit = (int)(be->amount + 0.5f);

	if (ammo == AM_NoAmmo)
		return false;

	if (ammo < 0 || ammo >= NUMAMMO)
		I_Error("GiveAmmoLimit: bad type %i", ammo);

	if ((!lose_em && limit < player->ammo[ammo].max) ||
		( lose_em && limit > player->ammo[ammo].max))
	{
		return false;
	}

	player->ammo[ammo].max = limit;
	return true;
}

//
// GiveWeapon
//
// The weapon thing may have a MF_DROPPED flag or'ed in.
//
// -AJA- 2000/03/02: Reworked for new benefit_t stuff.
//
static bool GiveWeapon(player_t * player, mobj_t * special,
					   benefit_t *be, bool lose_em, int *new_weap)
{
	weapondef_c *info = be->sub.weap;
	int pw_index;

	DEV_ASSERT2(info);

	if (lose_em)
		return P_RemoveWeapon(player, info);

	if (! P_AddWeapon(player, info, &pw_index))
		return false;

	if (new_weap != NULL)
		(*new_weap) = pw_index;

	return true;
}

//
// GiveHealth
//
// Returns false if not health is not needed,
//
// New Procedure: -ACB- 1998/06/21
//
static bool GiveHealth(player_t * player, mobj_t * special,
					   benefit_t *be, bool lose_em)
{
	if (lose_em)
	{
		P_DamageMobj(player->mo, special, NULL, be->amount, NULL);
		return true;
	}

	if (player->health >= be->limit)
		return false;

	player->health += be->amount;

	if (player->health > be->limit)
		player->health = be->limit;

	player->mo->health = player->health;

	return true;
}

//
// GiveArmour
//
// Returns false if the new armour would not benefit
//
static bool GiveArmour(player_t * player, mobj_t * special,
					   benefit_t *be, bool lose_em)
{
	armour_type_e a_class = (armour_type_e)be->sub.type;

	DEV_ASSERT2(0 <= a_class && a_class < NUMARMOUR);

	if (lose_em)
	{
		if (player->armours[a_class] == 0)
			return false;

		player->armours[a_class] -= be->amount;
		if (player->armours[a_class] < 0)
			player->armours[a_class] = 0;

		P_UpdateTotalArmour(player);
		return true;
	}

	float amount  = be->amount;
	float upgrade = 0;
	float slack;

	if (false)  // if (! Doom_Compat_Armour)
	{
		slack = be->limit - player->armours[a_class];

		if (amount > slack)
			amount = slack;
		
		if (amount <= 0)
			return false;
	}
	else
	{
		slack = be->limit - player->totalarmour;

		if (slack < 0)
			return false;

		// we try to Upgrade any lower class armour with this armour.
		for (int cl = a_class - 1; cl >= 0; cl--)
		{
			upgrade += player->armours[cl];
		}

		// cannot upgrade more than the specified amount
		if (upgrade > amount)
			upgrade = amount;

		slack += upgrade;

		if (amount > slack)
			amount = slack;
		
		DEV_ASSERT2(amount  >= 0);
		DEV_ASSERT2(upgrade >= 0);

		if (amount == 0 && upgrade == 0)
			return false;
	}

	player->armours[a_class] += amount;

	if (upgrade > 0)
	{
		for (int cl = a_class - 1; (cl >= 0) && (upgrade > 0); cl--)
		{
			if (player->armours[cl] >= upgrade)
			{
				player->armours[cl] -= upgrade;
				break;
			}
			else if (player->armours[cl] > 0)
			{
				upgrade -= player->armours[cl];
				player->armours[cl] = 0;
			}
		}
	}

	P_UpdateTotalArmour(player);
	return true;
}

//
// GiveKey
//
static bool GiveKey(player_t * player, mobj_t * special, benefit_t *be, bool lose_em)
{
	keys_e key = (keys_e)be->sub.type;

	if (lose_em)
	{
		if (! (player->cards & key))
			return false;

		player->cards = (keys_e)(player->cards & ~key);
	}
	else
	{
		if (player->cards & key)
			return false;

		player->cards = (keys_e)(player->cards | key);
	}

	return true;
}

//
// GivePower
//
// DDF Change: duration is now passed as a parameter, for the berserker
//             the value is the health given, extendedflags also passed.
//
// The code was changes to a switch instead of a series of if's, also
// included is the use of limit, which gives a maxmium amount of protection
// for this item. -ACB- 1998/06/20
//
static bool GivePower(player_t * player, mobj_t * special,
					  benefit_t *be, bool lose_em)
{ 
	// -ACB- 1998/06/20 - calculate duration in seconds
	float duration = be->amount * TICRATE;
	float limit    = be->limit  * TICRATE;

	if (lose_em)
	{
		if (player->powers[be->sub.type] == 0)
			return false;

		player->powers[be->sub.type] -= duration;

		if (player->powers[be->sub.type] < 0)
			player->powers[be->sub.type] = 0;

		return true;
	}

	if (player->powers[be->sub.type] >= limit)
		return false;

	player->powers[be->sub.type] += duration;

	if (player->powers[be->sub.type] > limit)
		player->powers[be->sub.type] = limit;

	// special handling for scuba...
	if (be->sub.type == PW_Scuba)
	{
		player->air_in_lungs = player->mo->info->lung_capacity;
	}

	return true;
}

//
// P_GiveBenefitList
//
// Give all the benefits in the list to the player.  `special' is the
// special object that all these benefits came from, or NULL if they
// came from the initial_benefits list.  When `lose_em' is true, the
// benefits should be taken away instead.  Returns true if _any_
// benefit was picked up (or lost), or false if none of them were.
//
bool P_GiveBenefitList(player_t *player, mobj_t * special, 
					   benefit_t *list, bool lose_em,
					   int *new_weap, int *new_ammo)
{
	bool pickup = false;
	// is it a weapon that will stay in old deathmatch?
	bool dm_weapon = false;

	// FIXME: leave placed weapons forever in old deathmatch mode,
	//        but only if we haven't already picked it up.

	for (; list; list=list->next)
	{
		if (list->type == BENEFIT_None)
			continue;

		// Put the checking in for neg amounts at benefit level. Powerups can be neg 
		// if they last all level. -ACB- 2004/02/04

		switch (list->type)
		{
			case BENEFIT_Ammo:
				if (list->amount >= 0.0)
					pickup |= GiveAmmo(player, special, list, lose_em,
						new_weap, new_ammo);
				break;

			case BENEFIT_AmmoLimit:
				if (list->amount >= 0.0)
					pickup |= GiveAmmoLimit(player, special, list, lose_em);
				break;

			case BENEFIT_Weapon:
				if (list->amount >= 0.0)
					pickup |= GiveWeapon(player, special, list, lose_em, new_weap);
				break;

			case BENEFIT_Key:
				if (list->amount >= 0.0)
					pickup |= GiveKey(player, special, list, lose_em);
				break;

			case BENEFIT_Health:
				if (list->amount >= 0.0)
					pickup |= GiveHealth(player, special, list, lose_em);
				break;

			case BENEFIT_Armour:
				if (list->amount >= 0.0)
					pickup |= GiveArmour(player, special, list, lose_em);
				break;

			case BENEFIT_Powerup:
				pickup |= GivePower(player, special, list, lose_em);
				break;

			default:
				break;
		}
	}

	return dm_weapon ? false : pickup;
}

//
// RunPickupEffects
//
static void RunPickupEffects(player_t *player, mobj_t *special, 
		pickup_effect_c *list)
{
	for (; list; list=list->next)
	{
		switch (list->type)
		{
			case PUFX_SwitchWeapon:
				P_PlayerSwitchWeapon(player, list->sub.weap);
				break;

			case PUFX_KeepPowerup:
				player->keep_powers |= (1 << list->sub.type);
				break;

			case PUFX_PowerupEffect:
				// FIXME
				break;

			case PUFX_ScreenEffect:
				// FIXME
				break;
			
			default: break;
		}
	}
}

static bool SpecialIsKey(mobj_t *special)
{
	for (benefit_t *B = special->info->pickup_benefits; B != NULL; B = B->next)
	{
		if (B->type == BENEFIT_Key)
			return true;
	}

	return false;
}

//
// P_TouchSpecialThing
//
// -KM- 1999/01/31 Things that give you item bonus are always
//  picked up.  Picked up object is set to death frame instead
//  of removed so that effects can happen.
//
void P_TouchSpecialThing(mobj_t * special, mobj_t * toucher)
{
	player_t *player;
	float delta;
	sfx_t *sound;
	bool pickup = false;

	delta = special->z - toucher->z;

	// out of reach
	if (delta > toucher->height || delta < -special->height)
		return;

	player = toucher->player;

	if (! player)
		return;

	// Dead thing touching. Can happen with a sliding player corpse.
	if (toucher->health <= 0)
		return;

	// -KM- 1998/09/27 Sounds.ddf
	sound = special->info->activesound;

	if (special->hyperflags & HF_FORCEPICKUP)
		pickup |= true;

	int new_weap = -1;  // the most recently added weapon (must be new)
	int new_ammo = -1;  // got fresh ammo (old count was zero).

	// First check for lost benefits
	pickup |= P_GiveBenefitList(player, special, 
		special->info->lose_benefits, true);

	// Run through the list of all pickup benefits...
	pickup |= P_GiveBenefitList(player, special, 
		special->info->pickup_benefits, false, &new_weap, &new_ammo);

	if (special->flags & MF_COUNTITEM)
	{
		player->itemcount++;
		pickup = true;
	}

	if (pickup)
	{
		// leave keys in COOP games (FIXME !!!! Temporary hack)
		if (COOP_MATCH() &&
			! (special->hyperflags & HF_FORCEPICKUP) &&
			SpecialIsKey(special))
		{
			/* keep it */
		}
		else
		{
			special->health = 0;
			P_KillMobj(player->mo, special, NULL);
		}

		player->bonuscount += BONUS_ADD;
		if (player->bonuscount > BONUS_LIMIT)
			player->bonuscount = BONUS_LIMIT;

		// -AJA- FIXME: OPTIMISE THIS!
		if (special->info->pickup_message &&
			language.IsValidRef(special->info->pickup_message))
		{
			CON_PlayerMessage(player->pnum, 
				language[special->info->pickup_message]);
		}

		if (sound)
        {
            int sfx_cat;

            if (player == players[consoleplayer])
                sfx_cat = SNCAT_ConPlayer;
            else
                sfx_cat = SNCAT_OtherPlayer;

			sound::StartFX(sound, sfx_cat, player->mo);
        }

		if (new_weap >= 0 || new_ammo >= 0)
			P_TrySwitchNewWeapon(player, new_weap, (ammotype_e)new_ammo);

		RunPickupEffects(player, special, special->info->pickup_effects);
	}
}

//
// P_KillMobj
//
// Altered to reflect the fact that the dropped item is a pointer to
// mobjtype_c, uses new procedure: P_MobjCreateObject.
//
// Note: Damtype can be NULL here.
//
// -ACB- 1998/08/01
//
// -AJA- 1999/09/12: Now uses P_SetMobjStateDeferred, since this
//       routine can be called by TryMove/PIT_CheckRelThing/etc.
//
void P_KillMobj(mobj_t * source, mobj_t * target, const damage_c *damtype)
{
	const mobjtype_c *item;
	statenum_t state;
	bool overkill;

	target->flags &= ~(MF_SPECIAL | MF_SHOOTABLE | MF_FLOAT | 
		MF_SKULLFLY | MF_TOUCHY);
	target->extendedflags &= ~(EF_BOUNCE | EF_USABLE | EF_CLIMBABLE);

	if (!(target->extendedflags & EF_NOGRAVKILL))
		target->flags &= ~MF_NOGRAVITY;

	target->flags |= MF_CORPSE | MF_DROPOFF;
	target->height /= 4;

	if (source && source->player)
	{
		// count for intermission
		if (target->flags & MF_COUNTKILL)
			source->player->killcount++;

		if (target->player)
		{
			// Killed a team mate?
			if (target->side & source->side)
			{
				source->player->frags--;
				source->player->totalfrags--;
			}
			else
			{
				source->player->frags++;
				source->player->totalfrags++;
			}
		}
	}
	else if (SP_MATCH() && (target->flags & MF_COUNTKILL))
	{
		// count all monster deaths,
		// even those caused by other monsters
		players[consoleplayer]->killcount++;
	}

	if (target->player)
	{
		// count environment kills against you
		if (!source)
		{
			target->player->frags--;
			target->player->totalfrags--;
		}

		target->flags &= ~MF_SOLID;
		target->player->playerstate = PST_DEAD;
		target->player->std_viewheight = MIN(DEATHVIEWHEIGHT,
			target->height / 3);

		P_DropWeapon(target->player);

		// don't die in auto map, switch view prior to dying
		if (target->player == players[consoleplayer] && automapactive)
			AM_Stop();
	}

	state = S_NULL;
	overkill = (target->health < -target->info->spawnhealth);

	if (overkill && damtype && damtype->overkill.label)
	{
		state = P_MobjFindLabel(target, damtype->overkill.label);
		if (state != S_NULL)
			state += damtype->overkill.offset;
	}

	if (state == S_NULL && overkill && target->info->overkill_state)
		state = target->info->overkill_state;

	if (state == S_NULL && damtype && damtype->death.label)
	{
		state = P_MobjFindLabel(target, damtype->death.label);
		if (state != S_NULL)
			state += damtype->death.offset;
	}

	if (state == S_NULL)
		state = target->info->death_state;

	P_SetMobjStateDeferred(target, state, P_Random() & 3);

	// Drop stuff. This determines the kind of object spawned
	// during the death frame of a thing.
	item = target->info->dropitem;

	if (item)
	{
		mobj_t *mo = P_MobjCreateObject(target->x, target->y,
			target->floorz, item);

		// -ES- 1998/07/18 NULL check to prevent crashing
		if (mo)
			mo->flags |= MF_DROPPED;
	}
}

//
// P_ThrustMobj
//
// Like P_DamageMobj, but only pushes the target object around
// (doesn't inflict any damage).  Parameters are:
//
// * target    - mobj to be thrust.
// * inflictor - mobj causing the thrusting.
// * thrust    - amount of thrust done (same values as damage).  Can
//               be negative to "pull" instead of push.
//
// -AJA- 1999/11/06: Wrote this routine.
//
void P_ThrustMobj(mobj_t * target, mobj_t * inflictor, float thrust)
{
	// check for immunity against the attack
	if (inflictor && inflictor->currentattack && BITSET_EMPTY ==
		(inflictor->currentattack->attack_class & ~target->info->immunity))
	{
		return;
	}

	float dx = target->x - inflictor->x;
	float dy = target->y - inflictor->y;

	// don't thrust if at the same location (no angle)
	if (fabs(dx) < 1.0f && fabs(dy) < 1.0f)
		return;

	angle_t angle = R_PointToAngle(0, 0, dx, dy);

	// -ACB- 2000/03/11 Div-by-zero check...
	CHECKVAL(target->info->mass);

	float push = 12.0f * thrust / target->info->mass;

	// limit thrust to reasonable values
	if (push < -40.0f) push = -40.0f;
	if (push >  40.0f) push =  40.0f;

	target->mom.x += push * M_Cos(angle);
	target->mom.y += push * M_Sin(angle);

	if (level_flags.true3dgameplay)
	{
		float dz = MO_MIDZ(target) - MO_MIDZ(inflictor);
		float slope = P_ApproxSlope(dx, dy, dz);

		target->mom.z += push * slope / 2;
	}
}

//
// P_DamageMobj
//
// Damages both enemies and players, decreases the amount of health
// an mobj has and "kills" an mobj in the event of health being 0 or
// less, the parameters are:
//
// * Target    - mobj to be damaged.
// * Inflictor - mobj which is causing the damage.
// * Source    - mobj who is responsible for doing the damage. Can be NULL
// * Amount    - amount of damage done.
// * Damtype   - type of damage (for override states).  Can be NULL
//
// Both source and inflictor can be NULL, slime damage and barrel
// explosions etc....
//
// -AJA- 1999/09/12: Now uses P_SetMobjStateDeferred, since this
//       routine can be called by TryMove/PIT_CheckRelThing/etc.
//
void P_DamageMobj(mobj_t * target, mobj_t * inflictor, 
				  mobj_t * source, float damage, const damage_c * damtype)
{
	player_t *player;
	statenum_t state;
	float saved = 0;
	int i;

	if (!(target->flags & MF_SHOOTABLE))
		return;

	if (target->health <= 0)
		return;

	// check for immunity against the attack
	if (inflictor && inflictor->currentattack && BITSET_EMPTY ==
		(inflictor->currentattack->attack_class & ~target->info->immunity))
	{
		return;
	}

	// check for partial resistance against the attack
	if (damage >= 0.1f && inflictor && inflictor->currentattack &&
		BITSET_EMPTY == (inflictor->currentattack->attack_class & ~target->info->resistance))
	{
		damage = (damage + 0.1f) * 0.4f;
	}

	// -ACB- 1998/07/12 Use Visibility Enum
	// A Damaged Stealth Creature becomes more visible
	if (target->flags & MF_STEALTH)
		target->vis_target = VISIBLE;

	if (target->flags & MF_SKULLFLY)
	{
		target->mom.x = target->mom.y = target->mom.z = 0;
		target->flags &= ~MF_SKULLFLY;
	}

	player = target->player;

	// take half damage in trainer mode
	if (player && gameskill == sk_baby)
		damage *= 0.5f;

	// Some close combat weapons should not
	// inflict thrust and push the victim out of reach,
	// thus kick away unless using the chainsaw.

	if (inflictor && !(target->flags & MF_NOCLIP) &&
		!(source && source->player && source->player->ready_wp >= 0 &&
		source->player->weapons[source->player->ready_wp].info->nothrust))
	{
		// make fall forwards sometimes
		if (damage < 40 && damage > target->health &&
			target->z - inflictor->z > 64 && (P_Random() & 1))
		{
			P_ThrustMobj(target, inflictor, -damage * 4);
		}
		else
			P_ThrustMobj(target, inflictor, damage);
	}

	// player specific
	if (player)
	{
		// ignore damage in GOD mode, or with INVUL powerup
		if ((player->cheats & CF_GODMODE) || player->powers[PW_Invulnerable])
			return;

		// check which armour can take some damage
		for (i=ARMOUR_Red; i >= ARMOUR_Green; i--)
		{
			if (damtype && damtype->no_armour)
				continue;

			if (player->armours[i] <= 0)
				continue;

			switch (i)
			{
				case ARMOUR_Green:  saved = damage * 0.33f; break;
				case ARMOUR_Blue:   saved = damage * 0.50f; break;
				case ARMOUR_Yellow: saved = damage * 0.75f; break;
				case ARMOUR_Red:    saved = damage * 0.90f; break;
				default: 
					I_Error("INTERNAL ERROR in P_DamageMobj: bad armour %d\n", i);
			}

			if (player->armours[i] <= saved)
			{
				// armour is used up
				saved = player->armours[i];
			}

			player->armours[i] -= saved;
			damage -= saved;

			// don't apply inner armour unless outer is finished
			if (player->armours[i] > 0)
				break;
		}

		P_UpdateTotalArmour(player);

		// mirror mobj health here for Dave
		player->health -= damage;

		if (player->health < 0)
			player->health = 0;

		player->attacker = source;

		// add damage after armour / invuln detection
		if (damage > 0)
			player->damagecount += (int)MAX(damage, DAMAGE_ADD_MIN);

		// teleport stomp does 10k points...
		if (player->damagecount > DAMAGE_LIMIT)
			player->damagecount = DAMAGE_LIMIT;
	}

	// do the damage
	target->health -= damage;

	if (target->health <= 0)
	{
		P_KillMobj(source, target, damtype);
		return;
	}

	// enter pain states
	if (!(target->flags & MF_SKULLFLY) && P_RandomTest(target->info->painchance))
	{
		// setup to hit back
		target->flags |= MF_JUSTHIT;

		state = S_NULL;

		if (damtype && damtype->pain.label)
		{
			state = P_MobjFindLabel(target, damtype->pain.label);
			if (state != S_NULL)
				state += damtype->pain.offset;
		}

		if (state == S_NULL)
			state = target->info->pain_state;

		if (state != S_NULL)
			P_SetMobjStateDeferred(target, state, 0);
	}

	// we're awake now...
	target->reactiontime = 0;

	bool ultra_loyal = (source && (target->hyperflags & HF_ULTRALOYAL) &&
		(source->side & target->side) != 0);

	if ((!target->threshold || target->extendedflags & EF_NOGRUDGE) &&
		source && source != target && (!(source->extendedflags & EF_NEVERTARGET)) &&
		! ultra_loyal)
	{
		// if not intent on another player, chase after this one
		P_MobjSetTarget(target, source);
		target->threshold = BASETHRESHOLD;

		if (target->state == &states[target->info->idle_state] &&
			target->info->chase_state)
		{
			P_SetMobjStateDeferred(target, target->info->chase_state, 0);
		}
	}
}

//
// P_TelefragMobj
//
// For killing monsters and players when something teleports on top
// of them.  Even the invulnerability powerup doesn't stop it.  Also
// used for the kill-all cheat.  Inflictor and damtype can be NULL.
//
void P_TelefragMobj(mobj_t * target, mobj_t * inflictor, const damage_c * damtype)
{
	if (target->health <= 0)
		return;
	
	target->health = -1000;

	if (target->flags & MF_STEALTH)
		target->vis_target = VISIBLE;

	if (target->flags & MF_SKULLFLY)
	{
		target->mom.x = target->mom.y = target->mom.z = 0;
		target->flags &= ~MF_SKULLFLY;
	}

	if (target->player)
	{
		target->player->attacker = inflictor;
		target->player->damagecount = DAMAGE_LIMIT;
	}

	P_KillMobj(inflictor, target, damtype);
}
