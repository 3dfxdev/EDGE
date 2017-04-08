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

bool CheckCheats(bool silent = false);

void Cheat_ToggleGodMode();
void Cheat_NoClip();
void Cheat_IDFA();
void Cheat_IDKFA();
void Cheat_Choppers();
void Cheat_Unlock();
void Cheat_HOM();
void Cheat_Loaded();
void Cheat_Suicide();
void Cheat_ShowPos();
void Cheat_KillAll();
void Cheat_Info();

static void CheatGiveWeapons(player_t *pl, int key = -2)
{
	epi::array_iterator_c it;
	
	for (it = weapondefs.GetIterator(0); it.IsValid(); it++)
	{
		weapondef_c *info = ITERATOR_TO_TYPE(it, weapondef_c*);

		if (info && !info->no_cheat && (key<0 || info->bind_key==key))
		{
			P_AddWeapon(pl, info, NULL);
		}
	}

	if (key < 0)
	{
		for (int slot=0; slot < MAXWEAPONS; slot++)
		{
			if (pl->weapons[slot].info)
				P_FillWeapon(pl, slot);
		}
	}

	P_UpdateAvailWeapons(pl);
}


#endif  // __P_CHEATS__