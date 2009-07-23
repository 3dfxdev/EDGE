//----------------------------------------------------------------------------
//  EDGE Sound System
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2008  The EDGE Team.
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
#include "i_sdlinc.h"

#include "g_game.h"
#include "g_state.h"
#include "m_argv.h"
#include "m_misc.h"
#include "m_random.h"
#include "w_wad.h"

#include "s_sound.h"
#include "s_cache.h"
#include "s_blit.h"

#include "p_local.h" // P_ApproxDistance


extern float listen_x;
extern float listen_y;
extern float listen_z;


cvar_c s_volume;
cvar_c s_mixchan;

cvar_c s_rate;
cvar_c s_bits;
cvar_c s_stereo;


const int category_limit_table[SNCAT_NUMTYPES][2] =
{
	/* 64 channel */

    //COOP  DM
	{   3,   3  }, // UI
	{   4,   4  }, // Player
	{   7,   7  }, // Weapon
	{   8,  24  }, // Opponent
	{  22,   4  }, // Monster
	{  12,  14  }, // Object
	{   8,   8  }, // Level
};


static int cat_limits[SNCAT_NUMTYPES];
static int cat_counts[SNCAT_NUMTYPES];


static void SetupCategoryLimits(void)
{
	// Assumes: num_chan is already set, and the DEATHMATCH()
	//          and COOP_MATCH() macros are working.

	int mode = DEATHMATCH() ? 1 : 0;

	int remainders[SNCAT_NUMTYPES];
	int total = 0;

	for (int t = 0; t < SNCAT_NUMTYPES; t++)
	{
		int limit = category_limit_table[t][mode];

		cat_limits[t] = limit * num_chan / 64;
		cat_counts[t] = 0;

		total = total + cat_limits[t];
		remainders[t] = limit * num_chan % 64;

//		I_Debugf("Initial limit [%d] = %d  remainder:%d\n", 
//		         t, cat_limits[t], remainders[t]);
	}

//	I_Debugf("Total: %d\n", total);

	SYS_ASSERT(total <= num_chan);

	// distribute any extra channels (weapons get preference)

	while (total < num_chan)
	{
		int best = 0;
		int best_rem = -1;

		for (int t = 0; t < SNCAT_NUMTYPES; t++)
			if (remainders[t] > best_rem)
			{
				best = t;
				best_rem = remainders[t];
			}

		cat_limits[best] += 1;
		remainders[best] = -1;

		total++;
	}

	I_Debugf("SFX Category limits: %d %d %d  %d %d %d %d\n",
			 cat_limits[0], cat_limits[1], cat_limits[2],
			 cat_limits[3], cat_limits[4], cat_limits[5],
			 cat_limits[6]);
}

static int FindFreeChannel(void)
{
	for (int i=0; i < num_chan; i++)
	{
		mix_channel_c *chan = mix_chan[i];

		if (chan->state == CHAN_Finished)
			S_KillChannel(i);

		if (chan->state == CHAN_Empty)
			return i;
	}

	return -1; // not found
}

static int FindPlayingFX(sfxdef_c *def, int cat, position_c *pos)
{
	for (int i=0; i < num_chan; i++)
	{
		mix_channel_c *chan = mix_chan[i];

		if (chan->state == CHAN_Playing && chan->category == cat &&
			chan->pos == pos)
		{
			if (chan->def == def)
				return i;

			if (chan->def->singularity > 0 && chan->def->singularity == def->singularity)
				return i;
		}
	}

	return -1; // not found
}

static int FindBiggestHog(int real_cat)
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
			biggest_hog   = hog;
			biggest_extra = extra;
		}
	}

	SYS_ASSERT(biggest_hog >= 0);

	return biggest_hog;
}

static void CountPlayingCats(void)
{
	for (int c=0; c < SNCAT_NUMTYPES; c++)
		cat_counts[c] = 0;

	for (int i=0; i < num_chan; i++)
	{
		mix_channel_c *chan = mix_chan[i];

		if (chan->state == CHAN_Playing)
			cat_counts[chan->category] += 1;
	}
}

static int ChannelScore(sfxdef_c *def, int category, position_c *pos, bool boss)
{
	// for full-volume sounds, use the priority from DDF
	if (category <= SNCAT_Weapon)
	{
		return 200 - def->priority;
	}

	// for stuff in the level, use the distance
	SYS_ASSERT(pos);

	float dist = boss ? 0 :
		P_ApproxDistance(listen_x - pos->x, listen_y - pos->y, listen_z - pos->z);

	int base_score = 999 - (int)(dist / 10.0);

	return base_score * 100 - def->priority;
}

static int FindChannelToKill(int kill_cat, int new_cat, int new_score)
{
	int kill_idx = -1;
	int kill_score = (1<<30);

	for (int j=0; j < num_chan; j++)
	{
		mix_channel_c *chan = mix_chan[j];

		if (chan->state != CHAN_Playing)
			continue;

		if (chan->category != kill_cat)
			continue;
		
		int score = ChannelScore(chan->def, chan->category,
								 chan->pos, chan->boss);

		if (score < kill_score)
		{
			kill_idx = j;
			kill_score = score;
		}
	}

	SYS_ASSERT(kill_idx >= 0);

	if (kill_cat != new_cat)
		return kill_idx;

	// if the score for new sound is worse than any existing
	// channel, then simply discard the new sound.
	if (new_score >= kill_score)
		return kill_idx;

	return -1;
}


void S_Init(void)
{
	if (nosound) return;

	int want_chan = CLAMP(16, s_mixchan.d, 64);

	I_Printf("I_StartupSound: Init %d mixing channels\n", want_chan);

	// setup channels
	S_InitChannels(want_chan);

	SetupCategoryLimits();

	S_QueueInit();

	// okidoke, start the ball rolling!
	SDL_PauseAudio(0);
}

void S_Shutdown(void)
{
	if (nosound) return;

	SDL_PauseAudio(1);

	// make sure mixing thread is not running our code
	SDL_LockAudio();
	SDL_UnlockAudio();

	S_QueueShutdown();

	S_FreeChannels();
}

// Not-rejigged-yet stuff..
sfxdef_c * LookupEffectDef(const sfx_t *s) 
{ 
	SYS_ASSERT(s->num >= 1);

	// need to use M_Random here to prevent demos and net games 
	// getting out of sync.

	int num;

	if (s->num > 1)
		num = s->sounds[M_Random() % s->num];
	else
		num = s->sounds[0];

	SYS_ASSERT(0 <= num && num < sfxdefs.GetSize());

	return sfxdefs[num];
}

static void S_PlaySound(int idx, sfxdef_c *def, int category, position_c *pos, int flags)
{
//I_Printf("S_PlaySound on idx #%d DEF:%p\n", idx, def);

	epi::sound_data_c *buf = S_CacheLoad(def);
	if (! buf)
		return;

	mix_channel_c *chan = mix_chan[idx];

	chan->state = CHAN_Playing;
	chan->data  = buf;

	chan->def = def;
	chan->pos = pos;
	chan->category = category;

	// volume computed during mixing (?)
    chan->volume_L = 0;
    chan->volume_R = 0;

	chan->offset = 0;
	chan->length = chan->data->length << 10;

	chan->loop = false;
	chan->boss = (flags & FX_Boss) ? true : false;

	chan->ComputeDelta();
}

static void DoStartFX(sfxdef_c *def, int category, position_c *pos, int flags)
{
	CountPlayingCats();

	int k = FindPlayingFX(def, category, pos);

	if (k >= 0)
	{
//I_Printf("@ already playing on #%d\n", k);
		mix_channel_c *chan = mix_chan[k];

		if (def->looping && def == chan->def)
		{
//I_Printf("@@ RE-LOOPING\n");
			chan->loop = true;
			return;
		}
		else if (flags & FX_Single)
		{
			if (flags & FX_Precious)
				return;

//I_Printf("@@ Killing sound for SINGULAR\n");
			S_KillChannel(k);
			S_PlaySound(k, def, category, pos, flags);
			return;
		}
	}

	k = FindFreeChannel();

//I_Printf("@ free channel = #%d\n", k);
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
//I_Printf("@ biggest hog: %d\n", kill_cat);
		}

		SYS_ASSERT(cat_counts[kill_cat] >= cat_limits[kill_cat]);

		k = FindChannelToKill(kill_cat, category, new_score);

//if (k<0) I_Printf("- new score too low\n");
		if (k < 0)
			return;

//I_Printf("- killing channel %d (kill_cat:%d)  my_cat:%d\n", k, kill_cat, category);
		S_KillChannel(k);
	}

	S_PlaySound(k, def, category, pos, flags);
}


void S_StartFX(sfx_t *sfx, int category, position_c *pos, int flags)
{
	if (nosound || !sfx) return;

	SYS_ASSERT(0 <= category && category < SNCAT_NUMTYPES);

	if (category >= SNCAT_Opponent && ! pos)
		I_Error("S_StartFX: position missing for category: %d\n", category);

	if (cat_limits[category] == 0)
		return;

	sfxdef_c *def = LookupEffectDef(sfx);
	SYS_ASSERT(def);

	// ignore very far away sounds
	if (category >= SNCAT_Opponent && !(flags & FX_Boss))
	{
		float dist = P_ApproxDistance(listen_x - pos->x, listen_y - pos->y, listen_z - pos->z);

		if (dist > def->max_distance)
			return;
	}

	if (def->singularity > 0)
	{
		flags |= FX_Single;
		flags |= (def->precious ? FX_Precious : 0);
	}

//I_Printf("StartFX: '%s' cat:%d flags:0x%04x\n", def->name.c_str(), category, flags);

	I_LockAudio();
	{
		DoStartFX(def, category, pos, flags);
	}
	I_UnlockAudio();
}


void S_StopFX(position_c *pos)
{
	if (nosound) return;

	I_LockAudio();
	{
		for (int i = 0; i < num_chan; i++)
		{
			mix_channel_c *chan = mix_chan[i];

			if (chan->state == CHAN_Playing && chan->pos == pos)
			{
				S_KillChannel(i);
			}
		}
	}
	I_UnlockAudio();
}

void S_StopLevelFX(void)
{
	if (nosound) return;

	I_LockAudio();
	{
		for (int i = 0; i < num_chan; i++)
		{
			mix_channel_c *chan = mix_chan[i];

			if (chan->state != CHAN_Empty && chan->category != SNCAT_UI)
			{
				S_KillChannel(i);
			}
		}
	}
	I_UnlockAudio();
}


void S_SoundTicker(void)
{
	if (nosound) return;

	if (s_mixchan.CheckModified())
		S_ChangeChannelNum();

	I_LockAudio();
	{
		if (gamestate == GS_LEVEL)
		{
			SYS_ASSERT(::numplayers > 0);

			mobj_t *pmo = ::players[displayplayer]->mo;
			SYS_ASSERT(pmo);

			S_UpdateSounds(pmo, pmo->angle);
		}
		else
		{
			S_UpdateSounds(NULL, 0);
		}
	}
	I_UnlockAudio();
}

void S_ChangeChannelNum(void)
{
	if (nosound) return;

	I_LockAudio();
	{
		int want_chan = CLAMP(16, s_mixchan.d, 64);

		S_ReallocChannels(want_chan);

		SetupCategoryLimits();
	}
	I_UnlockAudio();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
