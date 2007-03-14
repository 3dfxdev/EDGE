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
#include "m_swap.h"
#include "w_wad.h"

#include "s_sound.h"
#include "s_cache.h"


// #include "r_defs.h"  // sec_sfxorig_t


// If true, sound system is off/not working. Changed to false if sound init ok.
bool nosound = false;

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

/* See m_option.cc for corresponding menu items */
static const int sample_rates[4]   = { 11025, 16000, 22050, 44100 };
static const int sample_bits[2]    = { 8, 16 };
static const int channel_counts[4] = { 16, 32, 64, 128 };


static SDL_AudioSpec mydev;

static int dev_bits;
static int dev_bytes_per_sample;
static int dev_frag_pairs;


#define PRI_NOSOUND   -1
#define PRI_FINISHED  -2

// Error Description
static char errordesc[256] = "FOO";
static char scratcherror[256];

//
// I_StartupSound
//
bool I_StartupSound(void *sysinfo)
{
	int i;
	const char *p;
	SDL_AudioSpec firstdev;

	int want_freq;
	int want_bits;
	int want_stereo;

	if (nosound)
		return true;

	// clear channels
	for (i=0; i < MIX_CHANNELS; i++)
		mix_chan[i].priority = PRI_NOSOUND;

	p = M_GetParm("-freq");
	if (p)
		want_freq = atoi(p);
	else
		want_freq = 11025;

	want_bits = (M_CheckParm("-sound16") > 0) ? 16 : 8;
	want_stereo = (M_CheckParm("-mono") > 0) ? 0 : 1;

	firstdev.freq = want_freq;
	firstdev.format = (want_freq < 12) ? AUDIO_U8 : AUDIO_S16SYS;
	firstdev.channels = want_stereo ? 2 : 1;
	firstdev.samples = 512;
	firstdev.callback = InternalSoundFiller;

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
		I_Printf("I_StartupSound: unsupported format ! (%d)\n", mydev.format);
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
	DEV_ASSERT2(dev_bytes_per_sample > 0);

	dev_frag_pairs = mydev.size / dev_bytes_per_sample;
	DEV_ASSERT2(dev_frag_pairs > 0);

	// allocate mixing buffers
	mix_buffer_L = (int *) malloc(dev_frag_pairs * sizeof(int));

	if (! mix_buffer_L)
	{
		SDL_CloseAudio();
		I_Error("I_StartupSound: Out of memory.\n");
		return false;
	}

	mix_buffer_R = (int *) malloc(dev_frag_pairs * sizeof(int));

	if (! mix_buffer_R)
	{
		SDL_CloseAudio();
		I_Error("I_StartupSound: Out of memory.\n");
		return false;
	}

	// okidoke, start things rolling
	SDL_PauseAudio(0);

	return true;
}

//
// I_ShutdownSound
//
void I_ShutdownSound(void)
{
	if (nosound)
		return;

	SDL_CloseAudio();

	if (mix_buffer_L)
	{
		free(mix_buffer_L);
		mix_buffer_L = NULL;
	}

	if (mix_buffer_R)
	{
		free(mix_buffer_R);
		mix_buffer_L = NULL;
	}

	// FIXME: free sounds

	nosound = true;
}

//
// I_LoadSfx
//
bool I_LoadSfx(const unsigned char *data, unsigned int length,
    unsigned int freq, unsigned int handle)
{
	SDL_LockAudio();
	SDL_UnlockAudio();
}

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


// Init/Shutdown
void Init(void) { }
void Shutdown(void) { }

void ClearAllFX(void) { }

void StartFX(sfx_t *sfx, int category, epi::vec3_c pos, int flags) { }
void StartFX(sfx_t *sfx, int category, mobj_t *mo, int flags) { }
void StartFX(sfx_t *sfx, int category, sec_sfxorig_t *orig, int flags) { }
void StartFX(sfx_t *sfx, int category, int flags) { }

void StopFX(int handle) { }
void StopFX(sec_sfxorig_t *orig) { }
void StopFX(mobj_t *mo) { }

void StopLoopingFX(int handle) { }
void StopLoopingFX(sec_sfxorig_t *orig) { }
void StopLoopingFX(mobj_t *mo) { }

// Ticker
void Ticker() { }

void PauseAllFX()  { }
void ResumeAllFX() { }

bool IsFXPlaying(int handle) { return false; } 
bool IsFXPlaying(sec_sfxorig_t *orig) { return false; } 
bool IsFXPlaying(mobj_t *mo) { return false; } 

// Your effect reservation, sir...
int ReserveFX(int category) { return -1; }
void UnreserveFX(int handle) { }

// Playsim Object <-> Effect Linkage
void UnlinkFX(mobj_t *mo) { }
void UnlinkFX(sec_sfxorig_t *orig) { }

// Volume Control
int GetVolume() { return 1; }
void SetVolume(int volume) { }

} // namespace sound

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
