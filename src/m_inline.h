//----------------------------------------------------------------------------
//  EDGE Inline Functions
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

#include "i_defs.h"

#ifndef EDGE_INLINE
#define EDGE_INLINE(decl,body) extern decl;
#endif

#ifdef FLOAT_IEEE_754
// Use routines written for the usual 32-bit float format

EDGE_INLINE(
long M_FloatToInt(float fl),
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
// Some other kind of float is used, so we have to use the portable versions

EDGE_INLINE(
long M_FloatToInt(float fl),
{
  if (fl >= INT_MAX)
    return (long)INT_MAX;

  if (fl <= INT_MIN)
    return (long)INT_MIN;

  return (long)fl;
})

#endif

