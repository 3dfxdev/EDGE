//----------------------------------------------------------------------------
//  EDGE Generalised Image Handling
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2000  The EDGE Team.
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
//----------------------------------------------------------------------------
//
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#ifndef __W_IMAGE__
#define __W_IMAGE__

#include "dm_type.h"
#include "r_defs.h"
#include "r_state.h"
#include "z_zone.h"

// the transparent pixel value we use
#define TRANS_PIXEL  247

// Post end marker
#define P_SENTINEL  0xFF

typedef struct w_post_s
{
  // number of pixels to skip down from current position.  The initial
  // position is just the top of the sprite.  Can be P_SENTINEL for
  // the end-of-post marker.
  byte skip;

  // number of real pixels following this, not including the following
  // pad pixel and another one after the last pixel.
  byte length;

  // first pad pixel.  Another pad pixel implicitly follows the last
  // real pixel.  The pad pixels should be the same as the real pixel
  // they are next to, in case the fixed point arithmetic overflows.
  byte pad1;
}
w_post_t;

// 16-bit version of the above (EXPERIMENTAL)
typedef struct w_post16_s
{
  byte skip;
  byte length;
  
  unsigned short pad1;
}
w_post16_t;

typedef struct image_s
{
  // total image size, must be a power of two on each axis.
  unsigned short total_w;
  unsigned short total_h;

  // actual image size.  Images that are smaller than their total size
  // are located in the top left corner, cannot tile, and are padded
  // with black pixels if solid, or transparent pixels otherwise.
  unsigned short actual_w;
  unsigned short actual_h;

  // offset values.  Only used for sprites.
  short offset_x;
  short offset_y;

  // scale values, where 0x0100 is normal.  Higher values stretch the
  // image (on the wall/floor), lower values shrink it.
  unsigned short scale_x;
  unsigned short scale_y;

  // whether the image is solid (otherwise it contains transparent
  // parts).
  boolean_t solid;

  // ...rest of this structure is private...
}
image_t;

// opaque structure for using images
typedef byte cached_image_t;

// macro for converting image_t sizes to cached_image_t sizes
#define MIP_SIZE(size,mip)  MAX(1, (size) >> (mip))

// utility macros
#define IM_RIGHT(image)  ((float_t)(image)->actual_w / (image)->total_w)
#define IM_BOTTOM(image) ((float_t)(image)->actual_h / (image)->total_h)

typedef enum
{
  // Vertical posts.  Each post is essentially the same as in normal
  // doom, with one exception: the first byte is not an absolute Y
  // offset from top of sprite, but the number of pixels to skip from
  // the current position.  This removes the height limitation from
  // the column drawers (it still exists in the WAD format though).
  IMG_Post = 0,

  // Block o' pixels.  Total size is MIP_SIZE(total_h, mip) *
  // MIP_SIZE(total_w, mip).  Pixel ordering is optimised for walls:
  // 1st byte is top left, 2nd byte is next pixel down, etc...
  // Transparent parts (if any) have `TRANS_PIXEL' values.
  IMG_Block = 1,

  // OpenGL support.  The image is not read directly, but referred to
  // as a GL texture id number (which can be given to glBindTexture).
  // The `mip' value must be 0).  Transparent parts (if any) are given
  // an alpha of zero (otherwise alpha is 255).
  IMG_OGL = 2
}
image_mode_e;

//
//  IMAGE LOOKUP
//

const image_t *W_ImageFromTexture(const char *tex_name);
const image_t *W_ImageFromFlat(const char *flat_name);
const image_t *W_ImageFromPatch(const char *patch_name);
const image_t *W_ImageFromFont(const char *patch_name);
const image_t *W_ImageFromHalo(const char *patch_name);

//
//  IMAGE USAGE
//

extern boolean_t use_mipmapping;

void W_InitImages(void);
void W_UpdateImageAnims(void);

const cached_image_t *W_ImageCache(const image_t *image, 
    image_mode_e mode, int mip, boolean_t anim);
void W_ImageDone(const cached_image_t *c);

const w_post_t *W_ImageGetPost(const cached_image_t *c, int column);

const byte *W_ImageGetBlock(const cached_image_t *c);

#ifdef USE_GL
GLuint W_ImageGetOGL(const cached_image_t *c);
#endif


#endif  // __W_IMAGE__
