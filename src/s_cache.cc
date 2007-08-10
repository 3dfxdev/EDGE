//----------------------------------------------------------------------------
//  Sound Caching
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

#include "i_defs.h"

#include <stdlib.h>
#include <string.h>

#include <vector>

#include "epi/sound_data.h"
#include "epi/errors.h"
#include "epi/strings.h"

#include "s_sound.h"
#include "s_cache.h"

#include "errorcodes.h"

#include "ddf_main.h"
#include "ddf_sfx.h"
#include "m_argv.h"
#include "m_random.h"
#include "p_mobj.h"
#include "r_defs.h"
#include "w_wad.h"


static std::vector<epi::sound_data_c *> fx_cache;


static void Load_Silence(epi::sound_data_c *buf)
{
	buf->Allocate(256, epi::SBUF_Mono);

	memset(buf->data_L, 0, 256);
}

static void Load_DOOM(epi::sound_data_c *buf, const byte *lump, int length)
{
	buf->freq = lump[2] + (lump[3] << 8);

	if (buf->freq < 8000 || buf->freq > 44100)
		I_Warning("Sound Load: weird frequency: %d Hz\n", buf->freq);

	if (buf->freq < 4000)
		buf->freq = 4000;

	length -= 8;

	buf->Allocate(length, epi::SBUF_Mono);

	// convert to signed 16-bit format
	const byte *src = lump + 8;
	const byte *s_end = src + length;

	s16_t *dest = buf->data_L;

	for (; src < s_end; src++)
		*dest++ = (*src ^ 0x80) << 8;
}

static void Load_WAV(epi::sound_data_c *buf, const byte *lump, int length)
{
	I_Error("Sound Load: WAV format not supported!\n");
}

static void Load_OGG(epi::sound_data_c *buf, const byte *lump, int length)
{
	I_Error("Sound Load: OGG/Vorbis format not supported!\n");
}


//----------------------------------------------------------------------------

void S_CacheInit(void)
{
	// nothing to do
}

void S_FlushData(epi::sound_data_c *fx)
{
	SYS_ASSERT(fx->ref_count == 0);

	fx->Free();
}

void S_CacheClearAll(void)
{
	for (int i = 0; i < (int)fx_cache.size(); i++)
		delete fx_cache[i];

	fx_cache.erase(fx_cache.begin(), fx_cache.end());
}

epi::sound_data_c *S_CacheLoad(sfxdef_c *def)
{
	for (int i = 0; i < (int)fx_cache.size(); i++)
	{
		if (fx_cache[i]->priv_data == (void*)def)
		{
			fx_cache[i]->ref_count++;
			return fx_cache[i];
		}
	}


	// create data structure
	epi::sound_data_c *buf = new epi::sound_data_c();

	fx_cache.push_back(buf);

	buf->priv_data = def;
	buf->ref_count = 1;

	// load in the data from the WAD

	const char *name = def->lump_name;
	int lumpnum = W_CheckNumForName(name);

	if (lumpnum == -1)
	{
		if (strict_errors)
			I_Error("SFX '%s' doesn't exist", name);
		else
		{
			I_Warning("Unknown sound lump %s, ignoring.\n", name);
			Load_Silence(buf);
			return buf;
		}
	}

	// Load the data into the buffer

	const byte *lump = (const byte*)W_CacheLumpNum(lumpnum);

	int length = W_LumpLength(lumpnum);
	if (length < 8)
		I_Error("Bad SFX lump '%s' : too short!", name);

	if (memcmp(lump, "RIFF", 4) == 0)
		Load_WAV(buf, lump, length);
	else if (memcmp(lump, "Ogg", 3) == 0)
		Load_OGG(buf, lump, length);
	else
		Load_DOOM(buf, lump, length);

	// caching the LUMP data is rather useless (and inefficient)
	// since it won't be needed again until the current sound
	// has been flushed from the sound cache.  Hence we just
	// flush the lump data as early as possible.
	W_DoneWithLump_Flushable(lump);

	return buf;
}

void S_CacheRelease(epi::sound_data_c *data)
{
	SYS_ASSERT(data->ref_count >= 1);

	data->ref_count--;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
