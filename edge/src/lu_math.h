//----------------------------------------------------------------------------
//  EDGE Mathematics LookUp Tables
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
//
// In the order of appearance:
//
// int finetangent[4096] - Tangens LUT.
//   Should work with BAM fairly well (12 of 16bit, effectively, by shifting).
//
// int finesine[10240] - Sine lookup.
//   Guess what, serves as cosine, too. Remarkable thing is,
//        how to use BAMs with this?
//
// int tantoangle[2049] - ArcTan LUT,
//   maps tan(angle) to angle fast. Gotta search. 
//    

#ifndef __TABLES__
#define __TABLES__

#include "dm_type.h"

// Binary Angle Measument, BAM.
#define ANG1   0x00B60B61
#define ANG45  0x20000000
#define ANG90  0x40000000
#define ANG135 0x60000000
#define ANG180 0x80000000
#define ANG225 0xa0000000
#define ANG270 0xc0000000
#define ANG315 0xe0000000
// Only use this one with float.
#define ANG360  (4294967296.0)
#define ANGLEBITS  32

#define ANG5   (ANG45/9)

#define SLOPERANGE 2048
#define SLOPEBITS  11


// Conversion macros:

#define ANG_2_FLOAT(a)  ((float) (a) * 360.0f / 4294967296.0f)
#define FLOAT_2_ANG(n)  ((angle_t) ((n) / 360.0f * 4294967296.0f))

#define I_ROUND(n)  ((int) (((n) < 0.0f) ? ((n) - 0.5f) : ((n) + 0.5f)))
#define I_FLOOR(n)  ((int) (floor(n) + 0.25f))

#endif
