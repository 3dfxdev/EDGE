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
#include "i_sdlinc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/time.h>
///--- #include <sys/stat.h>
///--- #include <sys/ioctl.h>

#include "m_argv.h"
#include "m_misc.h"
#include "m_random.h"
#include "m_swap.h"
#include "w_wad.h"

#include "s_sound.h"
#include "s_cache.h"
#include "s_blit.h"


// If true, sound system is off/not working. Changed to false if sound init ok.
bool nosound = false;

/* See m_option.cc for corresponding menu items */
static const int sample_rates[5] = { 11025, 16000, 22050, 32000, 44100 };
static const int sample_bits[2]  = { 8, 16 };


static SDL_AudioSpec mydev;

int dev_freq;
int dev_bits;
int dev_bytes_per_sample;
int dev_frag_pairs;
bool dev_signed;  // S8 vs U8, S16 vs U16
bool dev_stereo;


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

	int want_freq = sample_rates[var_sample_rate];
	int want_bits = sample_bits [var_sound_bits];
	bool want_stereo = (var_sound_stereo >= 1);

	p = M_GetParm("-freq");
	if (p)
		want_freq = atoi(p);

	if (M_CheckParm("-sound16") > 0)
		want_bits = 16;

	if (M_CheckParm("-mono")   > 0) want_stereo = false;
	if (M_CheckParm("-stereo") > 0) want_stereo = true;

	SDL_AudioSpec firstdev;

	firstdev.freq     = want_freq;
	firstdev.format   = (want_bits < 12) ? AUDIO_U8 : AUDIO_S16SYS;
	firstdev.channels = want_stereo ? 2 : 1;
	firstdev.samples  = 512;
	firstdev.callback = SoundFill_Callback;

	if (SDL_OpenAudio(&firstdev, &mydev) < 0)
	{
		I_Printf("I_StartupSound: Couldn't open sound: %s\n", SDL_GetError());
		nosound = true;
		return false;
	}

#if 0
	// get round SDL's signal handlers
	signal(SIGFPE,  SIG_DFL);
	signal(SIGSEGV, SIG_DFL);
#endif

	// check stuff...

	switch (mydev.format)
	{
		case AUDIO_S16SYS: dev_bits=16; dev_signed=true;  break;
		case AUDIO_U16SYS: dev_bits=16; dev_signed=false; break;

		case AUDIO_S8: dev_bits=8; dev_signed=true;  break;
		case AUDIO_U8: dev_bits=8; dev_signed=false; break;

	    default:				   
			I_Printf("I_StartupSound: unsupported format: %d\n", mydev.format);
			SDL_CloseAudio();

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

	dev_bytes_per_sample = (mydev.channels) * (dev_bits / 8);
	dev_frag_pairs = mydev.size / dev_bytes_per_sample;

	SYS_ASSERT(dev_bytes_per_sample > 0);
	SYS_ASSERT(dev_frag_pairs > 0);

	dev_freq   = mydev.freq;
	dev_stereo = (mydev.channels == 2);

	// update Sound Options menu
	if (dev_bits != sample_bits[var_sound_bits])
		var_sound_bits = (dev_bits >= 16) ? 1 : 0;

	if (dev_stereo != (var_sound_stereo >= 1))
		var_sound_stereo = dev_stereo ? 1 : 0;

	if (dev_freq != sample_rates[var_sample_rate])
	{
		if (dev_freq >= 38000)
			var_sample_rate = 4; // 44 Khz
		else if (dev_freq >= 27000)
			var_sample_rate = 3; // 32 Khz
		else if (dev_freq >= 19000)
			var_sample_rate = 2; // 22 Khz
		else if (dev_freq >= 13500)
			var_sample_rate = 1; // 16 Khz
		else
			var_sample_rate = 0; // 11 Khz
	}

	// display some useful stuff
	I_Printf("I_StartupSound: SDL Started: %d Hz, %d bits, %s\n",
			dev_freq, dev_bits, dev_stereo ? "Stereo" : "Mono");

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
