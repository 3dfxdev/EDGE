//----------------------------------------------------------------------------
//  EDGE Timidity Music Player
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2004-2007  The EDGE Team.
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

#include "s_cache.h"
#include "s_blit.h"
#include "s_timid.h"

#define TIMV_NUM_SAMPLES  8192

extern bool dev_stereo;  // FIXME: encapsulation


tim_player_c::tim_player_c() : status(NOT_LOADED), looping(false), song(NULL)
{
}

tim_player_c::~tim_player_c()
{
	Close();
}


void tim_player_c::SetVolume(float gain) // FIXME: remove
{ }


void tim_player_c::Open(const void *data, size_t size)
{
	/* DataLump version */

	if (status != NOT_LOADED)
		Close();

	if (size < 8)
	{
		I_Printf("Timidity player: ignoring small lump (%d bytes)\n", size);
		return;
	}

	// check for MUS, convert to MIDI 
	if (data[0] == 'M' && data[1] == 'U' && data[2] == 'S')
	{
		CONVERT  -->  cache_dir / music.mid
	}

	OpenMidiFile(cached_name);
}

void tim_player_c::Open(const char *filename)
{
	/* File version */

	if (status != NOT_LOADED)
		Close();

	// FIXME: check for 'MUS' format !!!

#if 0
	ogg_file = fopen(filename, "rb");
    if (!ogg_file)
    {
		I_Error("[oggplayer_c::OpenFile] Could not open file.\n");
    }
#endif

	OpenMidiFile(filename);

}

void tim_player_c::OpenMidiFile(const char *filename)
{
	// Loaded, but not playing
	status = STOPPED;

	song = Timidity_LoadSong(filename);

	if (! song)
		I_Error("Timidity player: cannot load MIDI file!\n");

	Timidity_Start(song);
}


void tim_player_c::Close()
{
	if (status == NOT_LOADED)
		return;

	// Stop playback
	Stop();

	if (song)
	{
		Timidity_FreeSong(song);
		song = NULL;
	}
	
	status = NOT_LOADED;
}


void tim_player_c::Pause()
{
	if (status != PLAYING)
		return;

	status = PAUSED;
}


void tim_player_c::Resume()
{
	if (status != PAUSED)
		return;

	status = PLAYING;
}


void tim_player_c::Play(bool loop, float gain)
{
    if (status != NOT_LOADED && status != STOPPED)
        return;

	status = PLAYING;
	looping = loop;

	// Load up initial buffer data
	Ticker();
}


void tim_player_c::Stop()
{
	if (status != PLAYING && status != PAUSED)
		return;

	S_QueueStop();

	status = STOPPED;
}


void tim_player_c::Ticker()
{
	while (status == PLAYING)
	{
		epi::sound_data_c *buf = S_QueueGetFreeBuffer(TIMV_NUM_SAMPLES, 
				(is_stereo && dev_stereo) ? epi::SBUF_Interleaved : epi::SBUF_Mono);

		if (! buf)
			break;

		if (StreamIntoBuffer(buf))
		{
			S_QueueAddBuffer(buf, dev_freq);
		}
		else
		{
			// finished playing
			S_QueueReturnBuffer(buf);
			Stop();
		}
	}
}

bool tim_player_c::StreamIntoBuffer(epi::sound_data_c *buf)
{
    int samples = 0;

    while (samples < TIMV_NUM_SAMPLES)
    {
		s16_t *data_buf = buf->data_L + samples * (dev_stereo ? 2 : 1);

        int got_num = Timidity_PlaySome(data_buf, TIMV_NUM_SAMPLES - samples);

		if (got_num <= 0)  /* EOF */
		{
			if (looping)
			{
				Timidity_Stop(song);
				Timidity_Start(song);

				continue; // try again
			}

			break;
		}

		// FIXME error handling ???

		samples += got_num;
    }

    return (samples > 0);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
