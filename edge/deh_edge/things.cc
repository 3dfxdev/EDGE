//------------------------------------------------------------------------
//  THING Conversion
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
#include "things.h"

#include "info.h"
#include "frames.h"
#include "misc.h"
#include "mobj.h"
#include "sounds.h"
#include "system.h"
#include "util.h"
#include "wad.h"
#include "weapons.h"


#define EF_DISLOYAL    'D'
#define EF_TRIG_HAPPY  'H'
#define EF_BOSSMAN     'B'
#define EF_NO_RAISE    'R'
#define EF_NO_GRUDGE   'G'
#define EF_NO_ITEM_BK  'I'
#define EF_MONSTER     'M'


void Things::Startup(void)
{
	memset(mobj_modified,  0, sizeof(mobj_modified));
}

namespace Things
{
	bool got_one;
	bool flags_got_one;

	void BeginLump(void)
	{
		WAD::NewLump("DDFTHING");

		WAD::Printf(GEN_BY_COMMENT);
		WAD::Printf("<THINGS>\n\n");
	}

	void FinishLump(void)
	{
		WAD::Printf("\n");
		WAD::FinishLump();
	}

	typedef struct 
	{
		int flag;
		const char *name;
	}
	flagname_t;

	const flagname_t flagnamelist[] =
	{
		{ MF_NOBLOCKMAP,   "NOBLOCKMAP" },
		{ MF_NOBLOOD,      "DAMAGESMOKE" },
		{ MF_NOCLIP,       "NOCLIP" },
		{ MF_NOGRAVITY,    "NOGRAVITY" },
		{ MF_NOSECTOR,     "NOSECTOR" },
		{ MF_NOTDMATCH,    "NODEATHMATCH" },
		{ MF_CORPSE,       "CORPSE" },
		{ MF_COUNTITEM,    "COUNT_AS_ITEM" },
		{ MF_COUNTKILL,    "COUNT_AS_KILL" },
		{ MF_DROPOFF,      "DROPOFF" },
		{ MF_DROPPED,      "DROPPED" },
		{ MF_FLOAT,        "FLOAT" },
		{ MF_MISSILE,      "MISSILE" },
		{ MF_PICKUP,       "PICKUP" },
		{ MF_SHADOW,       "FUZZY" },
		{ MF_SHOOTABLE,    "SHOOTABLE" },
		{ MF_SKULLFLY,     "SKULLFLY" },
		{ MF_SLIDE,        "SLIDER" },
		{ MF_SOLID,        "SOLID" },
		{ MF_SPAWNCEILING, "SPAWNCEILING" },
		{ MF_SPECIAL,      "SPECIAL" },
		{ MF_TELEPORT,     "TELEPORT" },

		{ 0, NULL }  // End sentinel

		/* Not needed: MF_AMBUSH, MF_INFLOAT, MF_JUSTATTACKED, MF_JUSTHIT */
	};

	const flagname_t extflaglist[] =
	{
		{ EF_DISLOYAL,    "DISLOYAL,ATTACK_HURTS" },  // Must be first
		{ EF_TRIG_HAPPY,  "TRIGGER_HAPPY" },
		{ EF_BOSSMAN,     "BOSSMAN" },
		{ EF_NO_RAISE,    "NO_RESURRECT" },
		{ EF_NO_GRUDGE,   "NO_GRUDGE,NEVERTARGETED" },
		{ EF_NO_ITEM_BK,  "NO_RESPAWN" },
		{ EF_MONSTER,     "MONSTER" },

		{ 0, NULL }  // End sentinel
	};

	const char *GetExtFlags(int mt_num, int player)
	{
		if (player > 0)
			return "D";

		switch (mt_num)
		{
			case MT_INS:
			case MT_INV: return "I";

			case MT_POSSESSED:
			case MT_SHOTGUY:
			case MT_CHAINGUY: return "D";

			case MT_SKULL: return "DHM";
			case MT_UNDEAD: return "H";

			case MT_VILE: return "GR";
			case MT_CYBORG: return "BHR";
			case MT_SPIDER: return "BHR";

			case MT_BOSSBRAIN:
			case MT_BOSSSPIT: return "B";

			default:
				break;
		}

		return "";
	}

	void AddOneFlag(const mobjinfo_t *info, const char *name)
	{
		if (! flags_got_one)
		{
			flags_got_one = true;

			if (info->name[0] == '*')
				WAD::Printf("PROJECTILE_SPECIAL = ");
			else
				WAD::Printf("SPECIAL = ");
		}
		else
			WAD::Printf(",");

		WAD::Printf("%s", name);
	}

	void HandleFlags(const mobjinfo_t *info, int mt_num, int player)
	{
		int i;
		int cur_f = info->flags;

		// strangely absent from MT_PLAYER
		if (player)
			cur_f |= MF_SLIDE;

		// EDGE requires teleportman in sector. (DOOM uses thinker list)
		if (mt_num == MT_TELEPORTMAN)
			cur_f &= ~MF_NOSECTOR;

		bool force_disloyal = (Misc::monster_infight == 221 &&
			(info->missilestate || info->meleestate));

		flags_got_one = false;

		for (i = 0; flagnamelist[i].name != NULL; i++)
		{
			if (0 == (cur_f & flagnamelist[i].flag))
				continue;

			cur_f &= ~flagnamelist[i].flag;

			AddOneFlag(info, flagnamelist[i].name);
		}

		const char *eflags = GetExtFlags(mt_num, player);

		for (i = 0; extflaglist[i].name != NULL; i++)
		{
			char ch = (char) extflaglist[i].flag;

			if (ch == EF_DISLOYAL)
				force_disloyal = false;

			if (strchr(eflags, ch))
				AddOneFlag(info, extflaglist[i].name);
		}

		if (force_disloyal)
			AddOneFlag(info, extflaglist[0].name);

		if (flags_got_one)
			WAD::Printf(";\n");

		if (cur_f != 0)
			PrintWarn("Unconverted flags 0x%08x in entry [%s]\n",
				cur_f, info->name);
	}

	const char *GetSound(int sound_id)
	{
		assert(sound_id != sfx_None);
		assert(strlen(S_sfx[sound_id].orig_name) < 16);

		// handle random sounds
		switch (sound_id)
		{
			case sfx_podth1: case sfx_podth2: case sfx_podth3:
				return "\"PODTH?\"";

			case sfx_posit1: case sfx_posit2: case sfx_posit3:
				return "\"POSIT?\"";

			case sfx_bgdth1: case sfx_bgdth2:
				return "\"BGDTH?\"";

			case sfx_bgsit1: case sfx_bgsit2:
				return "\"BGSIT?\"";

			default: break;
		}

		static char name_buf[40];

		sprintf(name_buf, "\"%s\"", StrUpper(S_sfx[sound_id].orig_name));

///---		if (rnd_base != sfx_None)
///---		{
///---			// XXX ideally, check if names differ by a single digit
///---
///---			for (int j = 0; name_buf[j]; j++)
///---			{
///---				if (isdigit(name_buf[j]))
///---				{
///---					name_buf[j] = '?';
///---					break;
///---				}
///---			}
///---		}

		return name_buf;
	}

	const char *GetSpeed(int speed)
	{
		// Interestingly, speed is fixed point for attacks, but
		// plain int for things.  Here we automatically handle both.

		static char num_buf[128];

		if (speed >= 1024)
			sprintf(num_buf, "%1.2f", F_FIXED(speed));
		else
			sprintf(num_buf, "%d", speed);

		return num_buf;
	}

	void HandleSounds(const mobjinfo_t *info, int mt_num)
	{
		if (info->activesound != sfx_None)
		{
			if (info->flags & MF_PICKUP)
				WAD::Printf("PICKUP_SOUND = %s;\n", GetSound(info->activesound));
			else
				WAD::Printf("ACTIVE_SOUND = %s;\n", GetSound(info->activesound));
		}
		else if (mt_num == MT_TELEPORTMAN)
			WAD::Printf("ACTIVE_SOUND = %s;\n", GetSound(sfx_telept));

		if (info->seesound != sfx_None)
			WAD::Printf("SIGHTING_SOUND = %s;\n", GetSound(info->seesound));
		else if (mt_num == MT_BOSSSPIT)
			WAD::Printf("SIGHTING_SOUND = %s;\n", GetSound(sfx_bossit));

		if (info->attacksound != sfx_None && info->meleestate != S_NULL)
		{
			WAD::Printf("STARTCOMBAT_SOUND = %s;\n", GetSound(info->attacksound));
		}

		if (info->painsound != sfx_None)
			WAD::Printf("PAIN_SOUND = %s;\n", GetSound(info->painsound));

		if (info->deathsound != sfx_None)
			WAD::Printf("DEATH_SOUND = %s;\n", GetSound(info->deathsound));
	}

	void HandleFrames(const mobjinfo_t *info, int mt_num)
	{
		Frames::ResetAll();

		// special cases...

		if (mt_num == MT_TELEPORTMAN)
		{
			WAD::Printf("TRANSLUCENCY = 50%;\n");
			WAD::Printf("\n");
			WAD::Printf("STATES(IDLE) = %s:A:-1:NORMAL:TRANS_SET(0%);\n",
				sprnames[SPR_TFOG]);

			// EDGE doesn't use the TELEPORT_FOG object, instead it uses
			// the CHASE states of the TELEPORT_FLASH object (i.e. the one
			// used to find the destination in the target sector).

			if (0 == Frames::BeginGroup(mobjinfo[MT_TFOG].spawnstate, 'E'))
			{
				PrintWarn("Teleport fog has no spawn states.\n");
				return;
			}

			Frames::SpreadGroups();
			Frames::OutputGroup(mobjinfo[MT_TFOG].spawnstate, 'E');

			return;
		}

		// --- collect states into groups ---

		int count = 0;

		if (mt_num == MT_VILE)
			count += Frames::BeginGroup(S_VILE_HEAL1,   'H');

		count += Frames::BeginGroup(info->raisestate,   'R');
		count += Frames::BeginGroup(info->xdeathstate,  'X');
		count += Frames::BeginGroup(info->deathstate,   'D');
		count += Frames::BeginGroup(info->painstate,    'P');
		count += Frames::BeginGroup(info->missilestate, 'M');
		count += Frames::BeginGroup(info->meleestate,   'L');
		count += Frames::BeginGroup(info->seestate,     'E');
		count += Frames::BeginGroup(info->spawnstate,   'S');

		if (count == 0)
		{
			// only occurs with special/invisible objects, currently only
			// with teleport target (handled above) and brain spit targets.

			if (mt_num != MT_BOSSTARGET)
				PrintWarn("Mobj [%s:%d] has no states.\n", info->name, info->doomednum);

			WAD::Printf("TRANSLUCENCY = 0%%;\n");

			WAD::Printf("\n");
			WAD::Printf("STATES(IDLE) = %s:A:-1:NORMAL:NOTHING;\n",
				sprnames[SPR_CAND]);

			return;
		}

		Frames::SpreadGroups();

		Frames::OutputGroup(info->spawnstate,   'S');
		Frames::OutputGroup(info->seestate,     'E');
		Frames::OutputGroup(info->meleestate,   'L');
		Frames::OutputGroup(info->missilestate, 'M');
		Frames::OutputGroup(info->painstate,    'P');
		Frames::OutputGroup(info->deathstate,   'D');
		Frames::OutputGroup(info->xdeathstate,  'X');
		Frames::OutputGroup(info->raisestate,   'R');

		if (mt_num == MT_VILE)
			Frames::OutputGroup(S_VILE_HEAL1,   'H');

		if (Frames::act_flags & AF_WEAPON_ST)
		{
			PrintWarn("Mobj [%s:%d] uses weapon states.\n", info->name, info->doomednum);
		}
	}

	const int NUMPLAYERS = 8;

	typedef struct
	{
		const char *name;
		int num;
		const char *remap;
	}
	playerinfo_t;

	const playerinfo_t player_info[NUMPLAYERS] =
	{
		{ "OUR_HERO",    1, "PLAYER_GREEN" },
		{ "PLAYER2",     2, "PLAYER_DK_GREY" },
		{ "PLAYER3",     3, "PLAYER_BROWN" },
		{ "PLAYER4",     4, "PLAYER_DULL_RED" },
		{ "PLAYER5",  4001, "PLAYER_ORANGE" },
		{ "PLAYER6",  4002, "PLAYER_LT_GREY" },
		{ "PLAYER7",  4003, "PLAYER_LT_RED" },
		{ "PLAYER8",  4004, "PLAYER_PINK"  }
	};

	void HandlePlayer(const mobjinfo_t *info, int player)
	{
		if (player <= 0)
			return;

		assert(player <= NUMPLAYERS);

		const playerinfo_t *pi = player_info + (player - 1);

		WAD::Printf("PLAYER = %d;\n", player);
		WAD::Printf("SIDE = %d;\n", 1 << (player - 1));
		WAD::Printf("PALETTE_REMAP = %s;\n", pi->remap);

		WAD::Printf("INITIAL_BENEFIT = \n");
		WAD::Printf("    BULLETS.LIMIT(%d), ", Ammo::plr_max[am_bullet]);
		WAD::Printf(    "SHELLS.LIMIT(%d), ",  Ammo::plr_max[am_shell]);
		WAD::Printf(    "ROCKETS.LIMIT(%d), ", Ammo::plr_max[am_rocket]);
		WAD::Printf(    "CELLS.LIMIT(%d),\n",  Ammo::plr_max[am_cell]);
		WAD::Printf("    PELLETS.LIMIT(%d), ", 200);
		WAD::Printf(    "NAILS.LIMIT(%d), ",   100);
		WAD::Printf(    "GRENADES.LIMIT(%d), ", 50);
		WAD::Printf(    "GAS.LIMIT(%d),\n",    300);
		WAD::Printf("    BULLETS(%d);\n",      Misc::init_ammo);
	}

	typedef struct
	{
		int mt_num;
		const char *benefit;
		int pars;
		int amount, limit;
		const char *ldf;
		int sound;
	}
	pickupitem_t;

	const pickupitem_t pickup_item[] =
	{
		// Health & Armor....
		{ MT_MISC2, "HEALTH", 2, 1,200, "GotHealthPotion", sfx_itemup },  
		{ MT_MISC10, "HEALTH", 2, 10,100, "GotStim", sfx_itemup },  
		{ MT_MISC11, "HEALTH", 2, 25,100, "GotMedi", sfx_itemup },  
		{ MT_MISC3, "GREEN_ARMOUR", 2, 1,200, "GotArmourHelmet", sfx_itemup },  
		{ MT_MISC0, "GREEN_ARMOUR", 2, 100,100, "GotArmour", sfx_itemup },  
		{ MT_MISC1, "BLUE_ARMOUR", 2, 200,200, "GotMegaArmour", sfx_itemup },  

		// Keys....
		{ MT_MISC4, "KEY_BLUECARD", 0, 0,0, "GotBlueCard", sfx_itemup },
		{ MT_MISC6, "KEY_YELLOWCARD", 0, 0,0, "GotYellowCard", sfx_itemup },
		{ MT_MISC5, "KEY_REDCARD", 0, 0,0, "GotRedCard", sfx_itemup },
		{ MT_MISC8, "KEY_REDSKULL", 0, 0,0, "GotRedSkull", sfx_itemup },
		{ MT_MISC7, "KEY_YELLOWSKULL", 0, 0,0, "GotYellowSkull", sfx_itemup },
		{ MT_MISC9, "KEY_BLUESKULL", 0, 0,0, "GotBlueSkull", sfx_itemup },

		// Ammo....
		{ MT_CLIP,   "BULLETS", 1, 10,0, "GotClip", sfx_itemup },
		{ MT_MISC17, "BULLETS", 1, 50,0, "GotClipBox", sfx_itemup },
		{ MT_MISC22, "SHELLS", 1, 4,0, "GotShells", sfx_itemup },
		{ MT_MISC23, "SHELLS", 1, 20,0, "GotShellBox", sfx_itemup },
		{ MT_MISC18, "ROCKETS", 1, 1,0, "GotRocket", sfx_itemup },
		{ MT_MISC19, "ROCKETS", 1, 5,0, "GotRocketBox", sfx_itemup },
		{ MT_MISC20, "CELLS", 1, 20,0, "GotCell", sfx_itemup },
		{ MT_MISC21, "CELLS", 1, 100,0, "GotCellPack", sfx_itemup },

		// Powerups....
		{ MT_MISC12, "HEALTH", 2, 100,200, "GotSoul", sfx_getpow },  
		{ MT_MISC15, "POWERUP_AUTOMAP", 0, 0,0, "GotMap", sfx_getpow },
		{ MT_INS, "POWERUP_PARTINVIS", 2, 100,100, "GotInvis", sfx_getpow },  
		{ MT_INV, "POWERUP_INVULNERABLE", 2, 30,30, "GotInvulner", sfx_getpow },  
		{ MT_MISC16, "POWERUP_LIGHTGOGGLES", 2, 120,120, "GotVisor", sfx_getpow },  
		{ MT_MISC14, "POWERUP_ACIDSUIT", 2, 60,60, "GotSuit", sfx_getpow },  

		// Weapons....
		{ MT_SHOTGUN, "SHOTGUN,SHELLS", 1, 8,0, "GotShotGun", sfx_wpnup },
		{ MT_SUPERSHOTGUN, "SUPERSHOTGUN,SHELLS", 1, 8,0, "GotDoubleBarrel", sfx_wpnup },
		{ MT_CHAINGUN, "CHAINGUN,BULLETS", 1, 20,0, "GotChainGun", sfx_wpnup },
		{ MT_MISC27, "ROCKET_LAUNCHER,ROCKETS", 1, 2,0, "GotRocketLauncher", sfx_wpnup },
		{ MT_MISC28, "PLASMA_RIFLE,CELLS", 1, 40,0, "GotPlasmaGun", sfx_wpnup },
		{ MT_MISC25, "BFG9000,CELLS", 1, 40,0, "GotBFG", sfx_wpnup },
		{ MT_MISC26, "CHAINSAW", 0, 0,0, "GotChainSaw", sfx_wpnup },
		
		{ -1, NULL, 0,0,0, NULL }
	};

	void HandleItem(const mobjinfo_t *info, int mt_num)
	{
		// special cases:

		if (mt_num == MT_MISC13) // Berserk
		{
			WAD::Printf("PICKUP_BENEFIT = POWERUP_BERSERK(60:60),HEALTH(100:100);\n");
			WAD::Printf("PICKUP_MESSAGE = GotBerserk;\n");
			WAD::Printf("PICKUP_SOUND = %s;\n", GetSound(sfx_getpow));
			return;
		}
		else if (mt_num == MT_MEGA)  // Megasphere
		{
			WAD::Printf("PICKUP_BENEFIT = ");
			WAD::Printf("HEALTH(%d:%d),", Misc::mega_health, Misc::mega_health);
			WAD::Printf("BLUE_ARMOUR(%d:%d);\n", Misc::max_armour, Misc::max_armour);
			WAD::Printf("PICKUP_MESSAGE = GotMega;\n");
			WAD::Printf("PICKUP_SOUND = %s;\n", GetSound(sfx_getpow));
			return;
		}
		else if (mt_num == MT_MISC24)  // Backpack full of AMMO
		{
			WAD::Printf("PICKUP_BENEFIT = \n");
			WAD::Printf("    BULLETS.LIMIT(%d), ", 2 * Ammo::plr_max[am_bullet]);
			WAD::Printf("    SHELLS.LIMIT(%d),\n", 2 * Ammo::plr_max[am_shell]);
			WAD::Printf("    ROCKETS.LIMIT(%d), ", 2 * Ammo::plr_max[am_rocket]);
			WAD::Printf("    CELLS.LIMIT(%d),\n",  2 * Ammo::plr_max[am_cell]);
			WAD::Printf("    BULLETS(10), SHELLS(4), ROCKETS(1), CELLS(20);\n");
			WAD::Printf("PICKUP_MESSAGE = GotBackpack;\n");
			WAD::Printf("PICKUP_SOUND = %s;\n", GetSound(sfx_itemup));
			return;
		}

		int i;

		for (i = 0; pickup_item[i].benefit != NULL; i++)
		{
			if (mt_num == pickup_item[i].mt_num)
				break;
		}

		const pickupitem_t *pu = pickup_item + i;

		if (pu->benefit == NULL)  // not found
			return;

		int amount = pu->amount;
		int limit  = pu->limit;

		// handle patchable amounts

		switch (mt_num)
		{
			// Armor & health...
			case MT_MISC3:   // "ARMOUR_HELMET"  
				limit  = Misc::max_armour;
				break;

			case MT_MISC0:   // "GREEN_ARMOUR"
				amount = Misc::green_armour_class * 100;
				limit  = Misc::max_armour;
				break;

			case MT_MISC1:   // "BLUE_ARMOUR"  
				amount = Misc::blue_armour_class * 100;
				limit  = Misc::max_armour;
				break;

			case MT_MISC2:    // "HEALTH_POTION"
				limit  = Misc::max_health;  // Note: *not* MEDIKIT
				break;

			case MT_MISC12:   // "SOULSPHERE"  
				amount = Misc::soul_health;
				limit  = Misc::soul_limit;
				break;

			// Ammo...
			case MT_CLIP:     // "CLIP"
			case MT_MISC17:   // "BOX_OF_BULLETS"  
				amount = Ammo::pickups[am_bullet];
				break;

			case MT_MISC22:   // "SHELLS"  
			case MT_MISC23:   // "BOX_OF_SHELLS"  
				amount = Ammo::pickups[am_shell];
				break;

			case MT_MISC18:   // "ROCKET"  
			case MT_MISC19:   // "BOX_OF_ROCKETS"  
				amount = Ammo::pickups[am_rocket];
				break;

			case MT_MISC20:   // "CELLS"  
			case MT_MISC21:   // "CELL_PACK"  
				amount = Ammo::pickups[am_cell];
				break;

			default:
				break;
		}

		// big boxes of ammo
		if (mt_num == MT_MISC17 || mt_num == MT_MISC19 ||
			mt_num == MT_MISC21 || mt_num == MT_MISC23)
		{
			amount *= 5;
		}

		if (pu->pars == 2 && amount > limit)
			amount = limit;

		WAD::Printf("PICKUP_BENEFIT = %s", pu->benefit);

		if (pu->pars == 1)
			WAD::Printf("(%d)", amount);
		else if (pu->pars == 2)
			WAD::Printf("(%d:%d)", amount, limit);

		WAD::Printf(";\n");
		WAD::Printf("PICKUP_MESSAGE = %s;\n", pu->ldf);

		if (info->activesound == sfx_None)
			WAD::Printf("PICKUP_SOUND = %s;\n", GetSound(pu->sound));
	}

	void HandleCastOrder(const mobjinfo_t *info, int mt_num, int player)
	{
		if (player >= 2)
			return;

		int order = 0;

		switch (mt_num)
		{
			case MT_PLAYER:    order =  1; break;
			case MT_POSSESSED: order =  2; break;
			case MT_SHOTGUY:   order =  3; break;
			case MT_CHAINGUY:  order =  4; break;
			case MT_TROOP:     order =  5; break;
			case MT_SERGEANT:  order =  6; break;
			case MT_SKULL:     order =  7; break;
			case MT_HEAD:      order =  8; break;
			case MT_KNIGHT:    order =  9; break;
			case MT_BRUISER:   order = 10; break;
			case MT_BABY:      order = 11; break;
			case MT_PAIN:      order = 12; break;
			case MT_UNDEAD:    order = 13; break;
			case MT_FATSO:     order = 14; break;
			case MT_VILE:      order = 15; break;
			case MT_SPIDER:    order = 16; break;
			case MT_CYBORG:    order = 17; break;

			default:
				return;
		}

		WAD::Printf("CASTORDER = %d;\n", order);
	}

	void HandleDropItem(const mobjinfo_t *info, int mt_num)
	{
		const char *item = NULL;

		switch (mt_num)
		{
			case MT_WOLFSS:
			case MT_POSSESSED: item = "CLIP"; break;

			case MT_SHOTGUY:   item = "SHOTGUN"; break;
			case MT_CHAINGUY:  item = "CHAINGUN"; break;

			default:
				return;
		}

		assert(item);

		WAD::Printf("DROPITEM = \"%s\";\n", item);
	}

	void HandleAttacks(const mobjinfo_t *info, int mt_num)
	{
		if (! Frames::attack_slot[0] && ! Frames::attack_slot[1] &&
			! Frames::attack_slot[2])
		{
			return;
		}

		WAD::Printf("\n");

		if (Frames::attack_slot[Frames::RANGE])
		{
			WAD::Printf("RANGE_ATTACK = %s;\n",
				Frames::attack_slot[Frames::RANGE]);
			WAD::Printf("MINATTACK_CHANCE = 25%%;\n");
		}

		if (Frames::attack_slot[Frames::COMBAT])
			WAD::Printf("CLOSE_ATTACK = %s;\n",
				Frames::attack_slot[Frames::COMBAT]);

		if (Frames::attack_slot[Frames::SPARE])
			WAD::Printf("SPARE_ATTACK = %s;\n",
				Frames::attack_slot[Frames::SPARE]);
	}

	void ConvertMobj(const mobjinfo_t *info, int mt_num, int player);
}

void Things::ConvertMobj(const mobjinfo_t *info, int mt_num, int player)
{
	if (! got_one)
	{
		got_one = true;
		BeginLump();
	}
	
	if (info->name[0] == '*')  // attack
		return;

	if (player > 0)
		WAD::Printf("[%s:%d]\n", player_info[player-1].name, player_info[player-1].num);
	else if (info->doomednum < 0)
		WAD::Printf("[%s]\n", info->name);
	else
		WAD::Printf("[%s:%d]\n", info->name, info->doomednum);

	WAD::Printf("RADIUS = %1.1f;\n", F_FIXED(info->radius));
	WAD::Printf("HEIGHT = %1.1f;\n", F_FIXED(info->height));

	if (info->spawnhealth != 1000)
		WAD::Printf("SPAWNHEALTH = %d;\n", info->spawnhealth);

	if (player > 0)
		WAD::Printf("SPEED = 1;\n");
	else if (info->speed != 0)
		WAD::Printf("SPEED = %s;\n", GetSpeed(info->speed));

	if (info->mass != 100)
		WAD::Printf("MASS = %d;\n", info->mass);

	if (info->reactiontime != 0)
		WAD::Printf("REACTION_TIME = %dT;\n", info->reactiontime);

	if (info->painchance != 0)
		WAD::Printf("PAINCHANCE = %1.1f%%;\n",
			(float)info->painchance * 100.0 / 256.0);

	if (mt_num == MT_BOSSSPIT)
		WAD::Printf("SPIT_SPOT = BRAIN_SPAWNSPOT;\n");

	HandleCastOrder(info, mt_num, player);
	HandleDropItem(info, mt_num);
	HandleFlags(info, mt_num, player);
	HandlePlayer(info, player);
	HandleItem(info, mt_num);
	HandleSounds(info, mt_num);
	HandleFrames(info, mt_num);
	HandleAttacks(info, mt_num);

	if (Frames::act_flags & AF_EXPLODE)
		WAD::Printf("\nEXPLODE_DAMAGE.VAL = 128;\n");
	
	WAD::Printf("\n");
}

void Things::ConvertTHING(void)
{
	got_one = false;

	for (int i = 0; i < NUMMOBJTYPES; i++)
	{
	    if (! all_mode && ! mobj_modified[i])
			continue;

		if (i == MT_PLAYER)
		{
			for (int p = 1; p <= NUMPLAYERS; p++)
				ConvertMobj(mobjinfo + i, i, p);

			continue;
		}

		ConvertMobj(mobjinfo + i, i, 0);
	}

	if (true)  // XXX Modified
		ConvertMobj(&brain_explode_mobj, MT_ROCKET /* dummy */, 0);

	if (got_one)
		FinishLump();
}

void Things::MarkThing(int mt_num)
{
	assert(1 <= mt_num && mt_num < NUMMOBJTYPES);

	mobj_modified[mt_num] = true;

	// handle merged things/attacks

	if (mt_num == MT_TFOG)
		mobj_modified[MT_TELEPORTMAN] = true;
	
	if (mt_num == MT_SPAWNFIRE)
		mobj_modified[MT_SPAWNSHOT] = true;
}

// NOTES
//
//   Need to duplicate players
//   info->damage only used in attacks and LOST_SOUL
