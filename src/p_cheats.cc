
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

#include "i_defs.h"
#include "m_cheatcodes.h"

#include "con_main.h"
#include "../ddf/main.h"
#include "dstrings.h"
#include "g_game.h"
#include "m_menu.h"
#include "s_sound.h"
#include "s_music.h"
#include "p_local.h"
#include "p_mobj.h"
#include "p_bot.h"
#include "w_wad.h"
#include "p_cheats.h"

extern cvar_c debug_fps;
extern cvar_c debug_pos;


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
