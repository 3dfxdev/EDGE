//----------------------------------------------------------------------------
//  EDGE Status Bar Library Code
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

#include "i_defs.h"
#include "st_lib.h"

#include "dm_defs.h"
#include "dm_state.h"
#include "m_swap.h"
#include "st_stuff.h"
#include "r_local.h"
#include "v_colour.h"
#include "v_ctx.h"
#include "v_res.h"
#include "w_image.h"
#include "w_wad.h"
#include "z_zone.h"

void STLIB_Init(void)
{
	/* does nothing */
}

void STLIB_InitNum(st_number_t * n, int x, int y, 
				   const image_t ** digits, const image_t *minus, int *num, 
				   bool * on, int width)
{
	n->x = x;
	n->y = y;
	n->oldnum = 0;
	n->width = width;
	n->num = num;
	n->on = on;
	n->digits = digits;
	n->minus = minus;
	n->colmap = text_red_map;
}

void STLIB_InitFloat(st_float * n, int x, int y, 
					 const image_t ** digits, float *num, bool * on, int width)
{
	STLIB_InitNum(&n->num, x,y, digits,NULL, NULL, on, width);
	n->f = num;
}

#define DrawDigit(X,Y,Image,Map)  \
	vctx.DrawImage(FROM_320((X)-(Image)->offset_x), \
	FROM_200((Y)-(Image)->offset_y), \
	FROM_320(IM_WIDTH(Image)), FROM_200(IM_HEIGHT(Image)),  \
	(Image),0,0,IM_RIGHT(Image),IM_BOTTOM(Image),(Map),1.0)

static void DrawNum(st_number_t * n, bool refresh)
{
	int numdigits = n->width;
	int num = *n->num;
	int x;

	bool neg = false;

	n->oldnum = *n->num;

	// if non-number, do not draw it
	if (num == 1994)
		return;

	if (num < 0)
	{
		neg = true;

		num = -num;
		numdigits--;
	}

#if 0
	if (numdigits == 1 && num > 9)
		num = 9;
	else if (numdigits == 2 && num > 99)
		num = 99;
	else if (numdigits == 3 && num > 999)
		num = 999;
	else if (numdigits == 4 && num > 9999)
		num = 9999;
#endif

	x = n->x;

	// in the special case of 0, you draw 0
	if (num == 0)
	{
		x -= IM_WIDTH(n->digits[0]);
		DrawDigit(x, n->y, n->digits[0], n->colmap);
	}
	else
	{
		DEV_ASSERT2(num > 0);

		// draw the new number
		for (; num && (numdigits > 0); num /= 10, numdigits--)
		{
			x -= IM_WIDTH(n->digits[num % 10]);
			DrawDigit(x, n->y, n->digits[num % 10], n->colmap);
		}
	}

	if (neg && n->minus)
	{
		x -= IM_WIDTH(n->minus);
		DrawDigit(x, n->y, n->minus, n->colmap);
	}
}

void STLIB_UpdateNum(st_number_t * n, bool refresh)
{
	if (*n->on)
		DrawNum(n, refresh);
}

void STLIB_UpdateFloat(st_float * n, bool refresh)
{
	int i = *n->f;

	// HACK: Display 1 for numbers between 0 and 1. This is just because a
	// health of 0.3 otherwise would be displayed as 0%, which would make it
	// seem like you were a living dead.

	if (*n->f > 0 && *n->f < 1.0)
		i = 1;

	n->num.num = &i;
	STLIB_UpdateNum(&n->num, refresh);
	n->num.num = NULL;
}

void STLIB_InitPercent(st_percent_t * p, int x, int y, 
					   const image_t ** digits, const image_t *percsign,
					   float *num, bool * on)
{
	STLIB_InitFloat(&p->f, x, y, digits, num, on, 3);
	p->percsign = percsign;
}

void STLIB_UpdatePercent(st_percent_t * per, int refresh)
{
	st_number_t *num = &per->f.num;

	if (refresh && *num->on)
	{
		DrawDigit(num->x, num->y, per->percsign, num->colmap);
	}

	STLIB_UpdateFloat(&per->f, refresh);
}

void STLIB_InitMultIcon(st_multicon_t * i, int x, int y, 
						const image_t ** icons, int *inum, bool * on)
{
	i->x = x;
	i->y = y;
	i->oldinum = -1;
	i->inum = inum;
	i->on = on;
	i->icons = icons;
}

void STLIB_UpdateMultIcon(st_multicon_t * mi, bool refresh)
{
	const image_t *image;

	if (*mi->on && (mi->oldinum != *mi->inum || refresh)
		&& (*mi->inum != -1))
	{
		image = mi->icons[*mi->inum];

		VCTX_ImageEasy320(mi->x, mi->y, image);

		mi->oldinum = *mi->inum;
	}
}

void STLIB_InitBinIcon(st_binicon_t * b, int x, int y, 
					   const image_t * icon, bool * val, bool * on)
{
	b->x = x;
	b->y = y;
	b->oldval = 0;
	b->val = val;
	b->on = on;
	b->icon = icon;
}

void STLIB_UpdateBinIcon(st_binicon_t * bi, bool refresh)
{
	if (*bi->on && (bi->oldval != *bi->val || refresh))
	{
		if (*bi->val)
		{
			VCTX_ImageEasy320(bi->x, bi->y, bi->icon);
		}

		bi->oldval = *bi->val;
	}
}
