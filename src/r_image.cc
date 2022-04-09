//----------------------------------------------------------------------------
//  EDGE Generalised Image Handling
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2018  The EDGE Team.
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
//   -  faster search methods.
//   -  do some optimisation
//

#include "system/i_defs.h"
#include "system/i_defs_gl.h"

#include <limits.h>
#include <list>

#include "../epi/endianess.h"
#include "../epi/file.h"
#include "../epi/filesystem.h"
#include "../epi/file_memory.h"

#include "../epi/image_data.h"
#include "../epi/image_hq2x.h"
#include "../epi/image_png.h"
#include "../epi/image_jpeg.h"
#include "../epi/image_tga.h"

#include "ddf/flat.h"

#include "dm_data.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "e_search.h"
#include "e_main.h"
#include "m_argv.h"
#include "m_misc.h"
#include "p_local.h"
#include "r_defs.h"
#include "r_gldefs.h"
#include "r_image.h"
#include "r_sky.h"
#include "r_texgl.h"
#include "r_colormap.h"
#include "w_texture.h"
#include "w_wad.h"
#include "z_zone.h"
#include "games/wolf3d/wlf_rawdef.h"
#include "games/rott/rt_byteordr.h"

// LIGHTING DEBUGGING
// #define MAKE_TEXTURES_WHITE  1

swirl_type_e swirling_flats = SWIRL_Vanilla;

extern epi::image_data_c *ReadAsEpiBlock(image_c *rim);

extern epi::file_c *OpenUserFileOrLump(imagedef_c *def);

extern void CloseUserFileOrLump(imagedef_c *def, epi::file_c *f);

// FIXME: duplicated in r_doomtex
#define DUMMY_X  16
#define DUMMY_Y  16

extern void DeleteSkyTextures(void);
extern void DeleteColourmapTextures(void);

//
// This structure is for "cached" images (i.e. ready to be used for
// rendering), and is the non-opaque version of cached_image_t.  A
// single structure is used for all image modes (Block and OGL).
//
// Note: multiple modes and/or multiple mips of the same image_c can
// indeed be present in the cache list at any one time.
//
typedef struct cached_image_s
{
	// parent image
	image_c *parent;

	// colormap used for translated image, normally NULL
	const colourmap_c *trans_map;

	// general hue of image (skewed towards pure colors)
	rgbcol_t hue;

	// texture identifier within GL
	GLuint tex_id;
}
cached_image_t;

typedef std::list<image_c *> real_image_container_c;

static image_c *do_Lookup(real_image_container_c& bucket, const char *name,
                          int source_type = -1
						  /* use -2 to prevent USER override */)
{
	// for a normal lookup, we want USER images to override
	if (source_type == -1)
	{
		image_c *rim = do_Lookup(bucket, name, IMSRC_User);  // recursion
		if (rim)
			return rim;
	}

	real_image_container_c::reverse_iterator it;

	// search backwards, we want newer image to override older ones
	for (it = bucket.rbegin(); it != bucket.rend(); it++)
	{
		image_c *rim = *it;
		/*
				if(stricmp(name,"TMP_NORMAL")==0) {
					I_Printf("FOO '%s'\n",rim->name);
				}
		*/
		if (source_type >= 0 && source_type != (int)rim->source_type)
			continue;

		if (stricmp(name, rim->name) == 0)
			return rim;
	}

	return NULL;  // not found
}

static void do_Animate(real_image_container_c& bucket)
{
	real_image_container_c::iterator it;

	for (it = bucket.begin(); it != bucket.end(); it++)
	{
		image_c *rim = *it;

		if (rim->anim.speed == 0)  // not animated ?
			continue;

		SYS_ASSERT(rim->anim.count > 0);

		rim->anim.count--;

		if (rim->anim.count == 0 && rim->anim.cur->anim.next)
		{
			rim->anim.cur = rim->anim.cur->anim.next;
			rim->anim.count = rim->anim.speed;
		}
	}
}

#if 0
static void do_DebugDump(real_image_container_c& bucket)
{
	L_WriteDebug("{\n");

	real_image_container_c::iterator it;

	for (it = bucket.begin(); it != bucket.end(); it++)
	{
		image_c *rim = *it;

		L_WriteDebug("   [%s] type %d: %dx%d < %dx%d\n",
			rim->name, rim->source_type,
			rim->actual_w, rim->actual_h,
			rim->total_w, rim->total_h);
	}

	L_WriteDebug("}\n");
}
#endif

bool ROTT_IsDataFLRCL(const byte *data, int length)
{
	static byte rott_flat[4] = { 0x80, 0x00, 0x80, 0x80 };
	if (length < 4)
		return false;

	return memcmp(data, rott_flat, 4) == 0;
}

// mipmapping enabled ?
// 0 off, 1 bilinear, 2 trilinear
int var_mipmapping = 1;

int var_smoothing = 1;

int var_anisotropic = 1;

bool var_dithering = false;

int hq2x_scaling = 1;

// total set of images
static real_image_container_c real_graphics;
static real_image_container_c real_textures;
static real_image_container_c real_flats;
static real_image_container_c real_sprites;
static real_image_container_c raw_graphics;

const image_c *skyflatimage;

static const image_c *dummy_sprite;
static const image_c *dummy_skin;
static const image_c *dummy_hom[2];

// image cache (actually a ring structure)
static std::list<cached_image_t *> image_cache;

// tiny ring helpers
static inline void InsertAtTail(cached_image_t *rc)
{
	image_cache.push_back(rc);

#if 0  // OLD WAY
	SYS_ASSERT(rc != &imagecachehead);

	rc->prev = imagecachehead.prev;
	rc->next = &imagecachehead;

	rc->prev->next = rc;
	rc->next->prev = rc;
#endif
}
static inline void Unlink(cached_image_t *rc)
{
	// FIXME: Unlink
#if 0
	SYS_ASSERT(rc != &imagecachehead);

	rc->prev->next = rc->next;
	rc->next->prev = rc->prev;
#endif
}

//----------------------------------------------------------------------------
//
//  IMAGE CREATION
//

image_c::image_c() : actual_w(0), actual_h(0), total_w(0), total_h(0),
					 source_type(IMSRC_Dummy),
					 source_palette(-1),
					 cache()
{
	strcpy(name, "_UNINIT_");

	//memset() doesn't allocate memory here, only used to set memory to bytes.
	memset(&source, 0, sizeof(source));
	memset(&anim, 0, sizeof(anim));
}

image_c::~image_c()
{
	/* TODO: image_c destructor */
}

static image_c *NewImage(int width, int height, int opacity = OPAC_Unknown)
{
	image_c *rim = new image_c;

	rim->actual_w = width;
	rim->actual_h = height;
	rim->total_w = W_MakeValidSize(width);
	rim->total_h = W_MakeValidSize(height);
	rim->offset_x = rim->offset_y = 0;
	//rim->roffset_x = rim->roffset_y = 0;
	rim->scale_x = rim->scale_y = 1.0f;
	rim->opacity = opacity;
	rim->origsize = 0; //Not every patch will have this!

	// set initial animation info
	rim->anim.cur = rim;
	rim->anim.next = NULL;
	rim->anim.count = rim->anim.speed = 0;
	rim->liquid_type = LIQ_None;
	rim->swirled_gametic = 0;
	return rim;
}

static image_c *NewROTTImage(int width, int height, int opacity = OPAC_Unknown)
{
	image_c *rim = new image_c;

	rim->actual_w = width * 4;
	rim->actual_h = height;
	rim->total_w = W_MakeValidSize(width);
	rim->total_h = W_MakeValidSize(height);
	//rim->roffset_x = rim->roffset_y = 0;
	rim->scale_x = rim->scale_y = 1.0f;
	rim->opacity = opacity;

	rim->source_type = IMSRC_rottpic;

	// set initial animation info
	rim->anim.cur = rim;
	rim->anim.next = NULL;
	rim->anim.count = rim->anim.speed = 0;

	return rim;
}

static image_c *CreateDummyImage(const char *name, rgbcol_t fg, rgbcol_t bg)
{
	image_c *rim;

	rim = NewImage(DUMMY_X, DUMMY_Y, (bg == TRANS_PIXEL) ? OPAC_Masked : OPAC_Solid);

	strcpy(rim->name, name);

	rim->source_type = IMSRC_Dummy;
	rim->source_palette = -1;

	rim->source.dummy.fg = fg;
	rim->source.dummy.bg = bg;

	return rim;
}

static image_c *AddImageGraphic(const char *name, image_source_e type, int lump,
								real_image_container_c& container,
								const image_c *replaces = NULL)
{
	/* also used for Sprites and TX/HI stuff */

	int lump_len = W_LumpLength(lump);

	epi::file_c *f = W_OpenLump(lump);
	SYS_ASSERT(f);

	//byte *array;
	byte buffer[32];

	f->Read(buffer, sizeof(buffer));

	f->Seek(0, epi::file_c::SEEKPOINT_START);

	// determine info, and whether it is PNG or DOOM_PATCH
	int width = 0, height = 0;
	int offset_x = 0, offset_y = 0;
	int origsize = 0; //ROTT
	int roffset_x = 0, roffset_y = 0;

	// determine RAW ROTT picture info (!!!)
	int picwidth = 0;// = 0;
	int	picheight = 0;// = 0;
	int picorg_x = 0, picorg_y = 0;

	// set ROTT FLATS data (UPDN/SKY)
	int flatheight = 0;
	int flatwidth = 0;

	int lbmwidth = 320; //EXPRESSLY SET!
	int lbmheight = 200; //EXPRESSLY SET!

	bool is_png = false;
	bool solid = false;
	bool is_rott = false;
	bool is_rottflat;
	bool is_lpic = false;

	if (epi::PNG_IsDataPNG(buffer, lump_len))
	{
		is_png = true;

		if (! PNG_GetInfo(f, &width, &height, &solid) ||
		    width <= 0 || height <= 0)
		{
			I_Error("Error scanning PNG image in '%s' lump\n", W_GetLumpName(lump));
		}

		// close it
		delete f;
	}


#if 0

	else
	{
		is_rott = true;
		rottpatch_t *pat = (rottpatch_t *)buffer;

		origsize = EPI_LE_S16(pat->origsize);
		width = EPI_LE_S16(pat->width);
		height = EPI_LE_S16(pat->height);
		offset_x = EPI_LE_S16(pat->leftoffset);
		offset_y = EPI_LE_S16(pat->topoffset);

#if 1
		L_WriteDebug("ROTT GETINFO [%s] : size %dx%d\n", W_GetLumpName(lump), pat->origsize, pat->width, pat->height);
#endif

		delete f;

		if (origsize > 320 || width <= 0 || width > 320 ||
			height <= 0 || height > 320 ||
			ABS(offset_x) > 2048 || ABS(offset_y) > 1024)
		{
			// check for Heretic/Hexen/ROTT images, which are raw 320x200
			//if (lump_len == pat->origsize, pat->width, pat->height, type == IMSRC_Raw320x200)
			if (lump_len == 320 * 200 && type == IMSRC_Flat)
			{
				I_Printf("ROTT Graphic '%s' seems to be a flat\n", name);
				image_c *rim = NewImage(320, 200, OPAC_Solid);
				strcpy(rim->name, name);

				rim->source_type = IMSRC_Raw320x200;
				rim->source.flat.lump = lump;
				rim->source_palette = W_GetPaletteForLump(lump);
				return rim;
			}

			if (lump_len == 64 * 64 || lump_len == 64 * 65 || lump_len == 64 * 128)
				I_Warning("Graphic '%s' seems to be a flat.\n", name);
			else
				I_Warning("ROTT: Graphic '%s' does not seem to be a graphic.\n", name);

			return NULL;
		}
	}
#endif // 0


	else  // DOOM/ROTT GRAPHICS/PATCHES/RAW
	{
	// Interpret raw on-disk data to set up for image loading
	patch_t *pat = (patch_t *)buffer; // Normal DOOM Patch
	rottpatch_t *rpat = (rottpatch_t *)buffer; // Rise of the Triad Patch

	width = EPI_LE_S16(pat->width);
	height = EPI_LE_S16(pat->height);
	offset_x = EPI_LE_S16(pat->leftoffset);
	offset_y = EPI_LE_S16(pat->topoffset);
	origsize = EPI_LE_S16(rpat->origsize); // ROTT (rottpatch_t is in *rpat via the rottpatch_t header)
	roffset_x = EPI_LE_S16(rpat->leftoffset) + (EPI_LE_S16(rpat->origsize) / 2); //according to Icculus/WinROTT, leftoffset needs modification beforehand
	roffset_y = EPI_LE_S16(rpat->topoffset) + EPI_LE_S16(rpat->origsize); //according to Icculus/WinROTT, topoffset needs modification beforehand

	//---------------------------------------------------------------------------------------
	// Rise of the Triad FLATS FORMAT
	lpic_t *rottflat = (lpic_t *)buffer; 
	flatwidth = EPI_LE_S16(rottflat->width);
	flatheight = EPI_LE_S16(rottflat->height);
	picorg_x = EPI_LE_S16(rottflat->orgx);
	picorg_y = EPI_LE_S16(rottflat->orgy);
	//int expectlen = picwidth * picwidth + 8;
	//--------------------------------------------------------------------------------------

	//--------------------------------------------------------------------------------------
	// LBM ROTT 320x200 images (only used a few times in Darkwar)
	lbm_t *lbm = (lbm_t *)buffer; 
	lbmwidth = EPI_LE_S16(lbm->width); //0
	lbmheight = EPI_LE_S16(lbm->height); //2?
	//--------------------------------------------------------------------------------------

	//--------------------------------------------------------------------------------------
	// ROTT Picture Format (rottpic)
	pic_t *rottpic = (pic_t *)buffer;
	picwidth = rottpic->width;
	picheight = rottpic->height;
	//--------------------------------------------------------------------------------------

		delete f;

		
#if 0
		if (ROTT_IsDataFLRCL(buffer, lump_len))
		{
			I_Printf("ROTT: Data '%s' is floor or ceiling, get more information\n", name);

			if ((lump_len == 128 * 128) + 8 && type == IMSRC_Graphic)
			{
				is_rottflat = true;
				I_Printf("ROTT: '%s' header is a ROTT flat, 128x128 it...\n", name);
				image_c *rim = NewImage(128, 128, OPAC_Solid); //!!! remember: width/height were previously 320x200
				strcpy(rim->name, name);

				rim->source_type = IMSRC_Graphic;
				rim->source.flat.lump = lump;
				rim->source_palette = W_GetPaletteForLump(lump);
				return rim;
			}
			else I_Printf("Data lump_len/type loop is invalid, returning nothing!\n");
		}
#endif // 0


		// do some basic checks
				// !!! FIXME: identify lump types in wad code.
		if (width <= 0 || width > 2048 ||
			height <= 0 || height > 512 ||
			ABS(offset_x) > 2048 || ABS(offset_y) > 1024)
		{

			// do checking for ROTT Graphics _FIRST_
			int expectlen = width * height + 8; //if (lump_len != expectlen)
//#if 1
			if ((lump_len != expectlen) && (type == IMSRC_rottpic))// ^ IMSRC_rottpic))
			{
				//I_Printf("rottpic: '%s' seems to be a raw image + header (lpic_t)..lump_len = '%d'\n", name, lump_len);
			//	I_Printf("rottpic: '%s' width: '%d' height: '%d'\n", name, rottpic->width, rottpic->height);
				//I_Printf("rottpic: '%s': [%s]x[%s]..\n", name, picwidth, picheight);

				int width = picwidth * 4;
				int height = picheight;

				image_c *rim = NewImage(width, height, OPAC_Solid);// solid ? OPAC_Solid : OPAC_Unknown);
				//I_Printf("rottpic: '%s' width: '%d' height: '%d'\n", name, width, height);
				//epi::image_data_c *img = new epi::image_data_c(tw, th, 1);


				strcpy(rim->name, name);
				//I_Printf("rottpic: Read lpic Image: '%s'\n", name);

				rim->source_type = IMSRC_rottpic;
				rim->source.pic.lump = lump; //!!!
				rim->source_palette = W_GetPaletteForLump(lump);
				return rim;
			}
//#endif // 1
			else

			// check for Heretic/Hexen, which are raw 320x200
			if (lump_len == 320 * 200 && type == IMSRC_Graphic)
			{
				I_Printf("Heretic/Hexen: '%s' seems to be a raw image, 320x200 it..\n", name);
				image_c *rim = NewImage(320, 200, OPAC_Solid); //!!! remember: width/height were previously 320x200
				strcpy(rim->name, name);

				rim->source_type = IMSRC_Raw320x200;
				rim->source.flat.lump = lump;
				rim->source_palette = W_GetPaletteForLump(lump);
				return rim;
			}

			// ~CA: Explicitly check for Widescreen len and set it as such (todo: refactor mundo titlescreenWS hack)
			if (lump_len == 560 * 200 && type == IMSRC_Graphic)
			{
				I_Printf("AddImage: '%s' seems to be a widescreen image\n", name);
				image_c* rim = NewImage(560, 200, OPAC_Solid); //!!! remember: width/height were previously 320x200
				strcpy(rim->name, name);

				rim->source_type = IMSRC_WideTitle560x200;
				rim->source.flat.lump = lump;
				rim->source_palette = W_GetPaletteForLump(lump);
				return rim;
			}


			// check for ROTT stuff, which are raw 128x128
			if (lump_len == 128 * 128 && type == IMSRC_ROTTRaw128x128)// || IMSRC_ROTTGFX)
			{
				I_Printf("ROTT RawFlat: '%s' seems to be a raw image, 320x200 it..\n", name);
				image_c *rim = NewImage(128, 128, OPAC_Solid); //!!! remember: width/height were previously 320x200
				strcpy(rim->name, name);

				rim->source_type = IMSRC_ROTTRaw128x128;
				rim->source.flat.lump = lump;
				rim->source_palette = W_GetPaletteForLump(lump);
				return rim;
			}


			if (lump_len == 64 * 64 || lump_len == 64 * 65 || lump_len == 64 * 128 || lump_len == 128 * 128)
				I_Warning("Graphic '%s' seems to be a flat, RETURNING NULL.\n", name);
			else
				I_Warning("Graphic '%s' does not seem to be a graphic.\n", name);
				//L_WriteDebug("ROTT PIC_T [%s] : size %dx%d\n", W_GetLumpName(lump), rottpic->width, rottpic->height);
			return NULL;
		}
	}


	// create new image
	image_c *rim = NewImage(width, height, solid ? OPAC_Solid : OPAC_Unknown);

	
	rim->offset_x = offset_x;//!!! || roffset_x;
	rim->offset_y = offset_y;//!!! || roffset_y;

	//if (offset_x != roffset_x)
	//I_Printf("IMAGE [%s]: offset x = [%s], offset y = [%s]\n", rim->name, rim->offset_x, rim->offset_y);
	strcpy(rim->name, name);
#if (IMAGE_DEBUG)
	I_Printf("IMAGE: Creating new image [%s].\n", name);
#endif

	
	if (swirling_flats > SWIRL_Vanilla)
	{
		flatdef_c *current_flatdef = flatdefs.Find(rim->name);

		if (current_flatdef && !current_flatdef->liquid.empty())
		{
			if (strcasecmp(current_flatdef->liquid.c_str(), "THIN") == 0)
				rim->liquid_type = LIQ_Thin;
			else if (strcasecmp(current_flatdef->liquid.c_str(), "THICK") == 0)
				rim->liquid_type = LIQ_Thick;
		}
	}

	rim->source_type = type;
	rim->source.graphic.lump = lump;
	rim->source.graphic.is_png = is_png;
	rim->source_palette = W_GetPaletteForLump(lump);

	if (replaces)
	{
		rim->scale_x = replaces->actual_w / (float)width;
		rim->scale_y = replaces->actual_h / (float)height;

		if (is_png && replaces->source_type == IMSRC_Sprite)
		{
			rim->offset_x = replaces->offset_x;
			rim->offset_y = replaces->offset_y;
		}
	}

	container.push_back(rim);

	return rim;
}

static image_c *AddImageTexture(const char *name, texturedef_t *tdef)
{
	image_c *rim;

	rim = NewImage(tdef->width, tdef->height);

	strcpy(rim->name, name);
	
	if (swirling_flats > SWIRL_Vanilla)
	{
		flatdef_c *current_flatdef = flatdefs.Find(rim->name);

		if (current_flatdef && !current_flatdef->liquid.empty())
		{
			if (strcasecmp(current_flatdef->liquid.c_str(), "THIN") == 0)
				rim->liquid_type = LIQ_Thin;
			else if (strcasecmp(current_flatdef->liquid.c_str(), "THICK") == 0)
				rim->liquid_type = LIQ_Thick;
		}
	}

	if (tdef->scale_x) rim->scale_x = 8.0 / tdef->scale_x;
	if (tdef->scale_y) rim->scale_y = 8.0 / tdef->scale_y;

	rim->source_type = IMSRC_Texture;
	rim->source.texture.tdef = tdef;
	rim->source_palette = tdef->palette_lump;

	real_textures.push_back(rim);

	return rim;
}

static image_c *AddROTTPatchTexture(const char *name, texturedef_t *tdef)
{
	image_c *rim;

	rim = NewImage(tdef->width, tdef->height);

	strcpy(rim->name, name);

	if (tdef->scale_x) rim->scale_x = 8.0 / tdef->scale_x;
	if (tdef->scale_y) rim->scale_y = 8.0 / tdef->scale_y;

	rim->source_type = IMSRC_ROTTGFX;
	rim->source.texture.tdef = tdef;
	rim->source_palette = tdef->palette_lump;

	real_textures.push_back(rim);

	return rim;
}

static image_c *AddImageFlat(const char *name, int lump)
{
	image_c *rim;
	int len, size;

	len = W_LumpLength(lump);

	switch (len)
	{
	case 64 * 64: size = 64; break; // Wolf3D/et al

		// support for odd-size Heretic flats
	case 64 * 65: size = 64; break;

		// support for odd-size Hexen flats
	case 64 * 128: size = 64; break;

	case 128 * 128: size = 128; break;

		// support for ROTT "flats"
	case 128 * 128 + 8: size = 128; break;

		// -- EDGE feature: bigger than normal flats --
	
	case 256 * 256: size = 256; break;
	case 512 * 512: size = 512; break;
	case 1024 * 1024: size = 1024; break;

	default:
		return NULL;
	}

	rim = NewImage(size, size, OPAC_Solid);

	strcpy(rim->name, name);

	if (rott_mode)
	{
		I_Printf("ROTT Flats: Loading %s\n", name);
	}

	rim->source_type = IMSRC_Flat;
	rim->source.flat.lump = lump;
	rim->source_palette = W_GetPaletteForLump(lump);
	
	if (swirling_flats > SWIRL_Vanilla)
	{
		flatdef_c *current_flatdef = flatdefs.Find(rim->name);

		if (current_flatdef && !current_flatdef->liquid.empty())
		{
			if (strcasecmp(current_flatdef->liquid.c_str(), "THIN") == 0)
				rim->liquid_type = LIQ_Thin;
			else if (strcasecmp(current_flatdef->liquid.c_str(), "THICK") == 0)
				rim->liquid_type = LIQ_Thick;
		}
	}

	real_flats.push_back(rim);

	return rim;
}

static image_c *AddROTTFlat(const char *name, int lump)
{
	I_Printf("ROTT: AddROTTFlat. . .\n");
	image_c *rim = new image_c;
	//const char *name = W_GetLumpName(lump);
	//pic_t
	int length;
	byte *data = W_ReadLumpAlloc(lump, &length);

	if (!data || length != 16392)
		I_Printf("Data/Length does not match 16392 bytes for ROTT flat!\n");

	epi::image_data_c *img = new epi::image_data_c(128, 128, 3); //!!!! PAL

	byte *dest = img->pixels;

	// read in pixels
	for (int y = 0; y < 128; y++)
		for (int x = 0; x < 128; x++)
		{
			byte src = data[x * 128 + 127 - y];  // column-major order

			byte *pix = img->PixelAt(x, y);

			pix[0] = rott_palette[src * 3 + 0];
			pix[1] = rott_palette[src * 3 + 1];
			pix[2] = rott_palette[src * 3 + 2];
		}

	delete[] data;

	return rim;
}


static image_c *AddLBMImage(const char *name, int lump)
{
	image_c *rim;
	//int len, size;

	//len = W_LumpLength(lump);

rim = NewImage(320, 200, OPAC_Solid);

strcpy(rim->name, name);

rim->source_type = IMSRC_ROTTLBM;
rim->source.flat.lump = lump;
rim->source_palette = W_GetPaletteForLump(lump);

raw_graphics.push_back(rim);

return rim;
}

static image_c* AddImageUser(imagedef_c* def)
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
		//!!!!! (detail_level == 2) ? 512 : 256;
		w = 256;
		h = 256;
		solid = false;
		break;

	case IMGDT_File:
	case IMGDT_Lump:
	{
		const char* basename = def->info.c_str();

		epi::file_c* f = OpenUserFileOrLump(def);

		if (!f)
		{
			I_Warning("Unable to add image %s: %s\n",
				(def->type == IMGDT_Lump) ? "lump" : "file", basename);
			return NULL;
		}

		bool got_info;

		if (def->format == LIF_EXT)
			got_info = epi::JPEG_GetInfo(f, &w, &h, &solid);
		else if (def->format == LIF_JPEG)
			got_info = epi::JPEG_GetInfo(f, &w, &h, &solid);
		else if (def->format == LIF_TGA)
			got_info = epi::TGA_GetInfo(f, &w, &h, &solid);
		else if (def->format == LIF_PNG)
			got_info = epi::PNG_GetInfo(f, &w, &h, &solid);
		else //if (!def->format)
			got_info = epi::JPEG_GetInfo(f, &w, &h, &solid);

		if (!got_info)
			//	{
					//CloseUserFileOrLump(def, f);
			I_Error("Error occurred scanning image: %s\n", basename);
		//got_info = epi::JPEG_GetInfo(f, &w, &h, &solid);
		//return NULL;
//	}

		CloseUserFileOrLump(def, f);
#if 1
		L_WriteDebug("GETINFO [%s] : size %dx%d\n", def->name.c_str(), w, h);
#endif
	}
	break;

	default:
		I_Error("AddImageUser: Coding error, unknown type %d\n", def->type);
		return NULL; /* NOT REACHED */
	}

	image_c* rim = NewImage(w, h, solid ? OPAC_Solid : OPAC_Unknown);

	rim->offset_x = def->x_offset;
	rim->offset_y = def->y_offset;

	rim->scale_x = def->scale * def->aspect;
	rim->scale_y = def->scale;

	strcpy(rim->name, def->name.c_str());

	/* FIX NAME : replace space with '_' */
	for (int i = 0; rim->name[i]; i++)
		if (rim->name[i] == ' ')
			rim->name[i] = '_';

	rim->source_type = IMSRC_User;
	rim->source.user.def = def;

	// CA 6.5.21: Set grAb values for rim before they are copied to def
	if ((def->format == LIF_PNG))
	{
		epi::image_data_c* tmp_img = ReadAsEpiBlock(rim);
		if (tmp_img->grAb != nullptr)
		{
			rim->offset_x = tmp_img->grAb->x;
			rim->offset_y = tmp_img->grAb->y;
		}
	}

	if (def->special & IMGSP_Crosshair)
	{
		float dy = (200.0f - rim->actual_h * rim->scale_y) / 2.0f - WEAPONTOP;

		rim->offset_y += int(dy / rim->scale_y);
	}

	switch (def->belong)
	{
	case INS_Graphic: real_graphics.push_back(rim); break;
	case INS_Texture: real_textures.push_back(rim); break;
	case INS_Flat:    real_flats.push_back(rim); break;
	case INS_Sprite:  real_sprites.push_back(rim); break;
	case INS_rottpic: raw_graphics.push_back(rim); break;

	default:
		I_Error("INTERNAL ERROR: Bad belong value: %d\n", def->belong);
	}

	return rim;
}

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

	SYS_ASSERT(lumps);

	for (i = 0; i < number; i++)
	{
		if (lumps[i] < 0)
			continue;

		AddImageFlat(W_GetLumpName(lumps[i]), lumps[i]);
	}
}

void W_ImageCreateLBM(int *lumps, int number)
{
	int i;

	SYS_ASSERT(lumps);

	for (i = 0; i < number; i++)
	{
		if (lumps[i] < 0)
			continue;

		AddLBMImage(W_GetLumpName(lumps[i]), lumps[i]);
	}
}

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

	SYS_ASSERT(defs);

	for (i = 0; i < number; i++)
	{
		if (defs[i] == NULL)
			continue;

		//if (rott_mode)
			//AddROTTPatchTexture(defs[i]->name, defs[i]);
		//else
		AddImageTexture(defs[i]->name, defs[i]);
	}
}

//
// Used to fill in the image array with sprites from the WAD.  The
// lumps come from those occurring between S_START and S_END markers
// in each existing wad.
//
// NOTE: it is assumed that each new sprite is unique i.e. the name
// does not collide with any existing sprite image.
//
const image_c *W_ImageCreateSprite(const char *name, int lump, bool is_weapon)
{
	SYS_ASSERT(lump >= 0);

	image_c *rim = AddImageGraphic(name, IMSRC_Sprite, lump, real_sprites);
	if (!rim)
		return NULL;

	// adjust sprite offsets so that (0,0) is normal
	epi::image_data_c *offsets = ReadAsEpiBlock(rim);

	if (is_weapon)
	{
		rim->offset_x += (320 / 2 - rim->actual_w / 2);  // loss of accuracy
		rim->offset_y += (200 - 32 - rim->actual_h);
	}
	else
	{
		rim->offset_x -= rim->actual_w / 2;   // loss of accuracy
		rim->offset_y -= rim->actual_h;
	}
	return rim;
}

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
	do_DebugDump(real_textures);

	L_WriteDebug("Flats ------------------------------\n");
	do_DebugDump(real_flats);

	L_WriteDebug("Sprites ------------------------------\n");
	do_DebugDump(real_sprites);

	L_WriteDebug("Graphics ------------------------------\n");
	do_DebugDump(real_graphics);
#endif
}

void W_ImageAddTX(int lump, const char *name, bool hires)
{
	if (hires)
	{
		const image_c *rim = do_Lookup(real_textures, name, -2);
		if (rim && rim->source_type != IMSRC_User)
		{
			AddImageGraphic(name, IMSRC_TX_HI, lump, real_textures, rim);
			return;
		}

		rim = do_Lookup(real_flats, name, -2);
		if (rim && rim->source_type != IMSRC_User)
		{
			AddImageGraphic(name, IMSRC_TX_HI, lump, real_flats, rim);
			return;
		}

		rim = do_Lookup(real_sprites, name, -2);
		if (rim && rim->source_type != IMSRC_User)
		{
			AddImageGraphic(name, IMSRC_TX_HI, lump, real_sprites, rim);
			return;
		}

		// we do it this way to force the original graphic to be loaded
		rim = W_ImageLookup(name, INS_Graphic, ILF_Exact | ILF_Null);

		if (rim && rim->source_type != IMSRC_User)
		{
			AddImageGraphic(name, IMSRC_TX_HI, lump, real_graphics, rim);
			return;
		}

		I_Warning("HIRES replacement '%s' has no counterpart.\n", name);
	}

	AddImageGraphic(name, IMSRC_TX_HI, lump, real_textures);
}

//
// Only used during sprite initialisation.  The returned array of
// images is guaranteed to be sorted by name.
//
// Use delete[] to free the returned array.
//
const image_c ** W_ImageGetUserSprites(int *count)
{
	// count number of user sprites
	(*count) = 0;

	real_image_container_c::iterator it;

	for (it = real_sprites.begin(); it != real_sprites.end(); it++)
	{
		image_c *rim = *it;

		if (rim->source_type == IMSRC_User)
			(*count) += 1;
	}

	if (*count == 0)
	{
		L_WriteDebug("W_ImageGetUserSprites(count = %d)\n", *count);
		return NULL;
	}

	const image_c ** array = new const image_c *[*count];
	int pos = 0;

	for (it = real_sprites.begin(); it != real_sprites.end(); it++)
	{
		image_c *rim = *it;

		if (rim->source_type == IMSRC_User)
			array[pos++] = rim;
	}

#define CMP(a, b)  (strcmp(W_ImageGetName(a), W_ImageGetName(b)) < 0)
	QSORT(const image_c *, array, (*count), CUTOFF);
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
//  IMAGE LOADING / UNLOADING
//

// TODO: make methods of image_c class
static bool IM_ShouldClamp(const image_c *rim)
{
	switch (rim->source_type)
	{
	case IMSRC_Graphic:
//	case IMSRC_ROTTGFX:
	case IMSRC_Raw320x200:
	case IMSRC_Sprite:
//	case IMSRC_ROTTRAW:
//	case IMSRC_ROTTLBM:
		return true;

	case IMSRC_User:
		switch (rim->source.user.def->belong)
		{
		case INS_Graphic:
		case INS_Sprite:
			return true;

		default:
			return false;
		}

	default:
		return false;
	}
}

static bool IM_ShouldMipmap(image_c *rim)
{
	// the "SKY" check here is a hack...
	if (strnicmp(rim->name, "SKY", 3) == 0)
		return false;

	switch (rim->source_type)
	{
	case IMSRC_Texture:
	case IMSRC_Flat:
	case IMSRC_TX_HI:
		return true;

	case IMSRC_User:
		switch (rim->source.user.def->belong)
		{
		case INS_Texture:
		case INS_Flat:
			return true;

		default:
			return false;
		}

	default:
		return false;
	}
}

static bool IM_ShouldSmooth(image_c *rim)
{
	// the "SKY" check here is a hack...
	if (strnicmp(rim->name, "SKY", 3) == 0)
		return true;

	// TODO: more smooth options

	return var_smoothing ? true : false;
}

static bool IM_ShouldHQ2X(image_c *rim)
{
	// Note: no need to check IMSRC_User, since those images are
	//       always PNG or JPEG (etc) and never palettised, hence
	//       the Hq2x scaling would never apply.

	if (hq2x_scaling == 0)
		return false;

	if (hq2x_scaling >= 3)
		return true;

	switch (rim->source_type)
	{
	case IMSRC_Graphic:
//	case IMSRC_ROTTGFX:
	case IMSRC_Raw320x200:
//	case IMSRC_ROTTRAW:
		// UI elements
		return true;
#if 0
	case IMSRC_Texture:
		// the "SKY" check here is a hack...
		if (strnicmp(rim->name, "SKY", 3) == 0)
			return true;
		break;
#endif
	case IMSRC_Sprite:
		if (hq2x_scaling >= 2)
			return true;
		break;

	default:
		break;
	}

	return false;
}

static int IM_PixelLimit(image_c *rim)
{
	if (IM_ShouldMipmap(rim))
		return 65536 * (1 << (2 * detail_level));

	return (1 << 24);
}

static void CreateUserBuiltinShadow(epi::image_data_c *img)
{
	SYS_ASSERT(img->bpp == 4);
	int hw = img->width / 2;
	int hh = img->height;
	for (int y = 0; y < img->height; y++)
		for (int x = 0; x < img->width; x++)
		{
			byte *dest = img->pixels + (y * img->width + x) * 4;
			float dx = 1.0f - abs(x - hw) / float(hw);
			float dy = (hh - y) / float(hh);
			float dst = 0.7071 * sqrt(dx * dx + dy * dy);
			int v = int(dst * 192.0f);
			*dest++ = 0;
			*dest++ = 0;
			*dest++ = 0;
			*dest = *dest != 0 ? v : 0;
		}
}

static GLuint LoadImageOGL(image_c *rim, const colourmap_c *trans2)
{
	bool clamp = IM_ShouldClamp(rim);
	bool mip = IM_ShouldMipmap(rim);
	bool smooth = IM_ShouldSmooth(rim);

	int max_pix = IM_PixelLimit(rim);

	const colourmap_c *trans = trans2 == (const colourmap_c *)-1 ? NULL : trans2;

	
	//I_Printf("LoadImageOGL: Loading \"%.*s\"\n",16,rim->name);
	
	if (rim->source_type == IMSRC_User)
	{
		if (rim->source.user.def->special & IMGSP_Clamp)
			clamp = true;

		if (rim->source.user.def->special & IMGSP_Mip)
			mip = true;
		else if (rim->source.user.def->special & IMGSP_NoMip)
			mip = false;

		if (rim->source.user.def->special & IMGSP_Smooth)
			smooth = true;
		else if (rim->source.user.def->special & IMGSP_NoSmooth)
			smooth = false;
	}

	const byte *what_palette = (const byte *)&playpal_data[0];
	bool what_pal_cached = false;

	static byte trans_pal[256 * 3];

	if (trans != NULL)
	{
		// Note: we don't care about source_palette here. It's likely that
		// the translation table itself would not match the other palette,
		// and so we would still end up with messed up colours.

		R_TranslatePalette(trans_pal, what_palette, trans);
		what_palette = trans_pal;
	}
	else if (rim->source_palette >= 0)
	{
		what_palette = (const byte *)W_CacheLumpNum(rim->source_palette);
		what_pal_cached = true;
	}

	epi::image_data_c *tmp_img = ReadAsEpiBlock(rim);

	/* add offsets if they were read from the file */
	if (tmp_img->grAb != nullptr)
	{
		rim->offset_x = tmp_img->grAb->x;
		rim->offset_y = tmp_img->grAb->y;
	}
	
	if (rim->liquid_type > LIQ_None && (swirling_flats == SWIRL_SMMU || swirling_flats == SWIRL_SMMUSWIRL))
	{
		tmp_img->Swirl(leveltime, rim->liquid_type);
		rim->swirled_gametic = gametic;
	}

	if (rim->opacity == OPAC_Unknown)
		rim->opacity = R_DetermineOpacity(tmp_img);

	if ((tmp_img->bpp == 1) && IM_ShouldHQ2X(rim))
	{
		bool solid = (rim->opacity == OPAC_Solid);

		epi::Hq2x::Setup(what_palette, solid ? -1 : TRANS_PIXEL);

		epi::image_data_c *scaled_img =
			epi::Hq2x::Convert(tmp_img, solid, false /* invert */);

		delete tmp_img;
		tmp_img = scaled_img;
	}
	else if (tmp_img->bpp == 1)
	{
		epi::image_data_c *rgb_img =
			R_PalettisedToRGB(tmp_img, what_palette, rim->opacity);

		delete tmp_img;
		tmp_img = rgb_img;
	}
	else if (tmp_img->bpp >= 3 && trans != NULL)
	{
		if (trans == font_whiten_map)
			tmp_img->Whiten();
		else
			R_PaletteRemapRGBA(tmp_img, what_palette, (const byte *)&playpal_data[0]);
	}

	if (trans2 == (const colourmap_c *)-1)
		CreateUserBuiltinShadow(tmp_img); // make shadow

	GLuint tex_id = R_UploadTexture(tmp_img,
		(clamp ? UPL_Clamp : 0) |
		(mip ? UPL_MipMap : 0) |
		(smooth ? UPL_Smooth : 0) |
		((rim->opacity == OPAC_Masked) ? UPL_Thresh : 0), max_pix);

	delete tmp_img;

	if (what_pal_cached)
		W_DoneWithLump(what_palette);


	return tex_id;
}

#if 0
static
void UnloadImageOGL(cached_image_t *rc, image_c *rim)
{
	glDeleteTextures(1, &rc->tex_id);

	for (unsigned int i = 0; i < rim->cache.size(); i++)
	{
		if (rim->cache[i] == rc)
		{
			rim->cache[i] = NULL;
			return;
		}
	}

	I_Error("INTERNAL ERROR: UnloadImageOGL: no such RC in cache !\n");
}

//
// UnloadImage
//
// Unloads a cached image from the cache list and frees all resources.
// Mainly just a switch to more specialised image unloaders.
//
static void UnloadImage(cached_image_t *rc)
{
	image_c *rim = rc->parent;

	SYS_ASSERT(rc);
	SYS_ASSERT(rc != &imagecachehead);
	SYS_ASSERT(rim);

	// unlink from the cache list
	Unlink(rc);

	UnloadImageOGL(rc, rim);

	delete rc;
}
#endif

//----------------------------------------------------------------------------
//  IMAGE LOOKUP
//----------------------------------------------------------------------------

//
// BackupTexture
//
static const image_c *BackupTexture(const char *tex_name, int flags)
{
	const image_c *rim;

	// backup plan: try a flat with the same name
	if (!(flags & ILF_Exact))
	{
		rim = do_Lookup(real_flats, tex_name);
		if (rim)
			return rim;
	}

	if (flags & ILF_Null)
		return NULL;

	M_WarnError("Unknown texture found in level: '%s'\n", tex_name);

	image_c *dummy;

	if (strnicmp(tex_name, "SKY", 3) == 0)
		dummy = CreateDummyImage(tex_name, 0x0000AA, 0x55AADD);
	else
		dummy = CreateDummyImage(tex_name, 0xAA5511, 0x663300);

	// keep dummy texture so that future lookups will succeed
	real_textures.push_back(dummy);
	return dummy;
}

//
// BackupFlat
//
static const image_c *BackupFlat(const char *flat_name, int flags)
{
	const image_c *rim;

	// backup plan 1: if lump exists and is right size, add it.
	if (!(flags & ILF_NoNew))
	{
		int i = W_CheckNumForName(flat_name);

		if (i >= 0)
		{
			rim = AddImageFlat(flat_name, i);
			if (rim)
				return rim;
		}
	}

	// backup plan 2: Texture with the same name ?
	if (!(flags & ILF_Exact))
	{
		rim = do_Lookup(real_textures, flat_name);
		if (rim)
			return rim;
	}

	if (flags & ILF_Null)
		return NULL;

	M_WarnError("Unknown flat found in level: '%s'\n", flat_name);

	image_c *dummy = CreateDummyImage(flat_name, 0x11AA11, 0x115511);

	// keep dummy flat so that future lookups will succeed
	real_flats.push_back(dummy);
	return dummy;
}


//
// BackupGraphic
//
static const image_c *BackupGraphic(const char *gfx_name, int flags)
{
	const image_c *rim;

	// backup plan 1: look for sprites and heretic-background
	if ((flags & (ILF_Exact | ILF_Font)) == 0)
	{
		rim = do_Lookup(real_graphics, gfx_name, IMSRC_Raw320x200);
		if (rim)
			return rim;

		rim = do_Lookup(real_graphics, gfx_name, IMSRC_rottpic);
		if (rim)
			return rim;

		rim = do_Lookup(real_graphics, gfx_name, IMSRC_ROTTGFX);
		if (rim)
			return rim;

		rim = do_Lookup(real_sprites, gfx_name);
		if (rim)
			return rim;
	}

	// not already loaded ?  Check if lump exists in wad, if so add it.
	if (!(flags & ILF_NoNew))
	{
		int i = W_CheckNumForName_GFX(gfx_name);

		if (i >= 0)
		{
			rim = AddImageGraphic(gfx_name, IMSRC_Graphic, i, real_graphics);
			if (rim)
				return rim;
		}
	}

	if (flags & ILF_Null)
		return NULL;

	M_WarnError("Unknown graphic: '%s'\n", gfx_name);

	image_c *dummy;

	if (flags & ILF_Font)
		dummy = CreateDummyImage(gfx_name, 0xFFFFFF, TRANS_PIXEL);
	else
		dummy = CreateDummyImage(gfx_name, 0xFF0000, TRANS_PIXEL);

	// keep dummy graphic so that future lookups will succeed
	real_graphics.push_back(dummy);
	return dummy;
}

static const image_c *BackupSprite(const char *spr_name, int flags)
{
	if (flags & ILF_Null)
		return NULL;

	return W_ImageForDummySprite();
}

const image_c *W_ImageLookup(const char *name, image_namespace_e type, int flags)
{
	//
	// Note: search must be case insensitive.
	//

	// "NoTexture" marker.
	if (!name || !name[0] || name[0] == '-')
		return NULL;

	// "Sky" marker.
	if (type == INS_Flat &&
		(stricmp(name, "F_SKY1") == 0 ||
			stricmp(name, "F_SKY") == 0))
	{
		return skyflatimage;
	}

	// compatibility hack (first texture in IWAD is a dummy)
	if (type == INS_Texture &&
		((stricmp(name, "AASTINKY") == 0) ||
		(stricmp(name, "AASHITTY") == 0) ||
			(stricmp(name, "BADPATCH") == 0) ||
			(stricmp(name, "ABADONE") == 0)))
	{
		return NULL;
	}

	const image_c *rim;

	if (type == INS_Texture)
	{
		rim = do_Lookup(real_textures, name);
		return rim ? rim : BackupTexture(name, flags);
	}
	if (type == INS_Flat)
	{
		rim = do_Lookup(real_flats, name);
		return rim ? rim : BackupFlat(name, flags);
	}
	if (type == INS_Sprite)
	{
		rim = do_Lookup(real_sprites, name);
		return rim ? rim : BackupSprite(name, flags);
	}
	if (type == INS_rottpic)
	{
		rim = do_Lookup(real_graphics, name);
		return rim ? rim : BackupGraphic(name, flags);
	}

	/* INS_Graphic */

	rim = do_Lookup(real_graphics, name);
	
	//do_DebugDump(real_graphics);

	return rim ? rim : BackupGraphic(name, flags);
}

const image_c *W_ImageForDummySprite(void)
{
	return dummy_sprite;
}

const image_c *W_ImageForDummySkin(void)
{
	return dummy_skin;
}

const image_c *W_ImageForHOMDetect(void)
{
	return dummy_hom[(framecount & 0x10) ? 1 : 0];
}

const image_c *W_ImageParseSaveString(char type, const char *name)
{
	// Used by the savegame code.

	// this name represents the sky (historical reasons)
	if (type == 'd' && stricmp(name, "DUMMY__2") == 0)
	{
		return skyflatimage;
	}

	switch (type)
	{
	case 'K':
		return skyflatimage;

	case 'F':
		return W_ImageLookup(name, INS_Flat);

	case 'P':
		return W_ImageLookup(name, INS_Graphic);

	case 'S':
		return W_ImageLookup(name, INS_Sprite);

	default:
		I_Warning("W_ImageParseSaveString: unknown type '%c'\n", type);
		/* FALL THROUGH */

	case 'd': /* dummy */
	case 'T':
		return W_ImageLookup(name, INS_Texture);
	}

	return NULL; /* NOT REACHED */
}

void W_ImageMakeSaveString(const image_c *image, char *type, char *namebuf)
{
	// Used by the savegame code

	if (image == skyflatimage)
	{
		*type = 'K';
		strcpy(namebuf, "F_SKY1");
		return;
	}

	const image_c *rim = (const image_c *)image;

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
	//case IMSRC_ROTTRAW:
	case IMSRC_ROTTGFX:
	case IMSRC_Graphic: (*type) = 'P'; return;

	//case IMSRC_ROTTLBM: (*type) = 'L'; return;

	case IMSRC_TX_HI:
	case IMSRC_Texture: (*type) = 'T'; return;

	case IMSRC_Flat:    (*type) = 'F'; return;

	case IMSRC_Sprite:  (*type) = 'S'; return;

	case IMSRC_Dummy:   (*type) = 'd'; return;

	default:
		I_Error("W_ImageMakeSaveString: bad type %d\n", rim->source_type);
		break;
	}
}

const char *W_ImageGetName(const image_c *image)
{
	const image_c *rim;

	rim = (const image_c *)image;

	return rim->name;
}

//----------------------------------------------------------------------------
//
//  IMAGE USAGE
//

static cached_image_t *ImageCacheOGL(image_c *rim,
	const colourmap_c *trans)
{
	// check if image + translation is already cached

	int free_slot = -1;

	cached_image_t *rc = NULL;

	for (int i = 0; i < (int)rim->cache.size(); i++)
	{
		rc = rim->cache[i];

		if (!rc)
		{
			free_slot = i;
			continue;
		}

		if (rc->trans_map == trans)
			break;

		rc = NULL;
	}

	if (!rc)
	{
		// add entry into cache
		rc = new cached_image_t;

		rc->parent = rim;
		rc->trans_map = trans;
		rc->hue = RGB_NO_VALUE;
		rc->tex_id = 0;

		InsertAtTail(rc);

		if (free_slot >= 0)
			rim->cache[free_slot] = rc;
		else
			rim->cache.push_back(rc);
	}

	SYS_ASSERT(rc);
	
	if (rim->liquid_type > LIQ_None && (swirling_flats == SWIRL_SMMU || swirling_flats == SWIRL_SMMUSWIRL))
	{
		if (rc->parent->liquid_type > LIQ_None && rc->parent->swirled_gametic != gametic)
		{
			if (rc->tex_id != 0)
			{
				glDeleteTextures(1, &rc->tex_id);
				rc->tex_id = 0;
			}
		}
	}

#if 0  // REMOVE
	if (rc->invalidated)
	{
		SYS_ASSERT(rc->tex_id != 0);

		glDeleteTextures(1, &rc->tex_id);

		rc->tex_id = 0;
		rc->invalidated = false;
	}
#endif

	if (rc->tex_id == 0)
	{
		// load image into cache
		rc->tex_id = LoadImageOGL(rim, trans);
	}

	return rc;
}

//
// The top-level routine for caching in an image.  Mainly just a
// switch to more specialised routines.
//
GLuint W_ImageCache(const image_c *image, bool anim,
	const colourmap_c *trans)
{
	// Intentional Const Override
	image_c *rim = (image_c *)image;

	// handle animations
	if (anim)
		rim = rim->anim.cur;

	cached_image_t *rc = ImageCacheOGL(rim, trans);

	SYS_ASSERT(rc->parent);

	return rc->tex_id;
}

#if 0
rgbcol_t W_ImageGetHue(const image_c *img)
{
	SYS_ASSERT(c);

	// Intentional Const Override
	cached_image_t *rc = ((cached_image_t *)c) - 1;

	SYS_ASSERT(rc->parent);

	return rc->hue;
}
#endif

void W_ImagePreCache(const image_c *image)
{
	W_ImageCache(image, false);

	// Intentional Const Override
	image_c *rim = (image_c *)image;

	// pre-cache alternative images for switches too
	if (strlen(rim->name) >= 4 &&
		(strnicmp(rim->name, "SW1", 3) == 0 ||
			strnicmp(rim->name, "SW2", 3) == 0))
	{
		char alt_name[16];

		strcpy(alt_name, rim->name);
		alt_name[2] = (alt_name[2] == '1') ? '2' : '1';

		image_c *alt = do_Lookup(real_textures, alt_name);

		if (alt) W_ImageCache(alt, false);
	}
}

//----------------------------------------------------------------------------

static void W_CreateDummyImages(void)
{
	dummy_sprite = CreateDummyImage("DUMMY_SPRITE", 0xFFFF00, TRANS_PIXEL);
	dummy_skin = CreateDummyImage("DUMMY_SKIN", 0xFF77FF, 0x993399);

	skyflatimage = CreateDummyImage("DUMMY_SKY", 0x0000AA, 0x55AADD);

	dummy_hom[0] = CreateDummyImage("DUMMY_HOM1", 0xFF3333, 0x000000);
	dummy_hom[1] = CreateDummyImage("DUMMY_HOM2", 0x000000, 0xFF3333);

	// make the dummy sprite easier to see
	{
		// Intentional Const Override
		image_c *dsp = (image_c *)dummy_sprite;

		dsp->scale_x = 3.0f;
		dsp->scale_y = 3.0f;
	}
}

//
// Initialises the image system.
//
bool W_InitImages(void)
{
	// check options
	if (M_CheckParm("-nosmoothing"))
		var_smoothing = 0;
	else if (M_CheckParm("-smoothing"))
		var_smoothing = 1;

	if (M_CheckParm("-nomipmap"))
		var_mipmapping = 0;
	else if (M_CheckParm("-mipmap"))
		var_mipmapping = 1;
	else if (M_CheckParm("-trilinear"))
		var_mipmapping = 2;
	//else if (M_CheckParm("-aafilter"))
		//var_mipmapping = 3;

	//M_CheckBooleanParm("dither", &var_dithering, false);

	W_CreateDummyImages();

	return true;
}

//
// Animate all the images.
//
void W_UpdateImageAnims(void)
{
	do_Animate(real_graphics);
	do_Animate(real_textures);
	do_Animate(real_flats);
	do_Animate(raw_graphics);
}

void W_DeleteAllImages(void)
{
	std::list<cached_image_t *>::iterator CI;

	for (CI = image_cache.begin(); CI != image_cache.end(); CI++)
	{
		cached_image_t *rc = *CI;
		SYS_ASSERT(rc);

		if (rc->tex_id != 0)
		{
			glDeleteTextures(1, &rc->tex_id);
			rc->tex_id = 0;
		}
	}

	DeleteSkyTextures();
	DeleteColourmapTextures();
}

//
// W_AnimateImageSet
//
// Sets up the images so they will animate properly.  Array is
// allowed to contain NULL entries.
//
// NOTE: modifies the input array of images.
//
void W_AnimateImageSet(const image_c ** images, int number, int speed)
{
	int i, total;
	image_c *rim, *other;

	SYS_ASSERT(images);
	SYS_ASSERT(speed > 0);

	// ignore images that are already animating
	for (i = 0, total = 0; i < number; i++)
	{
		// Intentional Const Override
		rim = (image_c *)images[i];

		if (!rim)
			continue;

		if (rim->anim.speed > 0)
			continue;

		images[total++] = images[i];
	}

	// anything left to animate ?
	if (total < 2)
		return;

	for (i = 0; i < total; i++)
	{
		// Intentional Const Override
		rim = (image_c *)images[i];
		other = (image_c *)images[(i + 1) % total];

		rim->anim.next = other;
		rim->anim.speed = rim->anim.count = speed;
	}
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
