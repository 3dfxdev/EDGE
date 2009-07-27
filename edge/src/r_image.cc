//----------------------------------------------------------------------------
//  EDGE Generalised Image Handling
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2009  The EDGE Team.
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

#include "g_state.h"
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


// MipMapping: 0 off, 1 bilinear, 2 trilinear
cvar_c r_mipmapping;
cvar_c r_smoothing;
cvar_c r_dithering;
cvar_c r_hq2x;


// LIGHTING DEBUGGING
// #define MAKE_TEXTURES_WHITE  1

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
	// parent image
	image_c *parent;
  
	// colormap used for translated image, normally NULL
	const colourmap_c *trans_map;

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
		image_c *rim = do_Lookup(bucket, name, image_c::User);  // recursion
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


// total set of images
static real_image_container_c real_graphics;
static real_image_container_c real_textures;
static real_image_container_c real_flats;
static real_image_container_c real_sprites;


const image_c *skyflatimage;

static const image_c *dummy_sprite;
static const image_c *dummy_skin;
static const image_c *dummy_hom[2];


// image cache
static std::list<cached_image_t *> image_cache;

int image_reset_counter = 0;


//----------------------------------------------------------------------------
//
//  IMAGE CREATION
//

image_c::image_c() : actual_w(0), actual_h(0), total_w(0), total_h(0),
					 source_type(Dummy),
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


static image_c *R_ImageCreateDummy(const char *name, rgbcol_t fg, rgbcol_t bg)
{
	image_c *rim;
  
	rim = NewImage(DUMMY_X, DUMMY_Y, (bg == TRANS_PIXEL) ? OPAC_Masked : OPAC_Solid);
 
 	strcpy(rim->name, name);

	rim->source_type = image_c::Dummy;
	rim->source_palette = -1;

	rim->source.dummy.fg = fg;
	rim->source.dummy.bg = bg;

	return rim;
}


static image_c *AddImageGraphic(const char *name, int type, int lump)
{
	/* used for Sprites too */

	int width, height, offset_x, offset_y;
  
	image_c *im;

	patch_t *pat = (patch_t *) W_LoadLumpNum(lump);
  
	width    = EPI_LE_S16(pat->width);
	height   = EPI_LE_S16(pat->height);
	offset_x = EPI_LE_S16(pat->leftoffset);
	offset_y = EPI_LE_S16(pat->topoffset);
  
	Z_Free(pat);

	// do some basic checks
	// !!! FIXME: identify lump types in wad code.
	if (width <= 0 || width > 2048 || height <= 0 || height > 512 ||
		ABS(offset_x) > 2048 || ABS(offset_y) > 1024)
	{
		// check for Heretic/Hexen images, which are raw 320x200 
		int length = W_LumpLength(lump);

		if (length == 320*200 && type == image_c::Graphic)
		{
			im = NewImage(320, 200, OPAC_Solid);
			strcpy(im->name, name);

			im->source_type = image_c::Raw320x200;
			im->source.flat.lump = lump;
			im->source_palette = W_GetFileForLump(lump);
			return im;
		}

		if (length == 64*64 || length == 64*65 || length == 64*128)
			I_Warning("Graphic '%s' seems to be a flat.\n", name);
		else
			I_Warning("Graphic '%s' is not valid.\n", name);

		return NULL;
	}
 
	// create new image
	im = NewImage(width, height, OPAC_Unknown);
 
	im->offset_x = offset_x;
	im->offset_y = offset_y;

	strcpy(im->name, name);

	im->source_type = type;
	im->source.graphic.lump = lump;
	im->source_palette = W_GetFileForLump(lump);

	if (type == image_c::Sprite)
		real_sprites.push_back(im);
	else
		real_graphics.push_back(im);

	return im;
}


const image_c * R_ImageCreateTexture(texturedef_t *tdef)
{
	image_c *im = NewImage(tdef->width, tdef->height);
 
	strcpy(im->name, tdef->name);

	im->source_type = image_c::Texture;
	im->source.texture.tdef = tdef;
	im->source_palette = tdef->file;

	// NOTE: assumes new texture will be unique!

	real_textures.push_back(im);

	return im;
}

const image_c * R_ImageCreateFlat(const char *name, int lump)
{
	int len = W_LumpLength(lump);
	int size;
  
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
			I_Warning("Skipping weird flat '%s' (%d bytes)\n", name, len);
			return NULL;
	}
   
	image_c *im = NewImage(size, size, OPAC_Solid);
 
	strcpy(im->name, name);

	im->source_type = image_c::Flat;
	im->source.flat.lump = lump;
	im->source_palette = W_GetFileForLump(lump);

	real_flats.push_back(im);

	// NOTE: assumes new flat will be unique!

	return im;
}


const image_c *R_ImageCreateSprite(const char *name, int lump, bool is_weapon)
{
	SYS_ASSERT(lump >= 0);

	// NOTE: assumes new flat will be unique!

	image_c *im = AddImageGraphic(name, image_c::Sprite, lump);
	if (! im)
		return NULL;

	// adjust sprite offsets so that (0,0) is normal
	if (is_weapon)
	{
		im->offset_x += (320 / 2  - im->actual_w / 2);  // loss of accuracy
		im->offset_y += (200 - 32 - im->actual_h);
	}
	else
	{
		im->offset_x -= im->actual_w / 2;   // loss of accuracy
		im->offset_y -= im->actual_h;
	}

	return im;
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
			const char *basename = def->info.c_str();

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
			L_WriteDebug("GETINFO [%s] : size %dx%d\n", def->name.c_str(), w, h);
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

	strcpy(rim->name, def->name.c_str());

	// fix the name : replace space with underscore
	for (int i = 0; rim->name[i]; i++)
		if (rim->name[i] == ' ')
			rim->name[i] = '_';

	rim->source_type = image_c::User;
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
// Add the images defined in IMAGES.DDF.
//
void R_ImageCreateUser(void)
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

		if (rim->source_type == image_c::User)
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
    
		if (rim->source_type == image_c::User)
			array[pos++] = rim;
	}

#define CMP(a, b)  (strcmp((a)->name, (b)->name) < 0)
	QSORT(const image_c *, array, (*count), CUTOFF);
#undef CMP

#if 0  // DEBUGGING
	L_WriteDebug("W_ImageGetUserSprites(count = %d)\n", *count);
	L_WriteDebug("{\n");

	for (pos = 0; pos < *count; pos++)
		L_WriteDebug("   %p = [%s] %dx%d\n", array[pos], array[pos]->name),
			array[pos]->actual_w, array[pos]->actual_h);
		
	L_WriteDebug("}\n");
#endif

	return array;
}


//----------------------------------------------------------------------------
//
//  IMAGE LOADING / UNLOADING
//

bool image_c::ShouldClamp() const
{
	switch (source_type)
	{
		case Graphic:
		case Raw320x200:
		case Sprite:
			return true;

		case User:
			switch (source.user.def->belong)
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

bool image_c::ShouldMipmap() const
{
   	// the "SKY" check here is a hack...
   	if (strnicmp(name, "SKY", 3) == 0)
		return false;

	switch (source_type)
	{
		case Texture:
		case Flat:
			return true;
		
		case User:
			switch (source.user.def->belong)
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

bool image_c::ShouldSmooth() const
{
   	// the "SKY" check here is a hack...
   	if (strnicmp(name, "SKY", 3) == 0)
		return true;

	// TODO: more smooth options

	return r_smoothing.d ? true : false;
}

bool image_c::ShouldHQ2X() const
{
	// Note: no need to check IMSRC_User, since those images are
	//       always PNG or JPEG (etc) and never palettised, hence
	//       the Hq2x scaling would never apply.
	
	if (r_hq2x.d <= 0)
		return false;

	if (r_hq2x.d >= 3)
		return true;

	switch (source_type)
	{
		case Graphic:
		case Raw320x200:
			// UI elements
			return true;
#if 0
		case IMSRC_Texture:
			// the "SKY" check here is a hack...
			if (strnicmp(rim->name, "SKY", 3) == 0)
				return true;
			break;
#endif
		case Sprite:
			if (r_hq2x.d >= 2)
				return true;
			break;

		default:
			break;
	}

	return false;
}

int image_c::PixelLimit() const
{
	if (ShouldMipmap())
		return 65536 * (1 << (2 * CLAMP(0, r_detaillevel.d, 2)));

	return (1 << 24);
}


static GLuint LoadImageOGL(image_c *rim, const colourmap_c *trans)
{
	bool clamp  = rim->ShouldClamp();
	bool mip    = rim->ShouldMipmap();
	bool smooth = rim->ShouldSmooth();
 
 	int max_pix = rim->PixelLimit();

	if (rim->source_type == image_c::User)
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
		what_palette = W_GetPaletteData(rim->source_palette);
	}


	epi::image_data_c *tmp_img = rim->ReadBlock();


	if (rim->opacity == OPAC_Unknown)
		rim->opacity = R_DetermineOpacity(tmp_img);

	if ((tmp_img->bpp == 1) && rim->ShouldHQ2X())
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
			R_PaletteRemapRGBA(tmp_img, what_palette, (const byte *) &playpal_data[0]);
	}


	GLuint tex_id = R_UploadTexture(tmp_img,
		(clamp  ? UPL_Clamp  : 0) |
		(mip    ? UPL_MipMap : 0) |
		(smooth ? UPL_Smooth : 0) |
		((rim->opacity == OPAC_Masked) ? UPL_Thresh : 0), max_pix);

	delete tmp_img;

	return tex_id;
}


//----------------------------------------------------------------------------
//  IMAGE LOOKUP
//----------------------------------------------------------------------------


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

	image_c *dummy;

   	if (strnicmp(tex_name, "SKY", 3) == 0)
		dummy = R_ImageCreateDummy(tex_name, 0x0000AA, 0x55AADD);
	else
		dummy = R_ImageCreateDummy(tex_name, 0xAA5511, 0x663300);

	// keep dummy texture so that future looks will succeed
	real_textures.push_back(dummy);
	return dummy;
}


static const image_c *BackupFlat(const char *flat_name, int flags)
{
	const image_c *rim;

	// backup plan 1: if lump exists and is right size, add it.
	if (! (flags & ILF_NoNew))
	{
		int i = W_CheckNumForName(flat_name);

		if (i >= 0)
		{
			rim = R_ImageCreateFlat(flat_name, i);
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

	image_c *dummy = R_ImageCreateDummy(flat_name, 0x11AA11, 0x115511);

	// keep dummy flat so that future looks will succeed
	real_flats.push_back(dummy);
	return dummy;
}


static const image_c *BackupGraphic(const char *gfx_name, int flags)
{
	const image_c *rim;

	// backup plan 1: look for sprites and heretic-background
	if ((flags & (ILF_Exact | ILF_Font)) == 0)
	{
		rim = do_Lookup(real_graphics, gfx_name, image_c::Raw320x200);
		if (rim)
			return rim;
  
		rim = do_Lookup(real_sprites, gfx_name);
		if (rim)
			return rim;
	}
  
	// not already loaded ?  If lump exists in wad, create new gfx.
	if (! (flags & ILF_NoNew))
	{
		int i = W_CheckNumForName(gfx_name);

		if (i >= 0)
		{
			rim = AddImageGraphic(gfx_name, image_c::Graphic, i);
			if (rim)
				return rim;
		}
	}

	if (flags & ILF_Null)
		return NULL;

	M_WarnError("Unknown graphic: '%s'\n", gfx_name);

	image_c *dummy;

	if (flags & ILF_Font)
		dummy = R_ImageCreateDummy(gfx_name, 0xFFFFFF, TRANS_PIXEL);
	else
		dummy = R_ImageCreateDummy(gfx_name, 0xFF0000, TRANS_PIXEL);

	// keep dummy graphic so that future looks will succeed
	real_graphics.push_back(dummy);
	return dummy;
}


static const image_c *BackupSprite(const char *spr_name, int flags)
{
	if (flags & ILF_Null)
		return NULL;

	return R_ImageForDummySprite();
}


const image_c *R_ImageLookup(const char *name, image_namespace_e type, int flags)
{
	//
	// Note: search must be case insensitive.
	//

	// "NoTexture" marker.
	if (!name || !name[0] || name[0] == '-')
		return NULL;

	// "Sky" marker.
	if (type == INS_Flat && 
		( (stricmp(name, "F_SKY1") == 0) ||
		  (stricmp(name, "F_SKY")  == 0)))
	{
		return skyflatimage;
	}

	// compatibility hack (first texture in TEXTURE1 lump is only a placeholder)
	if (type == INS_Texture &&
		( (stricmp(name, "AASTINKY") == 0) ||
		  (stricmp(name, "AASHITTY") == 0) ||
		  (stricmp(name, "BADPATCH") == 0) ||
		  (stricmp(name, "ABADONE")  == 0)))
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

	/* INS_Graphic */

	rim = do_Lookup(real_graphics, name);
	return rim ? rim : BackupGraphic(name, flags);
}


const image_c *R_ImageForDummySprite(void)
{
	return dummy_sprite;
}

const image_c *R_ImageForDummySkin(void)
{
	return dummy_skin;
}

const image_c *R_ImageForHOMDetect(void)
{
	return dummy_hom[(framecount & 0x10) ? 1 : 0];
}


const image_c *R_ImageParseSaveString(char type, const char *name)
{
	// Used by the savegame code.

	switch (type)
	{
		case 'K':
			return skyflatimage;

		case 'F':
			return R_ImageLookup(name, INS_Flat);

		case 'G':
			return R_ImageLookup(name, INS_Graphic);

		case 'S':
			return R_ImageLookup(name, INS_Sprite);

		default:
			I_Warning("W_ImageParseSaveString: unknown type '%c'\n", type);
			/* FALL THROUGH */
		
		case 'd': /* dummy */
		case 'T':
			return R_ImageLookup(name, INS_Texture);
	}

	return NULL; /* NOT REACHED */
}


void R_ImageMakeSaveString(const image_c *image, char *type, char *namebuf)
{
	// Used by the savegame code

    if (image == skyflatimage)
	{
		*type = 'K';
		strcpy(namebuf, "F_SKY1");
		return;
	}

	strcpy(namebuf, image->name);

	/* handle User images (convert to a more general type) */
	if (image->source_type == image_c::User)
	{
		switch (image->source.user.def->belong)
		{
			case INS_Texture: (*type) = 'T'; return;
			case INS_Flat:    (*type) = 'F'; return;
			case INS_Sprite:  (*type) = 'S'; return;

			default:  /* INS_Graphic */
				(*type) = 'G';
				return;
		}
	}

	switch (image->source_type)
	{
		case image_c::Raw320x200:
		case image_c::Graphic: (*type) = 'G'; return;

		case image_c::Texture: (*type) = 'T'; return;
		case image_c::Flat:    (*type) = 'F'; return;
		case image_c::Sprite:  (*type) = 'S'; return;

		case image_c::Dummy:   (*type) = 'd'; return;

		default:
			I_Error("W_ImageMakeSaveString: bad type %d\n", image->source_type);
			break;
	}
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

		if (! rc)
		{
			free_slot = i;
			continue;
		}

		if (rc->trans_map == trans)
			break;

		rc = NULL;
	}

	if (! rc)
	{
		// add entry into cache
		rc = new cached_image_t;

		rc->parent = rim;
		rc->trans_map = trans;
		rc->tex_id = 0;

		if (free_slot >= 0)
			rim->cache[free_slot] = rc;
		else
			rim->cache.push_back(rc);

		image_cache.push_back(rc);
	}

	SYS_ASSERT(rc);

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
GLuint image_c::Cache(bool anim, const colourmap_c *trans) const
{
	// Intentional Const Override
	image_c *rim = (image_c *) this;
 
	// handle animations
	if (anim)
		rim = rim->anim.cur;

	cached_image_t *rc = ImageCacheOGL(rim, trans);

	SYS_ASSERT(rc->parent);

	return rc->tex_id;
}


void image_c::PreCache() const
{
	Cache(false);

	// Intentional Const Override
	image_c *rim = (image_c *) this;

	// pre-cache alternative images for switches too
	if (strlen(rim->name) >= 4 &&
		(strnicmp(rim->name, "SW1", 3) == 0 ||
		 strnicmp(rim->name, "SW2", 3) == 0 ))
	{
		char alt_name[16];

		strcpy(alt_name, rim->name);
		alt_name[2] = (alt_name[2] == '1') ? '2' : '1';

		image_c *alt = do_Lookup(real_textures, alt_name);

		if (alt) alt->Cache(false);
	}
}


//----------------------------------------------------------------------------

static void CreateDummyImages(void)
{
	// setup dummy images

	dummy_sprite = R_ImageCreateDummy("DUMMY_SPRITE", 0xFFFF00, TRANS_PIXEL);
	dummy_skin   = R_ImageCreateDummy("DUMMY_SKIN",   0xFF77FF, 0x993399);

	skyflatimage = R_ImageCreateDummy("DUMMY_SKY",    0x0000AA, 0x55AADD);

	dummy_hom[0] = R_ImageCreateDummy("DUMMY_HOM1",   0xFF3333, 0x000000);
	dummy_hom[1] = R_ImageCreateDummy("DUMMY_HOM2",   0x000000, 0xFF3333);

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
bool R_InitImages(void)
{
	CreateDummyImages();

	return true;
}


//
// Animate all the images.
//
void R_UpdateImageAnims(void)
{
	do_Animate(real_graphics);
	do_Animate(real_textures);
	do_Animate(real_flats);
}


void R_DeleteAllImages(void)
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

	image_reset_counter++;
}


//
// R_AnimateImageSet
//
// Sets up the images so they will animate properly.  Array is
// allowed to contain NULL entries.
//
// NOTE: modifies the input array of images.
// 
void R_AnimateImageSet(const image_c ** images, int number, int speed)
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
