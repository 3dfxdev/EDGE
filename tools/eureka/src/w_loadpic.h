//------------------------------------------------------------------------
//  WAD PIC LOADER
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor (C) 2001-2009 Andrew Apted
//                     (C) 1997-2003 André Majorel et al
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
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------


#ifndef YH_PIC2IMG  /* DO NOT INSERT ANYTHING BEFORE THIS LINE */
#define YH_PIC2IMG


#include "im_img.h"


/* LoadPicture() normally attempts to load the whole picture lump
   into memory at once. However, this variable sets an upper limit
   to the size of the buffer that it allocates. If the picture is
   larger than this, it will be read in several passes. */
#define MAX_PICTURE_BUFFER_SIZE 100000


int LoadPicture (
   Img& img,
#if 0
   img_pixel_t *buffer, /* Buffer to load picture into */
   int buf_width,   /* Dimensions of the buffer */
   int buf_height,
#endif
   const char *picname,   /* Picture lump name */
   const Lump_loc& picloc,  /* Picture lump name */
   int pic_x_offset,    /* Coordinates of top left corner of picture */
   int pic_y_offset,    /* relative to top left corner of buffer. */
   int *pic_width = 0,    /* To return the size of the picture */
   int *pic_height = 0);  /* (can be NULL) */


#endif  /* DO NOT ADD ANYTHING AFTER THIS LINE */
//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
