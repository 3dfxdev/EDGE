//----------------------------------------------------------------------------
//  EDGE Video Context
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2008  The EDGE Team.
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

#ifndef __R_DRAW_H__
#define __R_DRAW_H__

#include "r_image.h"

// We are referring to patches.
#include "hu_font.h"
#include "hu_style.h"


void HUD_SetCoordSys(int width, int height);

void HUD_SetFont(font_c *font);
void HUD_SetTextScale(float scale);
void HUD_SetTextColor(rgbcol_t color);
void HUD_SetAlpha(float alpha);

void HUD_Reset(const char *what = "fsca");
// letters can be:
// 	 'f' for Font   	's' for Scale
// 	 'c' for Color		'a' for Alpha


#define HU_MAXLINES		4
#define HU_MAXLINELENGTH	80

void RGL_DrawImage(float x, float y, float w, float h, const image_c *image,
				   float tx1, float ty1, float tx2, float ty2,
				   float alpha = 1.0f, rgbcol_t text_col = RGB_NO_VALUE,
				   const colourmap_c *palremap = NULL);
 
// Draw a solid colour box (possibly translucent) in the given
// rectangle.  Coordinates are inclusive.  Alpha ranges from 0
// (invisible) to 255 (totally opaque).  Colour is a palette index
// (0-255).  Drawing will be clipped to the current clipping
// rectangle.
void RGL_SolidBox(int x, int y, int w, int h, rgbcol_t col, float alpha = 1.0);

void HUD_SolidBox(float x, float y, float w, float h, rgbcol_t col);

// Draw a solid colour line (possibly translucent) between the two
// end points.  Coordinates are inclusive.  Used for the automap.
// Colour is a palette index (0-255).  Drawing will be clipped to
// the current clipping rectangle.
void RGL_SolidLine(int x1, int y1, int x2, int y2, rgbcol_t col, float alpha = 1.0);

void HUD_SolidLine(float x1, float y1, float x2, float y2, rgbcol_t col);

// Draw a thin outline of a box.
void RGL_ThinBox(int x, int y, int w, int h, rgbcol_t col, float alpha = 1.0);

// Like RGL_SolidBox but the colors of each corner (TL, BL, TR, BR) can
// be specified individually.
void RGL_GradientBox(int x, int y, int w, int h, rgbcol_t *cols, float alpha = 1.0);

// Convenience functions
void RGL_Image(float x, float y, float w, float h, const image_c *image);
void RGL_Image320(float x, float y, float w, float h, const image_c *image);
void RGL_ImageEasy320(float x, float y, const image_c *image);

void HL_Init(void);

void HUD_FrameSetup(void);

void HL_WriteText(style_c *style, int text_type, int x, int y,
		const char *str, float scale = 1.0f, float alpha = 1.0f);

void HL_WriteTextTrans(style_c *style, int text_type, int x, int y,
		rgbcol_t col, const char *str, float scale = 1.0f, float alpha = 1.0f);

#endif /* __R_DRAW_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
