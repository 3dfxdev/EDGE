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
//  Based on DeHackEd 2.3 source, by Greg Lewis.
//  Based on Linux DOOM Hack Editor 0.8a, by Sam Lantinga.
//  Based on PrBoom's DEH/BEX code, by Ty Halderman, TeamTNT.
//

#include "i_defs.h"
#include "misc.h"

#include "info.h"
#include "frames.h"
#include "mobj.h"
#include "sounds.h"
#include "system.h"
#include "wad.h"


int Misc::init_ammo   = 50;
int Misc::init_health = 0;

int Misc::max_armour  = 200;
int Misc::max_health  = 200;

int Misc::green_armour_class = 1;
int Misc::blue_armour_class  = 2;
int Misc::bfg_cells_per_shot = 40;

int Misc::soul_health  = 200;
int Misc::soul_limit   = 200;
int Misc::mega_health  = 200;

bool Misc::monster_infight   = false;

