//------------------------------------------------------------------------
//  EDGE Type definitions
//------------------------------------------------------------------------
//
//  Copyright (c) 2003-2008  The EDGE Team.
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

#ifndef __EPI_TYPE_H__
#define __EPI_TYPE_H__

// basic types

typedef signed char  s8_t; //SBYTE
typedef signed short s16_t;
typedef signed int   s32_t;
typedef long int l32_t;
typedef signed long long s64_t; //SQWORD
 
typedef unsigned char  u8_t; //BYTE
typedef unsigned short u16_t; //WORD

#ifndef USE_WINDOWS_DWORD
typedef uint32_t DWORD;
#endif
typedef unsigned int   u32_t;

typedef unsigned long long u64_t; //QWORD
//typedef unsigned int   u64_t;

typedef u8_t byte;

#ifdef __GNUC__
typedef long long i64_t;
#else
typedef __int64 i64_t;
#endif


#define FRACBITS 16
#define FRACUNIT (1<<FRACBITS)

typedef uint8_t byte;
typedef uint8_t BYTE;
typedef int8_t SBYTE;
typedef uint16_t word;
typedef uint16_t WORD;
typedef int16_t SWORD;
typedef int32_t fixed;
typedef fixed fixed_t;
typedef uint32_t longword;
#ifndef USE_WINDOWS_DWORD
typedef uint32_t DWORD;
typedef u32_t DWORD;
#endif
typedef int32_t SDWORD;
typedef uint64_t QWORD;
typedef int64_t SQWORD;
typedef void* memptr;
typedef uint32_t uint32;
typedef uint32_t BITFIELD;
typedef int INTBOOL;
typedef double real64;
typedef SDWORD int32;

#endif  /*__EPI_TYPE_H__*/

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
