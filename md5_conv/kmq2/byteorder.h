//----------------------------------------------------------------------------
//  EDGE2 ByteOrder
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2015 Isotope SoftWorks and Contributors.
//  Based on the Quake II GPL Source Code, (C) id Software.
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

#ifndef _BYTEORDER_H_
#define _BYTEORDER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "q2limits.h"

typedef long long qint64;

short   BigShort(short l);
short   LittleShort(short l);
int     BigLong (int l);
int     LittleLong (int l);
qint64  BigLong64 (qint64 l);
qint64  LittleLong64 (qint64 l);
float   BigFloat (float l);
float   LittleFloat (float l);

void    Swap_Init (void);

#ifdef __cplusplus
}
#endif

#endif
 