//----------------------------------------------------------------------------
//  EDGE Sky Handling Code
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

#ifndef __R_SKY__
#define __R_SKY__

// SKY, store the number for name.
#define SKYFLATNAME  "F_SKY1"

// The sky map is 256*4 wide (10 bits), and angles have 32 bits
#define ANGLETOSKYSHIFT  (32 - 10)

extern float skytexturemid;

extern const struct image_s *sky_image;

// -ES- 1999-04-11 Added This
extern float skytexturescale;

// Called every frame.
void R_InitSkyMap(void);

// Used by GL renderer
void R_ComputeSkyHeights(void);

#endif
