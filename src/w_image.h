//----------------------------------------------------------------------------
//  EDGE Generalised Image Handling
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
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

struct texturedef_s;

// the transparent pixel value we use
#define TRANS_PIXEL  247

// Post end marker
#define P_SENTINEL  0xFF


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

    // when true, the image is guaranteed to be solid (i.e. contain no
    // transparent parts).
	bool img_solid;

	// ...rest of this structure is private...
}
image_t;

// opaque structure for using images
typedef byte cached_image_t;

// macro for converting image_t sizes to cached_image_t sizes
#define MIP_SIZE(size,mip)  MAX(1, (size) >> (mip))

// utility macros
#define IM_RIGHT(image)  ((float)(image)->actual_w / (image)->total_w)
#define IM_BOTTOM(image) ((float)(image)->actual_h / (image)->total_h)

#define IM_WIDTH(image)  ((image)->actual_w * (image)->scale_x / 0x100)
#define IM_HEIGHT(image) ((image)->actual_h * (image)->scale_y / 0x100)

#define IM_OFFSETX(image) ((image)->offset_x * (image)->scale_x / 0x100)
#define IM_OFFSETY(image) ((image)->offset_y * (image)->scale_y / 0x100)

typedef enum
{
	WSKY_North = 0,
	WSKY_East,
	WSKY_South,
	WSKY_West,
	WSKY_Top,
	WSKY_Bottom
}
sky_box_face_e;

//
//  IMAGE LOOKUP
//

const image_t *W_ImageFromTexture(const char *tex_name, bool allow_null = false);
const image_t *W_ImageFromFlat(const char *flat_name,   bool allow_null = false);
const image_t *W_ImageFromPatch(const char *patch_name, bool allow_null = false);
const image_t *W_ImageFromFont(const char *patch_name,  bool allow_null = false);
const image_t *W_ImageFromSkyMerge(const image_t *sky, int face);
const image_t *W_ImageForDummySprite(void);

extern image_t *savepic_image;

const image_t *W_ImageFromString(char type, const char *name);
void W_ImageToString(const image_t *image, char *type, char *namebuf);


//
//  IMAGE USAGE
//

extern int use_mipmapping;
extern bool use_smoothing;
extern bool use_dithering;

bool W_InitImages(void);
void W_UpdateImageAnims(void);
void W_ResetImages(void);

void W_ImageCreateFlats(int *lumps, int number);
void W_ImageCreateTextures(struct texturedef_s ** defs, int number);
const image_t *W_ImageCreateSprite(int lump);
void W_ImageCreateUserImages(void);
void W_AnimateImageSet(const image_t ** images, int number, int speed);
void W_DrawSavePic(const byte *pixels);

void W_LockImagesOGL(void);
void W_UnlockImagesOGL(void);

const cached_image_t *W_ImageCache(const image_t *image, 
								   bool anim = true,
								   const colourmap_c *trans = NULL);
void W_ImageDone(const cached_image_t *c);
void W_ImagePreCache(const image_t *image);

// -AJA- planned....
// epi::basicimage_c *W_ImageGetEpiBlock(const cached_image_t *c);

GLuint W_ImageGetOGL(const cached_image_t *c);

#ifdef DEVELOPERS
const char *W_ImageDebugName(const image_t *image);
#endif

// internal routines -- only needed by rgl_wipe.c
int W_MakeValidSize(int value);

#endif  // __W_IMAGE__

