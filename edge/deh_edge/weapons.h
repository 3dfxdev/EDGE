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
#ifndef __WEAPONS__
#define __WEAPONS__

#include "i_defs.h"
#include "info.h"

namespace Ammo
{
	typedef enum
	{
		BULLETS, SHELLS, CELLS, ROCKETS,
		NUMAMMO 
	}
	ammotype_e;

	extern int plr_max[4];
	extern int pickups[4];
}

namespace Weapons
{
	void BeginLump(void);
	void FinishLump(void);

	void ConvertAll(void);
}


#endif /* __WEAPONS__ */
