//------------------------------------------------------------------------
//  Targa Image Handling
//------------------------------------------------------------------------
//
//  Copyright (c) 2006-2007  The EDGE Team.
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

#include "image_tga.h"

#include "errors.h"
#include "files.h"

#include <math.h>
#include <stdlib.h>

namespace epi
{

image_data_c *TGA_Load(file_c *f, bool invert, bool round_pow2)
{

	int width  = cinfo.image_width;
	int height = cinfo.image_height;

	int tot_W = width;
	int tot_H = height;

	if (round_pow2)
	{
		tot_W = 1; while (tot_W < (int)width)  tot_W <<= 1;
		tot_H = 1; while (tot_H < (int)height) tot_H <<= 1;
	}

	image_data_c *img = new image_data_c(tot_W, tot_H, 3);

	/* read image pixels */


	return img;
}

bool TGA_GetInfo(file_c *f, int *width, int *height, bool *solid)
{
}

}  // namespace epi
