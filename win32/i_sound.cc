//----------------------------------------------------------------------------
//  EDGE WIN32 DirectSound Handler Functions 
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2001 The Edge Team.
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

#include "..\i_defs.h"
#include "i_sysinc.h"

#define _PAN_LEFT   0
#define _PAN_CENTRE 128
#define _PAN_RIGHT  255

#define _VOL_MAX    256

#define _ABSOLUTE(x) if (x>0) x=0-x 

// Channel flags
typedef enum
{
	_DXS_LOOPING  = 0x01, // Is looping
	_DXS_NOBUFFER = 0x02, // Channel has no DirectSound buffer
	_DXS_PAUSED   = 0x04, // Is paused
	_DXS_PLAYING  = 0x08, // Is playing
}
_dxsound_e;

// List and data struct of sfx for playback
typedef struct storesfx_s 
{
	unsigned int length;
	unsigned int samplerate;
	unsigned char *data;
}
storesfx_t;

// Channel for internal sound control
typedef struct channel_s 
{
	LPDIRECTSOUNDBUFFER buffer; // DirectSound buffer
	int flags;                  // Behaviour Flags
	unsigned int sfxhandle;     // Sound Effect Handle
	DWORD pausedpos;            // Position for restart
}
channel_t;

#define MAX_CHANNELS  128

//
// Short Buffers:
//   These are inited to a certain size and used for
//   relatively short sounds that do not repeat. These are 
//   used to prevent the need to create new buffers for
//   every sound.
//
static const unsigned int SHORTBUFFLEN = 22050;

// ==== PRIVATE VARIABLES ====
// HWND handle
static HWND winhandle;

// Direct Sound Interfaces
static LPDIRECTSOUND soundobject;
static LPDIRECTSOUNDBUFFER primarybuffer;

// Stored Sound Effects
static storesfx_t **storedsfx = NULL;
static unsigned int numstoredsfx = 0;

// Channels
static channel_t **channels;
static unsigned int numchannels;

// Error Description
static char errordesc[256];
static char scratcherror[256];

// System Started?
static boolean_t inited;

// Difference between MAX and MIN vol
static int voldiff;                    

// ===========================

// ==== PRIVATE FUNCTIONS ====
static LPDIRECTSOUNDBUFFER CreateBuffer(int length);
static channel_t *AddChannel(int *index);
static channel_t *GetChannel(int *channelid, storesfx_t *sfx);
// ===========================

// ================ INTERNALS =================

//
// CalculatePanning
//
// This is a conversion of seperation value from
// the range 0(left)-128(mid)-255(right) to the
// DirectSound Values
//
static LONG CalculatePanning(int pan)
{
	int pandiff;
	int syspandiff;

	if (pan>_PAN_LEFT && pan<_PAN_CENTRE)
	{
		pandiff = _PAN_LEFT - _PAN_CENTRE;
		_ABSOLUTE(pandiff);

		syspandiff = DSBPAN_LEFT - DSBPAN_CENTER;
		_ABSOLUTE(syspandiff);

		pan = pan - _PAN_LEFT;

		pan *= syspandiff;

		if (pan)
			pan /= pandiff;

		pan = DSBPAN_LEFT + pan; 

		return (LONG)pan;
	}

	if (pan>_PAN_CENTRE && pan<_PAN_RIGHT)
	{
		pandiff = _PAN_CENTRE - _PAN_RIGHT;
		_ABSOLUTE(pandiff);

		syspandiff = DSBPAN_CENTER - DSBPAN_RIGHT;
		_ABSOLUTE(syspandiff);

		pan = pan - _PAN_CENTRE;

		pan *= syspandiff;

		if (pan)
			pan /= pandiff;

		pan = DSBPAN_CENTER + pan;

		return (LONG)pan;
	}

	if (pan==_PAN_LEFT)
		return (LONG)DSBPAN_LEFT;

	if (pan==_PAN_RIGHT)
		return (LONG)DSBPAN_RIGHT;

	return (LONG)DSBPAN_CENTER;
}

//
// CalculateVolume
//
// This is a conversion of seperation value from
// the range 0(quietest)-255(loudest) to the
// DirectSound Values. Due to DirectSound handling
// the calculation makes DirectSound half-vol the
// quietest (quality reasons).
//
static LONG CalculateVolume(int vol)
{
	if (vol)
	{
		vol *= voldiff;
		vol /= _VOL_MAX;
	}

	//
	// This should not be neccessary, but for some reason volume below half
	// of volume is inaudible
	//
	vol = DSBVOLUME_MIN + (voldiff/2) + (vol/2);

	return vol;
}

//
// CreateBuffer
//
// Creates a DirectSound Secondary Buffer of the correct length
//
LPDIRECTSOUNDBUFFER CreateBuffer(int length)
{
	LPDIRECTSOUNDBUFFER buffer;
	DSBUFFERDESC bufferdesc;
	PCMWAVEFORMAT pcmwf;

	// Set up wave format structure.
	memset(&pcmwf, 0, sizeof(PCMWAVEFORMAT));
	pcmwf.wf.wFormatTag         = WAVE_FORMAT_PCM;      
	pcmwf.wf.nChannels          = 1;
	pcmwf.wf.nSamplesPerSec     = 11025;
	pcmwf.wf.nBlockAlign        = 1; // ?
	pcmwf.wf.nAvgBytesPerSec    = 11025*1*1;
	pcmwf.wBitsPerSample        = (WORD)8;

	// Set up DSBUFFERDESC structure.
	memset(&bufferdesc, 0, sizeof(DSBUFFERDESC)); 
	bufferdesc.dwSize        = sizeof(DSBUFFERDESC);
	bufferdesc.dwFlags       = DSBCAPS_CTRLVOLUME|DSBCAPS_CTRLFREQUENCY|DSBCAPS_CTRLPAN|DSBCAPS_GETCURRENTPOSITION2;
	bufferdesc.dwBufferBytes = length;
	bufferdesc.lpwfxFormat   = (LPWAVEFORMATEX)&pcmwf;  

	if (soundobject->CreateSoundBuffer(&bufferdesc, &buffer, NULL) != DS_OK)
		return NULL;

	return buffer;
}

//
// AddChannel
//
// Add a new a new channel to the channel list
//
channel_t *AddChannel(int *index)
{
	channel_t *result;

	if (numchannels >= MAX_CHANNELS)
	{
		strcpy(errordesc, "AddChannel: no free channels.\n");
		return NULL;
	}

	result = new channel_t;
	if (!result)
	{
		I_Error("AddChannel: Out of memory\n");
		return NULL;
	}

	channels[numchannels] = result;

	*index = numchannels;
	numchannels++;

	result->flags = _DXS_NOBUFFER;
	return result;
}

//
// GetChannel
//
// Returns a channel that will be destroyed after use
//
channel_t *GetChannel(int *channelid, storesfx_t *sfx)
{
	int i, index;
	channel_t *channel;

	// Search out spare channel
	index = -1;

	for (i = 0; i < (int)numchannels; i++)
	{
		if (channels[i]->flags & _DXS_NOBUFFER)
		{
			index = i;
			break;
		}
	}

	// if all spaces used, knock out a unused reusable
	if (index < 0)
	{
		// Search out spare channel
		for (i = 0; i < (int)numchannels; i++)
		{
			if (!(channels[i]->flags & _DXS_PLAYING) && 
				(channels[i]->flags & _DXS_NOBUFFER)) // -ACB- 2001/01/01 Don't use _DXS_REUSABLE
			{
				index = i;
				break;
			}
		}

		//
		// If we have found a free reusable channel, destroy the current
		// buffer and reset the flags.
		//
		if (index >= 0)
		{
			channel = channels[i];
			channel->buffer->Release();
			channel->buffer = NULL;
			channel->flags = _DXS_NOBUFFER;
		}
		else // None free? lets create one.
		{
			channel = AddChannel(&index);

			// if no channel just return NULL: AddChannel would of set the
			// error message
			if (!channel)
				return NULL;
		}
	}

	channel = channels[index];
	*channelid = index;

	channel->buffer = CreateBuffer(sfx->length);

	// CreateBuffer failure
	if (!channel->buffer)
	{
		strcpy(errordesc, "GetChannel: CreateBuffer() Failed");
		return NULL;
	}

	channel->flags = 0; 
	return channel;
}

// ============= END OF INTERNALS =============

//
// I_StartupSound
//
// Exactly what it says on the label
//
// -ACB- 1999/10/06
// 
boolean_t I_StartupSound(void *sysinfo)
{
	WAVEFORMATEX format;
	DSBUFFERDESC bufferdesc;
	HWND window;
	HRESULT result;

	winhandle     = NULL;
	soundobject   = NULL;
	primarybuffer = NULL;
	channels      = NULL;
	numchannels   = 0;
	inited        = false;
	memset(errordesc, '\0', 256);

	window = *(HWND*)sysinfo;

	// Create DirectSound     
	if (DirectSoundCreate(NULL, &soundobject, NULL) != DS_OK)
	{
		strcpy(errordesc, "I_StartupSound: Unable to Create Direct Sound Object");
		return false;
	}

	// Set co-op level 
	if (soundobject->SetCooperativeLevel(window, DSSCL_EXCLUSIVE) != DS_OK)
	{
		strcpy(errordesc, "I_StartupSound: SetCooperativeLevel Failed");
		soundobject->Release();
		soundobject = NULL;
		return false;
	}

	// Setup Primary Buffer
	memset(&bufferdesc, 0, sizeof(DSBUFFERDESC));
	bufferdesc.dwSize = sizeof(DSBUFFERDESC);
	bufferdesc.dwFlags = DSBCAPS_PRIMARYBUFFER;

	// Create Primary
	result = soundobject->CreateSoundBuffer(&bufferdesc, &primarybuffer, NULL);
	if (result != DS_OK)
	{
		strcpy(errordesc, "I_StartupSound: CreateSoundBuffer Failed");
		soundobject->Release();
		soundobject = NULL;
		return false;
	}

	// Set primary buffer format
	memset(&format, 0, sizeof(WAVEFORMATEX)); 
	format.wFormatTag = WAVE_FORMAT_PCM;      
	format.nChannels = 2; 
	format.nSamplesPerSec = 11025; 
	format.wBitsPerSample = 8; 
	format.nBlockAlign = 2;
	format.nAvgBytesPerSec = 11025*2*1;
	format.cbSize = 0;

	if (primarybuffer->SetFormat(&format) != DS_OK)
	{
		strcpy(errordesc, "I_StartupSound: primarybuffer->SetFormat Failed");
		soundobject->Release();
		soundobject = NULL;
		return false;
	}

	// create channel array, pointers set to zero
	channels = new  channel_t*[MAX_CHANNELS];
	if (channels == NULL)
		I_Error("I_StartupSound: Out of memory\n");

	// Calc Volume Range
	voldiff = DSBVOLUME_MAX - DSBVOLUME_MIN;
	if (voldiff<0)
		voldiff = 0 - voldiff;

	winhandle = *(HWND*)sysinfo;
	inited = true;

	I_Printf("I_StartupSound: Initialised OK.\n");
	return true;
}

//
// I_LoadSfx
//
// Loads a sound effect from data.
//
// -ACB- 1999/10/05 Written
//
boolean_t I_LoadSfx(const unsigned char *data, unsigned int length, unsigned int freq, unsigned int handle)
{
	unsigned int i;
	storesfx_t **oldlist;
	byte *snddata;

	DEV_ASSERT2(data);

	if (handle >= numstoredsfx)
	{
		i = numstoredsfx;
		numstoredsfx = handle + 1;

		if (storedsfx)
		{
			oldlist = storedsfx;
			storedsfx = new storesfx_t*[numstoredsfx];
			if (!storedsfx)
			{
				I_Error("I_LoadSFX: Alloc failed on first storedsfx_t**\n");
				return false;
			}
			memcpy(storedsfx, oldlist, sizeof(storesfx_t*)*i);
			delete [] oldlist;
		}
		else
		{
			storedsfx = new storesfx_t*[numstoredsfx];
			if (!storedsfx)
			{
				I_Error("I_LoadSFX: Alloc failed on first storedsfx_t**\n");
				return false;
			}
		}

		// clear any new elements
		for (; i < numstoredsfx; i++)
			storedsfx[i] = NULL;
	}

	storedsfx[handle] = new storesfx_t;
	if (!storedsfx[handle])
	{
		I_Error("I_LoadSFX: Unable to malloc for stored sound effect\n");
		return false;
	}

	// -ACB- 2000/05/17 Copy data into storedsfx structure
	snddata = new byte[length];
	if (!snddata)
	{
		I_Error("I_LoadSFX: Unable to malloc for sound effect data\n");
		return false;
	}

	memcpy(snddata, data, sizeof(byte)*length);

	storedsfx[handle]->data       = snddata;
	storedsfx[handle]->length     = length;
	storedsfx[handle]->samplerate = freq;

	return true;
}

//
// I_UnloadSfx
//
// -ACB- 2000/02/19 Written
// -ACB- 2000/05/17 Free Sound Effects Data
//
boolean_t I_UnloadSfx(unsigned int handle)
{
	unsigned int i;

	DEV_ASSERT2(handle < numstoredsfx);

	// Check handle
	DEV_ASSERT2(storedsfx[handle] != NULL);

	// Kill playing sound effects
	for (i = 0; i < numchannels; i++)
	{
		// A paused sound effect is in use...
		if (channels[i]->sfxhandle == handle &&
			(channels[i]->flags&_DXS_PLAYING || channels[i]->flags&_DXS_PAUSED))
		{
			I_SoundKill(i);
		}
	}

	// Check handle data
	DEV_ASSERT2(storedsfx[handle]->data != NULL);

	delete [] storedsfx[handle]->data;
	delete storedsfx[handle];
	storedsfx[handle] = NULL;
	return true;
}

//
// I_SoundAlter
//
// Alters the parameters of a currently playing sound
//
// -ACB- 1999/10/06
//
boolean_t I_SoundAlter(unsigned int chanid, int pan, int vol)
{
	channel_t *channel;
	LONG actualpan;
	LONG actualvol;

	DEV_ASSERT2(chanid < numchannels);
	DEV_ASSERT2(0 <= pan && pan <= 255);
	DEV_ASSERT2(0 <= vol && vol <= 255);

	channel = channels[chanid];

	actualpan = CalculatePanning(pan);
	actualvol = CalculateVolume(vol);

	if (!(channel->flags & _DXS_PLAYING))
	{
		strcpy(errordesc, "I_SoundAlter: Channel is not playing or active");
		return false;
	}

	if (channel->buffer->SetVolume(actualvol) != DS_OK)
	{
		strcpy(errordesc, "I_SoundAlter: Unable to set DirectSound buffer volume");
		return false;
	}

	if (channel->buffer->SetPan(actualpan) != DS_OK)
	{
		strcpy(errordesc, "I_SoundAlter: Unable to set DirectSound buffer pan");
		return false;
	}

	return true;
}

//
// I_SoundKill
//
// This kills a sound. Stops the sound playing and destroys
// the buffer if necessary.
//
boolean_t I_SoundKill(unsigned int chanid)
{
	channel_t *channel;

	DEV_ASSERT2(chanid < numchannels);

	channel = channels[chanid];

	if (channel->flags & _DXS_NOBUFFER)
	{
		strcpy(errordesc, "I_SoundKill: Channel has no DSBuffer");
		return false;
	}

	if (channel->flags & _DXS_PLAYING)
	{
		if (channel->buffer->Stop() != DS_OK)
		{
			strcpy(errordesc, "I_SoundKill: Unable to stop channel playback");
			return false;
		}
	}

	channel->buffer->Release();
	channel->buffer    = NULL;
	channel->flags     = _DXS_NOBUFFER;
	channel->sfxhandle = -1;

	return true;
}

//
// I_SoundPlayback
//
// Sets a channel up for playback with given soundID.
// Returns -1 on failure.
//
// -ACB- 1999/10/05 Written.
//
int I_SoundPlayback(unsigned int handle, int pan, int vol, boolean_t looping)
{
	storesfx_t *sfx;
	channel_t *channel;
	LONG actualvol;
	LONG actualpan;
	LPVOID buff1;
	LPVOID buff2;
	DWORD len1;
	DWORD len2;
	int channelid;

	if (!inited)
	{
		I_Error("I_SoundPlayback: Sound System Not Inited\n");
		return -1;
	}

	DEV_ASSERT2(0 <= pan && pan <= 255);
	DEV_ASSERT2(0 <= vol && vol <= 255);

	// check sound ID
	DEV_ASSERT2(handle < numstoredsfx);

	actualvol = CalculateVolume(vol);
	actualpan = CalculatePanning(pan);
	sfx = storedsfx[handle];

	channel = GetChannel(&channelid, sfx);
	if (!channel)
	{
		return -1;
	}

	// Set the buffer position to zero
	channel->buffer->SetCurrentPosition(0);

	// Lock buffer - we need to copy the sound data into the buffer
	if (channel->buffer->Lock(0, sfx->length, &buff1, &len1, &buff2, &len2, DSBLOCK_FROMWRITECURSOR) != DS_OK)
	{
		strcpy(errordesc, "I_SoundPlayback: Unable to lock DirectSound buffer");
		return -1;
	}

	// Copy blank data into the buffer
	memset(buff1, 0x7F, len1);

	// Copy blank data into the buffer part II (if there is one)
	if (buff2 && len2)
		memset(buff2, 0x7F, len2);

	// Copy the data across
	if (len1 >= sfx->length)
		memcpy(buff1, sfx->data, sfx->length);
	else
		memcpy(buff1, sfx->data, len1);

	// Unlock the data buffer
	if (channel->buffer->Unlock(buff1, len1, buff2, len2) != DS_OK)
	{
		strcpy(errordesc, "I_SoundPlayback: Unable to unlock DirectSound buffer");
		return -1;
	}

	// Set the volume
	if (channel->buffer->SetVolume(actualvol) != DS_OK)
	{
		strcpy(errordesc, "I_SoundPlayback: Unable to set DirectSound buffer volume");
		return -1;
	}

	// Set the panning value 
	if (channel->buffer->SetPan(actualpan) != DS_OK)
	{
		strcpy(errordesc, "I_SoundPlayback: Unable to set DirectSound buffer pan");
		return -1;
	}

	// Frequency
	if (channel->buffer->SetFrequency(sfx->samplerate) != DS_OK)
	{
		strcpy(errordesc, "I_SoundPlayback: Unable to set DirectSound buffer freq");
		return -1;
	}

	if (channel->buffer->Play(0, 0, (looping?DSBPLAY_LOOPING:0)) != DS_OK)
	{
		strcpy(errordesc, "I_SoundPlayback: Unable to play sound");
		return -1;
	}

	if (looping)
		channel->flags |= _DXS_LOOPING;

	channel->flags |= _DXS_PLAYING;
	channel->sfxhandle = handle;

	return channelid;
}

//
// I_SoundCheck
//
// Returns if a channel is currently active
//
// -ACB- 1999/10/06 Written
//
boolean_t I_SoundCheck(unsigned int chanid)
{
	if (chanid >= numchannels)
		return false;

	return ((channels[chanid]->flags & _DXS_PLAYING)?true:false);
}

//
// I_SoundPause
//
boolean_t I_SoundPause(unsigned int chanid)
{
	channel_t *channel;
	DWORD curpos;
	DWORD status;

	if (!inited)
	{
		I_Error("I_SoundPause: Sound System Not Inited\n");
		return false;
	}

	DEV_ASSERT2(chanid < numchannels);

	channel = channels[chanid];

	if (!(channel->flags & _DXS_PLAYING))
	{
		strcpy(errordesc, "I_SoundPause: Channel is not playing or active");
		return false;
	}

	channel->buffer->GetStatus(&status);

	// Kill any lost buffer
	if (status & DSBSTATUS_BUFFERLOST)
	{
		channel->buffer->Release();
		channel->buffer = NULL;
		channel->flags = _DXS_NOBUFFER;
		strcpy(errordesc, "I_SoundPause: Channel's DirectSound Buffer has been lost.");
		return false;
	}

	if (channel->buffer->GetCurrentPosition(&curpos, NULL) != DS_OK)
	{
		strcpy(errordesc, "I_SoundPause: Unable to get current position");
		return false;
	}

	channel->pausedpos = curpos;

	if (channel->buffer->Stop() != DS_OK)
	{
		strcpy(errordesc, "I_SoundPause: Unable to stop sound playing");
		return false;
	}

	channel->flags &= ~_DXS_PLAYING;
	channel->flags |= _DXS_PAUSED;

	return true;
}

//
// I_SoundResume
//
boolean_t I_SoundResume(unsigned int chanid)
{
	channel_t *channel;
	DWORD status;

	if (!inited)
	{
		I_Error("I_SoundResume: Sound System Not Inited\n");
		return false;
	}

	DEV_ASSERT2(chanid < numchannels);

	channel = channels[chanid];

	if (!(channel->flags & _DXS_PAUSED))
	{
		strcpy(errordesc,"I_SoundResume: Channel is not paused");
		return false;
	}

	channel->buffer->GetStatus(&status);

	// Kill any lost buffer
	if (status & DSBSTATUS_BUFFERLOST)
	{
		channel->buffer->Release();
		channel->buffer = NULL;
		channel->flags = _DXS_NOBUFFER;
		strcpy (errordesc,"I_SoundResume: Channel's DirectSound Buffer has been lost");
		return false;
	}

	if (channel->buffer->SetCurrentPosition(channel->pausedpos) != DS_OK)
	{
		channel->buffer->Release();
		channel->buffer = NULL;
		channel->flags = _DXS_NOBUFFER;
		strcpy (errordesc,"I_SoundResume: Unable to set buffer position - effect NUKED");
		return false;
	}

	if (channel->flags & _DXS_LOOPING)
	{
		if (channel->buffer->Play(0,0,DSBPLAY_LOOPING) != DS_OK)
		{
			channel->buffer->Release();
			channel->buffer = NULL;
			channel->flags = _DXS_NOBUFFER;
			strcpy (errordesc,"I_SoundResume: Unable to restart looping playback - effect NUKED");
			return false;
		}
	}
	else
	{
		if (channel->buffer->Play(0,0,0) != DS_OK)
		{
			channel->buffer->Release();
			channel->buffer = NULL;
			channel->flags = _DXS_NOBUFFER;
			strcpy(errordesc, "I_SoundResume: Unable to restart playback - effect NUKED");
			return false;
		}
	}

	channel->flags &= ~_DXS_PAUSED;
	channel->flags |= _DXS_PLAYING;
	channel->pausedpos = 0;
	return true;
}

//
// I_SoundStopLooping
//
boolean_t I_SoundStopLooping(unsigned int chanid)
{
	channel_t *channel;

	if (!inited)
	{
		I_Error("I_SoundResume: Sound System Not Inited\n");
		return false;
	}

	DEV_ASSERT2(chanid < numchannels);

	channel = channels[chanid];

	if (channel->flags & _DXS_LOOPING)
	{
		channel->buffer->Stop();

		if (channel->buffer->Play(0,0,0) != DS_OK)
		{
			channel->buffer->Release();
			channel->buffer = NULL;
			channel->flags = _DXS_NOBUFFER;
			strcpy(errordesc, "I_SoundLooping: Unable to restart playback - effect NUKED");
			return false;
		}

		channel->flags &= ~_DXS_LOOPING;
	}

	return true;
}

//
// I_SoundTicker
//
// Updates the sound effects
//
// -ACB- 1999/10/06 Written.
//
void I_SoundTicker(void)
{
	unsigned int i;
	channel_t *channel;
	DWORD status;

	if (!inited)
		return;

	for (i = 0; i < numchannels; i++)
	{
		channel = channels[i];

		if (!(channel->flags & _DXS_PLAYING) && (channel->flags & _DXS_PAUSED))
			continue;

		// -ACB- 2000/05/20 Don't process channel with no buffer....
		if (!channel->buffer)
			continue;

		channel->buffer->GetStatus(&status);

		// Kill any lost buffer
		if (status & DSBSTATUS_BUFFERLOST)
		{
			channel->buffer->Release();
			channel->buffer = NULL;
			channel->flags = _DXS_NOBUFFER; // -ACB- 2000/05/20 No buffer here!
			continue;
		}

		if (!(status & DSBSTATUS_PLAYING))
		{
			channel->buffer->Release();
			channel->buffer = NULL;
			channel->flags = _DXS_NOBUFFER;
		}

	}

	return;
}

//
// I_ShutdownSound
//
// Shutdown of the sound output system
//
// -ACB- 1999/10/06 Written
// -ACB- 2000/05/17 Fixed to allow for sound caching
//
void I_ShutdownSound(void)
{
	unsigned int i;

	if (!inited)
		return;

	inited = false;

	if (channels)
	{
		for (i = 0; i < numchannels; i++)
		{
			if (!(channels[i]->flags & _DXS_NOBUFFER))
			{
				if (channels[i]->buffer) { channels[i]->buffer->Release(); }
			}

			delete channels[i];
		} 

		delete [] channels;
	}

	numchannels = 0;
	channels = NULL;

	if (numstoredsfx)
	{
		for (i=0; i<numstoredsfx; i++)
		{  
			if (storedsfx[i])
			{ 
				if (storedsfx[i]->data)
					delete [] storedsfx[i]->data;

				delete storedsfx[i];
			}
		}

		delete [] storedsfx;
	}

	numstoredsfx = 0;
	storedsfx = NULL;

	if (primarybuffer)
	{
		primarybuffer->Release();
		primarybuffer = NULL;
	}

	if (soundobject)
	{
		soundobject->Release();
		soundobject = NULL;
	}

	return;
}

//
// I_SoundReturnObject
//
// This really goes against the grain. DirectMusic requires
// that if a DirectSound system exists, we make it known. Trying
// to make modular code in WIN32 just got harder.
//
// -ACB- 2000/01/05
//
LPDIRECTSOUND I_SoundReturnObject(void)
{
	if (!inited)
		return NULL;

	return soundobject;
}

//
// I_SoundReturnError
//
const char *I_SoundReturnError(void)
{
	memcpy(scratcherror, errordesc, sizeof(char)*256);
	memset(errordesc, '\0', sizeof(char)*256);
	return scratcherror;
}
