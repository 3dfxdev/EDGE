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

// returns true if first attack gets filled
bool P_FillNewWeapon(player_t *p, int idx)
{
	bool result = false;

	weapondef_c *info = p->weapons[idx].info;

	if (info->ammo[0] != AM_NoAmmo && info->clip_size[0] != 1)
	{
		if (info->clip_size[0] <= p->ammo[info->ammo[0]].num)
		{
			p->weapons[idx].clip_size[0] = info->clip_size[0];
			p->ammo[info->ammo[0]].num  -= info->clip_size[0];

			result = true;
		}
	}

	// same for second attack
	if (info->ammo[1] != AM_NoAmmo && info->clip_size[1] != 1)
	{
		if (info->clip_size[1] <= p->ammo[info->ammo[1]].num)
		{
			p->weapons[idx].clip_size[1] = info->clip_size[1];
			p->ammo[info->ammo[1]].num  -= info->clip_size[1];
		}
	}

	return result;
}

static void ReloadWeapon(player_t *p, int idx, int ATK)
{
	weapondef_c *info = p->weapons[idx].info;

	if (info->ammo[ATK] == AM_NoAmmo || info->clip_size[ATK] == 1)
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
	if (info->clip_size[ATK] != 1)
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
	if (info->clip_size[ATK] == 1)
		return can_fire;

	// for clip weapons, cannot reload until clip is empty.
	if (! can_fire)
	{
		return (info->ammo[ATK] == AM_NoAmmo) ||
			   (info->clip_size[ATK] - p->weapons[idx].clip_size[ATK] <=
				p->ammo[info->ammo[ATK]].num);
	}

	return false;
}

static bool WeaponCanPartialReload(player_t *p, int idx, int ATK)
{
	weapondef_c *info = p->weapons[idx].info;

	// for non-clip weapons, assumes we lose some ammo.
	if (info->clip_size[ATK] == 1)
		WeaponCanFire(p, idx, ATK);

	// for clip weapons, cannot reload if clip is full
	return (p->weapons[idx].clip_size[ATK] < info->clip_size[ATK]);
}

// FIXME: doesn't handle first/second attack properly
static bool WeaponTotallyEmpty(player_t *p, int idx, bool no_sec_atk)
{
	weapondef_c *info = p->weapons[idx].info;

	if (info->ammo[0] == AM_NoAmmo)
		return false;

	int total = p->ammo[info->ammo[0]].num;

	if (info->clip_size[0] == 1 && info->ammopershot[0] > total)
		return true;

	// for clip weapons, either need a non-empty clip or enough
	// ammo to fill the clip.
	if (info->clip_size[0] != 1 &&
		info->ammopershot[0] > p->weapons[idx].clip_size[0] &&
		info->clip_size[0] > total)
	{
		return true;
	}

	if (info->attack[1] && ! no_sec_atk)
	{
		if (info->clip_size[1] == 1 && info->ammopershot[1] > total)
			return true;

		// for clip weapons, either need a non-empty clip or enough
		// ammo to fill the clip.
		if (info->clip_size[1] != 1 &&
			info->ammopershot[1] > p->weapons[idx].clip_size[1] &&
			info->clip_size[1] > total)
		{
			return true;
		}
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

	// -AJA- FIXME: probably need to take the _player_ thing out of its
	// attack state.
}

static void GotoEmptyState(player_t *p)
{
	int newstate = p->weapons[p->ready_wp].info->empty_state;

	P_SetPspriteDeferred(p, ps_weapon, newstate);
	P_SetPsprite(p, ps_crosshair, S_NULL);

	// -AJA- FIXME: probably need to take the _player_ thing out of its
	// attack state.
}

static void GotoAttackState(player_t * p, int ATK)
{
	weapondef_c *info = p->weapons[p->ready_wp].info;

	int newstate = 0;

	if (ATK == 0)
	{
		newstate = info->attack_state;

		if (p->remember_atk[0] >= 0)
		{
			newstate = p->remember_atk[0];
			p->remember_atk[0] = -1;
		}
	}
	else
	{
		newstate = info->sa_attack_state;

		if (p->remember_atk[1] >= 0)
		{
			newstate = p->remember_atk[1];
			p->remember_atk[1] = -1;
		}
	}

	if (newstate)
		P_SetPspriteDeferred(p, ps_weapon, newstate);
}

//
// SwitchAway
//
// Not enough ammo to shoot, selects the next weapon to use.
//
static void SwitchAway(player_t * p, int ATK)
{
	weapondef_c *info = p->weapons[p->ready_wp].info;

//!!!!	@@ can reload && reload states && check flag --> goto reload states

	if (! (info->specials[ATK] & WPSP_SwitchAway))
	{
		if (info->empty_state)
			GotoEmptyState(p);
		else 
			GotoReadyState(p);
	}
	else
		P_SelectNewWeapon(p, -100, AM_DontCare);
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

I_Printf("New selection = %d\n", sel);
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

		if (WeaponTotallyEmpty(p, i, false))
			continue;

		if (! P_CheckWeaponSprite(info))
			continue;

		p->pending_wp = (weapon_selection_e)i;
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

	// spawn the gun
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


// !!! FIXME: sx/sy are not set when firing (seems weird, also causes a jerk)

static void BobWeapon(player_t *p, weapondef_c *info)
{
	bool hasjetpack = p->powers[PW_Jetpack] > 0;
	pspdef_t *psp = &p->psprites[p->action_psp];

	// bob the weapon based on movement speed
	if (hasjetpack)
	{
		psp->sx = 1.0f;
		psp->sy = WEAPONTOP;
	}
	else
	{
		angle_t angle = (128 * leveltime) << ANGLETOFINESHIFT;
		psp->sx = 1.0f + p->bob * PERCENT_2_FLOAT(info->swaying) * M_Cos(angle);

		angle &= (ANG180 - 1);
		psp->sy = WEAPONTOP + p->bob * PERCENT_2_FLOAT(info->bobbing) * M_Sin(angle);
	}
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

	weapondef_c *info = p->weapons[p->ready_wp].info;

//!!!!!! DEBUGGING
if ((info->ammo[0] != AM_NoAmmo) && !(leveltime & 0x1F))
{
I_Printf("ammo %d/%d  clip = %d/%d\n", p->ammo[info->ammo[0]].num,
p->ammo[info->ammo[0]].max, info->clip_size[0],
p->weapons[p->ready_wp].clip_size[0]);
}

	if (info->idle && psp->state == &states[info->ready_state])
		S_StartSound(mo, info->idle);

	// check for change if player is dead, put the weapon away
	if (p->pending_wp != WPSEL_NoChange || p->health <= 0)
	{
		// change weapon (pending weapon should already be validated)
		GotoDownState(p);
		return;
	}

	if (ButtonDown(p, 0) != ButtonDown(p, 1))
	{
		for (int ATK = 0; ATK < 2; ATK++)
		{
			if (! ButtonDown(p, ATK))
			{
				p->attackdown[ATK] = false;
				continue;
			}

			// check for fire: the missile launcher and bfg do not auto fire
			if (!p->attackdown[ATK] || info->autofire[ATK])
			{
				p->attackdown[ATK] = true;
				p->flash = false;

				if (WeaponCanFire(p, p->ready_wp, ATK))
					GotoAttackState(p, ATK);
				else
					SwitchAway(p, ATK);
				return;
			}
		}
	}

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
				GotoAttackState(p, ATK);
			else
				SwitchAway(p, ATK);
			return;
		}
	}

	p->refire = info->refire_inacc ? 0 : 1;

	if (! WeaponCanFire(p, p->ready_wp, ATK))
		SwitchAway(p, ATK);
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
				SwitchAway(p, ATK);
			return;
		}
	}

	p->refire = info->refire_inacc ? 0 : 1;
	p->remember_atk[0] = does_return ? psp->state->nextstate : -1;

	if (WeaponCanFire(p, p->ready_wp, ATK))
		GotoReadyState(p);
	else
		SwitchAway(p, ATK);
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
// For weapons with a clip, only reloads when clip_size is 0 (and enough
// ammo available to fill it).  For non-clip weapons, reloads when
// enough ammo to fire exists in the "ammo bucket" (for NO_AMMO weapons,
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

I_Printf("A_CheckReload: ammo %d  clip %d\n", p->ammo[p->weapons[p->ready_wp].info->ammo[0]].num, p->weapons[p->ready_wp].clip_size[0]);

	if (WeaponCanFire(p, p->ready_wp, ATK))
		return;

	if (! WeaponCanReload(p, p->ready_wp, ATK))
	{
		SwitchAway(p, ATK);
		return;
	}

	weapondef_c *info = p->weapons[p->ready_wp].info;

	ReloadWeapon(p, p->ready_wp, ATK);

	if (ATK == 1 && info->sa_reload_state)
	{
		P_SetPspriteDeferred(p, ps_weapon, info->sa_reload_state);
		return;
	}

	// allow second attack to fall-back on normal reload states.

	if (info->reload_state)
		P_SetPspriteDeferred(p, ps_weapon, info->reload_state);
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

	if (level_flags.limit_zoom && viewiszoomed)
	{
		// In `LimitZoom' mode, disable any current zoom
		R_SetFOV(normalfov);
		viewiszoomed = false;
	}

	psp->sy += LOWERSPEED;

	// Is already down.
	if (psp->sy < WEAPONBOTTOM)
		return;

	// Player is dead.
	if (p->playerstate == PST_DEAD)
	{
		psp->sy = WEAPONBOTTOM;

		// don't bring weapon back up
		return;
	}

	// The old weapon has been lowered off the screen,
	// so change the weapon and start raising it
	if (p->health <= 0)
	{
		// Player is dead, so keep the weapon off screen.
		P_SetPsprite(p, ps_weapon, S_NULL);
		return;
	}

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

	psp->sy -= RAISESPEED;

	if (psp->sy > WEAPONTOP)
		return;

	psp->sy = WEAPONTOP;

	p->remember_atk[0] = -1;
	p->remember_atk[1] = -1;

	// The weapon has been raised all the way,
	//  so change to the ready state.
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

		P_SetPspriteDeferred(p, ps_flash, ATK ? info->sa_flash_state :
			info->flash_state);

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
		if (info->clip_size[ATK] != 1)
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

	if (level_flags.kicking && ATK == 0)  // FIXME: put in specials
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

	int flash_state = ATK ? info->sa_flash_state : info->flash_state;

	if (flash_state && !p->flash)
	{
		p->flash = true;
		P_SetPspriteDeferred(p, ps_flash, flash_state);
	}

	// wake up monsters
	if (! (info->specials[ATK] & WPSP_SilentToMon) &&
		! (info->attack[ATK]->flags & AF_SilentToMon))
	{
		P_NoiseAlert(p);
	}
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
void A_Light0(mobj_t * mo)
{
	mo->player->extralight = 0;
}

void A_Light1(mobj_t * mo)
{
	mo->player->extralight = 1;
}

void A_Light2(mobj_t * mo)
{
	mo->player->extralight = 2;
}

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

