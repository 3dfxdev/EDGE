//----------------------------------------------------------------------------
//  EDGE Generalised Image Handling
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2008  The EDGE Team.
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

#ifndef __R_IMAGE__
#define __R_IMAGE__

#include <vector>

#include "epi/image_data.h"
#include "ddf/image.h"

#include "r_defs.h"
#include "r_state.h"

struct texturedef_s;

#ifndef USING_GL_TYPES
#define GLuint  u32_t
#endif


// the transparent pixel value we use
#define TRANS_PIXEL  247

// Post end marker
#define P_SENTINEL  0xFF

// dynamic light sizing factor
#define DL_OUTER       64.0f
#define DL_OUTER_SQRT   8.0f


typedef enum
{
	OPAC_Unknown = 0,

	OPAC_Solid   = 1,  // utterly solid (alpha = 255 everywhere)
	OPAC_Masked  = 2,  // only uses alpha 255 and 0
	OPAC_Complex = 3,  // uses full range of alpha values
}
image_opacity_e;


class image_c
{
public:
	// actual image size.  Images that are smaller than their total size
	// are located in the bottom left corner, cannot tile, and are padded
	// with black pixels if solid, or transparent pixels otherwise.
	unsigned short actual_w;
	unsigned short actual_h;

	// total image size, must be a power of two on each axis.
	unsigned short total_w;
	unsigned short total_h;

    // offset values.  Only used for sprites and on-screen patches.
	short offset_x;
	short offset_y;

    // scale values, where 1.0f is normal.  Higher values stretch the
    // image (on the wall/floor), lower values shrink it.
	float scale_x;
	float scale_y;

    // one of the OPAC_XXX values
	int opacity;

	
	// -------- PRIVATE STUFF FROM NOW ON -------- //

	char name[16];

	enum
	{
		Graphic, // Source was a graphic name
		Raw320x200, // INTERNAL ONLY: Source was a raw block of 320x200 bytes (Heretic/Hexen)
		Sprite,  // Source was a sprite name
		Flat,    // Source was a flat name
		Texture, // Source was a texture name
		User,    // INTERNAL ONLY: Source is from IMAGE.DDF
		Dummy,   // INTERNAL ONLY: Source is dummy image
	};

	int source_type;
 
	union
	{
		// case IMSRC_Graphic:
		// case IMSRC_Sprite:
		struct { int lump; } graphic;

		// case IMSRC_Flat:
		// case IMSRC_Raw320x200:
		struct { int lump; } flat;

		// case IMSRC_Texture:
		struct { struct texturedef_s *tdef; } texture;

		// case IMSRC_Dummy:
		struct { rgbcol_t fg; rgbcol_t bg; } dummy;

		// case IMSRC_User:
		struct { imagedef_c *def; } user;
	}
	source;

	// palette file, or -1 to use the "GLOBAL" palette
	int source_palette;

	// --- information about caching ---

	std::vector< struct cached_image_s * > cache;

	// --- animation info ---

	struct
	{
		// current version of this image in the animation.  Initially points
		// to self.  For non-animated images, doesn't change.  Otherwise
		// when the animation flips over, it becomes cur->next.
		image_c *cur;

		// next image in the animation, or NULL.
		image_c *next;

		// tics before next anim change, or 0 if non-animated.
		unsigned short count;

		// animation speed (in tics), or 0 if non-animated.
		unsigned short speed;
	}
	anim;

public:
	 image_c();
	~image_c();

public:
	GLuint Cache(bool anim = true, const colourmap_c *trans = NULL) const;

	void PreCache() const;

	epi::image_data_c * ReadBlock();

	bool ShouldClamp()  const;
	bool ShouldMipmap() const;
	bool ShouldSmooth() const;
	bool ShouldHQ2X()   const;

	int PixelLimit() const;

	/* TODO: add methods here... */

private:
	epi::image_data_c * ReadFlat();
	epi::image_data_c * ReadTexture();
	epi::image_data_c * ReadPatch();
	epi::image_data_c * ReadUser();
	epi::image_data_c * ReadDummy();
};


// macro for converting image_c sizes to cached_image_t sizes
#define MIP_SIZE(size,mip)  MAX(1, (size) >> (mip))

// utility macros (FIXME: replace with class methods)
#define IM_RIGHT(image)  (float((image)->actual_w) / (image)->total_w)
#define IM_TOP(image)    (float((image)->actual_h) / (image)->total_h)

#define IM_WIDTH(image)  ((image)->actual_w * (image)->scale_x)
#define IM_HEIGHT(image) ((image)->actual_h * (image)->scale_y)

#define IM_OFFSETX(image) ((image)->offset_x * (image)->scale_x)
#define IM_OFFSETY(image) ((image)->offset_y * (image)->scale_y)

// deliberately long, should not be used (except for special cases)
#define IM_TOTAL_WIDTH(image)  ((image)->total_w * (image)->scale_x)
#define IM_TOTAL_HEIGHT(image) ((image)->total_h * (image)->scale_y)


//
//  IMAGE LOOKUP
//
typedef enum
{
	ILF_Null    = 0x0001,  // return NULL rather than a dummy image
	ILF_Exact   = 0x0002,  // type must be exactly the same
	ILF_NoNew   = 0x0004,  // image must already exist (don't create it)
	ILF_Font    = 0x0008,  // font character (be careful with backups)
}
image_lookup_flags_e;

const image_c *R_ImageLookup(const char *name, image_namespace_e = INS_Graphic,
	int flags = 0);

const image_c *R_ImageForDummySprite(void);
const image_c *R_ImageForDummySkin(void);
const image_c *R_ImageForHOMDetect(void);

// savegame code (Only)
const image_c *R_ImageParseSaveString(char type, const char *name);
void R_ImageMakeSaveString(const image_c *image, char *type, char *namebuf);


//
//  IMAGE USAGE
//

// this is incremented for every W_DeleteAllImages(), so that
// specialised images (e.g. for colormapping) can be recreated.
extern int  image_reset_counter;

bool R_InitImages(void);
void R_UpdateImageAnims(void);
void R_DeleteAllImages(void);

const image_c *R_ImageCreateTexture(struct texturedef_s *tdef);
const image_c *R_ImageCreateFlat(const char *name, int lump);
const image_c *R_ImageCreateSprite(const char *name, int lump, bool is_weapon);

void R_ImageCreateUser(void);
void R_AnimateImageSet(const image_c ** images, int number, int speed);
void W_DrawSavePic(const byte *pixels);


// -AJA- planned....
// rgbcol_t W_ImageGetHue(const image_c *c);

// this only needed during initialisation -- w_sprites.cpp
const image_c ** W_ImageGetUserSprites(int *count);

// internal routines -- only needed by hu_wipe.c
int W_MakeValidSize(int value);


#ifndef USING_GL_TYPES
#undef GLuint
#endif

#endif  // __R_IMAGE__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
