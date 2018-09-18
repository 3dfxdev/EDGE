//----------------------------------------------------------------------------
//  EDGE stb_image interface
//----------------------------------------------------------------------------
//
//  Copyright (c) 2018 The EDGE Team
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
//----------------------------------

#include "epi.h"

#include "file.h"
#include "filesystem.h"
#include "image_data.h"

#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"

static int eread(void *user, char *data, int size)
{
    epi::file_c *F = (epi::file_c *)user;
    return F->Read(data, size);
}

static void eskip(void *user, int n)
{
    epi::file_c *F = (epi::file_c *)user;
    F->Seek(n, epi::file_c::SEEKPOINT_CURRENT);
}

static int eeof(void *user)
{
    epi::file_c *F = (epi::file_c *)user;
    return F->GetPosition() >= F->GetLength() ? 1 : 0;
}

epi::image_data_c *epimg_load(epi::file_c *F, int read_flags)
{
    stbi_io_callbacks callbacks;
    int w = 0, h = 0, c = 0;

    callbacks.read = &eread;
    callbacks.skip = &eskip;
    callbacks.eof = &eeof;

    F->Seek(0,  epi::file_c::SEEKPOINT_START);
    if (!stbi_info_from_callbacks(&callbacks, (void *)F, &w, &h, &c))
    {
        fprintf(stderr, "epimg_load - stb_image cannot handle file!\n");
        return NULL;
    }
    bool solid = (c == 1) || (c == 3);

    F->Seek(0,  epi::file_c::SEEKPOINT_START);
    stbi_uc *p = stbi_load_from_callbacks(&callbacks, (void *)F, &w, &h, &c, solid ? 3 : 4);
    if (p == NULL)
    {
        fprintf(stderr, "epimg_load - stb_image cannot decode image!\n");
        return NULL;
    }

    int tot_W = w;
    int tot_H = h;
    if (read_flags & epi::image_read_flags_e::IRF_Round_POW2)
    {
        tot_W = 1; while (tot_W < w)  tot_W <<= 1;
        tot_H = 1; while (tot_H < h) tot_H <<= 1;
    }

    epi::image_data_c *img = new epi::image_data_c(tot_W, tot_H, solid ? 3 : 4);
    if (img)
    {
		img->used_w = w;
		img->used_h = h;

		if (img->used_w != tot_W || img->used_h != tot_H)
			img->Clear();

		for (int y = 0; y < img->used_h; y++)
            memcpy(img->PixelAt(0, img->used_h - 1 - y), &p[y*w*(solid ? 3 : 4)], w*(solid ? 3 : 4));
    }

    stbi_image_free(p);
    return img;
}

bool epimg_info(epi::file_c *F, int *width, int *height, bool *solid)
{
    stbi_io_callbacks callbacks;
    int w = 0, h = 0, c = 0;

    callbacks.read = &eread;
    callbacks.skip = &eskip;
    callbacks.eof = &eeof;

    F->Seek(0,  epi::file_c::SEEKPOINT_START);
    if (stbi_info_from_callbacks(&callbacks, (void *)F, &w, &h, &c))
    {
        *width = w;
        *height = h;
        *solid = (c != 4); // not rgba
        return true;
    }

    return false;
}
