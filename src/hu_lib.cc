//----------------------------------------------------------------------------
//  EDGE Heads-up-display library Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2008  The EDGE Team.
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
//
// -KM- 1998/10/29 Modified to allow foreign characters like '?'
//

#include "i_defs.h"
#include "hu_lib.h"

#include "e_keys.h"
#include "g_state.h"
#include "hu_stuff.h"
#include "r_local.h"
#include "r_colormap.h"
#include "r_draw.h"
#include "r_modes.h"

#define DUMMY_WIDTH(font)  (4)

#define HU_CHAR(ch)  (islower(ch) ? toupper(ch) : (ch))
#define HU_INDEX(c)  ((unsigned char) HU_CHAR(c))


void HL_Init(void)
{
	/* nothing to init */
}

//
// Write a string using the hu_font and index translator.
//
void HL_WriteTextTrans(style_c *style, int text_type, int x, int y,
	rgbcol_t col, const char *str, float scale, float alpha)
{
	float cx = x;
	float cy = y;

	font_c *font = style->fonts[text_type];

	scale *= style->def->text[text_type].scale;

	if (! font)
		I_Error("Style [%s] is missing a font !\n", style->def->ddf.name.c_str());

	for (; *str; str++)
	{
		char ch = *str;

		if (ch == '\n')
		{
			cx = x;
			cy += 12.0f * scale;  // FIXME: use font's height
			continue;
		}

		if (cx >= 320.0f)
			continue;

		font->DrawChar320(cx, cy, ch, scale,1.0f, col, alpha);

		cx += font->CharWidth(ch) * scale;
	}
}

//
// Write a string using the hu_font.
//
void HL_WriteText(style_c *style, int text_type, int x, int y, const char *str, float scale, float alpha)
{
	HL_WriteTextTrans(style, text_type, x, y,
			V_GetFontColor(style->def->text[text_type].colmap),
			str, scale, alpha);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
