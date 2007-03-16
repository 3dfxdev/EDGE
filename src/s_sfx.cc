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


namespace sound
{

const int category_limit_table[2][8][3] =
{
/* TEST:
 */
	{
		{ 1, 1, 1 }, /* UI */
		{ 1, 1, 1 }, /* Music */
		{ 1, 1, 1 }, /* Player */
		{ 1, 1, 1 }, /* Weapon */

		{ 1, 1, 1 }, /* Opponent */
		{ 1, 1, 1 }, /* Monster */
		{ 1, 1, 1 }, /* Object */
		{ 1, 1, 1 }, /* Level */
	},

	{
		{ 0, 0, 0 }, /* UI */
		{ 0, 0, 0 }, /* Music */
		{ 0, 0, 0 }, /* Player */
		{ 4, 4, 4 }, /* Weapon */

		{ 0, 0, 0 }, /* Opponent */
		{ 0, 0, 0 }, /* Monster */
		{ 0, 0, 0 }, /* Object */
		{ 4, 4, 4 }, /* Level */
	},
#if 0
	/* 16 channel */
	{
		{ 1, 1, 1 }, /* UI */
		{ 1, 1, 1 }, /* Music */
		{ 1, 1, 2 }, /* Player */
		{ 2, 2, 2 }, /* Weapon */

		{ 0, 2, 5 }, /* Opponent */
		{ 7, 5, 0 }, /* Monster */
		{ 0, 0, 0 }, /* Object */
		{ 4, 4, 5 }, /* Level */
	},

	/* 32 channel */
	{
		{ 2, 2, 2 }, /* UI */
		{ 2, 2, 2 }, /* Music */
		{ 2, 2, 2 }, /* Player */
		{ 3, 3, 3 }, /* Weapon */

		{ 0, 4, 9 }, /* Opponent */
		{13, 9, 3 }, /* Monster */
		{ 5, 5, 5 }, /* Object */
		{ 5, 5, 6 }, /* Level */
	},
#endif
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
	if (num_chan >= 32) idx=1;

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

#if 0
int CountPlayingCats(int use_cat, bool petty_only = false)
{
	int count = 0;

	for (int i=0; i < (int)playing_fx.size(); i++)
	{
		if (petty_only && (playing_fx[i].flags & FX_Petty))
			continue;

		if (playing_fx[i].def && playing_fx[i].use_cat == use_cat)
			count++;
	}

	return count;
}
#endif

int FindChannelToKill(int use_cat, float good_dist)
{
	int kill_idx = -1;
	int kill_score = (1<<30);

	for (int j=0; j < num_chan; j++)
	{
		mix_channel_c *chan = mix_chan[j];

		if (! chan->data)
			continue;
		
		if (chan->category != kill_cat)
			continue;
		
		int score = 200 - chan->def->priority; //!!!

		// find one with LOWEST score
		if (kill_idx < 0 || score < kill_score)
		{
			kill_idx = j;
			kill_score = score;
		}
	}

	DEV_ASSERT2(kill_idx >= 0);

	if (kill_cat != real_cat)
		return kill_idx;

	// if the score for new sound doesn't beat any existing channel,
	// then simply discard the new sound.
	if (new_score > kill_score)
		return kill_idx;

	return -1;
}

#if 0
int FindHogToKill(int use_cat)
{
	int i;
	int hogs[CAT_NUMTYPES];

	for (i=0; i < CAT_NUMTYPES; i++)
		hogs[i] = 0;

	for (int cat = 0; cat < CAT_NUMTYPES; cat++)
	{
		if (cat == use_cat)
			continue;

		int playing = CountPlayingCats(cat);

		if (playing > cat_limit[cat])
			hogs[cat] = playing - cat_limit[cat];
	}

	int hog_cat = -1;
	int hog_score = 0;

	for (int cat = CAT_NUMTYPES-1; cat >= 0; cat--)
	{
		if (hogs[cat] > hog_score)
		{
			hog_score = hogs[cat];
			hog_cat = cat;
		}
	}

	SYS_ASSERT(hog_cat >= 0);

	int hog_fx = FindCatToKill(hog_cat, -1.0f);

	SYS_ASSERT(hog_fx >= 0);

	return hog_fx;
}

#endif


// Init/Shutdown  FIXME: merge with I_Startup/I_ShutdownSound ?
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

static void S_PlaySoundOnChannel(int idx, sfxdef_c *def, int category, position_c *pos)
{
I_Printf("S_PlaySoundOnChannel on idx #%d DEF:%p\n", idx, def);

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

	chan->ComputeDelta();

I_Printf("FINISHED: delta=0x%x\n", chan->delta);
}

static void DoStartFX(sfxdef_c *def, int category, position_c *pos, int flags)
{
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
			S_PlaySoundOnChannel(k, def, category, pos);

			return;
		}

		// QUESTION: if already playing, and have to kill a
		// sound in existing channel -- what to do?
		// (allow multiple sounds in this case??)
	}

	k = FindFreeChannel();

	if (k < 0)
	{
		// all channels are in use.
		int my_score = 200 - def->priority; //!!!!

		// decide which category to kill a sound in.
		int kill_cat = category;
			
		if (cat_counts[category] < cat_limits[category])
		{
			// we haven't reached our quota yet, so kill a hog.
			kill_cat = FindBiggestHog();
		}

		DEV_ASSERT2(cat_counts[kill_cat] > cat_limits[kill_cat]);

		k = FindChannelToKill(  );

		if (k < 0)
			return;

		S_KillChannel(k);
	}

	S_PlaySoundOnChannel(k, def, category, pos);
}

void StartFX(sfx_t *sfx, int category, position_c *pos, int flags)
{
I_Printf("StartFX CALLED! nosound=%d\n", nosound);

	if (nosound || !sfx) return;

	sfxdef_c *def = LookupEffectDef(sfx);
	DEV_ASSERT2(def);

	if (def->looping)
		flags |= FX_Loop;

	if (def->singularity)
	{
		flags |= FX_Single;
		flags |= (def->precious ? 0 : FX_Cut);
	}

	DEV_ASSERT2(0 <= category && category < SNCAT_NUMTYPES);

//	bool is_relative = (category <= SNCAT_Weapon);

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

	//!!!!! FIXME
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
