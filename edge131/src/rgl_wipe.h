//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Definitions)
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

#ifndef __RGL_WIPE__
#define __RGL_WIPE__

#include "wp_main.h"  // -AJA- 2003/10/11: ouch, needed for wipetype_e


void RGL_InitWipe(int reverse, wipetype_e effect);
void RGL_StopWipe(void);
bool RGL_DoWipe(void);


#endif  // __RGL_WIPE__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
