//------------------------------------------------------------------------
//  WEAPON Conversion
//------------------------------------------------------------------------
//
//  DEH_EDGE  Copyright (C) 2004  The EDGE Team
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License (in COPYING.txt) for more details.
//
//------------------------------------------------------------------------
//
//  DEH_EDGE is based on:
//
//  +  DeHackEd source code, by Greg Lewis.
//  -  DOOM source code (C) 1993-1996 id Software, Inc.
//  -  Linux DOOM Hack Editor, by Sam Lantinga.
//  -  PrBoom's DEH/BEX code, by Ty Halderman, TeamTNT.
//
//------------------------------------------------------------------------

#include "i_defs.h"
#include "weapons.h"

#include "info.h"
#include "frames.h"
#include "misc.h"
#include "mobj.h"
#include "sounds.h"
#include "system.h"
#include "things.h"
#include "wad.h"


int Ammo::plr_max[4] = 
{ 
	200, 50, 300, 50  // doubled for backpack
};

int Ammo::pickups[4] =
{
	10, 4, 20, 1   // multiplied by 5 for boxes
};

// Weapon info: sprite frames, ammunition use.
typedef struct
{
	const char *ddf_name;

    int ammo, ammo_per_shot;
	int bind_key, priority;
	
	const char *flags;

    int upstate;
    int downstate;
    int readystate;
    int atkstate;
    int flashstate;
}
weaponinfo_t;

#define WF_FREE       'f'
#define WF_REF_INACC  'r'
#define WF_DANGEROUS  'd'
#define WF_NO_THRUST  't'
#define WF_FEEDBACK   'b'

weaponinfo_t weapon_info[NUMWEAPONS] =
{
    {
		"FIST", am_noammo, 0,  1,0, "f",
		S_PUNCHUP, S_PUNCHDOWN, S_PUNCH, S_PUNCH1, S_NULL
    },	
    {
		"PISTOL", am_bullet, 1,  2,2, "fr",
		S_PISTOLUP, S_PISTOLDOWN, S_PISTOL, S_PISTOL1, S_PISTOLFLASH
    },	
    {
		"SHOTGUN", am_shell, 1,  3,3, NULL,
		S_SGUNUP, S_SGUNDOWN, S_SGUN, S_SGUN1, S_SGUNFLASH1
    },
    {
		"CHAINGUN", am_bullet, 1,  4,5, "r",
		S_CHAINUP, S_CHAINDOWN, S_CHAIN, S_CHAIN1, S_CHAINFLASH1
    },
    {
		"ROCKET_LAUNCHER", am_rocket, 1,  5,7, "d",
		S_MISSILEUP, S_MISSILEDOWN, S_MISSILE, S_MISSILE1, S_MISSILEFLASH1
    },
    {
		"PLASMA_RIFLE", am_cell, 1,  6,6, NULL,
		S_PLASMAUP, S_PLASMADOWN, S_PLASMA, S_PLASMA1, S_PLASMAFLASH1
    },
    {
		"BFG_9000", am_cell, 40,  7,8, "d",
		S_BFGUP, S_BFGDOWN, S_BFG, S_BFG1, S_BFGFLASH1
    },
    {
		"CHAINSAW", am_noammo, 0,  1,1, "bt",
		S_SAWUP, S_SAWDOWN, S_SAW, S_SAW1, S_NULL
    },
    {
		"SUPER_SHOTGUN", am_shell, 2,  3,4, NULL,
		S_DSGUNUP, S_DSGUNDOWN, S_DSGUN, S_DSGUN1, S_DSGUNFLASH1
    },	
};

bool weapon_modified[NUMWEAPONS];


//----------------------------------------------------------------------------

void Weapons::Startup(void)
{
	memset(weapon_modified,  0, sizeof(weapon_modified));
}

namespace Weapons
{
	bool got_one;

	void BeginLump(void)
	{
		WAD::NewLump("DDFWEAP");

		WAD::Printf(GEN_BY_COMMENT);
		WAD::Printf("<WEAPONS>\n\n");
	}

	void FinishLump(void)
	{
		WAD::Printf("\n");
		WAD::FinishLump();
	}

	const char *GetAmmo(int type)
	{
		switch (type)
		{
			case am_bullet: return "BULLETS";
			case am_shell:  return "SHELLS";
			case am_rocket: return "ROCKETS";
			case am_cell:   return "CELLS";
			case am_noammo: return "NOAMMO";
		}

		InternalError("Bad ammo type %d\n", type);
		return NULL;
	}

	void HandleFlags(const weaponinfo_t *info, int w_num)
	{
		if (! info->flags)
			return;

		if (strchr(info->flags, WF_FREE))
			WAD::Printf("FREE = TRUE;\n");

		if (strchr(info->flags, WF_REF_INACC))
			WAD::Printf("REFIRE_INACCURATE = TRUE;\n");

		if (strchr(info->flags, WF_DANGEROUS))
			WAD::Printf("DANGEROUS = TRUE;\n");

		if (strchr(info->flags, WF_NO_THRUST))
			WAD::Printf("NOTHRUST = TRUE;\n");

		if (strchr(info->flags, WF_FEEDBACK))
			WAD::Printf("FEEDBACK = TRUE;\n");
	}

	void HandleSounds(const weaponinfo_t *info, int w_num)
	{
		if (w_num == wp_chainsaw)
		{
			WAD::Printf("START_SOUND = %s;\n", Things::GetSound(sfx_sawup));
			WAD::Printf("IDLE_SOUND = %s;\n", Things::GetSound(sfx_sawidl));
			WAD::Printf("ENGAGED_SOUND = %s;\n", Things::GetSound(sfx_sawful));
			return;
		}

		// otherwise nothing.
	}

	void HandleFrames(const weaponinfo_t *info, int w_num)
	{
		Frames::ResetAll();

		// --- collect states into groups ---

		int count = 0;

		count += Frames::BeginGroup(info->flashstate,  'f');
		count += Frames::BeginGroup(info->atkstate,    'a');
		count += Frames::BeginGroup(info->readystate,  'r');
		count += Frames::BeginGroup(info->downstate,   'd');
		count += Frames::BeginGroup(info->upstate,     'u');

		if (count == 0)
		{
			PrintWarn("Weapon [%s] has no states.\n", info->ddf_name);
			return;
		}

		Frames::SpreadGroups();

		Frames::OutputGroup(info->upstate,     'u');
		Frames::OutputGroup(info->downstate,   'd');
		Frames::OutputGroup(info->readystate,  'r');
		Frames::OutputGroup(info->atkstate,    'a');
		Frames::OutputGroup(info->flashstate,  'f');

		if (Frames::act_flags & AF_THING_ST)
		{
			PrintWarn("Weapon [%s] uses thing states.\n", info->ddf_name);
		}
	}

	void HandleAttacks(const weaponinfo_t *info, int w_num)
	{
		int count = 
			(Frames::attack_slot[0] ? 1 : 0) +
			(Frames::attack_slot[1] ? 1 : 0) +
			(Frames::attack_slot[2] ? 1 : 0);

		if (count == 0)
			return;

		if (count > 1)
			PrintWarn("Multiple attacks used in weapon [%s]\n", info->ddf_name);


		WAD::Printf("\n");

		const char *atk = Frames::attack_slot[0];

		if (! atk)
			atk = Frames::attack_slot[1];

		assert(Frames::attack_slot[2] == NULL);
		assert(atk != NULL);

		WAD::Printf("ATTACK = %s;\n", atk);
	}

	void ConvertWeapon(int w_num)
	{
		if (! got_one)
		{
			got_one = true;
			BeginLump();
		}

		const weaponinfo_t *info = weapon_info + w_num;

		WAD::Printf("[%s]\n", info->ddf_name);

		WAD::Printf("AMMOTYPE = %s;\n", GetAmmo(info->ammo));

		if (w_num == wp_bfg)
			WAD::Printf("AMMOPERSHOT = %d;\n", Misc::bfg_cells_per_shot);
		else if (info->ammo_per_shot != 0)
			WAD::Printf("AMMOPERSHOT = %d;\n", info->ammo_per_shot);

		WAD::Printf("AUTOMATIC = TRUE;\n");

		WAD::Printf("BINDKEY = %d;\n", info->bind_key);
		WAD::Printf("PRIORITY = %d;\n", info->priority);

		HandleFlags(info, w_num);
		HandleSounds(info, w_num);
		HandleFrames(info, w_num);
		HandleAttacks(info, w_num);

		WAD::Printf("\n");
	}
}

void Weapons::ConvertWEAP(void)
{
	got_one = false;

	for (int i = 0; i < NUMWEAPONS; i++)
	{
	    if (! all_mode && ! weapon_modified[i])
			continue;

		ConvertWeapon(i);
	}
		
	if (got_one)
		FinishLump();
}

