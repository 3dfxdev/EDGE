//----------------------------------------------------------------------------
//  EDGE Linux Color Translation
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
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
// Video mode translation
// by Colin Phipps (cph@lxdoom.linuxgames.com)

// CPhipps - Colourmapping
// Yes, I'm afraid so, it's another colour remapping layer.

#ifndef _L_VIDEO_TRANS_H_
#define _L_VIDEO_TRANS_H_

#include "dm_type.h"

typedef struct
{
	int mask;
	int bits;
	int shift;
}
colourshift_t;

// Not really part of this API, but put here as common code 
// This is the TrueColor palette construction stuff

extern colourshift_t redshift;
extern colourshift_t greenshift;
extern colourshift_t blueshift;

void SetColourShift (unsigned long mask, colourshift_t * ps);

#define _rgb_r_shift_16 (redshift.pshift)
#define _rgb_g_shift_16 (greenshift.pshift)
#define _rgb_b_shift_16 (blueshift.pshift)

#endif
