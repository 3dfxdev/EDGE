//------------------------------------------------------------------------
//  FRAME Handling
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
#include "frames.h"

#include "info.h"
#include "system.h"
#include "things.h"
#include "wad.h"
#include "weapons.h"


#define DEBUG_RANGES  0


const char *Frames::attack_slot[3];

int Frames::act_flags;

namespace Frames
{
	int lowest_touched;
	int highest_touched;
}


#define IS_WEAPON(group)  (islower(group))


typedef struct
{
	const char *ddf_name;

	int act_flags;

	// attacks implied by the action, often NULL.  The format is
	// "X:ATTACK_NAME" where X is 'R' for range attacks, 'C' for
	// close-combat attacks, and 'S' for spare attacks.
	const char *atk_1;
	const char *atk_2;
}
actioninfo_t;

static const actioninfo_t action_info[NUMACTIONS] =
{
    { "NOTHING", 0, NULL, NULL },     // A_NULL
    { "LIGHT0", 0, NULL, NULL },      // A_Light0
    { "READY", 0, NULL, NULL },       // A_WeaponReady
    { "LOWER", 0, NULL, NULL },       // A_Lower
    { "RAISE", 0, NULL, NULL },       // A_Raise
    { "SHOOT", 0, "C:PLAYER_PUNCH", NULL },       // A_Punch
    { "REFIRE", 0, NULL, NULL },      // A_ReFire
    { "SHOOT", 0, "R:PLAYER_PISTOL", NULL },       // A_FirePistol
    { "LIGHT1", 0, NULL, NULL },      // A_Light1
    { "SHOOT", 0, "R:PLAYER_SHOTGUN", NULL },       // A_FireShotgun
    { "LIGHT2", 0, NULL, NULL },      // A_Light2
    { "SHOOT", 0, "R:PLAYER_SHOTGUN2", NULL },       // A_FireShotgun2
    { "CHECKRELOAD", 0, NULL, NULL }, // A_CheckReload
    { "PLAYSOUND(DBOPN)", 0, NULL, NULL },  // A_OpenShotgun2
    { "PLAYSOUND(DBLOAD)", 0, NULL, NULL }, // A_LoadShotgun2
    { "PLAYSOUND(DBCLS)", 0, NULL, NULL },  // A_CloseShotgun2
    { "SHOOT", 0, "R:PLAYER_CHAINGUN", NULL },      // A_FireCGun
    { "FLASH", 0, NULL, NULL },      // A_GunFlash
    { "SHOOT", 0, "R:PLAYER_MISSILE", NULL },      // A_FireMissile
    { "SHOOT", 0, "C:PLAYER_SAW", NULL },      // A_Saw
    { "SHOOT", 0, "R:PLAYER_PLASMA", NULL },      // A_FirePlasma
    { "PLAYSOUND(BFG)", 0, NULL, NULL },      // A_BFGsound
    { "SHOOT", 0, "R:PLAYER_BFG9000", NULL },      // A_FireBFG
    { "SPARE_ATTACK", 0, NULL, NULL },      // A_BFGSpray
    { "EXPLOSIONDAMAGE", AF_EXPLODE, NULL, NULL },      // A_Explode
    { "MAKEPAINSOUND", 0, NULL, NULL },      // A_Pain
    { "PLAYER_SCREAM", 0, NULL, NULL },      // A_PlayerScream
    { "MAKEDEAD", 0, NULL, NULL },      // A_Fall
    { "MAKEOVERKILLSOUND", 0, NULL, NULL },      // A_XScream
    { "LOOKOUT", 0, NULL, NULL },      // A_Look
    { "CHASE", 0, NULL, NULL },      // A_Chase
    { "FACETARGET", 0, NULL, NULL },      // A_FaceTarget
    { "RANGE_ATTACK", 0, "R:FORMER_HUMAN_PISTOL", NULL },      // A_PosAttack
    { "MAKEDEATHSOUND", 0, NULL, NULL },      // A_Scream
    { "RANGE_ATTACK", 0, "R:FORMER_HUMAN_SHOTGUN", NULL },      // A_SPosAttack
    { "RESCHASE", 0, NULL, NULL },      // A_VileChase
    { "RANGEATTEMPTSND", 0, NULL, NULL },      // A_VileStart
    { "RANGE_ATTACK", 0, "R:ARCHVILE_FIRE", NULL },      // A_VileTarget
    { "EFFECTTRACKER", 0, NULL, NULL },      // A_VileAttack
    { "TRACKERSTART", 0, NULL, NULL },      // A_StartFire
    { "TRACKERFOLLOW", 0, NULL, NULL },      // A_Fire
    { "TRACKERACTIVE", 0, NULL, NULL },      // A_FireCrackle
    { "RANDOM_TRACER", 0, NULL, NULL },      // A_Tracer
    { "CLOSEATTEMPTSND", 0, NULL, NULL },      // A_SkelWhoosh
    { "CLOSE_ATTACK", 0, "C:REVENANT_CLOSECOMBAT", NULL },      // A_SkelFist
    { "RANGE_ATTACK", 0, "R:REVENANT_MISSILE", NULL },      // A_SkelMissile
    { "RANGEATTEMPTSND", 0, NULL, NULL },      // A_FatRaise
    { "RESET_SPREADER", 0, NULL, NULL },      // A_FatAttack1
    { "RANGE_ATTACK", 0, "R:MANCUBUS_FIREBALL", NULL },      // A_FatAttack2
    { "RANGE_ATTACK", 0, "R:MANCUBUS_FIREBALL", NULL },      // A_FatAttack3
    { "NOTHING", 0, NULL, NULL },      // A_BossDeath
    { "RANGE_ATTACK", 0, "R:FORMER_HUMAN_CHAINGUN", NULL },      // A_CPosAttack
    { "REFIRE_CHECK", 0, NULL, NULL },      // A_CPosRefire
    { "COMBOATTACK", 0, "R:IMP_FIREBALL", "C:IMP_CLOSECOMBAT" }, // A_TroopAttack
    { "CLOSE_ATTACK", 0, "C:DEMON_CLOSECOMBAT", NULL },      // A_SargAttack
    { "COMBOATTACK", 0, "R:CACO_FIREBALL", "C:CACO_CLOSECOMBAT" }, // A_HeadAttack
    { "COMBOATTACK", 0, "R:BARON_FIREBALL", "C:BARON_CLOSECOMBAT" }, // A_BruisAttack
    { "RANGE_ATTACK", 0, "R:SKULL_ASSAULT", NULL },      // A_SkullAttack
    { "WALKSOUND_CHASE", 0, NULL, NULL },      // A_Metal
    { "REFIRE_CHECK", 0, NULL, NULL },      // A_SpidRefire
    { "WALKSOUND_CHASE", 0, NULL, NULL },      // A_BabyMetal
    { "RANGE_ATTACK", 0, "R:ARACHNOTRON_PLASMA", NULL },      // A_BspiAttack
    { "PLAYSOUND(HOOF)", 0, NULL, NULL },      // A_Hoof
    { "RANGE_ATTACK", 0, "R:CYBERDEMON_MISSILE", NULL },      // A_CyberAttack
    { "RANGE_ATTACK", 0, "R:ELEMENTAL_SPAWNER", NULL },      // A_PainAttack
    { "SPARE_ATTACK", 0, "S:ELEMENTAL_DEATHSPAWN", NULL },      // A_PainDie
    { "NOTHING", 0, NULL, NULL },      // A_KeenDie
    { "MAKEPAINSOUND", 0, NULL, NULL },      // A_BrainPain
    { "BRAINSCREAM", 0, NULL, NULL },      // A_BrainScream
    { "BRAINDIE", 0, NULL, NULL },      // A_BrainDie
    { "NOTHING", 0, NULL, NULL },      // A_BrainAwake
    { "BRAINSPIT", 0, "R:BRAIN_CUBE", NULL },      // A_BrainSpit
    { "MAKEACTIVESOUND", 0, NULL, NULL },      // A_SpawnSound
    { "CUBETRACER", 0, NULL, NULL },      // A_SpawnFly
    { "BRAINMISSILEEXPLODE", 0, NULL, NULL },     // A_BrainExplode
    { "CUBESPAWN", 0, NULL, NULL }      // A_CubeSpawn (NEW)
};


//------------------------------------------------------------------------

typedef struct
{
	int obj_num;  // thing or weapon
	int start1, end1;
	int start2, end2;
}
staterange_t;

static const staterange_t thing_range[] =
{
	// Things...
    { MT_PLAYER, S_PLAY, S_PLAY_XDIE9, -1,-1 },
    { MT_POSSESSED, S_POSS_STND, S_POSS_RAISE4, -1,-1 },
    { MT_SHOTGUY, S_SPOS_STND, S_SPOS_RAISE5, -1,-1 },
    { MT_VILE, S_VILE_STND, S_VILE_DIE10, -1,-1 },
    { MT_UNDEAD, S_SKEL_STND, S_SKEL_RAISE6, -1,-1 },
    { MT_SMOKE, S_SMOKE1, S_SMOKE5, -1,-1 },
    { MT_FATSO, S_FATT_STND, S_FATT_RAISE8, -1,-1 },
    { MT_CHAINGUY, S_CPOS_STND, S_CPOS_RAISE7, -1,-1 },
    { MT_TROOP, S_TROO_STND, S_TROO_RAISE5, -1,-1 },
    { MT_SERGEANT, S_SARG_STND, S_SARG_RAISE6, -1,-1 },
    { MT_SHADOWS, S_SARG_STND, S_SARG_RAISE6, -1,-1 },
    { MT_HEAD, S_HEAD_STND, S_HEAD_RAISE6, -1,-1 },
    { MT_BRUISER, S_BOSS_STND, S_BOSS_RAISE7, -1,-1 },
    { MT_KNIGHT, S_BOS2_STND, S_BOS2_RAISE7, -1,-1 },
    { MT_SKULL, S_SKULL_STND, S_SKULL_DIE6, -1,-1 },
    { MT_SPIDER, S_SPID_STND, S_SPID_DIE11, -1,-1 },
    { MT_BABY, S_BSPI_STND, S_BSPI_RAISE7, -1,-1 },
    { MT_CYBORG, S_CYBER_STND, S_CYBER_DIE10, -1,-1 },
    { MT_PAIN, S_PAIN_STND, S_PAIN_RAISE6, -1,-1 },
    { MT_WOLFSS, S_SSWV_STND, S_SSWV_RAISE5, -1,-1 },
    { MT_KEEN, S_KEENSTND, S_KEENPAIN2, -1,-1 },
    { MT_BOSSBRAIN, S_BRAIN, S_BRAIN_DIE4, -1,-1 },
    { MT_BOSSSPIT, S_BRAINEYE, S_BRAINEYE1, -1,-1 },
    { MT_BARREL, S_BAR1, S_BEXP5, -1,-1 },
    { MT_PUFF, S_PUFF1, S_PUFF4, -1,-1 },
    { MT_BLOOD, S_BLOOD1, S_BLOOD3, -1,-1 },
    { MT_TFOG, S_TFOG, S_TFOG10, -1,-1 },
    { MT_IFOG, S_IFOG, S_IFOG5, -1,-1 },
    { MT_TELEPORTMAN, S_TFOG, S_TFOG10, -1,-1 },

    { MT_MISC0, S_ARM1, S_ARM1A, -1,-1 },
    { MT_MISC1, S_ARM2, S_ARM2A, -1,-1 },
    { MT_MISC2, S_BON1, S_BON1E, -1,-1 },
    { MT_MISC3, S_BON2, S_BON2E, -1,-1 },
    { MT_MISC4, S_BKEY, S_BKEY2, -1,-1 },
    { MT_MISC5, S_RKEY, S_RKEY2, -1,-1 },
    { MT_MISC6, S_YKEY, S_YKEY2, -1,-1 },
    { MT_MISC7, S_YSKULL, S_YSKULL2, -1,-1 },
    { MT_MISC8, S_RSKULL, S_RSKULL2, -1,-1 },
    { MT_MISC9, S_BSKULL, S_BSKULL2, -1,-1 },
    { MT_MISC10, S_STIM, S_STIM, -1,-1 },
    { MT_MISC11, S_MEDI, S_MEDI, -1,-1 },
    { MT_MISC12, S_SOUL, S_SOUL6, -1,-1 },
    { MT_INV, S_PINV, S_PINV4, -1,-1 },
    { MT_MISC13, S_PSTR, S_PSTR, -1,-1 },
    { MT_INS, S_PINS, S_PINS4, -1,-1 },
    { MT_MISC14, S_SUIT, S_SUIT, -1,-1 },
    { MT_MISC15, S_PMAP, S_PMAP6, -1,-1 },
    { MT_MISC16, S_PVIS, S_PVIS2, -1,-1 },
    { MT_MEGA, S_MEGA, S_MEGA4, -1,-1 },
    { MT_CLIP, S_CLIP, S_CLIP, -1,-1 },
    { MT_MISC17, S_AMMO, S_AMMO, -1,-1 },
    { MT_MISC18, S_ROCK, S_ROCK, -1,-1 },
    { MT_MISC19, S_BROK, S_BROK, -1,-1 },
    { MT_MISC20, S_CELL, S_CELL, -1,-1 },
    { MT_MISC21, S_CELP, S_CELP, -1,-1 },
    { MT_MISC22, S_SHEL, S_SHEL, -1,-1 },
    { MT_MISC23, S_SBOX, S_SBOX, -1,-1 },
    { MT_MISC24, S_BPAK, S_BPAK, -1,-1 },
    { MT_MISC25, S_BFUG, S_BFUG, -1,-1 },
    { MT_CHAINGUN, S_MGUN, S_MGUN, -1,-1 },
    { MT_MISC26, S_CSAW, S_CSAW, -1,-1 },
    { MT_MISC27, S_LAUN, S_LAUN, -1,-1 },
    { MT_MISC28, S_PLAS, S_PLAS, -1,-1 },
    { MT_SHOTGUN, S_SHOT, S_SHOT, -1,-1 },
    { MT_SUPERSHOTGUN, S_SHOT2, S_SHOT2, -1,-1 },
    { MT_MISC29, S_TECHLAMP, S_TECHLAMP4, -1,-1 },
    { MT_MISC30, S_TECH2LAMP, S_TECH2LAMP4, -1,-1 },
    { MT_MISC31, S_COLU, S_COLU, -1,-1 },
    { MT_MISC32, S_TALLGRNCOL, S_TALLGRNCOL, -1,-1 },
    { MT_MISC33, S_SHRTGRNCOL, S_SHRTGRNCOL, -1,-1 },
    { MT_MISC34, S_TALLREDCOL, S_TALLREDCOL, -1,-1 },
    { MT_MISC35, S_SHRTREDCOL, S_SHRTREDCOL, -1,-1 },
    { MT_MISC36, S_SKULLCOL, S_SKULLCOL, -1,-1 },
    { MT_MISC37, S_HEARTCOL, S_HEARTCOL2, -1,-1 },
    { MT_MISC38, S_EVILEYE, S_EVILEYE4, -1,-1 },
    { MT_MISC39, S_FLOATSKULL, S_FLOATSKULL3, -1,-1 },
    { MT_MISC40, S_TORCHTREE, S_TORCHTREE, -1,-1 },
    { MT_MISC41, S_BLUETORCH, S_BLUETORCH4, -1,-1 },
    { MT_MISC42, S_GREENTORCH, S_GREENTORCH4, -1,-1 },
    { MT_MISC43, S_REDTORCH, S_REDTORCH4, -1,-1 },
    { MT_MISC44, S_BTORCHSHRT, S_BTORCHSHRT4, -1,-1 },
    { MT_MISC45, S_GTORCHSHRT, S_GTORCHSHRT4, -1,-1 },
    { MT_MISC46, S_RTORCHSHRT, S_RTORCHSHRT4, -1,-1 },
    { MT_MISC47, S_STALAGTITE, S_STALAGTITE, -1,-1 },
    { MT_MISC48, S_TECHPILLAR, S_TECHPILLAR, -1,-1 },
    { MT_MISC49, S_CANDLESTIK, S_CANDLESTIK, -1,-1 },
    { MT_MISC50, S_CANDELABRA, S_CANDELABRA, -1,-1 },
    { MT_MISC51, S_BLOODYTWITCH, S_BLOODYTWITCH4, -1,-1 },
    { MT_MISC60, S_BLOODYTWITCH, S_BLOODYTWITCH4, -1,-1 },
    { MT_MISC52, S_MEAT2, S_MEAT2, -1,-1 },
    { MT_MISC53, S_MEAT3, S_MEAT3, -1,-1 },
    { MT_MISC54, S_MEAT4, S_MEAT4, -1,-1 },
    { MT_MISC55, S_MEAT5, S_MEAT5, -1,-1 },
    { MT_MISC56, S_MEAT2, S_MEAT2, -1,-1 },
    { MT_MISC57, S_MEAT4, S_MEAT4, -1,-1 },
    { MT_MISC58, S_MEAT3, S_MEAT3, -1,-1 },
    { MT_MISC59, S_MEAT5, S_MEAT5, -1,-1 },
    { MT_MISC61, S_HEAD_DIE6, S_HEAD_DIE6, -1,-1 },
    { MT_MISC62, S_PLAY_DIE7, S_PLAY_DIE7, -1,-1 },
    { MT_MISC63, S_POSS_DIE5, S_POSS_DIE5, -1,-1 },
    { MT_MISC64, S_SARG_DIE6, S_SARG_DIE6, -1,-1 },
    { MT_MISC65, S_SKULL_DIE6, S_SKULL_DIE6, -1,-1 },
    { MT_MISC66, S_TROO_DIE5, S_TROO_DIE5, -1,-1 },
    { MT_MISC67, S_SPOS_DIE5, S_SPOS_DIE5, -1,-1 },
    { MT_MISC68, S_PLAY_XDIE9, S_PLAY_XDIE9, -1,-1 },
    { MT_MISC69, S_PLAY_XDIE9, S_PLAY_XDIE9, -1,-1 },
    { MT_MISC70, S_HEADSONSTICK, S_HEADSONSTICK, -1,-1 },
    { MT_MISC71, S_GIBS, S_GIBS, -1,-1 },
    { MT_MISC72, S_HEADONASTICK, S_HEADONASTICK, -1,-1 },
    { MT_MISC73, S_HEADCANDLES, S_HEADCANDLES2, -1,-1 },
    { MT_MISC74, S_DEADSTICK, S_DEADSTICK, -1,-1 },
    { MT_MISC75, S_LIVESTICK, S_LIVESTICK2, -1,-1 },
    { MT_MISC76, S_BIGTREE, S_BIGTREE, -1,-1 },
    { MT_MISC77, S_BBAR1, S_BBAR3, -1,-1 },
    { MT_MISC78, S_HANGNOGUTS, S_HANGNOGUTS, -1,-1 },
    { MT_MISC79, S_HANGBNOBRAIN, S_HANGBNOBRAIN, -1,-1 },
    { MT_MISC80, S_HANGTLOOKDN, S_HANGTLOOKDN, -1,-1 },
    { MT_MISC81, S_HANGTSKULL, S_HANGTSKULL, -1,-1 },
    { MT_MISC82, S_HANGTLOOKUP, S_HANGTLOOKUP, -1,-1 },
    { MT_MISC83, S_HANGTNOBRAIN, S_HANGTNOBRAIN, -1,-1 },
    { MT_MISC84, S_COLONGIBS, S_COLONGIBS, -1,-1 },
    { MT_MISC85, S_SMALLPOOL, S_SMALLPOOL, -1,-1 },
    { MT_MISC86, S_BRAINSTEM, S_BRAINSTEM, -1,-1 },

	/* BRAIN_DEATH_MISSILE : S_BRAINEXPLODE1, S_BRAINEXPLODE3 */

	// Attacks...
    { MT_FIRE, S_FIRE1, S_FIRE30, -1,-1 },
    { MT_TRACER, S_TRACER, S_TRACEEXP3, -1,-1 },
    { MT_FATSHOT, S_FATSHOT1, S_FATSHOTX3, -1,-1 },
    { MT_BRUISERSHOT, S_BRBALL1, S_BRBALLX3, -1,-1 },
    { MT_SPAWNSHOT, S_SPAWN1, S_SPAWNFIRE8, -1,-1 },
    { MT_TROOPSHOT, S_TBALL1, S_TBALLX3, -1,-1 },
    { MT_HEADSHOT, S_RBALL1, S_RBALLX3, -1,-1 },
    { MT_ARACHPLAZ, S_ARACH_PLAZ, S_ARACH_PLEX5, -1,-1 },
    { MT_ROCKET, S_ROCKET, S_ROCKET, S_EXPLODE1, S_EXPLODE3 },
    { MT_PLASMA, S_PLASBALL, S_PLASEXP5, -1,-1 },
    { MT_BFG, S_BFGSHOT, S_BFGLAND6, -1,-1 },
    { MT_EXTRABFG, S_BFGEXP, S_BFGEXP4, -1,-1 },

    { -1, -1,-1, -1, -1 }  // End sentinel
};

static const staterange_t weapon_range[] =
{
	// Weapons...
    { wp_fist, S_PUNCH, S_PUNCH5, -1,-1 },
    { wp_chainsaw, S_SAW, S_SAW3, -1,-1 },
    { wp_pistol, S_PISTOL, S_PISTOLFLASH, S_LIGHTDONE, S_LIGHTDONE },
    { wp_shotgun, S_SGUN, S_SGUNFLASH2, S_LIGHTDONE, S_LIGHTDONE },
    { wp_chaingun, S_CHAIN, S_CHAINFLASH2, S_LIGHTDONE, S_LIGHTDONE },
    { wp_missile, S_MISSILE, S_MISSILEFLASH4, S_LIGHTDONE, S_LIGHTDONE },
    { wp_plasma, S_PLASMA, S_PLASMAFLASH2, S_LIGHTDONE, S_LIGHTDONE },
    { wp_bfg, S_BFG, S_BFGFLASH2, S_LIGHTDONE, S_LIGHTDONE },
    { wp_supershotgun, S_DSGUN, S_DSGUNFLASH2, S_LIGHTDONE, S_LIGHTDONE },

    { -1, -1,-1, -1, -1 }  // End sentinel
};

namespace Frames
{
	void MarkStateUsers(int state)
	{
		if (state == S_NULL)
			return;

		if (state <= S_LAST_WEAPON_STATE)
		{
			for (int w = 0; weapon_range[w].obj_num >= 0; w++)
			{
				const staterange_t *R = weapon_range + w;

				if ((R->start1 <= state && state <= R->end1) ||
				    (R->start2 <= state && state <= R->end2))
				{
					weapon_modified[R->obj_num] = true;
				}
			}

			return;
		}

		// check things.

		for (int t = 0; thing_range[t].obj_num >= 0; t++)
		{
			const staterange_t *R = thing_range + t;

			if ((R->start1 <= state && state <= R->end1) ||
				(R->start2 <= state && state <= R->end2))
			{
				mobj_modified[R->obj_num] = true;
			}
		}
	}
}


//------------------------------------------------------------------------

void Frames::Startup(void)
{
	memset(state_dyn, 0, sizeof(state_dyn));
}

void Frames::ResetAll(void)
{
	for (int i = 0; i < NUMSTATES; i++)
	{
		state_dyn[i].group = state_dyn[i].gr_idx = 0;
	}

	attack_slot[0] = attack_slot[1] = attack_slot[2] = NULL;

	act_flags = 0;

	lowest_touched  = 99999;
	highest_touched = S_NULL;
}

int Frames::BeginGroup(int first, char group)
{
	if (first == S_NULL)
		return 0;

	state_dyn[first].group  = group;
	state_dyn[first].gr_idx = 1;

	return 1;
}

void Frames::SpreadGroups(void)
{
	bool changes;

	do
	{
		changes = false;

		for (int i = 0; i < NUMSTATES; i++)
		{
			if (state_dyn[i].group == 0)
				continue;

			int next = states[i].nextstate;

			if (next == S_NULL)
				continue;

			if (state_dyn[next].group != 0)
				continue;

			state_dyn[next].group  = state_dyn[i].group;
			state_dyn[next].gr_idx = state_dyn[i].gr_idx + 1;
		}
	}
	while (changes);
}

bool Frames::CheckSpawnRemove(int first)
{
	assert(first != S_NULL);

	for (;;)
	{
		if (state_dyn[first].group != 'S')
			break;

		if (states[first].tics < 0)  // hibernation
			break;

		int prev_idx = state_dyn[first].gr_idx;

		first = states[first].nextstate;

		if (first == S_NULL)
			return true;

		int next_idx = state_dyn[first].gr_idx;

		if (next_idx != (prev_idx + 1))
			break;
	}

	return false;
}

namespace Frames
{
	void UpdateAttackSlots(const char *act_name, const char *atk)
	{
		if (! atk)
			return;

		assert(strlen(atk) >= 3);
		assert(atk[1] == ':');

		if (atk[0] == 'S')
		{
			attack_slot[SPARE] = atk + 2;
			return;
		}

		// try to use spare attack if normal attack is already uses, in
		// case some mods give a monster two different attacks.

		assert(atk[0] == 'R' || atk[0] == 'C');

		int N = (atk[0] == 'R') ? RANGE : COMBAT;

		if (attack_slot[N])
		{
			if (strcmp(attack_slot[N], atk + 2) == 0)
				return;  // no problem, already contains this attack

			N = SPARE;

			if (attack_slot[N])
			{
				// XXX should show the problem thing
				// FIXME: maybe disable the action ??
				PrintWarn("No free attack slots for action %s.\n", act_name);
				return;
			}
		}

		attack_slot[N] = atk + 2;
	}
}

void Frames::OutputState(int cur)
{
	assert(cur > 0);

	state_t *st = states + cur;

	if (cur <= S_LAST_WEAPON_STATE)
		act_flags |= AF_WEAPON_ST;
	else
		act_flags |= AF_THING_ST;

	if (cur != S_LIGHTDONE)
	{
		if (cur < lowest_touched)  lowest_touched  = cur;
		if (cur > highest_touched) highest_touched = cur;
	}

	const char *act_name = action_info[st->action].ddf_name;

	WAD::Printf("    %s:%c:%d:%s:%s",
		sprnames[st->sprite], 65 + ((int) st->frame & 63), (int) st->tics,
		(st->frame >= 32768) ? "BRIGHT" : "NORMAL", act_name);

	act_flags |= action_info[st->action].act_flags;

	UpdateAttackSlots(act_name, action_info[st->action].atk_1);
	UpdateAttackSlots(act_name, action_info[st->action].atk_2);
}

const char *Frames::GroupToName(char group, bool use_spawn)
{
	switch (group)
	{
		case 'S': return use_spawn ? "SPAWN" : "IDLE";
		case 'E': return "CHASE";
		case 'L': return "MELEE";
		case 'M': return "MISSILE";
		case 'P': return "PAIN";
		case 'D': return "DEATH";
		case 'X': return "OVERKILL";
		case 'R': return "RESPAWN";
		case 'H': return "RESURRECT";

		// weapons
		case 'u': return "UP";
		case 'd': return "DOWN";
		case 'r': return "READY";
		case 'a': return "ATTACK";
		case 'f': return "FLASH";

		default:
			InternalError("GroupToName: BAD GROUP '%c'\n", group);
	}		

	return NULL;
}

void Frames::OutputGroup(int first, char group)
{
	if (first == S_NULL)
		return;

	assert(state_dyn[first].group != 0);

	bool use_spawn = (group == 'S') && CheckSpawnRemove(first);

	// this allows group sharing (especially MELEE and MISSILE)
	char first_group = state_dyn[first].group;

	WAD::Printf("\n");
	WAD::Printf("STATES(%s) =\n", GroupToName(group, use_spawn));

	int cur = first;

	for (;;)
	{
		OutputState(cur);

		if (states[cur].tics < 0)  // go into hibernation
		{
			WAD::Printf(";\n");
			return;
		}

		int prev_idx = state_dyn[cur].gr_idx;

		cur = states[cur].nextstate;

		if (cur == S_NULL)
		{
			WAD::Printf(",#REMOVE;\n");
			return;
		}

		char next_gr = state_dyn[cur].group;
		int next_idx = state_dyn[cur].gr_idx;

		assert(next_gr != 0);

		// check if finished and DON'T need a redirector
		if (cur == first && (group == 'S' || group == 'E' ||
			group == 'u' || group == 'd' || group == 'r'))
		{
			WAD::Printf(";\n");
			return;
		}

		if (next_gr == first_group && next_idx == (prev_idx + 1))
		{
			WAD::Printf(",\n");
			continue;
		}

		// we must be finished, and we need redirection

		if (next_idx == 1)
			WAD::Printf(",#%s;\n", GroupToName(next_gr, use_spawn));
		else
			WAD::Printf(",#%s:%d;\n", GroupToName(next_gr, use_spawn), next_idx);

		return;
	}
}


//------- DEBUGGING ------------------------------------------------------

#if (DEBUG_RANGES)
static const char *state_names[NUMSTATES] =
{
    "S_NULL",
    "S_LIGHTDONE",
    "S_PUNCH", "S_PUNCHDOWN", "S_PUNCHUP",
    "S_PUNCH1", "S_PUNCH2", "S_PUNCH3", "S_PUNCH4", "S_PUNCH5",
    "S_PISTOL", "S_PISTOLDOWN", "S_PISTOLUP",
    "S_PISTOL1", "S_PISTOL2", "S_PISTOL3", "S_PISTOL4", "S_PISTOLFLASH",
    "S_SGUN", "S_SGUNDOWN", "S_SGUNUP",
    "S_SGUN1", "S_SGUN2", "S_SGUN3", "S_SGUN4", "S_SGUN5", "S_SGUN6",
	"S_SGUN7", "S_SGUN8", "S_SGUN9", "S_SGUNFLASH1", "S_SGUNFLASH2",
    "S_DSGUN", "S_DSGUNDOWN", "S_DSGUNUP",
    "S_DSGUN1", "S_DSGUN2", "S_DSGUN3", "S_DSGUN4", "S_DSGUN5", "S_DSGUN6",
    "S_DSGUN7", "S_DSGUN8", "S_DSGUN9", "S_DSGUN10", "S_DSNR1", "S_DSNR2",
    "S_DSGUNFLASH1", "S_DSGUNFLASH2",
    "S_CHAIN", "S_CHAINDOWN", "S_CHAINUP",
    "S_CHAIN1", "S_CHAIN2", "S_CHAIN3", "S_CHAINFLASH1", "S_CHAINFLASH2",
    "S_MISSILE", "S_MISSILEDOWN", "S_MISSILEUP",
    "S_MISSILE1", "S_MISSILE2", "S_MISSILE3",
    "S_MISSILEFLASH1", "S_MISSILEFLASH2", "S_MISSILEFLASH3", "S_MISSILEFLASH4",
    "S_SAW", "S_SAWB", "S_SAWDOWN", "S_SAWUP",
    "S_SAW1", "S_SAW2", "S_SAW3",
    "S_PLASMA", "S_PLASMADOWN", "S_PLASMAUP",
    "S_PLASMA1", "S_PLASMA2", "S_PLASMAFLASH1", "S_PLASMAFLASH2",
    "S_BFG", "S_BFGDOWN", "S_BFGUP",
    "S_BFG1", "S_BFG2", "S_BFG3", "S_BFG4", "S_BFGFLASH1", "S_BFGFLASH2",
    "S_BLOOD1", "S_BLOOD2", "S_BLOOD3",
    "S_PUFF1", "S_PUFF2", "S_PUFF3", "S_PUFF4",
    "S_TBALL1", "S_TBALL2", "S_TBALLX1", "S_TBALLX2", "S_TBALLX3",
    "S_RBALL1", "S_RBALL2", "S_RBALLX1", "S_RBALLX2", "S_RBALLX3",
    "S_PLASBALL", "S_PLASBALL2",
    "S_PLASEXP", "S_PLASEXP2", "S_PLASEXP3", "S_PLASEXP4", "S_PLASEXP5",
    "S_ROCKET",
    "S_BFGSHOT", "S_BFGSHOT2", "S_BFGLAND", "S_BFGLAND2", "S_BFGLAND3",
    "S_BFGLAND4", "S_BFGLAND5", "S_BFGLAND6", "S_BFGEXP", "S_BFGEXP2",
    "S_BFGEXP3", "S_BFGEXP4",
    "S_EXPLODE1", "S_EXPLODE2", "S_EXPLODE3",
    "S_TFOG", "S_TFOG01", "S_TFOG02", "S_TFOG2", "S_TFOG3", "S_TFOG4",
    "S_TFOG5", "S_TFOG6", "S_TFOG7", "S_TFOG8", "S_TFOG9", "S_TFOG10",
    "S_IFOG", "S_IFOG01", "S_IFOG02", "S_IFOG2", "S_IFOG3", "S_IFOG4", "S_IFOG5",
    "S_PLAY", "S_PLAY_RUN1", "S_PLAY_RUN2", "S_PLAY_RUN3", "S_PLAY_RUN4",
    "S_PLAY_ATK1", "S_PLAY_ATK2", "S_PLAY_PAIN", "S_PLAY_PAIN2",
    "S_PLAY_DIE1", "S_PLAY_DIE2", "S_PLAY_DIE3", "S_PLAY_DIE4", "S_PLAY_DIE5",
    "S_PLAY_DIE6", "S_PLAY_DIE7",
    "S_PLAY_XDIE1", "S_PLAY_XDIE2", "S_PLAY_XDIE3", "S_PLAY_XDIE4",
    "S_PLAY_XDIE5", "S_PLAY_XDIE6", "S_PLAY_XDIE7", "S_PLAY_XDIE8",
    "S_PLAY_XDIE9",
    "S_POSS_STND", "S_POSS_STND2", "S_POSS_RUN1", "S_POSS_RUN2", "S_POSS_RUN3",
    "S_POSS_RUN4", "S_POSS_RUN5", "S_POSS_RUN6", "S_POSS_RUN7", "S_POSS_RUN8",
    "S_POSS_ATK1", "S_POSS_ATK2", "S_POSS_ATK3", "S_POSS_PAIN", "S_POSS_PAIN2",
    "S_POSS_DIE1", "S_POSS_DIE2", "S_POSS_DIE3", "S_POSS_DIE4", "S_POSS_DIE5",
    "S_POSS_XDIE1", "S_POSS_XDIE2", "S_POSS_XDIE3", "S_POSS_XDIE4", "S_POSS_XDIE5",
    "S_POSS_XDIE6", "S_POSS_XDIE7", "S_POSS_XDIE8", "S_POSS_XDIE9", "S_POSS_RAISE1",
    "S_POSS_RAISE2", "S_POSS_RAISE3", "S_POSS_RAISE4",
    "S_SPOS_STND", "S_SPOS_STND2", "S_SPOS_RUN1", "S_SPOS_RUN2", "S_SPOS_RUN3",
    "S_SPOS_RUN4", "S_SPOS_RUN5", "S_SPOS_RUN6", "S_SPOS_RUN7", "S_SPOS_RUN8",
    "S_SPOS_ATK1", "S_SPOS_ATK2", "S_SPOS_ATK3", "S_SPOS_PAIN", "S_SPOS_PAIN2",
    "S_SPOS_DIE1", "S_SPOS_DIE2", "S_SPOS_DIE3", "S_SPOS_DIE4", "S_SPOS_DIE5",
    "S_SPOS_XDIE1", "S_SPOS_XDIE2", "S_SPOS_XDIE3", "S_SPOS_XDIE4", "S_SPOS_XDIE5",
    "S_SPOS_XDIE6", "S_SPOS_XDIE7", "S_SPOS_XDIE8", "S_SPOS_XDIE9", "S_SPOS_RAISE1",
    "S_SPOS_RAISE2", "S_SPOS_RAISE3", "S_SPOS_RAISE4", "S_SPOS_RAISE5",
    "S_VILE_STND", "S_VILE_STND2", "S_VILE_RUN1", "S_VILE_RUN2", "S_VILE_RUN3",
    "S_VILE_RUN4", "S_VILE_RUN5", "S_VILE_RUN6", "S_VILE_RUN7", "S_VILE_RUN8",
    "S_VILE_RUN9", "S_VILE_RUN10", "S_VILE_RUN11", "S_VILE_RUN12", "S_VILE_ATK1",
    "S_VILE_ATK2", "S_VILE_ATK3", "S_VILE_ATK4", "S_VILE_ATK5", "S_VILE_ATK6",
    "S_VILE_ATK7", "S_VILE_ATK8", "S_VILE_ATK9", "S_VILE_ATK10", "S_VILE_ATK11",
    "S_VILE_HEAL1", "S_VILE_HEAL2", "S_VILE_HEAL3", "S_VILE_PAIN", "S_VILE_PAIN2",
    "S_VILE_DIE1", "S_VILE_DIE2", "S_VILE_DIE3", "S_VILE_DIE4", "S_VILE_DIE5",
    "S_VILE_DIE6", "S_VILE_DIE7", "S_VILE_DIE8", "S_VILE_DIE9", "S_VILE_DIE10",
    "S_FIRE1", "S_FIRE2", "S_FIRE3", "S_FIRE4", "S_FIRE5", "S_FIRE6", "S_FIRE7",
    "S_FIRE8", "S_FIRE9", "S_FIRE10", "S_FIRE11", "S_FIRE12", "S_FIRE13", "S_FIRE14",
    "S_FIRE15", "S_FIRE16", "S_FIRE17", "S_FIRE18", "S_FIRE19", "S_FIRE20", "S_FIRE21",
    "S_FIRE22", "S_FIRE23", "S_FIRE24", "S_FIRE25", "S_FIRE26", "S_FIRE27",
    "S_FIRE28", "S_FIRE29", "S_FIRE30",
    "S_SMOKE1", "S_SMOKE2", "S_SMOKE3", "S_SMOKE4", "S_SMOKE5",
    "S_TRACER", "S_TRACER2", "S_TRACEEXP1", "S_TRACEEXP2", "S_TRACEEXP3",
    "S_SKEL_STND", "S_SKEL_STND2", "S_SKEL_RUN1", "S_SKEL_RUN2", "S_SKEL_RUN3",
    "S_SKEL_RUN4", "S_SKEL_RUN5", "S_SKEL_RUN6", "S_SKEL_RUN7", "S_SKEL_RUN8",
    "S_SKEL_RUN9", "S_SKEL_RUN10", "S_SKEL_RUN11", "S_SKEL_RUN12", "S_SKEL_FIST1",
    "S_SKEL_FIST2", "S_SKEL_FIST3", "S_SKEL_FIST4", "S_SKEL_MISS1", "S_SKEL_MISS2",
    "S_SKEL_MISS3", "S_SKEL_MISS4", "S_SKEL_PAIN", "S_SKEL_PAIN2", "S_SKEL_DIE1",
    "S_SKEL_DIE2", "S_SKEL_DIE3", "S_SKEL_DIE4", "S_SKEL_DIE5", "S_SKEL_DIE6",
    "S_SKEL_RAISE1", "S_SKEL_RAISE2", "S_SKEL_RAISE3", "S_SKEL_RAISE4",
    "S_SKEL_RAISE5", "S_SKEL_RAISE6",
    "S_FATSHOT1", "S_FATSHOT2", "S_FATSHOTX1", "S_FATSHOTX2", "S_FATSHOTX3",
    "S_FATT_STND", "S_FATT_STND2", "S_FATT_RUN1", "S_FATT_RUN2", "S_FATT_RUN3",
    "S_FATT_RUN4", "S_FATT_RUN5", "S_FATT_RUN6", "S_FATT_RUN7", "S_FATT_RUN8",
    "S_FATT_RUN9", "S_FATT_RUN10", "S_FATT_RUN11", "S_FATT_RUN12", "S_FATT_ATK1",
    "S_FATT_ATK2", "S_FATT_ATK3", "S_FATT_ATK4", "S_FATT_ATK5", "S_FATT_ATK6",
    "S_FATT_ATK7", "S_FATT_ATK8", "S_FATT_ATK9", "S_FATT_ATK10", "S_FATT_PAIN",
    "S_FATT_PAIN2", "S_FATT_DIE1", "S_FATT_DIE2", "S_FATT_DIE3", "S_FATT_DIE4",
    "S_FATT_DIE5", "S_FATT_DIE6", "S_FATT_DIE7", "S_FATT_DIE8", "S_FATT_DIE9",
    "S_FATT_DIE10", "S_FATT_RAISE1", "S_FATT_RAISE2", "S_FATT_RAISE3", "S_FATT_RAISE4",
    "S_FATT_RAISE5", "S_FATT_RAISE6", "S_FATT_RAISE7", "S_FATT_RAISE8",
    "S_CPOS_STND", "S_CPOS_STND2", "S_CPOS_RUN1", "S_CPOS_RUN2", "S_CPOS_RUN3",
    "S_CPOS_RUN4", "S_CPOS_RUN5", "S_CPOS_RUN6", "S_CPOS_RUN7", "S_CPOS_RUN8",
    "S_CPOS_ATK1", "S_CPOS_ATK2", "S_CPOS_ATK3", "S_CPOS_ATK4", "S_CPOS_PAIN",
    "S_CPOS_PAIN2", "S_CPOS_DIE1", "S_CPOS_DIE2", "S_CPOS_DIE3", "S_CPOS_DIE4",
    "S_CPOS_DIE5", "S_CPOS_DIE6", "S_CPOS_DIE7", "S_CPOS_XDIE1", "S_CPOS_XDIE2",
    "S_CPOS_XDIE3", "S_CPOS_XDIE4", "S_CPOS_XDIE5", "S_CPOS_XDIE6", "S_CPOS_RAISE1",
    "S_CPOS_RAISE2", "S_CPOS_RAISE3", "S_CPOS_RAISE4", "S_CPOS_RAISE5",
    "S_CPOS_RAISE6", "S_CPOS_RAISE7",
    "S_TROO_STND", "S_TROO_STND2", "S_TROO_RUN1", "S_TROO_RUN2", "S_TROO_RUN3",
    "S_TROO_RUN4", "S_TROO_RUN5", "S_TROO_RUN6", "S_TROO_RUN7", "S_TROO_RUN8",
    "S_TROO_ATK1", "S_TROO_ATK2", "S_TROO_ATK3", "S_TROO_PAIN", "S_TROO_PAIN2",
    "S_TROO_DIE1", "S_TROO_DIE2", "S_TROO_DIE3", "S_TROO_DIE4", "S_TROO_DIE5",
    "S_TROO_XDIE1", "S_TROO_XDIE2", "S_TROO_XDIE3", "S_TROO_XDIE4", "S_TROO_XDIE5",
    "S_TROO_XDIE6", "S_TROO_XDIE7", "S_TROO_XDIE8", "S_TROO_RAISE1",
    "S_TROO_RAISE2", "S_TROO_RAISE3", "S_TROO_RAISE4", "S_TROO_RAISE5",
    "S_SARG_STND", "S_SARG_STND2", "S_SARG_RUN1", "S_SARG_RUN2", "S_SARG_RUN3",
    "S_SARG_RUN4", "S_SARG_RUN5", "S_SARG_RUN6", "S_SARG_RUN7", "S_SARG_RUN8",
    "S_SARG_ATK1", "S_SARG_ATK2", "S_SARG_ATK3", "S_SARG_PAIN", "S_SARG_PAIN2",
    "S_SARG_DIE1", "S_SARG_DIE2", "S_SARG_DIE3", "S_SARG_DIE4", "S_SARG_DIE5",
    "S_SARG_DIE6", "S_SARG_RAISE1", "S_SARG_RAISE2", "S_SARG_RAISE3",
    "S_SARG_RAISE4", "S_SARG_RAISE5", "S_SARG_RAISE6",
    "S_HEAD_STND", "S_HEAD_RUN1", "S_HEAD_ATK1", "S_HEAD_ATK2", "S_HEAD_ATK3",
    "S_HEAD_PAIN", "S_HEAD_PAIN2", "S_HEAD_PAIN3", "S_HEAD_DIE1", "S_HEAD_DIE2",
    "S_HEAD_DIE3", "S_HEAD_DIE4", "S_HEAD_DIE5", "S_HEAD_DIE6", "S_HEAD_RAISE1",
    "S_HEAD_RAISE2", "S_HEAD_RAISE3", "S_HEAD_RAISE4", "S_HEAD_RAISE5", "S_HEAD_RAISE6",
    "S_BRBALL1", "S_BRBALL2", "S_BRBALLX1", "S_BRBALLX2", "S_BRBALLX3",
    "S_BOSS_STND", "S_BOSS_STND2", "S_BOSS_RUN1", "S_BOSS_RUN2", "S_BOSS_RUN3",
    "S_BOSS_RUN4", "S_BOSS_RUN5", "S_BOSS_RUN6", "S_BOSS_RUN7", "S_BOSS_RUN8",
    "S_BOSS_ATK1", "S_BOSS_ATK2", "S_BOSS_ATK3", "S_BOSS_PAIN", "S_BOSS_PAIN2",
    "S_BOSS_DIE1", "S_BOSS_DIE2", "S_BOSS_DIE3", "S_BOSS_DIE4", "S_BOSS_DIE5",
    "S_BOSS_DIE6", "S_BOSS_DIE7", "S_BOSS_RAISE1", "S_BOSS_RAISE2", "S_BOSS_RAISE3",
    "S_BOSS_RAISE4", "S_BOSS_RAISE5", "S_BOSS_RAISE6", "S_BOSS_RAISE7",
    "S_BOS2_STND", "S_BOS2_STND2", "S_BOS2_RUN1", "S_BOS2_RUN2", "S_BOS2_RUN3",
    "S_BOS2_RUN4", "S_BOS2_RUN5", "S_BOS2_RUN6", "S_BOS2_RUN7", "S_BOS2_RUN8",
    "S_BOS2_ATK1", "S_BOS2_ATK2", "S_BOS2_ATK3", "S_BOS2_PAIN", "S_BOS2_PAIN2",
    "S_BOS2_DIE1", "S_BOS2_DIE2", "S_BOS2_DIE3", "S_BOS2_DIE4", "S_BOS2_DIE5",
    "S_BOS2_DIE6", "S_BOS2_DIE7", "S_BOS2_RAISE1", "S_BOS2_RAISE2", "S_BOS2_RAISE3",
    "S_BOS2_RAISE4", "S_BOS2_RAISE5", "S_BOS2_RAISE6", "S_BOS2_RAISE7",
    "S_SKULL_STND", "S_SKULL_STND2", "S_SKULL_RUN1", "S_SKULL_RUN2",
    "S_SKULL_ATK1", "S_SKULL_ATK2", "S_SKULL_ATK3", "S_SKULL_ATK4",
    "S_SKULL_PAIN", "S_SKULL_PAIN2", "S_SKULL_DIE1", "S_SKULL_DIE2",
    "S_SKULL_DIE3", "S_SKULL_DIE4", "S_SKULL_DIE5", "S_SKULL_DIE6",
    "S_SPID_STND", "S_SPID_STND2", "S_SPID_RUN1", "S_SPID_RUN2", "S_SPID_RUN3",
    "S_SPID_RUN4", "S_SPID_RUN5", "S_SPID_RUN6", "S_SPID_RUN7", "S_SPID_RUN8",
    "S_SPID_RUN9", "S_SPID_RUN10", "S_SPID_RUN11", "S_SPID_RUN12",
    "S_SPID_ATK1", "S_SPID_ATK2", "S_SPID_ATK3", "S_SPID_ATK4",
    "S_SPID_PAIN", "S_SPID_PAIN2", "S_SPID_DIE1", "S_SPID_DIE2", "S_SPID_DIE3",
    "S_SPID_DIE4", "S_SPID_DIE5", "S_SPID_DIE6", "S_SPID_DIE7", "S_SPID_DIE8",
    "S_SPID_DIE9", "S_SPID_DIE10", "S_SPID_DIE11",
    "S_BSPI_STND", "S_BSPI_STND2", "S_BSPI_SIGHT", "S_BSPI_RUN1", "S_BSPI_RUN2",
    "S_BSPI_RUN3", "S_BSPI_RUN4", "S_BSPI_RUN5", "S_BSPI_RUN6", "S_BSPI_RUN7",
    "S_BSPI_RUN8", "S_BSPI_RUN9", "S_BSPI_RUN10", "S_BSPI_RUN11", "S_BSPI_RUN12",
    "S_BSPI_ATK1", "S_BSPI_ATK2", "S_BSPI_ATK3", "S_BSPI_ATK4", "S_BSPI_PAIN",
    "S_BSPI_PAIN2", "S_BSPI_DIE1", "S_BSPI_DIE2", "S_BSPI_DIE3", "S_BSPI_DIE4",
    "S_BSPI_DIE5", "S_BSPI_DIE6", "S_BSPI_DIE7", "S_BSPI_RAISE1", "S_BSPI_RAISE2",
    "S_BSPI_RAISE3", "S_BSPI_RAISE4", "S_BSPI_RAISE5", "S_BSPI_RAISE6", "S_BSPI_RAISE7",
    "S_ARACH_PLAZ", "S_ARACH_PLAZ2", "S_ARACH_PLEX", "S_ARACH_PLEX2", "S_ARACH_PLEX3",
    "S_ARACH_PLEX4", "S_ARACH_PLEX5",
    "S_CYBER_STND", "S_CYBER_STND2", "S_CYBER_RUN1", "S_CYBER_RUN2", "S_CYBER_RUN3",
    "S_CYBER_RUN4", "S_CYBER_RUN5", "S_CYBER_RUN6", "S_CYBER_RUN7", "S_CYBER_RUN8",
    "S_CYBER_ATK1", "S_CYBER_ATK2", "S_CYBER_ATK3", "S_CYBER_ATK4", "S_CYBER_ATK5",
    "S_CYBER_ATK6", "S_CYBER_PAIN", "S_CYBER_DIE1", "S_CYBER_DIE2", "S_CYBER_DIE3",
    "S_CYBER_DIE4", "S_CYBER_DIE5", "S_CYBER_DIE6", "S_CYBER_DIE7", "S_CYBER_DIE8",
    "S_CYBER_DIE9", "S_CYBER_DIE10",
	"S_PAIN_STND",
    "S_PAIN_RUN1", "S_PAIN_RUN2", "S_PAIN_RUN3", "S_PAIN_RUN4", "S_PAIN_RUN5",
    "S_PAIN_RUN6", "S_PAIN_ATK1", "S_PAIN_ATK2", "S_PAIN_ATK3", "S_PAIN_ATK4",
    "S_PAIN_PAIN", "S_PAIN_PAIN2", "S_PAIN_DIE1", "S_PAIN_DIE2", "S_PAIN_DIE3",
    "S_PAIN_DIE4", "S_PAIN_DIE5", "S_PAIN_DIE6", "S_PAIN_RAISE1", "S_PAIN_RAISE2",
    "S_PAIN_RAISE3", "S_PAIN_RAISE4", "S_PAIN_RAISE5", "S_PAIN_RAISE6",
    "S_SSWV_STND", "S_SSWV_STND2", "S_SSWV_RUN1", "S_SSWV_RUN2", "S_SSWV_RUN3",
    "S_SSWV_RUN4", "S_SSWV_RUN5", "S_SSWV_RUN6", "S_SSWV_RUN7", "S_SSWV_RUN8",
    "S_SSWV_ATK1", "S_SSWV_ATK2", "S_SSWV_ATK3", "S_SSWV_ATK4", "S_SSWV_ATK5",
    "S_SSWV_ATK6", "S_SSWV_PAIN", "S_SSWV_PAIN2", "S_SSWV_DIE1", "S_SSWV_DIE2",
    "S_SSWV_DIE3", "S_SSWV_DIE4", "S_SSWV_DIE5", "S_SSWV_XDIE1", "S_SSWV_XDIE2",
    "S_SSWV_XDIE3", "S_SSWV_XDIE4", "S_SSWV_XDIE5", "S_SSWV_XDIE6", "S_SSWV_XDIE7",
    "S_SSWV_XDIE8", "S_SSWV_XDIE9", "S_SSWV_RAISE1", "S_SSWV_RAISE2", "S_SSWV_RAISE3",
    "S_SSWV_RAISE4", "S_SSWV_RAISE5",
    "S_KEENSTND", "S_COMMKEEN", "S_COMMKEEN2", "S_COMMKEEN3", "S_COMMKEEN4",
    "S_COMMKEEN5", "S_COMMKEEN6", "S_COMMKEEN7", "S_COMMKEEN8", "S_COMMKEEN9",
    "S_COMMKEEN10", "S_COMMKEEN11", "S_COMMKEEN12", "S_KEENPAIN", "S_KEENPAIN2",
    "S_BRAIN", "S_BRAIN_PAIN", "S_BRAIN_DIE1", "S_BRAIN_DIE2", "S_BRAIN_DIE3",
    "S_BRAIN_DIE4", "S_BRAINEYE", "S_BRAINEYESEE", "S_BRAINEYE1",
    "S_SPAWN1", "S_SPAWN2", "S_SPAWN3", "S_SPAWN4",
    "S_SPAWNFIRE1", "S_SPAWNFIRE2", "S_SPAWNFIRE3", "S_SPAWNFIRE4",
    "S_SPAWNFIRE5", "S_SPAWNFIRE6", "S_SPAWNFIRE7", "S_SPAWNFIRE8",
    "S_BRAINEXPLODE1", "S_BRAINEXPLODE2", "S_BRAINEXPLODE3",
    "S_ARM1", "S_ARM1A", "S_ARM2", "S_ARM2A",
    "S_BAR1", "S_BAR2", "S_BEXP", "S_BEXP2", "S_BEXP3",
    "S_BEXP4", "S_BEXP5", "S_BBAR1", "S_BBAR2", "S_BBAR3",
    "S_BON1", "S_BON1A", "S_BON1B", "S_BON1C", "S_BON1D", "S_BON1E",
    "S_BON2", "S_BON2A", "S_BON2B", "S_BON2C", "S_BON2D", "S_BON2E",
    "S_BKEY", "S_BKEY2", "S_RKEY", "S_RKEY2", "S_YKEY", "S_YKEY2",
    "S_BSKULL", "S_BSKULL2", "S_RSKULL", "S_RSKULL2", "S_YSKULL", "S_YSKULL2",
    "S_STIM", "S_MEDI",
    "S_SOUL", "S_SOUL2", "S_SOUL3", "S_SOUL4", "S_SOUL5", "S_SOUL6",
    "S_PINV", "S_PINV2", "S_PINV3", "S_PINV4",
    "S_PSTR",
    "S_PINS", "S_PINS2", "S_PINS3", "S_PINS4",
    "S_MEGA", "S_MEGA2", "S_MEGA3", "S_MEGA4",
    "S_SUIT",
    "S_PMAP", "S_PMAP2", "S_PMAP3", "S_PMAP4", "S_PMAP5", "S_PMAP6",
    "S_PVIS", "S_PVIS2",
    "S_CLIP", "S_AMMO", "S_ROCK", "S_BROK",
    "S_CELL", "S_CELP", "S_SHEL", "S_SBOX",
    "S_BPAK",
    "S_BFUG", "S_MGUN", "S_CSAW", "S_LAUN", "S_PLAS", "S_SHOT", "S_SHOT2",
    "S_COLU", "S_STALAG",
    "S_BLOODYTWITCH", "S_BLOODYTWITCH2",
    "S_BLOODYTWITCH3", "S_BLOODYTWITCH4",
    "S_DEADTORSO", "S_DEADBOTTOM",
    "S_HEADSONSTICK",
    "S_GIBS",
    "S_HEADONASTICK", "S_HEADCANDLES", "S_HEADCANDLES2", "S_DEADSTICK",
    "S_LIVESTICK", "S_LIVESTICK2",
    "S_MEAT2", "S_MEAT3", "S_MEAT4", "S_MEAT5",
    "S_STALAGTITE",
    "S_TALLGRNCOL", "S_SHRTGRNCOL", "S_TALLREDCOL", "S_SHRTREDCOL",
    "S_CANDLESTIK", "S_CANDELABRA", "S_SKULLCOL",
    "S_TORCHTREE", "S_BIGTREE", "S_TECHPILLAR",
    "S_EVILEYE", "S_EVILEYE2", "S_EVILEYE3", "S_EVILEYE4",
    "S_FLOATSKULL", "S_FLOATSKULL2", "S_FLOATSKULL3",
    "S_HEARTCOL", "S_HEARTCOL2",
    "S_BLUETORCH", "S_BLUETORCH2", "S_BLUETORCH3", "S_BLUETORCH4",
    "S_GREENTORCH", "S_GREENTORCH2", "S_GREENTORCH3", "S_GREENTORCH4",
    "S_REDTORCH", "S_REDTORCH2", "S_REDTORCH3", "S_REDTORCH4",
    "S_BTORCHSHRT", "S_BTORCHSHRT2", "S_BTORCHSHRT3", "S_BTORCHSHRT4",
    "S_GTORCHSHRT", "S_GTORCHSHRT2", "S_GTORCHSHRT3", "S_GTORCHSHRT4",
    "S_RTORCHSHRT", "S_RTORCHSHRT2", "S_RTORCHSHRT3", "S_RTORCHSHRT4",
    "S_HANGNOGUTS", "S_HANGBNOBRAIN", "S_HANGTLOOKDN",
    "S_HANGTSKULL", "S_HANGTLOOKUP", "S_HANGTNOBRAIN",
    "S_COLONGIBS", "S_SMALLPOOL", "S_BRAINSTEM",
    "S_TECHLAMP", "S_TECHLAMP2", "S_TECHLAMP3", "S_TECHLAMP4",
    "S_TECH2LAMP", "S_TECH2LAMP2", "S_TECH2LAMP3", "S_TECH2LAMP4"
};
#endif

void Frames::DebugRange(const char *kind, const char *entry)
{
#if (DEBUG_RANGES)
	fprintf(stderr, "%s [%s] : ", kind, entry);
	
	if (highest_touched == S_NULL)
	{
		fprintf(stderr, "empty\n");
	}
	else
	{
		assert(0 < lowest_touched);
		assert(lowest_touched <= highest_touched);
		assert(highest_touched < NUMSTATES);

		fprintf(stderr, "%s -> %s\n", state_names[lowest_touched],
			state_names[highest_touched]);
	}
#endif
}
