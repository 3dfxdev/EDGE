//------------------------------------------------------------------------
//  IMAGES
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


#ifndef YH_IMG  /* DO NOT INSERT ANYTHING BEFORE THIS LINE */
#define YH_IMG


typedef byte           img_pixel_t;
typedef unsigned short img_dim_t;
class Img_priv;


/* The colour number used to represent transparent pixels in an
   Img. Any value will do but zero is probably best
   performance-wise. */
const img_pixel_t IMG_TRANSP = 247;


class Img
{
  public :
    Img ();
    Img (img_dim_t width, img_dim_t height, bool opaque);
    ~Img ();
    bool               is_null    () const; // Is it a null image ?
    img_dim_t          width      () const; // Return the width
    img_dim_t          height     () const; // Return the height
    const img_pixel_t *buf        () const; // Return pointer on buffer
    img_pixel_t       *wbuf       ();   // Return pointer on buffer
    void               clear      ();
    void               set_opaque (bool opaque);
    void               resize     (img_dim_t width, img_dim_t height);
    int                save       (const char *filename) const;
    
    Img * spectrify() const;

  private :
    Img            (const Img&);  // Too lazy to implement it
    Img& operator= (const Img&);  // Too lazy to implement it
    Img_priv *p;
};


// FIXME: methods of Img
void scale_img (const Img& img, double scale, Img& omg);


#endif  /* DO NOT ADD ANYTHING AFTER THIS LINE */
//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
