//----------------------------------------------------------------------------
//  EDGE Heads-up-display Font code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2004  The EDGE Team.
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
#include "hu_font.h"

#include "dm_defs.h"
#include "dm_state.h"
#include "m_inline.h"
#include "r_local.h"
#include "v_colour.h"
#include "v_ctx.h"
#include "v_res.h"
#include "w_image.h"


#define DUMMY_WIDTH  4

void font_c::LoadPatches()
{
	// !!!!!! FIXME
}

//
// font_c::Load
//
void font_c::Load()
{
	if (def->type == FNTYP_Patch)
		LoadPatches();
	else
		I_Error("Coding error, unknown font type %d\n", def->type);
}

//
// font_c::NominalWidth, NominalHeight
//
int font_c::NominalWidth() const
{
	return p_cache.width; // FIXME: assumes FNTYP_Patch
}

int font_c::NominalHeight() const
{
	return p_cache.height; // FIXME: assumes FNTYP_Patch
}

//
// font_c::HasChar
//
bool font_c::HasChar(char ch) const
{
	DEV_ASSERT2(def->type == FNTYP_Patch);

	int idx = int(ch) & 0x00FF;

	if (! (p_cache.first <= idx && idx <= p_cache.last))
		return false;
	
	return (p_cache.images[idx - p_cache.first] != NULL);
}

//
// font_c::CharImage
//
const struct image_s *font_c::CharImage(char ch) const
{
	DEV_ASSERT2(def->type == FNTYP_Patch);

	if (! HasChar(ch))
		return p_cache.missing;

	int idx = int(ch) & 0x00FF;

	DEV_ASSERT2(p_cache.first <= idx && idx <= p_cache.last);
	
	return p_cache.images[idx - p_cache.first];
}

//
// font_c::CharWidth
//
// Returns the width of the IBM cp437 char in the font.
//
int font_c::CharWidth(char ch) const
{
	DEV_ASSERT2(def->type == FNTYP_Patch);

	const struct image_s *im = CharImage(ch);

	if (! im)
		return DUMMY_WIDTH;

	int w = IM_WIDTH(im);

	// -AJA- make text look nicer on low resolutions
	if (SCREENWIDTH < 620 && SCREENWIDTH > 320)
	{
		w = (w * 320 + SCREENWIDTH - 64) / SCREENWIDTH;
	}

	return w;
}

//
// font_c::MaxFit
//
// Returns the maximum number of characters which can fit within pixel_w
// pixels.  The string may not contain any newline characters.
//
int font_c::MaxFit(int pixel_w, const char *str) const
{
	int w = 0;
	const char *s;

	// just add one char at a time until it gets too wide or the string ends.
	for (s = str; *s; s++)
	{
		w += CharWidth(*s);

		if (w > pixel_w)
		{
			// if no character could fit, an infinite loop would probably start,
			// so it's better to just imagine that one character fits.
			if (s == str)
				s++;

			break;
		}
	}

	// extra spaces at the end of the line can always be added
	while (*s == ' ')
		s++;

	return s - str;
}

//
// font_c::StringWidth
//
// Find string width from hu_font chars.  The string may not contain
// any newline characters.
//
int font_c::StringWidth(const char *str) const
{
	int w = 0;

	while (*str)
		w += CharWidth(*str++);

	return w;
}

//
// font_c::StringLines
//
// Find number of lines in string.
//
int font_c::StringLines(const char *str) const
{
	int lines = 1;

	for (; *str; str++)
		if (*str == '\n')
			lines++;

	return lines;
}

