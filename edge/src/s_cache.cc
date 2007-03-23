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

#include "s_sound.h"
#include "s_cache.h"

#include "errorcodes.h"

#include "m_argv.h"
#include "m_random.h"
#include "p_mobj.h"
#include "r_defs.h"
#include "w_wad.h"

#include "epi/errors.h"
#include "epi/strings.h"

#include <stdlib.h>
#include <string.h>

#include <vector>


fx_data_c::fx_data_c() : length(0), freq(0),
						 data_L(NULL), data_R(NULL),
						 def(NULL), ref_count(0)
{ }

fx_data_c::~fx_data_c()
{
	Free();
}

void fx_data_c::Free()
{
	length = 0;

	if (data_R && data_R != data_L)
		delete[] data_R;

	if (data_L)
		delete[] data_L;

	data_L = NULL;
	data_R = NULL;
}

void fx_data_c::Allocate(int samples, bool stereo)
{
	bool this_stereo = (data_L != data_R);

	// early out when requirements are already met
	if (data_L && length >= samples && this_stereo == stereo)
		return;

	if (data_L)
	{
		Free();
	}

	length = samples;

	data_L = new s16_t[samples];

	if (stereo)
		data_R = new s16_t[samples];
	else
		data_R = data_L;
}

//----------------------------------------------------------------------------

static std::vector<fx_data_c *> fx_cache;


//----------------------------------------------------------------------------


void S_CacheInit(void)
{
	// nothing to do
}

static void S_FlushData(fx_data_c *fx)
{
	DEV_ASSERT2(fx->ref_count == 0);

	fx->Free();

}

void S_CacheClearAll(void)
{
	for (int i = 0; i < (int)fx_cache.size(); i++)
		delete fx_cache[i];

	fx_cache.erase(fx_cache.begin(), fx_cache.end());
}

fx_data_c *S_CacheLoad(sfxdef_c *def)
{
	for (int i = 0; i < (int)fx_cache.size(); i++)
	{
		if (fx_cache[i]->def == def)
		{
			fx_cache[i]->ref_count++;
			return fx_cache[i];
		}
	}


	// create data structure
	fx_data_c *buf = new fx_data_c();

	fx_cache.push_back(buf);

	buf->def = def;
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
			delete buf;

			I_Warning("Unknown sound lump %s, ignoring.\n", name);
			return NULL;
		}
	}

	// Load the data into the buffer

	const byte *lump = (const byte*)W_CacheLumpNum(lumpnum);

	int length = W_LumpLength(lumpnum);
	if (length < 8)
		I_Error("Bad SFX lump '%s' : too short!", name);

	length -= 8;

	buf->Allocate(length, false);
	buf->freq = lump[2] + (lump[3] << 8);

	// convert to signed 16-bit format
	s16_t *dest = buf->data_L;

	for (int i=0; i < length; i++)
		*dest++ = (lump[8+i] ^ 0x80) << 8;

	// caching the LUMP data is rather useless (and inefficient)
	// since it won't be needed again until the current sound
	// has been flushed from the sound cache.  Hence we just
	// flush the lump data as early as possible.
	W_DoneWithLump_Flushable(lump);

	return buf;
}

void S_CacheRelease(fx_data_c *data)
{
	DEV_ASSERT2(data->ref_count >= 1);

	data->ref_count--;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
