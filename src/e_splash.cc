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


extern int I_GetMillies(void);


typedef struct
{
	float x, y;
	float t;
	float size;
	float r, g, b;
}
star_info_t;

static std::list<star_info_t> star_field;

#define STAR_ALPHA(t_diff)  (0.99 - (t_diff) / 3000.0)

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


static const char *edge_shape[7] =
{
	"                 ",
	" 3#7 ##1 3#7 3#7 ",
	" #   #9# #   #   ",
	" ##  # # # # ##  ",
	" #   #3# # # #   ",
	" 9#1 ##7 9## 9#1 ",
	"                 ",
};

static int edge_row_sizes[] =
{ 50, 30, 19, 19, 19, 30, 40 };

static int edge_column_sizes[] =
{
   5, 25,28,20, 6, 25,28,20, 6, 25,28,20, 6, 25,28,20, 5,
};

static void DrawName(int millies)
{
	int rows = 7;
	int cols = 17;

	glColor3f(0, 0, 0);

	int y1 = 0;
	int y2 = 0;

	for (int y = 0; y < rows; y++, y1 = y2)
	{
		y2 = y1 + edge_row_sizes[y];

		int x1 = 0;
		int x2 = 0;

		for (int x = 0; x < cols; x++, x1 = x2)
		{
			x2 = x1 + edge_column_sizes[x];

			char shape = edge_shape[rows-1-y][x];

			if (shape == '#')
				continue;

			int ix1 = x1 * SCREENWIDTH / 320;
			int ix2 = x2 * SCREENWIDTH / 320;

			int iy1 = y1 * SCREENHEIGHT / 200;
			int iy2 = y2 * SCREENHEIGHT / 200;

			glBegin(GL_POLYGON);

			switch (shape)
			{
				case '9':
					glVertex2i(ix1, iy1);
					glVertex2i(ix1, iy2);
					glVertex2i(ix2, iy1);
					break;

				case '7':
					glVertex2i(ix1, iy1);
					glVertex2i(ix2, iy2);
					glVertex2i(ix2, iy1);
					break;

				case '3':
					glVertex2i(ix1, iy1);
					glVertex2i(ix1, iy2);
					glVertex2i(ix2, iy2);
					break;

				case '1':
					glVertex2i(ix1, iy2);
					glVertex2i(ix2, iy2);
					glVertex2i(ix2, iy1);
					break;

				default:
					glVertex2i(ix1, iy1);
					glVertex2i(ix1, iy2);
					glVertex2i(ix2, iy2);
					glVertex2i(ix2, iy1);
					break;
			}

			glEnd();
		}
	}
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

	if (millies > 2740)
	{
		float f = (2000 - millies) / float(2000 - 740);
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

		st.r = pow((rand() & 0xFF) / 255.0, 0.3) / 2.5;
		st.g = pow((rand() & 0xFF) / 255.0, 0.3) / 1.5;
		st.b = pow((rand() & 0xFF) / 255.0, 0.3);

		star_field.push_back(st);
	}
}


bool E_DrawSplash(int millies)
{
	millies = millies - 500;

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_BLEND);

	if (millies > 0 && millies < 2200)
	{
		RemoveDeadStars(millies);

		DrawStars(millies);

		DrawName(millies);

		InsertNewStars(millies);
	}

	glDisable(GL_BLEND);

	I_FinishFrame();
	I_StartFrame();

	return (millies >= 2200);  // finished
}


void E_SplashScreen(void)
{
	int start_millies = I_GetMillies();

	for (;;)
	{
		int millies = I_GetMillies() - start_millies;

		if (E_DrawSplash(millies))
			break;

		// FIXME: abort on keyboard press
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
