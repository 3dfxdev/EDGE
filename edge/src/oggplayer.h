//----------------------------------------------------------------------------
//  EDGE OGG Music Player (HEADER)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2004-2005  The EDGE Team.
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
#include "i_defs_al.h"

#include <epi/strings.h>
#include <vorbis/vorbisfile.h>

class oggplayer_c
{
public:
	oggplayer_c();
	~oggplayer_c();

	enum status_e { NOT_LOADED, PLAYING, PAUSED, STOPPED };
	
	struct datalump_s
	{
		byte *data;
		size_t pos;
		size_t size;
	};

private:
	static const int OGG_BUFFERS = 16;
	static const int BUFFER_SIZE = 16384;

	FILE* ogg_file;
	datalump_s ogg_lump;
	OggVorbis_File ogg_stream;
	vorbis_info* vorbis_inf;

	char *pcm_buf;

	ALuint buffers[OGG_BUFFERS];
	ALenum format;

	bool looping;
	int status;

	epi::string_c GetError(int code);

    static ALuint music_source;

    bool CreateMusicSource();
    void DeleteMusicSource();
	void PostOpenInit(void);
	bool StreamIntoBuffer(int buffer);

public:
	void SetVolume(float gain);

	void Open(const void* data, size_t size);
	void Open(const char* name);

	void Play(bool loop, float gain);
	void Pause(void);
	void Resume(void);
	void Stop(void);
	void Ticker(void);

	void Close(void);
};

#endif  // __OGGPLAYER__
