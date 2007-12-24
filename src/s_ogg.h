//----------------------------------------------------------------------------
//  EDGE OGG Music Player (HEADER)
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
//
// -ACB- 2004/08/18 Written
//
#ifndef __OGGPLAYER__
#define __OGGPLAYER__

#include "i_defs.h"

#include <vorbis/vorbisfile.h>

#include "epi/sound_data.h"

class pl_entry_c;
class abstract_music_c;


class oggplayer_c
{
public:
	oggplayer_c();
	~oggplayer_c();

	struct datalump_s
	{
		byte *data;
		size_t pos;
		size_t size;
	};

private:

	enum status_e
	{
		NOT_LOADED, PLAYING, PAUSED, STOPPED
	};
	
	int status;
	bool looping;

	FILE *ogg_file;
	datalump_s ogg_lump;
	OggVorbis_File ogg_stream;
	vorbis_info *vorbis_inf;
	bool is_stereo;

	s16_t *mono_buffer;

	const char *GetError(int code);

	void PostOpenInit(void);

	bool StreamIntoBuffer(epi::sound_data_c *buf);

	void ConvertToMono(s16_t *dest, const s16_t *src, int len);

public:
	void SetVolume(float gain);

	void Open(const void *data, size_t size);
	void Open(const char *filename);
	void Close(void);

	void Play(bool loop, float gain);
	void Stop(void);

	void Pause(void);
	void Resume(void);

	void Ticker(void);
};


/* FUNCTIONS */

abstract_music_c * S_PlayOGGMusic(const pl_entry_c *play, float volume, bool looping);

#endif  // __OGGPLAYER__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
