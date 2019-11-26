//----------------------------------------------------------------------------
//  EDGE Cheat Sequence Checking
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2018  The EDGE Team.
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
// -KM- 1998/07/21 Moved the cheat sequence here from st_stuff.c
//                 ST_Responder in st_stuff.c calls cht_Responder to check for
//                 cheat codes.  Also added NO_NIGHTMARE_CHEATS #define.
//                 if defined, there can be no cheating in nightmare. :-)
//                 Made all the cheat codes non global.
//
// -ACB- 1998/07/30 Naming Convention stuff, all procedures m_*.*.
//                  Added Touching Mobj "Cheat".
//
// -ACB- 1999/09/19 CD Audio cheats removed.
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
DEF_CVAR(nocheats, int, "c", 0);

//
// CHEAT SEQUENCE PACKAGE
//
// This is so hackers couldn't discover the cheat codes.
#define SCRAMBLE(a) (a)

static int firsttime = 1;
static unsigned char cheat_xlate_table[256];

static cheatseq_t cheat_powerup[9] =
{
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},  // -MH- 1998/06/17  added "give jetpack" cheat
	{0, 0}  // -ACB- 1998/07/15  added "give nightvision" cheat
};

static cheatseq_t cheat_mus               = {0, 0};
static cheatseq_t cheat_mypos             = {0, 0};
static cheatseq_t cheat_showstats         = {0, 0};
static cheatseq_t cheat_choppers          = {0, 0};
static cheatseq_t cheat_clev              = {0, 0};
static cheatseq_t cheat_killall           = {0, 0};
static cheatseq_t cheat_suicide           = {0, 0};
static cheatseq_t cheat_loaded            = {0, 0};
static cheatseq_t cheat_takeall           = {0, 0};
static cheatseq_t cheat_god               = {0, 0};
static cheatseq_t cheat_ammo              = {0, 0};
static cheatseq_t cheat_ammonokey         = {0, 0};
static cheatseq_t cheat_keys              = {0, 0};
static cheatseq_t cheat_noclip            = {0, 0};
static cheatseq_t cheat_noclip2           = {0, 0};
static cheatseq_t cheat_hom               = {0, 0};

static cheatseq_t cheat_giveweapon[11] =
{
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
};

//
// M_CheckCheat
//
// Called in M_CheatResponder module, which handles the input.
// Returns a 1 if the cheat was successful, 0 if failed.
//
int M_CheckCheat(cheatseq_t * cht, char key)
{
	int i;
	int rc = 0;

	if (firsttime)
	{
		firsttime = 0;
		for (i = 0; i < 256; i++)
			cheat_xlate_table[i] = SCRAMBLE(i);
	}

	if (!cht->p)
		cht->p = cht->sequence;  // initialise if first time

	if (cheat_xlate_table[(unsigned char)key] == *cht->p)
		cht->p++;
	else
		cht->p = cht->sequence;

	if (*cht->p == 0)
	{  // end of sequence character

		cht->p = cht->sequence;
		rc = 1;
	}

	return rc;
}

void M_ChangeLevelCheat(const char *string)
{
	// User pressed <ESC>
	if (!string)
		return;

	// NOTE WELL: following assumes single player

	newgame_params_c params;

	params.skill = gameskill;	
	params.deathmatch = deathmatch;

	params.map = G_LookupMap(string);
	if (! params.map)
	{
		CON_MessageLDF("ImpossibleChange");
		return;
	}

	SYS_ASSERT(G_MapExists(params.map));
	SYS_ASSERT(params.map->episode);

	params.random_seed = I_PureRandom();

	if (splitscreen_mode)
		params.Splitscreen();
	else
		params.SinglePlayer(numbots);

	G_DeferredNewGame(params);

	CON_MessageLDF("LevelChange");
}

//
// M_ChangeMusicCheat
//
static void M_ChangeMusicCheat(const char *string)
{
	int entry_num;

	// User pressed <ESC>
	if (! string)
		return;

	entry_num = atoi(string);

	if (! entry_num)
		return;

	S_ChangeMusic(entry_num, true);
	CON_MessageLDF("MusChange");
}

bool M_CheatResponder(event_t * ev)
{
#ifdef NOCHEATS
	return false;
#endif
	if (nocheats > 0)
		return false;

	int i;
	player_t *pl = players[consoleplayer1];

	// if a user keypress...
	if (ev->type != ev_keydown)
		return false;

	char key = (char) ev->data1;

	if (!CheckCheats(true))
		return false;

	// 'dqd' cheat for toggleable god mode
	if (M_CheckCheat(&cheat_god, key))
	{
		Cheat_ToggleGodMode();
	}

	// 'fa' cheat for killer fucking arsenal
	//
	// -ACB- 1998/06/26 removed backpack from this as backpack is variable
	//
	else if (M_CheckCheat(&cheat_ammonokey, key))
	{
		Cheat_IDFA();
	}

	// 'kfa' cheat for key full ammo
	//
	// -ACB- 1998/06/26 removed backpack from this as backpack is variable
	//
	else if (M_CheckCheat(&cheat_ammo, key))
	{
		Cheat_IDKFA();
	}
	else if (M_CheckCheat(&cheat_keys, key))
	{
		Cheat_Unlock();
	}
	else if (M_CheckCheat(&cheat_loaded, key))
	{
		Cheat_Loaded();
	}
#if 0  // FIXME: this crashes ?
	else if (M_CheckCheat(&cheat_takeall, key))
	{
		P_GiveInitialBenefits(pl, pl->mo->info);

		// -ACB- 1998/08/26 Stuff removed language reference
		CON_MessageLDF("StuffRemoval");
	}
#endif
	else if (M_CheckCheat(&cheat_suicide, key))
	{
		Cheat_Suicide();
	}
	// -ACB- 1998/08/27 Used Mobj linked-list code, much cleaner.
	else if (M_CheckCheat(&cheat_killall, key))
	{
		Cheat_KillAll();
	}
	// Simplified, accepting both "noclip" and "idspispopd".
	// no clipping mode cheat
	else if (M_CheckCheat(&cheat_noclip,  key)
		  || M_CheckCheat(&cheat_noclip2, key))
	{
		Cheat_NoClip();
	}
	else if (M_CheckCheat(&cheat_hom, key))
	{
		Cheat_HOM();
	}

	// 'behold?' power-up cheats
	for (i = 0; i < 9; i++)
	{
		if (M_CheckCheat(&cheat_powerup[i], key))
		{
			if (!pl->powers[i])
				pl->powers[i] = 60 * TICRATE;
			else
				pl->powers[i] = 0;

			if (i == PW_Berserk)
				pl->keep_powers |= (1 << PW_Berserk);

			CON_MessageLDF("BeholdUsed");
		}
	}

#if 0  // -AJA- eh ?
	// 'behold' power-up menu
	if (M_CheckCheat(&cheat_powerup[9], key))
	{
		CON_MessageLDF("BeholdNote");
	}
#endif

	// 'give#' power-up cheats
	for (i = 0; i < 10; i++)
	{
		if (! M_CheckCheat(&cheat_giveweapon[i + 1], key))
			continue;

		CheatGiveWeapons(pl, i);
	}

	// 'choppers' invulnerability & chainsaw
	if (M_CheckCheat(&cheat_choppers, key))
	{
		Cheat_Choppers();
	}

	// 'mypos' for player position
	else if (M_CheckCheat(&cheat_mypos, key))
	{
		Cheat_ShowPos();
	}

	if (M_CheckCheat(&cheat_clev, key))
	{
		// 'clev' change-level cheat
		M_StartMessageInput(language["LevelQ"], M_ChangeLevelCheat);
	}
	else if (M_CheckCheat(&cheat_mus, key))
	{
		// 'mus' cheat for changing music
		M_StartMessageInput(language["MusicQ"], M_ChangeMusicCheat);
	}
	else if (M_CheckCheat(&cheat_showstats, key))
	{
		Cheat_Info();
	}

	return false;
}

// -KM- 1999/01/31 Loads cheats from languages file.
// -ES- 1999/08/26 Removed M_ConvertCheat stuff, the cheat terminator is
//      now just 0.
void M_CheatInit(void)
{
	int i;
	char temp[16];

	// Now what?
	cheat_mus.sequence       = language["idmus"];
	cheat_god.sequence       = language["iddqd"];
	cheat_ammo.sequence      = language["idkfa"];
	cheat_ammonokey.sequence = language["idfa"];
	cheat_noclip.sequence    = language["idspispopd"];
	cheat_noclip2.sequence   = language["idclip"];
	cheat_hom.sequence       = language["idhom"];

	for (i=0; i < 9; i++)
	{
		sprintf(temp, "idbehold%d", i + 1);
		cheat_powerup[i].sequence = language[temp];
	}

	cheat_choppers.sequence  = language["idchoppers"];
	cheat_clev.sequence      = language["idclev"];
	cheat_mypos.sequence     = language["idmypos"];

	//new cheats
	cheat_killall.sequence   = language["idkillall"];
	cheat_showstats.sequence = language["idinfo"];
	cheat_suicide.sequence   = language["idsuicide"];
	cheat_keys.sequence      = language["idunlock"];
	cheat_loaded.sequence    = language["idloaded"];
	cheat_takeall.sequence   = language["idtakeall"];

	for (i = 0; i < 11; i++)
	{
		sprintf(temp, "idgive%d", i);
		cheat_giveweapon[i].sequence = language[temp];
	}
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
