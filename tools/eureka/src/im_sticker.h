//------------------------------------------------------------------------
//  STICKER (Images)
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


class Img;
class Sticker_priv;


class Sticker
{
  public :
    Sticker ();
    Sticker (const Img& img, bool opaque);
    ~Sticker ();
    bool is_clear ();
    void clear ();
    void load (const Img& img, bool opaque);
    void draw (char grav, int x, int y);

  private :
    Sticker (const Sticker&);     // Too lazy to implement it
    Sticker& operator= (const Sticker&);  // Too lazy to implement it
    Sticker_priv *priv;
};

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
