//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Screen Effects)
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

// this conditional applies to the whole file
#include "i_defs.h"

#include "am_map.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "e_search.h"
#include "m_bbox.h"
#include "m_random.h"
#include "p_local.h"
#include "p_mobj.h"
#include "r_defs.h"
#include "r_main.h"
#include "r_sky.h"
#include "r_state.h"
#include "r_things.h"
#include "r2_defs.h"
#include "rgl_defs.h"
#include "v_colour.h"
#include "v_ctx.h"
#include "v_res.h"
#include "w_textur.h"
#include "w_wad.h"
#include "z_zone.h"

#define DEBUG  0


bool ren_allbright;
float ren_red_mul;
float ren_grn_mul;
float ren_blu_mul;

//
// RGL_RainbowEffect
//
// Effects that modify all colours, e.g. nightvision green.
//
void RGL_RainbowEffect(player_t *player)
{
	ren_allbright = false;
	ren_red_mul = ren_grn_mul = ren_blu_mul = 1.0f;

	bool fx_on = (player->effect_left >= EFFECT_MAX_TIME ||
				  (player->effect_left & 8));

	if (fx_on && player->powers[PW_Invulnerable] > 0)
	{
		ren_red_mul = 0.75;
		ren_grn_mul = 0.75;
		ren_blu_mul = 0.75;

		return;
	}

	if (fx_on && player->powers[PW_NightVision] > 0 && player->effect_colourmap)
	{
		float r, g, b;

		V_GetColmapRGB(player->effect_colourmap, &r, &g, &b, false);

		ren_red_mul = 1.0f - (1.0f - r);  // * s;
		ren_grn_mul = 1.0f - (1.0f - g);  // * s;
		ren_blu_mul = 1.0f - (1.0f - b);  // * s;
		ren_allbright = true;

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

	bool fx_on = (player->effect_left >= EFFECT_MAX_TIME ||
				  (player->effect_left & 8));

	if (fx_on && player->powers[PW_Invulnerable] > 0 && player->effect_colourmap)
	{
		float r, g, b;

		V_GetColmapRGB(player->effect_colourmap, &r, &g, &b, false);

		r = MAX(0.5f, r);
		g = MAX(0.5f, g);
		b = MAX(0.5f, b);

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

	bool fx_on = (player->effect_left >= EFFECT_MAX_TIME ||
				  (player->effect_left & 8));

	if (fx_on && player->powers[PW_Invulnerable] > 0 && player->effect_colourmap)
	{
		return;
		
		// -AJA- this looks good in standard Doom, but messes up HacX:
		//	 glColor4f(1.0f, 0.5f, 0.0f, 0.25f);
	}
	else if (fx_on && player->powers[PW_NightVision] > 0 && player->effect_colourmap)
	{
		float r, g, b;
		V_GetColmapRGB(player->effect_colourmap, &r, &g, &b, false);
		glColor4f(r, g, b, 0.20f);
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


