//----------------------------------------------------------------------------
//  EDGE Inline Functions
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

#include "i_defs.h"

#ifndef EDGE_INLINE
#define EDGE_INLINE(decl,body) extern decl;
#endif

EDGE_INLINE(
float_t M_FixedToFloat(fixed_t fix),
{
  return (float_t)fix / 65536;
})

#ifdef FLOAT_IEEE_754
// Use routines written for the usual 32-bit float format
EDGE_INLINE(
fixed_t M_FloatToFixed(float_t fl),
{
  long i;
  int sign;

  i = *(long*)&fl;
  // extract sign bit: -1 if negative, 0 if positive.
  sign = i >> 31;
  // mask sign bit.
  i &= 0x7FFFFFFF;
  // check bounds: Prevent shift wraparound
  if (i >= 0x47000000)
  {
    // Very big/small.
    // Return MAXINT or MININT depending on sign
    return 0x7FFFFFFF - sign;
  }
  // near zero.
  if (i <= 0x37000000)
  {
    return 0;
  }

  // Extract the mantissa (which is the last 23 bits of the float, plus
  // an extra leading bit). Shift these according to the exponent
  // (which is i>>23), and finally apply the sign.
  return (((((i & 0x007FFFFF) | 0x00800000) << 7) >> (0x8d - (i >> 23))) + sign) ^ sign;
})

EDGE_INLINE(
long M_FloatToInt(float_t fl),
{
  long i;
  int sign;

  i = *(long*)&fl;
  sign = i >> 31;
  i &= 0x7FFFFFFF;
  if (i >= 0x4f000000)
  {
    return 0x7FFFFFFF - sign;
  }
  if (i <= 0x3f000000)
  {
    return 0;
  }

  return (((((i & 0x007FFFFF) | 0x00800000) << 7) >> (0x7d - (i >> 23))) + sign) ^ sign;
})
#else
// Some other kind of float_t is used, so we have to use the portable versions

EDGE_INLINE(
fixed_t M_FloatToFixed(float_t fl),
{
  if (fl > 32767)
    return (fixed_t) INT_MAX;

  if (fl < -32767)
    return (fixed_t) INT_MIN;

  return (fixed_t)(fl * 65536);
})

EDGE_INLINE(
long M_FloatToInt(float_t fl),
{
  if (fl >= INT_MAX)
    return (long)INT_MAX;

  if (fl <= INT_MIN)
    return (long)INT_MIN;

  return (long)fl;
})

#endif

