//----------------------------------------------------------------------------
//  EDGE Thinker & Ticker Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2001  The EDGE Team.
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
// -ACB- 1998/09/14 Removed P_AllocateThinker: UNUSED
//
// -AJA- 1999/11/06: Changed from ring (thinkercap) to list.
//
// -ES- 2000/02/14: Removed thinker system.

#include "i_defs.h"
#include "p_tick.h"

#include "dm_state.h"
#include "p_local.h"
#include "z_zone.h"

int leveltime;

//
// P_Ticker
//
void P_Ticker(void)
{
  player_t *p;

  if (paused)
    return;

  // pause if in menu and at least one tic has been run
  if (!netgame && menuactive && !demoplayback && 
      consoleplayer->viewz != M_PI)
  {
    return;
  }

  for (p = players; p; p = p->next)
    P_PlayerThink(p);

  P_RunMobjThinkers();
  P_RunLights();
  P_RunActiveSectors();
  P_RunSectorSFX();
  P_UpdateSpecials();
  P_MobjItemRespawn();

  // for par times
  leveltime++;
}
