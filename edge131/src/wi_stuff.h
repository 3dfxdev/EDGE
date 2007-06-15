//----------------------------------------------------------------------------
//  EDGE Intermission Screen Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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

#ifndef __WI_STUFF__
#define __WI_STUFF__

#include "dm_defs.h"
#include "e_player.h"

// States for the intermission

typedef enum
{
	NoState = -1,
	StatCount,
	ShowNextLoc
}
stateenum_t;

// Called by main loop, animate the intermission.
void WI_Ticker(void);

// Called by main loop,
// draws the intermission directly into the screen buffer.
void WI_Drawer(void);

// Setup for an intermission screen.
void WI_Start(wbstartstruct_t * wbstartstruct);

// Clear Intermission Data
void WI_Clear(void);

bool WI_CheckForAccelerate(void);

#endif // __WI_STUFF__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
