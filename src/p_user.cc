//----------------------------------------------------------------------------
//  EDGE Player User Code
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

#include "i_defs.h"

#include "ddf_main.h"
#include "ddf_colm.h"
#include "e_event.h"
#include "e_input.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "g_game.h"
#include "m_inline.h"
#include "p_bot.h"
#include "p_local.h"
#include "p_mobj.h"
#include "rad_trig.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "w_wad.h"

#include "z_zone.h"

static void P_UpdatePowerups(player_t *player);

// 16 pixels of bob
#define MAXBOB        16.0f

//
// CalcHeight
//
// Calculate the walking / running height adjustment.
//
static void CalcHeight(player_t * player)
{
	float bob = 0;
	bool onground = player->mo->z <= player->mo->floorz;

	// Regular movement bobbing 
	// (needs to be calculated for gun swing even if not on ground).  
	// -AJA- Moved up here, to prevent weapon jumps when running down
	// stairs.

	player->bob = (player->mo->mom.x * player->mo->mom.x
		+ player->mo->mom.y * player->mo->mom.y) / 8;

	if (player->bob > MAXBOB)
		player->bob = MAXBOB;

	// ----CALCULATE BOB EFFECT----
	if (player->playerstate == PST_LIVE && onground)
	{
		angle_t angle = ANG90 / 5 * leveltime;

		bob = player->bob / 2 * player->mo->info->bobbing * M_Sin(angle);
	}

	// ----CALCULATE VIEWHEIGHT----
	if (player->playerstate == PST_LIVE)
	{
		player->viewheight += player->deltaviewheight;

		if (player->viewheight > player->std_viewheight)
		{
			player->viewheight = player->std_viewheight;
			player->deltaviewheight = 0;
		}
		else if (player->viewheight < player->std_viewheight / 2)
		{
			player->viewheight = player->std_viewheight / 2;

			if (player->deltaviewheight <= 0)
				player->deltaviewheight = 0.01f;
		}

		if (player->deltaviewheight != 0)
		{
			// use a weird number to minimise chance of hitting
			// zero when deltaviewheight goes neg -> positive.
			player->deltaviewheight += 0.24162f;
		}
	}

	player->viewz = player->mo->z + player->viewheight + bob;

	// No heads above the ceiling
	if (player->viewz > player->mo->ceilingz - 2)
		player->viewz = player->mo->ceilingz - 2;

	// No heads below the floor, please
	if (player->viewz < player->mo->floorz + 2)
		player->viewz = player->mo->floorz + 2;
}

//
// MovePlayer
//
static void MovePlayer(player_t * player)
{
	ticcmd_t *cmd;
	mobj_t *mo = player->mo;

	bool onground = player->mo->z <= player->mo->floorz;
	bool onladder = player->mo->on_ladder >= 0;
	bool hasjetpack = player->powers[PW_Jetpack] > 0;
	bool canswim = player->swimming;

	float dx, dy;
	float eh, ev;

	float base_xy_speed;
	float base_z_speed;

	float F_vec[3], U_vec[3], S_vec[3];

	cmd = &player->cmd;

	player->mo->angle += (angle_t)(cmd->angleturn << 16);

	// EDGE Feature: Vertical Look (Mlook)
	//
	// -ACB- 1998/07/02 New Code used, rerouted via Ticcmd
	// -ACB- 1998/07/27 Used defines for look limits.
	//
	if (level_flags.mlook)
	{
		player->mo->vertangle += (angle_t)(cmd->mlookturn << 16);

		if (player->mo->vertangle > LOOKUPLIMIT && player->mo->vertangle < LOOKDOWNLIMIT)
		{
			if (player->mo->vertangle <= ANG180)
				player->mo->vertangle = LOOKUPLIMIT;
			else
				player->mo->vertangle = LOOKDOWNLIMIT;
		}
	}

	// compute XY and Z speeds, taking swimming (etc) into account
	// (we try to swim in view direction -- assumes no gravity).

	base_xy_speed = player->mo->speed / 32.0f;
	base_z_speed  = base_xy_speed;

	// Do not let the player control movement if not onground.
	// -MH- 1998/06/18  unless he has the JetPack!

	if (! (onground || onladder || canswim || hasjetpack))
		base_xy_speed /= 16.0f;

	if (! (onladder || canswim || hasjetpack))
		base_z_speed /= 16.0f;

	// move slower when crouching
	if (player->mo->extendedflags & EF_CROUCHING)
		base_xy_speed *= CROUCH_SLOWDOWN;

	dx = M_Cos(player->mo->angle);
	dy = M_Sin(player->mo->angle);

	eh = 1;
	ev = 0;

	if (canswim)
	{
		float slope = M_Tan(player->mo->vertangle);

		float hyp = (float)sqrt((double)(1.0f + slope * slope));

		eh = 1.0f  / hyp;
		ev = slope / hyp;
	}

	// compute movement vectors

	F_vec[0] = eh * dx * base_xy_speed;
	F_vec[1] = eh * dy * base_xy_speed;
	F_vec[2] = ev * base_z_speed;

	S_vec[0] =  dy * base_xy_speed;
	S_vec[1] = -dx * base_xy_speed;
	S_vec[2] =  0;

	U_vec[0] = -ev * dx * base_xy_speed;
	U_vec[1] = -ev * dy * base_xy_speed;
	U_vec[2] =  eh * base_z_speed;

	player->mo->mom.x += F_vec[0] * cmd->forwardmove + S_vec[0] *
		cmd->sidemove + U_vec[0] * cmd->upwardmove;

	player->mo->mom.y += F_vec[1] * cmd->forwardmove + S_vec[1] *
		cmd->sidemove + U_vec[1] * cmd->upwardmove;

	player->mo->mom.z += F_vec[2] * cmd->forwardmove + S_vec[2] *
		cmd->sidemove + U_vec[2] * cmd->upwardmove;

	if (hasjetpack)
	{
		if (player->powers[PW_Jetpack] <= (5 * TICRATE))
		{
			if (!(leveltime & 10))
				S_StartSound(player->mo, sfx_jpflow);  // fuel low
		}
		else if (cmd->upwardmove > 0)
			S_StartSound(player->mo, sfx_jprise);
		else if (cmd->upwardmove < 0)
			S_StartSound(player->mo, sfx_jpdown);
		else if (cmd->forwardmove || cmd->sidemove)
			S_StartSound(player->mo, (onground ? sfx_jpidle : sfx_jpmove));
		else
			S_StartSound(player->mo, sfx_jpidle);
	}

	if ((cmd->forwardmove || cmd->sidemove)
		&& player->mo->state == &states[player->mo->info->idle_state])
	{
		if (player->mo->info->chase_state)
			P_SetMobjStateDeferred(player->mo, player->mo->info->chase_state, 0);
	}

	// EDGE Feature: Crouching

	if (level_flags.crouch && mo->info->crouchheight > 0)
	{
		if (player->cmd.upwardmove < 0 && mo->z == mo->floorz &&
			mo->height > mo->info->crouchheight)
		{
			mo->height = MAX(mo->height - 2.0f, mo->info->crouchheight);

			// update any things near the player
			P_ChangeThingSize(mo);

			if (mo->player->deltaviewheight == 0)
				mo->player->deltaviewheight = -1.0f;
		}
		else if (player->cmd.upwardmove >= 0 && 
			mo->height < mo->info->height)
		{
			// prevent standing up inside a solid area
			if ((mo->flags & MF_NOCLIP) || mo->z+mo->height+2 <= mo->ceilingz)
			{
				mo->height = MIN(mo->height + 2, mo->info->height);

				// update any things near the player
				P_ChangeThingSize(mo);

				if (mo->player->deltaviewheight == 0)
					mo->player->deltaviewheight = 1.0f;
			}
		}

		if (mo->height < (mo->info->height + mo->info->crouchheight) / 2.0f)
			mo->extendedflags |= EF_CROUCHING;
		else
			mo->extendedflags &= ~EF_CROUCHING;

		player->std_viewheight = mo->height * PERCENT_2_FLOAT(mo->info->viewheight);
	}

	// EDGE Feature: Jump Code
	//
	// -ACB- 1998/08/09 Check that jumping is allowed in the currmap
	//                  Make player pause before jumping again

	if (cmd->extbuttons & EBT_JUMP)
	{
		if (canswim)
		{
			player->mo->mom.z += base_z_speed * 10;
		}
		else if (level_flags.jump && player->jumpwait <= 0 && onground)
		{
			player->mo->mom.z += player->mo->info->jumpheight / 
				(player->mo->extendedflags & EF_CROUCHING ? 1.8f : 1.4f);
			player->jumpwait = player->mo->info->jump_delay;

			// -AJA- 1999/09/11: New JUMP_SOUND for ddf.
			if (player->mo->info->jump_sound)
				S_StartSound(player->mo, player->mo->info->jump_sound);
		}
	}

	// EDGE Feature: Zooming
	//
	if (cmd->extbuttons & EBT_ZOOM)
	{
		P_Zoom(player);
	}

	// EDGE Feature: Vertical Centering (Mlook)
	//
	// -ACB- 1998/07/02 Re-routed via Ticcmd
	//
	if (cmd->extbuttons & EBT_CENTER)
		player->mo->vertangle = 0;
}

//
// P_DeathThink
//
// Fall on your face when dying.
// Decrease POV height to floor height.
//
static void DeathThink(player_t * player)
{
	float dx, dy, dz;

	angle_t angle;
	angle_t delta, delta_s;
	float slope;

	// -AJA- 1999/12/07: don't die mid-air.
	player->powers[PW_Jetpack] = 0;

	P_MovePsprites(player);

	// fall to the ground
	if (player->viewheight > player->std_viewheight)
		player->viewheight -= 1.0f;
	else if (player->viewheight < player->std_viewheight)
		player->viewheight = player->std_viewheight;

	player->deltaviewheight = 0.0f;
	player->kick_offset = 0.0f;

	CalcHeight(player);

	if (player->attacker && player->attacker != player->mo)
	{
		dx = player->attacker->x - player->mo->x;
		dy = player->attacker->y - player->mo->y;
		dz = (player->attacker->z + player->attacker->height/2) - 
			(player->mo->z + player->viewheight);

		angle = R_PointToAngle(0, 0, dx, dy);
		delta = angle - player->mo->angle;

		slope = P_ApproxSlope(dx, dy, dz);
		slope = MIN(1.7f, MAX(-1.7f, slope));
		delta_s = M_ATan(slope) - player->mo->vertangle;

		if ((delta <= ANG1/2 || delta >= (angle_t)(0 - ANG1/2)) &&
			(delta_s <= ANG1/2 || delta_s >= (angle_t)(0 - ANG1/2)))
		{
			// Looking at killer, so fade damage flash down.
			player->mo->angle = angle;
			player->mo->vertangle = M_ATan(slope);

			if (player->damagecount)
				player->damagecount--;
		}
		else 
		{
			if (delta < ANG180)
				delta /= 5;
			else
				delta = (angle_t)(0 - (angle_t)(0 - delta) / 5);
			
			if (delta > ANG5 && delta < (angle_t)(0 - ANG5))
				delta = (delta < ANG180) ? ANG5 : (angle_t)(0 - ANG5);

			if (delta_s < ANG180)
				delta_s /= 5;
			else
				delta_s = (angle_t)(0 - (angle_t)(0 - delta_s) / 5);
			
			if (delta_s > (ANG5/2) && delta_s < (angle_t)(0 - ANG5/2))
				delta_s = (delta_s < ANG180) ? (ANG5/2) : (angle_t)(0 - ANG5/2);

			player->mo->angle += delta;
			player->mo->vertangle += delta_s;

			if (player->damagecount && (leveltime % 3) == 0)
				player->damagecount--;
		}
	}
	else if (player->damagecount)
		player->damagecount--;

	// -AJA- 1999/08/07: Fade out armor points too.
	if (player->bonuscount)
		player->bonuscount--;

	P_UpdatePowerups(player);

	// lose the zoom when dead
	if (viewiszoomed)
	{
		R_SetFOV(normalfov);
		viewiszoomed = false;
	}

	if (deathmatch >= 3 && player->mo->movecount > player->mo->info->respawntime)
		return;

	if (player->cmd.buttons & BT_USE)
		player->playerstate = PST_REBORN;
}

static void P_UpdatePowerups(player_t *player)
{
	float limit = FLT_MAX;
	int pw;

	if (player->playerstate == PST_DEAD)
		limit = 1;  // TICRATE * 5;

	for (pw = 0; pw < NUMPOWERS; pw++)
	{
		if (player->powers[pw] < 0)  // -ACB- 2004/02/04 Negative values last a level
			continue;

		float& qty_r = player->powers[pw];

		if (qty_r > limit)   qty_r = limit;
		else if (qty_r >= 1) qty_r -= 1;
		else if (qty_r > 0)  qty_r = 0;
	}

	// -AJA- 2004/09/29: FIXME: temp kludge to make berserk last a level
	if (player->powers[PW_Berserk] > 1.0f && player->powers[PW_Berserk] < 2.1f)
		player->powers[PW_Berserk] = -1.0f;

	if (player->powers[PW_PartInvis] >= 128 ||
		fmod(player->powers[PW_PartInvis], 16) >= 8)
		player->mo->flags |=  MF_FUZZY;
	else
		player->mo->flags &= ~MF_FUZZY;

	// Handling colourmaps.
	//
	// -AJA- 1999/07/10: Updated for colmap.ddf.
	//
	// !!! FIXME: overlap here with stuff in rgl_fx.cpp.

	player->effect_colourmap = NULL;
	player->effect_left = 0;

	if (player->powers[PW_Invulnerable] > 0.0f)
	{
		float s = player->powers[PW_Invulnerable];

		// -ACB- FIXME!!! Catch lookup failure!
		player->effect_colourmap = colourmaps.Lookup("ALLWHITE");
		player->effect_left = (s <= 0) ? 0 : MIN(int(s), EFFECT_MAX_TIME);
	}
	else if (player->powers[PW_Infrared] > 0.0f)
	{
		float s = player->powers[PW_Infrared];

		player->effect_left = (s <= 0) ? 0 : MIN(int(s), EFFECT_MAX_TIME);
	}
	else if (player->powers[PW_NightVision] > 0.0f)		// -ACB- 1998/07/15 NightVision Code
	{
		float s = player->powers[PW_NightVision];

		// -ACB- FIXME!!! Catch lookup failure!
		player->effect_colourmap = colourmaps.Lookup("ALLGREEN");
		player->effect_left = (s <= 0) ? 0 : MIN(int(s), EFFECT_MAX_TIME);
	}
}

//
// P_ConsolePlayerBuilder
//
// Does the thinking of the console player, i.e. read from input
void P_ConsolePlayerBuilder(const player_t *p, void *data, ticcmd_t *dest)
{
	E_BuildTiccmd(dest);

	dest->buttons |= BT_IN_GAME;
}

bool P_PlayerSwitchWeapon(player_t *player, weapondef_c *choice)
{
	int pw_index;

	// see if player owns this kind of weapon
	for (pw_index=0; pw_index < MAXWEAPONS; pw_index++)
	{
		if (! player->weapons[pw_index].owned)
			continue;

		if (player->weapons[pw_index].info == choice)
			break;
	}

	if (pw_index == MAXWEAPONS)
		return false;

	// ignore this choice if it the same as the current weapon
	if (player->ready_wp >= 0 && choice ==
		player->weapons[player->ready_wp].info)
	{
		return false;
	}

	player->pending_wp = (weapon_selection_e) pw_index;

	return true;
}

//
// P_PlayerThink
//
void P_PlayerThink(player_t * player)
{
	mobj_t *mo = player->mo;
	ticcmd_t *cmd;

	DEV_ASSERT2(mo);

#if 0  // DEBUG ONLY
	{
		touch_node_t *tn;
		L_WriteDebug("Player %d Touch List:\n", player->pnum);
		for (tn = mo->touch_sectors; tn; tn=tn->mo_next)
		{
			L_WriteDebug("  SEC %d  Other = %s\n", tn->sec - sectors,
				tn->sec_next ? tn->sec_next->mo->info->ddf.name :
			tn->sec_prev ? tn->sec_prev->mo->info->ddf.name : "(None)");

			DEV_ASSERT2(tn->mo == mo);
			if (tn->mo_next)
			{
				DEV_ASSERT2(tn->mo_next->mo_prev == tn);
			}
		}
	}
#endif

	// fixme: do this in the cheat code
	if (player->cheats & CF_NOCLIP)
		player->mo->flags |= MF_NOCLIP;
	else
		player->mo->flags &= ~MF_NOCLIP;

	// chain saw run forward
	cmd = &player->cmd;
	if (player->mo->flags & MF_JUSTATTACKED)
	{
		cmd->angleturn = 0;
		cmd->forwardmove = 64;
		cmd->sidemove = 0;
		player->mo->flags &= ~MF_JUSTATTACKED;
	}

	if (player->playerstate == PST_DEAD)
	{
		DeathThink(player);
		return;
	}

	// Move/Look around.  Reactiontime is used to prevent movement for a
	// bit after a teleport.

	if (player->mo->reactiontime)
		player->mo->reactiontime--;
	else
		MovePlayer(player);

	CalcHeight(player);

	if (player->mo->props->special ||
		player->mo->subsector->sector->exfloor_used > 0 ||
		player->underwater)
	{
		P_PlayerInSpecialSector(player, player->mo->subsector->sector);
	}

	// -AJA- FIXME: rework RTS execution as per docs/rts_rule.txt
	RAD_DoRadiTrigger(player);

	// Check for weapon change.

	if (cmd->buttons & BT_CHANGE)
	{
		// The actual changing of the weapon is done when the weapon
		// psprite can do it (read: not in the middle of an attack).

		int key = (cmd->buttons & BT_WEAPONMASK) >> BT_WEAPONSHIFT;

		if (key == BT_NEXT_WEAPON)
		{
			P_NextPrevWeapon(player, +1);
		}
		else if (key == BT_PREV_WEAPON)
		{
			P_NextPrevWeapon(player, -1);
		}
		else  /* numeric key */
		{
			int i, j;
			weaponkey_c *wk = &weaponkey[key];

			for (i=j=player->key_choices[key]; i < (j + wk->numchoices); i++)
			{
				weapondef_c *choice = wk->choices[i % wk->numchoices];

				if (! P_PlayerSwitchWeapon(player, choice))
					continue;

				player->key_choices[key] = i % wk->numchoices;
				break;
			}
		}
	}

	// check for use
	if (cmd->buttons & BT_USE)
	{
		if (!player->usedown)
		{
			P_UseLines(player);
			player->usedown = true;
		}
	}
	else
	{
		player->usedown = false;
	}

	// decrement jumpwait counter
	if (player->jumpwait > 0)
		player->jumpwait--;

	// cycle psprites
	P_MovePsprites(player);

	// Counters, time dependend power ups.

	P_UpdatePowerups(player);

	if (player->damagecount)
		player->damagecount--;

	if (player->bonuscount)
		player->bonuscount--;

	if (player->grin_count)
		player->grin_count--;

	if (player->attackdown[0] || player->attackdown[1])
		player->attackdown_count++;
	else
		player->attackdown_count = 0;

	player->kick_offset /= 1.6f;

}

//
// P_CreatePlayer
//
void P_CreatePlayer(int pnum, bool is_bot)
{
	DEV_ASSERT2(0 <= pnum && pnum < MAXPLAYERS);

	DEV_ASSERT(! players[pnum], ("P_CreatePlayer: %d already there", pnum));

	player_t *p = Z_ClearNew(player_t, 1);

	p->pnum = pnum;
	p->playerstate = PST_DEAD;

	players[pnum] = p;
	numplayers++;

	// determine name
	char namebuf[32];
	sprintf(namebuf, "Player%dName", pnum + 1);

	if (language.IsValidRef(namebuf))
	{
		strncpy(p->playername, language[namebuf], MAX_PLAYNAME-1);
		p->playername[MAX_PLAYNAME-1] = '\0';
	}
	else
	{
		// -ES- Default to player##
		sprintf(p->playername, "Player%d", pnum + 1);
	}

	if (is_bot)
		P_BotCreate(p, false);

	memset(p->consistency, -1, sizeof(p->consistency));
}

//
// P_DestroyAllPlayers
//
void P_DestroyAllPlayers(void)
{
	for (int pnum=0; pnum < MAXPLAYERS; pnum++)
	{
		if (! players[pnum])
			continue;

		Z_Free(players[pnum]);

		players[pnum] = NULL;
	}

	numplayers = 0;

	consoleplayer = -1;
	displayplayer = -1;
}

//
// P_UpdateAvailWeapons
//
// Must be called as soon as the player has received or lost a weapon.
// Updates the status bar icons.
//
void P_UpdateAvailWeapons(player_t *p)
{
	int key;

	for (key = 0; key < WEAPON_KEYS; key++)
		p->avail_weapons[key] = false;

	for (int i = 0; i < MAXWEAPONS; i++)
	{
		if (! p->weapons[i].owned)
			continue;

		DEV_ASSERT2(p->weapons[i].info);

		key = p->weapons[i].info->bind_key;

		// update the status bar icons
		if (0 <= key && key <= 9)
			p->avail_weapons[key] = true;
	}
}

//
// P_UpdateTotalArmour
//
void P_UpdateTotalArmour(player_t *p)
{
	int i;

	p->totalarmour = 0;

	for (i=0; i < NUMARMOUR; i++)
	{
		p->totalarmour += p->armours[i];
	}

	if (p->totalarmour > 999.0f)
		p->totalarmour = 999.0f;
}

//
// P_AddWeapon
//
// Returns true if player didn't already have the weapon.  If
// successful and `index' is non-NULL, it is set to the new index.
//
bool P_AddWeapon(player_t *player, weapondef_c *info, int *index)
{
	int slot = -1;
	int upgrade_slot = -1;

	// cannot own weapons if sprites are missing
	if (! P_CheckWeaponSprite(info))
		return false;

	for (int i=0; i < MAXWEAPONS; i++)
	{
		weapondef_c *cur_info = player->weapons[i].info;

		// skip weapons that are being removed
		if (player->weapons[i].flags & PLWEP_Removing)
			continue;

		// find free slot
		if (! player->weapons[i].owned)
		{
			if (slot < 0)
				slot = i;
			continue;
		}

		// check if already own this weapon
		if (cur_info == info)
			return false;

		// don't downgrade any UPGRADED weapons
		if (DDF_WeaponIsUpgrade(cur_info, info))
			return false;

		// check for weapon upgrades
		if (cur_info == info->upgrade_weap)
		{
			upgrade_slot = i;
			continue;
		}
	}

	if (slot < 0)
		return false;

	if (index)
		(*index) = slot;

	L_WriteDebug("P_AddWeapon: [%s] @ %d\n", info->ddf.name.GetString(), slot);

	player->weapons[slot].owned = true;
	player->weapons[slot].info  = info;
	player->weapons[slot].flags = 0;
	player->weapons[slot].clip_size[0] = 0;
	player->weapons[slot].clip_size[1] = 0;

	P_UpdateAvailWeapons(player);

	// for NoAmmo+Clip weapons, always begin with a full clip
	for (int ATK = 0; ATK < 2; ATK++)
	{
		if (info->clip_size[ATK] > 0 && info->ammo[ATK] == AM_NoAmmo)
			player->weapons[slot].clip_size[ATK] = info->clip_size[ATK];
	}

	// initial weapons should get a full clip
	if (info->autogive)
		P_TryFillNewWeapon(player, slot, AM_DontCare, NULL);

	if (upgrade_slot >= 0)
	{
		player->weapons[upgrade_slot].owned = false;

		// handle the case of holding the weapon which is being upgraded
		// by the new one.  We mark the old weapon for removal.

		if (player->ready_wp == upgrade_slot)
		{
			player->weapons[upgrade_slot].flags |= PLWEP_Removing;
			player->pending_wp = (weapon_selection_e) slot;
		}
		else
			player->weapons[upgrade_slot].info = NULL;

		// !!! FIXME: check and update key_choices[]
	}

	return true;
}

//
// P_RemoveWeapon
//
// Returns true if player had the weapon.
//
bool P_RemoveWeapon(player_t *player, weapondef_c *info)
{
	int i;

	for (i=0; i < MAXWEAPONS; i++)
	{
		if (! player->weapons[i].owned)
			continue;

		// Note: no need to check PLWEP_Removing

		if (player->weapons[i].info == info)
			break;
	}

	if (i >= MAXWEAPONS)
		return false;

	L_WriteDebug("P_RemoveWeapon: [%s] @ %d\n", info->ddf.name.GetString(), i);

	player->weapons[i].owned = false;

	P_UpdateAvailWeapons(player);

	// handle the case of already holding the weapon.  We mark the
	// weapon as being removed (the flag is cleared once lowered).

	if (player->ready_wp == i)
	{
		player->weapons[i].flags |= PLWEP_Removing;

		if (player->pending_wp == WPSEL_NoChange)
			P_DropWeapon(player);
	}
	else
		player->weapons[i].info = NULL;

	if (player->pending_wp == i)
		P_SelectNewWeapon(player, -100, AM_DontCare);

	return true;
}

//
// P_GiveInitialBenefits
//
// Give the player the initial benefits when they start a game (or
// restart after dying).  Sets up: ammo, ammo-limits, health, armour,
// keys and weapons.
//
// -AJA- 2000/03/02: written.
//
void P_GiveInitialBenefits(player_t *p, const mobjtype_c *info)
{
	epi::array_iterator_c it;
	weapondef_c *w;
	
	p->ready_wp   = WPSEL_None;
	p->pending_wp = WPSEL_NoChange;

	int i;

	for (i=0; i < WEAPON_KEYS; i++)
		p->key_choices[i] = 0;

	// clear out ammo & ammo-limits
	for (i=0; i < NUMAMMO; i++)
	{
		p->ammo[i].num = p->ammo[i].max = 0;
	}

	// set health and armour
	p->health = info->spawnhealth;
	p->air_in_lungs = info->lung_capacity;
	p->underwater = false;

	for (i = 0; i < NUMARMOUR; i++)
		p->armours[i] = 0;

	p->totalarmour = 0;
	p->cards = KF_NONE;

	// give all initial benefits
	P_GiveBenefitList(p, NULL, info->initial_benefits, false);

	// give all free weapons.  Needs to be after ammo, so that
	// clip weapons can get their clips filled.
	for (it=weapondefs.GetIterator(weapondefs.GetDisabledCount());
		it.IsValid(); it++)
	{
		w = ITERATOR_TO_TYPE(it, weapondef_c*);
		if (!w->autogive)
			continue;

		int pw_index;

		P_AddWeapon(p, w, &pw_index);
	}

	// refresh to remove all stuff from status bar
	P_UpdateAvailWeapons(p);
}

