//----------------------------------------------------------------------------
//  Sound Blitter
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2009  The EDGE Team.
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
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#ifndef __S_BLIT__
#define __S_BLIT__

#include "epi/sound_data.h"

#include "ddf/types.h"

// Forward declarations
class sfxdef_c;
class position_c;


// We use a 22.10 fixed point for sound offsets.  It's a reasonable
// compromise between longest sound and accumulated round-off error.
typedef unsigned long fixed22_t;

typedef enum
{
	CHAN_Empty = 0,
	CHAN_Playing = 1,
	CHAN_Finished = 2
}
chan_state_e;

// channel info
class mix_channel_c
{
public:
	int state;  // CHAN_xxx

	epi::sound_data_c *data;

	int category;
	sfxdef_c *def;
	position_c *pos;

	fixed22_t offset;
	fixed22_t length;
	fixed22_t delta;

	int volume_L;  // mixing volume
	int volume_R;

	bool loop;  // will loop *one* more time
	bool boss;

public:
	mix_channel_c();
	~mix_channel_c();

	void ComputeDelta();
	void ComputeVolume();
	void ComputeMusicVolume();
};


extern mix_channel_c *mix_chan[];
extern int num_chan;


void S_InitChannels(int total);
void S_FreeChannels(void);

void S_KillChannel(int k);
void S_ReallocChannels(int total);

void S_MixAllChannels(void *stream, int len);
// mix all active channels into the output stream.
// 'len' is the number of samples (for stereo: pairs)
// to mix into the stream.

void S_UpdateSounds(position_c *listener, angle_t angle);


//-------- API for Synthesised MUSIC --------------------

void S_QueueInit(void);
// initialise the queueing system.

void S_QueueShutdown(void);
// finalise the queuing system, stopping all playback.
// The data from all the buffers will be freed.

void S_QueueStop(void);
// stop the currently playing queue.  All playing buffers
// are moved into the free list.

epi::sound_data_c * S_QueueGetFreeBuffer(int samples, int buf_mode);
// returns the next unused (or finished) buffer, or NULL
// if there are none.  The data_L/data_R fields will be
// updated to ensure they hold the requested number of
// samples and conform to the wanted buffer mode.

void S_QueueAddBuffer(epi::sound_data_c *buf, int freq);
// add a new buffer to be end of the queue.

void S_QueueReturnBuffer(epi::sound_data_c *buf);
// if something goes wrong and you cannot add the buffer,
// then this call will return the buffer to the free list.

#endif // __S_BLIT__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
