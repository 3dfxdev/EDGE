//----------------------------------------------------------------------------
//  EDGE Video Context
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

#ifndef __V_CTX__
#define __V_CTX__

#include "dm_type.h"
#include "ddf_main.h"
#include "w_image.h"

// Move to somewhere appropriate later -ACB- 2004/08/19
void RGL_DrawImage(int x, int y, int w, int h, const image_t *image,
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
void RGL_SolidBox(int x, int y, int w, int h, rgbcol_t colour, float alpha);

// Draw a solid colour line (possibly translucent) between the two
// end points.  Coordinates are inclusive.  Used for the automap.
// Colour is a palette index (0-255).  Drawing will be clipped to
// the current clipping rectangle.
void RGL_SolidLine(int x1, int y1, int x2, int y2, int colour);

// Convenience macros
#define RGL_Image(X,Y,W,H,Image)                                   \
    RGL_DrawImage((X)-(Image)->offset_x,                           \
                   (Y)-(Image)->offset_y,                           \
                   (W),(H),                                         \
                   (Image), 0, 0,                                   \
                   IM_RIGHT(Image),IM_BOTTOM(Image), NULL, 1.0f)


#define RGL_Image320(X,Y,W,H,Image)                                \
    RGL_DrawImage(FROM_320((X)-(Image)->offset_x),                 \
                   FROM_200((Y)-(Image)->offset_y),                 \
                   FROM_320(W), FROM_200(H),                        \
                   (Image), 0, 0,                                   \
                   IM_RIGHT(Image), IM_BOTTOM(Image), NULL, 1.0f)


#define RGL_ImageEasy(X,Y,Image)                                   \
    RGL_DrawImage((X)-(Image)->offset_x,                           \
                   (Y)-(Image)->offset_y,                           \
                   IM_WIDTH(Image), IM_HEIGHT(Image),               \
                   (Image), 0, 0,                                   \
                   IM_RIGHT(Image), IM_BOTTOM(Image), NULL, 1.0f)

#define RGL_ImageEasy320(X,Y,Image)                                       \
    RGL_DrawImage(FROM_320((X)-(Image)->offset_x),                        \
                   FROM_200((Y)-(Image)->offset_y),                        \
                   FROM_320(IM_WIDTH(Image)), FROM_200(IM_HEIGHT(Image)),  \
                   (Image), 0, 0,                                          \
                   IM_RIGHT(Image), IM_BOTTOM(Image), NULL, 1.0f)

#endif  // __V_CTX__





