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
#include "r_texgl.h"


extern int I_GetMillies(void);

extern unsigned char rndtable[256];


typedef struct
{
	float x, y;
	float t;
	float size;
	float r, g, b;
}
star_info_t;

static std::list<star_info_t> star_field;

#define STAR_ALPHA(t_diff)  (0.99 - (t_diff) /  7000.0)

#define STAR_Z(t_diff)  (0.3 + (t_diff) /  100.0)

static GLuint star_tex;


static void CreateStarTex(void)
{
	epi::image_data_c *img = new epi::image_data_c(128, 128);

	for (int y = 0; y < 128; y++)
	for (int x = 0; x < 128; x++)
	{
		u8_t *pix = img->PixelAt(x, y);

		float dist = sqrt((x-64)*(x-64) + (y-64)*(y-64));

		angle_t ang = R_PointToAngle(64.5, 64.5, x, y);

		int rnd = rndtable[(u32_t)ang >> 24];

		*pix = CLAMP(0, 255 - dist * 4.2, 255) * (rnd / 512.0 + 0.5);

		pix[1] = pix[2] = pix[0];
	}

	star_tex = R_UploadTexture(img, UPL_Smooth);

	delete img;
}


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

	glTexCoord2f(0.0, 0.0); glVertex2f(x - sz, y - sz);
	glTexCoord2f(0.0, 1.0); glVertex2f(x - sz, y + sz);
	glTexCoord2f(1.0, 1.0); glVertex2f(x + sz, y + sz);
	glTexCoord2f(1.0, 0.0); glVertex2f(x + sz, y - sz);

	glEnd();
}

static void DrawStars(int millies)
{
	// additive blending
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, star_tex);

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

	glDisable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


static const char *edge_shape[7] =
{
	"                 ",
	" 3#7 ##1 3#7 3#7 ",
	" #   #8# #   #   ",
	" #7  # # # 1 #7  ",
	" #   #2# #2# #   ",
	" 9#1 ##7 9## 9#1 ",
	"                 ",
};

static int edge_row_sizes[] =
{ 54, 29, 17, 17, 17, 29, 44 };

static int edge_column_sizes[] =
{
   9, 24,29,18, 6, 24,29,18, 6, 24,29,18, 6, 24,29,18, 9,
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

			int hx = (ix1 + ix2) / 2;
			int hy = (iy1 + iy2) / 2;

			int ix3 = ix1 + (ix2 - ix1) / 7;
			int ix4 = ix2 - (ix2 - ix1) / 7;

			int iy3 = iy1 + (iy2 - iy1) / 7;
			int iy4 = iy2 - (iy2 - iy1) / 7;

			switch (shape)
			{
				case '9':
					glBegin(GL_TRIANGLE_FAN);
					glVertex2i(ix1, iy1);
					glVertex2i(ix2, iy1);
					glVertex2i(hx,  iy3);
					glVertex2i(ix3, hy);
					glVertex2i(ix1, iy2);
					glEnd();
					break;

				case '7':
					glBegin(GL_TRIANGLE_FAN);
					glVertex2i(ix2, iy1);
					glVertex2i(ix1, iy1);
					glVertex2i(hx,  iy3);
					glVertex2i(ix4, hy);
					glVertex2i(ix2, iy2);
					glEnd();
					break;

				case '3':
					glBegin(GL_TRIANGLE_FAN);
					glVertex2i(ix1, iy2);
					glVertex2i(ix2, iy2);
					glVertex2i(hx,  iy4);
					glVertex2i(ix3, hy);
					glVertex2i(ix1, iy1);
					glEnd();
					break;

				case '1':
					glBegin(GL_TRIANGLE_FAN);
					glVertex2i(ix2, iy2);
					glVertex2i(ix2, iy1);
					glVertex2i(ix4, hy);
					glVertex2i(hx,  iy4);
					glVertex2i(ix1, iy2);
					glEnd();
					break;

				case '8':
					glBegin(GL_POLYGON);
					glVertex2i(ix2, iy1);
					glVertex2i(ix1, iy1);
					glVertex2i(ix1, iy2);
					glVertex2f(hx,  iy4);
					glVertex2f(ix4, hy);
					glEnd();
					break;

				case '2':
					glBegin(GL_POLYGON);
					glVertex2i(ix1, iy1);
					glVertex2i(ix1, iy2);
					glVertex2i(ix2, iy2);
					glVertex2f(ix4, hy);
					glVertex2f(hx,  iy3);
					glEnd();
					break;

				default:
					glBegin(GL_POLYGON);
					glVertex2i(ix1, iy1);
					glVertex2i(ix1, iy2);
					glVertex2i(ix2, iy2);
					glVertex2i(ix2, iy1);
					glEnd();
					break;
			}
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

	if (millies > 7740)
	{
		float f = (2800 - millies) / float(2800 - 1740);
		total = millies + int(millies * f);
	}

	total = total * 4 / 4;

	for (; all_count < total; all_count++)
	{
		star_info_t st;

		st.x = (rand() & 0x0FFF) / 12.80f;
		st.y = (rand() & 0x0FFF) / 20.48f;

		st.t = millies;

		st.size = ((rand() & 0x0FFF) / 4096.0 + 1) * 16000.0;

		st.r = pow((rand() & 0xFF) / 255.0, 0.3) / 2.0;
		st.g = pow((rand() & 0xFF) / 255.0, 0.3) / 1.0;
		st.b = pow((rand() & 0xFF) / 255.0, 0.3) / 1.0;

		// st.g = st.b = st.r;

		star_field.push_back(st);
	}
}


bool E_DrawSplash(int millies)
{
	int max_time = 3200;

	millies = millies - 500;

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_BLEND);

	if (millies > 0 && millies < max_time)
	{
		RemoveDeadStars(millies);

		DrawStars(millies);

  		DrawName(millies);

		InsertNewStars(millies);
	}

	glDisable(GL_BLEND);

	I_FinishFrame();
	I_StartFrame();

	return (millies >= max_time);  // finished
}


void E_SplashScreen(void)
{
	CreateStarTex();

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
