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

#ifndef __W_WAD_H__
#define __W_WAD_H__


class Wad_file;
class Texture_info;


class Lump_c
{
friend class Wad_file;

private:
	Wad_file *parent;

	const char *name;

	int l_start;
	int l_length;

	// constructor is private
	Lump_c(Wad_file *_par, const char *_nam, int _start, int _len);
	Lump_c(Wad_file *_par, const struct raw_wad_entry_s *entry);

public:
	~Lump_c();

	int Length() const { return l_length; }

	// attempt to seek to a position within the lump (default is
	// the beginning).  Returns true if OK, false on error.
	bool Seek(int offset = 0);

	// read some data from the lump, returning true if OK.
	bool Read(void *data, int len);

private:
	// deliberately don't implement these
	Lump_c(const Lump_c& other);
	Lump_c& operator= (const Lump_c& other);
};


class Wad_file
{
friend class Lump_c;

private:
	FILE * fp;

	std::vector< Lump_c* > directory;

	int dir_start;
	int dir_count;
	u32_t dir_crc;

	// these are lump indices (into 'directory' vector)
	std::vector<short> levels;
	std::vector<short> patches;
	std::vector<short> sprites;
	std::vector<short> flats;

	Texture_info *tex_info;

	// this keeps track of unused areas in the WAD file
	std::vector< Lump_c* > holes;

	// constructor is private
	Wad_file(FILE * file);

public:
	~Wad_file();

	static Wad_file * Open(const char *filename);
	static Wad_file * Create(const char *filename);

	short NumLumps() const { return (short)directory.size(); }

	Lump_c * GetLump(short index);
	Lump_c * FindLump(const char *name);
	Lump_c * FindLumpInLevel(const char *name, short level);

	short FindLevel(const char *name);

private:
	void ReadDirectory();

private:
	// deliberately don't implement these
	Wad_file(const Wad_file& other);
	Wad_file& operator= (const Wad_file& other);
};


extern std::vector<Wad_file *> master_dir;

// the WAD which we are currently editing
extern Wad_file * editing_wad;


// attemps to find the level for editing.  If found, sets the
// 'editing_wad' global var to the wad containing the level and
// returns a LEVEL index number (NOT a lump number).  If not
// found, returns -1.
short WAD_FindEditLevel(const char *name);

// find a lump in any loaded wad (later ones tried first),
// returning NULL if not found.
Lump_c * WAD_FindLump(const char *name);


#endif  /* __W_WAD_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
