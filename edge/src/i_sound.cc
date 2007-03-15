//----------------------------------------------------------------------------
//  EDGE Sound System for SDL
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
//  -AJA- 2000/07/07: Began work on SDL sound support.
//

#include "i_defs.h"

#ifdef MACOSX
#include <SDL.h>
#else
#include <SDL/SDL.h>
#endif

///??? #include "i_sysinc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "m_argv.h"
#include "m_random.h"
#include "m_swap.h"
#include "w_wad.h"

#include "s_sound.h"
#include "s_cache.h"
#include "s_blit.h"


// If true, sound system is off/not working. Changed to false if sound init ok.
bool nosound = false;

/* See m_option.cc for corresponding menu items */
static const int sample_rates[4]   = { 11025, 16000, 22050, 44100 };
static const int sample_bits[2]    = { 8, 16 };
static const int channel_counts[4] = { 16, 32, 64, 128 };


static SDL_AudioSpec mydev;

int dev_freq;
int dev_bits;
int dev_bytes_per_sample;
int dev_frag_pairs;
int dev_stereo;


#define PRI_NOSOUND   -1
#define PRI_FINISHED  -2

// Error Description
static char errordesc[256] = "FOO";
static char scratcherror[256];


void SoundFill_Callback(void *udata, Uint8 *stream, int len)
{
	S_MixAllChannels(stream, len);
}

//
// I_StartupSound
//
bool I_StartupSound(void *sysinfo)
{
	if (nosound)
		return true;

	const char *p;
	SDL_AudioSpec firstdev;

	int want_freq;
	int want_bits;
	int want_stereo;

	p = M_GetParm("-freq");
	if (p)
		want_freq = atoi(p);
	else
		want_freq = 11025;

	want_bits = (M_CheckParm("-sound16") > 0) ? 16 : 8;
	want_stereo = false; //!!!!! (M_CheckParm("-mono") > 0) ? 0 : 1;

	firstdev.freq = want_freq;
	firstdev.format = (want_bits < 12) ? AUDIO_U8 : AUDIO_S16SYS;
	firstdev.channels = want_stereo ? 2 : 1;
	firstdev.samples = 512;
	firstdev.callback = SoundFill_Callback;

	if (SDL_OpenAudio(&firstdev, &mydev) < 0)
	{
		I_Printf("I_StartupSound: Couldn't open sound: %s\n", SDL_GetError());
		nosound = true;
		return false;
	}

#ifdef DEVELOPERS
	// get round SDL's signal handlers
	signal(SIGFPE,  SIG_DFL);
	signal(SIGSEGV, SIG_DFL);
#endif

	// check stuff...

	if (mydev.format == AUDIO_S16SYS)
		dev_bits = 16;
	else if (mydev.format == AUDIO_U8)
		dev_bits = 8;
	else
	{
		I_Printf("I_StartupSound: unsupported format: %d\n", mydev.format);
		nosound = true;
		return false;
	}

	if (dev_bits != want_bits)
		I_Printf("I_StartupSound: %d bits not available.\n", want_bits);

	if (want_stereo && mydev.channels != 2)
		I_Printf("I_StartupSound: stereo sound not available.\n");
	else if (!want_stereo && mydev.channels != 1)
		I_Printf("I_StartupSound: mono sound not available.\n");

	if (mydev.freq < (want_freq - want_freq/100) || 
		mydev.freq > (want_freq + want_freq/100))
	{
		I_Printf("I_StartupSound: %d Hz sound not available.\n", want_freq);
	}

	// display some useful stuff
	I_Printf("I_StartupSound: SDL Started: %d Hz, %d bits, %s\n",
			mydev.freq, dev_bits, (mydev.channels==2) ? "Stereo" : "Mono");

	dev_bytes_per_sample = (mydev.channels) * (dev_bits / 8);
	dev_frag_pairs = mydev.size / dev_bytes_per_sample;

	DEV_ASSERT2(dev_bytes_per_sample > 0);
	DEV_ASSERT2(dev_frag_pairs > 0);

	dev_freq   = mydev.freq;
	dev_stereo = (mydev.channels == 2);

	return true;
}

//
// I_ShutdownSound
//
void I_ShutdownSound(void)
{
	if (nosound)
		return;

	nosound = true;

	SDL_CloseAudio();

	S_FreeChannels();
}


#if 0
//
// I_SoundPlayback
//
int I_SoundPlayback(unsigned int soundid, int pan, int vol, bool looping)
{
	int i;
	int lvol, rvol;

	mix_sound_t *sfx;

	if (nosound)
		return true;

	DEV_ASSERT(soundid > 0 && soundid < stored_sfx_num,
			("I_SoundPlayback: Sound %d does not exist!", soundid));

	sfx = stored_sfx[soundid];

	SDL_LockAudio();

	for (i=0; i < MIX_CHANNELS; i++)
	{
		if (mix_chan[i].priority == PRI_NOSOUND)
			break;
	}

	if (i >= MIX_CHANNELS)
	{
		SDL_UnlockAudio();
		sprintf(errordesc, "I_SoundPlayback: Unable to allocate voice.");
		return -1;
	}

	// compute panning
	lvol = rvol = vol;

	if (mydev.channels == 2)
	{
		DEV_ASSERT2(0 <= pan && pan <= 255);

		// strictly linear equations
		lvol = (lvol * (255 - pan)) / 255;
		rvol = (rvol * (0   + pan)) / 255;
	}

	mix_chan[i].volume_L = lvol << 8;
	mix_chan[i].volume_R = rvol << 8;
	mix_chan[i].looping  = looping;
	mix_chan[i].sound    = sfx;
	mix_chan[i].paused   = 0;
	mix_chan[i].offset   = 0;
	mix_chan[i].priority = 1;  // set priority last

	SDL_UnlockAudio();
	return i;
}

//
// I_SoundKill
//
bool I_SoundKill(unsigned int chanid)
{
	SDL_LockAudio();

	if (mix_chan[chanid].priority == PRI_NOSOUND)
	{
		SDL_UnlockAudio();
		sprintf(errordesc, "I_SoundKill: channel not playing.\n");
		return false;
	}

	mix_chan[chanid].priority = PRI_NOSOUND;
	mix_chan[chanid].volume_L = 0;
	mix_chan[chanid].volume_R = 0;
	mix_chan[chanid].looping  = false;
	mix_chan[chanid].sound    = NULL;
	mix_chan[chanid].paused   = 0;
	mix_chan[chanid].offset   = 0;

	SDL_UnlockAudio();
	return true;
}

//
// I_SoundCheck
//
bool I_SoundCheck(unsigned int chanid)
{
	return (mix_chan[chanid].priority >= 0);
}

//
// I_SoundAlter
//
bool I_SoundAlter(unsigned int chanid, int pan, int vol)
{
	int lvol, rvol;

	if (mix_chan[chanid].priority == PRI_NOSOUND)
	{
		sprintf(errordesc, "I_SoundAlter: channel not playing.\n");
		return false;
	}

	// compute panning
	lvol = rvol = vol;

	if (mydev.channels == 2)
	{
		DEV_ASSERT2(0 <= pan && pan <= 255);

		// strictly linear equations
		lvol = (lvol * (255 - pan)) / 255;
		rvol = (rvol * (0   + pan)) / 255;
	}

	mix_chan[chanid].volume_L = lvol << 8;
	mix_chan[chanid].volume_R = rvol << 8;

	return true;
}

//
// I_SoundPause
//
bool I_SoundPause(unsigned int chanid)
{
	if (nosound)
		return true;

	if (mix_chan[chanid].priority == PRI_NOSOUND)
	{
		sprintf(errordesc, "I_SoundPause: channel not playing.\n");
		return false;
	}

	mix_chan[chanid].paused = 1;
	return true;
}

//
// I_SoundStopLooping
//
bool I_SoundStopLooping(unsigned int chanid)
{
	if (nosound)
		return true;

	mix_chan[chanid].looping = false;
	return true;
}


//
// I_SoundResume
//
bool I_SoundResume(unsigned int chanid)
{
	if (nosound)
		return true;

	if (mix_chan[chanid].priority == PRI_NOSOUND)
	{
		sprintf(errordesc, "I_SoundResume: channel not playing.\n");
		return false;
	}

	mix_chan[chanid].paused = 0;
	return true;
}
#endif

//----------------------------------------------------------------------------

//
// I_SoundTicker
//
void I_SoundTicker(void)
{
	if (nosound)
		return;
}

//
// I_SoundReturnError
//
const char *I_SoundReturnError(void)
{
	memcpy(scratcherror, errordesc, sizeof(scratcherror));
	memset(errordesc, '\0', sizeof(errordesc));

	return scratcherror;
}

//----------------------------------------------------------------------------

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

#if 0

int FindPlayingFX(sfxdef_c *def, int cat, POSX pos)
{
	for (int i=0; i < (int)playing_fx.size(); i++)
	{
		if (playing_fx[i].def == def && playing_fx[i].use_cat == use_cat && SAME POS)
			return i;

	}

	return -1;  // nope
}

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

int FindCatToKill(int use_cat, float good_dist)
{
	int fx = -1;

	for (int i=0; i < (int)playing_fx.size(); i++)
	{
		if (! playing_fx[i].def)
			continue;

		if (playing_fx[i].use_cat != use_cat)
			continue;

		float score = playing_fx[i].Score(XXX, YYY);

		if (score > good_dist)
		{
			good_dist = score;
			fx = i;
		}
	}

	return fx;
}

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

static void S_PlaySoundOnChannel(int idx, sfx_t *sfx, int category, position_c *pos)
{
I_Printf("S_PlaySoundOnChannel on idx #%d SFX:%p\n", idx, sfx);

	mix_channel_c *chan = mix_chan[idx];

	sfxdef_c *def = LookupEffectDef(sfx);
		
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

void StartFX(sfx_t *sfx, int category, position_c *pos, int flags)
{
I_Printf("StartFX CALLED! nosound=%d\n", nosound);

	if (nosound) return;
	if (!sfx) return;

	DEV_ASSERT2(0 <= category && category < SNCAT_NUMTYPES);

	int use_cat = category;

	while (cat_limits[use_cat] == 0)
		use_cat++;
	
	bool is_relative = (use_cat <= SNCAT_Weapon);

I_Printf("Locking SDL Audio...\n");
	SDL_LockAudio();
I_Printf("DONE.\n");

	for (int i=0; i < num_chan; i++)
	{
		DEV_ASSERT2(mix_chan[i]);

		if (! mix_chan[i]->data)
		{
			S_PlaySoundOnChannel(i, sfx, category, pos);
			break;
		}
	}

I_Printf("Unlocking...\n");
	SDL_UnlockAudio();
I_Printf("Returning to your regular viewing.\n");
}

void StopFX(position_c *pos)
{
	if (nosound) return;

	// FIXME !!!
}

void StopLoopingFX(position_c *pos) { }

bool IsFXPlaying(position_c *pos) { return false; } 

// Playsim Object <-> Effect Linkage
void UnlinkFX(position_c *pos)
{
	if (nosound) return;

	//!!!!! FIXME
}

// Ticker
void Ticker()
{
	if (nosound) return;

	if (::numplayers == 0) // FIXME: Yuck!
	{
		S_SetListener(NULL, 0);
	}
	else
	{
		mobj_t *pmo = ::players[displayplayer]->mo;
		DEV_ASSERT2(pmo);
	   
		S_SetListener(pmo, pmo->angle);
	}
}

void PauseAllFX()  { }
void ResumeAllFX() { }

// Your effect reservation, sir...
int ReserveFX(int category) { return -1; }
void UnreserveFX(int handle) { }

// Volume Control
int GetVolume() { return 1; }
void SetVolume(int volume) { }

} // namespace sound


// -AJA- 2005/02/26: table to convert slider position to GAIN.
//       Curve was hand-crafted to give useful distinctions of
//       volume levels at the quiet end.  Entry zero always
//       means total silence (in the table for completeness).
float slider_to_gain[20] =
{
	0.00000, 0.00200, 0.00400, 0.00800, 0.01600,
	0.03196, 0.05620, 0.08886, 0.12894, 0.17584,
	0.22855, 0.28459, 0.34761, 0.41788, 0.49553,
	0.58075, 0.67369, 0.77451, 0.88329, 1.00000
};


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
