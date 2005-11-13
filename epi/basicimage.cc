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

#include "basicimage.h"

#include <math.h>
#include <string.h>

namespace epi
{

basicimage_c::basicimage_c(int _w, int _h, int _bpp) :
    width(_w), height(_h), bpp(_bpp)
{
	int size = width * height * bpp;

	pixels = new u8_t[size];

	memset(pixels, 0, size);
}

basicimage_c::~basicimage_c()
{
	delete[] pixels;

	width = height = 0;
	pixels = 0;
}

};  // namespace epi

