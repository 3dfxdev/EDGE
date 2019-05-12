//----------------------------------------------------------------------------
//  EDGE VITA MATH DEFINES (NEON)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2019  The EDGE Team.
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

#ifndef _NEON_MATHFUN_H_
#define _NEON_MATHFUN_H_

#include <arm_neon.h>

typedef float32x4_t v4sf;  // vector of 4 float
typedef uint32x4_t v4su;  // vector of 4 uint32
typedef int32x4_t v4si;  // vector of 4 uint32

v4sf log_ps(v4sf x);
v4sf exp_ps(v4sf x);
void sincos_ps(v4sf x, v4sf *ysin, v4sf *ycos);
v4sf sin_ps(v4sf x);
v4sf cos_ps(v4sf x);

#endif