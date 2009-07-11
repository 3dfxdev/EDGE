//------------------------------------------------------------------------
//  COLORS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor (C) 2001-2009 Andrew Apted
//                     (C) 1997-2003 André Majorel et al
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

#ifndef YH_COLOUR  // Prevent multiple inclusion
#define YH_COLOUR  // Prevent multiple inclusion

class rgb_c
{
public:
  u8_t r, g, b;

public:
  rgb_c ()
  {
  }

  // Must be defined before rbg_c (r, g, b)
  void set (u8_t red, u8_t green, u8_t blue)
  {
     r = red;
     g = green;
     b = blue;
  }

  rgb_c (u8_t red, u8_t green, u8_t blue)
  {
     set (red, green, blue);
  }

  int operator == (const rgb_c& rgb2) const
  {
     return rgb2.r == r && rgb2.g == g && rgb2.b == b;
  }

  int operator - (const rgb_c& rgb2) const
  {
     return abs (rgb2.r - r) + abs (rgb2.g - g) + abs (rgb2.b - b);
  }
};


/* pcolour_t -- a physical colour number.
   The value of a pixel in the opinion of the output library.
   The exact size and meaning of this type vary.
   With X DirectColor visuals, it's an RGB value.
   With X PseudoColor visuals and BGI 256-colour modes,
   it's a palette index.
   With BGI 16-colour modes, it's an IRGB value. */

typedef unsigned long pcolour_t;  // X11: up to 32 BPP.

#define PCOLOUR_NONE  0xffffffff  /* An "impossible" colour no. */

pcolour_t *alloc_colours (rgb_c rgb_values[], size_t count);
void free_colours (pcolour_t *pc, size_t count);


pcolour_t *alloc_game_colours (int playpalnum);
void free_game_colours (pcolour_t *game_colours);

void irgb2rgb (int c, rgb_c *rgb);
int getcolour (const char *s, rgb_c *rgb);


extern pcolour_t *game_colour;  // Pixel values for the DOOM_COLOURS game clrs.
extern int colour0;   // Game colour to which g. colour 0 is remapped
extern int sky_colour;    // Game colour for a medium blue sky


/*
 *  Game_colour_24 - convert game colours to pixel values
 *  
 *  This class is used to speed up displaying of images on
 *  visuals where bits_per_pixel is 24.
 *
 *  Game_colour_24::lut() returns a pointer to a const table
 *  that is similar to game_colour[] except that it is
 *  optimized toward the needs of display_img() :
 *  each member can be readily copied into the XImage buffer
 *  (this is not true of game_colour[], at least not if the
 *  client is big-endian or does not have the same
 *  endianness as the server).
 *
 *  There is exactly 1 instance of this class (global). It's
 *  refreshed in InitGfx() and used in display_img().
 *  To avoid wasting memory when it's not needed (when the
 *  depth is != 24), the array is allocated upon refresh and
 *  refresh() is only called when the depth is == 24.
 */


typedef byte pv24_t[3];  // A 24-bit pixel value


class Game_colour_24
{
  public :
    Game_colour_24 () { pv_table = 0; }
    ~Game_colour_24 () { if (pv_table) delete[] pv_table; }
    // Create/refresh the table
    void refresh (const pcolour_t *game_colour, const bool big_endian);
    // Return a pointer on an array of pv24_t[DOOM_COLOURS]
    const pv24_t *lut () { return pv_table; }
  private :
    pv24_t *pv_table;
};


extern Game_colour_24 game_colour_24;


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


#endif  // Prevent multiple inclusions

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
