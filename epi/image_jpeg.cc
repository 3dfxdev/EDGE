//------------------------------------------------------------------------
//  JPEG Image Handling
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

#include "image_jpeg.h"

#include "errors.h"
#include "files.h"

#include <math.h>
#include <stdlib.h>

extern "C"
{
#include <jpeglib.h>
#include <jerror.h>
}

namespace epi
{

namespace JPEG
{
	/* -------- CUSTOM READING CODE ------------------------ */

	const int INPUT_BUF_SIZE = 4096;

	typedef struct
	{
		struct jpeg_source_mgr pub;	/* public fields */

		file_c *infile;
		JOCTET *buffer;
		bool start_of_file;
	}
	my_source_mgr;

	void init_source (j_decompress_ptr cinfo)
	{
		/* No-op */
	}

	void term_source (j_decompress_ptr cinfo)
	{
		/* No-op */
	}

	boolean fill_input_buffer (j_decompress_ptr cinfo)
	{
		my_source_mgr * src = (my_source_mgr *) cinfo->src;
		size_t nbytes;

		nbytes = src->infile->Read(src->buffer, INPUT_BUF_SIZE);

		if (nbytes <= 0)
		{
			if (src->start_of_file)	/* Treat empty input file as fatal error */
				ERREXIT(cinfo, JERR_INPUT_EMPTY);

			WARNMS(cinfo, JWRN_JPEG_EOF);

			/* Insert a fake EOI marker */
			src->buffer[0] = (JOCTET) 0xFF;
			src->buffer[1] = (JOCTET) JPEG_EOI;

			nbytes = 2;
		}

		src->pub.next_input_byte = src->buffer;
		src->pub.bytes_in_buffer = nbytes;
		src->start_of_file = FALSE;

		return TRUE;
	}

	void skip_input_data (j_decompress_ptr cinfo, long num_bytes)
	{
		my_source_mgr * src = (my_source_mgr *) cinfo->src;

		if (num_bytes > 0)
		{
			while (num_bytes > (long) src->pub.bytes_in_buffer)
			{
				num_bytes -= (long) src->pub.bytes_in_buffer;
				/* we assume that fill_input_buffer never returns FALSE */
				(void) fill_input_buffer(cinfo);
			}
			src->pub.next_input_byte += (size_t) num_bytes;
			src->pub.bytes_in_buffer -= (size_t) num_bytes;
		}
	}

	void setup_epifile_src (j_decompress_ptr cinfo, file_c * infile)
	{
		my_source_mgr * src;

		if (cinfo->src == NULL) 	/* first time for this JPEG object? */
		{
			cinfo->src = (struct jpeg_source_mgr *)
				(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
											sizeof(my_source_mgr));

			src = (my_source_mgr *) cinfo->src;

			src->buffer = (JOCTET *)
				(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
											INPUT_BUF_SIZE);
		}

		src = (my_source_mgr *) cinfo->src;

		src->pub.init_source = init_source;
		src->pub.fill_input_buffer = fill_input_buffer;
		src->pub.skip_input_data   = skip_input_data;
		src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
		src->pub.term_source = term_source;

		src->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
		src->pub.next_input_byte = NULL; /* until buffer loaded */

		src->infile = infile;
		src->start_of_file = true;
	}

};  // namespace JPEG

//------------------------------------------------------------------------

basicimage_c *JPEG::Load(file_c *f, bool invert, bool round_pow2)
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	setup_epifile_src(&cinfo, f);

	jpeg_read_header(&cinfo, 1);

	cinfo.quantize_colors = (boolean)FALSE;
	cinfo.out_color_space = JCS_RGB;
	cinfo.out_color_components = 3;
	cinfo.output_components    = 3;

	jpeg_calc_output_dimensions(&cinfo);

	int width  = cinfo.image_width;
	int height = cinfo.image_height;

	int tot_W = width;
	int tot_H = height;

	if (round_pow2)
	{
		tot_W = 1; while (tot_W < (int)width)  tot_W <<= 1;
		tot_H = 1; while (tot_H < (int)height) tot_H <<= 1;
	}

	basicimage_c *img = new basicimage_c(tot_W, tot_H, 3);

	/* read image pixels */

	jpeg_start_decompress(&cinfo);

	JSAMPROW row_pointer[1];

	while (cinfo.output_scanline < cinfo.output_height)
	{
		int y = cinfo.output_scanline;

		if (invert)
			row_pointer[0] = (JSAMPROW)
				(img->pixels + (img->height - 1 - y) * img->width * 3);
		else
			row_pointer[0] = (JSAMPROW) (img->pixels + y * img->width * 3);

		(void) jpeg_read_scanlines(&cinfo, row_pointer, (JDIMENSION) 1);
	}

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	return img;
}

bool JPEG::GetInfo(file_c *f, int *width, int *height, bool *solid)
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	setup_epifile_src(&cinfo, f);

	jpeg_read_header(&cinfo, 1);

	cinfo.quantize_colors = (boolean)FALSE;
	cinfo.out_color_space = JCS_RGB;
	cinfo.out_color_components = 3;
	cinfo.output_components    = 3;

	jpeg_calc_output_dimensions(&cinfo);

	*width  = cinfo.image_width;
	*height = cinfo.image_height;
	*solid  = true;  // JPEG images never have transparent parts

	jpeg_destroy_decompress(&cinfo);

	return true;
}

//------------------------------------------------------------------------

namespace JPEG
{
	void convert_row_to_RGB(const basicimage_c& image, int y, u8_t *buf)
	{
		/// ASSERT(buf);
		/// ASSERT(image.bpp == 4);

		const u8_t *col = image.PixelAt(0, y);

		for (int x=0; x < image.width; x++, col += 4, buf += 3)
		{
			// blend alpha with BLACK

			buf[0] = (int)col[0] * (int)col[3] / 255;
			buf[1] = (int)col[1] * (int)col[3] / 255;
			buf[2] = (int)col[2] * (int)col[3] / 255;
		}
	}
}

bool JPEG::Save(const basicimage_c& image, FILE *fp, int quality)
{
	u8_t *row_data = 0;

	if (image.bpp < 3)
		throw error_c(EPI_ERRGEN_ASSERTION, "[epi::JPEG::Save] image.bpp < 3", true);

	if (image.bpp == 4)
	{
		row_data = (u8_t *) malloc(image.width * 3);

		if (! row_data)
			return false;
	}

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	jpeg_stdio_dest(&cinfo, fp);

	cinfo.image_width  = image.width;
	cinfo.image_height = image.height;
	cinfo.in_color_space   = JCS_RGB;
	cinfo.input_components = 3;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);

	jpeg_start_compress(&cinfo, TRUE);

	JSAMPROW row_pointer[1];

	while (cinfo.next_scanline < cinfo.image_height)
	{
		int y = cinfo.next_scanline;

		if (image.bpp == 3)
			row_pointer[0] = (JSAMPROW) (image.pixels + y * image.width * 3);
		else
		{
			row_pointer[0] = (JSAMPROW) row_data;
			convert_row_to_RGB(image, y, row_data);
		}

		(void) jpeg_write_scanlines(&cinfo, row_pointer, (JDIMENSION) 1);
	}

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	if (row_data)
	{
		free(row_data);
		row_data = 0;
	}

	return true;
}

};  // namespace epi
