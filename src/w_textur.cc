//----------------------------------------------------------------------------
//  Texture Conversion and Caching code
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
// This module converts image lumps on disk to usable structures, and also
// provides a caching system for these.
//
// -ES- 2000/02/12 Written.

#include "i_defs.h"
#include "w_textur.h"

#include "e_search.h"
#include "dm_state.h"
#include "dm_defs.h"
#include "e_main.h"
#include "m_argv.h"
#include "m_misc.h"
#include "m_swap.h"
#include "p_local.h"
#include "r_local.h"
#include "r_sky.h"
#include "rgl_defs.h"
#include "v_colour.h"
#include "w_wad.h"
#include "z_zone.h"

#include "epi/epiutil.h"

typedef struct texture_set_s
{
	texturedef_t ** textures;
	int numtextures;
}
texture_set_t;

static texture_set_t *tex_sets = NULL;
static int num_tex_sets = 0;


//
// InstallTextureLumps
//
// -ACB- 1998/09/09 Removed the Doom II SkyName change: unnecessary and not DDF.
//                  Reformatted and cleaned up.
//
static void InstallTextureLumps(int file, int pnames, int palette,
		int texture1, int texture2)
{
	const maptexture_t *mtexture;
	const mappatch_t *mpatch;

	texturedef_t *texture;
	texpatch_t *patch;
	texture_set_t *cur_set;

	int i, j;
	int nummappatches;
	int offset;
	int maxoff;
	int maxoff2;
	int numtextures1;
	int numtextures2;
	int patchcount;

	const int *maptex;
	const int *maptex1;
	const int *maptex2;
	epi::s32array_c patchlookup;
	const int *directory;

	lumpname_c name;
	const char *names;
	const char *name_p;

	int base_size;
	int width;
	byte *base;

	// Load the patch names from pnames.lmp.
	names = (const char*)W_CacheLumpNum(pnames);
	nummappatches = LONG(*((const int *)names));
	name_p = names + 4;

	patchlookup.Size(nummappatches);

	for (i = 0; i < nummappatches; i++)
	{
		name.Set((const char*)(name_p + i * 8));
		j = W_CheckNumForTexPatch(name);
		patchlookup.Insert(j);
	}

	W_DoneWithLump(names);

	//
	// Load the map texture definitions from textures.lmp.
	//
	// The data is contained in one or two lumps:
	//   TEXTURE1 for shareware
	//   TEXTURE2 for commercial.
	//
	maptex = maptex1 = (const int*)W_CacheLumpNum(texture1);
	numtextures1 = LONG(*maptex);
	maxoff = W_LumpLength(texture1);
	directory = maptex + 1;

	if (texture2 != -1)
	{
		maptex2 = (const int*)W_CacheLumpNum(texture2);
		numtextures2 = LONG(*maptex2);
		maxoff2 = W_LumpLength(texture2);
	}
	else
	{
		maptex2 = NULL;
		numtextures2 = 0;
		maxoff2 = 0;
	}

	Z_Resize(tex_sets, texture_set_t, num_tex_sets + 1);
	cur_set = tex_sets + num_tex_sets;
	num_tex_sets++;

	cur_set->numtextures = numtextures1 + numtextures2;
	cur_set->textures = Z_New(texturedef_t *, cur_set->numtextures);

	for (i = 0; i < cur_set->numtextures; i++, directory++)
	{
		if (i == numtextures1)
		{
			// Start looking in second texture file.
			maptex = maptex2;
			maxoff = maxoff2;
			directory = maptex + 1;
		}

		offset = LONG(*directory);
		if (offset > maxoff)
			I_Error("W_InitTextures: bad texture directory");

		mtexture = (const maptexture_t *) ((const byte *) maptex + offset);

		// -ES- 2000/02/10 Texture must have patches.
		patchcount = SHORT(mtexture->patchcount);
		if (!patchcount)
			I_Error("W_InitTextures: Texture '%.8s' has no patches", mtexture->name);

		width = SHORT(mtexture->width);
		if (width == 0)
			I_Error("W_InitTextures: Texture '%.8s' has zero width", mtexture->name);

		// -ES- Allocate texture, patches and columnlump/ofs in one big chunk
		base_size = sizeof(texturedef_t) + sizeof(texpatch_t) * (patchcount - 1);
		texture = cur_set->textures[i] = (texturedef_t *) Z_Malloc(base_size + width * (sizeof(byte) + sizeof(short)));
		base = (byte *)texture + base_size;

		texture->columnofs = (unsigned short *)base;

		texture->width = width;
		texture->height = SHORT(mtexture->height);
		texture->file = file;
		texture->palette_lump = palette;
		texture->patchcount = patchcount;

		Z_StrNCpy(texture->name, mtexture->name, 8);
		strupr(texture->name);

		mpatch = &mtexture->patches[0];
		patch = &texture->patches[0];

		for (j = 0; j < texture->patchcount; j++, mpatch++, patch++)
		{
			patch->originx = SHORT(mpatch->originx);
			patch->originy = SHORT(mpatch->originy);
			patch->patch = patchlookup[SHORT(mpatch->patch)];

			if (patch->patch == -1)
			{
				I_Warning("Missing patch in texture \'%.8s\'\n", texture->name);

				// mark texture as a dud
				texture->patchcount = 0;
				break;
			}
		}
	}

	// free stuff
	W_DoneWithLump(maptex1);

	if (maptex2)
		W_DoneWithLump(maptex2);
}

//
// W_InitTextures
//
// Initialises the texture list with the textures from the world map.
//
// -ACB- 1998/09/09 Fixed the Display routine from display rubbish.
//
bool W_InitTextures(void)
{
	int max_file = W_GetNumFiles();
	int j, t, file;

	texturedef_t ** textures = NULL;
	texturedef_t ** cur;
	int numtextures = 0;

	I_Printf("W_InitTextures...\n");

	DEV_ASSERT2(num_tex_sets == 0);

	// iterate over each file, creating our sets of textures
	// -ACB- 1998/09/09 Removed the Doom II SkyName change: unnecessary and not DDF.

	for (file=0; file < max_file; file++)
	{
		E_LocalProgress(file, max_file, NULL);

		wadtex_resource_c WT;

		W_GetTextureLumps(file, &WT);

		if (WT.pnames < 0)
			continue;

		if (WT.texture1 >= 0)
			InstallTextureLumps(file, WT.pnames, WT.palette, WT.texture1, WT.texture2);
		else if (WT.texture2 >= 0)
			InstallTextureLumps(file, WT.pnames, WT.palette, WT.texture2, WT.texture1);
	}

	if (num_tex_sets == 0)
		I_Error("No textures found !  Make sure the chosen IWAD is valid.\n");

	// now clump all of the texturedefs together and sort 'em, primarily
	// by increasing name, secondarily by increasing file number
	// (measure of newness).  We ignore "dud" textures (missing
	// patches).

	for (j=0; j < num_tex_sets; j++)
		numtextures += tex_sets[j].numtextures;

	textures = cur = new texturedef_t*[numtextures];

	for (j=0; j < num_tex_sets; j++)
	{
		for (t=0; t < tex_sets[j].numtextures; t++)
			if (tex_sets[j].textures[t]->patchcount > 0)
				*cur++ = tex_sets[j].textures[t];
	}

	numtextures = cur - textures;

#define CMP(a, b)  \
	(strcmp(a->name, b->name) < 0 || \
	 (strcmp(a->name, b->name) == 0 && a->file < b->file))
		QSORT(texturedef_t *, textures, numtextures, CUTOFF);
#undef CMP

	// remove duplicate names.  Because the QSORT took newness into
	// account, only the last entry in a run of identically named
	// textures needs to be kept.

	for (j=1; j < numtextures; j++)
	{
		texturedef_t * a = textures[j - 1];
		texturedef_t * b = textures[j];

		if (strcmp(a->name, b->name) == 0)
		{
			textures[j - 1] = NULL;
		}
	}

#if 0  // DEBUGGING
	for (j=0; j < numtextures; j++)
	{
		if (textures[j] == NULL)
		{
			L_WriteDebug("TEXTURE #%d was a dupicate\n", j);
			continue;
		}
		L_WriteDebug("TEXTURE #%d:  name=[%s]  file=%d  size=%dx%d\n", j,
				textures[j]->name, textures[j]->file,
				textures[j]->width, textures[j]->height);
	}
#endif

	W_ImageCreateTextures(textures, numtextures); 

	// free pointer array.  We need to keep the definitions in memory
	// for (a) the image system and (b) texture anims.
	delete [] textures;

	return true;
}

//
// W_FindTextureSequence
//
// Returns the set number containing the texture names (with the
// offset values updated to the indexes), or -1 if none could be
// found.  Used by animation code.
//
// Note: search is from latest set to earliest set.
// 
int W_FindTextureSequence(const char *start, const char *end,
		int *s_offset, int *e_offset)
{
	int i, j;

	for (i=num_tex_sets-1; i >= 0; i--)
	{
		// look for start name
		for (j=0; j < tex_sets[i].numtextures; j++)
			if (stricmp(start, tex_sets[i].textures[j]->name) == 0)
				break;

		if (j >= tex_sets[i].numtextures)
			continue;

		(*s_offset) = j;

		// look for end name
		for (j++; j < tex_sets[i].numtextures; j++)
		{
			if (stricmp(end, tex_sets[i].textures[j]->name) == 0)
			{
				(*e_offset) = j;
				return i;
			}
		}
	}

	// not found
	return -1;
}

//
// W_TextureNameInSet
//
const char *W_TextureNameInSet(int set, int offset)
{
	DEV_ASSERT2(0 <= set && set < num_tex_sets);
	DEV_ASSERT2(0 <= offset && offset < tex_sets[set].numtextures);

	return tex_sets[set].textures[offset]->name;
}

