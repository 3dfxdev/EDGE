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

//
// HL_Init
//
void HL_Init(void)
{
	/* nothing to init */
}

//
// HL_WriteTextTrans
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
// HL_WriteText
//
// Write a string using the hu_font.
//
void HL_WriteText(style_c *style, int text_type, int x, int y, const char *str, float scale, float alpha)
{
	HL_WriteTextTrans(style, text_type, x, y,
			V_GetFontColor(style->def->text[text_type].colmap),
			str, scale, alpha);
}

//----------------------------------------------------------------------------

void HL_ClearTextLine(hu_textline_t * t)
{
	t->len = 0;
	t->ch[0] = 0;
	t->centre = false;
}

void HL_InitTextLine(hu_textline_t * t, int x, int y, style_c *style, int text_type)
{
	t->x = x;
	t->y = y;
	t->style = style;
	t->text_type = text_type;

	HL_ClearTextLine(t);
}

bool HL_AddCharToTextLine(hu_textline_t * t, char ch)
{
	if (t->len >= HU_MAXLINELENGTH-1)
		return false;

	t->ch[t->len++] = ch;
	t->ch[t->len] = 0;
	return true;
}

bool HL_DelCharFromTextLine(hu_textline_t * t)
{
	if (!t->len)
		return false;

	t->len--;
	t->ch[t->len] = 0;
	return true;
}

//
// HL_DrawTextLineAlpha
//
// New Procedure: Same as HL_DrawTextLine, but the
// colour is passed through the given translation table
// and scaled if possible.
//
// -ACB- 1998/09/11 Index changed from JC's Pre-Calculated to using
//                  the PALREMAP translation maps.
//
// -AJA- 2000/03/05: Index replaced with pointer to trans table.
// -AJA- 2000/10/22: Renamed for alpha support.
//
void HL_DrawTextLineAlpha(hu_textline_t * L, bool drawcursor,
			rgbcol_t col, float alpha)
{
	int i, x, y, w;

	// draw the new stuff
	x = L->x;
	y = L->y;

	font_c *font = L->style->fonts[L->text_type];

	if (! font)
		I_Error("Style [%s] is missing a font !\n", L->style->def->ddf.name.c_str());

	if (col == RGB_NO_VALUE)
		col = V_GetFontColor(L->style->def->text[L->text_type].colmap);

	// -AJA- 1999/09/07: centred text.
	if (L->centre)
	{
		x -= font->StringWidth(L->ch) / 2;
	}

	float scale = 1.0f;

	scale *= L->style->def->text[L->text_type].scale;

	for (i = 0; (i < L->len) && (x < 320); i++, x += w)
	{
		char ch = L->ch[i];

		w = font->CharWidth(ch);

		if (x < -w)
			continue;

		font->DrawChar320(x, y, ch, scale,1.0f, col, alpha);
	}

	// draw the cursor if requested
	if (font->HasChar('_'))
	{
		if (drawcursor && x < 320)
		{
			font->DrawChar320(x, y, '_', scale,1.0f, col, alpha);
		}
	}
}

void HL_DrawTextLine(hu_textline_t * L, bool drawcursor)
{
	HL_DrawTextLineAlpha(L, drawcursor, RGB_NO_VALUE, 1.0f);
}

// sorta called by HU_Erase and just better darn get things straight
void HL_EraseTextLine(hu_textline_t * l)
{
	// OBSOLETE
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
