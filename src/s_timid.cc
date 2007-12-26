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

#include "epi/file.h"
#include "epi/filesystem.h"
#include "epi/mus_2_midi.h"

#include "timidity/timidity.h"

#include "s_blit.h"
#include "s_music.h"
#include "s_timid.h"

#define TIMV_NUM_SAMPLES  8192

extern bool dev_stereo;  // FIXME: encapsulation
extern int  dev_freq;    //


class tim_player_c : public abstract_music_c
{
private:
	enum status_e
	{
		NOT_LOADED, PLAYING, PAUSED, STOPPED
	};
	
	int status;
	bool looping;

	MidiSong *song;

public:
	tim_player_c(MidiSong *_song) : status(NOT_LOADED), song(_song)
	{ }

	~tim_player_c()
	{
		Close();
	}

public:

	void Close(void)
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

	void Play(bool loop)
	{
		if (! (status == NOT_LOADED || status == STOPPED))
			return;

		status = PLAYING;
		looping = loop;

		SYS_ASSERT(song);

		Timidity_Start(song);

		// Load up initial buffer data
		Ticker();
	}

	void Stop(void)
	{
		if (! (status == PLAYING || status == PAUSED))
			return;

		S_QueueStop();

		status = STOPPED;
	}

	void Pause(void)
	{
		if (status != PLAYING)
			return;

		status = PAUSED;
	}

	void Resume(void)
	{
		if (status != PAUSED)
			return;

		status = PLAYING;
	}

	void Ticker(void)
	{
		while (status == PLAYING)
		{
			epi::sound_data_c *buf = S_QueueGetFreeBuffer(TIMV_NUM_SAMPLES, 
					dev_stereo ? epi::SBUF_Interleaved : epi::SBUF_Mono);

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

	void Volume(float gain)
	{
		// not needed, music volume is handled in s_blit.cc
		// (see mix_channel_c::ComputeMusicVolume).
	}

private:

	bool StreamIntoBuffer(epi::sound_data_c *buf)
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
					Timidity_Stop();
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

};



///---void OpenData(const void *data, size_t size)
///---{
///---	/* DataLump version */
///---
///---	if (status != NOT_LOADED)
///---		Close();
///---
///---	if (size < 8)
///---	{
///---		I_Printf("Timidity player: ignoring small lump (%d bytes)\n", size);
///---		return;
///---	}
///---
///---#if (1 == 0)
///---	// check for MUS, convert to MIDI 
///---	//
///---
///---	OpenMidiFile(cached_name);
///---#endif
///---}
///---
///---void OpenFile(const char *filename)
///---{
///---	/* File version */
///---
///---	if (status != NOT_LOADED)
///---		Close();
///---
///---	// FIXME: check for 'MUS' format !!!
///---
///---#if 0
///---	ogg_file = fopen(filename, "rb");
///---    if (!ogg_file)
///---    {
///---		I_Error("[oggplayer_c::OpenFile] Could not open file.\n");
///---    }
///---#endif
///---
///---	OpenMidiFile(filename);
///---
///---}
///---
///---void OpenMidiFile(const char *filename)
///---{
///---	// Loaded, but not playing
///---	status = STOPPED;
///---
///---
///---	Timidity_Start(song);
///---}


//----------------------------------------------------------------------------


abstract_music_c * S_PlayTimidity(byte *data, int length, bool is_mus,
			float volume, bool loop)
{
	if (is_mus)
	{
		I_Printf("tim_player_c: Converting MUS format to MIDI...\n");

		byte *midi_data;
		int midi_len;

		if (! Mus2Midi::Convert(data, length, &midi_data, &midi_len,
					Mus2Midi::DOOM_DIVIS, true))
		{
			delete [] data;

			I_Warning("Unable to convert MUS to MIDI !\n");
			return NULL;
		}

		delete[] data;

		data   = midi_data;
		length = midi_len;

		I_Debugf("Conversion done: new length is %d\n", length);
	}

	MidiSong *song = Timidity_LoadSong(data, length);

	delete[] data;

	if (! song)
		I_Error("Timidity player: failed to load MIDI file!\n");

	tim_player_c *player = new tim_player_c(song);

	player->Volume(volume);
	player->Play(loop);

	return player;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
