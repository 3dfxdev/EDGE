//------------------------------------------------------------------------
//  PNG Image Handling
//------------------------------------------------------------------------
// 
//  Copyright (c) 2008  Andrew J Apted
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

#include "headers.h"

#include "im_png.h"


#undef _SETJMP_H  // workaround for some weirdness in pngconf.h
#include <png.h>


#define CHECK_PNG_BYTES  4

static void local_read_fn(png_structp png_ptr, png_bytep data, png_size_t length)
{
  FILE *fp = (FILE *) png_get_io_ptr(png_ptr);

  if (fread(data, 1, length, fp) != length)
  {
    png_error(png_ptr, "Read Error");
  }
}


rgb_image_c *PNG_Load(FILE *fp)
{
  /* -AJA- all these volatiles here may seem strange.  They are needed
   * because the ANSI C standard (which GCC adheres to) says that when
   * setjmp/longjmp is being used, only volatile local variables are
   * guaranteed to keep their state if longjmp() gets called.
   */
  rgb_image_c * volatile img = 0;

  png_bytep * volatile row_pointers = 0;

  /* we take the address of these two, so we shouldn't need the
   * volatile.  (GCC complains about discarding qualifiers if the
   * volatile is there).
   */
  png_structp /*volatile*/ png_ptr = 0;
  png_infop   /*volatile*/ info_ptr = 0;

  png_byte sig_buf[CHECK_PNG_BYTES];

  /* check the signature */

  if (fread(sig_buf, 1, CHECK_PNG_BYTES, fp) != CHECK_PNG_BYTES ||
    png_sig_cmp(sig_buf, (png_size_t)0, CHECK_PNG_BYTES) != 0)
  {
    printf("PNG_Load : File is not a PNG image !\n");
    goto failed;
  }

  /* pass NULLs for the error functions -- therefore use setjump */

  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (! png_ptr)
    goto failed;

  info_ptr = png_create_info_struct(png_ptr);
  if (! info_ptr)
    goto failed;

  /* set error handling since we are using the setjmp/longjmp method
   * (this is the normal method of doing things with libpng).
   */
  if (setjmp(png_ptr->jmpbuf))
  {
    printf("PNG_Load : Error loading PNG image!\n");
    goto failed;
  }

  png_set_read_fn(png_ptr, (void *)fp, &local_read_fn);
  png_set_sig_bytes(png_ptr, CHECK_PNG_BYTES);

  /* read the header information */

  png_read_info(png_ptr, info_ptr);

  png_uint_32 width, height;
  int bit_depth, color_type, interlace_type;

  int tot_W, tot_H;
  bool solid;

  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
      &color_type, &interlace_type, NULL, NULL);

  solid = ((color_type & PNG_COLOR_MASK_ALPHA) == 0);

  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
    solid = false;

  tot_W = (int)width;
  tot_H = (int)height;

  printf("   loading PNG image, size: %dx%d\n", tot_W, tot_H);

  img = new rgb_image_c(tot_W, tot_H);

  /* tell libpng to strip 16 bits/color down to 8 bits/color */
  png_set_strip_16(png_ptr);

  /* convert low depths (1, 2 or 4) */
  if (bit_depth < 8)
    png_set_packing(png_ptr);

  /* expand paletted colors into true RGB triplets */
  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png_ptr);

  /* expand greyscale into RGB */
  if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
  {
    png_set_gray_to_rgb(png_ptr);
  }

  png_set_bgr(png_ptr);

  /* read alpha channel or add a dummy one */
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
  {
    png_set_tRNS_to_alpha(png_ptr);
  }

  if (color_type & PNG_COLOR_MASK_ALPHA)
  {
    png_set_invert_alpha(png_ptr);
  }
  else
  {
    png_set_add_alpha(png_ptr, ALPHA_SOLID, PNG_FILLER_AFTER);
  }

  /* let all the above calls take effect */
  png_read_update_info(png_ptr, info_ptr);

  row_pointers = new png_bytep[height];

  for (png_uint_32 y = 0; (int)y < img->height; y++)
  {
    row_pointers[y] = (png_bytep) &img->PixelAt(0, y);
  }

  /* now read in the image.  Yeah baby ! */

  png_read_image(png_ptr, row_pointers);
  png_read_end(png_ptr, info_ptr);

  png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);

  delete[] row_pointers;

#if (UT_BYTEORDER == UT_BIG_ENDIAN)
  img->SwapEndian();
#endif

  img->is_solid = solid;

  return img;

  /* -AJA- Normally I don't like gotos.  In this situation where there
   * are lots of points of possible failure and a growing set of
   * things to be undone, it makes for nicer code.
   */
failed:

  if (img)
  {
    delete img;
    img = 0;
  }

  if (png_ptr)
  {
    if (info_ptr)
      png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
    else
      png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
  }

  if (row_pointers)
    delete[] row_pointers;

  return 0;
}


bool PNG_Save(FILE *fp, rgb_image_c *img, int compress)
{
  SYS_ASSERT(compress >= Z_NO_COMPRESSION);
  SYS_ASSERT(compress <= Z_BEST_COMPRESSION);

  png_bytep * volatile row_pointers = 0;

  png_structp /*volatile*/ png_ptr = 0;
  png_infop   /*volatile*/ info_ptr = 0;

  /* pass NULLs for the error functions -- therefore use setjump */

  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (! png_ptr)
    goto failed;

  info_ptr = png_create_info_struct(png_ptr);
  if (! info_ptr)
    goto failed;

  /* set error handling since we are using the setjmp/longjmp method
   * (this is the normal method of doing things with libpng).
   */
  if (setjmp(png_ptr->jmpbuf))
  {
    printf("PNG_Save : Error saving PNG image!\n");
    goto failed;
  }

  png_init_io(png_ptr, fp);
  png_set_compression_level(png_ptr, compress);

  png_set_IHDR(png_ptr, info_ptr, img->width, img->height, 8,
      PNG_COLOR_TYPE_RGB_ALPHA,
      PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
      PNG_FILTER_TYPE_DEFAULT);

#if (UT_BYTEORDER == UT_BIG_ENDIAN)
  img->SwapEndian();
#endif

  png_set_bgr(png_ptr);

  row_pointers = new png_bytep[img->height];

  for (int y = 0; y < img->height; y++)
  {
    row_pointers[y] = (png_bytep) &img->PixelAt(0, y);
  }

  png_set_rows(png_ptr, info_ptr, (png_bytep*) row_pointers);

  png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

  //png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);

  delete[] row_pointers;

  return true;

failed:
  if (png_ptr)
  {
    if (info_ptr)
      png_destroy_write_struct(&png_ptr, &info_ptr);
    else
      png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
  }

  if (row_pointers)
    delete[] row_pointers;

  return false;
}

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
