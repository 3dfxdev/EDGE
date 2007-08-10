//----------------------------------------------------------------------------
//  EDGE Basic Types
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

#ifndef __EDGETYPES__
#define __EDGETYPES__

// RGB 8:8:8
// (FIXME: use epi::colour_c)
typedef unsigned int rgbcol_t;

#define RGB_NO_VALUE  0x00FFFF  /* bright CYAN */

#define RGB_MAKE(r,g,b)  (((r) << 16) | ((g) << 8) | (b))

#define RGB_RED(rgbcol)  ((float)(((rgbcol) >> 16) & 0xFF) / 255.0f)
#define RGB_GRN(rgbcol)  ((float)(((rgbcol) >>  8) & 0xFF) / 255.0f)
#define RGB_BLU(rgbcol)  ((float)(((rgbcol)      ) & 0xFF) / 255.0f)


// percentage type.  Ranges from 0.0f - 1.0f
typedef float percent_t;

#define PERCENT_MAKE(val)  ((val) / 100.0f)
#define PERCENT_2_FLOAT(perc)  (perc)


typedef u32_t angle_t;

#define ANGLEBITS  32

// Binary Angle Measument, BAM.
#define ANG0   0x00000000
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

#define ANG5   (ANG45/9)

// Conversion macros:

#define F2AX(n)  (((n) < 0) ? (360.0f + (n)) : (n))
#define ANG_2_FLOAT(a)  ((float) (a) * 360.0f / 4294967296.0f)
#define FLOAT_2_ANG(n)  ((angle_t) (F2AX(n) / 360.0f * 4294967296.0f))


#endif // __EDGETYPES__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
