//----------------------------------------------------------------------------
// EDGE Tic Command Definition
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
	// horizontal turning, *65536 for angle delta
	short angleturn;

	// vertical angle for mlook, *65536 for angle delta
	short mlookturn;

	// checks for net game
	short consistency;

	short unused1;

	// /32 for move
	signed char forwardmove;

	// /32 for move
	signed char sidemove;

	// -MH- 1998/08/23 upward movement
	signed char upwardmove;

	byte buttons;
	byte extbuttons;

	byte chatchar;
	byte special[2];  // currently unused
}
ticcmd_t;

//
// Button/action code definitions.
//
typedef enum
{
	// Press "Fire".
	BT_ATTACK = 1,

	// Use button, to open doors, activate switches.
	BT_USE = 2,

	// Flag, weapon change pending.
	// If true, the next 4 bits hold weapon num.
	BT_CHANGE = 4,

	// The 3bit weapon mask and shift, convenience.
	BT_WEAPONMASK = (8 + 16 + 32 + 64),
	BT_WEAPONSHIFT = 3,

	// This flag will always be set for active players.
	// An empty (all-zero) ticcmd_t from the server or demo file 
	// indicates the player dropped out / end of demo.
	BT_IN_GAME = 0x80
}
buttoncode_e;

//
// Extended Buttons: EDGE Specfics
// -ACB- 1998/07/03
//
typedef enum
{
	EBT_JUMP = 1,
	EBT_CENTER = 4,

	// -AJA- 2000/02/08: support for second attack.
	EBT_SECONDATK = 8,

	// -AJA- 2000/03/18: more control over zooming
	EBT_ZOOM = 16,

	// -AJA- 2004/11/10: manual weapon reload
	EBT_RELOAD = 32
}
extbuttoncode_e;

#endif
