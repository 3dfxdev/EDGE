//------------------------------------------------------------------------
//  Ticcmd Storage
//------------------------------------------------------------------------
//
//  Edge MultiPlayer Server (C) 2004  Andrew Apted
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
//------------------------------------------------------------------------

#include "includes.h"

#include "buffer.h"
#include "network.h"
#include "packet.h"
#include "protocol.h"
#include "ticstore.h"
#include "ui_log.h"


tic_band_c::tic_band_c(int _size) : size(_size)
{
	cmds = new raw_ticcmd_t[_size];
}

tic_band_c::~tic_band_c()
{
	delete[] cmds;
	cmds = NULL;
}

void tic_band_c::Read(raw_ticcmd_t *dest, int start, int count) const
{
	SYS_ASSERT(start >= 0 && count > 0);
	SYS_ASSERT(start + count <= size);

	if (count > 0)
		memcpy(dest, cmds + start, count * sizeof(raw_ticcmd_t));
}

void tic_band_c::Write(const raw_ticcmd_t *src, int start, int count)
{
	SYS_ASSERT(start >= 0 && count >= 0);
	SYS_ASSERT(start + count <= size);

	if (count > 0)
		memcpy(cmds + start, src, count * sizeof(raw_ticcmd_t));
}

//------------------------------------------------------------------------

tic_store_c::tic_store_c(int band_size)
{
	for (int i = 0; i < MP_SAVETICS*2; i++)
	{
		bands[i] = new tic_band_c(band_size);
		received[i] = false;
	}
}

tic_store_c::~tic_store_c()
{
	for (int i = 0; i < MP_SAVETICS*2; i++)
	{
		delete bands[i];
		bands[i] = NULL;
	}
}

void tic_store_c::Clear(int tic_num)
{
	received[tic_num % (MP_SAVETICS*2)] = 0;
}

bool tic_store_c::HasGot(int tic_num) const
{
	return received[tic_num % (MP_SAVETICS*2)];
}

void tic_store_c::Read(int tic_num, raw_ticcmd_t *dest) const
{
	// should never read a band which we didn't write
	SYS_ASSERT(received[tic_num % (MP_SAVETICS*2)]);

	tic_band_c *band = bands[tic_num % (MP_SAVETICS*2)];

	band->Read(dest, 0, band->size);
}

void tic_store_c::Write(int tic_num, const raw_ticcmd_t *src)
{
	tic_band_c *band = bands[tic_num % (MP_SAVETICS*2)];

	band->Write(src, 0, band->size);

	received[tic_num % (MP_SAVETICS*2)] = true;
}

