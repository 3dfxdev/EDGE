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
// -AJA- 2004/11/05: This is preferred method, doesn't invoke actions,
//       which (ideally) only occurs during P_MovePsprites().
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

// returns true if first attack gets filled
bool P_FillNewWeapon(player_t *p, int idx)
{
	bool result = false;

	weapondef_c *info = p->weapons[idx].info;

	if (info->ammo != AM_NoAmmo && info->clip_size > 1)
	{
		if (info->clip_size <= p->ammo[info->ammo].num)
		{
			p->weapons[idx].clip_size = info->clip_size;
			p->ammo[info->ammo].num  -= info->clip_size;

			result = true;
		}
	}

	// same for second attack
	if (info->sa_ammo != AM_NoAmmo && info->sa_clip_size > 1)
	{
		if (info->sa_clip_size <= p->ammo[info->sa_ammo].num)
		{
			p->weapons[idx].sa_clip_size = info->sa_clip_size;
			p->ammo[info->sa_ammo].num  -= info->sa_clip_size;
		}
	}

	return result;
}

static void ReloadWeapon(player_t *p, int idx, int attack)
{
	weapondef_c *info = p->weapons[idx].info;

	if (attack == 1)
	{
		if (info->ammo != AM_NoAmmo && info->clip_size > 1)
		{
			int qty = info->clip_size - p->weapons[idx].clip_size;

			DEV_ASSERT2(qty > 0);
			DEV_ASSERT2(qty <= p->ammo[info->ammo].num);

			p->weapons[idx].clip_size += qty;
			p->ammo[info->ammo].num -= qty;
		}
	}
	else
	{
		if (info->sa_ammo != AM_NoAmmo && info->sa_clip_size > 1)
		{
			int qty = info->sa_clip_size - p->weapons[idx].sa_clip_size;

			DEV_ASSERT2(qty > 0);
			DEV_ASSERT2(qty <= p->ammo[info->sa_ammo].num);

			p->weapons[idx].sa_clip_size += qty;
			p->ammo[info->sa_ammo].num -= qty;
		}
	}
}

static bool WeaponCanFire(player_t *p, int idx, int attack)
{
	weapondef_c *info = p->weapons[idx].info;

	if (attack == 1)
	{
		if (info->ammo == AM_NoAmmo)
			return true;

		if (info->clip_size <= 1)
			return (info->ammopershot <= p->ammo[info->ammo].num);
		else
			return (info->ammopershot <= p->weapons[idx].clip_size);
	}
	else
	{
		if (info->sa_ammo == AM_NoAmmo)
			return true;

		if (info->sa_clip_size <= 1)
			return (info->sa_ammopershot <= p->ammo[info->sa_ammo].num);
		else
			return (info->sa_ammopershot <= p->weapons[idx].clip_size);
	}
}

static bool WeaponCanReload(player_t *p, int idx, int attack)
{
	weapondef_c *info = p->weapons[idx].info;

	if (attack == 1)
	{
		if (info->ammo == AM_NoAmmo || info->clip_size <= 1)
			return WeaponCanFire(p, idx, attack);

I_Printf("CAN-RELOAD: ammo %d  clip %d\n", p->ammo[info->ammo].num, p->weapons[idx].clip_size);
		int qty = info->clip_size - p->weapons[idx].clip_size;
		return (qty <= p->ammo[info->ammo].num);
	}
	else
	{
		if (info->sa_ammo == AM_NoAmmo && info->sa_clip_size <= 1)
			return WeaponCanFire(p, idx, attack);

		int qty = info->sa_clip_size - p->weapons[idx].sa_clip_size;
		return (qty <= p->ammo[info->sa_ammo].num);
	}
}

static bool WeaponTotallyEmpty(player_t *p, int idx, bool no_sec_atk)
{
	weapondef_c *info = p->weapons[idx].info;

	if (info->ammo == AM_NoAmmo)
		return false;

	int total = p->ammo[info->ammo].num;

	if (info->clip_size <= 1 && info->ammopershot > total)
		return true;

	// for clip weapons, either need a non-empty clip or enough
	// ammo to fill the clip.
	if (info->clip_size > 1 &&
		info->ammopershot > p->weapons[idx].clip_size &&
		info->clip_size > total)
	{
		return true;
	}

	if (info->sa_attack && ! no_sec_atk)
	{
		if (info->sa_clip_size <= 1 && info->sa_ammopershot > total)
			return true;

		// for clip weapons, either need a non-empty clip or enough
		// ammo to fill the clip.
		if (info->sa_clip_size > 1 &&
			info->sa_ammopershot > p->weapons[idx].sa_clip_size &&
			info->sa_clip_size > total)
		{
			return true;
		}
	}

	return false;
}

static void GotoReadyState(player_t *p)
{
	int newstate = p->weapons[p->ready_wp].info->ready_state;

	P_SetPspriteDeferred(p, ps_weapon, newstate);
	P_SetPspriteDeferred(p, ps_crosshair, p->weapons[p->ready_wp].info->crosshair);

	// -AJA- FIXME: probably need to take the _player_ thing out of its
	// attack state.
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

	p->remember_atk1 = -1;
	p->remember_atk2 = -1;

I_Printf("New selection = %d\n", sel);
	if (sel == WPSEL_None)
	{
		p->attackdown = false;
		p->secondatk_down = false;

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

///---	// refill clips if necessary
///---	P_RefillClips(p);
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

		if (ammo != AM_DontCare && info->ammo != ammo)
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
// CheckAmmo
//
// Returns true if there is enough ammo to shoot.
// If not, selects the next weapon to use.
//
static bool CheckAmmoSwitch(player_t * p, int attack)
{
	weapondef_c *info = p->weapons[p->ready_wp].info;

	if (WeaponCanFire(p, p->ready_wp, attack))
		return true;

	if (info->empty_state)
		P_SetPspriteDeferred(p, ps_weapon, info->empty_state);
	else if ((info->special_flags & WPSP_NoAutoSwitch) || attack == 2)
		P_SetPspriteDeferred(p, ps_weapon, info->ready_state);
	else
		P_SelectNewWeapon(p, -100, AM_DontCare);

	return false;
}

static void GotoAttackState(player_t * p, int attack)
{
	weapondef_c *info = p->weapons[p->ready_wp].info;

	statenum_t newstate = 0;

	if (attack == 1)
	{
		newstate = info->attack_state;

		if (p->remember_atk1 >= 0)
		{
			newstate = p->remember_atk1;
			p->remember_atk1 = -1;
		}
	}
	else
	{
		newstate = info->sa_attack_state;

		if (p->remember_atk2 >= 0)
		{
			newstate = p->remember_atk2;
			p->remember_atk2 = -1;
		}
	}

	if (newstate)
		P_SetPspriteDeferred(p, ps_weapon, newstate);
}

//
// P_DropWeapon
//
// Player died, so put the weapon away.
//
void P_DropWeapon(player_t * p)
{
	p->remember_atk1 = -1;
	p->remember_atk2 = -1;

	if (p->ready_wp != WPSEL_None)
		P_SetPspriteDeferred(p, ps_weapon, p->weapons[p->ready_wp].info->down_state);
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

	if (info->empty_state && WeaponTotallyEmpty(p, p->ready_wp, false))
	{
		P_SetPspriteDeferred(p, ps_weapon, info->empty_state);
		return;
	}

//!!!!!! DEBUGGING
if ((info->ammo != AM_NoAmmo) && !(leveltime & 0x1F))
{
I_Printf("ammo %d/%d  clip = %d/%d\n", p->ammo[info->ammo].num,
p->ammo[info->ammo].max, info->clip_size,
p->weapons[p->ready_wp].clip_size);
}

	if (info->idle && psp->state == &states[info->ready_state])
		S_StartSound(mo, info->idle);

	// check for change if player is dead, put the weapon away
	if (p->pending_wp != WPSEL_NoChange || p->health <= 0)
	{
		// change weapon (pending weapon should already be validated)
		statenum_t newstate = p->weapons[p->ready_wp].info->down_state;
		P_SetPspriteDeferred(p, ps_weapon, newstate);
		P_SetPsprite(p, ps_crosshair, S_NULL);
		return;
	}

	int fire_1 = (p->cmd.buttons & BT_ATTACK);
	int fire_2 = (p->cmd.extbuttons & EBT_SECONDATK);

	// check for fire: the missile launcher and bfg do not auto fire
	if (fire_1 && !fire_2)
	{
		if (!p->attackdown || info->autofire)
		{
			p->attackdown = true;
			p->flash = false;

			if (CheckAmmoSwitch(p, 1))
				GotoAttackState(p, 1);
			return;
		}
	}
	else
		p->attackdown = false;

	if (fire_2 && !fire_1)
	{
		if (!p->secondatk_down || info->sa_autofire)
		{
			p->secondatk_down = true;
			p->flash = false;

			if (CheckAmmoSwitch(p, 2))
				GotoAttackState(p, 2);
			return;
		}
	}
	else
		p->secondatk_down = false;

	BobWeapon(p, info);
}

//
// A_WeaponEmpty
//
// The player cannot fire the weapon.  New ammo could cause a reload,
// and manual reload also possible.
//
void A_WeaponEmpty(mobj_t * mo)
{
	player_t *p = mo->player;
	pspdef_t *psp = &p->psprites[p->action_psp];

	weapondef_c *info = p->weapons[p->ready_wp].info;

	if (info->idle && psp->state == &states[info->empty_state])
		S_StartSound(mo, info->idle);

	// check for change if player is dead, put the weapon away
	if (p->pending_wp != WPSEL_NoChange || p->health <= 0)
	{
		// change weapon (pending weapon should already be validated)
		statenum_t newstate = p->weapons[p->ready_wp].info->down_state;
		P_SetPspriteDeferred(p, ps_weapon, newstate);
		P_SetPsprite(p, ps_crosshair, S_NULL);
		return;
	}

	BobWeapon(p, info);
}

//
// A_ReFire
//
// The player can re-fire the weapon
// without lowering it entirely.
//
// -AJA- 1999/08/10: Reworked for multiple attacks.
//
void A_ReFire(mobj_t * mo)
{
	player_t *p = mo->player;

	if (p->pending_wp >= 0 || p->health <= 0)
		return;

	weapondef_c *info = p->weapons[p->ready_wp].info;

	p->remember_atk1 = -1;

	// check for fire
	// (if a weaponchange is pending, let it go through instead)

	if (p->cmd.buttons & BT_ATTACK)
	{
		// -KM- 1999/01/31 Check for semiautomatic weapons.
		if (!p->attackdown || info->autofire)
		{
			p->refire++;
			p->flash = false;

			if (CheckAmmoSwitch(p, 1))
				GotoAttackState(p, 1);
			return;
		}
	}

	p->refire = info->refire_inacc ? 0 : 1;

	CheckAmmoSwitch(p, 1);
}

//
// A_ReFireSA
//
void A_ReFireSA(mobj_t * mo)
{
	player_t *p = mo->player;

	if (p->pending_wp >= 0 || p->health <= 0)
		return;

	weapondef_c *info = p->weapons[p->ready_wp].info;

	p->remember_atk2 = -1;

	// check for fire
	// (if a weaponchange is pending, let it go through instead)

	if (p->cmd.extbuttons & EBT_SECONDATK)
	{
		if (!p->secondatk_down || info->sa_autofire)
		{
			p->refire++;
			p->flash = false;

			if (CheckAmmoSwitch(p, 2))
				GotoAttackState(p, 2);
			return;
		}
	}

	p->refire = info->refire_inacc ? 0 : 1;
}

//
// A_NoFire
//
// If the player is still holding the fire button, continue, otherwise
// return to the weapon ready states.
//
// -AJA- 1999/08/18: written.
//
void A_NoFire(mobj_t * mo)
{
	player_t *p = mo->player;

	if (p->pending_wp >= 0 || p->health <= 0)
		return;

	weapondef_c *info = p->weapons[p->ready_wp].info;

	// check for fire
	//  (if a weaponchange is pending, let it go through instead)

	if (p->cmd.buttons & BT_ATTACK)
	{
		// -KM- 1999/01/31 Check for semiautomatic weapons.
		if (!p->attackdown || info->autofire)
		{
			p->refire++;
			p->flash = false;
			CheckAmmoSwitch(p, 1);
			return;
		}
	}

	p->refire = info->refire_inacc ? 0 : 1;
	p->remember_atk1 = -1;

	if (CheckAmmoSwitch(p, 1))
		GotoReadyState(p);
}

//
// A_NoFireSA
//
void A_NoFireSA(mobj_t * mo)
{
	player_t *p = mo->player;

	if (p->pending_wp >= 0 || p->health <= 0)
		return;

	weapondef_c *info = p->weapons[p->ready_wp].info;

	// check for fire
	// (if a weaponchange is pending, let it go through instead)

	if (p->cmd.extbuttons & EBT_SECONDATK)
	{
		if (!p->secondatk_down || info->sa_autofire)
		{
			p->refire++;
			p->flash = false;
			CheckAmmoSwitch(p, 2);
			return;
		}
	}

	p->refire = info->refire_inacc ? 0 : 1;
	p->remember_atk2 = -1;

	if (CheckAmmoSwitch(p, 2))
		GotoReadyState(p);
}

//
// A_NoFireReturn
//
// Like A_NoFire, but used for multiple attacks.  It remembers the
// position in the attack states when the player stops firing, and
// returns to this position when the player starts firing again.
//
// -AJA- 1999/08/11: written.
//
void A_NoFireReturn(mobj_t * mo)
{
	player_t *p = mo->player;

	if (p->pending_wp >= 0 || p->health <= 0)
		return;

	pspdef_t *psp = &p->psprites[p->action_psp];
	weapondef_c *info = p->weapons[p->ready_wp].info;

	// check for fire
	//  (if a weaponchange is pending, let it go through instead)

	if (p->cmd.buttons & BT_ATTACK)
	{
		// -KM- 1999/01/31 Check for semiautomatic weapons.
		if (!p->attackdown || info->autofire)
		{
			p->refire++;
			p->flash = false;
			CheckAmmoSwitch(p, 1);
			return;
		}
	}

	p->refire = info->refire_inacc ? 0 : 1;
	p->remember_atk1 = psp->state->nextstate;

	if (CheckAmmoSwitch(p, 1))
		GotoReadyState(p);
}

//
// A_NoFireReturnSA
//
// -AJA- 2001/05/14: written.
//
void A_NoFireReturnSA(mobj_t * mo)
{
	player_t *p = mo->player;

	if (p->pending_wp >= 0 || p->health <= 0)
		return;

	pspdef_t *psp = &p->psprites[p->action_psp];
	weapondef_c *info = p->weapons[p->ready_wp].info;

	// check for fire
	//  (if a weaponchange is pending, let it go through instead)

	if (p->cmd.extbuttons & EBT_SECONDATK)
	{
		// -KM- 1999/01/31 Check for semiautomatic weapons.
		if (!p->secondatk_down || info->sa_autofire)
		{
			p->refire++;
			p->flash = false;
			CheckAmmoSwitch(p, 2);
			return;
		}
	}

	p->refire = info->refire_inacc ? 0 : 1;
	p->remember_atk2 = psp->state->nextstate;

	if (CheckAmmoSwitch(p, 2))
		GotoReadyState(p);
}

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
// Check whether the player has used up
// the clip quantity of ammo.  If so, must reload.
//
// -KM- 1999/01/31 Check clip size.
// -AJA- 1999/08/11: Reworked for new playerweapon_t field.
//
void A_CheckReload(mobj_t * mo)
{
	player_t *p = mo->player;

	if (p->pending_wp >= 0 || p->health <= 0)
		return;

I_Printf("A_CheckReload: ammo %d  clip %d\n", p->ammo[p->weapons[p->ready_wp].info->ammo].num, p->weapons[p->ready_wp].clip_size);
	if (WeaponCanFire(p, p->ready_wp, 1))
		return;

	if (! WeaponCanReload(p, p->ready_wp, 1))
	{
		CheckAmmoSwitch(p, 1);
		return;
	}

	weapondef_c *info = p->weapons[p->ready_wp].info;

	ReloadWeapon(p, p->ready_wp, 1);

	if (info->reload_state)
		P_SetPspriteDeferred(p, ps_weapon, info->reload_state);
}

//
// A_CheckReloadSA
//
void A_CheckReloadSA(mobj_t * mo)
{
	player_t *p = mo->player;

	if (p->pending_wp >= 0 || p->health <= 0)
		return;

	if (WeaponCanFire(p, p->ready_wp, 2))
		return;

	if (! WeaponCanReload(p, p->ready_wp, 2))
	{
		CheckAmmoSwitch(p, 2);
		return;
	}

	weapondef_c *info = p->weapons[p->ready_wp].info;

	ReloadWeapon(p, p->ready_wp, 2);

	if (info->sa_reload_state)
		P_SetPspriteDeferred(p, ps_weapon, info->sa_reload_state);
	else if (info->reload_state)
		P_SetPspriteDeferred(p, ps_weapon, info->reload_state);
}

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

	p->remember_atk1 = -1;
	p->remember_atk2 = -1;

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
void A_GunFlash(mobj_t * mo)
{
	player_t *p = mo->player;

	if (!p->flash)
	{
		if (mo->info->missile_state)
			P_SetMobjState(mo, mo->info->missile_state);

		P_SetPspriteDeferred(p, ps_flash, p->weapons[p->ready_wp].
				info->flash_state);
		p->flash = true;
	}
}

//
// A_GunFlashSA
//
void A_GunFlashSA(mobj_t * mo)
{
	player_t *p = mo->player;

	if (!p->flash)
	{
		if (mo->info->missile_state)
			P_SetMobjState(mo, mo->info->missile_state);

		P_SetPspriteDeferred(p, ps_flash, p->weapons[p->ready_wp].
				info->sa_flash_state);
		p->flash = true;
	}
}

//
// WEAPON ATTACKS
//
void A_WeaponShoot(mobj_t * mo)
{
	player_t *p = mo->player;
	pspdef_t *psp = &p->psprites[p->action_psp];

	weapondef_c *info = p->weapons[p->ready_wp].info;
	atkdef_c *attack = info->attack;
	ammotype_e ammo;
	int count;

	// -AJA- 1999/08/10: Multiple attack support.
	if (psp->state && psp->state->action_par)
		attack = (atkdef_c *) psp->state->action_par;

	if (! attack)
		I_Error("Weapon [%s] missing attack.\n", info->ddf.name.GetString());

	ammo = info->ammo;

	// Minimal amount for one shot varies.
	count = info->ammopershot;

	// Some do not need ammunition anyway.
	// Return if current ammunition sufficient.
	if (! WeaponCanFire(p, p->ready_wp, 1))
		return;

	if (ammo != AM_NoAmmo)
	{
		if (info->clip_size > 1)
			p->weapons[p->ready_wp].clip_size -= count;
		else
			p->ammo[ammo].num -= count;
	}

	P_ActPlayerAttack(mo, attack);

	if (level_flags.kicking)
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
		P_SetMobjState(mo, mo->info->melee_state);
	}
	else if (mo->info->missile_state)
	{
		P_SetMobjState(mo, mo->info->missile_state);
	}

	if (info->flash_state && !p->flash)
	{
		P_SetPspriteDeferred(p, ps_flash, info->flash_state);
		p->flash = true;
	}

	// wake up monsters
	if (! (info->special_flags & WPSP_SilentToMon) &&
		! (info->attack->flags & AF_SilentToMon))
	{
		P_NoiseAlert(p);
	}
}

void A_WeaponShootSA(mobj_t * mo)
{
	player_t *p = mo->player;
	pspdef_t *psp = &p->psprites[p->action_psp];

	weapondef_c *info = p->weapons[p->ready_wp].info;
	atkdef_c *attack = info->sa_attack;
	ammotype_e ammo;
	int count;

	// -AJA- 1999/08/10: Multiple attack support.
	if (psp->state && psp->state->action_par)
		attack = (atkdef_c *) psp->state->action_par;

	if (! attack)
		I_Error("Weapon [%s] missing second attack.\n", info->ddf.name.GetString());

	ammo = info->sa_ammo;

	// Minimal amount for one shot varies.
	count = info->sa_ammopershot;

	// Some do not need ammunition anyway.
	// Return if current ammunition sufficient.
	if (! WeaponCanFire(p, p->ready_wp, 2))
		return;

	if (ammo != AM_NoAmmo)
	{
		if (info->sa_clip_size > 1)
			p->weapons[p->ready_wp].sa_clip_size -= count;
		else
			p->ammo[ammo].num -= count;
	}

	P_ActPlayerAttack(mo, attack);

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
		P_SetMobjState(mo, mo->info->melee_state);
	}
	else if (mo->info->missile_state)
	{
		P_SetMobjState(mo, mo->info->missile_state);
	}

	if (info->sa_flash_state && !p->flash)
	{
		P_SetPspriteDeferred(p, ps_flash, info->sa_flash_state);
		p->flash = true;
	}

	// wake up monsters
	if (! (info->special_flags & WPSP_SilentToMon) &&
		! (attack->flags & AF_SilentToMon))
	{
		P_NoiseAlert(p);
	}
}

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

