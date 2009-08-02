//------------------------------------------------------------------------
//  COLORS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
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
//------------------------------------------------------------------------
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#ifndef __IM_COLOR_H__
#define __IM_COLOR_H__


/* pcolour_t -- a physical colour number.
   The value of a pixel in the opinion of the output library.
   The exact size and meaning of this type vary.
   With X DirectColor visuals, it's an RGB value.
   With X PseudoColor visuals and BGI 256-colour modes,
   it's a palette index.
   With BGI 16-colour modes, it's an IRGB value. */

typedef unsigned long pcolour_t;

#define PCOL_INVALID  0xffffffff  /* An "impossible" colour */


void W_LoadPalette();


extern pcolour_t palette[256];

// game color closest to palette[IMG_TRANSP]
extern int trans_replace;


//------------------------------------------------------------//


#define BLACK           FL_BLACK
#define BLUE            FL_BLUE
#define GREEN           FL_GREEN
#define CYAN            FL_CYAN
#define RED             FL_RED
#define MAGENTA         FL_MAGENTA
#define BROWN           FL_DARK_RED
#define YELLOW          fl_rgb_color(255,255,0)
#define WHITE           FL_WHITE

#define LIGHTGREY       fl_rgb_color(128,128,128)
#define DARKGREY        fl_rgb_color(64,64,64)

#define LIGHTBLUE       fl_rgb_color(128,128,255)
#define LIGHTGREEN      fl_rgb_color(128,255,128)
#define LIGHTCYAN       fl_rgb_color(128,255,255)
#define LIGHTRED        fl_rgb_color(255,128,128)
#define LIGHTMAGENTA    fl_rgb_color(255,128,255)

#define WINBG           fl_rgb_color(0x2a, 0x24, 0x18)
#define WINBG_LIGHT     fl_rgb_color(0x48, 0x42, 0x3c)
#define WINBG_DARK      fl_rgb_color(0x20, 0x1b, 0x12)
#define WINBG_HL        fl_rgb_color(0x58, 0x50, 0x48)

#define WINFG           fl_rgb_color(0xa0, 0xa0, 0xa0)
#define WINFG_HL        fl_rgb_color(0xd0, 0xd0, 0xd0)
#define WINFG_DIM       fl_rgb_color(0x48, 0x48, 0x48)
#define WINFG_DIM_HL    fl_rgb_color(0x70, 0x70, 0x70)

#define WINLABEL        fl_rgb_color(0x78, 0x78, 0x78)
#define WINLABEL_HL     fl_rgb_color(0xa0, 0xa0, 0xa0)
#define WINLABEL_DIM    fl_rgb_color(0x38, 0x38, 0x38)
#define WINLABEL_DIM_HL fl_rgb_color(0x50, 0x50, 0x50)

#define GRID_POINT      fl_rgb_color(0, 0, 0xFF)
#define GRID_DARK       fl_rgb_color(0, 0, 0x99)
#define GRID_MEDIUM     fl_rgb_color(0, 0, 0xBB)
#define GRID_BRIGHT     fl_rgb_color(0, 0, 0xEE)

#define LINEDEF_NO      fl_rgb_color(0x40, 0xd0, 0xf0)
#define SECTOR_NO       fl_rgb_color(0x40, 0xd0, 0xf0)
#define THING_NO        fl_rgb_color(0x40, 0xd0, 0xf0)
#define VERTEX_NO       fl_rgb_color(0x40, 0xd0, 0xf0)
#define CLR_ERROR       fl_rgb_color(0xff, 0,    0)
#define THING_REM       fl_rgb_color(0x40, 0x40, 0x40)

#define SECTOR_TAG      fl_rgb_color(0x00, 0xff, 0x00)
#define SECTOR_TAGTYPE  fl_rgb_color(0x00, 0xe0, 0xe0)
#define SECTOR_TYPE     fl_rgb_color(0x00, 0x80, 0xff)

#define WINTITLE        fl_rgb_color(0xff, 0xff, 0x00)


#endif  /* __IM_COLOR_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
