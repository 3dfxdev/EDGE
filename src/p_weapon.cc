//----------------------------------------------------------------------------
//  EDGE Weapon (player sprites) Action Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
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
// -KM- 1998/11/25 Added/Changed stuff for weapons.ddf
//

#include "i_defs.h"
#include "p_weapon.h"

#include "e_event.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "m_misc.h"
#include "m_random.h"
#include "p_action.h"
#include "p_local.h"
#include "rad_trig.h"
#include "s_sound.h"
#include "w_wad.h"

//
// P_SetPsprite
//
void P_SetPsprite(player_t * p, int position, int stnum)
{
	pspdef_t *psp = &p->psprites[position];

	if (stnum == S_NULL)
	{
		// object removed itself
		psp->state = psp->next_state = NULL;
		return;
	}

	state_t *st = &states[stnum];

	psp->tics = st->tics;
	psp->state = st;
	psp->next_state = (st->nextstate == S_NULL) ? NULL : 
		(states + st->nextstate);

	// Call action routine.
	// Modified handling.

	p->action_psp = position;

	if (st->action)
		(* st->action)(p->mo);
}

//
// P_SetPspriteDeferred
//
// -AJA- 2004/11/05: This is preferred method, doesn't run any actions,
//       which (ideally) should only happen during P_MovePsprites().
//
void P_SetPspriteDeferred(player_t * p, int position, int stnum)
{
	pspdef_t *psp = &p->psprites[position];

	if (stnum == S_NULL || psp->state == NULL)
	{
		P_SetPsprite(p, position, stnum);
		return;
	}

	psp->tics = 0;
	psp->next_state = (states + stnum);
}

//
// P_CheckWeaponSprite
//
// returns true if the sprite(s) for the weapon exist.  Prevents being
// able to e.g. select the super shotgun when playing with a DOOM 1
// IWAD (and cheating).
//
// -KM- 1998/12/16 Added check to make sure sprites exist.
// -AJA- 2000: Made into a separate routine.
//
bool P_CheckWeaponSprite(weapondef_c *info)
{
	if (info->up_state == S_NULL)
		return false;

	return DDF_CheckSprites(info->first_state, info->last_state);
}

static bool ButtonDown(player_t *p, int ATK)
{
	if (ATK == 0)
		return (p->cmd.buttons & BT_ATTACK);
	else
		return (p->cmd.extbuttons & EBT_SECONDATK);
}

static void ReloadWeapon(player_t *p, int idx, int ATK)
{
	weapondef_c *info = p->weapons[idx].info;

	if (info->ammo[ATK] == AM_NoAmmo || info->clip_size[ATK] == 0)
		return;

	int qty = info->clip_size[ATK] - p->weapons[idx].clip_size[ATK];

	DEV_ASSERT2(qty > 0);
	DEV_ASSERT2(qty <= p->ammo[info->ammo[ATK]].num);

	p->weapons[idx].clip_size[ATK] += qty;
	p->ammo[info->ammo[ATK]].num   -= qty;
}

static bool WeaponCanFire(player_t *p, int idx, int ATK)
{
	weapondef_c *info = p->weapons[idx].info;

	// the order here is important, to allow NoAmmo+Clip weapons.
	if (info->clip_size[ATK] > 0)
		return (info->ammopershot[ATK] <= p->weapons[idx].clip_size[ATK]);

	if (info->ammo[ATK] == AM_NoAmmo)
		return true;

	return (info->ammopershot[ATK] <= p->ammo[info->ammo[ATK]].num);
}

static bool WeaponCanReload(player_t *p, int idx, int ATK)
{
	weapondef_c *info = p->weapons[idx].info;

	bool can_fire = WeaponCanFire(p, idx, ATK);

	// for non-clip weapon, can reload whenever enough ammo is avail.
	if (info->clip_size[ATK] == 0)
		return can_fire;

	// for clip weapons, cannot reload until clip is empty.
	if (can_fire)
		return false;

	// ammo check
	return (info->ammo[ATK] == AM_NoAmmo) ||
		   (info->clip_size[ATK] - p->weapons[idx].clip_size[ATK] <=
			p->ammo[info->ammo[ATK]].num);
}

static bool WeaponCanPartialReload(player_t *p, int idx, int ATK)
{
	weapondef_c *info = p->weapons[idx].info;

	// for non-clip weapons, same as WeaponCanReload()
	if (info->clip_size[ATK] == 0)
		return WeaponCanFire(p, idx, ATK);

	// clip check (cannot reload if clip is full)
	if (p->weapons[idx].clip_size[ATK] == info->clip_size[ATK])
		return false;

	// ammo check
	return (info->ammo[ATK] == AM_NoAmmo) ||
		   (info->clip_size[ATK] - p->weapons[idx].clip_size[ATK] <=
			p->ammo[info->ammo[ATK]].num);
}

// returns true when weapon will either fire or reload
// (assuming the button is held down).
static bool WeaponCouldAutoFire(player_t *p, int idx, int ATK)
{
	weapondef_c *info = p->weapons[idx].info;

	if (! info->attack_state[ATK])
		return false;

	if (info->ammo[ATK] == AM_NoAmmo)
		return true;

	int total = p->ammo[info->ammo[ATK]].num;

	if (info->clip_size[ATK] == 0 && info->ammopershot[ATK] <= total)
		return true;

	// for clip weapons, either need a non-empty clip or enough
	// ammo to fill the clip (which is able to be filled without the
	// manual reload key).
	if (info->clip_size[ATK] > 0 &&
		(info->ammopershot[ATK] <= p->weapons[idx].clip_size[ATK] ||
		 (info->clip_size[ATK] <= total &&
		  (info->specials[ATK] & (WPSP_Trigger | WPSP_Fresh)) )))
	{
		return true;
	}

	return false;
}

static void GotoDownState(player_t *p)
{
	int newstate = p->weapons[p->ready_wp].info->down_state;

	P_SetPspriteDeferred(p, ps_weapon, newstate);
	P_SetPsprite(p, ps_crosshair, S_NULL);
}

static void GotoReadyState(player_t *p)
{
	int newstate = p->weapons[p->ready_wp].info->ready_state;

	P_SetPspriteDeferred(p, ps_weapon, newstate);
	P_SetPspriteDeferred(p, ps_crosshair, p->weapons[p->ready_wp].info->crosshair);
}

static void GotoEmptyState(player_t *p)
{
	int newstate = p->weapons[p->ready_wp].info->empty_state;

	P_SetPspriteDeferred(p, ps_weapon, newstate);
	P_SetPsprite(p, ps_crosshair, S_NULL);
}

static void GotoAttackState(player_t * p, int ATK, bool can_warmup)
{
	weapondef_c *info = p->weapons[p->ready_wp].info;

	int newstate = info->attack_state[ATK];

	if (p->remember_atk[ATK] >= 0)
	{
		newstate = p->remember_atk[ATK];
		p->remember_atk[ATK] = -1;
	}
	else if (can_warmup && info->warmup_state[ATK])
	{
		newstate = info->warmup_state[ATK];
	}

	if (newstate)
	{
		P_SetPspriteDeferred(p, ps_weapon, newstate);
		p->idlewait = 0;
	}
}

static void GotoReloadState(player_t *p, int ATK)
{
	weapondef_c *info = p->weapons[p->ready_wp].info;

	ReloadWeapon(p, p->ready_wp, ATK);

	// second attack will fall-back to using normal reload states.
	if (ATK == 1 && ! info->reload_state[ATK])
		ATK = 0;

	if (info->reload_state[ATK])
	{
		P_SetPspriteDeferred(p, ps_weapon, info->reload_state[ATK]);
		p->idlewait = 0;
	}

	// if player has reload states, use 'em baby
	if (p->mo->info->reload_state)
		P_SetMobjStateDeferred(p->mo, p->mo->info->reload_state, 0);
}

//
// SwitchAway
//
// Not enough ammo to shoot, selects the next weapon to use.
// In some cases we prefer to reload the weapon (if we can).
// The NO_SWITCH special prevents the switch, enter empty or ready
// states instead.
//
static void SwitchAway(player_t * p, int ATK, int reload)
{
	weapondef_c *info = p->weapons[p->ready_wp].info;

	if (reload && WeaponCanReload(p, p->ready_wp, ATK))
		GotoReloadState(p, ATK);
	else if (info->specials[ATK] & WPSP_SwitchAway)
		P_SelectNewWeapon(p, -100, AM_DontCare);
	else if (info->empty_state && ! WeaponCouldAutoFire(p, p->ready_wp, 0))
		GotoEmptyState(p);
	else 
		GotoReadyState(p);
}

//
// P_BringUpWeapon
//
// Starts bringing the pending weapon up
// from the bottom of the screen.
//
static void P_BringUpWeapon(player_t * p)
{
	DEV_ASSERT2(p->pending_wp != WPSEL_NoChange);

	weapon_selection_e sel;
	sel = p->ready_wp = p->pending_wp;

	p->pending_wp = WPSEL_NoChange;
	p->psprites[ps_weapon].sy = WEAPONBOTTOM;

	p->remember_atk[0] = -1;
	p->remember_atk[1] = -1;
	p->idlewait = 0;

	if (sel == WPSEL_None)
	{
		p->attackdown[0] = false;
		p->attackdown[1] = false;

		P_SetPsprite(p, ps_weapon, S_NULL);
		P_SetPsprite(p, ps_flash, S_NULL);
		P_SetPsprite(p, ps_crosshair, S_NULL);

		if (viewiszoomed)
			R_SetFOV(zoomedfov);
		return;
	}

	weapondef_c *info = p->weapons[sel].info;

	if (info->specials[0] & WPSP_Animated)
		p->psprites[ps_weapon].sy = WEAPONTOP;

	// we don't need to check level_flags.limit_zoom, as viewiszoomed
	// should always be false when we get here.

	if (viewiszoomed)
	{
		if (info->zoom_fov > 0)
			R_SetFOV(info->zoom_fov);
		else
			R_SetFOV(zoomedfov);
	}

	if (info->start)
		S_StartSound(p->mo, info->start);

	P_SetPspriteDeferred(p, ps_weapon, info->up_state);
	P_SetPsprite(p, ps_flash,  S_NULL);
	P_SetPsprite(p, ps_crosshair, S_NULL);

	p->refire = info->refire_inacc ? 0 : 1;
}

//
// P_SelectNewWeapon
//
// Out of ammo, pick a weapon to change to.
// Preferences are set here.
//
// The `ammo' parameter is normally AM_DontCare, meaning that the user
// ran out of ammo while firing.  Otherwise it is some ammo just
// picked up by the player.
//
// This routine deliberately ignores second attacks.
//
void P_SelectNewWeapon(player_t * p, int priority, ammotype_e ammo)
{
	int key = -1;
	weapondef_c *info;

	for (int i = 0; i < MAXWEAPONS; i++)
	{
		info = p->weapons[i].info;

		if (! p->weapons[i].owned)
			continue;

		if (info->dangerous || info->priority < priority)
			continue;

		if (ammo != AM_DontCare && info->ammo[0] != ammo)
			continue;

		if (! WeaponCouldAutoFire(p, i, 0))
			continue;

		if (! P_CheckWeaponSprite(info))
			continue;

		p->pending_wp = (weapon_selection_e) i;
		priority = info->priority;
		key = info->bind_key;
	}

	// all out of choices ?
	if (priority < 0)
	{
		p->pending_wp = (ammo == AM_DontCare) ? WPSEL_None : WPSEL_NoChange;
		return;
	}

	if (p->pending_wp == p->ready_wp)
	{
		p->pending_wp = WPSEL_NoChange;
		return;
	}

	// update current key choice
	if (key >= 0)
	{
		DEV_ASSERT2(p->pending_wp >= 0);
		info = p->weapons[p->pending_wp].info;

		for (int j = 0; j < weaponkey[key].numchoices; j++)
		{
			if (weaponkey[key].choices[j] == info)
			{
				p->key_choices[key] = j;
				break;
			}
		}
	}
}

//
// P_TrySwitchNewWeapon
//
void P_TrySwitchNewWeapon(player_t *p, int new_weap, ammotype_e new_ammo)
{
	// be cheeky... :-)
	if (new_weap >= 0)
		p->grin_count = GRIN_TIME;

	if (p->pending_wp != WPSEL_NoChange)
		return;

	if (! level_flags.weapon_switch && p->ready_wp != WPSEL_None &&
		(WeaponCouldAutoFire(p, p->ready_wp, 0) ||
		 WeaponCouldAutoFire(p, p->ready_wp, 1)))
	{
		return;
	}

	if (new_weap >= 0)
	{
		if ( WeaponCouldAutoFire(p, new_weap, 0))
			p->pending_wp = (weapon_selection_e) new_weap;
		return;
	}

	DEV_ASSERT2(new_ammo >= 0);
	
	// We were down to zero ammo, so select a new weapon.
	// Choose the next highest priority weapon than the current one.
	// Don't override any weapon change already underway.
	// Don't change weapon if NO_SWITCH is true.

	int priority = -100;

	if (p->ready_wp >= 0)
	{
		weapondef_c *w = p->weapons[p->ready_wp].info;

		if (! (w->specials[0] & WPSP_SwitchAway))
			return;

		priority = w->priority;
	}

	P_SelectNewWeapon(p, priority, new_ammo);
}

//
// P_TryFillNewWeapon
//
// When ammo is AM_DontCare, uses any ammo the player has (qty parameter
// ignored).  Returns true if uses any of the ammo.
//
bool P_TryFillNewWeapon(player_t *p, int idx, ammotype_e ammo, int *qty)
{
	bool result = false;

	weapondef_c *info = p->weapons[idx].info;

	for (int ATK = 0; ATK < 2; ATK++)
	{
		if (! info->attack_state[ATK])
			continue;

		if (info->ammo[ATK] == AM_NoAmmo || info->clip_size[ATK] == 0)
			continue;

		if (ammo != AM_DontCare && info->ammo[ATK] != ammo)
			continue;

		if (ammo == AM_DontCare)
			qty = &p->ammo[info->ammo[ATK]].num;

		DEV_ASSERT2(qty);

		if (info->clip_size[ATK] <= *qty)
		{
			p->weapons[idx].clip_size[ATK] = info->clip_size[ATK];
			*qty -= info->clip_size[ATK];

			result = true;
		}
	}

	return result;
}

//
// P_DropWeapon
//
// Player died, so put the weapon away.
//
void P_DropWeapon(player_t * p)
{
	p->remember_atk[0] = -1;
	p->remember_atk[1] = -1;

	if (p->ready_wp != WPSEL_None)
		GotoDownState(p);
}

//
// P_SetupPsprites
//
// Called at start of level for each player.
//
void P_SetupPsprites(player_t * p)
{
	// remove all psprites
	for (int i = 0; i < NUMPSPRITES; i++)
	{
		pspdef_t *psp = &p->psprites[i];

		psp->state = NULL;
		psp->next_state = NULL;
		psp->sx = psp->sy = 0;
		psp->visibility = psp->vis_target = VISIBLE;
	}

	// choose highest priority FREE weapon as the default
	if (p->ready_wp == WPSEL_None)
		P_SelectNewWeapon(p, -100, AM_DontCare);
	else
		p->pending_wp = p->ready_wp;

	P_BringUpWeapon(p);
}

//
// P_MovePsprites
//
// Called every tic by player thinking routine.
//
#define MAX_PSP_LOOP  10

void P_MovePsprites(player_t * p)
{
	// check if player has NO weapon but wants to change
	if (p->ready_wp == WPSEL_None && p->pending_wp != WPSEL_NoChange)
	{
		P_BringUpWeapon(p);
	}

	pspdef_t *psp = &p->psprites[0];

	for (int i = 0; i < NUMPSPRITES; i++, psp++)
	{
		// a null state means not active
		if (! psp->state)
			continue;

		for (int loop_count=0; loop_count < MAX_PSP_LOOP; loop_count++)
		{
			// drop tic count and possibly change state
			// Note: a -1 tic count never changes.
			if (psp->tics < 0)
				break;

			psp->tics--;

			if (psp->tics > 0)
				break;

			P_SetPsprite(p, i, psp->next_state ?
					(psp->next_state - states) : S_NULL);

			if (psp->tics != 0)
				break;
		}

		// handle translucency fades
		psp->visibility = (34 * psp->visibility + psp->vis_target) / 35;
	}

	p->psprites[ps_flash].sx = p->psprites[ps_weapon].sx;
	p->psprites[ps_flash].sy = p->psprites[ps_weapon].sy;

	p->idlewait++;
}

//
// P_Zoom
//
void P_Zoom(player_t *p)
{
	if (viewiszoomed)
	{
		R_SetFOV(normalfov);
		viewiszoomed = false;
		return;
	}

	int fov = 0;

	if (! (p->ready_wp < 0 || p->pending_wp >= 0))
		fov = p->weapons[p->ready_wp].info->zoom_fov;
	
	// In `LimitZoom' mode, only allow zooming if weapon supports it
	if (level_flags.limit_zoom)
	{
		if (fov <= 0)
			return;
	}

	if (fov <= 0)
		fov = zoomedfov;

	R_SetFOV(fov);
	viewiszoomed = true;
}


//----------------------------------------------------------------------------
//  ACTION HANDLERS
//----------------------------------------------------------------------------


static void BobWeapon(player_t *p, weapondef_c *info)
{
	bool hasjetpack = p->powers[PW_Jetpack] > 0;
	pspdef_t *psp = &p->psprites[p->action_psp];

	float new_sx = 1.0f;
	float new_sy = WEAPONTOP;
	
	// bob the weapon based on movement speed
	if (! hasjetpack)
	{
		angle_t angle = (128 * leveltime) << ANGLETOFINESHIFT;
		new_sx = 1.0f + p->bob * PERCENT_2_FLOAT(info->swaying) * M_Cos(angle);

		angle &= (ANG180 - 1);
		new_sy = WEAPONTOP + p->bob * PERCENT_2_FLOAT(info->bobbing) * M_Sin(angle);
	}

	psp->sx = new_sx;
	psp->sy = new_sy;

#if 0  // won't really work with low framerates
	// try to prevent noticeable jumps
	if (fabs(new_sx - psp->sx) <= MAX_BOB_DX)
		psp->sx = new_sx;
	else
		psp->sx += (new_sx > psp->sx) ? MAX_BOB_DX : -MAX_BOB_DX;

	if (fabs(new_sy - psp->sy) <= MAX_BOB_DY)
		psp->sy = new_sy;
	else
		psp->sy += (new_sy > psp->sy) ? MAX_BOB_DY : -MAX_BOB_DY;
#endif
}

//
// A_WeaponReady
//
// The player can fire the weapon
// or change to another weapon at this time.
// Follows after getting weapon up,
// or after previous attack/fire sequence.
//
void A_WeaponReady(mobj_t * mo)
{
	player_t *p = mo->player;
	pspdef_t *psp = &p->psprites[p->action_psp];

	DEV_ASSERT2(p->ready_wp != WPSEL_None);

	weapondef_c *info = p->weapons[p->ready_wp].info;

	// check for change if player is dead, put the weapon away
	if (p->pending_wp != WPSEL_NoChange || p->health <= 0)
	{
		// change weapon (pending weapon should already be validated)
		GotoDownState(p);
		return;
	}

	// check for emptiness.  The ready_state check is needed since this
	// code is also used by the EMPTY action (prevent looping).
	if (info->empty_state && ! WeaponCouldAutoFire(p, p->ready_wp, 0) &&
		psp->state == &states[info->ready_state])
	{
		// don't use Deferred here, since we don't want the weapon to
		// display the ready sprite (even only briefly).
		P_SetPsprite(p, ps_weapon, info->empty_state);
		return;
	}

	if (info->idle && (psp->state == &states[info->ready_state] ||
		(info->empty_state && psp->state == &states[info->empty_state])))
	{
		S_StartSound(mo, info->idle);
	}

	bool fire_0 = ButtonDown(p, 0);
	bool fire_1 = ButtonDown(p, 1);

	if (fire_0 != fire_1)
	{
		for (int ATK = 0; ATK < 2; ATK++)
		{
			if (! info->attack_state[ATK])
				continue;

			if (! ButtonDown(p, ATK))
				continue;

			// check for fire: the missile launcher and bfg do not auto fire
			if (!p->attackdown[ATK] || info->autofire[ATK])
			{
				p->attackdown[ATK] = true;
				p->flash = false;

				if (WeaponCanFire(p, p->ready_wp, ATK))
					GotoAttackState(p, ATK, true);
				else
					SwitchAway(p, ATK, info->specials[ATK] & WPSP_Trigger);

				return;  // leave now
			}
		}
	}

	// reset memory of held buttons (must be done right here)
	if (! fire_0) p->attackdown[0] = false;
	if (! fire_1) p->attackdown[1] = false;

	// give that weapon a polish, soldier!
	if (info->idle_state && p->idlewait >= info->idle_wait)
	{
		if (M_RandomTest(info->idle_chance))
		{
			p->idlewait = 0;
			P_SetPspriteDeferred(p, ps_weapon, info->idle_state);
		}
		else
		{
			// wait another (idle_wait / 10) seconds before trying again
			p->idlewait = info->idle_wait * 9 / 10;
		}
	}

	// handle manual reload and fresh-ammo reload
	if (! fire_0 && ! fire_1)
	{
		for (int ATK = 0; ATK < 2; ATK++)
		{
			if (! info->attack_state[ATK])
				continue;

			if ((info->specials[ATK] & WPSP_Fresh) && info->clip_size[ATK] > 0 &&
				info->ammo[ATK] != AM_NoAmmo)
			{
				if (WeaponCanReload(p, p->ready_wp, ATK))
				{
					GotoReloadState(p, ATK);
					break;
				}
			}
			else if ((p->cmd.extbuttons & EBT_RELOAD) &&
					 (info->specials[ATK] & WPSP_Manual) &&
					  info->reload_state[ATK])
			{
				bool reload = ((info->specials[ATK] & WPSP_Partial) ||
						  info->discard_state[ATK]) ?
					WeaponCanPartialReload(p, p->ready_wp, ATK) :
					WeaponCanReload(p, p->ready_wp, ATK);

				// for non-clip weapons, chew up some ammo
				if (reload && info->clip_size[ATK] == 0 &&
					info->ammo[ATK] != AM_NoAmmo)
				{
					p->ammo[info->ammo[ATK]].num -= info->ammopershot[ATK];
				}

				// for discarding, we require a non-empty clip
				if (reload && info->discard_state[ATK] &&
					(info->clip_size == 0 || WeaponCanFire(p, p->ready_wp, ATK)))
				{
					p->weapons[p->ready_wp].clip_size[ATK] = 0;
					P_SetPspriteDeferred(p, ps_weapon, info->discard_state[ATK]);
					break;
				}
				else if (reload)
				{
					GotoReloadState(p, ATK);
					break;
				}
			}
		}  // for (ATK)

	}  // (! fire_0 && ! fire_1)

	BobWeapon(p, info);
}

//
// A_WeaponEmpty
//
void A_WeaponEmpty(mobj_t * mo)
{
	A_WeaponReady(mo);
}

//
// A_ReFire
//
// The player can re-fire the weapon without lowering it entirely.
//
// -AJA- 1999/08/10: Reworked for multiple attacks.
//
static void DoReFire(mobj_t * mo, int ATK)
{
	player_t *p = mo->player;

	if (p->pending_wp >= 0 || p->health <= 0)
	{
		GotoDownState(p);
		return;
	}

	weapondef_c *info = p->weapons[p->ready_wp].info;

	p->remember_atk[ATK] = -1;

	// check for fire
	// (if a weaponchange is pending, let it go through instead)

	if (ButtonDown(p, ATK))
	{
		// -KM- 1999/01/31 Check for semiautomatic weapons.
		if (!p->attackdown[ATK] || info->autofire[ATK])
		{
			p->refire++;
			p->flash = false;

			if (WeaponCanFire(p, p->ready_wp, ATK))
				GotoAttackState(p, ATK, false);
			else
				SwitchAway(p, ATK, info->specials[ATK] & WPSP_Trigger);
			return;
		}
	}

	p->refire = info->refire_inacc ? 0 : 1;

	if (! WeaponCouldAutoFire(p, p->ready_wp, ATK))
		SwitchAway(p, ATK, 0);
}

void A_ReFire  (mobj_t * mo) { DoReFire(mo, 0); }
void A_ReFireSA(mobj_t * mo) { DoReFire(mo, 1); }

//
// A_NoFire
//
// If the player is still holding the fire button, continue, otherwise
// return to the weapon ready states.
//
// -AJA- 1999/08/18: written.
//
static void DoNoFire(mobj_t * mo, int ATK, bool does_return)
{
	player_t *p = mo->player;
	pspdef_t *psp = &p->psprites[p->action_psp];

	if (p->pending_wp >= 0 || p->health <= 0)
	{
		GotoDownState(p);
		return;
	}

	weapondef_c *info = p->weapons[p->ready_wp].info;

	p->remember_atk[ATK] = -1;

	// check for fire
	//  (if a weaponchange is pending, let it go through instead)

	if (ButtonDown(p, ATK))
	{
		// -KM- 1999/01/31 Check for semiautomatic weapons.
		if (!p->attackdown[ATK] || info->autofire[ATK])
		{
			p->refire++;
			p->flash = false;

			if (! WeaponCanFire(p, p->ready_wp, ATK))
				SwitchAway(p, ATK, info->specials[ATK] & WPSP_Trigger);
			return;
		}
	}

	p->refire = info->refire_inacc ? 0 : 1;
	p->remember_atk[0] = does_return ? psp->state->nextstate : -1;

	if (WeaponCouldAutoFire(p, p->ready_wp, ATK))
		GotoReadyState(p);
	else
		SwitchAway(p, ATK, 0);
}

void A_NoFire  (mobj_t * mo)       { DoNoFire(mo, 0, false); }
void A_NoFireSA(mobj_t * mo)       { DoNoFire(mo, 1, false); }
void A_NoFireReturn  (mobj_t * mo) { DoNoFire(mo, 0, true);  }
void A_NoFireReturnSA(mobj_t * mo) { DoNoFire(mo, 1, true);  }

//
// A_WeaponKick
//
// -AJA- 2000/02/08: written.
//
void A_WeaponKick(mobj_t * mo)
{
	player_t *p = mo->player;
	pspdef_t *psp = &p->psprites[p->action_psp];

	float kick = 0.05f;

	if (! level_flags.kicking)
		return;

	if (psp->state && psp->state->action_par)
		kick = ((float *) psp->state->action_par)[0];

	p->deltaviewheight -= kick;
	p->kick_offset = kick;
}

//
// A_CheckReload
//
// Check whether the player has used up the clip quantity of ammo.
// If so, must reload.
//
// For weapons with a clip, only reloads when clip_size is 0 (and
// enough ammo available to fill it).  For non-clip weapons, reloads
// when enough ammo exists in the "ammo bucket" (for NO_AMMO weapons,
// it always reloads).
//
// -KM- 1999/01/31 Check clip size.
// -AJA- 1999/08/11: Reworked for new playerweapon_t field.
//
static void DoCheckReload(mobj_t * mo, int ATK)
{
	player_t *p = mo->player;

	if (p->pending_wp >= 0 || p->health <= 0)
	{
		GotoDownState(p);
		return;
	}

	if (WeaponCanReload(p, p->ready_wp, ATK))
		GotoReloadState(p, ATK);
	else if (! WeaponCanFire(p, p->ready_wp, ATK))
		SwitchAway(p, ATK, 0);
}

void A_CheckReload  (mobj_t * mo) { DoCheckReload(mo, 0); }
void A_CheckReloadSA(mobj_t * mo) { DoCheckReload(mo, 1); }

//
// A_Lower
//
// Lowers current weapon, and changes weapon at bottom.
//
void A_Lower(mobj_t * mo)
{
	player_t *p = mo->player;
	pspdef_t *psp = &p->psprites[p->action_psp];

	weapondef_c *info = p->weapons[p->ready_wp].info;

	if (level_flags.limit_zoom && viewiszoomed)
	{
		// In `LimitZoom' mode, disable any current zoom
		R_SetFOV(normalfov);
		viewiszoomed = false;
	}

	psp->sy += LOWERSPEED;

	// Is already down.
	if (! (info->specials[0] & WPSP_Animated))
		if (psp->sy < WEAPONBOTTOM)
			return;

	psp->sy = WEAPONBOTTOM;

	// Player is dead, don't bring weapon back up.
	if (p->playerstate == PST_DEAD || p->health <= 0)
	{
		p->ready_wp   = WPSEL_None;
		p->pending_wp = WPSEL_NoChange;

		P_SetPsprite(p, ps_weapon, S_NULL);
		return;
	}

	// handle weapons that were removed/upgraded while in use
	if (p->weapons[p->ready_wp].flags & PLWEP_Removing)
	{
		p->weapons[p->ready_wp].flags &= ~PLWEP_Removing;
		p->weapons[p->ready_wp].info = NULL;
	}

	// The old weapon has been lowered off the screen,
	// so change the weapon and start raising it

	if (p->pending_wp == WPSEL_NoChange)
	{
		p->ready_wp = WPSEL_None;
		P_SelectNewWeapon(p, -100, AM_DontCare);
	}

	P_BringUpWeapon(p);
}

//
// A_Raise
//
void A_Raise(mobj_t * mo)
{
	player_t *p = mo->player;
	pspdef_t *psp = &p->psprites[p->action_psp];

	weapondef_c *info = p->weapons[p->ready_wp].info;

	psp->sy -= RAISESPEED;

	if (psp->sy > WEAPONTOP)
		return;

	psp->sy = WEAPONTOP;

	// The weapon has been raised all the way,
	//  so change to the ready state.
	if (info->empty_state && ! WeaponCouldAutoFire(p, p->ready_wp, 0))
		GotoEmptyState(p);
	else
		GotoReadyState(p);
}

//
// A_SetCrosshair
//
void A_SetCrosshair(mobj_t * mo)
{
#if 0  // !! FIXME
	player_t *p = mo->player;
	pspdef_t *psp = &p->psprites[p->action_psp];

	P_SetPspriteDeferred(p, ps_crosshair,
			p->weapons[p->ready_wp].info->crosshair + psp->state->misc1);
#endif
}

void A_GotTarget(mobj_t * mo)
{
#if 0  // !! FIXME
	player_t *p = mo->player;
	pspdef_t *psp = &p->psprites[p->action_psp];

	atkdef_c *attack = p->weapons[p->ready_wp].info->attack;
	mobj_t *obj;

	obj = P_MapTargetAutoAim(mo, mo->angle, attack->range,
			attack->flags & AF_ForceAim);

	if (obj->extendedflags & EF_DUMMYMOBJ)
		obj = P_MapTargetAutoAim(mo, mo->angle + (1 << 26),
				attack->range, attack->flags & AF_ForceAim);

	if (obj->extendedflags & EF_DUMMYMOBJ)
		obj = P_MapTargetAutoAim(mo, mo->angle - (1 << 26),
				attack->range, attack->flags & AF_ForceAim);

	if (obj->extendedflags & EF_DUMMYMOBJ)
		P_SetPspriteDeferred(p, ps_crosshair,
				p->weapons[p->ready_wp].info->crosshair + psp->state->misc1);
	else
		P_SetPspriteDeferred(p, ps_crosshair,
				p->weapons[p->ready_wp].info->crosshair + psp->state->misc2);
#endif
}

//
// A_GunFlash
//
static void DoGunFlash(mobj_t * mo, int ATK)
{
	player_t *p = mo->player;
	
	DEV_ASSERT2(p->ready_wp >= 0);

	weapondef_c *info = p->weapons[p->ready_wp].info;

	if (!p->flash)
	{
		p->flash = true;

		P_SetPspriteDeferred(p, ps_flash, info->flash_state[ATK]);

#if 0  // the SHOOT actions already do this...
		if (mo->info->missile_state)
			P_SetMobjStateDeferred(mo, mo->info->missile_state, 0);
#endif
	}
}

void A_GunFlash  (mobj_t * mo) { DoGunFlash(mo, 0); }
void A_GunFlashSA(mobj_t * mo) { DoGunFlash(mo, 1); }

//
// WEAPON ATTACKS
//
static void DoWeaponShoot(mobj_t * mo, int ATK)
{
	player_t *p = mo->player;
	pspdef_t *psp = &p->psprites[p->action_psp];

	DEV_ASSERT2(p->ready_wp >= 0);

	weapondef_c *info = p->weapons[p->ready_wp].info;
	atkdef_c *attack = info->attack[ATK];

	// -AJA- 1999/08/10: Multiple attack support.
	if (psp->state && psp->state->action_par)
		attack = (atkdef_c *) psp->state->action_par;

	if (! attack)
		I_Error("Weapon [%s] missing %sattack.\n", info->ddf.name.GetString(),
			ATK ? "second " : "");

	ammotype_e ammo = info->ammo[ATK];

	// Minimal amount for one shot varies.
	int count = info->ammopershot[ATK];

	// Some do not need ammunition anyway.
	// Return if current ammunition sufficient.
	if (! WeaponCanFire(p, p->ready_wp, ATK))
		return;

	if (ammo != AM_NoAmmo)
	{
		if (info->clip_size[ATK] > 0)
		{
			p->weapons[p->ready_wp].clip_size[ATK] -= count;
			DEV_ASSERT2(p->weapons[p->ready_wp].clip_size[ATK] >= 0);
		}
		else
		{
			p->ammo[ammo].num -= count;
			DEV_ASSERT2(p->ammo[ammo].num >= 0);
		}
	}

	P_ActPlayerAttack(mo, attack);

	if (level_flags.kicking && ATK == 0)
	{
		p->deltaviewheight -= info->kick;
		p->kick_offset = info->kick;
	}

	if (mo->target && !(mo->target->extendedflags & EF_DUMMYMOBJ))
	{
		if (info->hit)
			S_StartSound(mo, info->hit);

		if (info->feedback)
			mo->flags |= MF_JUSTATTACKED;
	}
	else
	{
		if (info->engaged)
			S_StartSound(mo, info->engaged);
	}

	// show the player making the shot/attack...
	if (attack && attack->attackstyle == ATK_CLOSECOMBAT &&
			mo->info->melee_state)
	{
		P_SetMobjStateDeferred(mo, mo->info->melee_state, 0);
	}
	else if (mo->info->missile_state)
	{
		P_SetMobjStateDeferred(mo, mo->info->missile_state, 0);
	}

	if (info->flash_state[ATK] && !p->flash)
	{
		p->flash = true;
		P_SetPspriteDeferred(p, ps_flash, info->flash_state[ATK]);
	}

	// wake up monsters
	if (! (info->specials[ATK] & WPSP_SilentToMon) &&
		! (attack->flags & AF_SilentToMon))
	{
		P_NoiseAlert(p);
	}

	p->idlewait = 0;
}

void A_WeaponShoot  (mobj_t * mo) { DoWeaponShoot(mo, 0); }
void A_WeaponShootSA(mobj_t * mo) { DoWeaponShoot(mo, 1); }

//
// A_WeaponEject
//
// Used for ejecting shells (or other effects).
//
// -AJA- 1999/09/10: written.
//
void A_WeaponEject(mobj_t * mo)
{
	player_t *p = mo->player;
	pspdef_t *psp = &p->psprites[p->action_psp];

	weapondef_c *info = p->weapons[p->ready_wp].info;
	atkdef_c *attack = info->eject_attack;

	if (psp->state && psp->state->action_par)
		attack = (atkdef_c *) psp->state->action_par;

	P_ActPlayerAttack(mo, attack);
}

//
// A_WeaponPlaySound
//
// Generate an arbitrary sound from this weapon.
//
void A_WeaponPlaySound(mobj_t * mo)
{
	player_t *p = mo->player;
	pspdef_t *psp = &p->psprites[p->action_psp];

	sfx_t *sound = NULL;

	if (psp->state && psp->state->action_par)
		sound = (sfx_t *) psp->state->action_par;

	if (! sound)
	{
		M_WarnError("A_WeaponPlaySound: missing sound name !\n");
		return;
	}

	S_StartSound(mo, sound);
}

//
// A_WeaponKillSound
//
// Kill any current sound from this weapon.
//
void A_WeaponKillSound(mobj_t * mo)
{
	S_StopSound(mo);
}

//
// A_SFXWeapon1/2/3
//
void A_SFXWeapon1(mobj_t * mo)
{
	player_t *p = mo->player;
	S_StartSound(mo, p->weapons[p->ready_wp].info->sound1);
}

void A_SFXWeapon2(mobj_t * mo)
{
	player_t *p = mo->player;
	S_StartSound(mo, p->weapons[p->ready_wp].info->sound2);
}

void A_SFXWeapon3(mobj_t * mo)
{
	player_t *p = mo->player;
	S_StartSound(mo, p->weapons[p->ready_wp].info->sound3);
}

//
// These three routines make a flash of light when a weapon fires.
//
void A_Light0(mobj_t * mo) { mo->player->extralight = 0; }
void A_Light1(mobj_t * mo) { mo->player->extralight = 1; }
void A_Light2(mobj_t * mo) { mo->player->extralight = 2; }

//
// A_WeaponJump
//
void A_WeaponJump(mobj_t * mo)
{
	player_t *p = mo->player;
	pspdef_t *psp = &p->psprites[p->action_psp];

	weapondef_c *info = p->weapons[p->ready_wp].info;

	act_jump_info_t *jump;

	if (!psp->state || !psp->state->action_par)
	{
		M_WarnError("JUMP used in weapon [%s] without a label !\n",
				info->ddf.name.GetString());
		return;
	}

	jump = (act_jump_info_t *) psp->state->action_par;

	DEV_ASSERT2(jump->chance >= 0);
	DEV_ASSERT2(jump->chance <= 1);

	if (P_RandomTest(jump->chance))
	{
		psp->next_state = (psp->state->jumpstate == S_NULL) ? NULL :
			(states + psp->state->jumpstate);
	}
}

//
// A_WeaponTransSet
//
void A_WeaponTransSet(mobj_t * mo)
{
	player_t *p = mo->player;
	pspdef_t *psp = &p->psprites[p->action_psp];
	float value = VISIBLE;

	if (psp->state && psp->state->action_par)
	{
		value = ((percent_t *) psp->state->action_par)[0];
		value = MAX(0.0f, MIN(1.0f, value));
	}

	psp->visibility = psp->vis_target = value;
}

//
// A_WeaponTransFade
//
void A_WeaponTransFade(mobj_t * mo)
{
	player_t *p = mo->player;
	pspdef_t *psp = &p->psprites[p->action_psp];
	float value = INVISIBLE;

	if (psp->state && psp->state->action_par)
	{
		value = ((percent_t *) psp->state->action_par)[0];
		value = MAX(0.0f, MIN(1.0f, value));
	}

	psp->vis_target = value;
}

//
// A_WeaponEnableRadTrig
// A_WeaponDisableRadTrig
//
void A_WeaponEnableRadTrig(mobj_t *mo)
{
	player_t *p = mo->player;
	pspdef_t *psp = &p->psprites[p->action_psp];

	if (psp->state && psp->state->action_par)
	{
		int tag = *(int *)psp->state->action_par;
		RAD_EnableByTag(mo, tag, false);
	}
}

void A_WeaponDisableRadTrig(mobj_t *mo)
{
	player_t *p = mo->player;
	pspdef_t *psp = &p->psprites[p->action_psp];

	if (psp->state && psp->state->action_par)
	{
		int tag = *(int *)psp->state->action_par;
		RAD_EnableByTag(mo, tag, true);
	}
}

