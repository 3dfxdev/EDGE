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

#include "image_data.h"

#include <math.h>
#include <string.h>

#include "asserts.h"

namespace epi
{

image_data_c::image_data_c(int _w, int _h, int _bpp) :
    width(_w), height(_h), bpp(_bpp)
{
	int size = width * height * bpp;

	pixels = new u8_t[size];

	memset(pixels, 0, size);
}

image_data_c::~image_data_c()
{
	delete[] pixels;

	width = height = 0;
	pixels = 0;
}

image_data_c::Whiten()
{
	EPI_ASSERT(bpp >= 3);

	for (int y = 0; y < height; y++)
	for (int x = 0; x < width; x++)
	{
		u8_t *cur = img->PixelAt(x, y);

		int ity = MAX(cur[0], MAX(cur[1], cur[2]));

		ity = (ity * 128 + cur[0] * 38 + cur[1] * 64 + cur[2] * 25) >> 8;

		cur[0] = cur[1] = cur[2] = ity; 
	}
}

void image_data_c::Shrink(int new_w, int new_h)
{
	EPI_ASSERT(bpp >= 3);
	EPI_ASSERT(new_w <= width && new_h <= height);

	int step_x = width  / new_w;
	int step_y = height / new_h;
  int total  = step_x * step_y;

	// TODO: OPTIMISE this

  if (bpp == 3)
  {
    for (int dy=0; dy < new_h; dy++)
    for (int dx=0; dx < new_w; dx++)
    {
      u8_t *dest_pix = pixelAt(dx, dy);

      int sx = x * step_x;
      int sy = y * step_y;

      int r=0, g=0, b=0;

      // compute average colour of block
      for (int dx=0; x < step_x; x++)
      for (int dy=0; y < step_y; y++)
      {
        u8_t *src_pix = pixelAt(sx+x, sy+y);

        r += src_pix[0];
        g += src_pix[1];
        b += src_pix[2];
      }

      dest_pix[0] = r / total;
      dest_pix[1] = g / total;
      dest_pix[2] = b / total;
    }
  }
  else  /* bpp == 4 */
  {
    for (int dy=0; dy < new_h; dy++)
    for (int dx=0; dx < new_w; dx++)
    {
      u8_t *dest_pix = pixelAt(dx, dy);

      int sx = dx * step_x;
      int sy = dy * step_y;

      int r=0, g=0, b=0, a=0;

      // compute average colour of block
      for (int x=0; x < step_x; x++)
      for (int y=0; y < step_y; y++)
      {
        u8_t *src_pix = pixelAt(sx+x, sy+y);

        r += src_pix[0];
        g += src_pix[1];
        b += src_pix[2];
        a += src_pix[3];
      }

      dest_pix[0] = r / total;
      dest_pix[1] = g / total;
      dest_pix[2] = b / total;
      dest_pix[3] = a / total;
    }
  }
}

void image_data_c::Grow(int new_w, int new_h)
{
	EPI_ASSERT(new_w >= width && new_h >= height);

	int new_size = new_w * new_h * bpp;

	u8_t *new_pixels = new u8_t[new_size];

	// TODO: OPTIMISE this

	for (int dy = 0; dy < new_h; dy++)
	for (int dx = 0; dx < new_w; dx++)
	{
		int sx = dx * width  / new_w;
		int sy = dy * height / new_h;

		const u8_t *src = pixelAt(sx, sy);

		u8_t *dest = new_pixels + (dy * new_w + dx) * bpp;

		for (int i = 0; i < bpp; i++)
			*dest++ = *src++;
	}
	
	delete[] pixels;

	pixels = new_pixels;
	width  = new_w;
	height = new_h;
}

} // namespace epi

