//------------------------------------------------------------------------
//  Basic image storage
//------------------------------------------------------------------------
// 
//  Copyright (c) 2003-2007  The EDGE Team.
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

#ifndef __EPI_IMAGEDATA_H__
#define __EPI_IMAGEDATA_H__

#include "epi.h"
#include "types.h"

namespace epi
{

class image_data_c
{
public:
	short width;
	short height;

	short bpp;
	// Bytes Per Pixel, determines image mode:
	// 1 = palettised
	// 3 = format is RGB
	// 4 = format is RGBA

  short flags;

	u8_t *pixels;

  // TODO: color_c *palette;

public:
	image_data_c(int _w, int _h, int _bpp = 3);
	~image_data_c();

  void Clear(u8_t val = 0);

	inline u8_t *PixelAt(int x, int y) const
	{
		// Note: DOES NOT CHECK COORDS

		return pixels + (y * width + x) * bpp;
	}

	inline void CopyPixel(int sx, int sy, int dx, int dy)
	{
		u8_t *src  = PixelAt(sx, sy);
		u8_t *dest = PixelAt(dx, dy);

		for (int i = 0; i < bpp; i++)
			*dest++ = *src++;
	}

	void Whiten();
	// convert all RGB(A) pixels to a greyscale equivalent.
	
	void Shrink(int new_w, int new_h);
	// shrink an image to a smaller image.
	// The old size and the new size must be powers of two.
	// For RGB(A) images the pixel values are averaged.
	//
	// NOTE: Palettised images are not supported.

	void Grow(int new_w, int new_h);
	// scale the image up to a larger size.
	// The old size and the new size must be powers of two.
};

} // namespace epi

#endif  /* __EPI_IMAGEDATA_H__ */
