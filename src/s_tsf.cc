//----------------------------------------------------------------------------
//  EDGE2 TinySoundfont Music Player
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2004-2009  The EDGE Team.
//  Converted from the original Timidity-based player in 2021 - Dashodanger
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

#include "system/i_defs.h"

#include "../epi/file.h"
#include "../epi/filesystem.h"
#include "../epi/mus_2_midi.h"
#include "../epi/path.h"
#include "../epi/str_format.h"

#include "s_blit.h"
#include "s_music.h"
#include "s_tsf.h"

#include "dm_state.h"

#define TSF_IMPLEMENTATION
#include "system/sound/tinysoundfont/tsf.h"

#define TML_IMPLEMENTATION
#include "system/sound/tinysoundfont/tml.h"

#define TSF_NUM_SAMPLES  4096

extern bool dev_stereo;
extern int  dev_freq; 

static bool tsf_inited;

tsf *edge_tsf;

class tsf_player_c : public abstract_music_c
{
private:
	enum status_e
	{
		NOT_LOADED, PLAYING, PAUSED, STOPPED
	};
	
	int status;
	bool looping;
	double current_time;

	tml_message *song;

public:
	tsf_player_c(tml_message *_song) : status(NOT_LOADED), song(_song)
	{ }

	~tsf_player_c()
	{
		Close();
	}

public:

	void Close(void)
	{
		if (status == NOT_LOADED)
			return;

		// Stop playback
		if (status != STOPPED)
		  Stop();
	
		status = NOT_LOADED;
	}

	void Play(bool loop)
	{
		if (! (status == NOT_LOADED || status == STOPPED))
			return;

		status = PLAYING;
		looping = loop;

		SYS_ASSERT(song);
		current_time = 0;

		// Load up initial buffer data
		Ticker();
	}

	void Stop(void)
	{
		if (! (status == PLAYING || status == PAUSED))
			return;

		tsf_reset(edge_tsf);

		S_QueueStop();

		status = STOPPED;
	}

	void Pause(void)
	{
		if (status != PLAYING)
			return;

		tsf_note_off_all(edge_tsf);

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
			epi::sound_data_c *buf = S_QueueGetFreeBuffer(TSF_NUM_SAMPLES, 
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

	int PlaySome(s16_t *data_buf, int samples) {

		int SampleBlock, SampleCount = samples;

		int samples_played = 0;

		for (SampleBlock = TSF_RENDER_EFFECTSAMPLEBLOCK; SampleCount; SampleCount -= SampleBlock, data_buf += (SampleBlock * (dev_stereo ? 2 : 1 * sizeof(float))))
		{
			//We progress the MIDI playback and then process TSF_RENDER_EFFECTSAMPLEBLOCK samples at once
			if (SampleBlock > SampleCount) SampleBlock = SampleCount;

			for (current_time += SampleBlock * (1000.0 / dev_freq); song && current_time >= song->time; song = song->next)
			{
				switch (song->type)
				{
					case TML_PROGRAM_CHANGE: //channel program (preset) change (special handling for 10th MIDI channel with drums)
						tsf_channel_set_presetnumber(edge_tsf, song->channel, song->program, (song->channel == 9));
						break;
					case TML_NOTE_ON: //play a note
						tsf_channel_note_on(edge_tsf, song->channel, song->key, song->velocity / 127.0f);
						break;
					case TML_NOTE_OFF: //stop a note
						tsf_channel_note_off(edge_tsf, song->channel, song->key);
						break;
					case TML_PITCH_BEND: //pitch wheel modification
						tsf_channel_set_pitchwheel(edge_tsf, song->channel, song->pitch_bend);
						break;
					case TML_CONTROL_CHANGE: //MIDI controller messages
						tsf_channel_midi_control(edge_tsf, song->channel, song->control, song->control_value);
						break;
				}
			}

			// Render the block of audio samples in short format
			tsf_render_short(edge_tsf, data_buf, SampleBlock, 0);
			samples_played += SampleBlock;
		}
		
		return samples_played;
	}

	bool StreamIntoBuffer(epi::sound_data_c *buf)
	{
		int samples = 0;

		while (samples < TSF_NUM_SAMPLES)
		{
			s16_t *data_buf = buf->data_L + samples * (dev_stereo ? 2 : 1);

			int got_num = PlaySome(data_buf, TSF_NUM_SAMPLES - samples);

			if (got_num <= 0)  /* EOF */
			{
				if (looping)
				{
					current_time = 0;
					continue; // try again
				}

				return (samples == TSF_NUM_SAMPLES);
			}

			samples += got_num;
		}

		return true;
	}
};

bool S_StartupTSF(void)
{

	I_Printf("Initializing TinySoundFont...\n");

	edge_tsf = tsf_load_filename("soundfont/default.sf2");

	if (!edge_tsf) {
		I_Printf("Could not load soundfont! Ensure that default.sf2 is present in the soundfont directory!\n");
		return false;
	}

	tsf_channel_set_bank_preset(edge_tsf, 9, 128, 0);
	tsf_set_output(edge_tsf, dev_stereo ? TSF_STEREO_INTERLEAVED : TSF_MONO, dev_freq, 0);

	tsf_inited = true;

	return true; // OK!
}

abstract_music_c * S_PlayTSF(byte *data, int length, bool is_mus,
			float volume, bool loop)
{
	if (!tsf_inited)
		return NULL;

	if (is_mus)
	{
		I_Debugf("tsf_player_c: Converting MUS format to MIDI...\n");

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

	tml_message *song = tml_load_memory(data, length);

	delete[] data;

	if (!song) //Lobo: quietly log it instead of completely exiting EDGE
	{
		I_Debugf("TinySoundfont player: failed to load MIDI file!\n");
		return NULL;
	}

	tsf_player_c *player = new tsf_player_c(song);

	player->Volume(volume);
	player->Play(loop);

	return player;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
