//------------------------------------------------------------------------
//  WAD Reading / Writing
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
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
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#include "main.h"

#include "w_rawdef.h"
#include "w_wad.h"


std::vector< Wad_file* > master_dir;


Lump_c::Lump_c(int _ofs, int _len) : offset(_ofs), length(_len)
{
}

Lump_c::~Lump_c()
{
}


//------------------------------------------------------------------------


Wad_file::Wad_file(FILE * file) : fp(file), directory(),
	dir_offset(0), dir_length(0), dir_crc(0),
	levels(), patches(), sprites(), flats(), tex_info(NULL),
	holes()
{
}

Wad_file::~Wad_file()
{
	// TODO FreeDirectory()
}


Wad_file * Wad_file::Open(const char *filename)
{
	FILE *fp = fopen(filename, "rw");
	if (! fp)
		return NULL;

	Wad_file *w = new Wad_file(fp);

	w->ReadDirectory();

	return w;
}

Wad_file * Wad_file::Create(const char *filename)
{
	FILE *fp = fopen(filename, "rw");
	if (! fp)
		return NULL;

	Wad_file *w = new Wad_file(fp);

	// FIXME write out base header

	return w;
}


void Wad_file::ReadDirectory()
{
	// TODO: ReadDirectory
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
