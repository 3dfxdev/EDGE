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


tic_band_c::tic_band_c(int _size) : size(_size), received(false)
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

tic_store_c::tic_store_c(int band_size) : cur_tic(0), past_tic(0)
{
	for (int i = 0; i < MP_SAVETICS*2; i++)
		bands[i] = new tic_band_c(band_size);
}

tic_store_c::~tic_store_c()
{
	for (int i = 0; i < MP_SAVETICS*2; i++)
	{
		delete bands[i];
		bands[i] = NULL;
	}
}

void tic_store_c::Reset(int tic_num)
{
	cur_tic  = tic_num;
	past_tic = tic_num;  // i.e. none
}

bool tic_store_c::CanRead (int tic_num) const
{
	return (tic_num >= past_tic && tic_num < cur_tic + MP_SAVETICS);
}

bool tic_store_c::CanWrite(int tic_num) const
{
	return (tic_num >= cur_tic && tic_num < cur_tic + MP_SAVETICS);
}

void tic_store_c::Read(int tic_num, raw_ticcmd_t *dest, int start, int count) const
{
	SYS_ASSERT(CanRead(tic_num));

	tic_band_c *band = bands[tic_num % (MP_SAVETICS*2)];

	band->Read(dest, start, count);
}

void tic_store_c::Write(int tic_num, const raw_ticcmd_t *src, int start, int count)
{
	SYS_ASSERT(CanWrite(tic_num));

	tic_band_c *band = bands[tic_num % (MP_SAVETICS*2)];

	band->Write(src, start, count);
}

