//----------------------------------------------------------------------------
// EDGE Tic Command Definition
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

#ifndef __E_TICCMD_H__
#define __E_TICCMD_H__

#include "dm_type.h"

// The data sampled per tick (single player)
// and transmitted to other peers (multiplayer).
// Mainly movements/button commands per game tick,
// plus a checksum for internal state consistency.

typedef struct
{
  // vertical angle for mlook, /256 for slope
  signed char vertangle;

  // -MH- 1998/08/23 upward movement
  signed char upwardmove;

  // /32 for move
  signed char forwardmove;

  // /32 for move
  signed char sidemove;

  // *65536 for angle delta
  short angleturn;

  // checks for net game
  short consistency;

  byte chatchar;
  byte buttons;
  byte extbuttons;
}
ticcmd_t;

#endif
