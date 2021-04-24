//----------------------------------------------------------------------------
//  EDGE OpenAL sound system
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
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

#include <AL/al.h>
#include <AL/alc.h>
//#include <AL/alut.h>
#include <AL/alext.h>

#include "system\i_defs.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "e_player.h"
#include "p_mobj.h"
//#include "i_sysinc.h"

#include "dm_state.h"
#include "m_argv.h"
//#include "../m_swap.h"
#include "w_wad.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/types.h>
//#include <sys/ioctl.h>
#include <sys/stat.h>
//#include <sys/time.h>

static ALCdevice *alc_dev;
//static void *alc_ctx;
char* bufferData;
ALvoid* data;
ALCcontext* alc_ctx;
ALsizei size, freq;
ALenum format;
ALuint buffer, source;
ALfloat listenerOri[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
ALboolean loop = AL_FALSE;
ALCenum error;
ALint source_state;

static ALuint *stored_sfx = NULL;
static unsigned int stored_sfx_num = 0;


// Mixing info
#define MIX_CHANNELS  60

static ALuint mix_chan[MIX_CHANNELS];

// Error Description
static char errordesc[256] = "FOO";
static char scratcherror[256];

static void list_audio_devices(const ALCchar* devices)
{
	const ALCchar* device = devices, * next = devices + 1;
	size_t len = 0;

	I_Printf("OpenAL: Devices list:\n");
	I_Printf("----------\n");
	while (device && *device != '\0' && next && *next != '\0') 
	{
		I_Printf("%s\n", device);
		len = strlen(device);
		device += (len + 1);
		next += (len + 2);
	}
	I_Printf("----------\n");
}

static inline ALenum to_al_format(short channels, short samples)
{
	bool stereo = (channels > 1);

	switch (samples) 
	{
	case 16:
		if (stereo)
			return AL_FORMAT_STEREO16;
		else
			return AL_FORMAT_MONO16;
	case 8:
		if (stereo)
			return AL_FORMAT_STEREO8;
		else
			return AL_FORMAT_MONO8;
	default:
		return -1;
	}
}

//
// AL_StartupSound
//
void AL_StartupSound(void)
{
	const ALCchar* defaultDeviceName;
	ALboolean enumeration;
	const char *p;

	int want_freq;
	int want_bits;
	int want_stereo;

	int attr_list[100];

	if (nosound)
		return;

	enumeration = alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");
	if (enumeration == AL_FALSE)
		I_Printf:("OAL: enumeration extension not available!\n");
	
	list_audio_devices(alcGetString(NULL, ALC_DEVICE_SPECIFIER));

	//if (!defaultDeviceName)
		defaultDeviceName = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);


	// clear channels
	int i;
 	for (i=0; i < MIX_CHANNELS; i++)
 		mix_chan[i] = 0;

	p = M_GetParm("-freq");
	if (p)
		want_freq = atoi(p);
	else
		want_freq = 22025;

	want_bits = (M_CheckParm("-sound16") > 0) ? 16 : 8;
	want_stereo = (M_CheckParm("-mono") > 0) ? 0 : 1;

	alc_dev = alcOpenDevice(NULL);
	if (alc_dev == NULL)
	{
		I_Printf("I_StartupSound: Couldn't init OpenAL.\n");
		nosound = true;
		return;
	}
	else
	{
		I_Printf("ALDevice: %s\n", alcGetString(alc_dev, ALC_DEVICE_SPECIFIER));
		nosound = false;
		return;
	}
	alGetError();

	attr_list[0] = ALC_FREQUENCY;
	attr_list[1] = want_freq;
	attr_list[2] = ALC_INVALID;

	alc_ctx = alcCreateContext(alc_dev, attr_list);

	if (alc_ctx == NULL)
	{
		I_Printf("I_StartupSound: Couldn't create OpenAL context.\n");
		alcCloseDevice(alc_dev);
		nosound = true;
		return;
	}

	alcMakeContextCurrent(alc_ctx);

	ALfloat zeroes[] = { 0.0f, 0.0f,  0.0f };
	ALfloat front[]  = { 0.0f, 0.0f,  1.0f, 0.0f, 1.0f, 0.0f };

	alListenerfv(AL_POSITION, zeroes);
	alListenerfv(AL_VELOCITY, zeroes);
	alListenerfv(AL_ORIENTATION, front);
	alGenSources((ALuint)1, &source);

	I_Printf("I_StartupSound: initialized OpenAL.\n");

	I_Printf("OpenAL: Version: %s\n", alGetString(AL_VERSION));
	I_Printf("OpenAL: Device: (%s)\n", alcGetString(alc_dev, ALC_DEVICE_SPECIFIER));
	I_Printf("OpenAL: Vendor: %s\n", alGetString(AL_VENDOR));
	I_Printf("OpenAL: Renderer: %s\n", alGetString(AL_RENDERER));

	return;
}

//### static fixed22_t ComputeDelta(int data_freq, int device_freq)
//### {
//### 	// sound data's frequency close enough ?
//### 	if (data_freq > device_freq - device_freq/100 &&
//### 			data_freq < device_freq + device_freq/100)
//### 	{
//### 		return 1024;
//### 	}
//### 
//### 	return (fixed22_t) floor((float)data_freq * 1024.0f / device_freq);
//### }

//
// I_ShutdownSound
//
void AL_ShutdownSound(void)
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

	nosound = true;
}

//
// I_LoadSfx
//
bool I_LoadSfx(const unsigned char *data, unsigned int length,
    unsigned int freq, unsigned int handle)
{
	unsigned int i;

	if (handle >= stored_sfx_num)
	{
		i = stored_sfx_num;
		stored_sfx_num = handle + 1;

		// !!!! FIXME: use epiarray, not realloc

		stored_sfx = (ALuint *) realloc(stored_sfx, stored_sfx_num * sizeof(ALuint));

		if (! stored_sfx)
		{
			I_Error("Out of memory in sound code.\n");
			return false;
		}

		// clear any new elements
		for (; i < stored_sfx_num; i++)
			stored_sfx[i] = 0;
	}

	SYS_ASSERT(stored_sfx[handle] == 0);

	alGenBuffers(1, stored_sfx + handle);

	alGetError();  // clear any error

	alBufferData(stored_sfx[handle], AL_FORMAT_MONO8, (ALvoid*) data, length, freq);

	ALenum err_num = alGetError();

	if (err_num != AL_NO_ERROR)
	{
		sprintf(errordesc, "I_LoadSfx: couldn't create buffer: %s\n",
			alGetString(err_num));
		return false;
	}
	
	return true;
}

//
// I_UnloadSfx
//
bool I_UnloadSfx(unsigned int handle)
{
	//SYS_ASSERT(handle < stored_sfx_num,
	//		("I_UnloadSfx: %d out of range", handle));

	//SYS_ASSERT(stored_sfx[handle] > 0, ("I_UnloadSfx: Zero sample"));

	alDeleteBuffers(1, stored_sfx + handle);

	return true;
}

//
// I_SoundPlayback
//
int I_SoundPlayback(unsigned int soundid, int pan, int vol, bool looping)
{
	if (nosound)
		return true;
	
	int i;
	
	for (i=0; i < MIX_CHANNELS; i++)
		if (mix_chan[i] == 0)
			break;
	
	if (i >= MIX_CHANNELS)
		return -1;

	alGenSources(1, mix_chan + i);

if (mix_chan[i] == 0) return -1;
	
	//!!! alSourcefv(mix_chan[i], src_pos);
	alSourcef(mix_chan[i], AL_GAIN, vol / 255.0);
	alSourcei(mix_chan[i], AL_SOURCE_RELATIVE, AL_TRUE); //!!!
	alSourcei(mix_chan[i], AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
	alSourcei(mix_chan[i], AL_BUFFER, stored_sfx[soundid]);

	alSourcePlay(mix_chan[i]);

	return i;
}

//
// I_SoundKill
//
bool I_SoundKill(unsigned int chanid)
{
	if (nosound)
		return true;

 	if (mix_chan[chanid] == 0)
 	{
 		sprintf(errordesc, "I_SoundKill: channel not playing.\n");
 		return false;
 	}

#if 1
	// allow looping sounds to stop
	alSourcei(mix_chan[chanid], AL_LOOPING, AL_FALSE);
#else
	alSourceStop(mix_chan[chanid]);
	
	alDeleteSources(1, mix_chan + chanid);

	mix_chan[chanid] = 0;
#endif

	return true;
}

//
// I_SoundCheck
//
bool I_SoundCheck(unsigned int chanid)
{
	return mix_chan[chanid] > 0;
}

//
// I_SoundAlter
//
bool I_SoundAlter(unsigned int chanid, int pan, int vol)
{
	if (nosound)
		return true;

 	if (mix_chan[chanid] == 0)
 	{
 		sprintf(errordesc, "I_SoundAlter: channel not playing.\n");
 		return false;
 	}

	alSourcef(mix_chan[chanid], AL_GAIN, vol / 255.0);

	return true;
}

//
// I_SoundPause
//
bool I_SoundPause(unsigned int chanid)
{
	if (nosound)
		return true;

 	if (mix_chan[chanid] == 0)
 	{
 		sprintf(errordesc, "I_SoundPause: channel not playing.\n");
 		return false;
 	}

	alSourcePause(mix_chan[chanid]);

	return true;
}

//
// I_SoundStopLooping
//
bool I_SoundStopLooping(unsigned int chanid)
{
//### 	if (nosound)
//### 		return true;
//### 
//### 	mix_chan[chanid].looping = false;
	return true;
}


//
// I_SoundResume
//
bool I_SoundResume(unsigned int chanid)
{
	if (nosound)
		return true;

 	if (mix_chan[chanid] == 0)
 	{
 		sprintf(errordesc, "I_SoundPause: channel not playing.\n");
 		return false;
 	}

	alSourcePlay(mix_chan[chanid]);

	return true;
}

//### static void MixChannel(mix_channel_t *chan, int want)
//### {
//### 	int i;
//### 
//### 	int *dest_L = mix_buffer_L;
//### 	int *dest_R = mix_buffer_R;
//### 
//### 	char *src_L = chan->sound->data_L;
//### 
//### 	while (want > 0)
//### 	{
//### 		int count = MIN(chan->sound->length - (chan->offset >> 10), want);
//### 
//### 		DEV_ASSERT2(count > 0);
//### 
//### 		for (i=0; i < count; i++)
//### 		{
//### 			signed char src_sample = src_L[chan->offset >> 10];
//### 
//### 			*dest_L++ += src_sample * chan->volume_L;
//### 			*dest_R++ += src_sample * chan->volume_R;
//### 
//### 			chan->offset += chan->sound->delta;
//### 		}
//### 
//### 		want -= count;
//### 
//### 		// return if sound hasn't finished yet
//### 		if ((chan->offset >> 10) < chan->sound->length)
//### 			return;
//### 
//### 		if (! chan->looping)
//### 		{
//### 			chan->priority = PRI_FINISHED;
//### 			return;
//### 		}
//### 
//### 		// loop back to beginning
//### 		src_L = chan->sound->data_L;
//### 		chan->offset = 0;
//### 	}
//### }

//### static INLINE unsigned char ClipSampleS8(int value)
//### {
//### 	if (value < -0x7F) return 0x81;
//### 	if (value >  0x7F) return 0x7F;
//### 
//### 	return (unsigned char) value;
//### }
//### 
//### static INLINE unsigned char ClipSampleU8(int value)
//### {
//### 	value += 0x80;
//### 
//### 	if (value < 0)    return 0;
//### 	if (value > 0xFF) return 0xFF;
//### 
//### 	return (unsigned char) value;
//### }
//### 
//### static INLINE unsigned short ClipSampleS16(int value)
//### {
//### 	if (value < -0x7FFF) return 0x8001;
//### 	if (value >  0x7FFF) return 0x7FFF;
//### 
//### 	return (unsigned short) value;
//### }
//### 
//### static INLINE unsigned short ClipSampleU16(int value)
//### {
//### 	value += 0x8000;
//### 
//### 	if (value < 0)      return 0;
//### 	if (value > 0xFFFF) return 0xFFFF;
//### 
//### 	return (unsigned short) value;
//### }
//### 
//### static void BlitToSoundDevice(unsigned char *dest, int pairs)
//### {
//### 	int i;
//### 
//### 	if (dev_bits == 8)
//### 	{
//### 		if (mydev.channels == 2)
//### 		{
//### 			for (i=0; i < pairs; i++)
//### 			{
//### 				*dest++ = ClipSampleU8(mix_buffer_L[i] >> 16);
//### 				*dest++ = ClipSampleU8(mix_buffer_R[i] >> 16);
//### 			}
//### 		}
//### 		else
//### 		{
//### 			for (i=0; i < pairs; i++)
//### 			{
//### 				*dest++ = ClipSampleU8(mix_buffer_L[i] >> 16);
//### 			}
//### 		}
//### 	}
//### 	else
//### 	{
//### 		DEV_ASSERT2(dev_bits == 16);
//### 
//### 		unsigned short *dest2 = (unsigned short *) dest;
//### 
//### 		if (mydev.channels == 2)
//### 		{
//### 			for (i=0; i < pairs; i++)
//### 			{
//### 				*dest2++ = ClipSampleS16(mix_buffer_L[i] >> 8);
//### 				*dest2++ = ClipSampleS16(mix_buffer_R[i] >> 8);
//### 			}
//### 		}
//### 		else
//### 		{
//### 			for (i=0; i < pairs; i++)
//### 			{
//### 				*dest2++ = ClipSampleS16(mix_buffer_L[i] >> 8);
//### 			}
//### 		}
//### 	}
//### }

//### //
//### // InternalSoundFiller
//### //
//### void InternalSoundFiller(void *udata, Uint8 *stream, int len)
//### {
//### 	int i;
//### 	int pairs = len / dev_bytes_per_sample;
//### 
//### 	// check that we're not getting too much data
//### 	DEV_ASSERT2(pairs <= dev_frag_pairs);
//### 
//### 	if (nosound || pairs <= 0)
//### 		return;
//### 
//### 	// clear mixer buffer
//### 	memset(mix_buffer_L, 0, sizeof(int) * pairs);
//### 	memset(mix_buffer_R, 0, sizeof(int) * pairs);
//### 
//### 	// add each channel
//### 	for (i=0; i < MIX_CHANNELS; i++)
//### 	{
//### 		if (mix_chan[i].priority >= 0)
//### 			MixChannel(&mix_chan[i], pairs);
//### 	}
//### 
//### 	BlitToSoundDevice(stream, pairs);
//### }

//
// I_SoundTicker
//
void I_SoundTicker(void)
{
	player_t* player;
	if (nosound)
		return;

	if (gamestate == GS_LEVEL && consoleplayer1 && player->mo)
	{
		ALfloat listen_pos[3];

		listen_pos[0] = player->mo->x;
		listen_pos[1] = player->mo->y;
		listen_pos[2] = player->viewz;

		alListenerfv(AL_POSITION, listen_pos);
	}

#if 1
	// find sources that have stopped playing and kill 'em

	int i;

	for (i=0; i < MIX_CHANNELS; i++)
	{
		if (mix_chan[i] == 0)
			continue;

		ALenum state;

		alGetSourcei(mix_chan[i], AL_SOURCE_STATE, &state);

		if (state == AL_PLAYING || state == AL_PAUSED)
			continue;

		alSourceStop(mix_chan[i]);
		
		alDeleteSources(1, mix_chan + i);

		mix_chan[i] = 0;
	}
#endif
}

//
// I_SoundReturnError
//
const char *AL_SoundReturnError(void)
{
	memcpy(scratcherror, errordesc, sizeof(scratcherror));
	memset(errordesc, '\0', sizeof(errordesc));

	return scratcherror;
}

