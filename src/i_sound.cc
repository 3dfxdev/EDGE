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
// I_SoundReturnError
//
const char *I_SoundReturnError(void)
{
	memcpy(scratcherror, errordesc, sizeof(scratcherror));
	memset(errordesc, '\0', sizeof(errordesc));

	return scratcherror;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
