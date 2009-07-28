//------------------------------------------------------------------------
//  RENDER IMAGES
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
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

#include "main.h"

#include "m_dialog.h"
#include "m_game.h"      /* yg_picture_format */
#include "r_misc.h"
#include "levels.h"
#include "w_patches.h"
#include "w_loadpic.h"
#include "w_sprites.h"
#include "w_file.h"
#include "w_io.h"
#include "w_structs.h"

#include "r_images.h"


/*
 *  flat_list_entry_match
 *  Function used by bsearch() to locate a particular 
 *  flat in the FTexture.
 */
static int flat_list_entry_match (const void *key, const void *flat_list_entry)
{
	return y_strnicmp ((const char *) key,
						((const flat_list_entry_t *) flat_list_entry)->name,
						WAD_FLAT_NAME);
}


/*
 *  load a flat into a new image.  NULL if not found.
 */

Img * Flat2Img (const char * fname)
{
	char name[WAD_FLAT_NAME + 1];
	strncpy (name, fname, WAD_FLAT_NAME);
	name[WAD_FLAT_NAME] = 0;

	flat_list_entry_t *flat = (flat_list_entry_t *)
		bsearch (name, flat_list, NumFTexture, sizeof *flat_list,
				flat_list_entry_match);

	if (! flat)  // Not found in list
		return 0;

	int width  = DOOM_FLAT_WIDTH;  // Big deal !
	int height = DOOM_FLAT_HEIGHT;

	const Wad_file *wadfile = flat->wadfile;
	wadfile->seek (flat->offset);

	Img *img = new Img (width, height, false);

	wadfile->read_bytes (img->wbuf (), (long) width * height);

	return img;
}


/*
 * load a wall texture ("TEXTURE1" or "TEXTURE2" object) into an image.
 * Returns NULL if not found or error.
 */

Img * Tex2Img (const char * texname)
{
	MDirPtr  dir = 0; /* main directory pointer to the TEXTURE* entries */
	s32_t     *offsets; /* array of offsets to texture names */
	int      n;   /* general counter */
	s16_t      width, height; /* size of the texture */
	s16_t      npatches;  /* number of wall patches used to build this texture */
	s32_t      numtex;  /* number of texture names in TEXTURE* list */
	s32_t      texofs;  /* offset in the wad file to the texture data */
	char     tname[WAD_TEX_NAME + 1]; /* texture name */
	char     picname[WAD_PIC_NAME + 1]; /* wall patch name */
	bool     have_dummy_bytes;
	int      header_size;
	int      item_size;

	char name[WAD_TEX_NAME + 1];
	strncpy (name, texname, WAD_TEX_NAME);
	name[WAD_TEX_NAME] = 0;

	// Iwad-dependant details
	if (yg_texture_format == YGTF_NAMELESS)
	{
		have_dummy_bytes = true;
		header_size      = 14;
		item_size        = 10;
	}
	else if (yg_texture_format == YGTF_NORMAL)
	{
		have_dummy_bytes = true;
		header_size      = 14;
		item_size        = 10;
	}
	else if (yg_texture_format == YGTF_STRIFE11)
	{
		have_dummy_bytes = false;
		header_size      = 10;
		item_size        = 6;
	}
	else
	{
		nf_bug ("Bad texture format %d.", (int) yg_texture_format);
		return 0;
	}

	/* offset for texture we want. */
	texofs = 0;
	// Doom alpha 0.4 : "TEXTURES", no names
	if (yg_texture_lumps == YGTL_TEXTURES && yg_texture_format == YGTF_NAMELESS)
	{
		dir = FindMasterDir (MasterDir, "TEXTURES");
		if (dir != NULL)
		{
			dir->wadfile->seek (dir->dir.start);
			dir->wadfile->read_i32 (&numtex);
			if (WAD_TEX_NAME < 7) nf_bug ("WAD_TEX_NAME too small");  // Sanity
			if (! y_strnicmp (name, "TEX", 3)
					&& isdigit (name[3])
					&& isdigit (name[4])
					&& isdigit (name[5])
					&& isdigit (name[6])
					&& name[7] == '\0')
			{
				long num;
				if (sscanf (name + 3, "%4ld", &num) == 1
						&& num >= 0 && num < numtex)
				{
					dir->wadfile->seek (dir->dir.start + 4 + 4 * num);
					dir->wadfile->read_i32 (&texofs);
					texofs += dir->dir.start;
				}
			}
		}
	}
	// Doom alpha 0.5 : only "TEXTURES"
	else if (yg_texture_lumps == YGTL_TEXTURES
			&& (yg_texture_format == YGTF_NORMAL || yg_texture_format == YGTF_STRIFE11))
	{
		// Is it in TEXTURES ?
		dir = FindMasterDir (MasterDir, "TEXTURES");
		if (dir != NULL)  // (Theoretically, it should always exist)
		{
			dir->wadfile->seek (dir->dir.start);
			dir->wadfile->read_i32 (&numtex);
			/* read in the offsets for texture1 names and info. */
			offsets = (s32_t *) GetMemory ((long) numtex * 4);
			dir->wadfile->read_i32 (offsets, numtex);
			for (n = 0; n < numtex && !texofs; n++)
			{
				dir->wadfile->seek (dir->dir.start + offsets[n]);
				dir->wadfile->read_bytes (&tname, WAD_TEX_NAME);
				if (!y_strnicmp (tname, name, WAD_TEX_NAME))
					texofs = dir->dir.start + offsets[n];
			}
			FreeMemory (offsets);
		}
	}
	// Other iwads : "TEXTURE1" and "TEXTURE2"
	else if (yg_texture_lumps == YGTL_NORMAL
			&& (yg_texture_format == YGTF_NORMAL || yg_texture_format == YGTF_STRIFE11))
	{
		// Is it in TEXTURE1 ?
		dir = FindMasterDir (MasterDir, "TEXTURE1");
		if (dir != NULL)  // (Theoretically, it should always exist)
		{
			dir->wadfile->seek (dir->dir.start);
			dir->wadfile->read_i32 (&numtex);
			/* read in the offsets for texture1 names and info. */
			offsets = (s32_t *) GetMemory ((long) numtex * 4);
			dir->wadfile->read_i32 (offsets, numtex);
			for (n = 0; n < numtex && !texofs; n++)
			{
				dir->wadfile->seek (dir->dir.start + offsets[n]);
				dir->wadfile->read_bytes (&tname, WAD_TEX_NAME);
				if (!y_strnicmp (tname, name, WAD_TEX_NAME))
					texofs = dir->dir.start + offsets[n];
			}
			FreeMemory (offsets);
		}
		// Well, then is it in TEXTURE2 ?
		if (texofs == 0)
		{
			dir = FindMasterDir (MasterDir, "TEXTURE2");
			if (dir != NULL)  // Doom II has no TEXTURE2
			{
				dir->wadfile->seek (dir->dir.start);
				dir->wadfile->read_i32 (&numtex);
				/* read in the offsets for texture2 names */
				offsets = (s32_t *) GetMemory ((long) numtex * 4);
				dir->wadfile->read_i32 (offsets, numtex);
				for (n = 0; n < numtex && !texofs; n++)
				{
					dir->wadfile->seek (dir->dir.start + offsets[n]);
					dir->wadfile->read_bytes (&tname, WAD_TEX_NAME);
					if (!y_strnicmp (tname, name, WAD_TEX_NAME))
						texofs = dir->dir.start + offsets[n];
				}
				FreeMemory (offsets);
			}
		}
	}
	else
		nf_bug ("Invalid texture_format/texture_lumps combination.");

	/* texture name not found */
	if (texofs == 0)
		return 0;

	/* read the info for this texture */
	s32_t header_ofs;
	if (yg_texture_format == YGTF_NAMELESS)
		header_ofs = texofs;
	else
		header_ofs = texofs + WAD_TEX_NAME;
	dir->wadfile->seek (header_ofs + 4);
	dir->wadfile->read_i16 (&width);
	dir->wadfile->read_i16 (&height);
	if (have_dummy_bytes)
	{
		s16_t dummy;
		dir->wadfile->read_i16 (&dummy);
		dir->wadfile->read_i16 (&dummy);
	}
	dir->wadfile->read_i16 (&npatches);

	/* Compose the texture */
	Img *texbuf = new Img (width, height, false);

	/* Paste onto the buffer all the patches that the texture is
	   made of. */
	for (n = 0; n < npatches; n++)
	{
		s16_t xofs, yofs;  // offset in texture space for the patch
		s16_t pnameind;  // index of patch in PNAMES

		dir->wadfile->seek (header_ofs + header_size + (long) n * item_size);
		dir->wadfile->read_i16 (&xofs);
		dir->wadfile->read_i16 (&yofs);
		dir->wadfile->read_i16 (&pnameind);

		if (have_dummy_bytes)
		{
			s16_t stepdir;
			s16_t colormap;
			dir->wadfile->read_i16 (&stepdir);   // Always 1, unused.
			dir->wadfile->read_i16 (&colormap);  // Always 0, unused.
		}

		/* AYM 1998-08-08: Yes, that's weird but that's what Doom
		   does. Without these two lines, the few textures that have
		   patches with negative y-offsets (BIGDOOR7, SKY1, TEKWALL1,
		   TEKWALL5 and a few others) would not look in the texture
		   viewer quite like in Doom. This should be mentioned in
		   the UDS, by the way. */
		if (yofs < 0)
			yofs = 0;

		Lump_loc loc;
		{
			wad_pic_name_t *wname = patch_dir.name_for_num (pnameind);
			if (wname == 0)
			{
				warn ("texture \"%.*s\": patch %2d has bad index %d.\n",
						WAD_TEX_NAME, tname, (int) n, (int) pnameind);
				continue;
			}
			patch_dir.loc_by_name ((const char *) *wname, loc);
			*picname = '\0';
			strncat (picname, (const char *) *wname, sizeof picname - 1);
		}

		if (LoadPicture (*texbuf, picname, loc, xofs, yofs, 0, 0))
			warn ("texture \"%.*s\": patch \"%.*s\" not found.\n",
					WAD_TEX_NAME, tname, WAD_PIC_NAME, picname);
	}

	return texbuf;
}


/* --- ImageCache methods --- */


Img *ImageCache::GetFlat (const char * fname)
{
	std::string f_str = fname;

	flat_map_t::iterator P = flats.find (f_str);

	if (P != flats.end ())
		return P->second;

	// flat not in the list yet.  Add it.

	Img *result = Flat2Img (fname);
	flats[f_str] = result;

	// note that a NULL return from Flat2Img is OK, it means that no
	// such flat exists.  Our renderer will revert to using a solid
	// colour.

	return result;
}


Img *ImageCache::GetTex (const char * tname)
{
	if (tname[0] == 0 || tname[0] == '-')
		return 0;

	std::string t_str = tname;

	tex_map_t::iterator P = textures.find (t_str);

	if (P != textures.end ())
		return P->second;

	// texture not in the list yet.  Add it.

	Img *result = Tex2Img (tname);
	textures[t_str] = result;

	// note that a NULL return from Tex2Img is OK, it means that no
	// such texture exists.  Our renderer will revert to using a solid
	// colour.

	return result;
}


Img *ImageCache::GetSprite (const wad_ttype_t& type)
{
	sprite_map_t::iterator P = sprites.find (type);

	if (P != sprites.end ())
		return P->second;

	// sprite not in the list yet.  Add it.

	Img *result = 0;

	const char *sprite_root = get_thing_sprite (type);
	if (sprite_root)
	{
		Lump_loc loc;
		Sprite_loc_by_root (sprite_root, loc);
		result = new Img ();

		if (LoadPicture (*result, sprite_root, loc, 0, 0) != 0)
		{
			delete result;
			result = 0;
		}
	}

	// note that a NULL image is OK.  Our renderer will just ignore the
	// missing sprite.

	sprites[type] = result;
	return result;
}


/*
   Function to get the size of a wall texture
   */
void GetWallTextureSize (s16_t *width, s16_t *height, const char *texname)
{
	MDirPtr  dir = 0;     // Pointer in main directory to texname
	s32_t    *offsets;      // Array of offsets to texture names
	int      n;       // General counter
	s32_t      numtex;      // Number of texture names in TEXTURE*
	s32_t      texofs;      // Offset in wad for the texture data
	char     tname[WAD_TEX_NAME + 1]; // Texture name

	// Offset for texture we want
	texofs = 0;
	// Search for texname in TEXTURE1 (or TEXTURES)
	if (yg_texture_lumps == YGTL_TEXTURES && yg_texture_format == YGTF_NAMELESS)
	{
		dir = FindMasterDir (MasterDir, "TEXTURES");
		if (dir != NULL)
		{
			dir->wadfile->seek (dir->dir.start);
			dir->wadfile->read_i32 (&numtex);
			if (WAD_TEX_NAME < 7) nf_bug ("WAD_TEX_NAME too small");  // Sanity
			if (! y_strnicmp (texname, "TEX", 3)
					&& isdigit (texname[3])
					&& isdigit (texname[4])
					&& isdigit (texname[5])
					&& isdigit (texname[6])
					&& texname[7] == '\0')
			{
				long num;
				if (sscanf (texname + 3, "%4ld", &num) == 1
						&& num >= 0 && num < numtex)
				{
					dir->wadfile->seek (dir->dir.start + 4 + 4 * num);
					dir->wadfile->read_i32 (&texofs);
				}
			}
		}
	}
	else if (yg_texture_format == YGTF_NORMAL
			|| yg_texture_format == YGTF_STRIFE11)
	{
		if (yg_texture_lumps == YGTL_TEXTURES)
			dir = FindMasterDir (MasterDir, "TEXTURES");  // Doom alpha 0.5
		else if (yg_texture_lumps == YGTL_NORMAL)
			dir = FindMasterDir (MasterDir, "TEXTURE1");
		else
		{
			dir = 0;
			nf_bug ("Invalid texture_format/texture_lumps combination.");
		}
		if (dir != NULL)
		{
			dir->wadfile->seek (dir->dir.start);
			dir->wadfile->read_i32 (&numtex);
			// Read in the offsets for texture1 names and info
			offsets = (s32_t *) GetMemory ((long) numtex * 4);
			dir->wadfile->read_i32 (offsets, numtex);
			for (n = 0; n < numtex && !texofs; n++)
			{
				dir->wadfile->seek (dir->dir.start + offsets[n]);
				dir->wadfile->read_bytes (&tname, WAD_TEX_NAME);
				if (!y_strnicmp (tname, texname, WAD_TEX_NAME))
					texofs = dir->dir.start + offsets[n];
			}
			FreeMemory (offsets);
		}
		if (texofs == 0 && yg_texture_lumps == YGTL_NORMAL)
		{
			// Search for texname in TEXTURE2
			dir = FindMasterDir (MasterDir, "TEXTURE2");
			if (dir != NULL)  // Doom II has no TEXTURE2
			{
				dir->wadfile->seek (dir->dir.start);
				dir->wadfile->read_i32 (&numtex);
				// Read in the offsets for texture2 names
				offsets = (s32_t *) GetMemory ((long) numtex * 4);
				dir->wadfile->read_i32 (offsets);
				for (n = 0; n < numtex && !texofs; n++)
				{
					dir->wadfile->seek (dir->dir.start + offsets[n]);
					dir->wadfile->read_bytes (&tname, WAD_TEX_NAME);
					if (!y_strnicmp (tname, texname, WAD_TEX_NAME))
						texofs = dir->dir.start + offsets[n];
				}
				FreeMemory (offsets);
			}
		}
	}
	else
		nf_bug ("Invalid texture_format/texture_lumps combination.");

	if (texofs != 0)
	{
		// Read the info for this texture
		if (yg_texture_format == YGTF_NAMELESS)
			dir->wadfile->seek (texofs + 4L);
		else
			dir->wadfile->seek (texofs + 12L);
		dir->wadfile->read_i16 (width);
		dir->wadfile->read_i16 (height);
	}
	else
	{
		// Texture data not found
		*width  = -1;
		*height = -1;
	}
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
