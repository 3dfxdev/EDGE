//----------------------------------------------------------------------------
//  EDGE Cheat Sequence Checking
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

#include "i_defs.h"
#include "m_cheat.h"

#include "con_main.h"
#include "ddf_main.h"
#include "dstrings.h"
#include "g_game.h"
#include "m_menu.h"
#include "s_sound.h"
#include "p_local.h"
#include "p_mobj.h"
#include "p_bot.h"
#include "st_stuff.h"
#include "w_wad.h"

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
static cheatseq_t cheat_keys              = {0, 0};
static cheatseq_t cheat_loaded            = {0, 0};
static cheatseq_t cheat_takeall           = {0, 0};
static cheatseq_t cheat_god               = {0, 0};
static cheatseq_t cheat_lazarus           = {0, 0};
static cheatseq_t cheat_ammo              = {0, 0};
static cheatseq_t cheat_ammonokey         = {0, 0};
static cheatseq_t cheat_noclip            = {0, 0};
static cheatseq_t cheat_commercial_noclip = {0, 0};
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

static void M_ChangeLevelCheat(const char *string)
{
	// User pressed <ESC>
	if (!string)
		return;

	// NOTE WELL: following assumes single player

	newgame_params_c params;

	params.skill = gameskill;	
	params.deathmatch = 0;

	params.map = G_LookupMap(string);
	if (! params.map)
	{
		CON_MessageLDF("ImpossibleChange");
		return;
	}

	params.game = gamedefs.Lookup(params.map->episode_name);
	if (! params.game)
	{
		CON_MessageLDF("ImpossibleChange");
		return;
	}

	params.random_seed = I_PureRandom();

	params.SinglePlayer(startbots);

	if (! G_DeferredInitNew(params, true /* compat_check */))
	{
		CON_MessageLDF("ImpossibleChange");
		return;
	}

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

	int i, j;
	player_t *pl = players[consoleplayer];

	// disable cheats while in RTS menu, or demos
	if (rts_menuactive || demoplayback)
		return false;

	// if a user keypress...
	if (ev->type != ev_keydown)
		return false;

	char key = (char) ev->value.key;

	// no cheating in netgames or if disallowed in levels.ddf
	if (!level_flags.cheats)
		return false;

#if 0 //!!!! TEMP DISABLED, NETWORK DEBUGGING
	if (netgame)
		return false;
#endif

	// 'dqd' cheat for toggleable god mode
	if (M_CheckCheat(&cheat_god, key))
	{
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

	// 'fa' cheat for killer fucking arsenal
	//
	// -ACB- 1998/06/26 removed backpack from this as backpack is variable
	//
	else if (M_CheckCheat(&cheat_ammonokey, key))
	{
		pl->armours[CHEATARMOURTYPE] = CHEATARMOUR;

		epi::array_iterator_c it;
		weapondef_c* w;
		
		it = weapondefs.GetIterator(weapondefs.GetDisabledCount());
		while (it.IsValid())
		{
			w = ITERATOR_TO_TYPE(it, weapondef_c*);
			if (w)
				P_AddWeapon(pl, w, NULL);
				
			it++;
		}

		for (i = 0; i < NUMAMMO; i++)
			pl->ammo[i].num = pl->ammo[i].max;

		P_UpdateAvailWeapons(pl);
		P_UpdateTotalArmour(pl);

		CON_MessageLDF("AmmoAdded");
	}

	// 'kfa' cheat for key full ammo
	//
	// -ACB- 1998/06/26 removed backpack from this as backpack is variable
	//
	else if (M_CheckCheat(&cheat_ammo, key))
	{
		pl->armours[CHEATARMOURTYPE] = CHEATARMOUR;

		epi::array_iterator_c it;
		weapondef_c* w;
		
		it = weapondefs.GetIterator(weapondefs.GetDisabledCount());
		while (it.IsValid())
		{
			w = ITERATOR_TO_TYPE(it, weapondef_c*);
			if (w)
				P_AddWeapon(pl, w, NULL);
				
			it++;
		}

		for (i = 0; i < NUMAMMO; i++)
			pl->ammo[i].num = pl->ammo[i].max;

		pl->cards = KF_MASK;

		P_UpdateAvailWeapons(pl);
		P_UpdateTotalArmour(pl);

		CON_MessageLDF("VeryHappyAmmo");
	}
	else if (M_CheckCheat(&cheat_keys, key))
	{
		pl->cards = KF_MASK;

		CON_MessageLDF("UnlockCheat");
	}
	else if (M_CheckCheat(&cheat_loaded, key))
	{
		for (i = 0; i < NUMAMMO; i++)
			pl->ammo[i].num = pl->ammo[i].max;

		CON_MessageLDF("LoadedCheat");
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
		P_TelefragMobj(pl->mo, pl->mo, NULL);

		// -ACB- 1998/08/26 Suicide language reference
		CON_MessageLDF("SuicideCheat");
	}
	// -ACB- 1998/08/27 Used Mobj linked-list code, much cleaner.
	else if (M_CheckCheat(&cheat_killall, key))
	{
		int killcount = 0;
		mobj_t *currmobj;

		// Note: this may miss monsters spawned during death frames (like
		// when the Pain Elemental dies).

		for (currmobj=mobjlisthead; currmobj; currmobj=currmobj->next)
		{
			if ((currmobj->extendedflags & EF_MONSTER) && (currmobj->health > 0))
			{
				P_TelefragMobj(currmobj, NULL, NULL);
				killcount++;
			}
		}

		CON_MessageLDF("MonstersKilled", killcount);
	}
	// Simplified, accepting both "noclip" and "idspispopd".
	// no clipping mode cheat
	else if (M_CheckCheat(&cheat_noclip, key)
		|| M_CheckCheat(&cheat_commercial_noclip, key))
	{
		pl->cheats ^= CF_NOCLIP;

		if (pl->cheats & CF_NOCLIP)
			CON_MessageLDF("ClipOn");
		else
			CON_MessageLDF("ClipOff");
	}
	else if (M_CheckCheat(&cheat_hom, key))
	{
		hom_detect = ! hom_detect;

		if (hom_detect)
			CON_MessageLDF("HomDetectOn");
		else
			CON_MessageLDF("HomDetectOff");
	}
#if 0
	else if (M_CheckCheat(&cheat_lazarus, key))
	{
		if (pl->playerstate == PST_DEAD && (netgame==0))
		{
			DEV_ASSERT2(pl->mo);
			P_BringCorpseToLife(pl->mo);
		}
	}
#endif

	// 'behold?' power-up cheats
	for (i = 0; i < 9; i++)
	{
		if (M_CheckCheat(&cheat_powerup[i], key))
		{
			if (!pl->powers[i])
				pl->powers[i] = 60 * TICRATE;
			else
				pl->powers[i] = 0;

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

		for (j=0; j < weaponkey[i].numchoices; j++)
		{
			weapondef_c *info = weaponkey[i].choices[j];

			P_AddWeapon(pl, info, NULL);

			if (info->ammo[0] != AM_NoAmmo)
				pl->ammo[info->ammo[0]].num = pl->ammo[info->ammo[0]].max;
		}
	}

	// 'choppers' invulnerability & chainsaw
	if (M_CheckCheat(&cheat_choppers, key))
	{
		weapondef_c *w = weapondefs.Lookup("CHAINSAW");
		if (w)
		{
			P_AddWeapon(pl, w, NULL);
			pl->powers[PW_Invulnerable] = 1;
			CON_MessageLDF("CHOPPERSNote");
		}
	}

	// 'mypos' for player position
	else if (M_CheckCheat(&cheat_mypos, key))
	{
		CON_Message("ang=%f;x,y=(%f,%f)",
			ANG_2_FLOAT(pl->mo->angle), pl->mo->x, pl->mo->y);
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
		showstats = !showstats;

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
	cheat_mus.sequence               = language["idmus"];
	cheat_god.sequence               = language["iddqd"];
	cheat_lazarus.sequence           = "idlazarus";
	cheat_ammo.sequence              = language["idkfa"];
	cheat_ammonokey.sequence         = language["idfa"];
	cheat_noclip.sequence            = language["idspispopd"];
	cheat_commercial_noclip.sequence = language["idclip"];
	cheat_hom.sequence               = language["idhom"];

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
