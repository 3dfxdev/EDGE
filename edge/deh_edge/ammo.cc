//------------------------------------------------------------------------
//  AMMO Handling
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
#include "ammo.h"

#include "info.h"
#include "mobj.h"
#include "system.h"
#include "things.h"


int Ammo::plr_max[4] = 
{ 
	200, 50, 300, 50  // doubled for backpack
};

int Ammo::pickups[4] =
{
	10, 4, 20, 1   // multiplied by 5 for boxes
};

bool ammo_modified[4];


//----------------------------------------------------------------------------

void Ammo::Startup(void)
{
	memset(ammo_modified, 0, sizeof(ammo_modified));
}

namespace Ammo
{
	void MarkAmmo(int a_num)
	{
		assert (0 <= a_num && a_num < NUMAMMO);

		ammo_modified[a_num] = true;
	}

	void AmmoDependencies(void)
	{
		bool any = ammo_modified[0] || ammo_modified[1] ||
		           ammo_modified[2] || ammo_modified[3];

		if (any)
		{
			Things::MarkThing(MT_PLAYER);
			Things::MarkThing(MT_MISC24);  // backpack
		}

		if (ammo_modified[am_bullet])
		{
			Things::MarkThing(MT_CLIP);    // "CLIP"
			Things::MarkThing(MT_MISC17);  // "BOX_OF_BULLETS"  
		}

		if (ammo_modified[am_shell])
		{
			Things::MarkThing(MT_MISC22);  // "SHELLS"  
			Things::MarkThing(MT_MISC23);  // "BOX_OF_SHELLS"  
		}

		if (ammo_modified[am_rocket])
		{
			Things::MarkThing(MT_MISC18);  // "ROCKET"  
			Things::MarkThing(MT_MISC19);  // "BOX_OF_ROCKETS"  
		}

		if (ammo_modified[am_cell])
		{
			Things::MarkThing(MT_MISC20);  // "CELLS"  
			Things::MarkThing(MT_MISC21);  // "CELL_PACK"  
		}
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
}

