//----------------------------------------------------------------------------
//  EDGE Sound System
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

#include "i_defs.h"

#ifdef MACOSX
#include <SDL.h>
#else
#include <SDL/SDL.h>
#endif

#include "m_argv.h"
#include "m_random.h"
#include "m_swap.h"
#include "w_wad.h"

#include "s_sound.h"
#include "s_cache.h"
#include "s_blit.h"

#include "p_local.h" // P_ApproxDistance


static bool allow_hogs = true;

extern float listen_x;
extern float listen_y;

namespace sound
{

/* See m_option.cc for corresponding menu items */
const int channel_counts[5] = { 8, 16, 32, 64, 96 };


const int category_limit_table[3][8][3] =
{
	/* 8 channel (TEST) */
	{
		{ 1, 1, 1 }, // UI
		{ 1, 1, 1 }, // Music
		{ 1, 1, 1 }, // Player
		{ 1, 1, 1 }, // Weapon

		{ 1, 1, 1 }, // Opponent
		{ 1, 1, 1 }, // Monster
		{ 1, 1, 1 }, // Object
		{ 1, 1, 1 }, // Level
	},

	/* 16 channel */
	{
		{ 1, 1, 1 }, // UI
		{ 1, 1, 1 }, // Music
		{ 1, 1, 1 }, // Player
		{ 2, 2, 2 }, // Weapon

		{ 0, 2, 6 }, // Opponent
		{ 6, 4, 0 }, // Monster
		{ 3, 3, 3 }, // Object
		{ 2, 2, 2 }, // Level
	},

	/* 32 channel */
	{
		{ 2, 2, 2 }, // UI
		{ 2, 2, 2 }, // Music
		{ 2, 2, 2 }, // Player
		{ 3, 3, 3 }, // Weapon

		{ 0, 4,10 }, // Opponent
		{12, 9, 2 }, // Monster
		{ 7, 6, 7 }, // Object
		{ 4, 4, 4 }, // Level
	},

	// NOTE: never put a '0' on the WEAPON line, since the top
	// four categories should never be merged with the rest.
};

int cat_limits[SNCAT_NUMTYPES];
int cat_counts[SNCAT_NUMTYPES];


void SetupCategoryLimits(void)
{
	// Assumes: num_chan to be already set, and the DEATHMATCH()
	//          and COOP_MATCH() macros are working.

	int mode = 0;
	if (COOP_MATCH()) mode = 1;
	if (DEATHMATCH()) mode = 2;

	int idx = 0;
	if (num_chan >= 16) idx=1;
	if (num_chan >= 32) idx=2;

	int multiply = 1;
	if (num_chan >= 64) multiply = num_chan / 32;

	for (int t = 0; t < SNCAT_NUMTYPES; t++)
	{
		cat_limits[t] = category_limit_table[idx][t][mode] * multiply;
		cat_counts[t] = 0;
	}
}

int FindFreeChannel(void)
{
	for (int i=0; i < num_chan; i++)
		if (! mix_chan[i]->data)
			return i;

	return -1; // not found
}

int FindPlayingFX(sfxdef_c *def, int cat, position_c *pos)
{
	for (int i=0; i < num_chan; i++)
	{
		mix_channel_c *chan = mix_chan[i];

		if (chan->data && chan->def == def && chan->category == cat &&
			chan->pos == pos)
		{
			return i;
		}
	}

	return -1; // not found
}

int FindBiggestHog(int real_cat)
{
	int biggest_hog = -1;
	int biggest_extra = 0;

	for (int hog = 0; hog < SNCAT_NUMTYPES; hog++)
	{
		if (hog == real_cat)
			continue;

		int extra = cat_counts[hog] - cat_limits[hog];

		if (extra <= 0)
			continue;

		// found a hog!
		if (biggest_hog < 0 || extra > biggest_extra)
		{
			biggest_hog = hog;
			biggest_extra = extra;
		}
	}

	DEV_ASSERT2(biggest_hog >= 0);

	return biggest_hog;
}

void CountPlayingCats(void)
{
	for (int c=0; c < SNCAT_NUMTYPES; c++)
		cat_counts[c] = 0;

	for (int i=0; i < num_chan; i++)
	{
		mix_channel_c *chan = mix_chan[i];

		if (chan->data)
			cat_counts[chan->category] += 1;
	}
}

int ChannelScore(sfxdef_c *def, int category, position_c *pos, bool boss)
{
	// for full-volume sounds, use the priority from DDF
	if (category <= SNCAT_Weapon)
	{
		return 200 - def->priority;
	}

	// for stuff in the level, use the distance
	DEV_ASSERT2(pos);

	float dist = boss ? 0 :
		P_ApproxDistance(listen_x - pos->x, listen_y - pos->y);

I_Printf("listen @ (%1.0f,%1.0f)  |  pos @ (%1.0f,%1.0f)\n",
listen_x, listen_y, pos->x, pos->y);
	int base_score = 2000 - (int)(dist / 10.0);

	return base_score * 100 - def->priority;
}

int FindChannelToKill(int kill_cat, int real_cat, int new_score)
{
	int kill_idx = -1;
	int kill_score = (1<<30);

I_Printf("FindChannelToKill: cat:%d new_score:%d\n", kill_cat, new_score);
	for (int j=0; j < num_chan; j++)
	{
		mix_channel_c *chan = mix_chan[j];

		if (! chan->data)
			continue;
		
		if (chan->category != kill_cat)
			continue;
		
		int score = ChannelScore(chan->def, chan->category,
								 chan->pos, chan->boss);
I_Printf("> [%d] '%s' = %d\n", j, chan->def->lump_name.GetString(), score);
		// find one with LOWEST score
		if (score < kill_score)
		{
			kill_idx = j;
			kill_score = score;
		}
	}
I_Printf("kill_idx = %d\n", kill_idx);
	DEV_ASSERT2(kill_idx >= 0);

	if (kill_cat != real_cat)
		return kill_idx;

	// if the score for new sound is worse than any existing
	// channel, then simply discard the new sound.
	if (new_score >= kill_score)
		return kill_idx;

	return -1;
}


void Init(void)
{
	if (nosound) return;

	// setup channels
	S_InitChannels(8);

	SetupCategoryLimits();

	// okidoke, start the ball rolling!
	SDL_PauseAudio(0);
}

void Shutdown(void)
{
	if (nosound) return;

	SDL_PauseAudio(1);
}

void ClearAllFX(void)
{
	if (nosound) return;

	// FIXME !!!
}

// Not-rejigged-yet stuff..
sfxdef_c * LookupEffectDef(const sfx_t *s) 
{ 
	DEV_ASSERT2(s->num >= 1);

	// -KM- 1999/01/31 Using P_Random here means demos and net games 
	//  get out of sync.
	// -AJA- 1999/07/19: That's why we use M_Random instead :).

	int num;

	if (s->num > 1)
		num = s->sounds[M_Random() % s->num];
	else
		num = s->sounds[0];

	DEV_ASSERT2(0 <= num && num < sfxdefs.GetSize());

	return sfxdefs[num];
}

static void S_PlaySound(int idx, sfxdef_c *def, int category, position_c *pos, int flags)
{
I_Printf("S_PlaySound on idx #%d DEF:%p\n", idx, def);

	mix_channel_c *chan = mix_chan[idx];

I_Printf("Looked up def: %p, caching...\n", def);
	chan->data = S_CacheLoad(def);
	DEV_ASSERT2(chan->data);

I_Printf("chan=%p data=%p\n", chan, chan->data);

	chan->def = def;
	chan->pos = pos;
	chan->category = category; //?? store use_cat and orig_cat

	// volume computed during mixing (?)
    chan->volume_L = 0;
    chan->volume_R = 0;

	chan->offset   = 0;
	chan->length   = chan->data->length << 10;

	chan->loop = (flags & FX_Loop) ? true : false;
	chan->boss = (flags & FX_Boss) ? true : false;

	chan->ComputeDelta();

I_Printf("FINISHED: delta=0x%lx\n", chan->delta);
}

static void S_KillChannel(int k)
{
	//!!!! FIXME WAY FUCKED UP!
	mix_chan[k]->data = NULL;
}

static void DoStartFX(sfxdef_c *def, int category, position_c *pos, int flags)
{
	CountPlayingCats();

	int k = FindPlayingFX(def, category, pos);

	if (k >= 0)
	{
		mix_channel_c *chan = mix_chan[k];

		if (flags & FX_Loop)
		{
			chan->loop = true;
			return;
		}

		if (flags & FX_Single)
		{
			if (! (flags & FX_Cut))
				return;

			S_KillChannel(k);
			S_PlaySound(k, def, category, pos, flags);

			return;
		}
	}

	k = FindFreeChannel();

	if (! allow_hogs)
	{
		if (cat_counts[category] >= cat_limits[category])
			k = -1;
	}

	if (k < 0)
	{
		// all channels are in use.
		// either kill one, or drop the new sound.

		int new_score = ChannelScore(def, category, pos,
							(flags & FX_Boss) ? true : false);

		// decide which category to kill a sound in.
		int kill_cat = category;
			
		if (cat_counts[category] < cat_limits[category])
		{
			// we haven't reached our quota yet, hence kill a hog.
			kill_cat = FindBiggestHog(category);
		}

		DEV_ASSERT2(cat_counts[kill_cat] >= cat_limits[kill_cat]);

		k = FindChannelToKill(kill_cat, category, new_score);

if (k<0) I_Printf("- new score too low\n");
		if (k < 0)
			return;

I_Printf("- killing channel %d (kill_cat:%d)  my_cat:%d\n",
k, kill_cat, category);
		S_KillChannel(k);
	}

	S_PlaySound(k, def, category, pos, flags);
}

void StartFX(sfx_t *sfx, int category, position_c *pos, int flags)
{
	if (nosound || !sfx) return;

	sfxdef_c *def = LookupEffectDef(sfx);
	DEV_ASSERT2(def);

	// FIXME: ignore very far away sounds (> 5000 == S_CLIPPING_DIST)

	if (def->looping)
		flags |= FX_Loop;

	if (def->singularity > 0)
	{
		flags |= FX_Single;
		flags |= (def->precious ? 0 : FX_Cut);
	}

I_Printf("StartFX: '%s' cat:%d flags:0x%04x\n", def->lump_name.GetString(),
	category, flags);

	DEV_ASSERT2(0 <= category && category < SNCAT_NUMTYPES);

	while (cat_limits[category] == 0)
		category++;

	SDL_LockAudio();
	{
		DoStartFX(def, category, pos, flags);
	}
	SDL_UnlockAudio();
}

void StopFX(position_c *pos)
{
	if (nosound) return;

	// FIXME !!!
}

void StopLoopingFX(position_c *pos)
{
}

bool IsFXPlaying(position_c *pos)
{
	return false;
} 

void UnlinkFX(position_c *pos)
{
	if (nosound) return;

	SDL_LockAudio();
	{
		for (int i = 0; i < num_chan; i++)
		{
			mix_channel_c *chan = mix_chan[i];

			if (chan->data && chan->pos == pos)
				S_KillChannel(i);
		}
	}
	SDL_UnlockAudio();
}

// Ticker
void Ticker()
{
	if (nosound) return;

	SDL_LockAudio();
	{
		if (::numplayers == 0) // FIXME: Yuck!
		{
			S_UpdateSounds(NULL, 0);
		}
		else
		{
			mobj_t *pmo = ::players[displayplayer]->mo;
			DEV_ASSERT2(pmo);
		   
			S_UpdateSounds(pmo, pmo->angle);
		}
	}
	SDL_UnlockAudio();
}

void PauseAllFX()
{
}

void ResumeAllFX()
{
}

int ReserveFX(int category)
{
	return -1;
}

void UnreserveFX(int handle)
{
}

int GetVolume()
{
	return 1;  // fixme ?
}

void SetVolume(int volume)
{
}

} // namespace sound


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
