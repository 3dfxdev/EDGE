//----------------------------------------------------------------------------
//  ATTACK conversion
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
//  Based on DeHackEd 2.3 source, by Greg Lewis.
//  Based on Linux DOOM Hack Editor 0.8a, by Sam Lantinga.
//  Based on PrBoom's DEH/BEX code, by Ty Halderman, TeamTNT.
//

#include "i_defs.h"
#include "attacks.h"

#include "info.h"
#include "frames.h"
#include "misc.h"
#include "mobj.h"
#include "sounds.h"
#include "system.h"
#include "things.h"
#include "wad.h"


#define DUMP_ALL  1


void Attacks::BeginLump(void)
{
	WAD::NewLump("DDFATK");

	WAD::Printf(GEN_BY_COMMENT);

	WAD::Printf("<ATTACKS>\n\n");
}

void Attacks::FinishLump(void)
{
	WAD::FinishLump();
}

namespace Attacks
{
#if 0
	void HandleSounds(mobjinfo_t *info, int mt_num)
	{
		if (info->activesound != sfx_None)
		{
			if (info->flags & MF_PICKUP)
				WAD::Printf("PICKUP_SOUND = %s;\n", GetSound(info->activesound));
			else
				WAD::Printf("ACTIVE_SOUND = %s;\n", GetSound(info->activesound));
		}

		if (info->attacksound != sfx_None)
		{
			// FIXME: may be associated with attack instead
			WAD::Printf("STARTCOMBAT_SOUND = %s;\n", GetSound(info->attacksound));
		}

		if (info->deathsound != sfx_None)
			WAD::Printf("DEATH_SOUND = %s;\n", GetSound(info->deathsound));

		if (info->painsound != sfx_None)
			WAD::Printf("PAIN_SOUND = %s;\n", GetSound(info->painsound));

		if (info->seesound != sfx_None)
			WAD::Printf("SIGHTING_SOUND = %s;\n", GetSound(info->seesound));
		else if (mt_num == MT_BOSSSPIT)
			WAD::Printf("SIGHTING_SOUND = %s;\n", GetSound(sfx_bossit));
	}
#endif

#if 0
	void HandleFrames(mobjinfo_t *info, int mt_num)
	{
		// FIXME: archvile RESURRECT states

		Frames::ResetAll();

		// special cases...

		// --- collect states into groups ---

		int count = 0;

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
	}
#endif

	void ConvertAttack(mobjinfo_t *info, bool player_rocket);
}

void Attacks::ConvertAttack(mobjinfo_t *info, bool player_rocket)
{
	int mt_num = info - mobjinfo;

	if (info->name[0] != '*' || info->name[1] == 0)  // thing
		return;

	if (player_rocket)
		WAD::Printf("[%s]\n", "PLAYER_MISSILE");
	else
		WAD::Printf("[%s]\n", info->name + 1);

	WAD::Printf("RADIUS = %1.1f;\n", F_FIXED(info->radius));
	WAD::Printf("HEIGHT = %1.1f;\n", F_FIXED(info->height));

	if (info->spawnhealth != 1000)
		WAD::Printf("SPAWNHEALTH = %d;\n", info->spawnhealth);

	// interestingly, speed is fixed point for attacks, plain int for things
	// FIXME: verify the above statement
	if (info->speed != 0)
		WAD::Printf("SPEED = %1.2f;\n", F_FIXED(info->speed));

	if (info->mass != 100)
		WAD::Printf("MASS = %d;\n", info->mass);

	Things::HandleFlags(info, mt_num, 0);

// FIXME !!!	HandleSounds(info, mt_num);
// FIXME !!!!!	HandleFrames(info, mt_num);

	WAD::Printf("\n");
}

void Attacks::ConvertAll(void)
{
	int i;

	bool got_one = false;

	for (i = 0; i < NUMMOBJTYPES; i++)
	{
#ifndef DUMP_ALL
	    if (! attack_dyn[i].modified)
			continue;
#endif

		if (! got_one)
			BeginLump();
		
		got_one = true;

		ConvertAttack(mobjinfo + i, false);

		if (i == MT_ROCKET)
			ConvertAttack(mobjinfo + i, true);
	}
		
	if (got_one)
		FinishLump();
}

// NOTES
//
//   Handle PLAYER_MISSILE == CYBERDEMON_MISSILE
//   BRAIN_CUBE is a merger between MT_SPAWNSHOT and MT_SPAWNFIRE
