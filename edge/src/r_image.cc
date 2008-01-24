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
//
// -AJA- 2000/06/25: Began this image generalisation, based on Erik
//       Sandberg's w_textur.c/h code.
//
// TODO HERE:
//   -  faster search methods.
//   -  do some optimisation
//

#include "i_defs.h"
#include "i_defs_gl.h"

#include <limits.h>
#include <list>

#include "epi/endianess.h"
#include "epi/file.h"
#include "epi/filesystem.h"
#include "epi/file_memory.h"

#include "epi/image_data.h"
#include "epi/image_hq2x.h"
#include "epi/image_png.h"
#include "epi/image_jpeg.h"
#include "epi/image_tga.h"

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


// LIGHTING DEBUGGING
// #define MAKE_TEXTURES_WHITE  1

extern epi::image_data_c *ReadAsEpiBlock(image_c *rim);

extern epi::file_c *OpenUserFileOrLump(imagedef_c *def);

extern void CloseUserFileOrLump(imagedef_c *def, epi::file_c *f);

// FIXME: duplicated in r_doomtex
#define DUMMY_X  16
#define DUMMY_Y  16


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
	// link in cache list
	struct cached_image_s *next, *prev;

	// true if image has been invalidated -- unload a.s.a.p
	bool invalidated;
 
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
                          int source_type = -1)
{
	// for a normal lookup, we want USER images to override
	if (source_type == -1)
	{
		image_c *rim = do_Lookup(bucket, name, IMSRC_User);  // recursion
		if (rim)
			return rim;
	}

	real_image_container_c::iterator it;

	for (it = bucket.begin(); it != bucket.end(); it++)
	{
		image_c *rim = *it;
	
		if (source_type != -1 && source_type != (int)rim->source_type)
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
			rim->anim.cur   = rim->anim.cur->anim.next;
			rim->anim.count = rim->anim.speed;
		}
	}
}

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

// mipmapping enabled ?
// 0 off, 1 bilinear, 2 trilinear
int var_mipmapping = 1;

int var_smoothing  = 1;

bool var_dithering = false;

int hq2x_scaling = 1;


// total set of images
static real_image_container_c real_graphics;
static real_image_container_c real_textures;
static real_image_container_c real_flats;
static real_image_container_c real_sprites;

static real_image_container_c dummies;


const image_c *skyflatimage;

static const image_c *dummy_sprite;
static const image_c *dummy_skin;


// image cache (actually a ring structure)
static cached_image_t imagecachehead;

int image_reset_counter = 0;


// tiny ring helpers
static inline void InsertAtTail(cached_image_t *rc)
{
	SYS_ASSERT(rc != &imagecachehead);

	rc->prev =  imagecachehead.prev;
	rc->next = &imagecachehead;

	rc->prev->next = rc;
	rc->next->prev = rc;
}
static inline void Unlink(cached_image_t *rc)
{
	SYS_ASSERT(rc != &imagecachehead);

	rc->prev->next = rc->next;
	rc->next->prev = rc->prev;
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

	memset(&source, 0, sizeof(source));
	memset(&anim,   0, sizeof(anim));
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
	rim->total_w  = W_MakeValidSize(width);
	rim->total_h  = W_MakeValidSize(height);
	rim->offset_x = rim->offset_y = 0;
	rim->scale_x  = rim->scale_y = 1.0f;
	rim->opacity  = opacity;

	// set initial animation info
	rim->anim.cur = rim;
	rim->anim.next = NULL;
	rim->anim.count = rim->anim.speed = 0;

	return rim;
}

static image_c *AddDummyImage(const char *name, rgbcol_t fg, rgbcol_t bg)
{
	image_c *rim;
  
	rim = NewImage(DUMMY_X, DUMMY_Y, (bg == TRANS_PIXEL) ? OPAC_Masked : OPAC_Solid);
 
 	strcpy(rim->name, name);

	rim->source_type = IMSRC_Dummy;
	rim->source_palette = -1;

	rim->source.dummy.fg = fg;
	rim->source.dummy.bg = bg;

	dummies.push_back(rim);

	return rim;
}

static image_c *AddImageGraphic(const char *name,
									 image_source_e type, int lump)
{
	/* used for Sprites too */

	patch_t *pat;
	int width, height, offset_x, offset_y;
  
	image_c *rim;

	pat = (patch_t *) W_CacheLumpNum(lump);
  
	width    = EPI_LE_S16(pat->width);
	height   = EPI_LE_S16(pat->height);
	offset_x = EPI_LE_S16(pat->leftoffset);
	offset_y = EPI_LE_S16(pat->topoffset);
  
	W_DoneWithLump(pat);

	// do some basic checks
	// !!! FIXME: identify lump types in wad code.
	if (width <= 0 || width > 2048 || height <= 0 || height > 512 ||
		ABS(offset_x) > 2048 || ABS(offset_y) > 1024)
	{
		// check for Heretic/Hexen images, which are raw 320x200 
		int length = W_LumpLength(lump);

		if (length == 320*200 && type == IMSRC_Graphic)
		{
			rim = NewImage(320, 200, OPAC_Solid);
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
	rim = NewImage(width, height, OPAC_Unknown);
 
	rim->offset_x = offset_x;
	rim->offset_y = offset_y;

	strcpy(rim->name, name);

	rim->source_type = type;
	rim->source.graphic.lump = lump;
	rim->source_palette = W_GetPaletteForLump(lump);

	if (type == IMSRC_Sprite)
		real_sprites.push_back(rim);
	else
		real_graphics.push_back(rim);

	return rim;
}

static image_c *AddImageTexture(const char *name, texturedef_t *tdef)
{
	image_c *rim;
 
	rim = NewImage(tdef->width, tdef->height);
 
	strcpy(rim->name, name);

	rim->source_type = IMSRC_Texture;
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
   
	rim = NewImage(size, size, OPAC_Solid);
 
	strcpy(rim->name, name);

	rim->source_type = IMSRC_Flat;
	rim->source.flat.lump = lump;
	rim->source_palette = W_GetPaletteForLump(lump);

	real_flats.push_back(rim);

	return rim;
}


static image_c *AddImageUser(imagedef_c *def)
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
			const char *basename = def->name.c_str();

			epi::file_c *f = OpenUserFileOrLump(def);

			if (! f)
			{
				I_Warning("Unable to add image %s: %s\n", 
					(def->type == IMGDT_Lump) ? "lump" : "file", basename);
				return NULL;
			}

			bool got_info;

			if (def->format == LIF_JPEG)
				got_info = epi::JPEG_GetInfo(f, &w, &h, &solid);
			else if (def->format == LIF_TGA)
				got_info = epi::TGA_GetInfo(f, &w, &h, &solid);
			else
				got_info = epi::PNG_GetInfo(f, &w, &h, &solid);

			if (! got_info)
				I_Error("Error occurred scanning image: %s\n", basename);

			CloseUserFileOrLump(def, f);
#if 1
			L_WriteDebug("GETINFO [%s] : size %dx%d\n", def->ddf.name.c_str(), w, h);
#endif
		}
		break;

		default:
			I_Error("AddImageUser: Coding error, unknown type %d\n", def->type);
			return NULL; /* NOT REACHED */
	}
 
	image_c *rim = NewImage(w, h, solid ? OPAC_Solid : OPAC_Unknown);
 
	rim->offset_x = def->x_offset;
	rim->offset_y = def->y_offset;

	rim->scale_x = def->scale * def->aspect;
	rim->scale_y = def->scale;

	strcpy(rim->name, def->ddf.name.c_str());

	/* FIX NAME : replace space with '_' */
	for (int i = 0; rim->name[i]; i++)
		if (rim->name[i] == ' ')
			rim->name[i] = '_';

	rim->source_type = IMSRC_User;
	rim->source.user.def = def;

	if (def->special & IMGSP_Crosshair)
	{
		float dy = (200.0f - rim->actual_h * rim->scale_y) / 2.0f - WEAPONTOP;

		rim->offset_y += int(dy / rim->scale_y);
	}

	switch (def->belong)
	{
		case INS_Graphic: real_graphics.push_back(rim); break;
		case INS_Texture: real_textures.push_back(rim); break;
		case INS_Flat:    real_flats.   push_back(rim); break;
		case INS_Sprite:  real_sprites. push_back(rim); break;

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

	for (i=0; i < number; i++)
	{
		if (lumps[i] < 0)
			continue;
    
		AddImageFlat(W_GetLumpName(lumps[i]), lumps[i]);
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

	for (i=0; i < number; i++)
	{
		if (defs[i] == NULL)
			continue;
    
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

	image_c *rim = AddImageGraphic(name, IMSRC_Sprite, lump);
	if (! rim)
		return NULL;

	// adjust sprite offsets so that (0,0) is normal
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
		case IMSRC_Raw320x200:
		case IMSRC_Sprite:
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
		case IMSRC_Raw320x200:
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


static
cached_image_t *LoadImageOGL(image_c *rim, const colourmap_c *trans)
{
	bool clamp  = IM_ShouldClamp(rim);
	bool mip    = IM_ShouldMipmap(rim);
	bool smooth = IM_ShouldSmooth(rim);
 
 	int max_pix = IM_PixelLimit(rim);

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


	const byte *what_palette = (const byte *) &playpal_data[0];
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
		what_palette = (const byte *) W_CacheLumpNum(rim->source_palette);
		what_pal_cached = true;
	}


	cached_image_t *rc = new cached_image_t;

	rc->next = rc->prev = NULL;
	rc->parent = rim;
	rc->invalidated = false;
	rc->trans_map = trans;
	rc->hue = RGB_NO_VALUE;


	epi::image_data_c *tmp_img = ReadAsEpiBlock(rim);

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

	if (tmp_img->bpp >= 3 && trans != NULL)
	{
		if (trans == font_whiten_map)
			tmp_img->Whiten();
		else
			R_PaletteRemapRGBA(tmp_img, what_palette, (const byte *) &playpal_data[0]);
	}

	if (tmp_img->bpp == 1)
	{
		epi::image_data_c *rgb_img =
				R_PalettisedToRGB(tmp_img, what_palette, rim->opacity);

		delete tmp_img;
		tmp_img = rgb_img;
	}


	rc->tex_id = R_UploadTexture(tmp_img,
		(clamp  ? UPL_Clamp  : 0) |
		(mip    ? UPL_MipMap : 0) |
		(smooth ? UPL_Smooth : 0) |
		((rim->opacity == OPAC_Masked) ? UPL_Thresh : 0), max_pix);

	delete tmp_img;

	if (what_pal_cached)
		W_DoneWithLump(what_palette);

	InsertAtTail(rc);

	return rc;
}


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
	if (! (flags & ILF_Exact))
	{
		rim = do_Lookup(real_flats, tex_name);
		if (rim)
			return rim;
	}

	if (flags & ILF_Null)
		return NULL;

	M_WarnError("Unknown texture found in level: '%s'\n", tex_name);

   	if (strnicmp(tex_name, "SKY", 3) == 0)
	{
		rim = do_Lookup(dummies, "DUMMY_SKY");
		if (rim)
			return rim;
	}

	// return the texture dummy image
	rim = do_Lookup(dummies, "DUMMY_TEXTURE");
	SYS_ASSERT(rim);

	return rim;
}

//
// BackupFlat
//
//
static const image_c *BackupFlat(const char *flat_name, int flags)
{
	const image_c *rim;

	// backup plan 1: if lump exists and is right size, add it.
	if (! (flags & ILF_NoNew))
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
	if (! (flags & ILF_Exact))
	{
		rim = do_Lookup(real_textures, flat_name);
		if (rim)
			return rim;
	}

	if (flags & ILF_Null)
		return NULL;

	M_WarnError("Unknown flat found in level: '%s'\n", flat_name);

	// return the flat dummy image
	rim = do_Lookup(dummies, "DUMMY_FLAT");
	SYS_ASSERT(rim);

	return rim;
}

//
// BackupGraphic
//
static const image_c *BackupGraphic(const char *gfx_name, int flags)
{
	const image_c *rim;

	// backup plan 1: look for sprites and heretic-background
	if (! (flags & (ILF_Exact | ILF_Font)))
	{
		rim = do_Lookup(real_graphics, gfx_name, IMSRC_Raw320x200);
		if (rim)
			return rim;
  
		rim = do_Lookup(real_sprites, gfx_name);
		if (rim)
			return rim;
	}
  
	// not already loaded ?  Check if lump exists in wad, if so add it.
	if (! (flags & ILF_NoNew))
	{
		int i = W_CheckNumForName(gfx_name);

		if (i >= 0)
		{
			rim = AddImageGraphic(gfx_name, IMSRC_Graphic, i);
			if (rim)
				return rim;
		}
	}

	if (flags & ILF_Null)
		return NULL;

	M_WarnError("Unknown graphic: '%s'\n", gfx_name);

	// return the graphic dummy image
	rim = do_Lookup(dummies, (flags & ILF_Font) ? "DUMMY_FONT" : "DUMMY_GRAPHIC");
	SYS_ASSERT(rim);

	return rim;
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
	if (type == INS_Flat && (stricmp(name, "F_SKY1") == 0) ||
		(stricmp(name, "F_SKY") == 0))
	{
		return skyflatimage;
	}

	// compatibility hack (first texture in IWAD is a dummy)
	if (type == INS_Texture &&
		( (stricmp(name, "AASTINKY") == 0) ||
		  (stricmp(name, "AASHITTY") == 0) ||
		  (stricmp(name, "BADPATCH") == 0) ||
		  (stricmp(name, "ABADONE")  == 0)))
	{
	    return NULL;
	}
  
	const image_c *rim = NULL;

	switch (type)
	{
		case INS_Texture:
			rim = do_Lookup(real_textures, name);
			if (! rim)
				return BackupTexture(name, flags);
			break;

		case INS_Flat:
			rim = do_Lookup(real_flats, name);
			if (! rim)
				return BackupFlat(name, flags);
			break;

		case INS_Sprite:
			rim = do_Lookup(real_sprites, name);
			if (! rim)
			{
				if (flags & ILF_Null)
					return NULL;
				
				return W_ImageForDummySprite();
			}
			break;

		default: /* INS_Graphic */
			rim = do_Lookup(real_graphics, name);
			if (! rim)
				return BackupGraphic(name, flags);
			break;
	}

	SYS_ASSERT(rim);

	return rim;
}


const image_c *W_ImageForDummySprite(void)
{
	return dummy_sprite;
#if 0
	const image_c *rim = do_Lookup(dummies, "DUMMY_SPRITE");
	SYS_ASSERT(rim);

	return rim;
#endif
}


const image_c *W_ImageForDummySkin(void)
{
	return dummy_skin;
}


const image_c *W_ImageParseSaveString(char type, const char *name)
{
	// Used by the savegame code.

	// this name represents the sky (historical reasons)
	if (type == 'd' && stricmp(name, "DUMMY__2") == 0)
	{
		return skyflatimage;
	}

	const image_c *rim;

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
			rim = do_Lookup(dummies, name);
			if (rim)
				return rim;
			break;

		default:
			I_Warning("W_ImageParseSaveString: unknown type '%c'\n", type);
			break;
	}

	rim = do_Lookup(real_graphics, name); if (rim) return rim;
	rim = do_Lookup(real_textures, name); if (rim) return rim;
	rim = do_Lookup(real_flats, name);    if (rim) return rim;
	rim = do_Lookup(real_sprites, name);  if (rim) return rim;

	I_Warning("W_ImageParseSaveString: image [%c:%s] not found.\n", type, name);

	// return the texture dummy image
	rim = do_Lookup(dummies, "DUMMY_TEXTURE");
	SYS_ASSERT(rim);

	return rim;
}


void W_ImageMakeSaveString(const image_c *image, char *type, char *namebuf)
{
	// Used by the savegame code

    if (image == skyflatimage)
	{
		// this name represents the sky (historical reasons)
		*type = 'd';
		strcpy(namebuf, "DUMMY__2");
		return;
	}

	const image_c *rim = (const image_c *) image;

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

		default:
			I_Error("W_ImageMakeSaveString: bad type %d\n", rim->source_type);
			break;
	}
}


const char *W_ImageGetName(const image_c *image)
{
	const image_c *rim;

	rim = (const image_c *) image;

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

		if (rc && rc->invalidated)
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
		{
			return rc; // found it!
		}
	}

	if (free_slot < 0)
	{
		free_slot = (int)rim->cache.size();

		rim->cache.push_back(NULL);
	}

	// load into cache
	rc = LoadImageOGL(rim, trans);
	SYS_ASSERT(rc);

	rim->cache[free_slot] = rc;

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
	image_c *rim = (image_c *) image;
 
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
	cached_image_t *rc = ((cached_image_t *) c) - 1;

	SYS_ASSERT(rc->parent);

	return rc->hue;
}
#endif


void W_ImagePreCache(const image_c *image)
{
	W_ImageCache(image, false);

	// Intentional Const Override
	image_c *rim = (image_c *) image;

	// pre-cache alternative images for switches too
	if (strlen(rim->name) >= 4 &&
		(strnicmp(rim->name, "SW1", 3) == 0 ||
		 strnicmp(rim->name, "SW2", 3) == 0 ))
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
	// setup dummy images
	AddDummyImage("DUMMY_TEXTURE", 0xAA5511, 0x663300);
	AddDummyImage("DUMMY_FLAT",    0x11AA11, 0x115511);

	AddDummyImage("DUMMY_GRAPHIC", 0xFF0000, TRANS_PIXEL);
	AddDummyImage("DUMMY_FONT",    0xFFFFFF, TRANS_PIXEL);

	dummy_sprite = AddDummyImage("DUMMY_SPRITE", 0xFFFF00, TRANS_PIXEL);
	dummy_skin   = AddDummyImage("DUMMY_SKIN",   0xFF77FF, 0x993399);

	skyflatimage = AddDummyImage("DUMMY_SKY",    0x0000AA, 0x55AADD);

	// make the dummy sprite easier to see
	{
		// Intentional Const Override
		image_c *dsp = (image_c *) dummy_sprite;

		dsp->scale_x = 3.0f;
		dsp->scale_y = 3.0f;
	}
}


//
// Initialises the image system.
//
bool W_InitImages(void)
{
	// the only initialisation the cache list needs
	imagecachehead.next = imagecachehead.prev = &imagecachehead;

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

	M_CheckBooleanParm("dither", &var_dithering, false);

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
}


//
// Resets all images, causing all cached images to be invalidated.
// Needs to be called when gamma changes (GL renderer only), or when
// certain other parameters change (e.g. GL mipmapping modes).
//
void W_ResetImages(void)
{
	cached_image_t *rc, *next;

	for (rc=imagecachehead.next; rc != &imagecachehead; rc=next)
	{
		next = rc->next;

		rc->invalidated = true;
	}

	image_reset_counter++;
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
	for (i=0, total=0; i < number; i++)
	{
		// Intentional Const Override
		rim = (image_c *) images[i];

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
		rim   = (image_c *) images[i];
		other = (image_c *) images[(i+1) % total];

		rim->anim.next = other;
		rim->anim.speed = rim->anim.count = speed;
	}
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
