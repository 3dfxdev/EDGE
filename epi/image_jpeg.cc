//------------------------------------------------------------------------
//  JPEG Image Handling
//------------------------------------------------------------------------
//
//  Copyright (c) 2003-2018  The EDGE Team.
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

#include "epi.h"

#include "image_jpeg.h"
#include "image_data.h"

#include "stb_image.h"
#include "stb_image_write.h"
extern epi::image_data_c *epimg_load(epi::file_c *F, int read_flags);
extern bool epimg_info(epi::file_c *F, int *width, int *height, bool *solid);
extern bool epimg_write_jpg(FILE *fp, int w, int h, int c, void *data, int q);

namespace epi
{
    image_data_c *JPEG_Load(file_c *f, int read_flags)
    {
		image_data_c *img = NULL;

		img = epimg_load(f, read_flags);
        if (!img)
		{
			fprintf(stderr, "Image Loader - Couldn't load image!\n");
			return NULL;
		}

		return img;
    }

    bool JPEG_GetInfo(file_c *f, int *width, int *height, bool *solid)
    {
        return epimg_info(f, width, height, solid);
    }

    bool JPEG_Save(FILE *fp, const image_data_c *img, int quality)
    {
        SYS_ASSERT(img->bpp == 3);

        stbi_flip_vertically_on_write(1);

        if (epimg_write_jpg(fp, img->width, img->height, img->bpp, img->PixelAt(0, 0), quality))
            return true;

        return false;
    }

} // namespace epi

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
