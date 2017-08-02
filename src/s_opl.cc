//----------------------------------------------------------------------------
//  EDGE2 OPL Music Player
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2015-2017  The EDGE2 Team.
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

#include "../opllib/oplapi.h"

#include "s_blit.h"
#include "s_music.h"
#include "s_opl.h"
#include "w_wad.h"
#include "m_misc.h"


#define OPL_NUM_SAMPLES 4096

extern bool dev_stereo;  // FIXME: encapsulation
extern int  dev_freq;    //

static bool genmidi_init = false;
static OPL_Player *opl_player = NULL;

static bool opl_inited;

class opl_player_c : public abstract_music_c
{
private:
	enum status_e
	{
		NOT_LOADED, PLAYING, PAUSED, STOPPED
	};

	int status;

	OPL_Player *player;

public:
	opl_player_c(OPL_Player *_player) : status(NOT_LOADED), player(_player)
	{ }

	~opl_player_c()
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

		status = NOT_LOADED;
	}

	void Play(bool loop)
	{
		if (!(status == NOT_LOADED || status == STOPPED))
			return;

		status = PLAYING;

		SYS_ASSERT(player);

		player->OPL_Loop(loop);
		player->OPL_PlaySong();

		// Load up initial buffer data
		Ticker();
	}

	void Stop(void)
	{
		if (!(status == PLAYING || status == PAUSED))
			return;

		S_QueueStop();

		player->OPL_StopSong();

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
			epi::sound_data_c *buf = S_QueueGetFreeBuffer(OPL_NUM_SAMPLES,
				dev_stereo ? epi::SBUF_Interleaved : epi::SBUF_Mono);

			if (!buf)
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
		s16_t *data_buf = buf->data_L;

		player->OPL_Generate(data_buf, OPL_NUM_SAMPLES);

		return true;
	}
};


bool S_StartupOPL(void)
{
	opl_player = new OPL_Player();

	if (!opl_player)
	{
		I_Printf("OPL player: failed!\n");
		return NULL;
	}

	opl_inited = true;
	genmidi_init = false;

	return true; // OK!
}

void S_ShutdownOPL(void)
{
	if (!opl_inited)
		return;

	if (opl_player)
	{
		delete opl_player;
	}

	opl_inited = false;
}


abstract_music_c * S_PlayOPL(byte *data, int length, float volume, bool loop)
{
	if (!opl_inited)
		return NULL;

	if (!genmidi_init)
	{
		byte *genmidi = (byte*)W_CacheLumpName("GENMIDI");
		if (!genmidi)
		{
			delete[] data;
			I_Printf("OPL player: cannot find GENMIDI lump.\n");
			return NULL;
		}

		if (!opl_player->OPL_LoadGENMIDI((void*)genmidi))
		{
			delete[] data;
			W_DoneWithLump(genmidi);
			I_Printf("OPL player: couldn't load GENMIDI.\n");
			return NULL;
		}

		W_DoneWithLump(genmidi);
	}

	if (!opl_player->OPL_LoadSong((void*)data, length))
	{
		delete[] data;
		I_Error("OPL player: couldn't load song.\n");
	}

	delete[] data;

	opl_config conf = { dev_freq, var_opl_opl3mode, dev_stereo };

	opl_player->OPL_SendConfig(conf);

	opl_player_c *player = new opl_player_c(opl_player);

	player->Volume(volume);
	player->Play(loop);

	return player;
}
