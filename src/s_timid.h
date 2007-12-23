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

#ifndef __S_TIMID_H__
#define __S_TIMID_H__

#include "i_defs.h"

#include "timidity/timidity.h"


class tim_player_c
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
	 tim_player_c();
	~tim_player_c();

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

private:
	void OpenMidiFile(const char *filename);

	bool StreamIntoBuffer(epi::sound_data_c *buf);

};

#endif /* __S_TIMID_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
