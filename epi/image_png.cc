//------------------------------------------------------------------------
//  PNG Image Handling
//------------------------------------------------------------------------
//
//  Copyright (c) 2003-2017  The EDGE Team.
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
#include "endianess.h"

#include "image_png.h"
#include "image_data.h"

#include "stb_image.h"
#include "stb_image_write.h"
extern epi::image_data_c *epimg_load(epi::file_c *F, int read_flags);
extern bool epimg_info(epi::file_c *F, int *width, int *height, bool *solid);
extern bool epimg_write_png(FILE *fp, int w, int h, int c, void *data, int s);

namespace epi
{
	bool PNG_IsDataPNG(const byte *data, int length)
	{
        static byte png_sig[4] = { 0x89, 0x50, 0x4E, 0x47 };
		if (length < 4)
			return false;

		return memcmp(data, png_sig, 4) == 0;
	}

	image_data_c *PNG_Load(file_c *f, int read_flags)
	{
		image_data_c *img = NULL;

		byte sig_buf[4];

		/* check the signature */
        f->Seek(0,  epi::file_c::SEEKPOINT_START);
		f->Read(sig_buf, 4);
        if (!PNG_IsDataPNG(sig_buf, 4))
		{
			fprintf(stderr, "PNG_Load - File is not a PNG image!\n");
			return NULL;
		}

		img = epimg_load(f, read_flags);
        if (!img)
		{
			fprintf(stderr, "PNG_Load - Couldn't load PNG image!\n");
			return NULL;
		}

		/* check for grAb chunk */
        int i = 0, j = 0, len = f->GetLength();

        f->Seek(0,  epi::file_c::SEEKPOINT_START);
        for ( ; i<len && j<5; i++)
        {
            static byte pgs[5] = { 0x08, 'g', 'r', 'A', 'b' };
            byte tc;
            f->Read(&tc, 1);
            if (tc == pgs[j])
                j++;
            else
                j = 0;
        }

        if (j == 5)
        {
            int x, y;
            png_grAb_t *grAb = new png_grAb_t;

            f->Read(&x, 4);
            f->Read(&y, 4);
            grAb->x = EPI_BE_S32(x);
            grAb->y = EPI_BE_S32(y);
#if 1
            I_Printf("Got grAb struct: offset_x->%d/offset_y->%d\n", grAb->x, grAb->y);
#endif
			if (grAb->x)
				grAb->x = img->used_w / 2 - grAb->x; // convert grAb offset to OpenGL offset
			if (grAb->y)
				grAb->y = grAb->y - img->used_h; // convert grAb offset to OpenGL offset
            img->grAb = grAb;
        }

		return img;
	}

	bool PNG_GetInfo(file_c *f, int *width, int *height, bool *solid)
	{
		byte sig_buf[4];

		/* check the signature */
        f->Seek(0,  epi::file_c::SEEKPOINT_START);
		f->Read(sig_buf, 4);
        if (!PNG_IsDataPNG(sig_buf, 4))
            false; //TODO: V606 https://www.viva64.com/en/w/v606/ Ownerless token 'false'.

        return epimg_info(f, width, height, solid);
	}


	//------------------------------------------------------------------------

	bool PNG_Save(FILE *fp, const image_data_c *img, int compress)
	{
		SYS_ASSERT(img->bpp >= 3);

        stbi_write_png_compression_level = compress;
        stbi_flip_vertically_on_write(1);

        if (epimg_write_png(fp, img->used_w, img->used_h, img->bpp, img->PixelAt(0, 0), (int)img->width*img->bpp))
            return true;

        return false;
	}

}  // namespace epi

   //--- editor settings ---
   // vi:ts=4:sw=4:noexpandtab
