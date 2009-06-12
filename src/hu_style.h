//----------------------------------------------------------------------------
//  EDGE Heads-up-display Style code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2004-2009  The EDGE Team.
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

#ifndef __HU_STYLE__
#define __HU_STYLE__

#include "ddf/style.h"
#include "hu_font.h"
#include "r_image.h"

class style_c
{
friend class style_container_c;

public:
	style_c(styledef_c *_def);
	~style_c();

	styledef_c *def;

	font_c *fonts[styledef_c::NUM_TXST];

	const image_c *bg_image;

public:
	void Load();

	// Drawing functions
	void DrawBackground(int x = 0, int y = 0, int w = 0, int h = 0, int align = 0);
};


style_c *HU_LookupStyle(styledef_c *def);

#endif  // __HU_STYLE__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
