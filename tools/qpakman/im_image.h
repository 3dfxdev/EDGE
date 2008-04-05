//------------------------------------------------------------------------
//  Basic image storage
//------------------------------------------------------------------------
// 
//  Copyright (c) 2008  Andrew J. Apted
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

#ifndef __IMAGE_DATA_H__
#define __IMAGE_DATA_H__

#define ALPHA_SOLID  255
#define ALPHA_TRANS  0


#define MAKE_RGB(r,g,b)     (((r) << 16) | ((g) << 8) | (b) | (ALPHA_SOLID<<24))
#define MAKE_RGBA(r,g,b,a)  (((r) << 16) | ((g) << 8) | (b) | ((a)<<24))

#define RGB_R(col)  (((col) >> 16) & 0xFF)
#define RGB_G(col)  (((col) >>  8) & 0xFF)
#define RGB_B(col)  (((col)      ) & 0xFF)
#define RGB_A(col)  (((col) >> 24) & 0xFF)


class rgb_image_c
{
public:
  int width;
  int height;

  u32_t *pixels;

  // true if image is known to contain no transparent parts
  bool is_solid;

public:
   rgb_image_c(int _w, int _h);
  ~rgb_image_c();

  void Clear();
  // set all pixels to zero.

  inline u32_t& PixelAt(int x, int y) const
  {
    // Note: DOES NOT CHECK COORDS

    return pixels[(y * width + x)];
  }

  inline void CopyPixel(int sx, int sy, int dx, int dy)
  {
    PixelAt(dx, dy) = PixelAt(sx, sy);
  }

  rgb_image_c *Duplicate() const;

  void SwapEndian();
  // endian swap all pixels (used by PNG load/save code).

  void Whiten();
  // convert all RGB(A) pixels to a greyscale equivalent.
  
  void Mirror();
  // flip the image left-to-right.

  void Invert();
  // turn the image up-side-down.

  void RemoveAlpha();
  // convert an RGBA image to RGB.  Partially transparent colors
  // (alpha < 255) are blended with black.
  
  void ThresholdAlpha(u8_t alpha = 128);
  // test each alpha value in the RGBA image against the threshold:
  // lesser values become 0, and greater-or-equal values become 255.

  void UpdateSolid();
  // update the 'is_solid' flag by checking every pixel.
  
  rgb_image_c * ScaledDup(int new_w, int new_h);
  // duplicate the image, scaling it to the new size
  // (using an ugly nearest-pixel algorithm).

  rgb_image_c * NiceMip();
  // duplicate the image, scaling it exactly in half on each axis
  // (hence the image MUST have an even width and height).  The
  // color components (R,G,B,A) in each 2x2 block are averaged to
  // produce the target pixel.
};


#endif  /* __IMAGE_DATA_H__ */

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
