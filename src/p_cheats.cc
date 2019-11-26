
//----------------------------------------------------------------------------
//  EDGE Cheating and Debugging Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2017  The EDGE Team.
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
// This code does not handle the user front-end for entering cheats. This
// includes the old way in-game (iddqd, idkfa, etc), through the console
// (god, noclip, etc), and soon will also include through netcmds
// (demos, netgames, etc). Note that these interfaces will be responsible
// for invoking these commands - this file just contains code to actually
// carry them out.
//
//

#include "system/i_defs.h"
#include "m_cheatcodes.h"

#include "con_main.h"
#include "../ddf/main.h"
#include "dstrings.h"
#include "g_game.h"
#include "m_menu.h"
#include "s_sound.h"
#include "s_music.h"
#include "p_local.h"
#include "p_mobj.h"
#include "p_bot.h"
#include "w_wad.h"
#include "p_cheats.h"

extern int debug_fps;
extern int debug_pos;


bool CheckCheats(bool silent)
{
#ifdef NOCHEATS
	// we should never see this message, since this define removes the console entries anyway...
	if (!silent)
		I_Printf("Cheats are disabled in this build!\n");
	return false;
#endif

	// disable cheats while in RTS menu, or demos
	if (rts_menuactive)
		return false;

	// no cheating in netgames or if disallowed in levels.ddf
	if (!level_flags.cheats)
	{
		if (!silent)
			I_Printf("Cheats are currently disabled here!\n");
		return false;
	}

#if 0 //!!!! TEMP DISABLED, NETWORK DEBUGGING
	if (netgame)
	{
		if (!silent)
			I_Printf("Cheats are disabled in net game!\n");
		return false;
	}
#endif
	return true;
}

void Cheat_ToggleGodMode()
{
	player_t *pl = players[consoleplayer1];

	pl->cheats ^= CF_GODMODE;
	if (pl->cheats & CF_GODMODE)
	{
		if (pl->mo)
		{
			pl->health = pl->mo->health = pl->mo->info->spawnhealth;
		}
		CON_MessageLDF("GodModeOn");
	}
	else
		CON_MessageLDF("GodModeOff");
}

void Cheat_NoClip()
{
	player_t *pl = players[consoleplayer1];

	pl->cheats ^= CF_NOCLIP;

	if (pl->cheats & CF_NOCLIP)
		CON_MessageLDF("ClipOn");
	else
		CON_MessageLDF("ClipOff");
}

void Cheat_IDFA()
{
	player_t *pl = players[consoleplayer1];

	pl->armours[CHEATARMOURTYPE] = CHEATARMOUR;

	P_UpdateTotalArmour(pl);

	for (int i = 0; i < NUMAMMO; i++)
		pl->ammo[i].num = pl->ammo[i].max;

	CheatGiveWeapons(pl);

	CON_MessageLDF("AmmoAdded");
}

void Cheat_IDKFA()
{
	player_t *pl = players[consoleplayer1];

	pl->armours[CHEATARMOURTYPE] = CHEATARMOUR;

	P_UpdateTotalArmour(pl);

	for (int i = 0; i < NUMAMMO; i++)
		pl->ammo[i].num = pl->ammo[i].max;

	pl->cards = KF_MASK;

	CheatGiveWeapons(pl);

	CON_MessageLDF("VeryHappyAmmo");
}

void Cheat_Choppers()
{
	player_t *pl = players[consoleplayer1];
	weapondef_c *w = weapondefs.Lookup("CHAINSAW");

	if (w)
	{
		P_AddWeapon(pl, w, NULL);
		pl->powers[PW_Invulnerable] = 1;
		CON_MessageLDF("CHOPPERSNote");
	}
}

void Cheat_Unlock()
{
	player_t *pl = players[consoleplayer1];

	pl->cards = KF_MASK;

	CON_MessageLDF("UnlockCheat");
}

void Cheat_HOM()
{
	debug_hom = debug_hom ? 0 : 1;

	if (debug_hom)
		CON_MessageLDF("HomDetectOn");
	else
		CON_MessageLDF("HomDetectOff");
}

void Cheat_Loaded()
{
	player_t *pl = players[consoleplayer1];

	for (int i = 0; i < NUMAMMO; i++)
		pl->ammo[i].num = pl->ammo[i].max;

	CON_MessageLDF("LoadedCheat");
}

void Cheat_Suicide()
{
	player_t *pl = players[consoleplayer1];

	P_TelefragMobj(pl->mo, pl->mo, NULL);

	CON_MessageLDF("SuicideCheat");
}

void Cheat_ShowPos()
{
	player_t *pl = players[consoleplayer1];
	CON_Message("ang=%f;x,y=(%f,%f)",
		ANG_2_FLOAT(pl->mo->angle), pl->mo->x, pl->mo->y);
}

void Cheat_KillAll()
{
	int killcount = 0;

	mobj_t *mo;
	mobj_t *next;

	for (mo = mobjlisthead; mo; mo = next)
	{
		next = mo->next;

		if ((mo->extendedflags & EF_MONSTER) && (mo->health > 0))
		{
			P_TelefragMobj(mo, NULL, NULL);
			killcount++;
		}
	}

	CON_MessageLDF("MonstersKilled", killcount);
}

void Cheat_Info()
{
	debug_fps = debug_fps ? 0 : 1;
	debug_pos = debug_pos ? 0 : 1;
}