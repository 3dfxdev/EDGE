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
//
//  Based on IMG_tga.c in the SDL_image library.
//  SDL_image is Copyright (C) 1997-2006 Sam Lantinga.
//
//------------------------------------------------------------------------

#include "image_tga.h"

#include "errors.h"
#include "files.h"

#include <math.h>
#include <stdlib.h>

namespace epi
{

typedef struct
{
    u8_t info_len;        /* length of info field */
    u8_t has_cmap;        /* 1 if image has colormap, 0 otherwise */
    u8_t type;

    u8_t cmap_start[2];   /* index of first colormap entry */
    u8_t cmap_len[2];     /* number of entries in colormap */
    u8_t cmap_bits;       /* bits per colormap entry */

    u8_t y_origin[2];     /* image origin */
    u8_t x_origin[2];
    u8_t width[2];        /* image size */
    u8_t height[2];

    u8_t pixel_bits;      /* bits/pixel */
    u8_t flags;
}
tga_header_t;

#define GET_U16_FIELD(hdvar, field)  (u16_t)  \
		((hdvar).field[0] + ((hdvar).field[1] << 8))

#define GET_S16_FIELD(hdvar, field)  (s16_t)  \
		((hdvar).field[0] + ((hdvar).field[1] << 8))

typedef enum
{
	TGA_TYPE_INDEXED = 1,
    TGA_TYPE_RGB = 2,
    TGA_TYPE_BW = 3,
    TGA_TYPE_RLE_INDEXED = 9,
    TGA_TYPE_RLE_RGB = 10,
    TGA_TYPE_RLE_BW = 11
}
tga_type_e;


image_data_c *TGA_Load(file_c *f, int read_flags)
{
	tga_header_t header;

	size_t nbytes = f->Read((u8_t*) &header, sizeof(header));

	if (nbytes <= sizeof(header))
		return NULL;

	if (header.type != TGA_TYPE_INDEXED &&
        header.type != TGA_TYPE_RGB &&
	    header.type != TGA_TYPE_RLE_INDEXED &&
		header.type != TGA_TYPE_RLE_RGB)
		return NULL;

	int width  = GET_U16_FIELD(header, width);
	int height = GET_U16_FIELD(header, height);

	if (width  < 4 || width  > 4096 || 
		height < 4 || height > 4096)
		return NULL;

	int tot_W = width;
	int tot_H = height;

	if (read_flags & IRF_Round_POW2)
	{
		tot_W = 1; while (tot_W < (int)width)  tot_W <<= 1;
		tot_H = 1; while (tot_H < (int)height) tot_H <<= 1;
	}

	// skip the info field
	if (header.info_len > 0)
	{
		if (! f->Seek(header.info_len, file_c::SEEKPOINT_CURRENT))
			return NULL;
	}

	int new_bpp = 3;
	if (header.pixel_bits == 32) new_bpp = 4;

	image_data_c *img = new image_data_c(tot_W, tot_H, new_bpp);

	img->used_w = width;
	img->used_h = height;
	
	/* read image pixels */

	//!!!!!! FIXME: assumes TGA_TYPE_RGB
	
	for (int y = 0; y < img->used_h; y++)
	for (int x = 0; x < img->used_w; x++)
	{
		u8_t *dest = img->PixelAt(x, y);

		f->Read(dest, new_bpp);
	}

	return img;
}

bool TGA_GetInfo(file_c *f, int *width, int *height, bool *solid)
{
	tga_header_t header;

	size_t nbytes = f->Read((u8_t*) &header, sizeof(header));

	if (nbytes <= sizeof(header))
		return false;

	*width  = GET_U16_FIELD(header, width);
	*height = GET_U16_FIELD(header, height);

	if (header.type == TGA_TYPE_INDEXED || header.type == TGA_TYPE_RLE_INDEXED)
		*solid = (header.cmap_bits != 32); 
	else
		*solid = (header.pixel_bits != 32);

	// *x_offset = GET_S16_FIELD(header, x_origin);
	// *y_offset = GET_S16_FIELD(header, y_origin);

	return true;
}

} // namespace epi

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
