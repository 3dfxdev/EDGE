//----------------------------------------------------------------------------
//  EDGE Generalised Image Handling
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2007  The EDGE Team.
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
#include "r_image.h"

#include "e_search.h"
#include "e_main.h"
#include "m_argv.h"
#include "m_misc.h"
#include "p_local.h"
#include "r_gldefs.h"
#include "r_sky.h"
#include "r_colors.h"
#include "w_texture.h"
#include "w_wad.h"
#include "z_zone.h"

#include "epi/endianess.h"
#include "epi/files.h"
#include "epi/filesystem.h"
#include "epi/memfile.h"

#include "epi/image_data.h"
#include "epi/image_hq2x.h"
#include "epi/image_png.h"
#include "epi/image_jpeg.h"
#include "epi/image_tga.h"

#include <limits.h>
#include <math.h>
#include <string.h>


// LIGHTING DEBUGGING
// #define MAKE_TEXTURES_WHITE  1

extern epi::image_data_c *ReadAsEpiBlock(image_c *rim);

extern epi::file_c *OpenUserFileOrLump(imagedef_c *def);

extern void CloseUserFileOrLump(imagedef_c *def, epi::file_c *f);

// FIXME: duplicated in r_doomtex
#define DUMMY_X  16
#define DUMMY_Y  16


extern GLuint W_SendGLTexture(epi::image_data_c *img,
					   bool clamp, bool nomip, bool smooth,
					   int max_pix, const byte *what_palette);

extern void PaletteRemapRGBA(epi::image_data_c *img,
	const byte *new_pal, const byte *old_pal);

//
// This structure is for "cached" images (i.e. ready to be used for
// rendering), and is the non-opaque version of cached_image_t.  A
// single structure is used for all image modes (Block and OGL).
//
// Note: multiple modes and/or multiple mips of the same image_c can
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
	image_c *parent;
  
	// colormap used for translated image, normally NULL
	const colourmap_c *trans_map;

	// general hue of image (skewed towards pure colors)
	rgbcol_t hue;

	// texture identifier within GL
	GLuint tex_id;
}
real_cached_image_t;



// Image container
class real_image_container_c : public epi::array_c
{
public:
	real_image_container_c() : epi::array_c(sizeof(image_c*)) {}
	~real_image_container_c() { Clear(); }

private:
	void CleanupObject(void *obj)
	{
		image_c *rim = *(image_c**)obj;
		
		if (rim)
			delete rim;
	}

public:
	// List Management
	int GetSize() { return array_entries; } 
	int Insert(image_c *rim) { return InsertObject((void*)&rim); }
	
	image_c* operator[](int idx) 
	{ 
		return *(image_c**)FetchObject(idx); 
	} 

	image_c *Lookup(const char *name, int source_type = -1)
	{
		// for a normal lookup, we want USER images to override
		if (source_type == -1)
		{
			image_c *rim = Lookup(name, IMSRC_User);  // recursion
			if (rim)
				return rim;
		}

		epi::array_iterator_c it;

		for (it = GetBaseIterator(); it.IsValid(); it++)
		{
			image_c *rim = ITERATOR_TO_TYPE(it, image_c*);
		
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
			image_c *rim = ITERATOR_TO_TYPE(it, image_c*);

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

	void DebugDump()
	{
		L_WriteDebug("{\n");

		epi::array_iterator_c it;
		for (it = GetBaseIterator(); it.IsValid(); it++)
		{
			image_c *rim = ITERATOR_TO_TYPE(it, image_c*);
		
			L_WriteDebug("   [%s] type %d: %dx%d < %dx%d\n",
				rim->name, rim->source_type,
				rim->actual_w, rim->actual_h,
				rim->total_w, rim->total_h);
		}

		L_WriteDebug("}\n");
	}

};


// mipmapping enabled ?
// 0 off, 1 bilinear, 2 trilinear
int use_mipmapping = 1;

bool use_smoothing = true;
bool use_dithering = false;

// total set of images
static real_image_container_c real_graphics;
static real_image_container_c real_textures;
static real_image_container_c real_flats;
static real_image_container_c real_sprites;

static real_image_container_c sky_merges;
static real_image_container_c dummies;

const image_c *skyflatimage;


// image cache (actually a ring structure)
static real_cached_image_t imagecachehead;


// tiny ring helpers
static INLINE void InsertAtTail(real_cached_image_t *rc)
{
	SYS_ASSERT(rc != &imagecachehead);

	rc->prev =  imagecachehead.prev;
	rc->next = &imagecachehead;

	rc->prev->next = rc;
	rc->next->prev = rc;
}
static INLINE void Unlink(real_cached_image_t *rc)
{
	SYS_ASSERT(rc != &imagecachehead);

	rc->prev->next = rc->next;
	rc->next->prev = rc->prev;
}



//----------------------------------------------------------------------------

//
//  IMAGE CREATION
//

static image_c *NewImage(int width, int height, bool solid)
{
	image_c *rim = new image_c;

	// clear newbie
	memset(rim, 0, sizeof(image_c));

	rim->actual_w = width;
	rim->actual_h = height;
	rim->total_w  = W_MakeValidSize(width);
	rim->total_h  = W_MakeValidSize(height);
	rim->offset_x = rim->offset_y = 0;
	rim->scale_x  = rim->scale_y = 1.0f;
	rim->img_solid = solid;

	// set initial animation info
	rim->anim.cur = rim;
	rim->anim.next = NULL;
	rim->anim.count = rim->anim.speed = 0;

	return rim;
}

static image_c *AddDummyImage(const char *name, rgbcol_t fg, rgbcol_t bg)
{
	image_c *rim;
  
	rim = NewImage(DUMMY_X, DUMMY_Y, (bg == TRANS_PIXEL));
 
 	strcpy(rim->name, name);

	rim->source_type = IMSRC_Dummy;
	rim->source_palette = -1;

	rim->source.dummy.fg = fg;
	rim->source.dummy.bg = bg;

	dummies.Insert(rim);

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
 
	rim->offset_x = offset_x;
	rim->offset_y = offset_y;

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

static image_c *AddImageTexture(const char *name, 
									 texturedef_t *tdef)
{
	image_c *rim;
 
	// assume it is non-solid, we'll update it when we know for sure
	rim = NewImage(tdef->width, tdef->height, false);
 
	strcpy(rim->name, name);

	rim->source_type = IMSRC_Texture;
	rim->source.texture.tdef = tdef;
	rim->source_palette = tdef->palette_lump;

	real_textures.Insert(rim);

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
   
	rim = NewImage(size, size, true);
 
	strcpy(rim->name, name);

	rim->source_type = IMSRC_Flat;
	rim->source.flat.lump = lump;
	rim->source_palette = W_GetPaletteForLump(lump);

	real_flats.Insert(rim);

	return rim;
}

static image_c *AddImageSkyMerge(const image_c *sky, int face, int size)
{
	static const char *face_names[6] = 
	{
		"SKYBOX_NORTH", "SKYBOX_EAST",
		"SKYBOX_SOUTH", "SKYBOX_WEST",
		"SKYBOX_TOP",   "SKYBOX_BOTTOM"
	};

	image_c *rim;
 
	rim = NewImage(size, size, true /* solid */);
 
	strcpy(rim->name, face_names[face]);

	rim->source_type = IMSRC_SkyMerge;
	rim->source.merge.sky = sky;
	rim->source.merge.face = face;
	rim->source_palette = -1;

	sky_merges.Insert(rim);

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
			w = h = 256; //!!!!! (detail_level == 2) ? 512 : 256;
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
				got_info = epi::JPEG_GetInfo(f, &w, &h, &solid);
			else if (def->format == LIF_TGA)
				got_info = epi::TGA_GetInfo(f, &w, &h, &solid);
			else
				got_info = epi::PNG_GetInfo(f, &w, &h, &solid);

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
 
	image_c *rim = NewImage(w, h, solid);
 
	rim->offset_x = def->x_offset;
	rim->offset_y = def->y_offset;

	rim->scale_x = def->scale * def->aspect;
	rim->scale_y = def->scale;

	strcpy(rim->name, def->ddf.name.GetString());

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

	SYS_ASSERT(lumps);

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

	SYS_ASSERT(defs);

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
const image_c ** W_ImageGetUserSprites(int *count)
{
	// count number of user sprites
	(*count) = 0;

	epi::array_iterator_c it;

	for (it=real_sprites.GetBaseIterator(); it.IsValid(); it++)
	{
		image_c *rim = ITERATOR_TO_TYPE(it, image_c*);
    
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

	for (it=real_sprites.GetBaseIterator(); it.IsValid(); it++)
	{
		image_c *rim = ITERATOR_TO_TYPE(it, image_c*);
    
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

static
real_cached_image_t *LoadImageOGL(image_c *rim, const colourmap_c *trans)
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

	real_cached_image_t *rc = new real_cached_image_t;

	rc->next = rc->prev = NULL;
	rc->parent = rim;
	rc->users = 0;
	rc->invalidated = false;
	rc->trans_map = trans;
	rc->hue = RGB_NO_VALUE;

	epi::image_data_c *tmp_img = ReadAsEpiBlock(rim);

/// if (strcmp(rim->name, "EDGETTL")==0)
/// DumpImage(tmp_img);

	if (var_hq_scale && (tmp_img->bpp == 1) &&
		(rim->source_type == IMSRC_Raw320x200 ||
		 rim->source_type == IMSRC_Graphic ||
		 (var_hq_all &&
		  (rim->source_type == IMSRC_Sprite ||
		   rim->source_type == IMSRC_Flat ||
		   rim->source_type == IMSRC_Texture))))
	{
//		epi::Hq2x::Setup(&playpal_data[0][0][0], TRANS_PIXEL);
		epi::Hq2x::Setup(what_palette, TRANS_PIXEL);

		epi::image_data_c *scaled_img =
			epi::Hq2x::Convert(tmp_img, rim->img_solid, true /* invert */);

		delete tmp_img;
		tmp_img = scaled_img;
	}

	if (tmp_img->bpp >= 3 && trans != NULL)
	{
		if (trans == font_whiten_map)
			tmp_img->Whiten();
		else
			PaletteRemapRGBA(tmp_img, what_palette, (const byte *) &playpal_data[0]);
	}

	rc->tex_id = W_SendGLTexture(tmp_img, clamp, nomip,
		smooth, max_pix, what_palette);

	delete tmp_img;

	if (what_pal_cached)
		W_DoneWithLump(what_palette);

	rc->users++;
	InsertAtTail(rc);

	return rc;
}


static
void UnloadImageOGL(real_cached_image_t *rc, image_c *rim)
{
	glDeleteTextures(1, &rc->tex_id);

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
	image_c *rim = rc->parent;

	SYS_ASSERT(rc);
	SYS_ASSERT(rc != &imagecachehead);
	SYS_ASSERT(rim);
	SYS_ASSERT(rc->users == 0);

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
		rim = real_flats.Lookup(tex_name);
		if (rim)
			return rim;
	}

	if (flags & ILF_Null)
		return NULL;

	M_WarnError("Unknown texture found in level: '%s'\n", tex_name);

   	if (strncasecmp(tex_name, "SKY", 3) == 0)
	{
		rim = dummies.Lookup("DUMMY_SKY");
		if (rim)
			return rim;
	}

	// return the texture dummy image
	rim = dummies.Lookup("DUMMY_TEXTURE");
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
		rim = real_textures.Lookup(flat_name);
		if (rim)
			return rim;
	}

	if (flags & ILF_Null)
		return NULL;

	M_WarnError("Unknown flat found in level: '%s'\n", flat_name);

	// return the flat dummy image
	rim = dummies.Lookup("DUMMY_FLAT");
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
		rim = real_graphics.Lookup(gfx_name, IMSRC_Raw320x200);
		if (rim)
			return rim;
  
		rim = real_sprites.Lookup(gfx_name);
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
	rim = dummies.Lookup((flags & ILF_Font) ? "DUMMY_FONT" : "DUMMY_GRAPHIC");
	SYS_ASSERT(rim);

	return rim;
}

//
// W_ImageLookup
//
// Note: search must be case insensitive.
//
const image_c *W_ImageLookup(const char *name, image_namespace_e type, int flags)
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
				
				return W_ImageForDummySprite();
			}
			break;

		default: /* INS_Graphic */
			rim = real_graphics.Lookup(name);
			if (! rim)
				return BackupGraphic(name, flags);
			break;
	}

	SYS_ASSERT(rim);

	return rim;
}

static const char *UserSkyFaceName(const char *base, int face)
{
	static char buffer[64];
	static char *letters = "NESWTB";

	sprintf(buffer, "%s_%c", base, letters[face]);
	return buffer;
}

// FIXME: duplicated in r_doomtex
static inline bool SkyIsNarrow(const image_c *sky)
{
	// check the aspect of the image
	return (IM_WIDTH(sky) / IM_HEIGHT(sky)) < 2.28f;
}


//
// W_ImageFromSkyMerge
// 
const image_c *W_ImageFromSkyMerge(const image_c *sky, int face)
{
	// check IMAGES.DDF for the face image
	// (they need to be Textures)

	const image_c *rim = (const image_c *)sky;
	const char *user_face_name = UserSkyFaceName(rim->name, face);

	rim = real_textures.Lookup(user_face_name);
	if (rim)
		return rim;

	// nope, look for existing sky-merge image

	epi::array_iterator_c it;

	for (it=sky_merges.GetBaseIterator(); it.IsValid(); it++)
	{
		rim = ITERATOR_TO_TYPE(it, image_c*);
    
		SYS_ASSERT(rim->source_type == IMSRC_SkyMerge);

		if (sky != rim->source.merge.sky)
			continue;

		if (face == rim->source.merge.face)
			return rim;

		// Optimisations:
		//    1. in MIRROR mode, bottom is same as top.
		//    2. when sky is narrow, south == north, west == east.
		// NOTE: we rely on lookup order of RGL_UpdateSkyBoxTextures.

		if (sky_stretch >= STRETCH_MIRROR &&
			face == WSKY_Bottom && rim->source.merge.face == WSKY_Top)
		{
#if 0  // DISABLED
			L_WriteDebug("W_ImageFromSkyMerge: using TOP for BOTTOM.\n");
			return rim;
#endif
		}
		else if (SkyIsNarrow(sky) &&
			(face == WSKY_South && rim->source.merge.face == WSKY_North))
		{
			L_WriteDebug("W_ImageFromSkyMerge: using NORTH for SOUTH.\n");
			return rim;
		}
		else if (SkyIsNarrow(sky) &&
			 (face == WSKY_West && rim->source.merge.face == WSKY_East))
		{
			L_WriteDebug("W_ImageFromSkyMerge: using EAST for WEST.\n");
			return rim;
		}
	}

	// use low-res box sides for low-res sky images
	int size = (sky->actual_h < 228) ? 128 : 256;

	rim = AddImageSkyMerge(sky, face, size);
	return rim;
}

//
// W_ImageForDummySprite
// 
const image_c *W_ImageForDummySprite(void)
{
	const image_c *rim = dummies.Lookup("DUMMY_SPRITE");
	SYS_ASSERT(rim);

	return rim;
}

//
// W_ImageParseSaveString
//
// Used by the savegame code.
// 
const image_c *W_ImageParseSaveString(char type, const char *name)
{
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
			rim = dummies.Lookup(name);
			if (rim)
				return rim;
			break;

		default:
			I_Warning("W_ImageParseSaveString: unknown type '%c'\n", type);
			break;
	}

	rim = real_graphics.Lookup(name); if (rim) return rim;
	rim = real_textures.Lookup(name); if (rim) return rim;
	rim = real_flats.Lookup(name);    if (rim) return rim;
	rim = real_sprites.Lookup(name);  if (rim) return rim;

	I_Warning("W_ImageParseSaveString: image [%c:%s] not found.\n", type, name);

	// return the texture dummy image
	rim = dummies.Lookup("DUMMY_TEXTURE");
	SYS_ASSERT(rim);

	return rim;
}

//
// W_ImageMakeSaveString
//
// Used by the savegame code.
// 
void W_ImageMakeSaveString(const image_c *image, char *type, char *namebuf)
{
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

		case IMSRC_SkyMerge:
		default:
			I_Error("W_ImageMakeSaveString: bad type %d\n", rim->source_type);
			break;
	}
}

//
// W_ImageGetName
//
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


static inline
real_cached_image_t *ImageCacheOGL(image_c *rim)
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
		rc->users++;
	}
	else
	{
		// load into cache
		rc = rim->ogl_cache = LoadImageOGL(rim, NULL);
	}

	SYS_ASSERT(rc);

	return rc;
}

real_cached_image_t *ImageCacheTransOGL(image_c *rim,
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

	SYS_ASSERT(rc);

	return rc;
}


//
// W_ImageCache
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

	real_cached_image_t *rc;

	if (trans)
		rc = ImageCacheTransOGL(rim, trans);
	else
		rc = ImageCacheOGL(rim);

	SYS_ASSERT(rc->parent);

	return rc->tex_id;
}


#if 0  // UNUSED
void W_ImageDone(real_cached_image_t *rc)
{
///---	real_cached_image_t *rc;
///---
///---	SYS_ASSERT(c);
///---
///---	// Intentional Const Override
///---	rc = ((real_cached_image_t *) c) - 1;

	SYS_ASSERT(rc->users > 0);

	rc->users--;

	if (rc->users == 0)
	{
		// move cached image to the end of the cache list.  This way,
		// the Most Recently Used (MRU) images are at the tail of the
		// list, and thus the Least Recently Used (LRU) images are at the
		// head of the cache list.

		Unlink(rc);
		InsertAtTail(rc);
	}
}
#endif


#if 0
rgbcol_t W_ImageGetHue(const image_c *img)
{
	SYS_ASSERT(c);

	// Intentional Const Override
	real_cached_image_t *rc = ((real_cached_image_t *) c) - 1;

	SYS_ASSERT(rc->parent);

	return rc->hue;
}
#endif


//
// W_ImagePreCache
// 
void W_ImagePreCache(const image_c *image)
{
	W_ImageCache(image, false);

	// Intentional Const Override
	image_c *rim = (image_c *) image;

	// pre-cache alternative images for switches too
	if (strlen(rim->name) >= 4 &&
		(strncasecmp(rim->name, "SW1", 3) == 0 ||
		 strncasecmp(rim->name, "SW2", 3) == 0 ))
	{
		char alt_name[16];

		strcpy(alt_name, rim->name);
		alt_name[2] = (alt_name[2] == '1') ? '2' : '1';

		image_c *alt = real_textures.Lookup(alt_name);

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
	AddDummyImage("DUMMY_SPRITE",  0xFFFF00, TRANS_PIXEL);
	AddDummyImage("DUMMY_FONT",    0xFFFFFF, TRANS_PIXEL);

	skyflatimage = AddDummyImage("DUMMY_SKY", 0x0000AA, 0x55AADD);
}

//
// W_InitImages
//
// Initialises the image system.
//
bool W_InitImages(void)
{
	// the only initialisation the cache list needs
	imagecachehead.next = imagecachehead.prev = &imagecachehead;

	real_graphics.Clear();
	real_textures.Clear();
	real_flats.Clear();
	real_sprites.Clear();

	sky_merges.Clear();
	dummies.Clear();

    // check options
	M_CheckBooleanParm("smoothing", &use_smoothing, false);
	M_CheckBooleanParm("dither", &use_dithering, false);

	if (M_CheckParm("-nomipmap"))
		use_mipmapping = 0;
	else if (M_CheckParm("-mipmap"))
		use_mipmapping = 1;
	else if (M_CheckParm("-trilinear"))
		use_mipmapping = 2;

	W_CreateDummyImages();

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
