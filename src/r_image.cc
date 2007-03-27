//----------------------------------------------------------------------------
//  EDGE Generalised Image Handling
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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
//
// -AJA- 2000/06/25: Began this image generalisation, based on Erik
//       Sandberg's w_textur.c/h code.
//
// TODO HERE:
//   +  support an IMG_RGBBlock retrieve mode (use for glDrawPixels)
//   -  faster search methods.
//   -  do some optimisation
//

#include "i_defs.h"
#include "i_defs_gl.h"
#include "r_image.h"

#include "e_search.h"
#include "e_main.h"
#include "m_argv.h"
#include "m_misc.h"
#include "p_local.h"
#include "r2_defs.h"
#include "rgl_defs.h"
#include "rgl_sky.h"
#include "v_colour.h"
#include "w_textur.h"
#include "w_wad.h"
#include "z_zone.h"

#include "epi/basicimage.h"
#include "epi/endianess.h"

#include "epi/files.h"
#include "epi/filesystem.h"
#include "epi/memfile.h"

#include "epi/image_hq2x.h"
#include "epi/image_png.h"
#include "epi/image_jpeg.h"

#include <limits.h>
#include <math.h>
#include <string.h>

// -AJA- FIXME !!! temporary hack, awaiting good GL extension handling
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE  0x812F
#endif


// range of mip values is 0 to 11, allowing images upto 2048x2048
#define MAX_MIP  11


// LIGHTING DEBUGGING
// #define MAKE_TEXTURES_WHITE  1


typedef enum
{
	// Source was a graphic name
	IMSRC_Graphic = 0,

	// INTERNAL ONLY: Source was a raw block of 320x200 bytes (Heretic/Hexen)
	IMSRC_Raw320x200,

	// Source was a sprite name
	IMSRC_Sprite,

	// Source was a flat name
	IMSRC_Flat,

	// Source was a texture name
	IMSRC_Texture,

	// INTERNAL ONLY: Source is from IMAGE.DDF
	IMSRC_User,

	// INTERNAL ONLY: Source was a sky texture, merged for a pseudo sky box
	IMSRC_SkyMerge,

	// INTERNAL ONLY: Source is dummy image
	IMSRC_Dummy,
}
image_source_e;

typedef enum
{
    // Block o' RGB pixels.  Total size is MIP_SIZE(total_h, mip) *
    // MIP_SIZE(total_w, mip).
    // Transparent parts (if any) have `TRANS_PIXEL' values.
	IMG_EpiBlock = 0,

	// OpenGL support.  The image is not read directly, but referred to
	// as a GL texture id number (which can be given to glBindTexture).
	// The `mip' value must be 0.  Transparent parts (if any) are given
	// an alpha of zero (otherwise alpha is 255).
	IMG_OGL = 1
}
image_mode_e;


struct real_image_s;


//
// This structure is for "cached" images (i.e. ready to be used for
// rendering), and is the non-opaque version of cached_image_t.  A
// single structure is used for all image modes (Block and OGL).
//
// Note: multiple modes and/or multiple mips of the same image_t can
// indeed be present in the cache list at any one time.
//
typedef struct real_cached_image_s
{
	// link in cache list
	struct real_cached_image_s *next, *prev;

    // number of current users
	int users;

	// true if image has been invalidated -- unload a.s.a.p
	bool invalidated;
 
	// parent image
	struct real_image_s *parent;
  
	// mip value (>= 0)
	unsigned short mip;

	// current image mode
	image_mode_e mode;

	// colormap used for translated image, normally NULL.  (GL Only)
	const colourmap_c *trans_map;

	// general hue of image (skewed towards pure colors)
	rgbcol_t hue;

	union
	{
        // case IMG_EpiBlock:
		struct
		{
			// RGB pixel block. MIP_SIZE(total_h,mip) * MIP_SIZE(total_w,mip)
			// is the size. Transparent pixels are TRANS_PIXEL.
			epi::basicimage_c *img;
		}
		block;

		// case IMG_OGL:
		struct
		{
			// texture identifier within GL
			GLuint tex_id;
		}
		ogl;
	}
	info;

	// total memory size taken up by this image.  Includes this
	// structure.
	int size;

	// NOTE: Block data may follow this structure...
}
real_cached_image_t;


//
// This structure is the full version of image_t.  It contains all the
// information needed to create the actual cached images when needed.
//
typedef struct real_image_s
{
	// base is the publicly visible structure
	image_t pub;

	// --- information about where this image came from ---

	char name[16];

	image_source_e source_type;
 
	union
	{
		// case IMSRC_Graphic:
		// case IMSRC_Sprite:
		struct { int lump; } graphic;

		// case IMSRC_Flat:
		// case IMSRC_Raw320x200:
		struct { int lump; } flat;

		// case IMSRC_Texture:
		struct { texturedef_t *tdef; } texture;

		// case IMSRC_SkyMerge:
		struct { const struct image_s *sky; int face; } merge;

		// case IMSRC_Dummy:
		struct { byte fg, bg; } dummy;

		// case IMSRC_User:
		struct { imagedef_c *def; } user;
	}
	source;

	// palette lump, or -1 to use the "GLOBAL" palette
	int source_palette;

	// --- information about caching ---

	// no mipmapping here, GL does this itself
	real_cached_image_t * ogl_cache;

	// --- cached translated images (OpenGL only) ---

	struct
	{
		int num_trans;
		real_cached_image_t ** trans;
	}
	trans_cache;

	// --- animation info ---

	struct
	{
		// current version of this image in the animation.  Initially points
		// to self.  For non-animated images, doesn't change.  Otherwise
		// when the animation flips over, it becomes cur->next.
		struct real_image_s *cur;

		// next image in the animation, or NULL.
		struct real_image_s *next;

		// tics before next anim change, or 0 if non-animated.
		unsigned short count;

		// animation speed (in tics), or 0 if non-animated.
		unsigned short speed;
	}
	anim;
}
real_image_t;


// needed for SKY
static epi::basicimage_c *ReadAsEpiBlock(real_image_t *rim);


// Image container
class real_image_container_c : public epi::array_c
{
public:
	real_image_container_c() : epi::array_c(sizeof(real_image_t*)) {}
	~real_image_container_c() { Clear(); }

private:
	void CleanupObject(void *obj)
	{
		real_image_t *rim = *(real_image_t**)obj;
		
		if (rim)
			delete rim;
	}

public:
	// List Management
	int GetSize() { return array_entries; } 
	int Insert(real_image_t *rim) { return InsertObject((void*)&rim); }
	
	real_image_t* operator[](int idx) 
	{ 
		return *(real_image_t**)FetchObject(idx); 
	} 

	real_image_t *Lookup(const char *name, int source_type = -1)
	{
		// for a normal lookup, we want USER images to override
		if (source_type == -1)
		{
			real_image_t *rim = Lookup(name, IMSRC_User);  // recursion
			if (rim)
				return rim;
		}

		epi::array_iterator_c it;

		for (it = GetBaseIterator(); it.IsValid(); it++)
		{
			real_image_t *rim = ITERATOR_TO_TYPE(it, real_image_t*);
		
			if (source_type != -1 && source_type != (int)rim->source_type)
				continue;

			if (stricmp(name, rim->name) == 0)
				return rim;
		}

		return NULL;  // not found
	}

	void Animate()
	{
		for (epi::array_iterator_c it = GetBaseIterator(); it.IsValid(); it++)
		{
			real_image_t *rim = ITERATOR_TO_TYPE(it, real_image_t*);

			if (rim->anim.speed == 0)  // not animated ?
				continue;

			DEV_ASSERT2(rim->anim.count > 0);

			rim->anim.count--;

			if (rim->anim.count == 0 && rim->anim.cur->anim.next)
			{
				rim->anim.cur = rim->anim.cur->anim.next;
				rim->anim.count = rim->anim.speed;
			}
		}
	}

	void DebugDump()
	{
		L_WriteDebug("{\n");

		epi::array_iterator_c it;
		for (it = GetBaseIterator(); it.IsValid(); it++)
		{
			real_image_t *rim = ITERATOR_TO_TYPE(it, real_image_t*);
		
			L_WriteDebug("   [%s] type %d: %dx%d < %dx%d\n",
				rim->name, rim->source_type,
				rim->pub.actual_w, rim->pub.actual_h,
				rim->pub.total_w, rim->pub.total_h);
		}

		L_WriteDebug("}\n");
	}

};


// mipmapping enabled ?
// 0 off, 1 bilinear, 2 trilinear
int use_mipmapping = 1;

bool use_smoothing = true;
bool use_dithering = false;

static bool w_locked_ogl = false;

// total set of images
static real_image_container_c real_graphics;
static real_image_container_c real_textures;
static real_image_container_c real_flats;
static real_image_container_c real_sprites;

static real_image_container_c sky_merges;
static real_image_container_c dummies;

#define RIM_DUMMY_TEX     dummies[0]
#define RIM_DUMMY_FLAT    dummies[1]
#define RIM_SKY_FLAT      dummies[2]
#define RIM_DUMMY_GFX     dummies[3]
#define RIM_DUMMY_SPRITE  dummies[4]
#define RIM_DUMMY_FONT    dummies[5]

const struct image_s *skyflatimage;

// -AJA- Another hack, this variable holds the current sky when
// compute sky merging.  We hold onto the image, because there are
// six sides to compute, and we don't want to load the original
// image six times.  Removing this hack requires caching it in the
// cache system (which is not possible right now).
static epi::basicimage_c *merging_sky_image;

// use a common buffer for image shrink operations, saving the
// overhead of allocating a new buffer for every image.
static byte *img_shrink_buffer;
static int img_shrink_buf_size;


// forward decls
static epi::file_c *OpenUserFileOrLump(imagedef_c *def);
static void CloseUserFileOrLump(imagedef_c *def, epi::file_c *f);


// image cache (actually a ring structure)
static real_cached_image_t imagecachehead;


// tiny ring helpers
static INLINE void InsertAtTail(real_cached_image_t *rc)
{
	DEV_ASSERT2(rc != &imagecachehead);

	rc->prev =  imagecachehead.prev;
	rc->next = &imagecachehead;

	rc->prev->next = rc;
	rc->next->prev = rc;
}
static INLINE void Unlink(real_cached_image_t *rc)
{
	DEV_ASSERT2(rc != &imagecachehead);

	rc->prev->next = rc->next;
	rc->next->prev = rc->prev;
}

// the number of bytes of the texture cache that currently can be
// freed.  Useful when we decide how much to flush.  This changes when
// the number of users of a block changes from 1 to 0 or back.
static int cache_size = 0;


// Dummy image, for when texture/flat/graphic is unknown.  Row major
// order.  Could be packed, but why bother ?
#define DUMMY_X  16
#define DUMMY_Y  16
static byte dummy_graphic[DUMMY_X * DUMMY_Y] =
{
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,
	0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,
	0,0,1,1,1,1,0,0,0,0,0,1,1,1,0,0,
	0,0,0,1,1,0,0,0,0,0,0,1,1,1,0,0,
	0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,
	0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,
	0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,
	0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};


//----------------------------------------------------------------------------

//
//  IMAGE CREATION
//

int W_MakeValidSize(int value)
{
	DEV_ASSERT2(value > 0);

	if (value <=    1) return    1;
	if (value <=    2) return    2;
	if (value <=    4) return    4;
	if (value <=    8) return    8;
	if (value <=   16) return   16;
	if (value <=   32) return   32;
	if (value <=   64) return   64;
	if (value <=  128) return  128;
	if (value <=  256) return  256;
	if (value <=  512) return  512;
	if (value <= 1024) return 1024;
	if (value <= 2048) return 2048;
	if (value <= 4096) return 4096;

	I_Error("Texture size (%d) too large !\n", value);
	return -1; /* NOT REACHED */
}

static real_image_t *NewImage(int width, int height, bool solid)
{
	real_image_t *rim = new real_image_t;

	// clear newbie
	memset(rim, 0, sizeof(real_image_t));

	rim->pub.actual_w = width;
	rim->pub.actual_h = height;
	rim->pub.total_w  = W_MakeValidSize(width);
	rim->pub.total_h  = W_MakeValidSize(height);
	rim->pub.offset_x = rim->pub.offset_y = 0;
	rim->pub.scale_x  = rim->pub.scale_y = 1.0f;
	rim->pub.img_solid = solid;

	// set initial animation info
	rim->anim.cur = rim;
	rim->anim.next = NULL;
	rim->anim.count = rim->anim.speed = 0;

	return rim;
}

static real_image_t *AddImageDummy(image_source_e realsrc, const char *name)
{
	real_image_t *rim;
  
	rim = NewImage(DUMMY_X, DUMMY_Y, 
				   (realsrc != IMSRC_Graphic) && (realsrc != IMSRC_Sprite));
 
 	strcpy(rim->name, name);

	rim->source_type = IMSRC_Dummy;
	rim->source_palette = -1;

	switch (realsrc)
	{
		case IMSRC_Texture:
		case IMSRC_SkyMerge:
			rim->source.dummy.fg = pal_black;
			rim->source.dummy.bg = pal_brown1;
			break;

		case IMSRC_Flat:
		case IMSRC_Raw320x200:
			rim->source.dummy.fg = pal_black;
			rim->source.dummy.bg = pal_green1;
			break;

		case IMSRC_Graphic:
		case IMSRC_Sprite:
			rim->source.dummy.fg = pal_yellow;
			rim->source.dummy.bg = TRANS_PIXEL;
			break;
    
		default:
			I_Error("AddImageDummy: bad realsrc value %d !\n", realsrc);
	}

	dummies.Insert(rim);

	return rim;
}

static real_image_t *AddImageGraphic(const char *name,
									 image_source_e type, int lump)
{
	/* used for Sprites too */

	patch_t *pat;
	int width, height, offset_x, offset_y;
  
	real_image_t *rim;

	pat = (patch_t *) W_CacheLumpNum(lump);
  
	width    = EPI_LE_S16(pat->width);
	height   = EPI_LE_S16(pat->height);
	offset_x = EPI_LE_S16(pat->leftoffset);
	offset_y = EPI_LE_S16(pat->topoffset);
  
	W_DoneWithLump(pat);

	// do some basic checks
	// !!! FIXME: identify lump types in wad code.
	if (width <= 0 || width > 2048 || height <= 0 || height > 512 ||
		ABS(offset_x) > 1024 || ABS(offset_y) > 1024)
	{
		// check for Heretic/Hexen images, which are raw 320x200 
		int length = W_LumpLength(lump);

		if (length == 320*200 && type == IMSRC_Graphic)
		{
			rim = NewImage(320, 200, true);
			strcpy(rim->name, name);

			rim->source_type = IMSRC_Raw320x200;
			rim->source.flat.lump = lump;
			rim->source_palette = W_GetPaletteForLump(lump);
			return rim;
		}

		if (length == 64*64 || length == 64*65 || length == 64*128)
			I_Warning("Graphic '%s' seems to be a flat.\n", name);
		else
			I_Warning("Graphic '%s' does not seem to be a graphic.\n", name);

		return NULL;
	}
 
	// create new image
	rim = NewImage(width, height, false);
 
	rim->pub.offset_x = offset_x;
	rim->pub.offset_y = offset_y;

	strcpy(rim->name, name);

	rim->source_type = type;
	rim->source.graphic.lump = lump;
	rim->source_palette = W_GetPaletteForLump(lump);

	if (type == IMSRC_Sprite)
		real_sprites.Insert(rim);
	else
		real_graphics.Insert(rim);

	return rim;
}

static real_image_t *AddImageTexture(const char *name, 
									 texturedef_t *tdef)
{
	real_image_t *rim;
 
	// assume it is non-solid, we'll update it when we know for sure
	rim = NewImage(tdef->width, tdef->height, false);
 
	strcpy(rim->name, name);

	rim->source_type = IMSRC_Texture;
	rim->source.texture.tdef = tdef;
	rim->source_palette = tdef->palette_lump;

	real_textures.Insert(rim);

	return rim;
}

static real_image_t *AddImageFlat(const char *name, int lump)
{
	real_image_t *rim;
	int len, size;
  
	len = W_LumpLength(lump);
  
	switch (len)
	{
		case 64 * 64: size = 64; break;

		// support for odd-size Heretic flats
		case 64 * 65: size = 64; break;
  
		// support for odd-size Hexen flats
		case 64 * 128: size = 64; break;
  
		// -- EDGE feature: bigger than normal flats --  
		case 128 * 128: size = 128; break;
		case 256 * 256: size = 256; break;
		case 512 * 512: size = 512; break;
		case 1024 * 1024: size = 1024; break;
    
		default:
			return NULL;
	}
   
	rim = NewImage(size, size, true);
 
	strcpy(rim->name, name);

	rim->source_type = IMSRC_Flat;
	rim->source.flat.lump = lump;
	rim->source_palette = W_GetPaletteForLump(lump);

	real_flats.Insert(rim);

	return rim;
}

static real_image_t *AddImageSkyMerge(const image_t *sky, int face, int size)
{
	static const char *face_names[6] = 
	{
		"SKYBOX_NORTH", "SKYBOX_EAST",
		"SKYBOX_SOUTH", "SKYBOX_WEST",
		"SKYBOX_TOP",   "SKYBOX_BOTTOM"
	};

	real_image_t *rim;
 
	rim = NewImage(size, size, true /* solid */);
 
	strcpy(rim->name, face_names[face]);

	rim->source_type = IMSRC_SkyMerge;
	rim->source.merge.sky = sky;
	rim->source.merge.face = face;
	rim->source_palette = -1;

	sky_merges.Insert(rim);

	return rim;
}

static real_image_t *AddImageUser(imagedef_c *def)
{
	int w, h;
	bool solid;

	switch (def->type)
	{
		case IMGDT_Colour:
			w = h = 8;
			solid = true;
			break;

		case IMGDT_Builtin:
			w = h = (detail_level == 2) ? 512 : 256;
			solid = false;
			break;

		case IMGDT_File:
		case IMGDT_Lump:
		{
			const char *basename = def->name.GetString();

			epi::file_c *f = OpenUserFileOrLump(def);

			if (! f)
			{
				I_Warning("Unable to add image %s: %s\n", 
					(def->type == IMGDT_Lump) ? "lump" : "file", basename);
				return NULL;
			}

			bool got_info;

			if (def->format == LIF_JPEG)
				got_info = epi::JPEG::GetInfo(f, &w, &h, &solid);
			else
				got_info = epi::PNG::GetInfo(f, &w, &h, &solid);

			if (! got_info)
				I_Error("Error occurred scanning image: %s\n", basename);

			CloseUserFileOrLump(def, f);
#if 1
			L_WriteDebug("GETINFO [%s] : size %dx%d\n", def->ddf.name.GetString(), w, h);
#endif
		}
		break;

		default:
			I_Error("AddImageUser: Coding error, unknown type %d\n", def->type);
			return NULL; /* NOT REACHED */
	}
 
	real_image_t *rim = NewImage(w, h, solid);
 
	rim->pub.offset_x = def->x_offset;
	rim->pub.offset_y = def->y_offset;

	rim->pub.scale_x = def->scale * def->aspect;
	rim->pub.scale_y = def->scale;

	strcpy(rim->name, def->ddf.name.GetString());

	/* FIX NAME : replace space with '_' */
	for (int i = 0; rim->name[i]; i++)
		if (rim->name[i] == ' ')
			rim->name[i] = '_';

	rim->source_type = IMSRC_User;
	rim->source.user.def = def;

	if (def->special & IMGSP_Crosshair)
	{
		float dy = (200.0f - rim->pub.actual_h * rim->pub.scale_y) / 2.0f - WEAPONTOP;

		rim->pub.offset_y += int(dy / rim->pub.scale_y);
	}

	switch (def->belong)
	{
		case INS_Graphic: real_graphics.Insert(rim); break;
		case INS_Texture: real_textures.Insert(rim); break;
		case INS_Flat:    real_flats.   Insert(rim); break;
		case INS_Sprite:  real_sprites. Insert(rim); break;

		default: I_Error("Bad belong value.\n");  // FIXME; throw error_c
	}

	return rim;
}

//
// W_ImageCreateFlats
//
// Used to fill in the image array with flats from the WAD.  The set
// of lumps is those that occurred between F_START and F_END in each
// existing wad file, with duplicates set to -1.
//
// NOTE: should only be called once, as it assumes none of the flats
// in the list have names colliding with existing flat images.
// 
void W_ImageCreateFlats(int *lumps, int number)
{
	int i;

	DEV_ASSERT2(lumps);

	for (i=0; i < number; i++)
	{
		if (lumps[i] < 0)
			continue;
    
		AddImageFlat(W_GetLumpName(lumps[i]), lumps[i]);
	}
}

//
// W_ImageCreateTextures
//
// Used to fill in the image array with textures from the WAD.  The
// list of texture definitions comes from each TEXTURE1/2 lump in each
// existing wad file, with duplicates set to NULL.
//
// NOTE: should only be called once, as it assumes none of the
// textures in the list have names colliding with existing texture
// images.
// 
void W_ImageCreateTextures(struct texturedef_s ** defs, int number)
{
	int i;

	DEV_ASSERT2(defs);

	for (i=0; i < number; i++)
	{
		if (defs[i] == NULL)
			continue;
    
		AddImageTexture(defs[i]->name, defs[i]);
	}
}

//
// W_ImageCreateSprite
// 
// Used to fill in the image array with sprites from the WAD.  The
// lumps come from those occurring between S_START and S_END markers
// in each existing wad.
//
// NOTE: it is assumed that each new sprite is unique i.e. the name
// does not collide with any existing sprite image.
// 
const image_t *W_ImageCreateSprite(const char *name, int lump, bool is_weapon)
{
	DEV_ASSERT2(lump >= 0);

	real_image_t *rim = AddImageGraphic(name, IMSRC_Sprite, lump);
	if (! rim)
		return NULL;

	// adjust sprite offsets so that (0,0) is normal
	if (is_weapon)
	{
		rim->pub.offset_x += (320 / 2 - rim->pub.actual_w / 2);  // loss of accuracy
		rim->pub.offset_y += (200 - 32 - rim->pub.actual_h);
	}
	else
	{
		rim->pub.offset_x -= rim->pub.actual_w / 2;   // loss of accuracy
		rim->pub.offset_y -= rim->pub.actual_h;
	}

	return &rim->pub;
}

//
// W_CreateUserImages
// 
// Add the images defined in IMAGES.DDF.
//
void W_ImageCreateUser(void)
{
	for (int i = 0; i < imagedefs.GetSize(); i++)
	{
		imagedef_c* def = imagedefs[i];

		if (def)
			AddImageUser(def);

		E_LocalProgress(i, imagedefs.GetSize());
	}

#if 0
	L_WriteDebug("Textures -----------------------------\n");
	real_textures.DebugDump();

	L_WriteDebug("Flats ------------------------------\n");
	real_flats.DebugDump();

	L_WriteDebug("Sprites ------------------------------\n");
	real_sprites.DebugDump();

	L_WriteDebug("Graphics ------------------------------\n");
	real_graphics.DebugDump();
#endif
}

//
// W_ImageGetUserSprites
//
// Only used during sprite initialisation.  The returned array of
// images is guaranteed to be sorted by name.
//
// Use delete[] to free the returned array.
//
const image_t ** W_ImageGetUserSprites(int *count)
{
	// count number of user sprites
	(*count) = 0;

	epi::array_iterator_c it;

	for (it=real_sprites.GetBaseIterator(); it.IsValid(); it++)
	{
		real_image_t *rim = ITERATOR_TO_TYPE(it, real_image_t*);
    
		if (rim->source_type == IMSRC_User)
			(*count) += 1;
	}

	if (*count == 0)
	{
		L_WriteDebug("W_ImageGetUserSprites(count = %d)\n", *count);
		return NULL;
	}

	const image_t ** array = new const image_t *[*count];
	int pos = 0;

	for (it=real_sprites.GetBaseIterator(); it.IsValid(); it++)
	{
		real_image_t *rim = ITERATOR_TO_TYPE(it, real_image_t*);
    
		if (rim->source_type == IMSRC_User)
			array[pos++] = &rim->pub;
	}

#define CMP(a, b)  (strcmp(W_ImageGetName(a), W_ImageGetName(b)) < 0)
	QSORT(const image_t *, array, (*count), CUTOFF);
#undef CMP

#if 0  // DEBUGGING
	L_WriteDebug("W_ImageGetUserSprites(count = %d)\n", *count);
	L_WriteDebug("{\n");

	for (pos = 0; pos < *count; pos++)
		L_WriteDebug("   %p = [%s] %dx%d\n", array[pos], W_ImageGetName(array[pos]),
			array[pos]->actual_w, array[pos]->actual_h);
		
	L_WriteDebug("}\n");
#endif

	return array;
}


//----------------------------------------------------------------------------

//
//  UTILITY
//

#define PIXEL_RED(pix)  (what_palette[pix*3 + 0])
#define PIXEL_GRN(pix)  (what_palette[pix*3 + 1])
#define PIXEL_BLU(pix)  (what_palette[pix*3 + 2])

#define GAMMA_RED(pix)  GAMMA_CONV(PIXEL_RED(pix))
#define GAMMA_GRN(pix)  GAMMA_CONV(PIXEL_GRN(pix))
#define GAMMA_BLU(pix)  GAMMA_CONV(PIXEL_BLU(pix))

//
// DrawColumnIntoBlock
//
// Clip and draw an old-style column from a patch into a block.
//
static void DrawColumnIntoEpiBlock(real_image_t *rim, epi::basicimage_c *img,
   const column_t *patchcol, int x, int y)
{
	DEV_ASSERT2(patchcol);

	int w1 = rim->pub.actual_w;
	int h1 = rim->pub.actual_h;
	int w2 = rim->pub.total_w;
//	int h2 = rim->pub.total_h;

	// clip horizontally
	if (x < 0 || x >= w1)
		return;

	while (patchcol->topdelta != P_SENTINEL)
	{
		int top = y + (int) patchcol->topdelta;
		int count = patchcol->length;

		byte *src = (byte *) patchcol + 3;
		byte *dest = img->pixels + x;

		if (top < 0)
		{
			count += top;
			top = 0;
		}

		if (top + count > h1)
			count = h1 - top;

		// copy the pixels, remapping any TRANS_PIXEL values
		for (; count > 0; count--, src++, top++)
		{
			if (*src == TRANS_PIXEL)
				dest[top * w2] = pal_black;
			else
				dest[top * w2] = *src;
		}

		patchcol = (const column_t *) ((const byte *) patchcol + 
									   patchcol->length + 4);
	}
}

//
// CheckBlockSolid
//
// FIXME: Avoid future checks.
//
#define MAX_STRAY_PIXELS  2

static void CheckEpiBlockSolid(real_image_t *rim, epi::basicimage_c *img)
{
	DEV_ASSERT2(img->bpp == 1);

	int w1 = rim->pub.actual_w;
	int h1 = rim->pub.actual_h;
	int w2 = rim->pub.total_w;
	int h2 = rim->pub.total_h;

	int total_num = w1 * h1;
	int stray_count=0;

	int x, y;

	for (x=0; x < w1; x++)
		for (y=0; y < h1; y++)
		{
			byte src_pix = img->pixels[y * img->width + x];

			if (src_pix != TRANS_PIXEL)
				continue;

			stray_count++;

			// only ignore stray pixels on large images
			if (total_num < 256 || stray_count > MAX_STRAY_PIXELS)
				return;
		}

	// image is totally solid.  Blacken any transparent parts.
	rim->pub.img_solid = true;

	for (x=0; x < w2; x++)
		for (y=0; y < h2; y++)
			if (x >= w1 || y >= h1 ||
				img->pixels[y * img->width + x] == TRANS_PIXEL)
			{
				img->pixels[y * img->width + x] = pal_black;
			}
}



//----------------------------------------------------------------------------

//
//  GL UTILITIES
//

static byte *ShrinkGetBuffer(int required)
{
	if (img_shrink_buffer && img_shrink_buf_size >= required)
		return img_shrink_buffer;
	
	if (img_shrink_buffer)
		delete[] img_shrink_buffer;
	
	img_shrink_buffer   = new byte[required];
	img_shrink_buf_size = required;

	return img_shrink_buffer;
}

//
// ShrinkBlockRGBA
//
// Just like ShrinkBlock() above, but the returned format is RGBA.
// Source format is column-major (i.e. normal block), whereas result
// is row-major (ano note that GL textures are _bottom up_ rather than
// the usual top-down ordering).  The new size should be scaled down
// to fit into glmax_tex_size.
//
static byte *ShrinkBlockRGBA(byte *src, int total_w, int total_h,
							 int new_w, int new_h, const byte *what_palette)
{
	byte *dest;
 
	int x, y, dx, dy;
	int step_x, step_y;

	DEV_ASSERT2(new_w > 0);
	DEV_ASSERT2(new_h > 0);
	DEV_ASSERT2(new_w <= glmax_tex_size);
	DEV_ASSERT2(new_h <= glmax_tex_size);
	DEV_ASSERT2((total_w % new_w) == 0);
	DEV_ASSERT2((total_h % new_h) == 0);

	dest = ShrinkGetBuffer(new_w * new_h * 4);

	step_x = total_w / new_w;
	step_y = total_h / new_h;

	// faster method for the usual case (no shrinkage)

	if (step_x == 1 && step_y == 1)
	{
		for (y=0; y < total_h; y++)
			for (x=0; x < total_w; x++)
			{
				byte src_pix = src[y * total_w + x];
				byte *dest_pix = dest + (((total_h-1-y) * total_w + x) * 4);

				if (src_pix == TRANS_PIXEL)
				{
					dest_pix[0] = dest_pix[1] = dest_pix[2] = dest_pix[3] = 0;
				}
				else
				{
					dest_pix[0] = GAMMA_RED(src_pix);
					dest_pix[1] = GAMMA_GRN(src_pix);
					dest_pix[2] = GAMMA_BLU(src_pix);
					dest_pix[3] = 255;
				}
			}
		return dest;
	}

	// slower method, as we must shrink the bugger...

	for (y=0; y < new_h; y++)
		for (x=0; x < new_w; x++)
		{
			byte *dest_pix = dest + (((new_h-1-y) * new_w + x) * 4);

			int px = x * step_x;
			int py = y * step_y;

			int tot_r=0, tot_g=0, tot_b=0, a_count=0, alpha;
			int total = step_x * step_y;

			// compute average colour of block
			for (dx=0; dx < step_x; dx++)
				for (dy=0; dy < step_y; dy++)
				{
					byte src_pix = src[(py+dy) * total_w + (px+dx)];

					if (src_pix == TRANS_PIXEL)
						a_count++;
					else
					{
						tot_r += GAMMA_RED(src_pix);
						tot_g += GAMMA_GRN(src_pix);
						tot_b += GAMMA_BLU(src_pix);
					}
				}

			if (a_count >= total)
			{
				// all pixels were translucent.  Keep r/g/b as zero.
				alpha = 0;
			}
			else
			{
				alpha = (total - a_count) * 255 / total;

				total -= a_count;

				tot_r /= total;
				tot_g /= total;
				tot_b /= total;
			}

			dest_pix[0] = tot_r;
			dest_pix[1] = tot_g;
			dest_pix[2] = tot_b;
			dest_pix[3] = alpha;
		}

	return dest;
}

static byte *ShrinkNormalRGB(byte *rgb, int total_w, int total_h,
							  int new_w, int new_h)
{
	byte *dest;
 
	int i, x, y, dx, dy;
	int step_x, step_y;

	DEV_ASSERT2(new_w > 0);
	DEV_ASSERT2(new_h > 0);
	DEV_ASSERT2(new_w <= glmax_tex_size);
	DEV_ASSERT2(new_h <= glmax_tex_size);
	DEV_ASSERT2((total_w % new_w) == 0);
	DEV_ASSERT2((total_h % new_h) == 0);

	dest = ShrinkGetBuffer(new_w * new_h * 3);

	step_x = total_w / new_w;
	step_y = total_h / new_h;

	// faster method for the usual case (no shrinkage)

	if (step_x == 1 && step_y == 1)
	{
		for (y=0; y < total_h; y++)
			for (x=0; x < total_w; x++)
				for (i=0; i < 3; i++)
					dest[(y * total_w + x) * 3 + i] = GAMMA_CONV(
						rgb[(y * total_w + x) * 3 + i]);
		return dest;
	}

	// slower method, as we must shrink the bugger...

	for (y=0; y < new_h; y++)
		for (x=0; x < new_w; x++)
		{
			byte *dest_pix = dest + ((y * new_w + x) * 3);

			int px = x * step_x;
			int py = y * step_y;

			int tot_r=0, tot_g=0, tot_b=0;
			int total = step_x * step_y;

			// compute average colour of block
			for (dx=0; dx < step_x; dx++)
				for (dy=0; dy < step_y; dy++)
				{
					byte *src_pix = rgb + (((py+dy) * total_w + (px+dx)) * 3);

					tot_r += GAMMA_CONV(src_pix[0]);
					tot_g += GAMMA_CONV(src_pix[1]);
					tot_b += GAMMA_CONV(src_pix[2]);
				}

			dest_pix[0] = tot_r / total;
			dest_pix[1] = tot_g / total;
			dest_pix[2] = tot_b / total;
		}

	return dest;
}

static byte *ShrinkNormalRGBA(byte *rgba, int total_w, int total_h,
							  int new_w, int new_h)
{
	byte *dest;
 
	int i, x, y, dx, dy;
	int step_x, step_y;

	DEV_ASSERT2(new_w > 0);
	DEV_ASSERT2(new_h > 0);
	DEV_ASSERT2(new_w <= glmax_tex_size);
	DEV_ASSERT2(new_h <= glmax_tex_size);
	DEV_ASSERT2((total_w % new_w) == 0);
	DEV_ASSERT2((total_h % new_h) == 0);

	dest = ShrinkGetBuffer(new_w * new_h * 4);

	step_x = total_w / new_w;
	step_y = total_h / new_h;

	// faster method for the usual case (no shrinkage)

	if (step_x == 1 && step_y == 1)
	{
		for (y=0; y < total_h; y++)
			for (x=0; x < total_w; x++)
				for (i=0; i < 4; i++)
					dest[(y * total_w + x) * 4 + i] = GAMMA_CONV(
						rgba[(y * total_w + x) * 4 + i]);
		return dest;
	}

	// slower method, as we must shrink the bugger...

	for (y=0; y < new_h; y++)
		for (x=0; x < new_w; x++)
		{
			byte *dest_pix = dest + ((y * new_w + x) * 4);

			int px = x * step_x;
			int py = y * step_y;

			int tot_r=0, tot_g=0, tot_b=0, tot_a=0;
			int total = step_x * step_y;

			// compute average colour of block
			for (dx=0; dx < step_x; dx++)
				for (dy=0; dy < step_y; dy++)
				{
					byte *src_pix = rgba + (((py+dy) * total_w + (px+dx)) * 4);

					tot_r += GAMMA_CONV(src_pix[0]);
					tot_g += GAMMA_CONV(src_pix[1]);
					tot_b += GAMMA_CONV(src_pix[2]);
					tot_a += GAMMA_CONV(src_pix[3]);
				}

			dest_pix[0] = tot_r / total;
			dest_pix[1] = tot_g / total;
			dest_pix[2] = tot_b / total;
			dest_pix[3] = tot_a / total;
		}

	return dest;
}

#if 0
static rgbcol_t ExtractImageHue(byte *src, int w, int h, int bpp)
{
	int y_total[3];
	int y_num = 0;
	memset(y_total, 0, sizeof(y_total));

	for (int y = 0; y < h; y++)
	{
		int x_total[3];
		int x_num = 0;
		memset(x_total, 0, sizeof(x_total));

		for (int x = 0; x < w; x++, src += bpp)
		{
			int r = src[0];
			int g = src[1];
			int b = src[2];

			if (bpp == 4 && src[3] < 255)
			{
				int a = src[3];

				if (a == 0) // skip blank areas
					continue;

				// blend against black
				r = (r * (1+a)) >> 8;
				g = (g * (1+a)) >> 8;
				b = (b * (1+a)) >> 8;
			}

			int v = MAX(r, MAX(g, b));
			int m = MIN(r, MIN(g, b));

			if (v == 0)  // ignore black
				continue;

			// weighting (based on saturation)
			v = 4 + ((v - m) * 28) / v;
			v = v * v;

			x_total[0] += r * (v << 4);
			x_total[1] += g * (v << 4);
			x_total[2] += b * (v << 4);

			x_num += v;
		}

		if (x_num > 0)
		{
			y_total[0] += x_total[0] / x_num;
			y_total[1] += x_total[1] / x_num;
			y_total[2] += x_total[2] / x_num;

			y_num++;
		}
	}

	if (y_num < 1)
		return RGB_NO_VALUE;

	int r = (y_total[0] / y_num) >> 4;
	int g = (y_total[1] / y_num) >> 4;
	int b = (y_total[2] / y_num) >> 4;

#if 1	// brighten up
	int v = MAX(r, MAX(g, b));
	if (v > 0)
	{
		r = r * 255 / v;
		g = g * 255 / v;
		b = b * 255 / v;
	}
#endif

	rgbcol_t hue = RGB_MAKE(r, g, b);

	if (hue == RGB_NO_VALUE)
		hue ^= RGB_MAKE(1,1,1);

	return hue;
}
#endif


//
// W_SendGLTexture
//
// Send the texture data to the GL, and returns the texture ID
// assigned to it.  The format of the data must be a normal block if
// palette-indexed pixels.
//
static GLuint minif_modes[2*3] =
{
	GL_NEAREST,
	GL_NEAREST_MIPMAP_NEAREST,
	GL_NEAREST_MIPMAP_LINEAR,
  
	GL_LINEAR,
	GL_LINEAR_MIPMAP_NEAREST,
	GL_LINEAR_MIPMAP_LINEAR
};

static GLuint W_SendGLTexture(epi::basicimage_c *img,
					   bool clamp, bool nomip, bool smooth,
					   int max_pix, const byte *what_palette)
{
  	int total_w = img->width;
	int total_h = img->height;

	int new_w, new_h;

	// scale down, if necessary, to fix the maximum size
	for (new_w = total_w; new_w > glmax_tex_size; new_w /= 2)
	{ /* nothing here */ }

	for (new_h = total_h; new_h > glmax_tex_size; new_h /= 2)
	{ /* nothing here */ }

	while (new_w * new_h > max_pix)
	{
		if (new_h >= new_w)
			new_h /= 2;
		else
			new_w /= 2;
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	GLuint id;

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	int tmode = GL_REPEAT;

	if (clamp)
		tmode = glcap_edgeclamp ? GL_CLAMP_TO_EDGE : GL_CLAMP;

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, tmode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tmode);

	// magnification mode
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
					smooth ? GL_LINEAR : GL_NEAREST);

	// minification mode
	use_mipmapping = MIN(2, MAX(0, use_mipmapping));

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
					minif_modes[(smooth ? 3 : 0) +
							   (nomip ? 0 : use_mipmapping)]);

	for (int mip=0; ; mip++)
	{
		byte *rgba_src = 0;

		switch (img->bpp)
		{
			case 1:
				rgba_src = ShrinkBlockRGBA(img->pixels, total_w, total_h,
					new_w, new_h, what_palette);
				break;

			case 3:
				rgba_src = ShrinkNormalRGB(img->pixels, total_w, total_h, new_w, new_h);
				break;
    
			case 4:
				rgba_src = ShrinkNormalRGBA(img->pixels, total_w, total_h, new_w, new_h);
				break;
		}

		DEV_ASSERT2(rgba_src);
    
		glTexImage2D(GL_TEXTURE_2D, mip, (img->bpp == 3) ? GL_RGB : GL_RGBA,
					 new_w, new_h, 0 /* border */,
					 (img->bpp == 3) ? GL_RGB : GL_RGBA,
					 GL_UNSIGNED_BYTE, rgba_src);
#if 0
		if (mip == 0)
			(*hue) = ExtractImageHue(rgba_src, new_w, new_h, (img->bpp == 3) ? 3 : 4);
#endif
		// (cannot free rgba_src, since it == img_shrink_buffer)

		// stop if mipmapping disabled or we have reached the end
		if (nomip || !use_mipmapping || (new_w == 1 && new_h == 1))
			break;

		new_w = MAX(1, new_w / 2);
		new_h = MAX(1, new_h / 2);

		// -AJA- 2003/12/05: workaround for Radeon 7500 driver bug, which
		//       incorrectly draws the 1x1 mip texture as black.
#ifndef WIN32
		if (new_w == 1 && new_h == 1)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mip);
#endif
	}

	return id;
}

static void FontWhitenRGBA(epi::basicimage_c *img)
{
	for (int y = 0; y < img->height; y++)
		for (int x = 0; x < img->width; x++)
		{
			u8_t *cur = img->PixelAt(x, y);

			int ity = MAX(cur[0], MAX(cur[1], cur[2]));

			// ity = ((ity << 7) + cur[0] * 38 + cur[1] * 64 + cur[2] * 26) >> 8;
			
			cur[0] = cur[1] = cur[2] = ity; 
		}
}

static void PaletteRemapRGBA(epi::basicimage_c *img,
	const byte *new_pal, const byte *old_pal)
{
	const int max_prev = 32;

	// cache of previously looked-up colours (in pairs)
	u8_t previous[max_prev * 6];
	int num_prev = 0;

	for (int y = 0; y < img->height; y++)
		for (int x = 0; x < img->width; x++)
		{
			u8_t *cur = img->PixelAt(x, y);

			// skip completely transparent pixels
			if (img->bpp == 4 && cur[3] == 0)
				continue;

			// optimisation: if colour matches previous one, don't need
			// to compute the remapping again.
			int i;
			for (i = 0; i < num_prev; i++)
				if (previous[i*6+0] == cur[0] &&
				    previous[i*6+1] == cur[1] &&
				    previous[i*6+2] == cur[2])
				{
					break;
				}

			if (i < num_prev)
			{
				// move to front (Most Recently Used)
				if (i != 0)
				{
					u8_t tmp[6];

					memcpy(tmp, previous, 6);
					memcpy(previous, previous + i*6, 6);
					memcpy(previous + i*6, tmp, 6);
				}

				cur[0] = previous[3];
				cur[1] = previous[4];
				cur[2] = previous[5];

				continue;
			}

			if (num_prev < max_prev)
			{
				memmove(previous+6, previous, num_prev*6);
				num_prev++;
			}

			// most recent lookup is at the head
			previous[0] = cur[0];
			previous[1] = cur[1];
			previous[2] = cur[2];

			int best = 0;
			int best_dist = (1 << 30);

			int R = int(cur[0]);
			int G = int(cur[1]);
			int B = int(cur[2]);

			for (int p = 0; p < 256; p++)
			{
				int dR = int(old_pal[p*3+0]) - R;
				int dG = int(old_pal[p*3+1]) - G;
				int dB = int(old_pal[p*3+2]) - B;

				int dist = dR * dR + dG * dG + dB * dB;

				if (dist < best_dist)
				{
					best_dist = dist;
					best = p;
				}
			}

			// if this colour is not affected by the colourmap, then
			// keep the original colour (which has more precision).
			if (old_pal[best*3+0] != new_pal[best*3+0] ||
			    old_pal[best*3+1] != new_pal[best*3+1] ||
			    old_pal[best*3+2] != new_pal[best*3+2])
			{
				cur[0] = new_pal[best*3+0];
				cur[1] = new_pal[best*3+1];
				cur[2] = new_pal[best*3+2];
			}

			previous[3] = cur[0];
			previous[4] = cur[1];
			previous[5] = cur[2];
		}
}


//----------------------------------------------------------------------------

//
//  BLOCK READING STUFF
//

//
// ReadFlatAsBlock
//
// Loads a flat from the wad and returns the image block for it.
// Doesn't do any mipmapping (this is too "raw" if you follow).
//
static epi::basicimage_c *ReadFlatAsEpiBlock(real_image_t *rim)
{
	DEV_ASSERT2(rim->source_type == IMSRC_Flat ||
				rim->source_type == IMSRC_Raw320x200);

	int tw = MAX(rim->pub.total_w, 1);
	int th = MAX(rim->pub.total_h, 1);

	int w = rim->pub.actual_w;
	int h = rim->pub.actual_h;

	epi::basicimage_c *img = new epi::basicimage_c(tw, th, 1);

	byte *dest = img->pixels;

#ifdef MAKE_TEXTURES_WHITE
	memset(dest, pal_white, tw * th);
	return img;
#endif

	// clear initial image to black
	memset(dest, pal_black, tw * th);

	// read in pixels
	const byte *src = (const byte*)W_CacheLumpNum(rim->source.flat.lump);

	for (int y=0; y < h; y++)
		for (int x=0; x < w; x++)
		{
			byte src_pix = src[y * w + x];

			byte *dest_pix = &dest[x + y * tw];

			// make sure TRANS_PIXEL values (which do not occur naturally in
			// Doom images) are properly remapped.
			if (src_pix != TRANS_PIXEL)
				dest_pix[0] = src_pix;
		}

	W_DoneWithLump(src);

	return img;
}

//
// ReadTextureAsBlock
//
// Loads a texture from the wad and returns the image block for it.
// Doesn't do any mipmapping (this is too "raw" if you follow).
// This routine will also update the `solid' flag if texture turns
// out to be solid.
//
static epi::basicimage_c *ReadTextureAsEpiBlock(real_image_t *rim)
{
	DEV_ASSERT2(rim->source_type == IMSRC_Texture);

	texturedef_t *tdef = rim->source.texture.tdef;
	DEV_ASSERT2(tdef);

	int tw = rim->pub.total_w;
	int th = rim->pub.total_h;

	epi::basicimage_c *img = new epi::basicimage_c(tw, th, 1);

	byte *dest = img->pixels;

#ifdef MAKE_TEXTURES_WHITE
	memset(dest, pal_white, tw * th);
	return img;
#endif

	// Clear initial pixels to either totally transparent, or totally
	// black (if we know the image should be solid).  If the image turns
	// out to be solid instead of transparent, the transparent pixels
	// will be blackened.
  
	if (rim->pub.img_solid)
		memset(dest, pal_black, (tw * th));
	else
		memset(dest, TRANS_PIXEL, (tw * th));

	int i;
	texpatch_t *patch;

	// Composite the columns into the block.
	for (i=0, patch=tdef->patches; i < tdef->patchcount; i++, patch++)
	{
		const patch_t *realpatch = (const patch_t*)W_CacheLumpNum(patch->patch);

		int realsize = W_LumpLength(patch->patch);

		int x1 = patch->originx;
		int y1 = patch->originy;
		int x2 = x1 + EPI_LE_S16(realpatch->width);

		int x = MAX(0, x1);

		x2 = MIN(tdef->width, x2);

		for (; x < x2; x++)
		{
			int offset = EPI_LE_S32(realpatch->columnofs[x - x1]);

			if (offset < 0 || offset >= realsize)
				I_Error("Bad image offset 0x%08x in image [%s]\n", offset, rim->name);

			const column_t *patchcol = (const column_t *)
				((const byte *) realpatch + offset);

			DrawColumnIntoEpiBlock(rim, img, patchcol, x, y1);
		}

		W_DoneWithLump(realpatch);
	}

	// update solid flag, if needed
	if (! rim->pub.img_solid)
		CheckEpiBlockSolid(rim, img);

	return img;
}

//
// ReadPatchAsBlock
//
// Loads a patch from the wad and returns the image block for it.
// Very similiar to ReadTextureAsBlock() above.  Doesn't do any
// mipmapping (this is too "raw" if you follow).  This routine will
// also update the `solid' flag if it turns out to be 100% solid.
//
static epi::basicimage_c *ReadPatchAsEpiBlock(real_image_t *rim)
{
	DEV_ASSERT2(rim->source_type == IMSRC_Graphic ||
				rim->source_type == IMSRC_Sprite);

	int tw = rim->pub.total_w;
	int th = rim->pub.total_h;

	epi::basicimage_c *img = new epi::basicimage_c(tw, th, 1);

	byte *dest = img->pixels;

	// Clear initial pixels to either totally transparent, or totally
	// black (if we know the image should be solid).  If the image turns
	// out to be solid instead of transparent, the transparent pixels
	// will be blackened.
  
	if (rim->pub.img_solid)
		memset(dest, pal_black, tw * th);
	else
		memset(dest, TRANS_PIXEL, tw * th);

	// Composite the columns into the block.
	const patch_t *realpatch = (const patch_t*)W_CacheLumpNum(rim->source.graphic.lump);

	int realsize = W_LumpLength(rim->source.graphic.lump);

	DEV_ASSERT2(rim->pub.actual_w == EPI_LE_S16(realpatch->width));
	DEV_ASSERT2(rim->pub.actual_h == EPI_LE_S16(realpatch->height));
  
	for (int x=0; x < rim->pub.actual_w; x++)
	{
		int offset = EPI_LE_S32(realpatch->columnofs[x]);

		if (offset < 0 || offset >= realsize)
			I_Error("Bad image offset 0x%08x in image [%s]\n", offset, rim->name);

		const column_t *patchcol = (const column_t *)
			((const byte *) realpatch + offset);

		DrawColumnIntoEpiBlock(rim, img, patchcol, x, 0);
	}

	W_DoneWithLump(realpatch);

	// update solid flag, if needed
	if (! rim->pub.img_solid)
		CheckEpiBlockSolid(rim, img);

	return img;
}

static void CalcSphereCoord(int px, int py, int pw, int ph, int face,
	float *sx, float *sy, float *sz)
{
	float ax = ((float)px + 0.5f) / (float)pw * 2.0f - 1.0f;
	float ay = ((float)py + 0.5f) / (float)ph * 2.0f - 1.0f;

	ay = -ay;

	switch (face)
	{
		case WSKY_North:
			*sx = ax; *sy = 1.0f; *sz = ay; break;

		case WSKY_South:
			*sx = -ax; *sy = -1.0f; *sz = ay; break;

		case WSKY_East:
			*sx = 1.0f; *sy = -ax; *sz = ay; break;

		case WSKY_West:
			*sx = -1.0f; *sy = ax; *sz = ay; break;

		case WSKY_Top:
			*sx = ax; *sy = -ay; *sz = 1.0f; break;

		case WSKY_Bottom:
			*sx = ax; *sy = ay; *sz = -1.0f; break;

		default:
			*sx = *sy = *sz = 0; break;
	}

	// normalise the vector (FIXME: optimise the sqrt)
	float len = sqrt((*sx) * (*sx) + (*sy) * (*sy) + (*sz) * (*sz));

	if (len > 0)
	{
		*sx /= len; *sy /= len; *sz /= len;
	}
}

static inline bool SkyIsNarrow(const image_t *sky)
{
	// check the aspect of the image
	return (IM_WIDTH(sky) / IM_HEIGHT(sky)) < 2.28f;
}

//
// ReadSkyMergeAsBlock
//
static epi::basicimage_c *ReadSkyMergeAsEpiBlock(real_image_t *rim)
{
	DEV_ASSERT2(rim->source_type == IMSRC_SkyMerge);
	DEV_ASSERT2(rim->pub.actual_w == rim->pub.total_w);
	DEV_ASSERT2(rim->pub.actual_h == rim->pub.total_h);

	int tw = rim->pub.total_w;
	int th = rim->pub.total_h;

	// Yuck! Recursive call into image system. Hope nothing breaks...
	const image_t *sky = rim->source.merge.sky;
	real_image_t *sky_rim = (real_image_t *) sky; // Intentional Const Override

	// FIXME: may be wrong palette
	const byte *what_palette = (const byte *) &playpal_data[0];

	// big hack (see note near top of file)
	if (! merging_sky_image)
		merging_sky_image = ReadAsEpiBlock(sky_rim);

	epi::basicimage_c *sky_img = merging_sky_image;

	epi::basicimage_c *img = new epi::basicimage_c(tw, th, 3);

#if 0 // DEBUG
	I_Printf("SkyMerge: Image %p face %d\n", rim, rim->source.merge.face);
#endif
	bool narrow = SkyIsNarrow(sky);

	byte *src = sky_img->pixels;
	byte *dest = img->pixels;

	int sk_w = sky_img->width;
	int ds_w = img->width;
	int ds_h = img->height;

	for (int y=0; y < rim->pub.total_h; y++)
	for (int x=0; x < rim->pub.total_w; x++)
	{
		float sx, sy, sz;
		float tx, ty;

		CalcSphereCoord(x, y, rim->pub.total_w, rim->pub.total_h,
			rim->source.merge.face, &sx, &sy, &sz);

		RGL_CalcSkyCoord(sx, sy, sz, narrow, &tx, &ty);

		int TX = (int)(tx * sky_img->width  * 16);
		int TY = (int)(ty * sky_img->height * 16);

		TX = (TX + sky_img->width  * 64) % (sky_img->width  * 16);
		TY = (TY + sky_img->height * 64) % (sky_img->height * 16);

		if (TX < 0) TX = 0;
		if (TY < 0) TY = 0;

		TX = sky_img->width*16-1-TX;

		// FIXME: handle images everywhere with bottom-up coords
		if (sky_img->bpp >= 3) TY = sky_img->height*16-1-TY;

		int FX = TX % 16;
		int FY = TY % 16;

		TX = TX / 16;
		TY = TY / 16;
		
#if 0 // DEBUG
		if ((x==0 || x==rim->pub.total_w-1) && (y==0 || y==rim->pub.total_h-1))
		{
			I_Printf("At (%d,%d) : sphere (%1.2f,%1.2f,%1.2f)  tex (%1.4f,%1.4f)\n",
			x, y, sx, sy, sz, tx, ty);
		}
#endif

		// bilinear filtering

		int TY2 = (TY >= sky_img->height-1) ? TY : (TY+1);
		int TX2 = (TX + 1) % sky_img->width;

		byte rA, rB, rC, rD;
		byte gA, gB, gC, gD;
		byte bA, bB, bC, bD;

		switch (sky_img->bpp)
		{
			case 1:
			{
				byte src_A = src[TY  * sk_w + TX];
				byte src_B = src[TY  * sk_w + TX2];
				byte src_C = src[TY2 * sk_w + TX];
				byte src_D = src[TY2 * sk_w + TX2];

				rA = PIXEL_RED(src_A); rB = PIXEL_RED(src_B);
				rC = PIXEL_RED(src_C); rD = PIXEL_RED(src_D);

				gA = PIXEL_GRN(src_A); gB = PIXEL_GRN(src_B);
				gC = PIXEL_GRN(src_C); gD = PIXEL_GRN(src_D);

				bA = PIXEL_BLU(src_A); bB = PIXEL_BLU(src_B);
				bC = PIXEL_BLU(src_C); bD = PIXEL_BLU(src_D);
			}
			break;

			case 3:
			{
				rA = src[(TY * sk_w + TX) * 3 + 0];
				gA = src[(TY * sk_w + TX) * 3 + 1];
				bA = src[(TY * sk_w + TX) * 3 + 2];

				rB = src[(TY * sk_w + TX2) * 3 + 0];
				gB = src[(TY * sk_w + TX2) * 3 + 1];
				bB = src[(TY * sk_w + TX2) * 3 + 2];

				rC = src[(TY2 * sk_w + TX) * 3 + 0];
				gC = src[(TY2 * sk_w + TX) * 3 + 1];
				bC = src[(TY2 * sk_w + TX) * 3 + 2];

				rD = src[(TY2 * sk_w + TX2) * 3 + 0];
				gD = src[(TY2 * sk_w + TX2) * 3 + 1];
				bD = src[(TY2 * sk_w + TX2) * 3 + 2];
			}
			break;

			case 4:
			{
				rA = src[(TY * sk_w + TX) * 4 + 0];
				gA = src[(TY * sk_w + TX) * 4 + 1];
				bA = src[(TY * sk_w + TX) * 4 + 2];

				rB = src[(TY * sk_w + TX2) * 4 + 0];
				gB = src[(TY * sk_w + TX2) * 4 + 1];
				bB = src[(TY * sk_w + TX2) * 4 + 2];

				rC = src[(TY2 * sk_w + TX) * 4 + 0];
				gC = src[(TY2 * sk_w + TX) * 4 + 1];
				bC = src[(TY2 * sk_w + TX) * 4 + 2];

				rD = src[(TY2 * sk_w + TX2) * 4 + 0];
				gD = src[(TY2 * sk_w + TX2) * 4 + 1];
				bD = src[(TY2 * sk_w + TX2) * 4 + 2];
			}
			break;

			default:  // remove compiler warning
				rA = rB = rC = rD = 0;
				gA = gB = gC = gD = 0;
				bA = bB = bC = bD = 0;
				break;
		}

		int r = (int)rA * (15-FX) * (15-FY) +
				(int)rB * (   FX) * (15-FY) +
				(int)rC * (15-FX) * (   FY) +
				(int)rD * (   FX) * (   FY);

		int g = (int)gA * (15-FX) * (15-FY) +
				(int)gB * (   FX) * (15-FY) +
				(int)gC * (15-FX) * (   FY) +
				(int)gD * (   FX) * (   FY);

		int b = (int)bA * (15-FX) * (15-FY) +
				(int)bB * (   FX) * (15-FY) +
				(int)bC * (15-FX) * (   FY) +
				(int)bD * (   FX) * (   FY);

		r /= 225; g /= 225; b /= 225;

		int yy = ds_h - 1 - y;

		dest[(yy * ds_w + x) * 3 + 0] = r;
		dest[(yy * ds_w + x) * 3 + 1] = g;
		dest[(yy * ds_w + x) * 3 + 2] = b;
	}

	return img;
}


//
// ReadDummyAsBlock
//
// Creates a dummy image.
//
static epi::basicimage_c *ReadDummyAsEpiBlock(real_image_t *rim)
{
	DEV_ASSERT2(rim->source_type == IMSRC_Dummy);
	DEV_ASSERT2(rim->pub.actual_w == rim->pub.total_w);
	DEV_ASSERT2(rim->pub.actual_h == rim->pub.total_h);
	DEV_ASSERT2(rim->pub.total_w == DUMMY_X);
	DEV_ASSERT2(rim->pub.total_h == DUMMY_Y);

	int tw = rim->pub.total_w;
	int th = rim->pub.total_h;

	epi::basicimage_c *img = new epi::basicimage_c(tw, th, 1);

	byte *dest = img->pixels;

	// copy pixels
	for (int y=0; y < DUMMY_Y; y++)
		for (int x=0; x < DUMMY_X; x++)
		{
			byte src_pix = dummy_graphic[y * DUMMY_X + x];

			byte *dest_pix = dest + (y * rim->pub.total_w) + x;

			*dest_pix = src_pix ? rim->source.dummy.fg : rim->source.dummy.bg;
		}

	return img;
}

static void NormalizeClearAreas(epi::basicimage_c *img)
{
	// makes sure that any totally transparent pixel (alpha == 0)
	// has a colour of black.  This shows up when smoothing is on.

	DEV_ASSERT2(img->bpp == 4);

	byte *dest = img->pixels;

	for (int y = 0; y < img->height; y++)
	for (int x = 0; x < img->width;  x++)
	{
		if (dest[3] == 0)
		{
			dest[0] = dest[1] = dest[2] = 0;
		}

		dest += 4;
	}
}

static void CreateUserColourImage(epi::basicimage_c *img, imagedef_c *def)
{
	byte *dest = img->pixels;

	for (int y = 0; y < img->height; y++)
	for (int x = 0; x < img->width;  x++)
	{
		*dest++ = (def->colour & 0xFF0000) >> 16;  // R
		*dest++ = (def->colour & 0x00FF00) >>  8;  // G
		*dest++ = (def->colour & 0x0000FF);        // B

		if (img->bpp == 4)
			*dest++ = 0xFF;
	}
}

static void FourWaySymmetry(epi::basicimage_c *img)
{
	// the already-drawn corner has the lowest x and y values.  When
	// width or height is odd, the middle column/row must already be
	// drawn.

	int w2 = (img->width  + 1) / 2;
	int h2 = (img->height + 1) / 2;

	for (int y = 0; y < h2; y++)
	for (int x = 0; x < w2; x++)
	{
		int ix = img->width  - 1 - x;
		int iy = img->height - 1 - y;

		img->CopyPixel(x, y, ix,  y);
		img->CopyPixel(x, y,  x, iy);
		img->CopyPixel(x, y, ix, iy);
	}
}

static void EightWaySymmetry(epi::basicimage_c *img)
{
	// Note: the corner already drawn has lowest x and y values, and
	// the triangle piece is where x >= y.  The diagonal (x == y) must
	// already be drawn.

	DEV_ASSERT2(img->width == img->height);

	int hw = (img->width + 1) / 2;

	for (int y = 0;   y < hw; y++)
	for (int x = y+1; x < hw; x++)
	{
		img->CopyPixel(x, y, y, x);
	}

	FourWaySymmetry(img);
}

static void CreateUserBuiltinLinear(epi::basicimage_c *img, imagedef_c *def)
{
	DEV_ASSERT2(img->bpp == 4);
	DEV_ASSERT2(img->width == img->height);

	int hw = (img->width + 1) / 2;

	for (int y = 0; y < hw; y++)
	for (int x = y; x < hw; x++)
	{
		byte *dest = img->pixels + (y * img->width + x) * 4;

		float dx = (hw-1 - x) / float(hw);
		float dy = (hw-1 - y) / float(hw);

		float hor_p2 = dx * dx + dy * dy;
		float sq = 1.0f / sqrt(1.0f + DL_OUTER * DL_OUTER * hor_p2);

		// ramp intensity down to zero at the outer edge
		float horiz = sqrt(hor_p2);
		if (horiz > 0.80f)
			sq = sq * (0.98f - horiz) / (0.98f - 0.80f);

		int v = int(sq * 255.4f);

		if (v < 0 || 
			x == 0 || x == img->width-1 ||
			y == 0 || y == img->height-1)
		{
			v = 0;
		}

		dest[0] = dest[1] = dest[2] = v;
		dest[3] = 255;
	}

	EightWaySymmetry(img);
}

static void CreateUserBuiltinQuadratic(epi::basicimage_c *img, imagedef_c *def)
{
	DEV_ASSERT2(img->bpp == 4);
	DEV_ASSERT2(img->width == img->height);

	int hw = (img->width + 1) / 2;

	for (int y = 0; y < hw; y++)
	for (int x = y; x < hw; x++)
	{
		byte *dest = img->pixels + (y * img->width + x) * 4;

		float dx = (hw-1 - x) / float(hw);
		float dy = (hw-1 - y) / float(hw);

		float hor_p2 = dx * dx + dy * dy;

		float sq = 0.3f / (1.0f + DL_OUTER * hor_p2) +
		           0.7f * MIN(1.0f, 2.0f / (1.0f + DL_OUTER * 2.0f * hor_p2));

		// ramp intensity down to zero at the outer edge
		float horiz = sqrt(hor_p2);
		if (horiz > 0.80f)
			sq = sq * (0.98f - horiz) / (0.98f - 0.80f);

		int v = int(sq * 255.4f);

		if (v < 0 ||
			x == 0 || x == img->width-1 ||
			y == 0 || y == img->height-1)
		{
			v = 0;
		}

		dest[0] = dest[1] = dest[2] = v;
		dest[3] = 255;
	}

	EightWaySymmetry(img);
}

static void CreateUserBuiltinShadow(epi::basicimage_c *img, imagedef_c *def)
{
	DEV_ASSERT2(img->bpp == 4);
	DEV_ASSERT2(img->width == img->height);

	int hw = (img->width + 1) / 2;

	for (int y = 0; y < hw; y++)
	for (int x = y; x < hw; x++)
	{
		byte *dest = img->pixels + (y * img->width + x) * 4;

		float dx = (hw-1 - x) / float(hw);
		float dy = (hw-1 - y) / float(hw);

		float horiz = sqrt(dx * dx + dy * dy);
		float sq = 1.0f - horiz;

		int v = int(sq * 170.4f);

		if (v < 0 ||
			x == 0 || x == img->width-1 ||
			y == 0 || y == img->height-1)
		{
			v = 0;
		}

		*dest++ = 0;
		*dest++ = 0;
		*dest++ = 0;
		*dest   = v;
	}

	EightWaySymmetry(img);
}

static epi::file_c *OpenUserFileOrLump(imagedef_c *def)
{
	if (def->type == IMGDT_File)
	{
		// -AJA- 2005/01/15: filenames in DDF relative to GAMEDIR
		return M_OpenComposedEPIFile(game_dir.GetString(), def->name.GetString());
	}
	else  /* LUMP */
	{
		int lump = W_CheckNumForName(def->name.GetString());

		if (lump < 0)
			return NULL;

		const byte *lump_data = (byte *)W_CacheLumpNum(lump);
		int length = W_LumpLength(lump);

		return new epi::mem_file_c(lump_data, length, false /* no copying */);
	}
}

static void CloseUserFileOrLump(imagedef_c *def, epi::file_c *f)
{
	if (def->type == IMGDT_File)
	{
		epi::the_filesystem->Close(f);
	}
	else  /* LUMP */
	{
		// FIXME: create sub-class of mem_file_c in WAD code
		epi::mem_file_c *mf = (epi::mem_file_c *) f;
		W_DoneWithLump(mf->GetNonCopiedDataPointer());

		delete f;
	}
}

static epi::basicimage_c *CreateUserFileImage(real_image_t *rim, imagedef_c *def)
{
	epi::file_c *f = OpenUserFileOrLump(def);

	if (! f)
		I_Error("Missing image file: %s\n", def->name.GetString());

	epi::basicimage_c *img;

	if (def->format == LIF_JPEG)
		img = epi::JPEG::Load(f, true /* invert */, true /* round_pow2 */);
	else
		img = epi::PNG::Load (f, true /* invert */, true /* round_pow2 */);

	CloseUserFileOrLump(def, f);

	if (! img)
		I_Error("Error occurred loading image file: %s\n",
			def->name.GetString());

#if 1  // DEBUGGING
	L_WriteDebug("CREATE IMAGE [%s] %dx%d < %dx%d %s --> %p %dx%d bpp %d\n",
	rim->name,
	rim->pub.actual_w, rim->pub.actual_h,
	rim->pub.total_w, rim->pub.total_h,
	rim->pub.img_solid ? "SOLID" : "MASKED",
	img, img->width, img->height, img->bpp);
#endif

	if (img->bpp == 4)
		NormalizeClearAreas(img);

	DEV_ASSERT2(rim->pub.total_w == img->width);
	DEV_ASSERT2(rim->pub.total_h == img->height);

	return img;
}

//
// ReadUserAsBlock
//
// Loads or Creates the user defined image.
// Doesn't do any mipmapping (this is too "raw" if you follow).
//
static epi::basicimage_c *ReadUserAsEpiBlock(real_image_t *rim)
{
	DEV_ASSERT2(rim->source_type == IMSRC_User);

	int tw = MAX(rim->pub.total_w, 1);
	int th = MAX(rim->pub.total_h, 1);

	int bpp = rim->pub.img_solid ? 3 : 4;

	// clear initial image to black / transparent
	/// ALREADY DONE: memset(dest, pal_black, tw * th * bpp);

	imagedef_c *def = rim->source.user.def;

	switch (def->type)
	{
		case IMGDT_Colour:
		{
			epi::basicimage_c *img = new epi::basicimage_c(tw, th, bpp);
			CreateUserColourImage(img, def);
			return img;
		}

		case IMGDT_Builtin:
		{
			epi::basicimage_c *img = new epi::basicimage_c(tw, th, bpp);
			switch (def->builtin)
			{
				case BLTIM_Linear:
					CreateUserBuiltinLinear(img, def);
					break;

				case BLTIM_Quadratic:
					CreateUserBuiltinQuadratic(img, def);
					break;

				case BLTIM_Shadow:
					CreateUserBuiltinShadow(img, def);
					break;

				default:
					I_Error("ReadUserAsEpiBlock: Unknown builtin %d\n", def->builtin);
					break;
			}
			return img;
		}

		case IMGDT_File:
		case IMGDT_Lump:
		    return CreateUserFileImage(rim, def);

		default:
			I_Error("ReadUserAsEpiBlock: Coding error, unknown type %d\n", def->type);
	}

	return NULL;  /* NOT REACHED */
}

//
// ReadAsEpiBlock
//
// Read the image from the wad into an epi::basicimage class.
// The image returned is normally palettised (bpp == 1), and the
// palette must be determined from rim->source_palette.  Mainly
// just a switch to more specialised image readers.
//
// Never returns NULL.
//
static epi::basicimage_c *ReadAsEpiBlock(real_image_t *rim) 
{
	switch (rim->source_type)
	{
		case IMSRC_Flat:
		case IMSRC_Raw320x200:
			return ReadFlatAsEpiBlock(rim);

		case IMSRC_Texture:
			return ReadTextureAsEpiBlock(rim);

		case IMSRC_Graphic:
		case IMSRC_Sprite:
			return ReadPatchAsEpiBlock(rim);

		case IMSRC_SkyMerge:
			return ReadSkyMergeAsEpiBlock(rim);

		case IMSRC_Dummy:
			return ReadDummyAsEpiBlock(rim);
    
		case IMSRC_User:
			return ReadUserAsEpiBlock(rim);
      
		default:
			I_Error("ReadAsBlock: unknown source_type %d !\n", rim->source_type);
			return NULL;
	}
}


//----------------------------------------------------------------------------

//
//  IMAGE LOADING / UNLOADING
//


#if 0  // planned....
static INLINE
real_cached_image_t *LoadImageBlock(real_image_t *rim, int mip)
{
	// OPTIMISE: check if a blockified version at a lower mip already
	// exists, saving us the trouble to read the stuff from the WAD.
 
	real_cached_image_t *rc;
  
	rc = ReadAsBlock(rim, mip);

	DEV_ASSERT2(rc->mode == IMG_Block);

	rc->users++;
	InsertAtTail(rc);

	return rc;
}
#endif

static INLINE
real_cached_image_t *LoadImageOGL(real_image_t *rim, const colourmap_c *trans)
{
	static byte trans_pal[256 * 3];

	bool clamp  = false;
	bool nomip  = false;
	bool smooth = use_smoothing;
 
 	int max_pix = 65536 * (1 << (2 * detail_level));

	const byte *what_palette;
	bool what_pal_cached = false;

	if (rim->source_type == IMSRC_Graphic || rim->source_type == IMSRC_Raw320x200 ||
		rim->source_type == IMSRC_Sprite  || rim->source_type == IMSRC_SkyMerge)
	{
		clamp = true;
	}

	if (rim->source_type == IMSRC_User)
	{
		if (rim->source.user.def->special & IMGSP_Clamp)
			clamp = true;

		if (rim->source.user.def->special & IMGSP_NoMip)
			nomip = true;

		if (rim->source.user.def->special & IMGSP_Smooth)
			smooth = true;
	}

   	// the "SKY" check here is a hack...
   	if (strncasecmp(rim->name, "SKY", 3) == 0)
	{
		if (sky_stretch != STRETCH_ORIGINAL)
			smooth = true;

		max_pix *= 4;  // kludgy
	}
	else if (rim->source_type == IMSRC_Graphic ||
		     rim->source_type == IMSRC_Raw320x200 ||
			 (rim->source_type == IMSRC_User &&
		      rim->source.user.def->belong == INS_Graphic))
	{
		max_pix *= 4;  // fix for title screens
	}

	if (trans != NULL)
	{
		// Note: we don't care about source_palette here. It's likely that
		// the translation table itself would not match the other palette,
		// and so we would still end up with messed up colours.

		what_palette = trans_pal;

		// do the actual translation
		const byte *trans_table = V_GetTranslationTable(trans);

		for (int j = 0; j < 256; j++)
		{
			int k = trans_table[j];

			trans_pal[j*3 + 0] = playpal_data[0][k][0];
			trans_pal[j*3 + 1] = playpal_data[0][k][1];
			trans_pal[j*3 + 2] = playpal_data[0][k][2];
		}
	}
	else if (rim->source_palette < 0)
	{
		what_palette = (const byte *) &playpal_data[0];
	}
	else
	{
		what_palette = (const byte *) W_CacheLumpNum(rim->source_palette);
		what_pal_cached = true;
	}

	real_cached_image_t *rc = (real_cached_image_t *)
		Z_New(real_cached_image_t,1);

	// compute approximate size (including mipmaps)
	int size = sizeof(real_cached_image_t) +
		rim->pub.total_w * rim->pub.total_h * 16 / 3;

	rc->next = rc->prev = NULL;
	rc->parent = rim;
	rc->mip = 0;
	rc->users = 0;
	rc->invalidated = false;
	rc->mode = IMG_OGL;
	rc->trans_map = trans;
	rc->hue = RGB_NO_VALUE;
	rc->size = size;

	epi::basicimage_c *tmp_img = ReadAsEpiBlock(rim);

	if (var_hq_scale && (tmp_img->bpp == 1) &&
		(rim->source_type == IMSRC_Raw320x200 ||
		 rim->source_type == IMSRC_Graphic ||
		 (var_hq_all &&
		  (rim->source_type == IMSRC_Sprite ||
		   rim->source_type == IMSRC_Flat ||
		   rim->source_type == IMSRC_Texture))))
	{
		epi::Hq2x::Setup(&playpal_data[0][0][0], TRANS_PIXEL);

		epi::basicimage_c *scaled_img =
			epi::Hq2x::Convert(tmp_img, rim->pub.img_solid, true /* invert */);

		delete tmp_img;
		tmp_img = scaled_img;
	}

	if (tmp_img->bpp >= 3 && trans != NULL)
	{
		if (trans == font_whiten_map)
			FontWhitenRGBA(tmp_img);
		else
			PaletteRemapRGBA(tmp_img, what_palette, (const byte *) &playpal_data[0]);
	}

	rc->info.ogl.tex_id = W_SendGLTexture(tmp_img, clamp, nomip,
		smooth, max_pix, what_palette);

	delete tmp_img;

	if (what_pal_cached)
		W_DoneWithLump(what_palette);

	rc->users++;
	InsertAtTail(rc);

	return rc;
}


static INLINE 
void UnloadImageBlock(real_cached_image_t *rc, real_image_t *rim)
{
	// nothing to do, pixel data was allocated along with the
	// real_cached_image_t structure.
}

static
void UnloadImageOGL(real_cached_image_t *rc, real_image_t *rim)
{
	glDeleteTextures(1, &rc->info.ogl.tex_id);

	if (rc->trans_map == NULL)
	{
		rim->ogl_cache = NULL;
		return;
	}

	int i;

	for (i = 0; i < rim->trans_cache.num_trans; i++)
	{
		if (rim->trans_cache.trans[i] == rc)
		{
			rim->trans_cache.trans[i] = NULL;
			return;
		}
	}

	I_Error("INTERNAL ERROR: UnloadImageOGL: no such RC in trans_cache !\n");
}


//
// UnloadImage
//
// Unloads a cached image from the cache list and frees all resources.
// Mainly just a switch to more specialised image unloaders.
//
static void UnloadImage(real_cached_image_t *rc)
{
	real_image_t *rim = rc->parent;

	DEV_ASSERT2(rc);
	DEV_ASSERT2(rc != &imagecachehead);
	DEV_ASSERT2(rim);
	DEV_ASSERT2(rc->users == 0);

	// unlink from the cache list
	Unlink(rc);

	cache_size -= rc->size;

	switch (rc->mode)
	{
#if 0  // planned...
		case IMG_EpiBlock:
		{
			UnloadImageBlock(rc, rim);
			rim->block_cache.mips[rc->mip] = NULL;
			break;
		}
#endif

		case IMG_OGL:
		{
			UnloadImageOGL(rc, rim);
		}
		break;

		default:
			I_Error("UnloadImage: bad mode %d !\n", rc->mode);
	}

	// finally, free the rest of the mem
	Z_Free(rc);
}


//----------------------------------------------------------------------------
//  IMAGE LOOKUP
//----------------------------------------------------------------------------

//
// BackupTexture
//
static const image_t *BackupTexture(const char *tex_name, int flags)
{
	const real_image_t *rim;

	// backup plan: try a flat with the same name
	if (! (flags & ILF_Exact))
	{
		rim = real_flats.Lookup(tex_name);
		if (rim)
			return &rim->pub;
	}

	if (flags & ILF_Null)
		return NULL;

	M_WarnError("Unknown texture found in level: '%s'\n", tex_name);

	// return the texture dummy image
	rim = RIM_DUMMY_TEX;
	return &rim->pub;
}

//
// BackupFlat
//
//
static const image_t *BackupFlat(const char *flat_name, int flags)
{
	const real_image_t *rim;

	// backup plan 1: if lump exists and is right size, add it.
	if (! (flags & ILF_NoNew))
	{
		int i = W_CheckNumForName(flat_name);

		if (i >= 0)
		{
			rim = AddImageFlat(flat_name, i);
			if (rim)
				return &rim->pub;
		}
	}

	// backup plan 2: Texture with the same name ?
	if (! (flags & ILF_Exact))
	{
		rim = real_textures.Lookup(flat_name);
		if (rim)
			return &rim->pub;
	}

	if (flags & ILF_Null)
		return NULL;

	M_WarnError("Unknown flat found in level: '%s'\n", flat_name);

	// return the flat dummy image
	rim = RIM_DUMMY_FLAT;
	return &rim->pub;
}

//
// BackupGraphic
//
static const image_t *BackupGraphic(const char *gfx_name, int flags)
{
	const real_image_t *rim;

	// backup plan 1: look for sprites and heretic-background
	if (! (flags & (ILF_Exact | ILF_Font)))
	{
		rim = real_graphics.Lookup(gfx_name, IMSRC_Raw320x200);
		if (rim)
			return &rim->pub;
  
		rim = real_sprites.Lookup(gfx_name);
		if (rim)
			return &rim->pub;
	}
  
	// not already loaded ?  Check if lump exists in wad, if so add it.
	if (! (flags & ILF_NoNew))
	{
		int i = W_CheckNumForName(gfx_name);

		if (i >= 0)
		{
			rim = AddImageGraphic(gfx_name, IMSRC_Graphic, i);
			if (rim)
				return &rim->pub;
		}
	}

	if (flags & ILF_Null)
		return NULL;

	M_WarnError("Unknown graphic: '%s'\n", gfx_name);

	// return the graphic dummy image
	rim = (flags & ILF_Font) ? RIM_DUMMY_FONT : RIM_DUMMY_GFX;
	return &rim->pub;
}

//
// W_ImageLookup
//
// Note: search must be case insensitive.
//
const image_t *W_ImageLookup(const char *name, image_namespace_e type, int flags)
{
	// "NoTexture" marker.
	if (!name || !name[0] || name[0] == '-')
		return NULL;

	// "Sky" marker.
	if (type == INS_Flat && (stricmp(name, "F_SKY1") == 0) ||
		(stricmp(name, "F_SKY") == 0))
	{
		return skyflatimage;
	}
  
	const real_image_t *rim = NULL;

	switch (type)
	{
		case INS_Texture:
			rim = real_textures.Lookup(name);
			if (! rim)
				return BackupTexture(name, flags);
			break;

		case INS_Flat:
			rim = real_flats.Lookup(name);
			if (! rim)
				return BackupFlat(name, flags);
			break;

		case INS_Sprite:
			rim = real_sprites.Lookup(name);
			if (! rim)
			{
				if (flags & ILF_Null)
					return NULL;
				
				rim = RIM_DUMMY_SPRITE;
			}
			break;

		default: /* INS_Graphic */
			rim = real_graphics.Lookup(name);
			if (! rim)
				return BackupGraphic(name, flags);
			break;
	}

	DEV_ASSERT2(rim);

	return &rim->pub;
}

static const char *UserSkyFaceName(const char *base, int face)
{
	static char buffer[64];
	static char *letters = "NESWTB";

	sprintf(buffer, "%s_%c", base, letters[face]);
	return buffer;
}

//
// W_ImageFromSkyMerge
// 
const image_t *W_ImageFromSkyMerge(const image_t *sky, int face)
{
	// check IMAGES.DDF for the face image
	// (they need to be Textures)

	const real_image_t *rim = (const real_image_t *)sky;
	const char *user_face_name = UserSkyFaceName(rim->name, face);

	rim = real_textures.Lookup(user_face_name);
	if (rim)
		return &rim->pub;

	// nope, look for existing sky-merge image

	epi::array_iterator_c it;

	for (it=sky_merges.GetBaseIterator(); it.IsValid(); it++)
	{
		rim = ITERATOR_TO_TYPE(it, real_image_t*);
    
		DEV_ASSERT2(rim->source_type == IMSRC_SkyMerge);

		if (sky != rim->source.merge.sky)
			continue;

		if (face == rim->source.merge.face)
			return &rim->pub;

		// Optimisations:
		//    1. in MIRROR mode, bottom is same as top.
		//    2. when sky is narrow, south == north, west == east.
		// NOTE: we rely on lookup order of RGL_UpdateSkyBoxTextures.

		if (sky_stretch >= STRETCH_MIRROR &&
			face == WSKY_Bottom && rim->source.merge.face == WSKY_Top)
		{
#if 0  // DISABLED
			L_WriteDebug("W_ImageFromSkyMerge: using TOP for BOTTOM.\n");
			return &rim->pub;
#endif
		}
		else if (SkyIsNarrow(sky) &&
			(face == WSKY_South && rim->source.merge.face == WSKY_North))
		{
			L_WriteDebug("W_ImageFromSkyMerge: using NORTH for SOUTH.\n");
			return &rim->pub;
		}
		else if (SkyIsNarrow(sky) &&
			 (face == WSKY_West && rim->source.merge.face == WSKY_East))
		{
			L_WriteDebug("W_ImageFromSkyMerge: using EAST for WEST.\n");
			return &rim->pub;
		}
	}

	// use low-res box sides for low-res sky images
	int size = (sky->actual_h < 228) ? 128 : 256;

	rim = AddImageSkyMerge(sky, face, size);
	return &rim->pub;
}

//
// W_ImageForDummySprite
// 
const image_t *W_ImageForDummySprite(void)
{
	const real_image_t *rim = RIM_DUMMY_SPRITE;
	return &rim->pub;
}

//
// W_ImageParseSaveString
//
// Used by the savegame code.
// 
const image_t *W_ImageParseSaveString(char type, const char *name)
{
	// this name represents the sky (historical reasons)
	if (type == 'd' && stricmp(name, "DUMMY__2") == 0)
	{
		return skyflatimage;
	}

	const real_image_t *rim;

	switch (type)
	{
		case 'o': /* font (backwards compat) */
		case 'r': /* raw320x200 (backwards compat) */
		case 'P':
			return W_ImageLookup(name, INS_Graphic);

		case 'T':
			return W_ImageLookup(name, INS_Texture);

		case 'F':
			return W_ImageLookup(name, INS_Flat);

		case 'S':
			return W_ImageLookup(name, INS_Sprite);

		case 'd': /* dummy */
			rim = dummies.Lookup(name);
			if (! rim)
				rim = RIM_DUMMY_TEX;
			return &rim->pub;

		default:
			I_Warning("W_ImageParseSaveString: unknown type '%c'\n", type);
			break;
	}

	rim = real_graphics.Lookup(name); if (rim) return &rim->pub;
	rim = real_textures.Lookup(name); if (rim) return &rim->pub;
	rim = real_flats.Lookup(name);    if (rim) return &rim->pub;
	rim = real_sprites.Lookup(name);  if (rim) return &rim->pub;

	I_Warning("W_ImageParseSaveString: image [%c:%s] not found.\n", type, name);

	// return the texture dummy image
	rim = RIM_DUMMY_TEX;
	return &rim->pub;
}

//
// W_ImageMakeSaveString
//
// Used by the savegame code.
// 
void W_ImageMakeSaveString(const image_t *image, char *type, char *namebuf)
{
	if (image == skyflatimage)
	{
		// this name represents the sky (historical reasons)
		*type = 'd';
		strcpy(namebuf, "DUMMY__2");
		return;
	}

	const real_image_t *rim = (const real_image_t *) image;

	strcpy(namebuf, rim->name);

	/* handle User images (convert to a more general type) */
	if (rim->source_type == IMSRC_User)
	{
		switch (rim->source.user.def->belong)
		{
			case INS_Texture: (*type) = 'T'; return;
			case INS_Flat:    (*type) = 'F'; return;
			case INS_Sprite:  (*type) = 'S'; return;

			default:  /* INS_Graphic */
				(*type) = 'P';
				return;
		}
	}

	switch (rim->source_type)
	{
		case IMSRC_Raw320x200:
		case IMSRC_Graphic: (*type) = 'P'; return;

		case IMSRC_Texture: (*type) = 'T'; return;
		case IMSRC_Flat:    (*type) = 'F'; return;
		case IMSRC_Sprite:  (*type) = 'S'; return;

		case IMSRC_Dummy:   (*type) = 'd'; return;

		case IMSRC_SkyMerge:
		default:
			I_Error("W_ImageMakeSaveString: bad type %d\n", rim->source_type);
			break;
	}
}

//
// W_ImageGetName
//
const char *W_ImageGetName(const image_t *image)
{
	const real_image_t *rim;

	rim = (const real_image_t *) image;

	return rim->name;
}


//----------------------------------------------------------------------------

//
//  IMAGE USAGE
//


#if 0  // planned....
static
const cached_image_t *ImageCacheBlock(real_image_t *rim, int mip)
{
	real_cached_image_t *rc;
  
	if (mip >= rim->block_cache.num_mips)
	{
		// reallocate mip array
    
		Z_Resize(rim->block_cache.mips, real_cached_image_t *, mip+1);

		while (rim->block_cache.num_mips <= mip)
			rim->block_cache.mips[rim->block_cache.num_mips++] = NULL;
	}

	rc = rim->block_cache.mips[mip];

	if (rc && rc->users == 0 && rc->invalidated)
	{
		UnloadImageBlock(rc, rim);
		rc = rim->block_cache.mips[rc->mip] = NULL;
	}

	// already cached ?
	if (rc)
	{
		if (rc->users == 0)
		{
			// no longer freeable
			cache_size -= rc->size;
		}
		rc->users++;
	}
	else
	{
		// load into cache
		rc = rim->block_cache.mips[mip] = LoadImageBlock(rim, mip);
	}

	DEV_ASSERT2(rc);
	DEV_ASSERT2(rc->mode == IMG_Block);

	return (const cached_image_t *)(rc + 1);
}
#endif

static INLINE
const cached_image_t *ImageCacheOGL(real_image_t *rim)
{
	real_cached_image_t *rc;

	rc = rim->ogl_cache;

	if (rc && rc->users == 0 && rc->invalidated)
	{
		UnloadImageOGL(rc, rim);
		rc = NULL;
	}

	// already cached ?
	if (rc)
	{
		if (rc->users == 0)
		{
			// no longer freeable
			cache_size -= rc->size;
		}
		rc->users++;
	}
	else
	{
		// load into cache
		rc = rim->ogl_cache = LoadImageOGL(rim, NULL);
	}

	DEV_ASSERT2(rc);
	DEV_ASSERT2(rc->mode == IMG_OGL);

	return (const cached_image_t *)(rc + 1);
}

const cached_image_t *ImageCacheTransOGL(real_image_t *rim,
	const colourmap_c *trans)
{
	// already cached ?

	int i;
	int free_slot = -1;

	real_cached_image_t *rc = NULL;

	// find translation in set.  Afterwards, rc will be NULL if not found.

	for (i = 0; i < rim->trans_cache.num_trans; i++)
	{
		rc = rim->trans_cache.trans[i];

		if (rc && rc->users == 0 && rc->invalidated)
		{
			UnloadImageOGL(rc, rim);
			rc = NULL;
		}

		if (! rc)
		{
			free_slot = i;
			continue;
		}

		if (rc->trans_map == trans)
			break;

		rc = NULL;
	}

	if (rc)
	{
		if (rc->users == 0)
		{
			// no longer freeable
			cache_size -= rc->size;
		}
		rc->users++;
	}
	else
	{
		if (free_slot < 0)
		{
			// reallocate trans array
		
			free_slot = rim->trans_cache.num_trans;

			Z_Resize(rim->trans_cache.trans, real_cached_image_t *, free_slot+1);

			rim->trans_cache.num_trans++;
		}

		// load into cache
		rc = rim->trans_cache.trans[free_slot] = LoadImageOGL(rim, trans);
	}

	DEV_ASSERT2(rc);
	DEV_ASSERT2(rc->mode == IMG_OGL);

	return (const cached_image_t *)(rc + 1);
}


//
// W_ImageCache
//
// The top-level routine for caching in an image.  Mainly just a
// switch to more specialised routines.  Never returns NULL.
//
const cached_image_t *W_ImageCache(const image_t *image, 
								   bool anim,
								   const colourmap_c *trans)
{
	// Intentional Const Override
	real_image_t *rim = (real_image_t *) image;
 
	// handle animations
	if (anim)
		rim = rim->anim.cur;

	if (trans)
		return ImageCacheTransOGL(rim, trans);

	return ImageCacheOGL(rim);
}


//
// W_ImageDone
//
void W_ImageDone(const cached_image_t *c)
{
	real_cached_image_t *rc;

	SYS_ASSERT(c);

	// Intentional Const Override
	rc = ((real_cached_image_t *) c) - 1;

	SYS_ASSERT(rc->users > 0);

	rc->users--;

	if (rc->users == 0)
	{
		cache_size += rc->size;

		// move cached image to the end of the cache list.  This way,
		// the Most Recently Used (MRU) images are at the tail of the
		// list, and thus the Least Recently Used (LRU) images are at the
		// head of the cache list.

		Unlink(rc);
		InsertAtTail(rc);
	}
}


//
// W_ImageGetEpiBlock
//
// Return the pixel block for the cached image.
// Never returns NULL.
//
#if 0  // planned...
const epi::basicimage_c *W_ImageGetEpiBlock(const cached_image_t *c)
{
	real_cached_image_t *rc;
 
	DEV_ASSERT2(c);

	// Intentional Const Override
	rc = ((real_cached_image_t *) c) - 1;

	DEV_ASSERT2(rc->parent);
	DEV_ASSERT2(rc->mode == IMG_Block);

	DEV_ASSERT2(rc->info.block.pixels);

	return rc->info.block.pixels;
}
#endif


//
// W_ImageGetOGL
//
GLuint W_ImageGetOGL(const cached_image_t *c)
{
	DEV_ASSERT2(c);

	// Intentional Const Override
	real_cached_image_t *rc = ((real_cached_image_t *) c) - 1;

	DEV_ASSERT2(rc->parent);
	DEV_ASSERT2(rc->mode == IMG_OGL);

	return rc->info.ogl.tex_id;
}

#if 0
rgbcol_t W_ImageGetHue(const cached_image_t *c)
{
	DEV_ASSERT2(c);

	// Intentional Const Override
	real_cached_image_t *rc = ((real_cached_image_t *) c) - 1;

	DEV_ASSERT2(rc->parent);
	DEV_ASSERT2(rc->mode == IMG_OGL);

	return rc->hue;
}
#endif

static void FlushImageCaches(z_urgency_e urge)
{
	int bytes_to_free = 0;
	real_cached_image_t *rc, *next;

	if (img_shrink_buffer)
	{
		delete[] img_shrink_buffer;
		img_shrink_buffer = NULL;
		img_shrink_buf_size = 0;
	}

	if (w_locked_ogl)
		return;

	//!!! FIXME: make triple sure that if this is called from _within_
	//!!! one of routines above, nothing bad will happen.

	switch (urge)
	{
		case Z_UrgencyLow: bytes_to_free = cache_size / 16; break;
		case Z_UrgencyMedium: bytes_to_free = cache_size / 8; break;
		case Z_UrgencyHigh: bytes_to_free = cache_size / 2; break;
		case Z_UrgencyExtreme: bytes_to_free = INT_MAX; break;

		default:
			I_Error("FlushImageCaches: Invalid urgency level %d\n", urge);
	}

	// the Least Recently Used (LRU) images are at the head of the image
	// cache list, so we unload those ones first.
 
	for (rc = imagecachehead.next; 
		 rc != &imagecachehead && bytes_to_free > 0; rc = next)
	{
		next = rc->next;

		if (rc->users == 0)
		{
			bytes_to_free -= rc->size;
			UnloadImage(rc);
		}
	}
}

//
// W_ImagePreCache
// 
void W_ImagePreCache(const image_t *image)
{
	W_ImageDone(W_ImageCache(image, false));

	// Intentional Const Override
	real_image_t *rim = (real_image_t *) image;

	// pre-cache alternative images for switches too
	if (strlen(rim->name) >= 4 &&
		(strncasecmp(rim->name, "SW1", 3) == 0 ||
		 strncasecmp(rim->name, "SW2", 3) == 0 ))
	{
		char alt_name[16];

		strcpy(alt_name, rim->name);
		alt_name[2] = (alt_name[2] == '1') ? '2' : '1';

		real_image_t *alt = real_textures.Lookup(alt_name);

		if (alt)
			W_ImageDone(W_ImageCache(&alt->pub, false));
	}
}


//----------------------------------------------------------------------------

//
// W_InitImages
//
// Initialises the image system.
//
bool W_InitImages(void)
{
	// the only initialisation the cache list needs
	imagecachehead.next = imagecachehead.prev = &imagecachehead;

	Z_RegisterCacheFlusher(FlushImageCaches);

	real_graphics.Clear();
	real_textures.Clear();
	real_flats.Clear();
	real_sprites.Clear();

	sky_merges.Clear();
	dummies.Clear();

	// setup dummy images
	AddImageDummy(IMSRC_Texture, "DUMMY_TEXTURE");
	AddImageDummy(IMSRC_Flat,    "DUMMY_FLAT");
	AddImageDummy(IMSRC_Flat,    "DUMMY_SKY");
	AddImageDummy(IMSRC_Graphic, "DUMMY_GRAPHIC");
	AddImageDummy(IMSRC_Sprite,  "DUMMY_SPRITE");

	real_image_t *rim = AddImageDummy(IMSRC_Graphic, "DUMMY_FONT");
	rim->source.dummy.fg = pal_white;

	skyflatimage = &RIM_SKY_FLAT->pub;

    // check options
	M_CheckBooleanParm("smoothing", &use_smoothing, false);
	M_CheckBooleanParm("dither", &use_dithering, false);

	if (M_CheckParm("-nomipmap"))
		use_mipmapping = 0;
	else if (M_CheckParm("-mipmap"))
		use_mipmapping = 1;
	else if (M_CheckParm("-trilinear"))
		use_mipmapping = 2;

	return true;
}

//
// W_UpdateImageAnims
//
// Animate all the images.
//
void W_UpdateImageAnims(void)
{
	real_graphics.Animate();
	real_textures.Animate();
	real_flats.Animate();
}

//
// W_ResetImages
//
// Resets all images, causing all cached images to be invalidated.
// Needs to be called when gamma changes (GL renderer only), or when
// certain other parameters change (e.g. GL mipmapping modes).
//
void W_ResetImages(void)
{
	real_cached_image_t *rc, *next;

	for (rc=imagecachehead.next; rc != &imagecachehead; rc=next)
	{
		next = rc->next;
    
		if (rc->users == 0)
			UnloadImage(rc);
		else
			rc->invalidated = true;
	}
}

//
// W_LockImagesOGL
//
// Prevents OGL texture ids from being deleted.  Essentially this
// routine is like giving all cached images an extra user.  It is
// needed due to the curreny way the RGL_UNIT code works.
//
void W_LockImagesOGL(void)
{
	SYS_ASSERT(!w_locked_ogl);

	w_locked_ogl = true;
}

//
// W_UnlockImagesOGL
//
void W_UnlockImagesOGL(void)
{
	SYS_ASSERT(w_locked_ogl);

	w_locked_ogl = false;
}

//
// W_AnimateImageSet
//
// Sets up the images so they will animate properly.  Array is
// allowed to contain NULL entries.
//
// NOTE: modifies the input array of images.
// 
void W_AnimateImageSet(const image_t ** images, int number, int speed)
{
	int i, total;
	real_image_t *rim, *other;

	DEV_ASSERT2(images);
	DEV_ASSERT2(speed > 0);

	// ignore images that are already animating
	for (i=0, total=0; i < number; i++)
	{
		// Intentional Const Override
		rim = (real_image_t *) images[i];

		if (! rim)
			continue;

		if (rim->anim.speed > 0)
			continue;

		images[total++] = images[i];
	}

	// anything left to animate ?
	if (total < 2)
		return;

	for (i=0; i < total; i++)
	{
		// Intentional Const Override
		rim   = (real_image_t *) images[i];
		other = (real_image_t *) images[(i+1) % total];

		rim->anim.next = other;
		rim->anim.speed = rim->anim.count = speed;
	}
}

void W_ImageClearMergingSky(void)
{
	if (merging_sky_image)
		delete merging_sky_image;

	merging_sky_image = NULL;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
