//----------------------------------------------------------------------------
//  MISCELLANY
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
#ifndef __MISC__
#define __MISC__

#include "i_defs.h"
#include "info.h"

namespace Misc
{
	extern int init_ammo;
	extern int init_health;

	extern int max_armour;
	extern int max_health;

	extern int green_armour_class;
	extern int blue_armour_class;
	extern int bfg_cells_per_shot;

	extern int soul_health;
	extern int soul_limit;
	extern int mega_health;  // and limit

	extern bool monster_infight;

	// NOTE: we don't support the amounts given by cheats
	//       (God Mode Health, IDKFA Armor, etc).
}


#endif /* __MISC__ */
