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

#ifndef __TICSTORE_H__
#define __TICSTORE_H__

#include "protocol.h"

class tic_band_c
{
	// holds the ticcmds for a set of players.
	// you could consider this the horizontal dimension.

friend class tic_store_c;

public:
	tic_band_c(int _size);
	~tic_band_c();

private:
	int size;

	raw_ticcmd_t *cmds;

public:
	void Read(       raw_ticcmd_t *dest, int start, int count) const;
	void Write(const raw_ticcmd_t *src,  int start, int count);
};

class tic_store_c
{
	// holds a history of ticcmds for a band.
	// you could consider this the vertical dimension.
	
public:
	tic_store_c(int band_size);
	~tic_store_c();

private:
	tic_band_c *bands[MP_SAVETICS * 2];

	bool received[MP_SAVETICS * 2];

public:
	void Clear(int tic_num);
	bool HasGot(int tic_num) const;

	// these read and write a whole band
	void Read( int tic_num,       raw_ticcmd_t *dest) const;
	void Write(int tic_num, const raw_ticcmd_t *src);
};

#endif /* __TICSTORE_H__ */
