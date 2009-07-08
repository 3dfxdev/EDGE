/*
 *  rgb.h
 *  AYM 1998-11-28
 */


/*
This file is part of Yadex.

Yadex incorporates code from DEU 5.21 that was put in the public domain in
1994 by Raphaël Quinet and Brendon Wyber.

The rest of Yadex is Copyright © 1997-2003 André Majorel and others.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA 02111-1307, USA.
*/


#ifndef YH_RGB  /* Prevent multiple inclusion */
#define YH_RGB  /* Prevent multiple inclusion */


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


#endif  /* DO NOT ADD ANYTHING AFTER THIS LINE */
