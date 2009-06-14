//----------------------------------------------------------------------------
//  EDGE TITLE SCREEN
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2009  The EDGE Team.
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

#include "g_game.h"
#include "r_misc.h"
#include "r_gldefs.h"
#include "r_draw.h"
#include "r_modes.h"


typedef struct
{
	float x, y;
	float t;
	float size;
	float r, g, b;
}
star_info_t;

static std::list<star_info_t> star_field;

#define STAR_ALPHA(t_diff)  (0.99 - (t_diff) / 2000.0)

#define STAR_Z(t_diff)  (0.3 + (t_diff) / 100.0)


static void RemoveDeadStars(int millies)
{
	for (;;)
	{
		if (star_field.empty())
			break;

		float t_diff = star_field.front().t - millies;

		if (STAR_ALPHA(t_diff) > 0)
			break;

		star_field.pop_front();
	}
}


static void DrawOneStar(float mx, float my, float sz,
                        float r, float g, float b, float alpha)
{
	float x = SCREENWIDTH  * mx / 320.0f;
	float y = SCREENHEIGHT * my / 200.0f;

	glColor4f(r, g, b, alpha);

	glBegin(GL_POLYGON);

	glVertex2f(x - sz, y - sz);
	glVertex2f(x - sz, y + sz);
	glVertex2f(x + sz, y + sz);
	glVertex2f(x + sz, y - sz);

	glEnd();
}

static void DrawStars(int millies)
{
	float dist = 0; //// (millies < 1600) ? 0 : (millies - 1600);

	std::list<star_info_t>::iterator SI;

	for (SI = star_field.begin(); SI != star_field.end(); SI++)
	{
		float t_diff = millies - SI->t;

		float alpha = STAR_ALPHA(t_diff);
		float z     = STAR_Z(t_diff);

		if (z * 611 < 6930 - dist * 100.0)
		{
			float w = SI->size / (7000 - z * 611);

			float dx = (SI->x - 160) * z / 7.0;
			float dy = (SI->y - 100) * z / 12.0;
		
			DrawOneStar(SI->x + dx, SI->y + dy, w,
						SI->r, SI->g, SI->b, alpha);
		}
  	}
}


static void DrawName(int millies)
{
#if 0
  hud.set_alpha(1)
  hud.solid_box(0, 0, 320, 30, "#000000")
  hud.solid_box(0, 170, 320, 30, "#000000")

  hud.stretch_image(0, 30, 320, 140, "EDGE")

  fade = (mmm - 11) / 30 
  if fade > 1 then fade = 2 - fade end
  if fade > 0 then
    hud.set_alpha(fade ^ 1.5)
    hud.stretch_image(60, 160, 200, 20, "EDGE_MNEM")
  end
#endif
}


static void InsertNewStars(int millies)
{
	static int all_count = 0;

	int total = millies;

	if (millies > 740)
	{
		float f = (2300 - millies) / float(2300 - 740);
		total = millies + int(millies * f);
	}

	total = total * 2 / 4;

	for (; all_count < total; all_count++)
	{
		star_info_t st;

		st.x = (rand() & 0x0FFF) / 12.80f;
		st.y = (rand() & 0x0FFF) / 20.48f;

		st.t = millies;

		st.size = ((rand() & 0x0FFF) / 4096.0 + 1) * 16000.0;

		st.r = pow((rand() & 0xFF) / 255.0, 0.3);
		st.g = pow((rand() & 0xFF) / 255.0, 0.3) / 2.0;
		st.b = pow((rand() & 0xFF) / 255.0, 0.3);

		star_field.push_back(st);
	}
}


bool RGL_DrawTitle(int millies)
{
	millies = millies - 500;

	if (millies > 3000)
		return true;  // finished


	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_BLEND);

	if (millies > 0)
	{
		RemoveDeadStars(millies);

		DrawStars(millies);

		DrawName(millies);

		InsertNewStars(millies);
	}

	glDisable(GL_BLEND);

	I_FinishFrame();
	I_StartFrame();

	return false;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
