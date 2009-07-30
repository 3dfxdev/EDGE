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

Wad_file * editing_wad;


Lump_c::Lump_c(Wad_file *_par, const char *_nam, int _ofs, int _len) :
	parent(_par), offset(_ofs), length(_len)
{
	name = strdup(_nam);
}

Lump_c::~Lump_c()
{
	free((void*)name);
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
	// TODO free stuff
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

	// write out base header
	raw_wad_header_t header;

	memset(&header, 0, sizeof(header));
	memcpy(header.ident, "PWAD", 4);

	fwrite(&header, sizeof(header), 1, fp);
	fflush(fp);

	return w;
}


Lump_c * Wad_file::GetLump(short index)
{
	SYS_ASSERT(0 <= index && index < (short)directory.size());
	SYS_ASSERT(directory[index]);

	return directory[index];
}


Lump_c * Wad_file::FindLump(const char *name)
{
	for (int k = 0; k < (int)directory.size(); k++)
		if (y_stricmp(directory[k]->name, name) == 0)
			return directory[k];

	return NULL;
}


Lump_c * Wad_file::FindLumpInLevel(short level, const char *name)
{
	SYS_ASSERT(0 <= level && level < (short)directory.size());

	// TODO: FindLumpInLevel

	for (int i = 1; i < 12; i++)
	{
		
	}

	return NULL;
}


short Wad_file::FindLevel(const char *name)
{
	for (int k = 0; k < (int)levels.size(); k++)
	{
		short index = levels[k];

		SYS_ASSERT(0 <= index && index < (short)directory.size());
		SYS_ASSERT(directory[index]);

		if (y_stricmp(directory[index]->name, name) == 0)
			return index;
	}

	return -1;  // not found
}


void Wad_file::ReadDirectory()
{
	// TODO: ReadDirectory
}


//------------------------------------------------------------------------


short WAD_FindEditLevel(const char *name)
{
	for (int i = (int)master_dir.size()-1; i >= 0; i--)
	{
		editing_wad = master_dir[i];

		short level = editing_wad->FindLevel(name);
		if (level >= 0)
			return level;
	}

	// not found
	editing_wad = NULL;
	return -1;
}

Lump_c * WAD_FindLump(const char *name)
{
	for (int i = (int)master_dir.size()-1; i >= 0; i--)
	{
		Lump_c *L = master_dir[i]->FindLump(name);
		if (L)
			return L;
	}

	return NULL;  // not found
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
