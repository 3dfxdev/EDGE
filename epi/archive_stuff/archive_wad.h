//------------------------------------------------------------------------
//  EDGE WAD : Basic WAD I/O
//------------------------------------------------------------------------
// 
//  Copyright (c) 2003-2005  The EDGE Team.
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
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//------------------------------------------------------------------------

#ifndef __EPI_ARCHIVE_WAD_H__
#define __EPI_ARCHIVE_WAD_H__

#include "types.h"
#include "macros.h"
#include "archive.h"
#include "bytearray.h"
#include "files.h"

#include "rawdef_wad.h"

namespace epi
{
	//
	// NOTES ABOUT WAD HANDLING:
	//
	// 1) Names in Wads must be 8 characters or less, and must be totally
	//    uppercase (this code will ensure that).  Hence when using the
	//    find operator, the given string needs to be uppercase also, e.g.
	//    the expression my_wad["Hello"] will _always_ fail.
	//
	// 2) Wads do not support directories.  There are however some places
	//    in a WAD file where a bunch of lumps are grouped together,
	//    namely:
	//
	//    - sprites (grouped between S_START and S_END)
	//    - flats   (grouped between F_START and F_END)
	//    - patches (grouped between P_START and P_END)
    //    - colmaps (grouped between C_START and C_END, a Boom addition)
	//
	//    - levels (e.g. MAP01 marker, then THINGS lump, etc..)
	//    - GL nodes (e.g. GL_MAP01 marker, then GL_VERT lump, etc..)
	//    - player skins (S_SKINxx marker, followed by sprites)
	//
	//    The reading code will detect these situations and create fake
	//    directories for them.  The end markers ("x_END") are NOT included
	//    in the sub-directory, they remain at top level.  For writing, the
	//    code allows the creation of directories, but only at top level.
	//
	//    Another thing: these fake directories can have data, and hence
	//    you can call Read() and Write() on them.  Hexen level markers
	//    are known to contain some info, and skin markers definitely do.
	//

class waddir_c;  /* INTERNAL CLASS */

class wadentry_c : public archive_entry_c
{
friend class wadfile_c;

private:
	wadentry_c(const char *_name, bool _dir, int _flags,
			   int _pos = 0, int _size = 0);
	wadentry_c(const raw_wad_entry_t& raw_ent, int _flags);
	~wadentry_c();

	enum
	{
		LF_ReadOnly  = 0x0001,
		LF_WriteOnly = 0x0002,
		LF_NoDirs    = 0x0004,

///		LF_Truncated = 0x0004,  // append mode: clear previous data

		LF_NeedCopy  = 0x0008,  // append mode: need to copy data

		LF_Starter   = 0x0010,  // S_START etc...
		LF_Ender     = 0x0020,  // S_END etc...
		LF_Level     = 0x0040,
		LF_Skin      = 0x0080,
	};

	char name[12];
	int position;
	int size;
	int flags;

	waddir_c *dir;  // usually NULL

	bytearray_c wr_data;  // storage for writing

	byte *DoRead(file_c *src) const;
	bool DoWrite(file_c *dest, file_c *src, int *cur_pos);

	/* ---- ARCHIVE_ENTRY_C METHODS ---- */

public:
	bool isDir() const;
	const char *getName() const;
	int getSize() const;
	bool getTimeStamp(timestamp_c *time) const;

	int NumEntries() const;
	archive_entry_c *operator[](int idx) const;
	archive_entry_c *operator[](const char *name) const;
	archive_entry_c *Add(const char *name, const timestamp_c& time,
		bool dir = false, bool compress = false);
	bool Remove(int idx);

	/* ---- EXTRA METHODS ---- */

	bool isStarter() const;
	bool isEnder() const;

	bool isLevel() const;
	bool isSkin() const;

	int CountAllEntries() const;  // directories only

	bool Match(const char *other) const
	{
		return (strcmp(name, other) == 0);
	}
};

class wadfile_c : public archive_c
{
private:
	wadfile_c(const char *_filename, const char *_mode);
	~wadfile_c();

	file_c *fp;

	const char *filename;

	bool read_only;
	bool write_only;

	waddir_c *root;

	inline bool isAppend() { return !read_only && !write_only; }

	// internal methods
	bool CheckMagic(const char *type);
	void FindLevels(wadentry_c **list, int total);
	int FindEnder(wadentry_c **list, int total, int start);
	int GroupLevel(wadentry_c **list, int total, int start);
	bool CheckSkinFollows(wadentry_c *entry);
	int GroupSkin(wadentry_c **list, int total, int start);

	void ReadDirectory();

public:
	static wadfile_c *Open(const char *filename, const char *mode);
	// mode is "r" to just read the WAD, "w" to write a new WAD,
	// and "a" to open an existing WAD file for modification.  Throws
	// an epi::error_c exception on failure (never returns NULL).

	static void Close(wadfile_c *wad);
	// closes the WAD file.  For write and append modes, writes all the
	// data to the file.  The wadfile_c object is also freed.  Throws
	// an epi::error_c exception on failure.

	/* ---- ARCHIVE_C METHODS ---- */

	const char *getFilename() const;
	const char *getComment()  const;
	bool setComment(const char *s)  const;
	int getSize() const;

	int NumEntries() const;
	archive_entry_c *operator[](int idx) const;
	archive_entry_c *operator[](const char *name) const;
	archive_entry_c *Add(const char *name, const timestamp_c& time,
		 bool dir = false, bool compress = false);
	bool Remove(int idx);

	byte *Read(archive_entry_c *entry) const;
	bool Truncate(archive_entry_c *entry);
	bool Write(archive_entry_c *entry, const byte *data, int length);

	/* ---- EXTRA METHODS ---- */

	// bool isIWad() const;
    // void makeIWad();
};

}  // namespace epi

#endif  /* __EPI_ARCHIVE_WAD_H__ */
