//------------------------------------------------------------------------
//  MISCELLANEOUS Definitions
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
#include "misc.h"

#include "info.h"
#include "mobj.h"
#include "patch.h"
#include "sounds.h"
#include "storage.h"
#include "system.h"
#include "things.h"
#include "util.h"
#include "weapons.h"


int Misc::init_ammo   = 50;

int Misc::max_armour  = 200;
int Misc::max_health  = 200;

int Misc::green_armour_class = 1;
int Misc::blue_armour_class  = 2;
int Misc::bfg_cells_per_shot = 40;

int Misc::soul_health  = 200;
int Misc::soul_limit   = 200;
int Misc::mega_health  = 200;

int Misc::monster_infight = 202;


typedef struct
{
	const char *deh_name;
	int minimum;
	int *var;
	const int *affected_mobjs;
}
miscinfo_t;

namespace Misc
{
	int init_ammo_mobj[] = { MT_PLAYER, -1 };

	int max_heal_mobj[] = { MT_MISC2, -1 };
	int max_arm_mobj [] = { MT_MISC0, MT_MISC1, MT_MISC3, MT_MEGA, -1 };

	int green_class_mobj[] = { MT_MISC0, -1 };
	int blue_class_mobj [] = { MT_MISC1, -1 };

	int soulsphere_mobj [] = { MT_MISC12, -1 };
	int megasphere_mobj [] = { MT_MEGA,   -1 };

	const miscinfo_t misc_info[] =
	{
		{ "Initial Bullets",   0, &init_ammo, init_ammo_mobj },
		{ "Max Health",        1, &max_health, max_heal_mobj },
		{ "Max Armor",         1, &max_armour, max_arm_mobj },
		{ "Green Armor Class", 0, &green_armour_class, green_class_mobj },
		{ "Blue Armor Class",  0, &blue_armour_class,  blue_class_mobj },
		{ "Max Soulsphere",    1, &soul_limit,  soulsphere_mobj },
		{ "Soulsphere Health", 1, &soul_health, soulsphere_mobj },
		{ "Megasphere Health", 1, &mega_health, megasphere_mobj },

		{ "God Mode Health",   0, NULL, NULL },
		{ "IDFA Armor",        0, NULL, NULL },
		{ "IDFA Armor Class",  0, NULL, NULL },
		{ "IDKFA Armor",       0, NULL, NULL },
		{ "IDKFA Armor Class", 0, NULL, NULL },

		{ NULL, 0, NULL, 0 }  // End sentinel
	};
}


//------------------------------------------------------------------------

namespace Misc
{
	void MarkAllMonsters(void)
	{
		for (int i = 0; i < NUMMOBJTYPES; i++)
		{
			mobjinfo_t *mobj = mobjinfo + i;

			if (! mobj->meleestate && ! mobj->missilestate)
				continue;

			Things::MarkThing(i);
		}
	}
}

void Misc::AlterMisc(int new_val)
{
	const char *misc_name = Patch::line_buf;

	// --- special cases ---

	if (StrCaseCmp(misc_name, "Initial Health") == 0)
	{
		if (new_val < 1)
		{
			PrintWarn("Bad value '%d' for MISC field: %s\n", new_val, misc_name);
			return;
		}

		Storage::RememberMod(&mobjinfo[MT_PLAYER].spawnhealth, new_val);

		Things::MarkThing(MT_PLAYER);
		return;
	}

	if (StrCaseCmp(misc_name, "BFG Cells/Shot") == 0)
	{
		if (new_val < 1)
		{
			PrintWarn("Bad value '%d' for MISC field: %s\n", new_val, misc_name);
			return;
		}

		Storage::RememberMod(&bfg_cells_per_shot, new_val);

		Weapons::MarkWeapon(wp_bfg);
		return;
	}

	if (StrCaseCmp(misc_name, "Monsters Infight") == 0)
	{
		if (new_val != 202 && new_val != 221)
		{
			PrintWarn("Bad value '%d' for MISC field: %s\n", new_val, misc_name);
			return;
		}
		
		Storage::RememberMod(&monster_infight, new_val);

		if (monster_infight == 221)
			MarkAllMonsters();

		return;
	}
	
	// --- normal variables ---

	int j;

	for (j = 0; misc_info[j].deh_name; j++)
	{
		if (StrCaseCmp(misc_name, misc_info[j].deh_name) == 0)
			break;
	}

	const miscinfo_t *info = misc_info + j;

	if (! info->deh_name)
	{
		PrintWarn("UNKNOWN MISC FIELD: %s\n", misc_name);
		return;
	}

	if (! info->var)
	{
		PrintWarn("Ignoring MISC field: %s\n", misc_name);
		return;
	}

	if (new_val < info->minimum)  // mainly here to disallow negative values
	{
		PrintWarn("Bad value '%d' for MISC field: %s\n", new_val, misc_name);
		new_val = info->minimum;
	}

	Storage::RememberMod(info->var, new_val);

	// mark mobjs that have been modified

	const int *affect = info->affected_mobjs;
	assert(affect);

	for (; *affect >= 0; affect++)
	{
		Things::MarkThing(*affect);
	}
}

