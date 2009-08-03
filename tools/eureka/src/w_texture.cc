//------------------------------------------------------------------------
//  TEXTURE LOADING
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

#include <map>
#include <algorithm>
#include <string>

#include "m_dialog.h"
#include "m_game.h"      /* yg_picture_format */
#include "r_misc.h"
#include "levels.h"
#include "w_patches.h"
#include "w_loadpic.h"
#include "w_file.h"
#include "w_io.h"
#include "w_structs.h"

#include "w_rawdef.h"
#include "w_texture.h"


typedef std::map<std::string, Img *> tex_map_t;

static tex_map_t textures;


static void DeleteTex(const tex_map_t::value_type& P)
{
	delete P.second;
}

void W_ClearTextures()
{
	std::for_each(textures.begin(), textures.end(), DeleteTex);

	textures.clear();
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
	{
		have_dummy_bytes = true;
		header_size      = 14;
		item_size        = 10;
	}

	/* offset for texture we want. */
	texofs = 0;

	// Other iwads : "TEXTURE1" and "TEXTURE2"
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

	/* texture name not found */
	if (texofs == 0)
		return 0;

	/* read the info for this texture */
	s32_t header_ofs;
	
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

		{
			wad_pic_name_t *wname = patch_dir.name_for_num (pnameind);
			if (wname == 0)
			{
				warn ("texture \"%.*s\": patch %2d has bad index %d.\n",
						WAD_TEX_NAME, tname, (int) n, (int) pnameind);
				continue;
			}

			*picname = '\0';
			strncat (picname, (const char *) *wname, sizeof picname - 1);
		}

		Lump_c *lump = W_FindPatchLump(picname);

		if (! lump ||
		    ! LoadPicture(*texbuf, lump, picname, xofs, yofs, 0, 0))
			warn ("texture \"%.*s\": patch \"%.*s\" not found.\n",
					WAD_TEX_NAME, tname, WAD_PIC_NAME, picname);
	}

	return texbuf;
}


static void LoadTextureLump(Lump_c *lump, byte *pnames, int pname_size)
{
	// skip size word at front of PNAMES
	pnames += 4;

	pname_size /= 8;

	// load TEXTUREx data into memory for easier processing
	byte *tex_data;
	int tex_length = W_LoadLumpData(lump, &tex_data);

	// at the front of the TEXTUREx lump are some 4-byte integers
	s32_t *tex_data_s32 = (s32_t *)tex_data;

	int num_tex = LE_S32(tex_data_s32[0]);

	// FIXME validate num_tex
	
	for (int n = 0; n < num_tex; n++)
	{
		int offset = LE_S32(tex_data_s32[1+n]);

		// FIXME: validate offset

		const raw_texture_t *raw = (const raw_texture_t *)(tex_data + offset);

		// create the new Img
		int width  = LE_U16(raw->width);
		int height = LE_U16(raw->height);
DebugPrintf("Texture [%.8s] : %dx%d\n", raw->name, width, height);

		if (width == 0 || height == 0)
			FatalError("W_InitTextures: Texture '%.8s' has zero size", raw->name);

		Img *img = new Img(width, height, false);

		// apply all the patches
		int num_patches = LE_S16(raw->patch_count);
		if (! num_patches)
			FatalError("W_InitTextures: Texture '%.8s' has no patches", raw->name);

		const raw_patchdef_t *patdef = (const raw_patchdef_t *) & raw->patches[0];

		for (int j = 0; j < num_patches; j++, patdef++)
		{
			int xofs = LE_S16(patdef->x_origin);
			int yofs = LE_S16(patdef->y_origin);
			int pname_idx = LE_U16(patdef->pname);

			// AYM 1998-08-08: Yes, that's weird but that's what Doom
			// does. Without these two lines, the few textures that have
			// patches with negative y-offsets (BIGDOOR7, SKY1, TEKWALL1,
			// TEKWALL5 and a few others) would not look in the texture
			// viewer quite like in Doom. This should be mentioned in
			// the UDS, by the way.
			if (yofs < 0)
				yofs = 0;

			if (pname_idx >= pname_size)
			{
				warn("Invalid pname in texture \'%.8s\'\n", raw->name);
				continue;
			}

			char picname[16];
			memcpy(picname, pnames + 8*pname_idx, 8);
			picname[8] = 0;

DebugPrintf("-- %d patch [%s]\n", j, picname);
			Lump_c *lump = W_FindPatchLump(picname);

			if (! lump ||
				! LoadPicture(*img, lump, picname, xofs, yofs, 0, 0))
				warn ("texture \"%.8s\": patch \"%.8s\" not found.\n",
						raw->name, picname);
		}

		// store the new texture
		char namebuf[16];
		memcpy(namebuf, raw->name, 8);
		namebuf[8] = 0;

		std::string t_str(namebuf);

		// FIXME: free any existing one with same name

		textures[t_str] = img;
	}

	W_FreeLumpData(&tex_data);
}


void W_LoadTextures()
{
	for (int i = 0; i < (int)master_dir.size(); i++)
	{
		LogPrintf("Loading Textures from WAD #%d\n", i+1);

		Lump_c *pnames   = master_dir[i]->FindLump("PNAMES");
		Lump_c *texture1 = master_dir[i]->FindLump("TEXTURE1");
		Lump_c *texture2 = master_dir[i]->FindLump("TEXTURE2");

		// Note that we _require_ the PNAMES lump to exist along
		// with the TEXTURE1/2 lump which uses it.  Probably a
		// few wads exist which lack the PNAMES lump (relying on
		// the one in the IWAD), however this practice is too
		// error-prone (using the wrong IWAD will break it),
		// so I think supporting it is a bad idea.  -- AJA

		if (!pnames)
			continue;
		
		byte *pname_data;
		int pname_size = W_LoadLumpData(pnames, &pname_data);

		if (texture1)
			LoadTextureLump(texture1, pname_data, pname_size);

		if (texture2)
			LoadTextureLump(texture2, pname_data, pname_size);
	}
}


Img * W_GetTexture(const char *name)
{
	if (name[0] == 0 || name[0] == '-')
		return 0;

	std::string t_str = name;

	tex_map_t::iterator P = textures.find(t_str);

	if (P != textures.end())
		return P->second;

	return NULL;
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
	{
		dir = FindMasterDir (MasterDir, "TEXTURE1");
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
		if (texofs == 0)
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

	if (texofs != 0)
	{
		// Read the info for this texture
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
