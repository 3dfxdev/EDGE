//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Wipes)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2001  The EDGE Team.
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

// this conditional applies to the whole file
#ifdef USE_GL

#include "i_defs.h"

#include "m_random.h"
#include "r2_defs.h"
#include "rgl_defs.h"
#include "v_colour.h"
#include "v_res.h"
#include "w_image.h"
#include "w_wad.h"
#include "wp_main.h"
#include "z_zone.h"


// we're limited to one wipe at a time...
static int cur_wipe_reverse = 0;
static int cur_wipe_effect  = WIPE_None;
static int cur_wipe_start;

static GLuint cur_wipe_tex  = 0;

#define MELT_DIVS  128
static int melt_yoffs[MELT_DIVS+1];
static int melt_last_progress;


static GLuint SendWipeTexture(byte *rgb_src, int total_w, int total_h)
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

	glTexImage2D(GL_TEXTURE_2D, 0, 3, total_w, total_h,
		0, GL_RGB, GL_UNSIGNED_BYTE, rgb_src);

	glDisable(GL_TEXTURE_2D);

	return id;
}

static GLuint CaptureScreenAsTexture(void)
{
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

	pixels = Z_New(byte, total_w * total_h * 3);

	line_buf = Z_New(byte, SCREENWIDTH * 3);

	// read pixels from screen, scaling down to target size which must
	// be both power-of-two and within the GL's tex_size limitation.

	glReadBuffer(GL_FRONT);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	for (y=0; y < total_h; y++)
	{
		int px;
		int py = y * SCREENHEIGHT / total_h;

		glReadPixels(0, py, SCREENWIDTH, 1, GL_RGB, GL_UNSIGNED_BYTE, line_buf);

		for (x=0; x < total_w; x++)
		{
			byte *dest_p = pixels + ((y * total_w + x) * 3);

			px = x * SCREENWIDTH / total_w;

			dest_p[0] = line_buf[px*3 + 0];
			dest_p[1] = line_buf[px*3 + 1];
			dest_p[2] = line_buf[px*3 + 2];
		}
	}

	id = SendWipeTexture(pixels, total_w, total_h);

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
void RGL_InitWipe(int reverse, int effect)
{
	cur_wipe_reverse = reverse;
	cur_wipe_effect  = effect;

	cur_wipe_start = -1;
	cur_wipe_tex = CaptureScreenAsTexture();

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
	}
}


//----------------------------------------------------------------------------

static void RGL_Wipe_Fading(float how_far)
{
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glBindTexture(GL_TEXTURE_2D, cur_wipe_tex);
	glColor4f(1.0, 1.0, 1.0, 1.0 - how_far);

	glBegin(GL_QUADS);

	glTexCoord2f(0.0, 0.0);
	glVertex2i(0, 0);

	glTexCoord2f(0.0, 1.0);
	glVertex2i(0, SCREENHEIGHT);

	glTexCoord2f(1.0, 1.0);
	glVertex2i(SCREENWIDTH, SCREENHEIGHT);

	glTexCoord2f(1.0, 0.0);
	glVertex2i(SCREENWIDTH, 0);

	glEnd();

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
}

static void RGL_Wipe_Melt(void)
{
	int x;

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glBindTexture(GL_TEXTURE_2D, cur_wipe_tex);
	glColor3f(1.0, 1.0, 1.0);

	glBegin(GL_QUAD_STRIP);

	for (x=0; x <= MELT_DIVS; x++)
	{
		int yoffs = MAX(0, melt_yoffs[x]);

		float tx = (float) x / MELT_DIVS;
		float sx = (float) x * SCREENWIDTH / MELT_DIVS;
		float sy = (float) (200 - yoffs) * SCREENHEIGHT / 200.0f;

		glTexCoord2f(tx, 1.0);
		glVertex2f(sx, sy);

		glTexCoord2f(tx, 0.0);
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
	glColor3f(1.0, 1.0, 1.0);

	glBegin(GL_QUADS);

	glTexCoord2f(0.0, 0.0);
	glVertex2f(dx, dy);

	glTexCoord2f(0.0, 1.0);
	glVertex2f(dx, dy + SCREENHEIGHT);

	glTexCoord2f(1.0, 1.0);
	glVertex2f(dx + SCREENWIDTH, dy + SCREENHEIGHT);

	glTexCoord2f(1.0, 0.0);
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
	glColor3f(1.0, 1.0, 1.0);

	// left side
	glBegin(GL_QUADS);

	glTexCoord2f(0.0, 0.0);
	glVertex2f(0, 0);

	glTexCoord2f(0.0, 1.0);
	glVertex2f(0, SCREENHEIGHT);

	glTexCoord2f(0.5, 1.0);
	glVertex2f(dx, SCREENHEIGHT - dy);

	glTexCoord2f(0.5, 0.0);
	glVertex2f(dx, dy);

	glEnd();

	// right side
	glBegin(GL_QUADS);

	glTexCoord2f(0.5, 0.0);
	glVertex2f(SCREENWIDTH - dx, dy);

	glTexCoord2f(0.5, 1.0);
	glVertex2f(SCREENWIDTH - dx, SCREENHEIGHT - dy);

	glTexCoord2f(1.0, 1.0);
	glVertex2f(SCREENWIDTH, SCREENHEIGHT);

	glTexCoord2f(1.0, 0.0);
	glVertex2f(SCREENWIDTH, 0);

	glEnd();

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

	if (progress > 40)
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

		case WIPE_Corners:  // Fixme
		case WIPE_Right:
			RGL_Wipe_Slide(how_far, +SCREENWIDTH, 0);
			break;

		case WIPE_Doors:
			RGL_Wipe_Doors(how_far);
			break;

		case WIPE_Crossfade:
		case WIPE_Pixelfade:
		default:
			RGL_Wipe_Fading(how_far);
			break;
	}

	return false;
}


#endif  // USE_GL
