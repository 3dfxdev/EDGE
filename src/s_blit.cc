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

#include "i_defs.h"

#include "s_sound.h"
#include "s_cache.h"
#include "s_blit.h"

#include "m_misc.h"
#include "r_main.h"  // R_PointToAngle
#include "p_local.h"  // P_ApproxDistance

#include <string.h>


// Sound must be clipped to prevent distortion (clipping is
// a kind of distortion of course, but it's much better than
// the "white noise" you get when values overflow).
//
// The more safe bits there are, the less likely the final
// output sum will overflow into white noise, but the less
// precision you have left for the volume multiplier.
#define SAFE_BITS  3
#define CLIP_THRESHHOLD  ((1L << (31-SAFE_BITS)) - 1)

// This value is how much breathing room there is until
// sounds start clipping.  More bits means less chance
// of clipping, but every extra bit makes the sound half
// as loud as before.
#define QUIET_BITS  0


#define MAX_CHANNELS  128

mix_channel_c *mix_chan[MAX_CHANNELS];
int num_chan;

static int *mix_buffer;
static int mix_buf_len;

static int sfxvolume = 19;  //!!!! FIXME
static bool game_paused = false; //!!!!!! FIXME

// these are analogous to viewx/y/z, viewangle
float listen_x;
float listen_y;
float listen_z;
angle_t listen_angle;

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

extern int dev_freq;
extern int dev_bits;
extern int dev_bytes_per_sample;
extern int dev_frag_pairs;
extern int dev_stereo;


mix_channel_c::mix_channel_c() : data(NULL)
{ }

mix_channel_c::~mix_channel_c()
{ }
	
void mix_channel_c::ComputeDelta()
{
	// frequency close enough ?
	if (data->freq > (dev_freq - dev_freq/100) &&
		data->freq < (dev_freq + dev_freq/100))
	{
		delta = (1 << 10);
	}
	else
	{
		delta = (fixed22_t) floor((float)data->freq * 1024.0f / dev_freq);
	}
}

void mix_channel_c::ComputeVolume()
{
	float sep = 0.5f;
	float mul = 1.0f;

	if (pos) // && pos != ::players[displayplayer]->mo
	{
		if (var_sound_stereo)
		{
			angle_t angle = R_PointToAngle(listen_x, listen_y, pos->x, pos->y);

			// same equation from original DOOM 
			sep = 0.5f - 0.38f * M_Sin(angle - listen_angle);
		}

		if (! boss)
		{
			// approximate distance (with hack for Z)
			float dist = P_ApproxDistance(listen_x - pos->x, listen_y - pos->y);

			dist += fabs(listen_z - pos->z) / 4.0f;

			// -AJA- this equation was chosen to mimic the DOOM falloff
			//       function, but doesn't cut out @ dist=1600, instead
			//       tapering off exponentially.
			mul = exp(-MAX(1.0f, dist - S_CLOSE_DIST) / 800.0f);
		}
	}

	float MAX_VOL = (1 << (16 - SAFE_BITS - MAX(0,var_quiet_factor-1))) - 3;

	if (var_quiet_factor == 0)
		MAX_VOL *= 2.0;

	MAX_VOL = MAX_VOL * mul * slider_to_gain[sfxvolume];

	// strictly linear equations
	volume_L = (int) (MAX_VOL * (1.0 - sep));
	volume_R = (int) (MAX_VOL * (0.0 + sep));

	if (var_sound_stereo == 2)  /* SWAP ! */
	{
		int tmp = volume_L;
		volume_L = volume_R;
		volume_R = tmp;
	}
}


//----------------------------------------------------------------------------

static void BlitToU8(const int *src, u8_t *dest, int length)
{
	const int *s_end = src + length;

	while (src < s_end)
	{
		int val = *src++;

		     if (val >  CLIP_THRESHHOLD) val =  CLIP_THRESHHOLD;
		else if (val < -CLIP_THRESHHOLD) val = -CLIP_THRESHHOLD;

		*dest++ = (u8_t) ((val >> (24-SAFE_BITS)) ^ 0x80);
	}
}

static void BlitToS8(const int *src, s8_t *dest, int length)
{
	const int *s_end = src + length;

	while (src < s_end)
	{
		int val = *src++;

		     if (val >  CLIP_THRESHHOLD) val =  CLIP_THRESHHOLD;
		else if (val < -CLIP_THRESHHOLD) val = -CLIP_THRESHHOLD;

		*dest++ = (s8_t) (val >> (24-SAFE_BITS));
	}
}

static void BlitToU16(const int *src, u16_t *dest, int length)
{
	const int *s_end = src + length;

	while (src < s_end)
	{
		int val = *src++;

		     if (val >  CLIP_THRESHHOLD) val =  CLIP_THRESHHOLD;
		else if (val < -CLIP_THRESHHOLD) val = -CLIP_THRESHHOLD;

		*dest++ = (u16_t) ((val >> (16-SAFE_BITS)) ^ 0x8000);
	}
}

static void BlitToS16(const int *src, s16_t *dest, int length)
{
	const int *s_end = src + length;

	while (src < s_end)
	{
		int val = *src++;

		     if (val >  CLIP_THRESHHOLD) val =  CLIP_THRESHHOLD;
		else if (val < -CLIP_THRESHHOLD) val = -CLIP_THRESHHOLD;

		*dest++ = (s16_t) (val >> (16-SAFE_BITS));
	}
}


static void MixMono(mix_channel_c *chan, int *dest, int length)
{
	DEV_ASSERT2(length > 0);

	const s16_t *src_L = chan->data->data_L;

	register int *d_pos = dest;
	int *d_end = d_pos + length;

	fixed22_t offset = chan->offset;

	while (d_pos < d_end)
	{
		*d_pos++ += src_L[offset >> 10] * chan->volume_L;

		offset += chan->delta;
	}

	chan->offset = offset;

	DEV_ASSERT2(offset - chan->delta < chan->length);
}

static void MixStereo(mix_channel_c *chan, int *dest, int length)
{
	DEV_ASSERT2(length > 0);

	const s16_t *src_L = chan->data->data_L;
	const s16_t *src_R = chan->data->data_R;

	register int *d_pos = dest;
	int *d_end = d_pos + length * 2;

	fixed22_t offset = chan->offset;

	while (d_pos < d_end)
	{
		*d_pos++ += src_L[offset >> 10] * chan->volume_L;
		*d_pos++ += src_R[offset >> 10] * chan->volume_R;

		offset += chan->delta;
	}

	chan->offset = offset;

	DEV_ASSERT2(offset - chan->delta < chan->length);
}

static void MixChannel(mix_channel_c *chan, int pairs)
{
	// check if channel active
	if (! chan->data)
		return;

	if (game_paused && chan->category >= SNCAT_Player)
		return;

	if (chan->volume_L == 0 && chan->volume_R == 0)
		return;

	int *dest = mix_buffer;
	
	while (pairs > 0)
	{
		// check if enough sound data is left
		if (chan->offset + pairs * chan->delta >= chan->length)
		{
			// TEST CRAP !!!!! FIXME
			S_CacheRelease(chan->data);
			chan->data = NULL;
			break;
		}

		int count = pairs;

		if (dev_stereo)
			MixStereo(chan, dest, count);
		else
			MixMono(chan, dest, count);

		dest += count * (dev_stereo ? 2 : 1);

		// FIXME
		break;
	}
}


void S_MixAllChannels(void *stream, int len)
{
	if (nosound || len <= 0)
		return;

	int samples = len / dev_bytes_per_sample;

	// check that we're not getting too much data
	DEV_ASSERT2(samples <= dev_frag_pairs);

	// allocate mixer buffer if needed
	if (! mix_buffer or mix_buf_len < samples)
	{
		if (mix_buffer)
			delete[] mix_buffer;

		mix_buf_len = samples;
		mix_buffer = new int[mix_buf_len];
	}

	// clear mixer buffer
	memset(mix_buffer, 0, mix_buf_len * sizeof(int));

#if 0  // TESTING.. TESTING.. 1 2 3...
	mix_buffer[0] = CLIP_THRESHHOLD;
	mix_buffer[32] = -CLIP_THRESHHOLD;
#endif

	// add each channel
	for (int i=0; i < num_chan; i++)
	{
		MixChannel(mix_chan[i], samples);
	} 

	// blit to the SDL stream
	if (dev_bits == 8)
		BlitToU8(mix_buffer, (u8_t *)stream, samples);
	else
		BlitToS16(mix_buffer, (s16_t *)stream, samples);
}


void S_InitChannels(int total)
{
	// NOTE: assumes audio is locked!

	num_chan = total;

	for (int i = 0; i < num_chan; i++)
		mix_chan[i] = new mix_channel_c();
}

void S_FreeChannels(void)
{
	// NOTE: assumes audio is locked!
	
	for (int i = 0; i < num_chan; i++)
	{
		mix_channel_c *chan = mix_chan[i];

		if (chan->data)
		{
			S_CacheRelease(chan->data);
			chan->data = NULL;
		}

		delete chan;
	}

	memset(mix_chan, 0, sizeof(mix_chan));
}

void S_UpdateSounds(position_c *listener, angle_t angle)
{
	// NOTE: assume SDL_LockAudio has been called

	listen_x = listener ? listener->x : 0;
	listen_y = listener ? listener->y : 0;
	listen_z = listener ? listener->z : 0;

	listen_angle = angle;

	for (int i = 0; i < num_chan; i++)
	{
		mix_channel_c *chan = mix_chan[i];

		if (chan->data)
			chan->ComputeVolume();
	}
}

