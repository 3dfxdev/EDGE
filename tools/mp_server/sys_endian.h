//------------------------------------------------------------------------
//  Endian handling
//------------------------------------------------------------------------
//
//  Edge MultiPlayer Server (C) 2004  Andrew Apted
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public License
//  (LGPL) as published by the Free Software Foundation.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------
//
//  Based on SDL_byteorder.h and SDL_endian.h.
//
//------------------------------------------------------------------------

#ifndef __SYS_ENDIAN_H__
#define __SYS_ENDIAN_H__

// the two types of endianness
#define SYS_LIL_ENDIAN  1234
#define SYS_BIG_ENDIAN  4321

#if defined(__LITTLE_ENDIAN__) ||  \
    defined(__i386__)  || defined(__ia64__) || defined(WIN32)   ||  \
	defined(__alpha__) || defined(__alpha)  || defined(__arm__) ||  \
	(defined(__mips__) && defined(__MIPSEL__)) ||  \
	defined(__SYMBIAN32__) || defined(__x86_64__)
#define SYS_BYTEORDER   SYS_LIL_ENDIAN
#else
#define SYS_BYTEORDER   SYS_BIG_ENDIAN
#endif

// The macros used to swap values.  Try to use superfast macros on systems
// that support them, otherwise use C++ inline functions.

#ifdef LINUX
#include <endian.h>
#ifdef __arch__swab16
#define SYS_Swap16  __arch__swab16
#endif
#ifdef __arch__swab32
#define SYS_Swap32  __arch__swab32
#endif
#endif /* LINUX */

#ifndef SYS_Swap16
#define SYS_Swap16  endian_swapper_c::Swap16
#endif

#ifndef SYS_Swap32
#define SYS_Swap32  endian_swapper_c::Swap32
#endif


// Byteswap item between the specified endianness to the native endianness
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
#define SYS_LE_U16(X)  ((u16_t)(X))
#define SYS_LE_U32(X)  ((u32_t)(X))
#define SYS_BE_U16(X)  SYS_Swap16(X)
#define SYS_BE_U32(X)  SYS_Swap32(X)
#else
#define SYS_LE_U16(X)  SYS_Swap16(X)
#define SYS_LE_U32(X)  SYS_Swap32(X)
#define SYS_BE_U16(X)  ((u16_t)(X))
#define SYS_BE_U32(X)  ((u32_t)(X))
#endif

#define SYS_LE_S16(X)  ((s16_t) SYS_LE_U16((u16_t) (X)))
#define SYS_LE_S32(X)  ((s32_t) SYS_LE_U32((u32_t) (X)))
#define SYS_BE_S16(X)  ((s16_t) SYS_BE_U16((u16_t) (X)))
#define SYS_BE_S32(X)  ((s32_t) SYS_BE_U32((u32_t) (X)))

class endian_swapper_c
{
public:
	static u16_t Swap16(u16_t x);
	static u32_t Swap32(u32_t x);
};

// Swap 16bit, that is, MSB and LSB byte.
inline u16_t endian_swapper_c::Swap16(u16_t x)
{
	// No masking with 0xFF should be necessary.
	return (x >> 8) | (x << 8);
}

// Swapping 32bit.
inline u32_t endian_swapper_c::Swap32(u32_t x)
{
	return
		(x >> 24)
		| ((x >> 8) & 0xff00)
		| ((x << 8) & 0xff0000)
		| (x << 24);
}

#endif  /* __SYS_ENDIAN_H__ */
