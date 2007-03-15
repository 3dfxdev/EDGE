//----------------------------------------------------------------------------
//  Sound Blitter
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
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#ifndef __S_BLIT__
#define __S_BLIT__

class sfxdef_c;
class position_c;


// We use a 22.10 fixed point for sound offsets.  It's a reasonable
// compromise between longest sound and accumulated round-off error.
typedef unsigned long fixed22_t;

// channel info
class mix_channel_c
{
public:
	fx_data_c *data;  // NULL means unused channel

	int category;
	sfxdef_c *def;
	position_c *pos;

	fixed22_t offset;
	fixed22_t length;
	fixed22_t delta;

	int volume_L;
	int volume_R;

public:
	mix_channel_c();
	~mix_channel_c();

	void ComputeDelta();
	void ComputeVolume();
};


extern mix_channel_c *mix_chan[];
extern int num_chan;


void S_InitChannels(int total);
void S_FreeChannels(void);

void S_SetListener(position_c *pos, angle_t angle);

void S_MixAllChannels(void *stream, int len);
// mix all active channels into the output stream.
// 'len' is the number of samples (for stereo: pairs)
// to mix into the stream.


#endif // __S_BLIT__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
