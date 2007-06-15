//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Occlusion testing)
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
//
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#ifndef __RGL_OCC_
#define __RGL_OCC_


void RGL_1DOcclusionClear(void);
void RGL_1DOcclusionSet(angle_t low, angle_t high);
bool RGL_1DOcclusionTest(angle_t low, angle_t high);

void RGL_1DOcclusionPush(void);
void RGL_1DOcclusionPop(void);


#endif  // __RGL_OCC_

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
