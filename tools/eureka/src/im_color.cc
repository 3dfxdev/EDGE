//------------------------------------------------------------------------
//  COLORS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
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
//------------------------------------------------------------------------
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#include "main.h"

#include "im_color.h"
#include "im_img.h"  // IMG_TRANSP
#include "r_misc.h"
#include "w_file.h"
#include "w_wad.h"


pcolour_t palette[256];

int trans_replace;


extern int usegamma;
extern int gammatable[5][256];


void W_LoadPalette()
{
	Lump_c *lump = WAD_FindLump("PLAYPAL");

	if (! lump)
	{
		FatalError("PLAYPAL lump not found.\n");
		return;
	}

	byte raw_pal[256*3];

	if (! lump->Seek() ||
		! lump->Read(raw_pal, sizeof(raw_pal)))
	{
		warn("PLAYPAL: read error\n");
		return;
	}

	byte tr = raw_pal[IMG_TRANSP*3 + 0];
	byte tg = raw_pal[IMG_TRANSP*3 + 0];
	byte tb = raw_pal[IMG_TRANSP*3 + 0];

	int best_dist = (1 << 30);

	for (int c = 0; c < 256; c++)
	{
		byte r = raw_pal[c*3+0];
		byte g = raw_pal[c*3+1];
		byte b = raw_pal[c*3+2];

		// Find the colour closest to IMG_TRANSP
		if (c != IMG_TRANSP)
		{
			int sr = (int)r - (int)tr;
			int sg = (int)g - (int)tg;
			int sb = (int)b - (int)tb;

			int dist = sr*sr + sg*sg + sb*sb;

			if (dist < best_dist)
			{
				best_dist = dist;
				trans_replace = c;
			}
		}

		r = gammatable[usegamma][r];
		g = gammatable[usegamma][g];
		b = gammatable[usegamma][b];
		
		palette[c] = fl_rgb_color(r, g, b);
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
