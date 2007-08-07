//----------------------------------------------------------------------------
//  EDGE Rendering things (objects as sprites) Code
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
// -KM- 1998/07/26 Replaced #ifdef RANGECHECK with #ifdef DEVELOPERS
// -KM- 1998/09/27 Dynamic colourmaps
// -AJA- 1999/07/12: Now uses colmap.ddf.
//

#include "i_defs.h"
#include "r_things.h"

#include "e_main.h"
#include "e_search.h"
#include "r_image.h"
#include "w_wad.h"
#include "z_zone.h"

#include <string.h>

// The minimum distance between player and a visible sprite.
// FIXME: Decrease value, lower values are valid when float is used.
#define MINZ        (4.0f)

//
// Sprite rotation 0 is facing the viewer,
//  rotation 1 is one angle turn CLOCKWISE around the axis.
// This is not the same as the angle,
//  which increases counter clockwise (protractor).
// There was a lot of stuff grabbed wrong, so I changed it...
//

// Sprite definitions
spritedef_array_c sprites;
int numsprites = 0;

// Sorted map of sprite defs.  Only used during initialisation.
static spritedef_c ** sprite_map = NULL;
static int sprite_map_len;

//
// spritedef_c constructor
//
spritedef_c::spritedef_c(const char* _name)  : numframes(0), frames(NULL), weapon_frames(0)
{
    strcpy(name, _name);
}

//
// INITIALISATION FUNCTIONS
//

typedef struct add_spr_cache_s
{
	char name[6];
	int index;
}
add_spr_cache_t;

static int add_spr_cache_num = 0;
static add_spr_cache_t add_spr_cache[2] = 
{{ "", 0 }, { "", 0 }};

//
// R_AddSpriteName
// 
// Looks up the sprite name, returning the index if found.  When not
// found, a new sprite is created, and that index is returned.  The
// given sprite name should be upper case.
// 
int R_AddSpriteName(const char *name, int frame, bool is_weapon)
{
	if (strcmp(name, "NULL") == 0)
	{
		if (numsprites == 0)  // fill out dummy entry, #0
		{
			spritedef_c *def = new spritedef_c(name);

			sprites.Insert(def);
			numsprites = sprites.GetSize();
		}

		return SPR_NULL;
	}

	int index;

	SYS_ASSERT(strlen(name) == 4);

	// look in cache
	if (strcmp(name, add_spr_cache[0].name) == 0)
		index = add_spr_cache[0].index;
	else if (strcmp(name, add_spr_cache[1].name) == 0)
		index = add_spr_cache[1].index;
	else
	{
		// look backwards, assuming a recent sprite is more likely
		for (index=numsprites-1; index >= 1; index--)
		{
			if (strcmp(sprites[index]->name, name) == 0)
				break;
		}

		if (index == 0)
		{
			index = numsprites;

			spritedef_c *def = new spritedef_c(name);

			sprites.Insert(def);
			numsprites = sprites.GetSize();
		}

		// update cache
		strcpy(add_spr_cache[add_spr_cache_num].name, name);
		add_spr_cache[add_spr_cache_num].index = index;

		add_spr_cache_num ^= 1;
	}

	// update maximal frame count
	// NOTE: frames are allocated during R_InitSprites

	if (frame+1 > sprites[index]->numframes)
		sprites[index]->numframes = frame+1;
	
	if (is_weapon)
		sprites[index]->MarkWeapon(frame);

	return index;
}

//
// SPRITE LOADING FUNCTIONS
//

static spriteframe_c *WhatFrame(spritedef_c *def, const char *name, int pos)
{
	char frame_ch = name[pos];

	int index;

	if ('A' <= frame_ch && frame_ch <= 'Z')
	{
		index = (frame_ch - 'A');
	}
	else switch (frame_ch)
	{
		case '[':  index = 26; break;
		case '\\': index = 27; break;
		case ']':  index = 28; break;
		case '^':  index = 29; break;
		case '_':  index = 30; break;

		default:
		   I_Warning("Sprite lump %s has illegal frame.\n", name);
		   return NULL;
	}

	SYS_ASSERT(index >= 0);

	// ignore frames larger than what is used in DDF
	if (index >= def->numframes)
		return NULL;

	return & def->frames[index];
}

static void SetExtendedRots(spriteframe_c *frame)
{
	if (frame->rots == 16)
		return;

	frame->rots = 16;

	for (int i = 1; i <= 7; i++)
	{
		frame->flip[2*i]   = frame->flip[i];
		frame->images[2*i] = frame->images[i];

		frame->flip[i]   = 0;
		frame->images[i] = NULL;
	}
}

static int WhatRot(spriteframe_c *frame, const char *name, int pos )
{
	char rot_ch = name[pos];

	int rot;

	// NOTE: rotations 9 and A-G are EDGE specific.

	if ('0' <= rot_ch && rot_ch <= '9')
		rot = (rot_ch - '0');
	else if ('A' <= rot_ch && rot_ch <= 'G')
		rot = (rot_ch - 'A') + 10;
	else
	{
		I_Warning("Sprite lump %s has illegal rotation.\n", name);
		return -1;
	}

	if (frame->rots == 0)
		frame->rots = 1;

	if (rot >= 1)
		frame->rots = 8;

	if (rot >= 9)
		SetExtendedRots(frame);

	switch (frame->rots)
	{
		case 1:
			return 0;

		case 8:
			return rot-1;

		case 16:
			if (rot >= 9)
				return 1 + (rot-9) * 2;
			else
				return 0 + (rot-1) * 2;

		default:
			I_Error("INTERNAL ERROR: frame->rots = %d\n", frame->rots);
			return -1; /* NOT REACHED */
	}
}

static void InstallSpriteLump(spritedef_c *def, int lump,
    const char *lumpname, int pos, byte flip)
{
	spriteframe_c *frame = WhatFrame(def, lumpname, pos);
	if (! frame)
		return;

	// don't disturb any frames already loaded
	if (frame->finished)
		return;

	int rot = WhatRot(frame, lumpname, pos+1);
	if (rot < 0)
		return;

	SYS_ASSERT(0 <= rot && rot < 16);

	if (frame->images[rot])
	{
		I_Warning("Sprite %s has two lumps mapped to it (frame %c).\n", 
				lumpname, lumpname[pos]);
		return;
	}

	frame->images[rot] = W_ImageCreateSprite(lumpname, lump,
		def->IsWeapon(frame - def->frames));

	frame->flip[rot] = flip;
}

static void InstallSpriteImage(spritedef_c *def, const image_c *img,
    const char *img_name, int pos, byte flip)
{
	spriteframe_c *frame = WhatFrame(def, img_name, pos);
	if (! frame)
		return;

	// don't disturb any frames already loaded
	if (frame->finished)
		return;

	int rot = WhatRot(frame, img_name, pos+1);
	if (rot < 0)
		return;

	if (frame->images[rot])
	{
		I_Warning("Sprite %s has two images mapped to it (frame %c)\n", 
				img_name, img_name[pos]);
		return;
	}

	frame->images[rot] = img;
	frame->flip[rot] = flip;
}

//
// FillSpriteFrames
//
static void FillSpriteFrames(int file, int prog_base, int prog_total)
{
	epi::u32array_c& lumps = W_GetListLumps(file, LMPLST_Sprites);
	int lumpnum = lumps.GetSize();

	if (lumpnum == 0)
		return;

	// check all lumps for prefixes matching the ones in the sprite
	// list.  Both lists have already been sorted to make this as fast
	// as possible.

	int S = 0, L = 0;

	while (S < sprite_map_len && L < lumpnum)
	{
		const char *sprname  = sprite_map[S]->name;
		const char *lumpname = W_GetLumpName(lumps[L]);

		if (strlen(lumpname) != 6 && strlen(lumpname) != 8)
		{
			I_Warning("Sprite name %s has illegal length.\n", lumpname);
			L++; continue;
		}

		int comp = strncmp(sprname, lumpname, 4);

		if (comp < 0)  // S < L
		{
			S++; continue;
		}
		else if (comp > 0)  // S > L
		{
			L++; continue;
		}

		// we have a match
		InstallSpriteLump(sprite_map[S], lumps[L], lumpname, 4, 0);

		if (lumpname[6])
			InstallSpriteLump(sprite_map[S], lumps[L], lumpname, 6, 1);

		L++;

		// update startup progress bar
		E_LocalProgress(prog_base + L * 100 / lumpnum, prog_total);
	}
}

//
// FillSpriteFramesUser
//
// Like the above, but made especially for IMAGES.DDF.
//
static void FillSpriteFramesUser(int prog_base, int prog_total)
{
	int img_num;
	const image_c ** images = W_ImageGetUserSprites(&img_num);

	if (img_num == 0)
		return;

	SYS_ASSERT(images);

	int S = 0, L = 0;

	while (S < sprite_map_len && L < img_num)
	{
		const char *sprname  = sprite_map[S]->name;
		const char *img_name = W_ImageGetName(images[L]);

		if (strlen(img_name) != 6 && strlen(img_name) != 8)
		{
			I_Warning("Sprite name %s (IMAGES.DDF) has illegal length.\n", img_name);
			L++; continue;
		}

		int comp = strncmp(sprname, img_name, 4);

		if (comp < 0)  // S < L
		{
			S++; continue;
		}
		else if (comp > 0)  // S > L
		{
			L++; continue;
		}

		// we have a match
		InstallSpriteImage(sprite_map[S], images[L], img_name, 4, 0);

		if (img_name[6])
			InstallSpriteImage(sprite_map[S], images[L], img_name, 6, 1);

		L++;

		// update startup progress bar
		E_LocalProgress(prog_base + L * 100 / img_num, prog_total);
	}

	delete[] images;
}

static void MarkCompletedFrames(void)
{
	int src, dst;

	for (src = dst = 0; src < sprite_map_len; src++)
	{
		spritedef_c *def = sprite_map[src];

		int finish_num = 0;

		for (int f = 0; f < def->numframes; f++)
		{
			char frame_ch = 'A' + f;
			spriteframe_c *frame = def->frames + f;

			if (frame->finished)
			{
				finish_num++;
				continue;
			}

			int rot_count = 0;

			// check if all image pointers are NULL
			for (int i=0; i < frame->rots; i++)
				rot_count += frame->images[i] ? 1 : 0;

			if (rot_count == 0)
				continue;

			frame->finished = 1;
			finish_num++;

			if (rot_count < frame->rots)
				I_Warning("Sprite %s:%c is missing rotations (%d of %d).\n",
					def->name, frame_ch, frame->rots - rot_count, frame->rots);

///##		if (frame->rots == 0)
///##		{
///##			if (rot_count != 1)
///##				I_Warning("Sprite %s:%c has extra rotations.\n", def->name, frame_ch);
///##
///##			SYS_ASSERT(frame->images[0] != NULL);
///##			continue;
///##		}

///##		// Note: two passes are needed
///##		for (int j=0; j < (16 * 2); j++)
///##		{
///##			if (frame->images[j % 16] && !frame->images[(j+1) % 16])
///##			{
///##				frame->images[(j+1) % 16] = frame->images[j % 16];
///##				frame->flip  [(j+1) % 16] = frame->flip  [j % 16];
///##			}
///##		}
		}

		// remove complete sprites from sprite_map
		if (finish_num == def->numframes)
			continue;

		sprite_map[dst++] = def;
	}

	sprite_map_len = dst;
}

// show warnings for missing patches
static void CheckSpriteFrames(spritedef_c *def)
{
	int missing = 0;

	for (int i = 0; i < def->numframes; i++)
		if (! def->frames[i].finished)
			missing++;

	if (missing > 0 && missing < def->numframes)
		I_Warning("Missing %d frames in sprite: %s\n", missing, def->name);

	// free some memory for completely missing sprites
	if (missing == def->numframes)
	{
		delete[] def->frames;

		def->numframes = 0;
		def->frames = NULL;
	}
}

//
// R_InitSprites
//
// Use the sprite lists in the WAD (S_START..S_END) to flesh out the
// known sprite definitions (global `sprites' array, created while
// parsing DDF) with images.
//
// Checking for missing frames still done at run time.
// 
// -AJA- 2001/02/01: rewrote this stuff.
// 
void R_InitSprites(void)
{
	if (numsprites <= 1)
		I_Error("Missing sprite definitions !!\n");

	I_Printf("R_InitSprites: Finding sprite patches\n");

	// allocate frames (ignore NULL sprite, #0)
	int i;
	for (i=1; i < numsprites; i++)
	{
		spritedef_c *def = sprites[i];

		SYS_ASSERT(strlen(def->name) == 4);
		SYS_ASSERT(def->numframes > 0);

		def->frames = new spriteframe_c[def->numframes];

		// names in the sprite list should be unique
		if (i > 0)
		{
			SYS_ASSERT(strncmp(sprites[i-1]->name, def->name, 4) != 0);
		}
	}

	// create a sorted list (ignore NULL entry, #0)
	sprite_map_len = numsprites - 1;
	sprite_map = Z_New(spritedef_c *, sprite_map_len);

	for (i=0; i < sprite_map_len; i++)
		sprite_map[i] = sprites[i + 1];

#define CMP(a, b)  (strcmp(a->name, b->name) < 0)
	QSORT(spritedef_c *, sprite_map, sprite_map_len, CUTOFF);
#undef CMP

	// iterate over each file.  Order is important, we must go from
	// newest wad to oldest, so that new sprites override the old ones.
	// Completely finished sprites get removed from the list as we go.  
	//
	// NOTE WELL: override granularity is single frames.

	int numfiles = W_GetNumFiles();

	FillSpriteFramesUser(0, (numfiles+1) * 100);

	for (int file = numfiles - 1; file >= 0; file--)
	{
		FillSpriteFrames(file, (numfiles - file) * 100, (numfiles+1) * 100);
		MarkCompletedFrames();
	}

	// perform checks and free stuff
	for (i=1; i < numsprites; i++)
		CheckSpriteFrames(sprites[i]);

	Z_Free(sprite_map);
	sprite_map = NULL;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
