//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Wipes)
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
//
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#include "i_defs.h"
#include "i_defs_gl.h"

#include "m_random.h"
#include "rgl_defs.h"
#include "rgl_wipe.h"
#include "r_image.h"
#include "v_res.h"
#include "z_zone.h"

#include <math.h>

// we're limited to one wipe at a time...
static int cur_wipe_reverse = 0;
static wipetype_e cur_wipe_effect = WIPE_None;
static int cur_wipe_start;

static GLuint cur_wipe_tex = 0;

#define MELT_DIVS  128
static int melt_yoffs[MELT_DIVS+1];
static int melt_last_progress;


static GLuint SendWipeTexture(byte *rgb_src, int total_w, int total_h,
	bool use_alpha)
{
	GLuint id;

	DEV_ASSERT2(0 < total_w && total_w <= glmax_tex_size);
	DEV_ASSERT2(0 < total_h && total_h <= glmax_tex_size);

	glEnable(GL_TEXTURE_2D);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, use_alpha ? 4 : 3, total_w, total_h,
		0, use_alpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, rgb_src);

	glDisable(GL_TEXTURE_2D);

	return id;
}

static INLINE byte SpookyAlpha(int x, int y)
{
	y += (x & 32) / 2;

	x = (x & 31) - 15;
	y = (y & 31) - 15;

	return (x*x + y * y) / 2;
}

static GLuint CaptureScreenAsTexture(bool use_alpha, bool spooky)
{
	use_alpha = use_alpha || spooky;

	int total_w, total_h;
	int x, y, id;

	byte *pixels;
	byte *line_buf;

	total_w = W_MakeValidSize(SCREENWIDTH);
	total_h = W_MakeValidSize(SCREENHEIGHT);

	while (total_w > glmax_tex_size)
		total_w /= 2;

	while (total_h > glmax_tex_size)
		total_h /= 2;

	pixels = Z_New(byte, total_w * total_h * 4);

	line_buf = Z_New(byte, SCREENWIDTH * 4);

	// read pixels from screen, scaling down to target size which must
	// be both power-of-two and within the GL's tex_size limitation.

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	for (y=0; y < total_h; y++)
	{
		int px;
		int py = y * SCREENHEIGHT / total_h;

		glReadPixels(0, py, SCREENWIDTH, 1, GL_RGB, GL_UNSIGNED_BYTE, line_buf);

		int bp = use_alpha ? 4 : 3;
		byte i = rndtable[y & 0xff];

		for (x=0; x < total_w; x++)
		{
			byte *dest_p = pixels + ((y * total_w + x) * bp);

			px = x * SCREENWIDTH / total_w;

			dest_p[0] = line_buf[px*3 + 0];
			dest_p[1] = line_buf[px*3 + 1];
			dest_p[2] = line_buf[px*3 + 2];

			if (use_alpha)
			{
				if (spooky)
					dest_p[3] = SpookyAlpha(x, y);
				else
					dest_p[3] = rndtable[i++];
			}
		}
	}

	id = SendWipeTexture(pixels, total_w, total_h, use_alpha);

	Z_Free(line_buf);
	Z_Free(pixels);

	return id;
}

static void RGL_Init_Melt(void)
{
	int x, r;

	melt_last_progress = 0;

	melt_yoffs[0] = - (M_Random() % 16);

	for (x=1; x <= MELT_DIVS; x++)
	{
		r = (M_Random() % 3) - 1;

		melt_yoffs[x] = melt_yoffs[x-1] + r;
		melt_yoffs[x] = MAX(-15, MIN(0, melt_yoffs[x]));
	}
}

static void RGL_Update_Melt(int tics)
{
	int x, r;

	for (; tics > 0; tics--)
	{
		for (x=0; x <= MELT_DIVS; x++)
		{
			r = melt_yoffs[x];

			if (r < 0)
				r = 1;
			else if (r > 15)
				r = 8;
			else
				r += 1;

			melt_yoffs[x] += r;
		}
	}
}

//
// RGL_InitWipe
// 
void RGL_InitWipe(int reverse, wipetype_e effect)
{
	if (cur_wipe_effect == WIPE_None)
		return;

	cur_wipe_reverse = reverse;
	cur_wipe_effect  = effect;

	cur_wipe_start = -1;
	cur_wipe_tex = CaptureScreenAsTexture(effect == WIPE_Pixelfade,
		effect == WIPE_Spooky);

	if (cur_wipe_effect == WIPE_Melt)
		RGL_Init_Melt();
}

//
// RGL_StopWipe
// 
void RGL_StopWipe(void)
{
	cur_wipe_effect = WIPE_None;

	if (cur_wipe_tex != 0)
	{
		glDeleteTextures(1, &cur_wipe_tex);
		cur_wipe_tex = 0;
	}
}


//----------------------------------------------------------------------------

static void RGL_Wipe_Fading(float how_far)
{
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glBindTexture(GL_TEXTURE_2D, cur_wipe_tex);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f - how_far);

	glBegin(GL_QUADS);

	glTexCoord2f(0.0f, 0.0f);
	glVertex2i(0, 0);

	glTexCoord2f(0.0f, 1.0f);
	glVertex2i(0, SCREENHEIGHT);

	glTexCoord2f(1.0f, 1.0f);
	glVertex2i(SCREENWIDTH, SCREENHEIGHT);

	glTexCoord2f(1.0f, 0.0f);
	glVertex2i(SCREENWIDTH, 0);

	glEnd();

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
}

static void RGL_Wipe_Pixelfade(float how_far)
{
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);

	glAlphaFunc(GL_GEQUAL, how_far);

	glBindTexture(GL_TEXTURE_2D, cur_wipe_tex);
	glColor3f(1.0f, 1.0f, 1.0f);

	glBegin(GL_QUADS);

	glTexCoord2f(0.0f, 0.0f);
	glVertex2i(0, 0);

	glTexCoord2f(0.0f, 1.0f);
	glVertex2i(0, SCREENHEIGHT);

	glTexCoord2f(1.0f, 1.0f);
	glVertex2i(SCREENWIDTH, SCREENHEIGHT);

	glTexCoord2f(1.0f, 0.0f);
	glVertex2i(SCREENWIDTH, 0);

	glEnd();

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);

	glAlphaFunc(GL_GREATER, 1.0f / 32.0f);
}

static void RGL_Wipe_Melt(void)
{
	int x;

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glBindTexture(GL_TEXTURE_2D, cur_wipe_tex);
	glColor3f(1.0f, 1.0f, 1.0f);

	glBegin(GL_QUAD_STRIP);

	for (x=0; x <= MELT_DIVS; x++)
	{
		int yoffs = MAX(0, melt_yoffs[x]);

		float tx = (float) x / MELT_DIVS;
		float sx = (float) x * SCREENWIDTH / MELT_DIVS;
		float sy = (float) (200 - yoffs) * SCREENHEIGHT / 200.0f;

		glTexCoord2f(tx, 1.0f);
		glVertex2f(sx, sy);

		glTexCoord2f(tx, 0.0f);
		glVertex2f(sx, sy - SCREENHEIGHT);
	}

	glEnd();

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
}

static void RGL_Wipe_Slide(float how_far, float dx, float dy)
{
	dx *= how_far;
	dy *= how_far;

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glBindTexture(GL_TEXTURE_2D, cur_wipe_tex);
	glColor3f(1.0f, 1.0f, 1.0f);

	glBegin(GL_QUADS);

	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(dx, dy);

	glTexCoord2f(0.0f, 1.0f);
	glVertex2f(dx, dy + SCREENHEIGHT);

	glTexCoord2f(1.0f, 1.0f);
	glVertex2f(dx + SCREENWIDTH, dy + SCREENHEIGHT);

	glTexCoord2f(1.0f, 0.0f);
	glVertex2f(dx + SCREENWIDTH, dy);

	glEnd();

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
}

static void RGL_Wipe_Doors(float how_far)
{
	float dx = cos(how_far * M_PI / 2) * (SCREENWIDTH/2);
	float dy = sin(how_far * M_PI / 2) * (SCREENHEIGHT/3);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glBindTexture(GL_TEXTURE_2D, cur_wipe_tex);
	glColor3f(1.0f, 1.0f, 1.0f);

	for (int column = 0; column < 5; column++)
	{
		float c = column / 10.0f;
		float e = column / 5.0f;

		for (int side = 0; side < 2; side++)
		{
			float t_x1 = (side == 0) ? c : (0.9f - c);
			float t_x2 = t_x1 + 0.1f;

			float v_x1 = (side == 0) ? (dx * e) : (SCREENWIDTH - dx * (e + 0.2f));
			float v_x2 = v_x1 + dx * 0.2f;

			float v_y1 = (side == 0) ? (dy * e) : (dy * (e + 0.2f));
			float v_y2 = (side == 1) ? (dy * e) : (dy * (e + 0.2f));

			glBegin(GL_QUAD_STRIP);

			for (int row = 0; row <= 5; row++)
			{
				float t_y = row / 5.0f;

				float j1 = (SCREENHEIGHT - v_y1 * 2.0f) / 5.0f;
				float j2 = (SCREENHEIGHT - v_y2 * 2.0f) / 5.0f;

				glTexCoord2f(t_x2, t_y); glVertex2f(v_x2, v_y2 + j2 * row);
				glTexCoord2f(t_x1, t_y); glVertex2f(v_x1, v_y1 + j1 * row);
			}

			glEnd();
		}
	}

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
}

//
// RGL_DoWipe
//
// NOTE: we assume 2D project matrix is already setup.
//
bool RGL_DoWipe(void)
{
	int progress;
	float how_far;

	if (cur_wipe_effect == WIPE_None || cur_wipe_tex == 0)
		return true;

	// determine how many tics since we started.  If this is the first
	// call to DoWipe() since InitWipe(), then the clock starts now.
	// 
	progress = I_GetTime();

	if (cur_wipe_start < 0)
		cur_wipe_start = progress;

	progress = MAX(0, progress - cur_wipe_start);

	if (progress > 40)  // FIXME: have option for wipe speed
		return true;

	how_far = (float) progress / 40.0f;

	switch (cur_wipe_effect)
	{
		case WIPE_Melt:
			RGL_Wipe_Melt();
			RGL_Update_Melt(progress - melt_last_progress);
			melt_last_progress = progress;
			break;

		case WIPE_Top:
			RGL_Wipe_Slide(how_far, 0, +SCREENHEIGHT);
			break;

		case WIPE_Bottom:
			RGL_Wipe_Slide(how_far, 0, -SCREENHEIGHT);
			break;

		case WIPE_Left:
			RGL_Wipe_Slide(how_far, -SCREENWIDTH, 0);
			break;

		case WIPE_Right:
			RGL_Wipe_Slide(how_far, +SCREENWIDTH, 0);
			break;


		case WIPE_Doors:
			RGL_Wipe_Doors(how_far);
			break;

		case WIPE_Spooky:  // difference is in alpha channel
		case WIPE_Pixelfade:
			RGL_Wipe_Pixelfade(how_far);
			break;

		case WIPE_Crossfade:
		default:
			RGL_Wipe_Fading(how_far);
			break;
	}

	return false;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
