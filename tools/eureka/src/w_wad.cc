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

#include "lib_crc.h"
#include "w_rawdef.h"
#include "w_wad.h"


std::vector< Wad_file* > master_dir;

Wad_file * editing_wad;


Lump_c::Lump_c(Wad_file *_par, const char *_nam, int _start, int _len) :
	parent(_par), l_start(_start), l_length(_len)
{
	name = strdup(_nam);
}

Lump_c::Lump_c(Wad_file *_par, const struct raw_wad_entry_s *entry) :
	parent(_par)
{
	// handle the entry name, which can lack a terminating NUL
	char buffer[10];
	strncpy(buffer, entry->name, 8);
	buffer[8] = 0;

	name = strdup(buffer);

	l_start  = LE_U32(entry->pos);
	l_length = LE_U32(entry->size);

//	DebugPrintf("new lump '%s' @ %d len:%d\n", name, l_start, l_length);
}

Lump_c::~Lump_c()
{
	free((void*)name);
}

void Lump_c::MakeEntry(struct raw_wad_entry_s *entry)
{
	strncpy(entry->name, name, 8);

	entry->pos    = LE_U32(l_start);
	entry->length = LE_U32(l_length);
}


bool Lump_c::Seek(int offset)
{
	SYS_ASSERT(offset >= 0);

	return (fseek(parent->fp, l_start + offset, SEEK_SET) == 0);
}

bool Lump_c::Read(void *data, int len)
{
	SYS_ASSERT(data && len > 0);

	return (fread(data, len, 1, parent->fp) == 1);
}

bool Lump_c::Write(void *data, int len)
{
	SYS_ASSERT(data && len > 0);

	l_length += len;

	// hmmm, maybe move this into Wad_file
	parent->total_size += len;

	return (fwrite(data, len, 1, parent->fp) == 1);
}

bool Lump_c::Finish()
{
	parent->FinishLump();
}


//------------------------------------------------------------------------


Wad_file::Wad_file(FILE * file) : fp(file), total_size(0),
	directory(), dir_start(0), dir_count(0), dir_crc(0),
	levels(), patches(), sprites(), flats(), tex_info(NULL),
	can_write(false)
{
}

Wad_file::~Wad_file()
{
	// TODO free stuff
}


Wad_file * Wad_file::Open(const char *filename)
{
	LogPrintf("Opening WAD file: %s\n", filename);

	FILE *fp = fopen(filename, "r+b");
	if (! fp)
		return NULL;

	Wad_file *w = new Wad_file(fp);

	// determine total size (seek to end)
	if (fseek(fp, 0, SEEK_END) != 0)
		FatalError("Error determining WAD size.\n");
	
	w->total_size = (int)ftell(fp);

	DebugPrintf("total_size = %d\n", w->total_size);

	if (w->total_size < 0)
		FatalError("Error determining WAD size.\n");

	w->ReadDirectory();
	w->DetectLevels();
	w->ProcessNamespaces();

	return w;
}

Wad_file * Wad_file::Create(const char *filename)
{
	LogPrintf("Creating new WAD file: %s\n", filename);

	FILE *fp = fopen(filename, "w+b");
	if (! fp)
		return NULL;

	Wad_file *w = new Wad_file(fp);

	// write out base header
	raw_wad_header_t header;

	memset(&header, 0, sizeof(header));
	memcpy(header.ident, "PWAD", 4);

	fwrite(&header, sizeof(header), 1, fp);
	fflush(fp);

	w->total_size = (int)sizeof(header);

	return w;
}


Lump_c * Wad_file::GetLump(short index)
{
	SYS_ASSERT(0 <= index && index < NumLumps());
	SYS_ASSERT(directory[index]);

	return directory[index];
}


Lump_c * Wad_file::FindLump(const char *name)
{
	for (int k = 0; k < NumLumps(); k++)
		if (y_stricmp(directory[k]->name, name) == 0)
			return directory[k];

	return NULL;
}


Lump_c * Wad_file::FindLumpInLevel(const char *name, short level)
{
	SYS_ASSERT(0 <= level && level < (short)levels.size());

	// determine how far past the level marker (MAP01 etc) to search
	short last = levels[level] + 14;

	if (last >= NumLumps())
		last = NumLumps() - 1;
	
	// assumes levels[] are in increasing lump order!
	if (level+1 < (short)levels.size())
		if (last >= levels[level+1])
			last = levels[level+1] - 1;

	for (short k = levels[level]+1; k <= last; k++)
	{
		SYS_ASSERT(0 <= k && k < NumLumps());

		if (y_stricmp(directory[k]->name, name) == 0)
			return directory[k];
	}

	return NULL;
}


short Wad_file::FindLevel(const char *name)
{
	for (int k = 0; k < (int)levels.size(); k++)
	{
		short index = levels[k];

		SYS_ASSERT(0 <= index && index < NumLumps());
		SYS_ASSERT(directory[index]);

		if (y_stricmp(directory[index]->name, name) == 0)
			return k;
	}

	return -1;  // not found
}


void Wad_file::ReadDirectory()
{
	// TODO: no fatal errors

	rewind(fp);

	raw_wad_header_t header;

	if (fread(&header, sizeof(header), 1, fp) != 1)
		FatalError("Error reading WAD header.\n");

	// TODO: check ident for PWAD or IWAD

	dir_start = LE_S32(header.dir_start);
	dir_count = LE_S32(header.num_entries);

	if (dir_count < 0 || dir_count > 32000)
		FatalError("Bad WAD header, too many entries (%d)\n", dir_count);

	crc32_c checksum;

	if (fseek(fp, dir_start, SEEK_SET) != 0)
		FatalError("Error seeking to WAD directory.\n");

	for (int i = 0; i < dir_count; i++)
	{
		raw_wad_entry_t entry;

		if (fread(&entry, sizeof(entry), 1, fp) != 1)
			FatalError("Error reading WAD directory.\n");

		// update the checksum with each _RAW_ entry
		checksum.AddBlock((u8_t *) &entry, sizeof(entry));

		Lump_c *lump = new Lump_c(this, &entry);

		// TODO: check if entry is valid

		directory.push_back(lump);
	}

	dir_crc = checksum.raw;

	LogPrintf("Loaded directory. crc = %08x\n", dir_crc);
}


static int WhatLevelPart(const char *name)
{
	if (y_stricmp(name, "THINGS")   == 0) return 1;
	if (y_stricmp(name, "LINEDEFS") == 0) return 2;
	if (y_stricmp(name, "SIDEDEFS") == 0) return 3;
	if (y_stricmp(name, "VERTEXES") == 0) return 4;
	if (y_stricmp(name, "SECTORS")  == 0) return 5;

	return 0;
}

static bool IsLevelLump(const char *name)
{
	if (y_stricmp(name, "SEGS")     == 0) return true;
	if (y_stricmp(name, "SSECTORS") == 0) return true;
	if (y_stricmp(name, "NODES")    == 0) return true;
	if (y_stricmp(name, "REJECT")   == 0) return true;
	if (y_stricmp(name, "BLOCKMAP") == 0) return true;
	if (y_stricmp(name, "BEHAVIOR") == 0) return true;
	if (y_stricmp(name, "SCRIPTS")  == 0) return true;

	return WhatLevelPart(name) != 0;
}


void Wad_file::DetectLevels()
{
	// Determine what lumps in the wad are level markers, based on
	// the lumps which follow it.  Store the result in the 'levels'
	// vector.  The test here is rather lax, as I'm told certain
	// wads exist with a non-standard ordering of level lumps.

	for (int k = 0; k+5 < NumLumps(); k++)
	{
		int part_mask  = 0;
		int part_count = 0;

		// check whether the next four lumps are level lumps
		for (int i = 1; i <= 4; i++)
		{
			int part = WhatLevelPart(directory[k+i]->name);

			if (part == 0)
				break;

			// do not allow duplicates
			if (part_mask & (1 << part))
				break;

			part_mask |= (1 << part);
			part_count++;
		}

		if (part_count == 4)
		{
			levels.push_back(k);

			DebugPrintf("Detected level : %s\n", directory[k]->name);
		}
	}
}


static bool IsDummyMarker(const char *name)
{
	// matches P1_START, F3_END etc...

	if (strlen(name) < 3)
		return false;

	if (! strchr("PSF", toupper(name[0])))
		return false;
	
	if (! isdigit(name[1]))
		return false;

	if (y_stricmp(name+2, "_START") == 0 ||
	    y_stricmp(name+2, "_END") == 0)
		return true;

	return false;
}

void Wad_file::ProcessNamespaces()
{
	char active = 0;

	for (int k = 0; k < NumLumps(); k++)
	{
		const char *name = directory[k]->name;

		// skip the sub-namespace markers
		if (IsDummyMarker(name))
			continue;

		if (y_stricmp(name, "P_START") == 0 || y_stricmp(name, "PP_START") == 0)
		{
			if (active && active != 'P')
				LogPrintf("WARNING: missing %c_END marker.\n", active);
			
			active = 'P';
			continue;
		}
		else if (y_stricmp(name, "P_END") == 0 || y_stricmp(name, "PP_END") == 0)
		{
			if (active != 'P')
				LogPrintf("WARNING: stray P_END marker found.\n");
			
			active = 0;
			continue;
		}

		if (y_stricmp(name, "S_START") == 0 || y_stricmp(name, "SS_START") == 0)
		{
			if (active && active != 'S')
				LogPrintf("WARNING: missing %c_END marker.\n", active);
			
			active = 'S';
			continue;
		}
		else if (y_stricmp(name, "S_END") == 0 || y_stricmp(name, "SS_END") == 0)
		{
			if (active != 'S')
				LogPrintf("WARNING: stray S_END marker found.\n");
			
			active = 0;
			continue;
		}

		if (y_stricmp(name, "F_START") == 0 || y_stricmp(name, "FF_START") == 0)
		{
			if (active && active != 'F')
				LogPrintf("WARNING: missing %c_END marker.\n", active);
			
			active = 'F';
			continue;
		}
		else if (y_stricmp(name, "F_END") == 0 || y_stricmp(name, "FF_END") == 0)
		{
			if (active != 'F')
				LogPrintf("WARNING: stray F_END marker found.\n");
			
			active = 0;
			continue;
		}

		if (active)
		{
			if (directory[k]->Length() == 0)
			{
				LogPrintf("WARNING: skipping empty lump %s in %c_START\n",
						  name, active);
				continue;
			}

//			DebugPrintf("Namespace %c lump : %s\n", active, name);

			switch (active)
			{
				case 'P': patches.push_back(k); break;
				case 'S': sprites.push_back(k); break;
				case 'F': flats.  push_back(k); break;

				default:
					BugError("ProcessNamespaces: active = 0x%02x\n", (int)active);
			}
		}
	}

	if (active)
		LogPrintf("WARNING: Missing %c_END marker (at EOF)\n", active);
}


bool Wad_file::WasExternallyModified()
{
	if (fseek(fp, 0, SEEK_END) != 0)
		FatalError("Error determining WAD size.\n");

	if (total_size != (int)ftell(fp))
		return true;
	
	rewind(fp);

	raw_wad_header_t header;

	if (fread(&header, sizeof(header), 1, fp) != 1)
		FatalError("Error reading WAD header.\n");

	if (dir_start != LE_S32(header.dir_start) ||
		dir_count != LE_S32(header.num_entries))
		return true;

	fseek(fp, dir_start, SEEK_SET);

	crc32_c checksum;

	for (int i = 0; i < dir_count; i++)
	{
		raw_wad_entry_t entry;

		if (fread(&entry, sizeof(entry), 1, fp) != 1)
			FatalError("Error reading WAD directory.\n");

		checksum.AddBlock((u8_t *) &entry, sizeof(entry));

	}

	DebugPrintf("New CRC : %08x\n", checksum.raw);

	return (dir_crc != checksum.raw);
}


//------------------------------------------------------------------------


void Wad_file::BeginWrite()
{
	if (can_write)
		BugError("Wad_file::BeginWrite() called again with EndWrite()\n");

	// put the size into a quantum state
	total_size = 0;

	can_write = true;
}

void Wad_file::EndWrite()
{
	if (! can_write)
		BugError("Wad_file::EndWrite() called without BeginWrite()\n");

	can_write = false;

	WriteDirectory();
}


void Wad_file::RemoveLumps(short index, short count)
{
	SYS_ASSERT(can_write);
	SYS_ASSERT(0 <= index && index < NumLumps());
	SYS_ASSERT(directory[index]);

	short i;

	for (i = 0; i < count; i++)
	{
		Lump_c * lump = directory[index + i];

		// it would be possible to put this lump into a list of
		// 'holes' for later re-use.  However the possibility of
		// aliasing (i.e. two entries in the directory refering
		// to the same data) make that a pain to implement.

		delete lump;
	}

	for (i = index; i+count < NumLumps(); i++)
		directory[i] = directory[i+count];

	directory.resize(directory.size() - (size_t)count);
}

bool Wad_file::RemoveLump(const char *name)
{
	// TODO: RemoveLump
}

bool Wad_file::RemoveLevel(short level)
{
	SYS_ASSERT(can_write);

	// TODO: RemoveLevel
}


Lump_c * Wad_file::AddLump(const char *name, int max_size)
{
	SYS_ASSERT(can_write);


		l_start = parent->PositionForWrite();



	// TODO: AddLump
}

Lump_c * Wad_file::AddLevel(const char *name, int max_size)
{
	levels.push_back(NumLumps());

	return AddLump(name, max_size);
}


int Wad_file::HighWaterMark()
{
	int offset = (int)sizeof(raw_wad_header_t);

	for (int k = 0; k < NumLumps(); k++)
	{
		Lump_c *lump = directory[k];

		// ignore zero-length lumps (their offset could be anything)
		if (lump->Length() <= 0)
			continue;

		int l_end = lump->l_start + lump->l_length;

		if (offset < l_end)
			offset = l_end;
	}

	// round up to a multiple of 4
	offset = ((offset + 3) / 4) * 4;

	return offset;
}

int Wad_file::PositionForWrite()
{
	// already got the position?
	if (total_size > 0)
		return total_size;

	total_size = HighWaterMark();
	SYS_ASSERT(total_size > 0);

	if (fseek(fp, 0, SEEK_END) < 0)
		FatalError("Error seeking to new write position.\n");

	int pos = (int)ftell(fp);

	if (pos < 0)
		FatalError("Error seeking to new write position.\n");
	
	if (pos < total_size)
	{
		WritePadding(total_size - pos);
		fflush(fp);
	}
	else if (pos > total_size)
	{
		if (fseek(fp, total_size, SEEK_SET) < 0)
			FatalError("Error seeking to new write position.\n");
	}

	return total_size;
}


void Wad_file::FinishLump()
{
	if (total_size & 3)
	{
		total_size += WritePadding(4 - (total_size & 3));
	}

	fflush(fp);
}

int Wad_file::WritePadding(int count)
{
	static byte zeros[8] = { 0,0,0,0,0,0,0,0 };

	SYS_ASSERT(1 <= count && count <= 8);

	fwrite(zeros, count, 1, fp);

	return count;
}


void Wad_file::WriteDirectory()
{
	dir_start = PositionForWrite();
	dir_count = NumLumps();

	LogPrintf("WriteDirectory...\n");
	DebugPrintf("dir_start:%d  dir_count:%d\n", dir_start, dir_count);

	crc32_c checksum;

	for (int k = 0; k < dir_count; k++)
	{
		Lump_c *lump = directory[k];
		SYS_ASSERT(lump);

		raw_wad_entry_t entry;

		lump->MakeEntry(&entry);

		// update the CRC
		checksum.AddBlock((u8_t *) &entry, sizeof(entry));

		if (fwrite(&entry, sizeof(entry), 1, fp) != 1)
			FatalError("Error writing WAD directory.\n");
	}

	dir_crc = checksum.raw;
	DebugPrintf("dir_crc: %08x\n", dir_crc);

	fflush(fp);

	total_size = (int)ftell(fp);
	DebugPrintf("total_size: %d\n", total_size);

	if (total_size < 0)
		FatalError("Error determining WAD size.\n");
	
	// update header at start of file
	
	// FIXME!!!
}


#if 0  // NOT USED
void Wad_file::InvalidateHoles()
{
	int dest = 0;

	for (int i = 0; i < (int)holes.size(); i++)
	{
		Lump_c *hole = holes[i];
		SYS_ASSERT(hole);

		if (hole->l_start + hole->l_length >= total_size)
		{
			delete hole; continue;
		}

		// compact the array as we go...
		holes[dest++] = hole;
	}

	holes.resize(dest);
}
#endif


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
