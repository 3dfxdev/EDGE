//----------------------------------------------------------------------------
// EDGE Tic Command Definition
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2007  The EDGE Team.
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


// The data sampled per tick (single player)
// and transmitted to other peers (multiplayer).
// Mainly movements/button commands per game tick,
// plus a checksum for internal state consistency.

typedef struct
{
	// horizontal turning, *65536 for angle delta
	s16_t angleturn;

	// vertical angle for mlook, *65536 for angle delta
	s16_t mlookturn;

	// checks for net game
	s16_t consistency;

	// active player number, -1 for "dropped out" player
	s16_t player_idx;


	// /32 for move
	s8_t forwardmove;

	// /32 for move
	s8_t sidemove;

	// -MH- 1998/08/23 upward movement
	s8_t upwardmove;

	byte buttons, extbuttons;

	byte chatchar;

	byte unused2, unused3;

	void ByteSwap();
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
}
buttoncode_e;

// special weapon numbers
#define BT_NEXT_WEAPON  14
#define BT_PREV_WEAPON  15


//
// Extended Buttons: EDGE Specfics
// -ACB- 1998/07/03
//
typedef enum
{
///---	EBT_JUMP = 1,

	EBT_CENTER = 4,

	// -AJA- 2000/02/08: support for second attack.
	EBT_SECONDATK = 8,

	// -AJA- 2000/03/18: more control over zooming
	EBT_ZOOM = 16,

	// -AJA- 2004/11/10: manual weapon reload
	EBT_RELOAD = 32
}
extbuttoncode_e;

#endif // __E_TICCMD_H__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
