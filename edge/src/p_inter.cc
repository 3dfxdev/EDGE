//----------------------------------------------------------------------------
//  EDGE Interactions (picking up items etc..) Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2001  The EDGE Team.
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
#include "z_zone.h"

#define BONUS_ADD    6
#define BONUS_LIMIT  100

#define DAMAGE_ADD_MIN  3
#define DAMAGE_LIMIT  100

#define GRIN_TIME   (TICRATE * 2)


//
// P_GiveAmmo
//
// Returns false if the ammo can't be picked up at all
//
// -ACB- 1998/06/19 DDF Change: Number passed is the exact amount of ammo given.
// -KM- 1998/11/25 Handles weapon change from priority.
//
static boolean_t GiveAmmo(player_t * player, mobj_t * special,
    benefit_t *be, boolean_t lose_em)
{
  int dropped = (special && (special->flags & MF_DROPPED));

  int ammo  = be->subtype;  
  int num   = floor(be->amount) / (dropped ? 2 : 1);

  boolean_t change_weap;
  int priority = -100;

  if (ammo == AM_NoAmmo || num <= 0)
    return false;

  if (ammo < 0 || ammo >= NUMAMMO)
    I_Error("GiveAmmo: bad type %i", ammo);

  if (lose_em)
  {
    player->ammo[ammo].num -= num;
    if (player->ammo[ammo].num < 0)
      player->ammo[ammo].num = 0;
    return true;
  }
  
  if (player->ammo[ammo].num == player->ammo[ammo].max)
    return false;

  // In Nightmare you need the extra ammo, in "baby" you are given double
  if (special)
  {
    if ((gameskill == sk_baby) || (gameskill == sk_nightmare))
      num <<= 1;
  }

  // if there was some old ammo, we don't need to change weapons 
  change_weap = (player->ammo[ammo].num == 0);

  player->ammo[ammo].num += num;

  if (player->ammo[ammo].num > player->ammo[ammo].max)
    player->ammo[ammo].num = player->ammo[ammo].max;

  if (! change_weap)
    return true;

  // We were down to zero, so select a new weapon.
  // Choose the next highest priority weapon than the current one.
  // Don't override any weapon change already underway.

  P_RefillClips(player);

  if (player->pending_wp != WPSEL_NoChange)
    return true;

  if (player->ready_wp >= 0)
    priority = player->weapons[player->ready_wp].info->priority;

  P_SelectNewWeapon(player, priority, (ammotype_e) ammo);
  return true;
}

//
// GiveAmmoLimit
//
static boolean_t GiveAmmoLimit(player_t * player, mobj_t * special,
    benefit_t *be, boolean_t lose_em)
{
  int ammo  = be->subtype;  
  int limit = floor(be->amount);

  if (ammo == AM_NoAmmo)
    return false;

  if (ammo < 0 || ammo >= NUMAMMO)
    I_Error("GiveAmmoLimit: bad type %i", ammo);

  if ((!lose_em && limit < player->ammo[ammo].max) ||
      (lose_em && limit > player->ammo[ammo].max))
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
static boolean_t GiveWeapon(player_t * player, mobj_t * special,
    benefit_t *be, boolean_t lose_em)
{
  weaponinfo_t *info = weaponinfo[be->subtype];
  int pw_index;

  if (lose_em)
    return P_RemoveWeapon(player, info);

  if (! P_AddWeapon(player, info, &pw_index))
    return false;

  player->pending_wp = (weapon_selection_e) pw_index;

  // be cheeky... :-)
  player->grin_count = GRIN_TIME;

  return true;
}

//
// GiveHealth
//
// Returns false if not health is not needed,
//
// New Procedure: -ACB- 1998/06/21
//
static boolean_t GiveHealth(player_t * player, mobj_t * special,
    benefit_t *be, boolean_t lose_em)
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
// -ACB- 1998/06/21
//
static boolean_t GiveArmour(player_t * player, mobj_t * special,
    benefit_t *be, boolean_t lose_em)
{
  armour_type_e a_class = (armour_type_e)be->subtype;
  
  DEV_ASSERT2(0 <= a_class && a_class < NUMARMOUR);

  if (lose_em)
  {
    player->armours[a_class] -= be->amount;
    if (player->armours[a_class] < 0)
      player->armours[a_class] = 0;
    return true;
  }

  if (player->armours[a_class] >= be->limit)
    return false;

  player->armours[a_class] += be->amount;

  if (player->armours[a_class] > be->limit)
    player->armours[a_class] = be->limit;

  return true;
}

//
// GiveKey
//
static boolean_t GiveKey(player_t * player, mobj_t * special, benefit_t *be, boolean_t lose_em)
{
  keys_e key = (keys_e)be->subtype;
 
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

  // -ACB- 1998/06/10 Force redraw of status bar, to update keys.
  stbar_update = true;

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
static boolean_t GivePower(player_t * player, mobj_t * special,
    benefit_t *be, boolean_t lose_em)
{ 
  int i;

  // -ACB- 1998/06/20 - calculate duration in seconds
  flo_t duration = be->amount * TICRATE;
  flo_t limit    = be->limit  * TICRATE;

  if (lose_em)
  {
    player->powers[be->subtype] -= duration;
    if (player->powers[be->subtype] < 0)
      player->powers[be->subtype] = 0;
    return true;
  }
 
  if (player->powers[be->subtype] >= limit)
    return false;

  player->powers[be->subtype] += duration;

  if (player->powers[be->subtype] > limit)
    player->powers[be->subtype] = limit;

  // special handling for berserk...
  if (be->subtype == PW_Berserk)
  {
    for (i=0; i < MAXWEAPONS; i++)
    {
      playerweapon_t *pw = &player->weapons[i];

      // -AJA- FIXME: choose lowest priority close combat.
      if (pw->owned && DDF_CompareName(pw->info->ddf.name, "FIST") == 0)
      {
        player->pending_wp = (weapon_selection_e)i;
        break;
      }
    }
  }

  // special handling for scuba...
  if (be->subtype == PW_Scuba)
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
boolean_t P_GiveBenefitList(player_t *player, mobj_t * special, 
    benefit_t *list, boolean_t lose_em)
{
  boolean_t pickup = false;
  // is it a weapon that will stay in old deathmatch?
  boolean_t dm_weapon = false;

  // leave placed weapons forever in old deathmatch mode
  // but only if we haven't already picked it up.
 
#if 0  // FIXME
  // is it a old deathmatch weapon that we have any use for?
  boolean_t dm_needweapon = false;
  if (!lose_em && netgame && deathmatch <= 1)
  {
    benefit_t *b;
    for (b = list; b; b=b->next)
    {
      if (b->type == BENEFIT_Weapon && (!(special && (special->flags & MF_DROPPED))))
      {
        // it is an "old dm" weapon.
        dm_weapon = true;
        // minor hack: give the weapon, and see if we needed it.
        // in that case, the benefit should be picked up
        if (GiveWeapon(player, special, b))
          dm_needweapon = true;
      }
    }
    if (dm_weapon && !dm_needweapon)
      // it was a dm weapon, but we already have it, so don't pick it up again.
      return false;
  }
#endif

  for (; list; list=list->next)
  {
    if (list->type == BENEFIT_None || list->amount <= 0.0)
      continue;
    
    switch (list->type)
    {
      case BENEFIT_Ammo:
        pickup |= GiveAmmo(player, special, list, lose_em);
        break;

      case BENEFIT_AmmoLimit:
        pickup |= GiveAmmoLimit(player, special, list, lose_em);
        break;

      case BENEFIT_Weapon:
        pickup |= GiveWeapon(player, special, list, lose_em);
        break;

      case BENEFIT_Key:
        pickup |= GiveKey(player, special, list, lose_em);
        break;

      case BENEFIT_Health:
        pickup |= GiveHealth(player, special, list, lose_em);
        break;

      case BENEFIT_Armour:
        pickup |= GiveArmour(player, special, list, lose_em);
        break;

      case BENEFIT_Powerup:
        pickup |= GivePower(player, special, list, lose_em);
        break;

      default:
        break;
    }
  }

  if (dm_weapon)
    pickup = false;

  return pickup;
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
  flo_t delta;
  sfx_t *sound;
  boolean_t pickup = false;

  delta = special->z - toucher->z;

  // out of reach
  if (delta > toucher->height || delta < -special->height)
    return;

  player = toucher->player;

  DEV_ASSERT2(player);

  // Dead thing touching. Can happen with a sliding player corpse.
  if (toucher->health <= 0)
    return;

  // -KM- 1998/09/27 Sounds.ddf
  sound = special->info->activesound;
  toucher->flags |= MF_JUSTPICKEDUP;

  // First check for lost benefits
  pickup |= P_GiveBenefitList(player, special, 
      special->info->lose_benefits, true);
  
  // Run through the list of all pickup benefits...
  pickup |= P_GiveBenefitList(player, special, 
      special->info->pickup_benefits, false);

  if (special->flags & MF_COUNTITEM)
  {
    player->itemcount++;
    pickup = true;
  }

  if (pickup)
  {
    special->health = 0;
    P_KillMobj(player->mo, special, NULL);

    player->bonuscount += BONUS_ADD;
    if (player->bonuscount > BONUS_LIMIT)
      player->bonuscount = BONUS_LIMIT;

    // -AJA- FIXME: OPTIMISE THIS!
    if (special->info->pickup_message &&
        DDF_LanguageValidRef(special->info->pickup_message))
    {
      CON_PlayerMessage(player, 
          DDF_LanguageLookup(special->info->pickup_message));
    }

    if (sound)
      S_StartSound(player->mo, sound);
  }
}

//
// P_KillMobj
//
// Altered to reflect the fact that the dropped item is a pointer to
// mobjinfo_t, uses new procedure: P_MobjCreateObject.
//
// Note: Damtype can be NULL here.
//
// -ACB- 1998/08/01
//
// -AJA- 1999/09/12: Now uses P_SetMobjStateDeferred, since this
//       routine can be called by TryMove/PIT_CheckRelThing/etc.
//
void P_KillMobj(mobj_t * source, mobj_t * target, const damage_t *damtype)
{
  const mobjinfo_t *item;
  statenum_t state;
  boolean_t overkill;

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
  else if (!netgame && (target->flags & MF_COUNTKILL))
  {
    // count all monster deaths,
    // even those caused by other monsters
    consoleplayer->killcount++;
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
    if (target->player == consoleplayer && automapactive)
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
// * source    - mobj doing the thrusting.
// * thrust    - amount of thrust done (same values as damage).  Can
//               be negative to "pull" instead of push.
//
// -AJA- 1999/11/06: Wrote this routine.
//
void P_ThrustMobj(mobj_t * target, mobj_t * source, flo_t thrust)
{
  flo_t dx, dy, dz;
  flo_t push, slope;
  angle_t angle;

  dx = target->x - source->x;
  dy = target->y - source->y;
  dz = MO_MIDZ(target) - MO_MIDZ(source);
  
  angle = R_PointToAngle(0, 0, dx, dy);

  // -ACB- 2000/03/11 Div-by-zero check...
  CHECKVAL(target->info->mass);

  push = 12.0 * thrust / target->info->mass;

  target->mom.x += push * M_Cos(angle);
  target->mom.y += push * M_Sin(angle);

  if (level_flags.true3dgameplay)
  {
    slope = P_ApproxSlope(dx, dy, dz);
  
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
    mobj_t * source, flo_t damage, const damage_t * damtype)
{
  player_t *player;
  statenum_t state;
  flo_t saved = 0;
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
    damage /= 2;

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
    // Below certain threshold, ignore damage in GOD mode, or with INVUL power
    if (damage < 1000 &&
        ((player->cheats & CF_GODMODE) || player->powers[PW_Invulnerable]))
    {
      return;
    }

    // check which armour can take some damage
    for (i=ARMOUR_Red; i >= ARMOUR_Green; i--)
    {
      if (damtype && damtype->no_armour)
        continue;

      if (player->armours[i] <= 0)
        continue;

      switch (i)
      {
        case ARMOUR_Green:  saved = damage * 0.33; break;
        case ARMOUR_Blue:   saved = damage * 0.50; break;
        case ARMOUR_Yellow: saved = damage * 0.75; break;
        case ARMOUR_Red:    saved = damage * 0.90; break;
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
    }

    // mirror mobj health here for Dave
    player->health -= damage;

    if (player->health < 0)
      player->health = 0;

    player->attacker = source;

    // add damage after armour / invuln detection
    if (damage > 0)
      player->damagecount += MAX(damage, DAMAGE_ADD_MIN);

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

  if ((!target->threshold || target->extendedflags & EF_NOGRUDGE) &&
      source && source != target && (!(source->extendedflags & EF_NEVERTARGET)))
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

