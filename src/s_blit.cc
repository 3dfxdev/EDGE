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


// Sound must be clipped to prevent distortion (clipping is
// a kind of distortion of course, but it's much better than
// the "white noise" you get when values overflow).
//
// The more safe bits there are, the less likely the final
// output sum will overflow into white noise, but the less
// precision you have left for the volume multiplier.
#define SAFE_BITS  3
#define CLIP_THRESHHOLD  ((1 << (31-SAFE_BITS)) - 1)

// This value is how much breathing room there is until
// sounds start clipping.  More bits means less chance
// of clipping, but every extra bit makes the sound half
// as loud as before.
#define QUIET_BITS  2


#define MAX_CHANNELS  128

static mix_channel_c *mix_chan[MAX_CHANNELS];
static int num_chan;

static int *mix_buffer;


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


static void MixMonoSamples(mix_channel_t *chan, int length)
{

}

static void MixStereoSamples(mix_channel_t *chan, int count)
{
	u16_t *src_L = chan->data->data_L;
	u16_t *src_R = chan->data->data_R;

	int *dest = mix_buffer;
	int d_end = dest + length * 2;

	fixed22_t offset = chan->offset;

	while (dest < d_end)
	{
		s16_t sample = src_L[offset >> 10];

		*dest++ += sample * chan->volume_L;
		*dest++ += sample * chan->volume_R;

		offset += chan->delta;

		if (offset >= chan->length)
			return (d_end - dest) / 2
	}

	chan->offset = offset;

	return -1;
}


static void MixChannel(mix_channel_t *chan, int want)
{
	int i;

	int *dest_L = mix_buffer_L;
	int *dest_R = mix_buffer_R;

	char *src_L = chan->sound->data_L;
	int length = chan->sound->length;

	while (want > 0)
	{
		int count = MIN(chan->sound->length - (chan->offset >> 10), want);

		DEV_ASSERT2(count > 0);

		MixStereoSamples(BLAH);

		want -= count;

		// return if sound hasn't finished yet
		if ((chan->offset >> 10) < length)
			return;

		if (! chan->looping)
		{
			chan->priority = PRI_FINISHED;
			return;
		}

		// loop back to beginning
		src_L = chan->sound->data_L;
		chan->offset = 0;
	}
}

static INLINE unsigned char ClipSampleS8(int value)
{
	if (value < -0x7F) return 0x81;
	if (value >  0x7F) return 0x7F;

	return (unsigned char) value;
}

static INLINE unsigned char ClipSampleU8(int value)
{
	value += 0x80;

	if (value < 0)    return 0;
	if (value > 0xFF) return 0xFF;

	return (unsigned char) value;
}

static INLINE unsigned short ClipSampleS16(int value)
{
	if (value < -0x7FFF) return 0x8001;
	if (value >  0x7FFF) return 0x7FFF;

	return (unsigned short) value;
}

static INLINE unsigned short ClipSampleU16(int value)
{
	value += 0x8000;

	if (value < 0)      return 0;
	if (value > 0xFFFF) return 0xFFFF;

	return (unsigned short) value;
}

static void BlitToSoundDevice(unsigned char *dest, int pairs)
{
	int i;

	if (dev_bits == 8)
	{
		if (mydev.channels == 2)
		{
			for (i=0; i < pairs; i++)
			{
				*dest++ = ClipSampleU8(mix_buffer_L[i] >> 16);
				*dest++ = ClipSampleU8(mix_buffer_R[i] >> 16);
			}
		}
		else
		{
			for (i=0; i < pairs; i++)
			{
				*dest++ = ClipSampleU8(mix_buffer_L[i] >> 16);
			}
		}
	}
	else
	{
		DEV_ASSERT2(dev_bits == 16);

		unsigned short *dest2 = (unsigned short *) dest;

		if (mydev.channels == 2)
		{
			for (i=0; i < pairs; i++)
			{
				*dest2++ = ClipSampleS16(mix_buffer_L[i] >> 8);
				*dest2++ = ClipSampleS16(mix_buffer_R[i] >> 8);
			}
		}
		else
		{
			for (i=0; i < pairs; i++)
			{
				*dest2++ = ClipSampleS16(mix_buffer_L[i] >> 8);
			}
		}
	}
}


void S_InternalSoundFiller(void *udata, Uint8 *stream, int len)
{
	int i;
	int pairs = len / dev_bytes_per_sample;

	// check that we're not getting too much data
	DEV_ASSERT2(pairs <= dev_frag_pairs);

	if (nosound || pairs <= 0)
		return;

	// clear mixer buffer
	memset(mix_buffer_L, 0, sizeof(int) * pairs);
	memset(mix_buffer_R, 0, sizeof(int) * pairs);

	// add each channel
	for (i=0; i < MIX_CHANNELS; i++)
	{
		if (mix_chan[i].priority >= 0)
			MixChannel(&mix_chan[i], pairs);
	}

	BlitToSoundDevice(stream, pairs);
}


fixed22_t S_ComputeDelta(int data_freq, int device_freq)
{
	// sound data's frequency close enough ?
	if (data_freq > device_freq - device_freq/100 &&
			data_freq < device_freq + device_freq/100)
	{
		return 1024;
	}

	return (fixed22_t) floor((float)data_freq * 1024.0f / device_freq);
}

