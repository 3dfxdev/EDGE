//----------------------------------------------------------------------------
//  EDGE Endianess handling
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2003  The EDGE Team.
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

#ifndef __M_SWAP__
#define __M_SWAP__

// Endianess handling.
// WAD files are stored little endian.

#ifdef __BIG_ENDIAN__

// -AJA- 2003/10/06: use inline functions to prevent warnings
class endian_swapper_c
{
  public:
    static short SHORT(unsigned short x);
    static long  LONG (unsigned long  x);
};

// Swap 16bit, that is, MSB and LSB byte.
inline short endian_swapper_c::SHORT(unsigned short x)
{
  // No masking with 0xFF should be necessary.
  return (x >> 8) | (x << 8);
}

// Swapping 32bit.
inline long endian_swapper_c::LONG(unsigned long x)
{
  return
      (x >> 24)
      | ((x >> 8) & 0xff00)
      | ((x << 8) & 0xff0000)
      | (x << 24);
}

#define SHORT(x)  endian_swapper_c::SHORT(x)
#define LONG(x)   endian_swapper_c::LONG(x)

#else

#define SHORT(x) ((short) (x))
#define LONG(x)  ((long) (x))

#endif

#define USHORT(x) ((unsigned short) SHORT(x))
#define ULONG(x)  ((unsigned long) LONG(x))

#endif
