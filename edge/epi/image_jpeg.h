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

#ifndef __EPI_IMAGE_JPEG_H__
#define __EPI_IMAGE_JPEG_H__

#include "epi.h"
#include "basicimage.h"
#include "files.h"

#include <stdio.h>

namespace epi
{

namespace JPEG
{
	const int DEF_QUALITY = 90;

	/* ------ Functions ------------------------------------- */

	basicimage_c *Load(file_c *f, bool invert = false, bool round_pow2 = false);
	// loads the given JPEG image.  Returns 0 if something went wrong.
	// The image will be RGB or RGBA (never paletted).  The size of
	// image (width and height) are rounded to the next highest
	// power-of-two when round_pow2 is set.
	// FIXME: throw exception on failure

	bool GetInfo(file_c *f, int *width, int *height, bool *solid);
	// reads the principle information from the PNG header.
	// (should be much faster than loading the whole image).
	// Returns false if something went wrong.
	// Note: size returned here is the real size, and may be different
	// from the image returned by Load() which rounds to power-of-two.
	// FIXME: throw exception on failure

	bool Save(const basicimage_c& image, FILE *fp, int quality = DEF_QUALITY);
	// saves the image (in JPEG format) to the given file.  Returns false if
	// something went wrong.  The 'quality' parameter is a percentage, the
	// range is roughly 70 to 95 (values outside of this are possible).
	// If there are transparent colors in the image, they will be mixed with
	// black in the output, since JPEG doesn't support transparency.
	// FIXME: throw exception on failure & use file_c instead of FILE*
}

};  // namespace epi

#endif  /* __EPI_IMAGE_JPEG_H__ */
