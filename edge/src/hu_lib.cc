//----------------------------------------------------------------------------
//  EDGE Heads-up-display library Code
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

#include "dm_defs.h"
#include "dm_state.h"
#include "hu_stuff.h"
#include "m_inline.h"
#include "r_local.h"
#include "v_colour.h"
#include "v_ctx.h"
#include "v_res.h"

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
	const colourmap_c *colmap, const char *str)
{
	int cx = x;
	int cy = y;

	font_c *font = style->fonts[text_type];

	if (! font)
		I_Error("Style [%s] is missing a font !\n", style->def->ddf.name.GetString());

	for (; *str; str++)
	{
		char ch = *str;

		if (ch == '\n')
		{
			cx = x;
			cy += 12;
			continue;
		}

		if (cx >= 320)
			continue;

		int w = font->CharWidth(ch);

		font->DrawChar(cx, cy, ch, colmap, 1.0f);

		cx += w;
	}
}

//
// HL_WriteText
//
// Write a string using the hu_font.
//
void HL_WriteText(style_c *style, int text_type, int x, int y, const char *str)
{
	HL_WriteTextTrans(style, text_type, x, y, style->def->text[text_type].colmap, str);
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
void HL_DrawTextLineAlpha(hu_textline_t * L, bool drawcursor, const colourmap_c *colmap,
	float alpha)
{
	int i, x, y, w;

	// draw the new stuff
	x = L->x;
	y = L->y;

	font_c *font = L->style->fonts[L->text_type];

	if (! font)
		I_Error("Style [%s] is missing a font !\n", L->style->def->ddf.name.GetString());

	if (! colmap)
		colmap =L->style->def->text[L->text_type].colmap;

	// -AJA- 1999/09/07: centred text.
	if (L->centre)
	{
		x -= font->StringWidth(L->ch) / 2;
	}

	for (i = 0; (i < L->len) && (x < 320); i++, x += w)
	{
		char ch = L->ch[i];

		w = font->CharWidth(ch);

		if (x < -w)
			continue;

		font->DrawChar(x, y, ch, colmap, alpha);
	}

	// draw the cursor if requested
	if (font->HasChar('_'))
	{
		if (drawcursor && x < 320)
		{
			font->DrawChar(x, y, '_', colmap, alpha);
		}
	}
}

void HL_DrawTextLine(hu_textline_t * L, bool drawcursor)
{
	HL_DrawTextLineAlpha(L, drawcursor, NULL, 1.0f);
}

// sorta called by HU_Erase and just better darn get things straight
void HL_EraseTextLine(hu_textline_t * l)
{
	// OBSOLETE
}

//----------------------------------------------------------------------------

void HL_InitSText(hu_stext_t * s, int x, int y, int h, style_c *style, int text_type)
{
	int i;

	s->h = h;
	s->curline = 0;

	font_c *font = style->fonts[text_type];

	if (! font)
		I_Error("Style [%s] is missing a font !\n", style->def->ddf.name.GetString());

	for (i = 0; i < h; i++)
	{
		HL_InitTextLine(&s->L[i], x, y, style, text_type);

		y -= font->NominalHeight() + 1;
	}
}

void HL_AddLineToSText(hu_stext_t * s)
{
	// add a clear line
	if (++s->curline == s->h)
		s->curline = 0;

	HL_ClearTextLine(&s->L[s->curline]);

}

void HL_AddMessageToSText(hu_stext_t * s, const char *prefix, const char *msg)
{
	HL_AddLineToSText(s);

	if (prefix)
		for (; *prefix; prefix++)
			HL_AddCharToTextLine(&s->L[s->curline], *prefix);

	for (; *msg; msg++)
		HL_AddCharToTextLine(&s->L[s->curline], *msg);
}

void HL_DrawSText(hu_stext_t * s)
{
	int i, idx;

	// draw everything
	for (i=0; i < s->h; i++)
	{
		idx = s->curline - i;
		if (idx < 0)
			idx += s->h;  // handle queue of lines

		// need a decision made here on whether to skip the draw.
		// no cursor, please.
		HL_DrawTextLine(&s->L[idx], false);
	}
}

void HL_EraseSText(hu_stext_t * s)
{
	int i;

	for (i=0; i < s->h; i++)
	{
		HL_EraseTextLine(&s->L[i]);
	}
}

//----------------------------------------------------------------------------

void HL_InitIText(hu_itext_t * it, int x, int y, style_c *style, int text_type)
{
	// default left margin is start of text
	it->margin = 0;

	HL_InitTextLine(&it->L, x, y, style, text_type);
}

// The following deletion routines adhere to the left margin restriction
void HL_DelCharFromIText(hu_itext_t * it)
{
	if (it->L.len != it->margin)
		HL_DelCharFromTextLine(&it->L);
}

void HL_EraseLineFromIText(hu_itext_t * it)
{
	while (it->margin != it->L.len)
		HL_DelCharFromTextLine(&it->L);
}

// Resets left margin as well
void HL_ResetIText(hu_itext_t * it)
{
	it->margin = 0;
	HL_ClearTextLine(&it->L);
}

void HL_AddPrefixToIText(hu_itext_t * it, const char *str)
{
	for (; *str; str++)
		HL_AddCharToTextLine(&it->L, *str);

	it->margin = it->L.len;
}

// wrapper function for handling general keyed input.
// returns true if it ate the key
bool HL_KeyInIText(hu_itext_t * it, const char ch)
{
	if (ch >= ' ' && ch <= '_')
		HL_AddCharToTextLine(&it->L, (char)ch);
	else if (ch == KEYD_BACKSPACE)
		HL_DelCharFromIText(it);
	else if (ch != KEYD_ENTER)
		return false;

	// ate the key
	return true;
}

void HL_DrawIText(hu_itext_t * it)
{
	// draw the line with cursor
	HL_DrawTextLine(&it->L, true);
}

void HL_EraseIText(hu_itext_t * it)
{
	HL_EraseTextLine(&it->L);
}

