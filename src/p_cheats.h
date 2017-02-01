//----------------------------------------------------------------------------
//  EDGE2 Cheating and Debugging Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2017  The EDGE2 Team.
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
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------
//
// This code does not handle the user front-end for entering cheats. This
// includes the old way in-game (iddqd, idkfa, etc), through the console
// (god, noclip, etc), and soon will also include through netcmds
// (demos, netgames, etc). Note that these interfaces will be responsible
// for invoking these commands - this file just contains code to actually
// carry them out.
//
//


#ifndef __P_CHEATS__
#define __P_CHEATS__

static void CheatGiveWeapons(player_t *pl, int key = -2);

#endif  // __P_CHEATS__