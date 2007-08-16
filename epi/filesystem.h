//----------------------------------------------------------------------------
//  EDGE Filesystem API
//----------------------------------------------------------------------------
//
//  Copyright (c) 2003-2007  The EDGE Team.
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
//----------------------------------------------------------------------------

#ifndef __EPI_FILESYSTEM_H__
#define __EPI_FILESYSTEM_H__

#include "arrays.h"
#include "strings.h"
#include "timestamp.h"

namespace epi
{

// Forward declarations
class file_c;

// A Filesystem directory entry
struct filesystem_direntry_s
{
	filesystem_direntry_s()
	{
		name = NULL;
		size = 0;
		dir = false;
	}

	string_c *name;
	unsigned int size;
	bool dir;
};

// A Filesystem directory
class filesystem_dir_c : public array_c
{
public:
	filesystem_dir_c();
	~filesystem_dir_c();

private:
	void CleanupObject(void *obj);
		
public:
	bool AddEntry(filesystem_direntry_s* fs_entry);

	int GetSize(void) { return array_entries; }

	filesystem_direntry_s* operator[](int idx); 	
};

// ---- The Filesystem ----

// Directory Functions
bool FS_GetCurrDir(char *dir, unsigned int bufsize); 
bool FS_SetCurrDir(const char *dir);

bool FS_IsDir(const char *dir);
bool FS_MakeDir(const char *dir);
bool FS_RemoveDir(const char *dir);

bool FS_ReadDir(filesystem_dir_c *fsd, const char *dir, const char *mask);

// File Functions
bool FS_Access(const char *name, unsigned int flags);
file_c *FS_Open(const char *name, unsigned int flags);

// NOTE: there's no FS_Close() function, just delete the object.

bool FS_Copy(const char *dest, const char *src);
bool FS_Delete(const char *name);
bool FS_Rename(const char *oldname, const char *newname);

bool FS_GetModifiedTime(const char *filename, timestamp_c& t);

} // namespace epi

#endif /*__EPI_FILESYSTEM_H__*/

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
