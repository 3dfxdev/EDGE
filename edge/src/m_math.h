//----------------------------------------------------------------------------
//  EDGE Floating Point Math Stuff
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

#ifndef __M_MATH_H__
#define __M_MATH_H__

#include "m_fixed.h"

typedef struct vec2_s
{
  float_t x, y;
}
vec2_t;

#define Vec2Add(dest, src)  do {  \
    (dest).x += (src).x; (dest).y += (src).y; } while(0)

#define Vec2Sub(dest, src)  do {  \
    (dest).x -= (src).x; (dest).y -= (src).y; } while(0)

#define Vec2Mul(dest, val)  do {  \
    (dest).x *= (val); (dest).y *= (val); } while(0)

typedef struct vec3_s
{
  float_t x, y, z;
}
vec3_t;

#define Vec3Add(dest, src)  do {  \
    (dest).x += (src).x;  (dest).y += (src).y;  \
    (dest).z += (src).z; } while(0)

#define Vec3Sub(dest, src)  do {  \
    (dest).x -= (src).x;  (dest).y -= (src).y;  \
    (dest).z -= (src).z; } while(0)

#define Vec3Mul(dest, val)  do {  \
    (dest).x *= (val); (dest).y *= (val);  \
    (dest).z *= (val); } while(0)


fixed_t FixedDiv(fixed_t a, fixed_t b) GCCATTR(const);
fixed_t FixedMul(fixed_t a, fixed_t b) GCCATTR(const);
float_t M_Tan(angle_t ang)             GCCATTR(const);
angle_t M_ATan(float_t slope)          GCCATTR(const);
float_t M_Cos(angle_t ang)             GCCATTR(const);
float_t M_Sin(angle_t ang)             GCCATTR(const);
void M_Angle2Matrix(angle_t ang, vec2_t *x, vec2_t *y);

/* CRC stuff */

void CRC32_Init(unsigned long *crc);
void CRC32_ProcessByte(unsigned long *crc, byte data);
void CRC32_ProcessBlock(unsigned long *crc, const byte *data, int len);
void CRC32_Done(unsigned long *crc);

void CRC32_ProcessInt(unsigned long *crc, int value);
void CRC32_ProcessFixed(unsigned long *crc, fixed_t value);
void CRC32_ProcessFloat(unsigned long *crc, float_t value);
void CRC32_ProcessStr(unsigned long *crc, const char *str);

#endif //__M_MATH_H__

