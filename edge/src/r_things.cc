//----------------------------------------------------------------------------
//  EDGE Rendering things (objects as sprites) Code
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

#include "e_search.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "m_argv.h"
#include "m_inline.h"
#include "m_swap.h"
#include "p_local.h"
#include "r_local.h"
#include "v_colour.h"
#include "w_wad.h"
#include "z_zone.h"

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

// Sprite defs.  allocated in blocks of 16
spritedef_t *sprites = NULL;
int numsprites = 0;

// Sorted map of sprite defs.  Only used during initialisation.
static spritedef_t ** sprite_map = NULL;
static int sprite_map_len;


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
int R_AddSpriteName(const char *name, int frame)
{
	if (strcmp(name, "NULL") == 0)
	{
		if (numsprites == 0)  // fill out dummy entry, #0
		{
			Z_Resize(sprites, spritedef_t, numsprites + 16);
			numsprites++;

			Z_Clear(sprites + 0, spritedef_t, 1);
			strcpy(sprites[0].name, name);
		}

		return SPR_NULL;
	}

	int index;

	DEV_ASSERT2(strlen(name) == 4);
	frame += 1;

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
			if (strcmp(sprites[index].name, name) == 0)
				break;
		}

		if (index == 0)
		{
			index = numsprites;

			if ((numsprites % 16) == 0)
				Z_Resize(sprites, spritedef_t, numsprites + 16);

			numsprites++;

			Z_Clear(sprites + index, spritedef_t, 1);
			strcpy(sprites[index].name, name);
		}

		// update cache
		strcpy(add_spr_cache[add_spr_cache_num].name, name);
		add_spr_cache[add_spr_cache_num].index = index;

		add_spr_cache_num ^= 1;
	}

	// update maximal frame count
	// NOTE: frames are allocated during R_InitSprites
	if (frame > sprites[index].numframes)
		sprites[index].numframes = frame;

	return index;
}

//
// SPRITE LOADING FUNCTIONS
//

static void InstallSpriteLump(spritedef_t *def, int lump,
    const char *lumpname, int pos, byte flip)
{
	int frame, rot;

	char frame_ch = lumpname[pos];
	char rot_ch   = lumpname[pos+1];

	// convert frame & rotation characters
	// NOTE: rotations 9 and A-G are EDGE specific.

	if ('A' <= frame_ch && frame_ch <= 'Z')
		frame = (frame_ch - 'A');
	else switch (frame_ch)
	{
		case '[':  frame = 26; break;
		case '\\': frame = 27; break;
		case ']':  frame = 28; break;
		case '^':  frame = 29; break;
		case '_':  frame = 30; break;

		default:
				   I_Warning("Sprite lump %s has illegal frame.\n", lumpname);
				   return;
	}

	// ignore frames larger than what is used in DDF
	if (frame >= def->numframes)
		return;

	if ('0' == rot_ch)
		rot = 0;
	else if ('1' <= rot_ch && rot_ch <= '8')
		rot = (rot_ch - '1') * 2;
	else if ('9' == rot_ch)
		rot = 1;
	else if ('A' <= rot_ch && rot_ch <= 'G')
		rot = (rot_ch - 'A') * 2 + 3;
	else
	{
		I_Warning("Sprite lump %s has illegal rotation.\n", lumpname);
		return;
	}

	DEV_ASSERT2(0 <= frame && frame < def->numframes);

	// don't disturb any already-loaded frames
	if (def->frames[frame].finished)
		return;

	def->frames[frame].rotated = (rot_ch == '0') ? 0 : 1;

	if (rot & 1)
		def->frames[frame].extended = 1;

	DEV_ASSERT2(0 <= rot && rot < 16);

	if (def->frames[frame].images[rot])
	{
		I_Warning("Sprite %s:%c has two lumps mapped to it.\n", 
				lumpname, frame_ch);
		return;
	}

	def->frames[frame].images[rot] = W_ImageCreateSprite(lump);
	def->frames[frame].flip[rot] = flip;
}

//
// FillSpriteFrames
//
static void FillSpriteFrames(int file)
{
	const int *lumps;
	int lumpnum;

	int S, L;

	lumps = W_GetListLumps(file, LMPLST_Sprites, &lumpnum);

	if (lumpnum == 0)
		return;

	// check all lumps for prefixes matching the ones in the sprite
	// list.  Both lists have already been sorted to make this as fast
	// as possible.

	S = L = 0; 

	while (S < sprite_map_len && L < lumpnum)
	{
		const char *sprname  = sprite_map[S]->name;
		const char *lumpname = W_GetLumpName(lumps[L]);

		int comp;

		if (strlen(lumpname) != 6 && strlen(lumpname) != 8)
		{
			I_Warning("Sprite name %s has illegal length.\n", lumpname);
			L++; continue;
		}

		comp = strncmp(sprname, lumpname, 4);

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
	}
}

static void MarkCompletedFrames(void)
{
	int src, dst;

	for (src = dst = 0; src < sprite_map_len; src++)
	{
		spritedef_t *def = sprite_map[src];

		int f, i;
		int count;
		int finish_num = 0;

		for (f=0; f < def->numframes; f++)
		{
			char frame_ch = 'A' + f;
			spriteframe_t *frame = def->frames + f;

			if (frame->finished)
			{
				finish_num++;
				continue;
			}

			// check if all image pointers are NULL
			for (i=count=0; i < 16; i++)
				count += frame->images[i] ? 1 : 0;

			if (count == 0)
				continue;

			frame->finished = 1;
			finish_num++;

			// fill in any gaps.  This is especially needed because of the
			// extra 8 rotations, but it also handles missing sprites well.

			if (! frame->rotated)
			{
				if (count != 1)
					I_Warning("Sprite %s:%c has extra rotations.\n", def->name,
							frame_ch);

				DEV_ASSERT2(frame->images[0] != NULL);

				for (i=1; i < 16; i++)
				{
					frame->images[i] = frame->images[0];
					frame->flip[i]   = frame->flip[0];
				}

				continue;
			}

			if (count != 8 && count != 16)
				I_Warning("Sprite %s:%c is missing rotations.\n", def->name, 
						frame_ch);

			// Note two passes are needed
			for (i=0; i < (16 * 2); i++)
			{
				if (frame->images[i % 16] && !frame->images[(i+1) % 16])
				{
					frame->images[(i+1) % 16] = frame->images[i % 16];
					frame->flip  [(i+1) % 16] = frame->flip  [i % 16];
				}
			}
		}

		// remove complete sprites from sprite_map
		if (finish_num == def->numframes)
			continue;

		sprite_map[dst++] = def;
	}

	sprite_map_len = dst;
}

// show warnings for missing patches
static void CheckSpriteFrames(spritedef_t *def)
{
	int i;
	int missing;

	for (i = missing = 0; i < def->numframes; i++)
	{
		if (! def->frames[i].finished)
			missing++;
	}

	if (missing > 0 && missing < def->numframes)
		I_Warning("Missing %d frames in sprite: %s\n", missing, def->name);

	// free some memory for completely missing sprites
	if (missing == def->numframes)
	{
		Z_Free(def->frames);

		def->numframes = 0;
		def->frames = NULL;
	}
}

//
// R_InitSprites
//
// Use the sprite lists in the WAD (S_START..S_END) to flesh out the
// known sprite definitions (global `sprites' array) with images.
//
// Checking for missing frames still done at run time.
// 
// -AJA- 2001/02/01: rewrote this stuff.
// 
bool R_InitSprites(void)
{
	int i, file;

	if (numsprites <= 1)
		I_Error("Missing sprite definitions !!\n");

	I_Printf("R_InitSprites: Finding sprite patches\n");

	// allocate frames (ignore NULL sprite, #0)
	for (i=1; i < numsprites; i++)
	{
		DEV_ASSERT2(strlen(sprites[i].name) == 4);
		DEV_ASSERT2(sprites[i].numframes > 0);

		sprites[i].frames = Z_ClearNew(spriteframe_t, sprites[i].numframes);

		// names in the sprite list should be unique
		if (i > 0)
		{
			DEV_ASSERT2(strncmp(sprites[i-1].name, sprites[i].name, 4) != 0);
		}
	}

	// create a sorted list (ignore NULL entry, #0)
	sprite_map_len = numsprites - 1;
	sprite_map = Z_New(spritedef_t *, sprite_map_len);

	for (i=0; i < sprite_map_len; i++)
		sprite_map[i] = sprites + 1 + i;

#define CMP(a, b)  (strcmp(a->name, b->name) < 0)
	QSORT(spritedef_t *, sprite_map, sprite_map_len, CUTOFF);
#undef CMP

	// iterate over each file.  Order is important, we must go from
	// newest wad to oldest, so that new sprites override the old ones.
	// Completely finished sprites get removed from the sorted list as
	// we go.  
	//
	// NOTE WELL: override granularity is single frames.

	for (file=W_GetNumFiles() - 1; file >= 0; file--)
	{
		FillSpriteFrames(file);
		MarkCompletedFrames();
	}

	// perform checks and free stuff
	for (i=1; i < numsprites; i++)
		CheckSpriteFrames(sprites + i);

	Z_Free(sprite_map);
	sprite_map = NULL;

	return true;
}

