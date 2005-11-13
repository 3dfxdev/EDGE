//------------------------------------------------------------------------
//  Basic image storage
//------------------------------------------------------------------------
// 
//  Copyright (c) 2003-2005  The EDGE Team.
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

#ifndef __EPI_BASICIMAGE_H__
#define __EPI_BASICIMAGE_H__

#include "epi.h"
#include "types.h"

namespace epi
{

class basicimage_c
{
public:
	int width;
	int height;

	int bpp;
	// bytes per pixel, determines image mode:
	// 1 = palettised (NOTE: for w_image internal use only). 
	// 3 = format is RGB, three bytes per pixel.
	// 4 = format is RGBA, four bytes per pixel.
  
	u8_t *pixels;

	basicimage_c(int _w, int _h, int _bpp = 3);
	~basicimage_c();

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
};

}; // namespace epi

#endif  /* __EPI_BASICIMAGE_H__ */
