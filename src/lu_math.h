//----------------------------------------------------------------------------
//  EDGE Mathematics LookUp Tables
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

#include "m_fixed.h"
#include "dm_type.h"
#define FINEANGLES 8192
#define FINEMASK (FINEANGLES-1)

// 0x100000000 to 0x2000
#define ANGLETOFINESHIFT 19

// Effective size is 10240.
extern fixed_t finesine[5 * FINEANGLES / 4];

// Re-use data, is just PI/2 phase shift.
extern fixed_t *finecosine;

// Effective size is 4096.
extern fixed_t finetangent[FINEANGLES / 2];

// Binary Angle Measument, BAM.
#define ANG1   0x00B60B61
#define ANG45  0x20000000
#define ANG90  0x40000000
#define ANG135 0x60000000
#define ANG180 0x80000000
#define ANG225 0xa0000000
#define ANG270 0xc0000000
#define ANG315 0xe0000000
// Only use this one with float_t.
#define ANG360  (4294967296.0)
#define ANGLEBITS  32

#define SLOPERANGE 2048
#define SLOPEBITS  11
#define DBITS      (FRACBITS-SLOPEBITS)

// Effective size is 2049;
// The +1 size is to handle the case when x==y without additional checking.
extern angle_t tantoangle[SLOPERANGE + 1];

// Utility function, called by R_PointToAngle.
int SlopeDiv(unsigned num, unsigned den);

// Conversion macros:

#define FIX_2_FLOAT(x)  ((float_t) (x) / 65536.0)
#define FLOAT_2_FIX(f)  ((fixed_t) ((f) * 65536.0))

#define ANG_2_FLOAT(a)  ((float_t) (a) * 360.0 / 4294967296.0)
#define FLOAT_2_ANG(f)  ((angle_t) ((f) / 360.0 * 4294967296.0))

#endif
