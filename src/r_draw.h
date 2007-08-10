//----------------------------------------------------------------------------
//  EDGE Video Context
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2007  The EDGE Team.
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

#include "ddf/main.h"
#include "r_image.h"

// Move to somewhere appropriate later -ACB- 2004/08/19
void RGL_DrawImage(float x, float y, float w, float h, const image_c *image,
				   float tx1, float ty1, float tx2, float ty2,
				   const colourmap_c *colmap, float alpha);
 
void RGL_ReadScreen(int x, int y, int w, int h, byte *rgb_buffer);

// This routine should inform the lower level system(s) that the
// screen has changed size/depth.  New size/depth is given.  Must be
// called before any rendering has occurred (e.g. just before
// I_StartFrame).
void RGL_NewScreenSize(int width, int height, int bits);

// Draw a solid colour box (possibly translucent) in the given
// rectangle.  Coordinates are inclusive.  Alpha ranges from 0
// (invisible) to 255 (totally opaque).  Colour is a palette index
// (0-255).  Drawing will be clipped to the current clipping
// rectangle.
void RGL_SolidBox(int x, int y, int w, int h, rgbcol_t col, float alpha = 1.0);

// Draw a solid colour line (possibly translucent) between the two
// end points.  Coordinates are inclusive.  Used for the automap.
// Colour is a palette index (0-255).  Drawing will be clipped to
// the current clipping rectangle.
void RGL_SolidLine(int x1, int y1, int x2, int y2, rgbcol_t col, float alpha = 1.0);

// Convenience functions
void RGL_Image(float x, float y, float w, float h, const image_c *image);
void RGL_Image320(float x, float y, float w, float h, const image_c *image);
void RGL_ImageEasy320(float x, float y, const image_c *image);

#endif /* __R_DRAW_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
