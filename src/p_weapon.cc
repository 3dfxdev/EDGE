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
	pspdef_t *psp;
	state_t *st;

	psp = &p->psprites[position];

	if (stnum == S_NULL)
	{
		// object removed itself
		psp->state = psp->next_state = NULL;
		return;
	}

	st = &states[stnum];

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
// P_CheckWeaponSprite
//
// returns true if the sprite(s) for the weapon exist.  Prevents being
// able to e.g. select the super shotgun when playing with a DOOM 1
// IWAD (and cheating).
//
// -KM- 1998/12/16 Added check to make sure sprites exist.
// -AJA- 2000: Made into a separate routine.
//
bool P_CheckWeaponSprite(weaponinfo_t *info)
{
	if (info->up_state == S_NULL)
		return false;

	return DDF_CheckSprites(info->first_state, info->last_state);
}

//
// P_RefillClips
//
void P_RefillClips(player_t * p)
{
	playerweapon_t *pw;
	weaponinfo_t *info;

	if (p->ready_wp == WPSEL_None)
		return;

	pw = &p->weapons[p->ready_wp];
	info = pw->info;

	if (pw->clip_size <= 0)
		pw->clip_size = info->clip;

	if (pw->sa_clip_size <= 0)
		pw->sa_clip_size = info->sa_clip;

	// make sure clips don't surpass total ammo

	if (info->ammo != AM_NoAmmo && 
			pw->clip_size > p->ammo[info->ammo].num)
	{
		pw->clip_size = p->ammo[info->ammo].num;
	}

	if (info->sa_ammo != AM_NoAmmo && 
			pw->sa_clip_size > p->ammo[info->sa_ammo].num)
	{
		pw->sa_clip_size = p->ammo[info->sa_ammo].num;
	}
}

//
// P_BringUpWeapon
//
// Starts bringing the pending weapon up
// from the bottom of the screen.
//
void P_BringUpWeapon(player_t * p)
{
	weaponinfo_t *info;
	weapon_selection_e sel;

	DEV_ASSERT2(p->pending_wp != WPSEL_NoChange);

	sel = p->ready_wp = p->pending_wp;

	p->pending_wp = WPSEL_NoChange;
	p->psprites[ps_weapon].sy = WEAPONBOTTOM;

	p->remember_atk1 = -1;
	p->remember_atk2 = -1;

	if (sel == WPSEL_None)
	{
		p->attackdown = false;
		p->secondatk_down = false;

		P_SetPsprite(p, ps_weapon, S_NULL);
		P_SetPsprite(p, ps_flash,  S_NULL);

		if (viewiszoomed)
			R_SetFOV(zoomedfov);
		return;
	}

	info = p->weapons[sel].info;

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

	P_SetPsprite(p, ps_weapon, info->up_state);
	P_SetPsprite(p, ps_flash,  S_NULL);

	p->refire = info->refire_inacc ? 0 : 1;

	// refill clips if necessary
	P_RefillClips(p);
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
void P_SelectNewWeapon(player_t * p, int priority, ammotype_e ammo)
{
	int i;
	int key = -1;
	weaponinfo_t *info;

	for (i=0; i < MAXWEAPONS; i++)
	{
		info = p->weapons[i].info;

		if (! p->weapons[i].owned)
			continue;

		if (info->dangerous || info->priority < priority)
			continue;

		if (ammo != AM_DontCare && info->ammo != ammo)
			continue;

		if (ammo == AM_DontCare && !(info->ammo == AM_NoAmmo || 
					p->ammo[info->ammo].num >= info->ammopershot))
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

		for (i=0; i < weaponkey[key].numchoices; i++)
		{
			if (weaponkey[key].choices[i] == info)
			{
				p->key_choices[key] = i;
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
static bool CheckAmmo(player_t * p)
{
	weaponinfo_t *info;

	if (p->ready_wp == WPSEL_None)
		return false;

	info = p->weapons[p->ready_wp].info;

	// Some do not need ammunition anyway.
	// Return if current ammunition sufficient.
	if (info->ammo == AM_NoAmmo || p->ammo[info->ammo].num >=
			info->ammopershot)
		return true;

	P_SelectNewWeapon(p, -100, AM_DontCare);

	return false;
}

//
// CheckAmmoSA
//
static bool CheckAmmoSA(player_t * p)
{
	weaponinfo_t *info;

	if (p->ready_wp == WPSEL_None)
		return false;

	info = p->weapons[p->ready_wp].info;

	// Some do not need ammunition anyway.
	// Return if current ammunition sufficient.
	if (info->sa_ammo == AM_NoAmmo ||
			p->ammo[info->sa_ammo].num >= info->sa_ammopershot)
		return true;

	// Note Well: we don't bother selecting a new weapon

	return false;
}

//
// P_FireWeapon.
//
// -AJA- 1999/08/10: Reworked for multiple attacks.
//
static void P_FireWeapon(player_t * p)
{
	statenum_t newstate;

	if (!CheckAmmo(p))
		return;

	DEV_ASSERT2(p->ready_wp >= 0);

	p->flash = false;

	if (p->remember_atk1 >= 0)
	{
		newstate = p->remember_atk1;
		p->remember_atk1 = -1;
	}
	else
	{
		newstate = p->weapons[p->ready_wp].info->attack_state;
	}

	P_SetPsprite(p, ps_weapon, newstate);

	if (! (p->weapons[p->ready_wp].info->special_flags &
				WPSP_SilentToMonsters))
	{
		P_NoiseAlert(p);
	}
}

//
// P_FireSecondAttack
//
// -AJA- 2000/02/08: written (hastily).
//
static void P_FireSecondAttack(player_t * p)
{
	statenum_t newstate;

	if (!CheckAmmoSA(p))
		return;

	DEV_ASSERT2(p->ready_wp >= 0);

	p->flash = false;

	if (p->remember_atk2 >= 0)
	{
		newstate = p->remember_atk2;
		p->remember_atk2 = -1;
	}
	else
	{
		newstate = p->weapons[p->ready_wp].info->sa_attack_state;
	}

	if (!newstate)
		return;

	P_SetPsprite(p, ps_weapon, newstate);

	if (! (p->weapons[p->ready_wp].info->special_flags &
				WPSP_SilentToMonsters))
	{
		P_NoiseAlert(p);
	}
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
		P_SetPsprite(p, ps_weapon, 
				p->weapons[p->ready_wp].info->down_state);
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
	bool hasjetpack = p->powers[PW_Jetpack] > 0;
	pspdef_t *psp = &p->psprites[p->action_psp];

	statenum_t newstate;
	angle_t angle;
	weaponinfo_t *w = p->weapons[p->ready_wp].info;

	int fire_1 = (p->cmd.buttons & BT_ATTACK);
	int fire_2 = (p->cmd.extbuttons & EBT_SECONDATK);

	DEV_ASSERT2(p->ready_wp >= 0);

	if (w->idle && psp->state == &states[w->ready_state])
		S_StartSound(mo, w->idle);

	// check for change if player is dead, put the weapon away
	if (p->pending_wp != WPSEL_NoChange || !p->health)
	{
		// change weapon (pending weapon should already be validated)
		newstate = p->weapons[p->ready_wp].info->down_state;
		P_SetPsprite(p, ps_weapon, newstate);
		P_SetPsprite(p, ps_crosshair, S_NULL);
		return;
	}

	// check for fire: the missile launcher and bfg do not auto fire
	if (fire_1 && !fire_2)
	{
		if (!p->attackdown || w->autofire)
		{
			p->attackdown = true;
			P_FireWeapon(p);
			return;
		}
	}
	else
		p->attackdown = false;

	if (fire_2 && !fire_1)
	{
		if (!p->secondatk_down || w->sa_autofire)
		{
			p->secondatk_down = true;
			P_FireSecondAttack(p);
			return;
		}
	}
	else
		p->secondatk_down = false;

	// bob the weapon based on movement speed
	if (hasjetpack)
	{
		psp->sx = 1.0f;
		psp->sy = WEAPONTOP;
	}
	else
	{
		angle = (128 * leveltime) << ANGLETOFINESHIFT;
		psp->sx = 1.0f + p->bob * PERCENT_2_FLOAT(w->swaying) * M_Cos(angle);

		angle &= (ANG180 - 1);
		psp->sy = WEAPONTOP + p->bob * PERCENT_2_FLOAT(w->bobbing) * M_Sin(angle);
	}
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

	weaponinfo_t *w = p->weapons[p->ready_wp].info;

	DEV_ASSERT2(p->ready_wp >= 0);

	p->remember_atk1 = -1;

	// check for fire
	// (if a weaponchange is pending, let it go through instead)

	if (p->cmd.buttons & BT_ATTACK)
	{
		// -KM- 1999/01/31 Check for semiautomatic weapons.
		if (p->pending_wp < 0 && p->health && 
				(!p->attackdown || w->autofire))
		{
			p->refire++;
			P_FireWeapon(p);
			return;
		}
	}

	p->refire = w->refire_inacc ? 0 : 1;
	CheckAmmo(p);
}

//
// A_ReFireSA
//
void A_ReFireSA(mobj_t * mo)
{
	player_t *p = mo->player;

	weaponinfo_t *w = p->weapons[p->ready_wp].info;

	DEV_ASSERT2(p->ready_wp >= 0);

	p->remember_atk2 = -1;

	// check for fire
	// (if a weaponchange is pending, let it go through instead)

	if (p->cmd.extbuttons & EBT_SECONDATK)
	{
		if (p->pending_wp < 0 && p->health && 
				(!p->secondatk_down || w->sa_autofire))
		{
			p->refire++;
			P_FireSecondAttack(p);
			return;
		}
	}

	p->refire = w->refire_inacc ? 0 : 1;
	CheckAmmoSA(p);
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
	pspdef_t *psp = &p->psprites[p->action_psp];

	weaponinfo_t *w = p->weapons[p->ready_wp].info;

	DEV_ASSERT2(p->ready_wp >= 0);

	// check for fire
	//  (if a weaponchange is pending, let it go through instead)

	if (p->cmd.buttons & BT_ATTACK)
	{
		// -KM- 1999/01/31 Check for semiautomatic weapons.
		if (p->pending_wp < 0 && p->health && 
				(!p->attackdown || w->autofire))
		{
			p->remember_atk1 = psp->state->nextstate;
			p->refire++;

			P_FireWeapon(p);
			return;
		}
	}

	p->refire = w->refire_inacc ? 0 : 1;
	p->remember_atk1 = -1;

	if (CheckAmmo(p))
	{
		// Player not firing, and no need to change weapon.
		// Therefore return weapon to ready state.

		int newstate = p->weapons[p->ready_wp].info->ready_state;

		P_SetPsprite(p, ps_weapon, newstate);
		P_SetPsprite(p, ps_crosshair, p->weapons[p->ready_wp].info->crosshair);

		// -AJA- FIXME: probably need to take the _player_ thing out of its
		// attack state.
	}
}

//
// A_NoFireSA
//
void A_NoFireSA(mobj_t * mo)
{
	player_t *p = mo->player;
	pspdef_t *psp = &p->psprites[p->action_psp];

	weaponinfo_t *w = p->weapons[p->ready_wp].info;

	DEV_ASSERT2(p->ready_wp >= 0);

	// check for fire
	// (if a weaponchange is pending, let it go through instead)

	if (p->cmd.extbuttons & EBT_SECONDATK)
	{
		if (p->pending_wp < 0 && p->health && 
				(!p->secondatk_down || w->sa_autofire))
		{
			p->remember_atk2 = psp->state->nextstate;
			p->refire++;

			P_FireSecondAttack(p);
			return;
		}
	}

	p->refire = w->refire_inacc ? 0 : 1;
	p->remember_atk2 = -1;

	if (CheckAmmoSA(p))
	{
		// Player not firing, and no need to change weapon.
		// Therefore return weapon to ready state.

		int newstate = p->weapons[p->ready_wp].info->ready_state;

		P_SetPsprite(p, ps_weapon, newstate);
		P_SetPsprite(p, ps_crosshair, p->weapons[p->ready_wp].info->crosshair);

		// -AJA- FIXME: probably need to take the _player_ thing out of its
		// attack state.
	}
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
	pspdef_t *psp = &p->psprites[p->action_psp];

	weaponinfo_t *w = p->weapons[p->ready_wp].info;

	DEV_ASSERT2(p->ready_wp >= 0);

	p->remember_atk1 = psp->state->nextstate;

	// check for fire
	//  (if a weaponchange is pending, let it go through instead)

	if (p->cmd.buttons & BT_ATTACK)
	{
		// -KM- 1999/01/31 Check for semiautomatic weapons.
		if (p->pending_wp < 0 && p->health && 
				(!p->attackdown || w->autofire))
		{
			p->refire++;
			P_FireWeapon(p);
			return;
		}
	}

	p->refire = w->refire_inacc ? 0 : 1;

	if (CheckAmmo(p))
	{
		// Player not firing, and no need to change weapon.
		// Therefore return weapon to ready state.

		int newstate = p->weapons[p->ready_wp].info->ready_state;

		P_SetPsprite(p, ps_weapon, newstate);
		P_SetPsprite(p, ps_crosshair, p->weapons[p->ready_wp].info->crosshair);

		// -AJA- FIXME: probably need to take the _player_ thing out of its
		// attack state.
	}
}

//
// A_NoFireReturnSA
//
// -AJA- 2001/05/14: written.
//
void A_NoFireReturnSA(mobj_t * mo)
{
	player_t *p = mo->player;
	pspdef_t *psp = &p->psprites[p->action_psp];

	weaponinfo_t *w = p->weapons[p->ready_wp].info;

	DEV_ASSERT2(p->ready_wp >= 0);

	p->remember_atk2 = psp->state->nextstate;

	// check for fire
	//  (if a weaponchange is pending, let it go through instead)

	if (p->cmd.extbuttons & EBT_SECONDATK)
	{
		// -KM- 1999/01/31 Check for semiautomatic weapons.
		if (p->pending_wp < 0 && p->health && 
				(!p->secondatk_down || w->sa_autofire))
		{
			p->refire++;
			P_FireSecondAttack(p);
			return;
		}
	}

	p->refire = w->refire_inacc ? 0 : 1;

	if (CheckAmmoSA(p))
	{
		// Player not firing, and no need to change weapon.
		// Therefore return weapon to ready state.

		int newstate = p->weapons[p->ready_wp].info->ready_state;

		P_SetPsprite(p, ps_weapon, newstate);
		P_SetPsprite(p, ps_crosshair, p->weapons[p->ready_wp].info->crosshair);

		// -AJA- FIXME: probably need to take the _player_ thing out of its
		// attack state.
	}
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

	DEV_ASSERT2(p->ready_wp >= 0);

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

	weaponinfo_t *info;

	DEV_ASSERT2(p->ready_wp >= 0);

	CheckAmmo(p);

	if (p->ready_wp == WPSEL_None || p->pending_wp >= 0)
		return;

	info = p->weapons[p->ready_wp].info;

	if (!info->reload_state)
		return;

	if (p->weapons[p->ready_wp].clip_size <= 0)
	{
		P_RefillClips(p);
		P_SetPsprite(p, ps_weapon, info->reload_state);
	}
}

//
// A_CheckReloadSA
//
void A_CheckReloadSA(mobj_t * mo)
{
	player_t *p = mo->player;

	weaponinfo_t *info;

	DEV_ASSERT2(p->ready_wp >= 0);

	CheckAmmoSA(p);

	if (p->ready_wp == WPSEL_None || p->pending_wp >= 0)
		return;

	info = p->weapons[p->ready_wp].info;

	if (!info->sa_reload_state)
		return;

	if (p->weapons[p->ready_wp].sa_clip_size <= 0)
	{
		P_RefillClips(p);
		P_SetPsprite(p, ps_weapon, info->sa_reload_state);
	}
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

	psp->sy += LOWERSPEED;

	DEV_ASSERT2(p->ready_wp >= 0);

	if (level_flags.limit_zoom && viewiszoomed)
	{
		// In `LimitZoom' mode, disable any current zoom
		R_SetFOV(normalfov);
		viewiszoomed = false;
	}

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
	if (!p->health)
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

	statenum_t newstate;

	DEV_ASSERT2(p->ready_wp >= 0);

	psp->sy -= RAISESPEED;

	if (psp->sy > WEAPONTOP)
		return;

	psp->sy = WEAPONTOP;

	p->remember_atk1 = -1;
	p->remember_atk2 = -1;

	// The weapon has been raised all the way,
	//  so change to the ready state.
	newstate = p->weapons[p->ready_wp].info->ready_state;

	P_SetPsprite(p, ps_weapon, newstate);
	P_SetPsprite(p, ps_crosshair, p->weapons[p->ready_wp].info->crosshair);
}

//
// A_SetCrosshair
//
void A_SetCrosshair(mobj_t * mo)
{
#if 0  // !! FIXME
	player_t *p = mo->player;
	pspdef_t *psp = &p->psprites[p->action_psp];

	DEV_ASSERT2(p->ready_wp >= 0);

	P_SetPsprite(p, ps_crosshair,
			p->weapons[p->ready_wp].info->crosshair + psp->state->misc1);
#endif
}

void A_GotTarget(mobj_t * mo)
{
#if 0  // !! FIXME
	player_t *p = mo->player;
	pspdef_t *psp = &p->psprites[p->action_psp];

	attacktype_t *attack = p->weapons[p->ready_wp].info->attack;
	mobj_t *obj;

	DEV_ASSERT2(p->ready_wp >= 0);

	obj = P_MapTargetAutoAim(mo, mo->angle, attack->range,
			attack->flags & AF_ForceAim);

	if (obj->extendedflags & EF_DUMMYMOBJ)
		obj = P_MapTargetAutoAim(mo, mo->angle + (1 << 26),
				attack->range, attack->flags & AF_ForceAim);

	if (obj->extendedflags & EF_DUMMYMOBJ)
		obj = P_MapTargetAutoAim(mo, mo->angle - (1 << 26),
				attack->range, attack->flags & AF_ForceAim);

	if (obj->extendedflags & EF_DUMMYMOBJ)
		P_SetPsprite(p, ps_crosshair,
				p->weapons[p->ready_wp].info->crosshair + psp->state->misc1);
	else
		P_SetPsprite(p, ps_crosshair,
				p->weapons[p->ready_wp].info->crosshair + psp->state->misc2);
#endif
}

//
// A_GunFlash
//
void A_GunFlash(mobj_t * mo)
{
	player_t *p = mo->player;

	DEV_ASSERT2(p->ready_wp >= 0);

	if (!p->flash)
	{
		if (mo->info->missile_state)
			P_SetMobjState(mo, mo->info->missile_state);

		P_SetPsprite(p, ps_flash, p->weapons[p->ready_wp].
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

	DEV_ASSERT2(p->ready_wp >= 0);

	if (!p->flash)
	{
		if (mo->info->missile_state)
			P_SetMobjState(mo, mo->info->missile_state);

		P_SetPsprite(p, ps_flash, p->weapons[p->ready_wp].
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

	weaponinfo_t *w = p->weapons[p->ready_wp].info;
	attacktype_t *attack = w->attack;
	ammotype_e ammo;
	int count;

	DEV_ASSERT2(p->ready_wp >= 0);

	// -AJA- 1999/08/10: Multiple attack support.
	if (psp->state && psp->state->action_par)
		attack = (attacktype_t *) psp->state->action_par;

	ammo = w->ammo;

	// Minimal amount for one shot varies.
	count = w->ammopershot;

	// Some do not need ammunition anyway.
	// Return if current ammunition sufficient.
	if (!(ammo == AM_NoAmmo || p->ammo[ammo].num >= count))
		return;

	// -AJA- 1999/08/11: Increase fire count.
	p->weapons[p->ready_wp].clip_size--;

	P_ActPlayerAttack(mo, attack);

	if (level_flags.kicking)
	{
		p->deltaviewheight -= w->kick;
		p->kick_offset = w->kick;
	}

	if (mo->target && !(mo->target->extendedflags & EF_DUMMYMOBJ))
	{
		if (w->hit)
			S_StartSound(mo, w->hit);

		if (w->feedback)
			mo->flags |= MF_JUSTATTACKED;
	}
	else
	{
		if (w->engaged)
			S_StartSound(mo, w->engaged);
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

	if (ammo != AM_NoAmmo)
		p->ammo[ammo].num -= count;

	if (w->flash_state && !p->flash)
	{
		P_SetPsprite(p, ps_flash, w->flash_state);
		p->flash = true;
	}
}

void A_WeaponShootSA(mobj_t * mo)
{
	player_t *p = mo->player;
	pspdef_t *psp = &p->psprites[p->action_psp];

	weaponinfo_t *w = p->weapons[p->ready_wp].info;
	attacktype_t *attack = w->sa_attack;
	ammotype_e ammo;
	int count;

	DEV_ASSERT2(p->ready_wp >= 0);

	// -AJA- 1999/08/10: Multiple attack support.
	if (psp->state && psp->state->action_par)
		attack = (attacktype_t *) psp->state->action_par;

	ammo = w->sa_ammo;

	// Minimal amount for one shot varies.
	count = w->sa_ammopershot;

	// Some do not need ammunition anyway.
	// Return if current ammunition sufficient.
	if (!(ammo == AM_NoAmmo || p->ammo[ammo].num >= count))
		return;

	// -AJA- 1999/08/11: Increase fire count.
	p->weapons[p->ready_wp].sa_clip_size--;

	P_ActPlayerAttack(mo, attack);

	if (mo->target && !(mo->target->extendedflags & EF_DUMMYMOBJ))
	{
		if (w->hit)
			S_StartSound(mo, w->hit);

		if (w->feedback)
			mo->flags |= MF_JUSTATTACKED;
	}
	else
	{
		if (w->engaged)
			S_StartSound(mo, w->engaged);
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

	if (ammo != AM_NoAmmo)
		p->ammo[ammo].num -= count;

	if (w->sa_flash_state && !p->flash)
	{
		P_SetPsprite(p, ps_flash, w->sa_flash_state);
		p->flash = true;
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

	weaponinfo_t *w = p->weapons[p->ready_wp].info;
	attacktype_t *attack = w->eject_attack;

	DEV_ASSERT2(p->ready_wp >= 0);

	if (psp->state && psp->state->action_par)
		attack = (attacktype_t *) psp->state->action_par;

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

	DEV_ASSERT2(p->ready_wp >= 0);

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

	DEV_ASSERT2(p->ready_wp >= 0);
	S_StartSound(mo, p->weapons[p->ready_wp].info->sound1);
}

void A_SFXWeapon2(mobj_t * mo)
{
	player_t *p = mo->player;

	DEV_ASSERT2(p->ready_wp >= 0);
	S_StartSound(mo, p->weapons[p->ready_wp].info->sound2);
}

void A_SFXWeapon3(mobj_t * mo)
{
	player_t *p = mo->player;

	DEV_ASSERT2(p->ready_wp >= 0);
	S_StartSound(mo, p->weapons[p->ready_wp].info->sound3);
}

//
// These three routines make a flash of light when a weapon fires.
//
void A_Light0(mobj_t * mo)
{
	player_t *p = mo->player;

	DEV_ASSERT2(p->ready_wp >= 0);
	p->extralight = 0;
}

void A_Light1(mobj_t * mo)
{
	player_t *p = mo->player;

	DEV_ASSERT2(p->ready_wp >= 0);
	p->extralight = 1;
}

void A_Light2(mobj_t * mo)
{
	player_t *p = mo->player;

	DEV_ASSERT2(p->ready_wp >= 0);
	p->extralight = 2;
}

//
// A_WeaponJump
//
void A_WeaponJump(mobj_t * mo)
{
	player_t *p = mo->player;
	pspdef_t *psp = &p->psprites[p->action_psp];
	weaponinfo_t *w = p->weapons[p->ready_wp].info;

	act_jump_info_t *jump;

	DEV_ASSERT2(p->ready_wp >= 0);

	if (!psp->state || !psp->state->action_par)
	{
		M_WarnError("JUMP used in weapon [%s] without a label !\n",
				w->ddf.name);
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

	DEV_ASSERT2(p->ready_wp >= 0);

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

	DEV_ASSERT2(p->ready_wp >= 0);

	if (psp->state && psp->state->action_par)
	{
		value = ((percent_t *) psp->state->action_par)[0];
		value = MAX(0.0f, MIN(1.0f, value));
	}

	psp->vis_target = value;
}

//
// P_SetupPsprites
//
// Called at start of level for each player.
//
void P_SetupPsprites(player_t * p)
{
	int i;

	// remove all psprites
	for (i = 0; i < NUMPSPRITES; i++)
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
	int i;
	pspdef_t *psp;
	int loop_count;

	// check if player has NO weapon but wants to change
	if (! p->psprites[ps_weapon].state &&
			p->pending_wp != WPSEL_NoChange)
	{
		P_BringUpWeapon(p);
	}

	psp = &p->psprites[0];

	for (i = 0; i < NUMPSPRITES; i++, psp++)
	{
		// a null state means not active
		if (! psp->state)
			continue;

		for (loop_count=0; loop_count < MAX_PSP_LOOP; loop_count++)
		{
			// drop tic count and possibly change state
			// Note: a -1 tic count never changes.
			if (psp->tics < 0)
				break;

			psp->tics--;

			if (psp->tics >= 1)
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
// A_WeaponEnableRadTrig
// A_WeaponDisableRadTrig
//
void A_WeaponEnableRadTrig(mobj_t *mo)
{
	player_t *p = mo->player;
	pspdef_t *psp = &p->psprites[p->action_psp];

	DEV_ASSERT2(p->ready_wp >= 0);

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

	DEV_ASSERT2(p->ready_wp >= 0);

	if (psp->state && psp->state->action_par)
	{
		int tag = *(int *)psp->state->action_par;
		RAD_EnableByTag(mo, tag, true);
	}
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
