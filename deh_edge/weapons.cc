//----------------------------------------------------------------------------
//  WEAPON stuff
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
#include "weapons.h"

#include "info.h"
#include "frames.h"
#include "mobj.h"
#include "sounds.h"
#include "system.h"
#include "wad.h"


#define DUMP_ALL  1


int Ammo::plr_max[4] = 
{ 
	200, 50, 300, 50  // doubled for backpack
};

int Ammo::pickups[4] =
{
	10, 4, 20, 1   // multiplied by 5 for boxes
};


//----------------------------------------------------------------------------

void Weapons::BeginLump(void)
{
	WAD::NewLump("DDFWEAP");

	WAD::Printf(GEN_BY_COMMENT);

	WAD::Printf("<WEAPONS>\n\n");
}

void Weapons::FinishLump(void)
{
	WAD::FinishLump();
}

void Weapons::ConvertAll(void)
{
	int i;

	bool got_one = false;

	for (i = 0; i < NUMMOBJTYPES; i++)
	{
#ifndef DUMP_ALL
	    if (! weapon_info[i].modified)
			continue;
#endif

		if (! got_one)
			BeginLump();
		
		got_one = true;

//!!!!		ConvertWeapon(weapon_info + i, 0);
	}
		
	if (got_one)
		FinishLump();
}

// NOTES
//
//   Need to duplicate players
//   info->damage only used in attacks and LOST_SOUL
//   random sound groups: PODTH1/2/3, POSIT1/2/3, BGDTH1/2, BGSIT1/2.
