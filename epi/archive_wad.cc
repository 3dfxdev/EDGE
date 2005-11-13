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

#include "epi.h"
#include "archive_wad.h"

#include "asserts.h"
#include "endianess.h"
#include "filesystem.h"

namespace epi
{

class waddir_c : public archive_dir_c  /* PRIVATE CLASS */
{
friend class wadentry_c;
friend class wadfile_c;

public:
	waddir_c() : archive_dir_c() { }
	~waddir_c() { }

	int CountAllEntries() const
	{
		int result = NumEntries();

		for (int j = result-1; j >= 0; j--)
		{
			wadentry_c *w_ent = (wadentry_c *) operator[](j);

			if (w_ent->isDir())
				result += w_ent->CountAllEntries();
		}

		return result;
	}
};

//------------------------------------------------------------------------

wadentry_c::wadentry_c(const char *_name, bool _dir, int _flags,
	int _pos, int _size) :
		position(_pos), size(_size), flags(_flags), wr_data()
{
	// Name size already checked (FIXME: add assertion)
	strcpy(name, _name);

	dir = _dir ? new waddir_c() : NULL;
}

wadentry_c::wadentry_c(const raw_wad_entry_t& raw_ent, int _flags) :
	position(EPI_LE_U32(raw_ent.start)),
	size(EPI_LE_U32(raw_ent.length)),
	flags(_flags), dir(NULL), wr_data()
{
	for (int i = 0; i < 8; i++)
		name[i] = toupper(raw_ent.name[i]);

	name[9] = 0;

	if (strncmp(name, "S_SKIN", 6) == 0)
		flags |= LF_Skin;
	
	// FIXME: match FONTx_S and FONTx_E as markers (Hexen)

	if (Match("S_START") || Match("SS_START") ||
	    Match("P_START") || Match("PP_START") ||
	    Match("F_START") || Match("FF_START") ||
	    Match("C_START"))
	{
		flags |= LF_Starter;
	}

	if (Match("S_END") || Match("SS_END") ||
	    Match("P_END") || Match("PP_END") ||
	    Match("F_END") || Match("FF_END") ||
	    Match("C_END"))
	{
		flags |= LF_Ender;
	}
}

wadentry_c::~wadentry_c()
{
	if (dir)
	{
		delete dir;
		dir = NULL;
	}
}

bool wadentry_c::isDir() const
{
	return dir ? true : false;
}

const char *wadentry_c::getName() const
{
	return name;
}

bool wadentry_c::getTimeStamp(timestamp_c *time) const
{
	return false;
}

int wadentry_c::getSize() const
{
	return size;
}

int wadentry_c::NumEntries() const
{
	// ASSERT(dir);
	return dir->NumEntries();
}

archive_entry_c *wadentry_c::operator[](int idx) const
{
	// ASSERT(dir);
	return (*dir)[idx];
}

archive_entry_c *wadentry_c::operator[](const char *_name) const
{
	// ASSERT(dir);
	return (*dir)[name];
}

archive_entry_c *wadentry_c::Add(const char *_name, const timestamp_c& time,
	bool is_dir, bool compress)
{
	// FIXME: check if name too long, make uppercase

	if (flags & LF_NoDirs)
		return NULL;

	// ASSERT(dir);

	int _flags = flags & (LF_ReadOnly | LF_WriteOnly);
	
	if (is_dir)
		_flags |= LF_NoDirs;

	wadentry_c *entry = new wadentry_c(_name, is_dir, _flags);

	int idx = dir->Insert(entry);

	if (idx < 0)
	{
		delete entry;
		entry = NULL;
	}

	return entry;
}

bool wadentry_c::Remove(int idx)
{
	// ASSERT(dir);
	return dir->Delete(idx);
}

bool wadentry_c::isStarter() const
{
	return (flags & LF_Starter) ? true : false;
}

bool wadentry_c::isEnder() const
{
	return (flags & LF_Ender) ? true : false;
}

bool wadentry_c::isLevel() const
{
	return (flags & LF_Level) ? true : false;
}

bool wadentry_c::isSkin() const
{
	return (flags & LF_Skin) ? true : false;
}

int wadentry_c::CountAllEntries() const
{
	// ASSERT(dir)
	return dir->CountAllEntries();
}

byte *wadentry_c::DoRead(file_c *src) const
{
	if (! src->Seek(position, file_c::SEEKPOINT_START))
		return NULL;
	
	unsigned int u_size = (unsigned int) size;

	byte *data = new byte[u_size + 1];

	data[u_size] = 0;

	if (u_size > 0)
	{
		if (src->Read(data, u_size) != u_size)
		{
			delete[] data;
			return NULL;
		}
	}

	return data;
}

bool wadentry_c::DoWrite(file_c *dest, file_c *src, int *cur_pos)
{
	if ((flags & LF_NeedCopy) && size > 0)
	{
		// ASSERT(src)

		if (! src->Seek(position, file_c::SEEKPOINT_START))
			return false;

		for (int copy_size = size; copy_size > 0; )
		{
			static byte copy_buf[1024];

			unsigned int actual = (unsigned int) MIN(1024, copy_size);

			if (src->Read(copy_buf, actual) != actual)
				return false;

			if (dest->Write(copy_buf, actual) != actual)
				return false;

			copy_size -= actual;
			*cur_pos  += actual;
		}
	}

	unsigned int wr_length = wr_data.Length();

	if (wr_length > 0)
	{
		if (dest->Write(wr_data.GetRawBase(), wr_length) != wr_length)
			return false;

		*cur_pos += wr_length;
	}

	return true;
}

//------------------------------------------------------------------------

wadfile_c::wadfile_c(const char *_filename, const char *_mode) :
	fp(NULL), read_only(false), write_only(false)
{
	filename = strdup(_filename);
	root = new waddir_c();

	switch (_mode[0])
	{
		case 'r': read_only  = true; break;
		case 'w': write_only = true; break;
		case 'a': break;

		default: //FIXME: Assert::fail("Bad switch in %s:%d\n", __FILE__, __LINE__);
			break;
	}
}

wadfile_c::~wadfile_c()
{
	if (fp)
	{
		the_filesystem->Close(fp);
		fp = NULL;
	}

	free((void*)filename);
	filename = NULL;

	delete root;
	root = NULL;
}

const char *wadfile_c::getFilename() const
{
	return filename;
}

const char *wadfile_c::getComment() const
{
	return "";
}

bool wadfile_c::setComment(const char *s) const
{
	return false;
}

int wadfile_c::NumEntries() const
{
	return root->NumEntries();
}

archive_entry_c *wadfile_c::operator[](int idx) const
{
	return (*root)[idx];
}

archive_entry_c *wadfile_c::operator[](const char *name) const
{
	return (*root)[name];
}

archive_entry_c *wadfile_c::Add(const char *name, const timestamp_c& time,
	 bool is_dir, bool compress)
{
	// FIXME: check if name too long, make uppercase

	int flags = (read_only  ? wadentry_c::LF_ReadOnly  : 0) |
				(write_only ? wadentry_c::LF_WriteOnly : 0);

	wadentry_c *entry = new wadentry_c(name, is_dir, flags);

	int idx = root->Insert(entry);

	if (idx < 0)
	{
		delete entry;
		entry = NULL;
	}

	return entry;
}

bool wadfile_c::Remove(int idx)
{
	return root->Delete(idx);
}

byte *wadfile_c::Read(archive_entry_c *entry) const
{
	// ASSERT(fp)

	if (write_only)
		return NULL;

	wadentry_c *w_ent = (wadentry_c *) entry;

	return w_ent->DoRead(fp);
}

bool wadfile_c::Truncate(archive_entry_c *entry)
{
	if (read_only)
		return false;

	wadentry_c *w_ent = (wadentry_c *) entry;
	
	w_ent->flags &= ~ wadentry_c::LF_NeedCopy;
	w_ent->wr_data.Resize(0);
	
	return true;
}

bool wadfile_c::Write(archive_entry_c *entry, const byte *data, int length)
{
	if (read_only)
		return false;

	wadentry_c *w_ent = (wadentry_c *) entry;
	w_ent->wr_data.Append(data, length);

	return true;
}

//------------------------------------------------------------------------
//  READING CODE
//------------------------------------------------------------------------

bool wadfile_c::CheckMagic(const char *type)
{
	return ((type[0] == 'I' || type[0] == 'P') &&
		 	 type[1] == 'W' && type[2] == 'A' && type[3] == 'D');
}

void wadfile_c::FindLevels(wadentry_c **list, int total)
{
	for (int j = 0; j < (total - 4); j++)
	{
		if (list[j+1]->Match("THINGS")   &&
			list[j+2]->Match("LINEDEFS") &&
			list[j+3]->Match("SIDEDEFS") &&
			list[j+4]->Match("VERTEXES"))
		{
			list[j]->flags |= wadentry_c::LF_Level;
		}

		// GL nodes
		if (list[j+1]->Match("GL_VERT")  &&
			list[j+2]->Match("GL_SEGS")  &&
			list[j+3]->Match("GL_SSECT") &&
			list[j+4]->Match("GL_NODES"))
		{
			list[j]->flags |= wadentry_c::LF_Level;
		}
	}
}

int wadfile_c::FindEnder(wadentry_c **list, int total, int start)
{
	for (start++; start < total; start++)
	{
		if (list[start]->isEnder())
			return start;
	}

	return -1;
}

int wadfile_c::GroupLevel(wadentry_c **list, int total, int start)
{
	const char *level_base = list[start]->getName();

	bool is_gl = (strncmp(level_base, "GL_", 3) == 0);

	static const char *level_lumps[] =
	{
		"THINGS", "LINEDEFS", "SIDEDEFS", "VERTEXES", "SEGS", 
		"SSECTORS", "NODES", "SECTORS", "REJECT", "BLOCKMAP",
		"BEHAVIOR" /* hexen */, "SCRIPTS", /* vavoom */
		NULL
	};

	static const char *gl_lumps[]=
	{
		"GL_VERT", "GL_SEGS", "GL_SSECT", "GL_NODES",
		"GL_PVS", /* vavoom */
		NULL
	};

	for (start++; start < total; start++)
	{
		bool matches = false;

		if (is_gl)
		{
			for (int j = 0; gl_lumps[j]; j++)
				if (list[start]->Match(gl_lumps[j]))
					matches = true;
		}
		else
		{
			for (int j = 0; level_lumps[j]; j++)
				if (list[start]->Match(level_lumps[j]))
					matches = true;
		}

		if (! matches)
			break;
	}

	return start - 1;
}

bool wadfile_c::CheckSkinFollows(wadentry_c *entry)
{
	return true;  //!!!! FIXME
}

int wadfile_c::GroupSkin(wadentry_c **list, int total, int start)
{
	bool is_inline = CheckSkinFollows(list[start]);

	if (! is_inline)
		return -1;

	start++;  // skip skin marker

	if (start >= total)
		return -1;

	const char *sprite_base = list[start]->getName();

	if (strlen(sprite_base) < 6)
		return -1;

	for (start++; start < total; start++)
	{
		if (strncmp(list[start]->getName(), sprite_base, 4) != 0)
			break;
	}

	return start - 1;
}

void wadfile_c::ReadDirectory()
{
	// ASSERT(fp)

	/* ---- Step 1: read header ---- */

	raw_wad_header_t header;

	if (! fp->Read(&header, sizeof(header)))
		throw epi::error_c(EPI_ERRGEN_FILEERROR, "Unable to read WAD header", true);

	if (! CheckMagic(header.type))
		throw epi::error_c(EPI_ERRGEN_FILEERROR, "File is not a WAD", true);
                                                                                            
	int total   = EPI_LE_U32(header.num_entries);
	int dir_pos = EPI_LE_U32(header.dir_start);

	// sanity check
	if (total > 65000)
		throw epi::error_c(EPI_ERRGEN_FILEERROR, "WAD seems corrupt", true);

	/* ---- Step 2: read entries ---- */

	if (! fp->Seek(dir_pos, file_c::SEEKPOINT_START))
		throw epi::error_c(EPI_ERRGEN_FILEERROR, "Unable to read directory", true);
	
	wadentry_c **tmp_list = new wadentry_c* [total];

	// OPTIMISE: read multiple entries at a time

	try
	{
		for (int idx = 0; idx < total; idx++)
		{
			raw_wad_entry_t raw_entry;

			if (! fp->Read(&raw_entry, sizeof(raw_entry)))
				throw epi::error_c(EPI_ERRGEN_FILEERROR, "Unable to read directory", true);

			tmp_list[idx] = new wadentry_c(raw_entry, 0 /* ??? */);

#if 0  // DEBUGGING
			fprintf(stderr, "Lump [%-8s]  pos 0x%08x  size 0x%08x\n",
				tmp_list[idx]->name, tmp_list[idx]->position, tmp_list[idx]->size);
#endif
		}

		FindLevels(tmp_list, total);
	}
	catch (error_c)
	{
		delete[] tmp_list;
		throw;
	}

	/* ---- Step 3: process entries ---- */

	int next = -1;

	for (int i = 0; i < total; i = next)
	{
		next = i + 1;

		int group_end = -1;

		wadentry_c *entry = tmp_list[i];

		root->Insert(entry);

		if (entry->isLevel())
			group_end = GroupLevel(tmp_list, total, i);
		else if (entry->isSkin())
			group_end = GroupSkin(tmp_list, total, i);
		else if (entry->isStarter())
		{
			group_end = FindEnder(tmp_list, total, i);

			// make sure it matches
			// FIXME: for hexen FONTx_S and FONTx_E markers
			if (group_end >= 0 && entry->name[0] != tmp_list[group_end]->name[0])
				group_end = -1;
			else
				group_end--;  // don't put end marker in sub-dir
		}

		if (group_end >= 0)
		{
			entry->dir = new waddir_c();
			entry->flags |= wadentry_c::LF_NoDirs;

			for (int j = i+1; j <= group_end; j++)
			{
				entry->dir->Insert(tmp_list[j]);

#if 0  // DEBUGGING
				fprintf(stderr, "Subdir %8s/%-8s\n", entry->getName(),
					tmp_list[j]->getName());
#endif
			}

			next = group_end + 1;
		}
	}

	delete[] tmp_list;
}


//------------------------------------------------------------------------
// GRUNT WORK: READING AND WRITING WADS
//------------------------------------------------------------------------

wadfile_c *wadfile_c::Open(const char *filename, const char *mode)
{
	// FIXME: check extension, disallow backups (BAK, BK1-9)

	if (mode[0] != 'r' && mode[0] != 'w' && mode[0] != 'a')
		return NULL;

	wadfile_c *wad = new wadfile_c(filename, mode);

	try
	{
		if (! wad->write_only)
		{
			wad->fp = the_filesystem->Open(filename, file_c::ACCESS_READ);

			if (! wad->fp)
				throw epi::error_c(EPI_ERRGEN_FILENOTFOUND, "File not found", true);

			wad->ReadDirectory();
		}
	}
	catch (error_c)
	{
		delete wad;
		throw;
	}

	return wad;
}

void wadfile_c::Close(wadfile_c *wad)
{
	if (wad->read_only)
	{
		delete wad;
		return;
	}

	if (wad->write_only)
	{
		file_c *fp = the_filesystem->Open(wad->filename, file_c::ACCESS_WRITE);

		if (! fp)
			throw epi::error_c(EPI_ERRGEN_FILENOTFOUND, "Unable to create file ", true);

		//!!!! FIXME: WRITE MODE...

		delete wad;
		return;
	}

	// APPEND MODE:

	// 1. close read FP
	// 2. copy original to .BK1
	// 3. re-open the .BK1 file
	// 4. write new WAD (probably same code-path as write-only)
	// 5. if successful, delete .BK1 file

	//!!!! FIXME: APPEND MODE...

}

}  // namespace epi

