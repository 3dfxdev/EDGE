//----------------------------------------------------------------------------
//  EDGE Wipe Functions
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

#include "i_defs.h"

#include "dm_state.h"
#include "m_inline.h"
#include "m_random.h"
#include "v_res.h"
#include "v_colour.h"
#include "wp_main.h"
#include "z_zone.h"

#ifndef NOHICOLOUR
// -ES- 1999/06/14 Unrolled loop and changed algorithm
static void DoWipe_Crossfade16(wipeparm_t * wp)
{
	unsigned long *d;
	unsigned long *e;
	unsigned long *s;

	int height = wp->dest->height;
	int width = wp->dest->width;
	int level = (int)(wp->progress * 64.0f + 0.5f);

	unsigned long (*s2rgb)[2], (*e2rgb)[2];
	unsigned long c1, c2;  // source colours

	unsigned long d1, d2;  // destination colours

	int y;
	int i;

	s2rgb = col2rgb16[64 - level];
	e2rgb = col2rgb16[level];

	for (y = 0; y < height; y++)
	{
		d = (unsigned long *)(wp->dest->data + y * wp->dest->pitch + width * 2);
		e = (unsigned long *)(wp->end->data + y * wp->end->pitch + width * 2);
		s = (unsigned long *)(wp->start->data + y * wp->start->pitch + width * 2);

		for (i = (-width) >> 1; i < 0; i++)
		{
			// -ES- 1998/11/29 Use the new algorithm
			c1 = s[i];
			c2 = e[i];

#ifdef __BIG_ENDIAN__
			d1 = s2rgb[(unsigned char)(c1 >> 8)][0] +
				s2rgb[(unsigned char)c1][1] +
				e2rgb[(unsigned char)(c2 >> 8)][0] +
				e2rgb[(unsigned char)c2][1];
			c1 >>= 16;
			c2 >>= 16;
			d2 = s2rgb[(unsigned char)(c1 >> 8)][0] +
				s2rgb[(unsigned char)c1][1] +
				e2rgb[(unsigned char)(c2 >> 8)][0] +
				e2rgb[(unsigned char)c2][1];
			d1 |= hicolourtransmask;
			d2 |= hicolourtransmask;
#else
			d1 = s2rgb[(unsigned char)c1][0] +
				s2rgb[(unsigned char)(c1 >> 8)][1] +
				e2rgb[(unsigned char)c2][0] +
				e2rgb[(unsigned char)(c2 >> 8)][1];
			c1 >>= 16;
			c2 >>= 16;
			d2 = s2rgb[(unsigned char)c1][0] +
				s2rgb[(unsigned char)(c1 >> 8)][1] +
				e2rgb[(unsigned char)c2][0] +
				e2rgb[(unsigned char)(c2 >> 8)][1];
			d1 |= hicolourtransmask;
			d2 |= hicolourtransmask;
#endif

			d[i] = (d1 & (d1 >> 16)) | (d2 & (d2 << 16));
		}
	}

}
#endif

// -ES- 1999/06/14 Changed col2rgb format
static void DoWipe_Crossfade8(wipeparm_t * wp)
{
	byte *d;
	byte *e;
	byte *s;
	int y;
	int i;
	int height = wp->dest->height;
	int width = wp->dest->width;
	int level = (int)(wp->progress * 64.0f + 0.5f);

	unsigned long *s2rgb, *e2rgb;
	unsigned long c;

	s2rgb = col2rgb8[64 - level];
	e2rgb = col2rgb8[level];

	for (y = 0; y < height; y++)
	{
		d = wp->dest->data + y * wp->dest->pitch + width;
		e = wp->end->data + y * wp->end->pitch + width;
		s = wp->start->data + y * wp->start->pitch + width;
		for (i = -width; i < 0; i++)
		{
			c = (s2rgb[s[i]] + e2rgb[e[i]]) | 0x07c1Fc1F;
			d[i] = rgb_32k[0][0][c & (c >> 17)];
		}
	}
}

static void DoWipe_Crossfade(wipeparm_t * wp)
{
#ifndef NOHICOLOUR
	// Move to v_res.c?
	if (BPP == 1)
		DoWipe_Crossfade8(wp);
	else
		DoWipe_Crossfade16(wp);
#else
	DoWipe_Crossfade8(wp);
#endif
}

static void Wipe_Pixelfade8(byte * src, byte * dest, int offs, byte i, int w, int progress)
{
	int x;
	int offset[2];

	offset[0] = 0;
	offset[1] = offs;

	dest += w;
	src += w;
	for (x = -w; x < 0; x++)
	{
		dest[x] = src[x + offset[(progress + rndtable[i++]) >> 8]];
	}
}

static void Wipe_Pixelfade16(byte * src, byte * dest, int offs, byte i, int w, int progress)
{
	int x;
	int offset[2];

	offset[0] = 0;
	offset[1] = offs;

	dest += w * 2;
	src += w * 2;

	for (x = -w; x < 0; x++)
	{
		*(short *)(dest + 2 * x) = *(short *)(src + 2 * x + offset[(progress + rndtable[i++]) >> 8]);
	}
}

static void DoWipe_Pixelfade(wipeparm_t * wp)
{
	int y, progress;
	byte *src;
	byte *dest;
	int width = wp->dest->width;
	int dpitch = wp->dest->pitch;
	int spitch = wp->start->pitch;
	int epitch = wp->end->pitch;
	int offs;
	void (*fadefunc) (byte * src, byte * dest, int offs, byte i, int w, int progress);

#ifndef NOHICOLOUR
	fadefunc = BPP == 1 ? Wipe_Pixelfade8 : Wipe_Pixelfade16;
#else
	fadefunc = Wipe_Pixelfade8;
#endif

	progress = (int)(wp->progress * 256.0f);

	src = wp->start->data;
	offs = wp->end->data - src;
	dest = wp->dest->data;
	for (y = 0; y < wp->dest->height; y++)
	{
		fadefunc(src, dest, offs, rndtable[(byte) y], width, progress);

		src += spitch;
		offs += epitch - spitch;
		dest += dpitch;
	}
}

typedef struct
{
	float max;
	float yoffs[1];
}
meltdata_t;

/*
   Melting

   yoffs contains an array of randomised values, telling the relative y offset
   of each of the end columns.

   This copies two pixels at a time, and will thereby work in 16-bit modes too.
 */
static void DoWipe_Melt(wipeparm_t * wp)
{
	int y, h, x;
	meltdata_t *data = (meltdata_t*)wp->data;
	float *yoffs;
	float extray;
	int bpp = wp->dest->bytepp;
	int width = wp->dest->width / 2 * bpp;
	int height = wp->dest->height;
	int skiprows;
	short *d;
	short *s;
	int dpitch = wp->dest->pitch / 2;
	int spitch = wp->start->pitch / 2;
	int epitch = wp->end->pitch / 2;

	yoffs = data->yoffs;

	extray = wp->progress * height + data->max;

	// optimisation: the top rectangle of end screen can be copied right away
	// (CopyRect is faster than the wipe inner loops)
	skiprows = (int)(extray - data->max);
	if (skiprows <= 0)
		skiprows = 0;
	else
		V_CopyRect(wp->dest, wp->end, 0, 0, wp->dest->width, skiprows, 0, 0);

	for (x = 0; x < width; x++)
	{
		h = (int)(extray - yoffs[x]);
		if (h > height)
			h = height;

		d = ((short *)wp->dest->data + skiprows * dpitch) + x;
		s = ((short *)wp->end->data + skiprows * epitch) + x;
		for (y = skiprows; y < h; y++)
		{
			*d = *s;
			d += dpitch;
			s += epitch;
		}
		s = ((short *)wp->start->data) + x;
		for (; y < height; y++)
		{
			*d = *s;
			d += dpitch;
			s += spitch;
		}
	}
}

static void InitData_Melt(wipeparm_t * wp)
{
	float *yoffs;
	int width = wp->dest->width / 2 * wp->dest->bytepp;
	meltdata_t *data;
	int x;
	float r;
	float max;
	float highest, lowest;
	float scale = (float)(((3.0f * 8.0f * 320.0f / 200.0f / 256.0f) / wp->dest->bytepp) * wp->dest->height / wp->dest->width);

	wp->data = data = (meltdata_t *) Z_Malloc(sizeof(meltdata_t) + (width - 1) * sizeof(float));

	yoffs = data->yoffs;
	max = (float)(wp->dest->height * (16.0f * 8.0f / 200.0f));

	// keep track of top and bottom of the array
	highest = 0;
	lowest = max;

	r = M_Random() * max / 256;

	for (x = 0; x < width; x++)
	{
		r += (M_Random() - 128) * scale;
		if (r > max)
			r = max;
		if (r < 0)
			r = 0;

		if (r > highest)
			highest = r;
		if (r < lowest)
			lowest = r;

		yoffs[x] = r;
	}

	// clean up: the lowest one should be at zero
	for (x = 0; x < width; x++)
	{
		yoffs[x] -= lowest;
	}

	data->max = highest - lowest;
}

static void DestroyData_ZFree(wipeparm_t * wp)
{
	Z_Free(wp->data);
}

static int DefDuration_Doors(wipeinfo_t * wi)
{
	return 35;
}

static int DefDuration_Fade(wipeinfo_t * wi)
{
	return 35;
}

static int DefDuration_Melt(wipeinfo_t * wi)
{
	// in original DOOM, screen melted down at a speed of 8 pixels per tic, so
	// in 320x200 the wiping took 200/8=25 tics.
	return 5 + 25 * wi->height / 200;
}
static int DefDuration_VScroll(wipeinfo_t * wi)
{
	return 35;
}
static int DefDuration_HScroll(wipeinfo_t * wi)
{
	return 35;
}

static int DefDuration_None(wipeinfo_t * wi)
{
	return 0;
}

static void DoWipe_Top(wipeparm_t * wp)
{
	int y, h;

	y = (int)(wp->progress * wp->dest->height);
	h = wp->dest->height - y;

	V_CopyRect(wp->dest, wp->end, 0, h, wp->end->width, y, 0, 0);
	// optimisation: the start screen might already be there
	if (wp->dest->data != wp->start->data)
		V_CopyRect(wp->dest, wp->start, 0, y, wp->end->width, h, 0, y);
}
static void DoWipe_Bottom(wipeparm_t * wp)
{
	int y, h;

	y = (int)(wp->progress * wp->dest->height);
	h = (int)(wp->dest->height - y);

	V_CopyRect(wp->dest, wp->end, 0, 0, wp->end->width, y, 0, h);
	// optimisation: the start screen might already be there
	if (wp->dest->data != wp->start->data)
		V_CopyRect(wp->dest, wp->start, 0, 0, wp->end->width, h, 0, 0);
}

static void DoWipe_Left(wipeparm_t * wp)
{
	int x, w;

	x = (int)(wp->progress * wp->dest->width);
	w = (int)(wp->dest->width - x);

	V_CopyRect(wp->dest, wp->end, w, 0, x, wp->end->height, 0, 0);
	// optimisation: the start screen might already be there
	if (wp->dest->data != wp->start->data)
		V_CopyRect(wp->dest, wp->start, x, 0, w, wp->end->height, x, 0);
}

static void DoWipe_Right(wipeparm_t * wp)
{
	int x, w;

	x = (int)(wp->progress * wp->dest->width);
	w = (int)(wp->dest->width - x);

	V_CopyRect(wp->dest, wp->end, 0, 0, x, wp->end->height, w, 0);
	// optimisation: the start screen might already be there
	if (wp->dest->data != wp->start->data)
		V_CopyRect(wp->dest, wp->start, 0, 0, w, wp->end->height, 0, 0);
}

static void DoWipe_Corners(wipeparm_t * wp)
{
	int x, w, y, h;

	w = (int)(wp->progress * wp->dest->width / 2.0f);
	x = wp->dest->width;
	h = (int)(wp->progress * wp->dest->height / 2.0f);
	y = wp->dest->height;

	V_CopyRect(wp->dest, wp->end, x / 2 - w, y / 2 - h, w, h, 0, 0);
	V_CopyRect(wp->dest, wp->start, w, 0, x - 2 * w, h, w, 0);
	V_CopyRect(wp->dest, wp->end, x / 2, y / 2 - h, w, h, x - w, 0);

	V_CopyRect(wp->dest, wp->start, 0, h, x, y - 2 * h, 0, h);

	V_CopyRect(wp->dest, wp->end, x / 2 - w, y / 2, w, h, 0, y - h);
	V_CopyRect(wp->dest, wp->start, w, y - h, x - 2 * w, h, w, y - h);
	V_CopyRect(wp->dest, wp->end, x / 2, y / 2, w, h, x - w, y - h);
}

// doordata is an array where index i contains start->pitch*i.
typedef int *doordata_t;
static void InitData_Doors(wipeparm_t * wp)
{
	doordata_t data;
	int h = wp->start->height;
	int d = wp->start->pitch;
	int i;
	int v;

	data = Z_New(int, h);

	for (i = 0, v = 0; i < h; i++, v += d)
		data[i] = v;

	wp->data = data;
}

#ifndef NOHICOLOUR
static void DrawDoorCol16(screen_t * dest, int destx, screen_t * src, int srcx, int y, int h, fixed_t yscale, doordata_t yoffs)
{
	byte *d;
	byte *s;
	int i;
	int srcy;
	int dpitch = dest->pitch;

	d = dest->data + y * dpitch + destx * 2;
	s = src->data + srcx * 2;

	srcy = src->height * FRACUNIT / 2 - yscale * h / 2;  // + FRACUNIT/2;

	for (i = 0; i < h; i++)
	{
		*(short *)d = *(short *)(s + yoffs[srcy >> FRACBITS]);
		srcy += yscale;
		d += dpitch;
	}
}
#endif
static void DrawDoorCol8(screen_t * dest, int destx, screen_t * src, int srcx, int y, int h, fixed_t yscale, doordata_t yoffs)
{
	byte *d;
	byte *s;
	int i;
	int srcy;
	int dpitch = dest->pitch;

	d = dest->data + y * dpitch + destx;
	s = src->data + srcx;

	srcy = src->height * FRACUNIT / 2 - yscale * h / 2;

	for (i = 0; i < h; i++)
	{
		*d = s[yoffs[srcy >> FRACBITS]];
		srcy += yscale;
		d += dpitch;
	}
}

static int GetDoorCol(int width, int x, fixed_t sinval, fixed_t cosval)
{
	// distance to door is width: FOV=90
#define DOORDIST width
	fixed_t retval;

	// the *2 and +1)/2 rounds retval instead of truncating it
	retval = FixedDiv(2 * DOORDIST * x, (width - x) * sinval + DOORDIST * cosval);
	retval = (retval + 1) / 2;
	if (retval > width)
		return -1;
	return retval;
}

static fixed_t GetDoorScale(int width, int x, fixed_t sinval, fixed_t cosval)
{
	return FixedDiv(DOORDIST * cosval + width * sinval, DOORDIST * cosval + (width - x) * sinval);
#undef DOORDIST
}

static void DoWipe_Doors(wipeparm_t * wp)
{
	int srcx, destx;
	fixed_t yscale;
	int width = wp->dest->width;
	int height = wp->dest->height;
	int h;
	angle_t angle;
	fixed_t sinval;
	fixed_t cosval;

	void (*DrawDoorCol) (screen_t * dest, int destx, screen_t * src, int srcx, int y, int h, fixed_t yscale, doordata_t yoffs);

#ifndef NOHICOLOUR
	DrawDoorCol = BPP == 1 ? DrawDoorCol8 : DrawDoorCol16;
#else
	DrawDoorCol = DrawDoorCol8;
#endif

	// optimise me
	V_CopyScreen(wp->dest, wp->start);

	// max angle is a bit lower than 135 degrees
	angle = (angle_t)((1.0f - wp->progress) * (ANG90 + ANG45 - ANG1));

	sinval = M_FloatToFixed(M_Sin(angle));
	cosval = M_FloatToFixed(M_Cos(angle));

	// left door
	for (destx = 0; destx < width / 2; destx++)
	{
		srcx = GetDoorCol(width / 2, destx, sinval, cosval);
		if (srcx == -1)
			break;

		yscale = GetDoorScale(width / 2, destx, sinval, cosval);
		h = FixedDiv(height, yscale);

		DrawDoorCol(wp->dest, destx, wp->end, srcx, (height - h) / 2, h, yscale, (doordata_t)wp->data);
	}
	// right door
	for (destx = 0; destx < width / 2; destx++)
	{
		srcx = GetDoorCol(width / 2, destx, sinval, cosval);
		if (srcx == -1)
			break;

		yscale = GetDoorScale(width / 2, destx, sinval, cosval);
		h = FixedDiv(height, yscale);

		DrawDoorCol(wp->dest, width - destx - 1, wp->end, width - srcx - 1, (height - h) / 2, h, yscale, (doordata_t)wp->data);
	}
}

static void DoWipe_None(wipeparm_t * wp)
{
}

wipetype_t wipes[WIPE_NUMWIPES] =
{
	// None
	{1, 1, NULL, NULL, DefDuration_None, DoWipe_None},
	// Melt down
	{0, 1, InitData_Melt, DestroyData_ZFree, DefDuration_Melt, DoWipe_Melt},
	// Crossfade
	{1, 1, NULL, NULL, DefDuration_Fade, DoWipe_Crossfade},
	// Pixelfade
	{1, 1, NULL, NULL, DefDuration_Fade, DoWipe_Pixelfade},
	// scrolls in from top
	{1, 0, NULL, NULL, DefDuration_VScroll, DoWipe_Top},
	// scrolls in from bottom
	{1, 0, NULL, NULL, DefDuration_VScroll, DoWipe_Bottom},
	// scrolls in from left
	{1, 0, NULL, NULL, DefDuration_HScroll, DoWipe_Left},
	// scrolls in from right
	{1, 0, NULL, NULL, DefDuration_HScroll, DoWipe_Right},
	// scrolls in from all corners
	{1, 0, NULL, NULL, DefDuration_HScroll, DoWipe_Corners},
	// opening doors
	{0, 0, InitData_Doors, DestroyData_ZFree, DefDuration_Doors, DoWipe_Doors}
};

// for CVar enums
const char WIPE_EnumStr[] = "none/melt/crossfade/pixelfade/top/bottom/left/right/corners/doors";
