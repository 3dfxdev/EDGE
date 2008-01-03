//----------------------------------------------------------------------------
//  Sound Gather class
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2008  The EDGE Team.
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

#include "epi.h"
#include "sound_gather.h"

namespace epi
{

class gather_chunk_c
{
public:
	s16_t *samples;

	int num_samples;  // total number is *2 for stereo

	bool is_stereo;

public:
	gather_chunk_c(int _count, bool _stereo) :
		num_samples(_count), is_stereo(_stereo)
	{
		SYS_ASSERT(num_samples > 0);

		samples = new s16_t[num_samples * (is_stereo ? 2:1)];
	}
	
	~gather_chunk_c()
	{
		delete[] samples;
	}
};


//----------------------------------------------------------------------------

sound_gather_c::sound_gather_c() : chunks(), total_samples(0), request(NULL)
{ }

sound_gather_c::~sound_gather_c()
{
	if (request)
		DiscardChunk();

	for (unsigned int i=0; i < chunks.size(); i++)
		delete chunks[i];
}

s16_t * sound_gather_c::MakeChunk(int max_samples, bool _stereo)
{
	SYS_ASSERT(! request);
	SYS_ASSERT(max_samples > 0);

	request = new gather_chunk_c(_stereo, max_samples);

	return request->samples;
}

void sound_gather_c::CommitChunk(int actual_samples)
{
	SYS_ASSERT(request);
	SYS_ASSERT(actual_samples >= 0);

	if (actual_samples == 0)
	{
		DiscardChunk();
		return;
	}

	SYS_ASSERT(actual_samples <= request->num_samples);

	chunks.push_back(request);

	request = NULL;
}

void sound_gather_c::DiscardChunk()
{
	SYS_ASSERT(request);

	delete request;

	request = NULL;
}

void sound_gather_c::Finalise(sound_data_c *buf, bool want_stereo)
{
	// TODO Finalise
}

}  // namespace epi

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
