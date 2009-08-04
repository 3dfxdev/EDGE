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

	void MakeEntry(struct raw_wad_entry_s *entry);

public:
	~Lump_c();

	const char *Name() const { return name; }
	int Length() const { return l_length; }

	// attempt to seek to a position within the lump (default is
	// the beginning).  Returns true if OK, false on error.
	bool Seek(int offset = 0);

	// read some data from the lump, returning true if OK.
	bool Read(void *data, int len);

	// write some data to the lump.  Only the lump which had just
	// been created with Wad_file::AddLump() can be written to.
	bool Write(void *data, int len);

	// mark the lump as finished (after writing it).
	bool Finish();

private:
	// deliberately don't implement these
	Lump_c(const Lump_c& other);
	Lump_c& operator= (const Lump_c& other);
};


class Wad_file
{
friend class Lump_c;
friend void W_LoadFlats();

private:
	FILE * fp;

	char kind;  // 'P' for PWAD, 'I' for IWAD

	// zero means "currently unknown", which only occurs after a
	// call to BeginWrite() and before any call to AddLump() or
	// the finalizing EndWrite().
	int total_size;

	std::vector<Lump_c *> directory;

	int dir_start;
	int dir_count;
	u32_t dir_crc;

	// these are lump indices (into 'directory' vector)
	std::vector<short> levels;
	std::vector<short> patches;
	std::vector<short> sprites;
	std::vector<short> flats;

	Texture_info *tex_info;

	bool can_write;

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

	short FindLumpNum(const char *name);
	short FindLevel(const char *name);

	Lump_c * FindLumpInNamespace(const char *name, char group);

	// check whether another program has modified this WAD, and return
	// either true or false.  We test for change in file size, change
	// in directory size or location, and directory contents (CRC).
	bool WasExternallyModified();

	// all changes to the was must occur between calls to BeginWrite()
	// and EndWrite() methods.
	void BeginWrite();
	void EndWrite();

	// remove the given lump(s)
	void RemoveLumps(short index, short count = 1);

	// this removes the level marker PLUS all associated level
	// lumps which follow it.
	void RemoveLevel(short index);

	// insert a new lump.  The second form is for a level marker.
	// The 'max_size' parameter (if >= 0) specifies the most data
	// you will write into the lump -- writing more will corrupt
	// something else in the WAD.
	Lump_c * AddLump (const char *name, int max_size = -1);
	Lump_c * AddLevel(const char *name, int max_size = -1);

private:
	// read the existing directory.
	void ReadDirectory();

	void DetectLevels();
	void ProcessNamespaces();

	// look at all the lumps and determine the lowest offset from
	// start of file where we can write new data.
	int HighWaterMark();

	// set the file write position to match 'total_size', appending
	// zeros if the file is currently shorter (the different should
	// be no more than a few bytes).  Returns new position.
	int PositionForWrite();

	bool FinishLump();
	int  WritePadding(int count);

	// write the new directory, updating the dir_xxx variables
	// (including the CRC).
	void WriteDirectory();

	void FixGroup(std::vector<short>& group, short index, short removed);

private:
	// deliberately don't implement these
	Wad_file(const Wad_file& other);
	Wad_file& operator= (const Wad_file& other);
};


extern std::vector<Wad_file *> master_dir;


// find a lump in any loaded wad (later ones tried first),
// returning NULL if not found.
Lump_c * W_FindLump(const char *name);

// find a lump that only exists in a certain namespace (sprite,
// or patch) of a loaded wad (later ones tried first).
Lump_c * W_FindSpriteLump(const char *name);
Lump_c * W_FindPatchLump(const char *name);

// load the lump into memory, returning the size
int  W_LoadLumpData(Lump_c *lump, byte ** buf_ptr);
void W_FreeLumpData(byte ** buf_ptr);

#endif  /* __W_WAD_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
