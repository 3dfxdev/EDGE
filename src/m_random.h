//----------------------------------------------------------------------------
//  EDGE Pseudo-Random Number Code (via LUT)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2000  The EDGE Team.
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

#ifndef __M_RANDOM__
#define __M_RANDOM__

#include "dm_type.h"

// 
// M_Random
//
// Returns a number from 0 to 255.
//
// -AJA- Note: this function should be called for all random values
// that do not interfere with demo/netgame synchronisation (for example,
// placement of bullet puffs).

int M_Random(void);

// 
// P_Random
//
// Returns a number from 0 to 255.
//
// -AJA- Note: that this function should be called for all random values
// values that determine demo/netgame synchronisation (for example,
// which way a monster should travel).

int P_Random(void);

// 
// P_RandomNegPos
//
// Returns a number between -255 and 255, but skewed so that values near
// zero have a higher probability.  Replaces "P_Random()-P_Random()" in
// the code, which as Lee Killough points out can produce different
// results depending upon the order of evaluation.
//
// -AJA- Note: same usage rules as P_Random.

int P_RandomNegPos(void);

// Fix randoms for demos.
void M_SeedRandom(int seed);

// Something to do with consistency...
int P_RandomIndex(void);

// This is used in various places:
extern unsigned char rndtable[256];

#endif
