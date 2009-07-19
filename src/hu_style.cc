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

#include "i_defs.h"

#include <map>

#include "hu_style.h"
#include "g_state.h"
#include "r_local.h"
#include "r_colormap.h"
#include "hu_draw.h"
#include "r_modes.h"
#include "r_image.h"


// Edge has lots of style
static std::map<styledef_c *, style_c*> hu_styles;


style_c::style_c(styledef_c *_def) : def(_def), bg_image(NULL)
{
	for (int T = 0; T < styledef_c::NUM_TXST; T++)
		fonts[T] = NULL;
}

style_c::~style_c()
{
	/* nothing to do */
}


void style_c::Load()
{
	if (def->bg.image_name.c_str())
	{
		const char *name = def->bg.image_name.c_str();

		bg_image = W_ImageLookup(name, INS_Flat, ILF_Null);

		if (! bg_image)
			bg_image = W_ImageLookup(name, INS_Graphic);
	}

	for (int T = 0; T < styledef_c::NUM_TXST; T++)
	{
		if (def->text[T].font)
			fonts[T] = HU_LookupFont(def->text[T].font);
	}
}

//
// !!! FIXME: align -- temp fix for console.
//
void style_c::DrawBackground(int x, int y, int w, int h, int align)
{
	if (w == 0)
		x = 0, w = SCREENWIDTH;

	if (h == 0)
		y = 0, h = SCREENHEIGHT;

	float alpha = PERCENT_2_FLOAT(def->bg.translucency);

	if (alpha < 0.02)
		return;

	if (! bg_image)
	{
		if (def->bg.colour != RGB_NO_VALUE)
			RGL_SolidBox(x, y, w, h, def->bg.colour, alpha);
		return;
	}

	float right = IM_RIGHT(bg_image);
	float top   = IM_TOP(bg_image);

	if (def->special & SYLSP_Tiled)
	{
		float y_scale = def->bg.scale;
		float x_scale = def->bg.aspect * y_scale;

		x_scale *= (float)SCREENWIDTH  / 320.0f;
		y_scale *= (float)SCREENHEIGHT / 200.0f;

		RGL_DrawImage(x, y, w, h, bg_image,
				0.0f, align ? (1.0f - top * h / IM_HEIGHT(bg_image) / y_scale) : 0.0f,
				right  * w / IM_WIDTH(bg_image)  / x_scale,
				align ? 1.0f : (top * h / IM_HEIGHT(bg_image) / y_scale),
				alpha);
	}
	else if (def->special & SYLSP_TiledNoScale)
	{
		RGL_DrawImage(x, y, w, h, bg_image,
				0.0f, align ? (1.0f - top * h / IM_HEIGHT(bg_image)) : 0.0f,
				right  * w / IM_WIDTH(bg_image),
				align ? 1.0f : (top * h / IM_HEIGHT(bg_image)),
				alpha);
	}
	else
	{
		RGL_DrawImage(x, y, w, h, bg_image, 0.0f, 0.0f,
					  right, top, alpha);
	}
}


style_c *HU_LookupStyle(styledef_c *def)
{
	if (hu_styles.find(def) != hu_styles.end())
	{
		return hu_styles[def];
	}

	style_c *new_st = new style_c(def);
	hu_styles[def] = new_st;

	new_st->Load();

	return new_st;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
