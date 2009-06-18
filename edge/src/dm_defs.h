//----------------------------------------------------------------------------
//  EDGE Basic Definitions File
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2008  The EDGE Team.
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

#ifndef __DEFINITIONS__
#define __DEFINITIONS__


//
// Global parameters/defines.
//

// The current state of the game: whether we are
// playing, gazing at the intermission screen,
// the game final animation, or a demo. 
typedef enum
{
	GS_NOTHING = 0,
	GS_TITLESCREEN,
	GS_LEVEL,
	GS_INTERMISSION,
	GS_FINALE,
}
gamestate_e;

//
// Difficulty/skill settings/filters.
//

// Skill flags.
#define MTF_EASY         1
#define MTF_NORMAL       2
#define MTF_HARD         4

// Deaf monsters/do not react to sound.
#define MTF_AMBUSH       8

// Multiplayer only.
#define MTF_NOT_SINGLE  16

// -AJA- 1999/09/22: Boom compatibility.
#define MTF_NOT_DM      32
#define MTF_NOT_COOP    64

// -AJA- 2000/07/31: Friend flag, from MBF
#define MTF_FRIEND      128 

// -AJA- 2004/11/04: This bit should be zero (otherwise old WAD).
#define MTF_RESERVED    256

// -AJA- 2008/03/08: Extrafloor placement
#define MTF_EXFLOOR_MASK    0x3C00
#define MTF_EXFLOOR_SHIFT   10

typedef enum
{
	sk_invalid   = -1,
	sk_baby      = 0,
	sk_easy      = 1,
	sk_medium    = 2,
	sk_hard      = 3,
	sk_nightmare = 4,
	sk_numtypes  = 5
}
skill_t;


#define  VISIBLE (1.0f)
#define  VISSTEP (1.0f/256.0f)
#define  INVISIBLE (0.0f)


#include "e_keys.h"  //!!! FIXME TEMP SHITE


#endif // __DEFINITIONS__


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
