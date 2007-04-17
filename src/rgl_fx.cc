//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Screen Effects)
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

#include "i_defs.h"
#include "i_defs_gl.h"

#include "m_misc.h"
#include "r_main.h"
#include "v_colour.h"
#include "r_modes.h"

#define DEBUG  0


int ren_extralight;

float ren_red_mul;
float ren_grn_mul;
float ren_blu_mul;

static INLINE float EffectStrength(player_t *player)
{
	if (player->effect_left >= EFFECT_MAX_TIME)
		return 1.0f;

	if (var_fadepower)
	{
		return player->effect_left / (float)EFFECT_MAX_TIME;
	}

	return (player->effect_left & 8) ? 1.0f : 0.0f;
}

//
// RGL_RainbowEffect
//
// Effects that modify all colours, e.g. nightvision green.
//
void RGL_RainbowEffect(player_t *player)
{
	ren_extralight = player->extralight * 16;

	ren_red_mul = ren_grn_mul = ren_blu_mul = 1.0f;

	float s = EffectStrength(player);

	if (s > 0 && player->powers[PW_Invulnerable] > 0)
	{
		ren_red_mul = 0.90f - usegamma / 10.0f; 
		ren_red_mul += (1.0f - ren_red_mul) * (1.0f - s);

		ren_grn_mul = ren_red_mul;
		ren_blu_mul = ren_red_mul;
		return;
	}

	if (s > 0 && player->powers[PW_NightVision] > 0 && player->effect_colourmap)
	{
		float r, g, b;

		V_GetColmapRGB(player->effect_colourmap, &r, &g, &b, false);

		ren_red_mul = 1.0f - (1.0f - r) * s;
		ren_grn_mul = 1.0f - (1.0f - g) * s;
		ren_blu_mul = 1.0f - (1.0f - b) * s;

		ren_extralight = int(s * 255);
		return;
	}

	if (s > 0 && player->powers[PW_Infrared] > 0)
	{
		ren_extralight = int(s * 255);
		return;
	}
}


//
// RGL_ColourmapEffect
//
// For example: all white for invulnerability.
//
void RGL_ColourmapEffect(player_t *player)
{
	int x1, y1;
	int x2, y2;

	float s = EffectStrength(player);

	if (s > 0 && player->powers[PW_Invulnerable] > 0 && player->effect_colourmap)
	{
		float r, g, b;

		V_GetColmapRGB(player->effect_colourmap, &r, &g, &b, false);

		r = MAX(0.5f, r) * (s + 1.0f) / 2.0f;
		g = MAX(0.5f, g) * (s + 1.0f) / 2.0f;
		b = MAX(0.5f, b) * (s + 1.0f) / 2.0f;

		glColor4f(r, g, b, 0.0f);
		glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);

		glEnable(GL_BLEND);

		glBegin(GL_QUADS);
	  
		x1 = viewwindowx;
		x2 = viewwindowx + viewwindowwidth;

		y1 = SCREENHEIGHT - viewwindowy;
		y2 = SCREENHEIGHT - viewwindowy - viewwindowheight;

		glVertex2i(x1, y1);
		glVertex2i(x2, y1);
		glVertex2i(x2, y2);
		glVertex2i(x1, y2);

		glEnd();
	  
		glDisable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}

//
// RGL_PaletteEffect
//
// For example: red wash for pain.
//
void RGL_PaletteEffect(player_t *player)
{
	byte rgb_data[3];

	float s = EffectStrength(player);

	if (s > 0 && player->powers[PW_Invulnerable] > 0 && player->effect_colourmap)
	{
		if (! player->effect_colourmap->lump_name.IsEmpty())  // TEMP HACK
		{
			// -AJA- this looks good in standard Doom, but messes up HacX:
			glColor4f(1.0f, 0.5f, 0.0f, 0.22f * s);
		}
	}
	else if (s > 0 && player->powers[PW_NightVision] > 0 && player->effect_colourmap)
	{
		float r, g, b;
		V_GetColmapRGB(player->effect_colourmap, &r, &g, &b, false);
		glColor4f(r, g, b, 0.20f * s);
	}
	else
	{
		V_IndexColourToRGB(pal_black, rgb_data);

		int rgb_max = MAX(rgb_data[0], MAX(rgb_data[1], rgb_data[2]));

		if (rgb_max == 0)
			return;
	  
		rgb_max = MIN(200, rgb_max);

		glColor4f((float) rgb_data[0] / (float) rgb_max,
				  (float) rgb_data[1] / (float) rgb_max,
				  (float) rgb_data[2] / (float) rgb_max,
			      (float) rgb_max / 255.0f);
	}

	glEnable(GL_BLEND);

	glBegin(GL_QUADS);
  
	glVertex2i(0, SCREENHEIGHT);
	glVertex2i(SCREENWIDTH, SCREENHEIGHT);
	glVertex2i(SCREENWIDTH, 0);
	glVertex2i(0, 0);

	glEnd();
  
	glDisable(GL_BLEND);
}



//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
