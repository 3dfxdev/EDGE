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

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"

static void ewrite(void *cntx, void *data, int size)
{
    FILE *fp = (FILE *)cntx;
    fwrite(data, size, 1, fp);
}

bool epimg_write_png(FILE *fp, int w, int h, int c, void *data, int s)
{
    return stbi_write_png_to_func(&ewrite, (void *)fp, w, h, c, data, s) ? true : false;
}

bool epimg_write_jpg(FILE *fp, int w, int h, int c, void *data, int q)
{
    return stbi_write_jpg_to_func(&ewrite, (void *)fp, w, h, c, data, q) ? true : false;
}
