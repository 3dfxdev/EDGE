//----------------------------------------------------------------------------
//  EDGE OpenAL sound system
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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

#include "i_defs.h"
#include "i_defs_al.h"

#include "m_argv.h"

#include <stdlib.h>

ALCdevice *alc_dev;
ALCcontext *alc_ctx;

//
// I_StartupSound
//
bool I_StartupSound(void *sysinfo) 
{ 
	const char *p;

	int want_freq;
	int attr_list[4];

	if (nosound)
		return true;
			
	p = M_GetParm("-freq");
	if (p)
		want_freq = atoi(p);
	else
		want_freq = 44100;

    p = M_GetParm("-audiodev");

	if (!p)
	{	
		// Unless "audio device select" is specified, we always default to directsound.
		if(!M_CheckParm("-bestaudio"))
			p = "DirectSound";
	}

	alc_dev = alcOpenDevice((const ALubyte*)p);
	if (alc_dev == NULL)
	{
		I_Printf("I_StartupSound: Couldn't init OpenAL.\n");
		nosound = true;
		return false;
	}

	attr_list[0] = ALC_FREQUENCY;
	attr_list[1] = want_freq;
	attr_list[2] = ALC_INVALID;
	attr_list[3] = ALC_INVALID;

	alc_ctx = alcCreateContext(alc_dev, attr_list);
	if (alc_ctx == NULL)
	{
		I_Printf("I_StartupSound: Couldn't create OpenAL context.\n");
		alcCloseDevice(alc_dev);
		nosound = true;
		return false;
	}

	alcMakeContextCurrent(alc_ctx);

	ALfloat zeroes[] = { 0.0f, 0.0f,  0.0f };
	ALfloat front[]  = { 0.0f, 0.0f,  -1.0f, 0.0f, 1.0f, 0.0f };

	alListenerf(AL_GAIN, 1.0f);
	alListenerfv(AL_POSITION, zeroes);
	alListenerfv(AL_VELOCITY, zeroes);
	alListenerfv(AL_ORIENTATION, front);

	I_Printf("I_StartupSound: initialized OpenAL.\n");

	I_Printf("OpenAL: Version: %s\n", alGetString(AL_VERSION));

	I_Printf("OpenAL: Device: (%s)\n", 
             alcGetString(alc_dev, ALC_DEVICE_SPECIFIER));

	I_Printf("OpenAL: Vendor: %s\n", alGetString(AL_VENDOR));
	I_Printf("OpenAL: Renderer: %s\n", alGetString(AL_RENDERER));

    alDistanceModel(AL_INVERSE_DISTANCE);

	alDopplerFactor(0.0);
	alDopplerVelocity(400.0);

	alGetError(); // Clear any error generated above...

	return true;
}

//
// I_ShutdownSound
//
void I_ShutdownSound(void)
{
	if (nosound)
		return;

	alcMakeContextCurrent(NULL);

	if (alc_ctx)
	{
		alcDestroyContext(alc_ctx);
		alc_ctx = 0;
	}

	if (alc_dev)
	{
		alcCloseDevice(alc_dev);
		alc_dev = 0;
	}
}
