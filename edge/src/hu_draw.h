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


void HUD_SetCoordSys(int width, int height);

void HUD_SetFont(font_c *font);
void HUD_SetScale(float scale);
void HUD_SetTextColor(rgbcol_t color);
void HUD_SetAlpha(float alpha);

void HUD_SetAlignment(int xa, int ya);
// xa is -1 for left, 0 for centred, +1 for right
// ya is -1 for top,  0 for centred, +! for bottom
// (Note: ya is ignored for text)

void HUD_Reset(const char *what = "acfgs");
// letters can be:
// 	 'f' for Font   	's' for Scale
// 	 'c' for Color		'a' for Alpha
// 	 'g' for Alignment


void HL_Init(void);

void HUD_FrameSetup(void);


void HUD_PushScissor(float x1, float y1, float x2, float y2);
void HUD_PopScissor();
// manage the current clip rectangle.  The first push enables the
// scissor test, subsequent pushes merely shrink the area, and the
// last pop disables the scissor test.


void RGL_DrawImage(float x, float y, float w, float h, const image_c *image,
				 float tx1, float ty1, float tx2, float ty2,
				 float alpha = 1.0f, rgbcol_t text_col = RGB_NO_VALUE,
				 const colourmap_c *palremap = NULL);
 
void HUD_SolidBox(float x1, float y1, float x2, float y2, rgbcol_t col);
// Draw a solid colour box (possibly translucent) in the given
// rectangle.

void HUD_SolidLine(float x1, float y1, float x2, float y2, rgbcol_t col);
// Draw a solid colour line (possibly translucent) between the two
// end points.  Coordinates are inclusive.  Used for the automap.
// Drawing will be clipped to the current clipping rectangle.

void HUD_ThinBox(float x1, float y1, float x2, float y2, rgbcol_t col);
// Draw a thin outline of a box.

void HUD_GradientBox(float x1, float y1, float x2, float y2, rgbcol_t *cols);
// Like HUD_SolidBox but the colors of each corner (TL, BL, TR, BR) can
// be specified individually.

void HUD_DrawImage(float x, float y, const image_c *image);
void HUD_StretchImage(float x, float y, float w, float h, const image_c *image);
void HUD_TileImage(float x, float y, float w, float h, const image_c *image,
				   float offset_x = 0.0f, float offset_y = 0.0f);


float HUD_FontWidth(void);
float HUD_FontHeight(void);

float HUD_StringWidth(const char *str);
float HUD_StringHeight(const char *str);

void HUD_DrawChar(float left_x, float top_y, const image_c *img);

void HUD_DrawText(float x, float y, const char *str);
// draw a text string with the current font, current color (etc).


#endif /* __R_DRAW_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
