//----------------------------------------------------------------------------
//  EDGE Weapon (player sprites) Action Code
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

#ifndef __P_PSPR__
#define __P_PSPR__

// Basic data types.
// Needs fixed point, and BAM angles.
#include "lu_math.h"
#include "m_fixed.h"

#include "ddf_main.h"

// maximum weapons player can hold at once
#define MAXWEAPONS  32

//
// Overlay psprites are scaled shapes
// drawn directly on the view screen,
// coordinates are given for a 320*200 view screen.
//
typedef enum
{
	ps_weapon = 0,
	ps_flash,
	ps_crosshair,
	ps_NOT_USED,

	// -AJA- Savegame code relies on NUMPSPRITES == 4.
	NUMPSPRITES
}
psprnum_t;

typedef struct
{
	// current state.  NULL state means not active
	const state_t *state;

	// state to enter next.
	const state_t *next_state;

	// time (in tics) remaining for current state
	int tics;

	// screen position values
	float sx;
	float sy;

	// translucency values
	float visibility;
	float vis_target;
}
pspdef_t;

typedef enum
{
	PLWEP_Removing  = 0x0001,  // weapon is being removed (or upgraded)
}
playerweapon_flags_e;

//
// Per-player Weapon Info.
// 
typedef struct
{
	weapondef_c *info;

	// player has this weapon.
	bool owned;

	// various flags
	int flags;

	// current clip sizes
	int clip_size[2];
}
playerweapon_t;

#endif
