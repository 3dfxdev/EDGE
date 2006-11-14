//----------------------------------------------------------------------------
//  EDGE Level Loading/Setup Code
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

#ifndef __P_SETUP__
#define __P_SETUP__

#include "dm_defs.h"

#include "epi/math_crc.h"

extern epi::crc32_c mapsector_CRC;
extern epi::crc32_c mapline_CRC;
extern epi::crc32_c mapthing_CRC;

extern int mapthing_NUM;

// Called by startup code.
void P_Init(void);

// -KM- 1998/11/25 Added autotag.  Linedefs with this tag are automatically
//   triggered.
void P_SetupLevel(skill_t skill, int autotag);

// Stop a level playing (but not killing it). At the 
// moment it just stops sector effects looping. -ACB- 2005/09/08
void P_StopLevel(void);

// Needed by savegame code.
void P_RemoveMobjs(void);
void P_RemoveItemsInQue(void);

#endif // __P_SETUP__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
