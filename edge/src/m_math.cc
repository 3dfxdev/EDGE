//----------------------------------------------------------------------------
//  EDGE Floating Point Math Stuff
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

#include "i_defs.h"
#include "m_math.h"

//
// Fixed Number shit - to be removed.
//
static fixed_t FixedDiv2 (fixed_t a, fixed_t b)
{
#ifdef USE_INT64
	return (fixed_t) (((i64_t) a << 16) / ((i64_t) b));
#else
	return (fixed_t) (65536 * ((double) a / (double) b));
#endif
}

fixed_t FixedDiv (fixed_t a, fixed_t b)
{
	if ((abs (a) >> 14) >= abs (b))
		return (a ^ b) < 0 ? INT_MIN : INT_MAX;
	
	return FixedDiv2 (a, b);
}

fixed_t FixedMul (fixed_t a, fixed_t b)
{
	return (fixed_t) ((((i64_t) (a)) * (b)) >> FRACBITS);
}



//
// M_FixedToFloat
//
// Converts a fixed point number to float.
//
// This has been moved to m_inline.h.

//
// M_FloatToFixed
//
// Converts a float to fixed-point.
//
// This has been moved to m_inline.h.

float M_Sin (angle_t ang)
{
	return (float) sin ((double) ang * M_PI / (float) ANG180);
}

float M_Cos (angle_t ang)
{
	return (float) cos ((double) ang * M_PI / (float) ANG180);
}

float M_Tan (angle_t ang)
{
	return (float) tan ((double) ang * M_PI / (float) ANG180);
}

angle_t M_ATan (float slope)
{
	return (angle_t) ((float) ANG180 * atan (slope) / M_PI);
}

void M_Angle2Matrix (angle_t ang, vec2_t * x, vec2_t * y)
{
	x->x = M_Cos (ang);
	x->y = M_Sin (ang);

    y->x = -x->y;
    y->y =  x->x;
}

