//----------------------------------------------------------------------------
//  EDGE Filesystem Class
//----------------------------------------------------------------------------
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
//----------------------------------------------------------------------------
#ifndef __EPI_FILESYSTEM_CLASS__
#define __EPI_FILESYSTEM_CLASS__

#include "epi.h"
#include "arrays.h"

namespace epi
{

// Forward declarations
class file_c;
class string_c;

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

// The Filesystem
class filesystem_c
{
public:
	filesystem_c();
	virtual ~filesystem_c();

protected:
	
public:
	// Directory Functions
	virtual bool GetCurrDir(const char *dir, unsigned int bufsize) = 0; 
	virtual bool SetCurrDir(const char *dir) = 0;
	
	virtual bool IsDir(const char *dir) = 0;

	virtual bool MakeDir(const char *dir) = 0;
	virtual bool RemoveDir(const char *dir) = 0;
	
	virtual bool ReadDir(filesystem_dir_c *fsd, const char *dir, const char *mask) = 0;

	// File Functions
	virtual bool Access(const char *name, unsigned int flags) = 0;
	virtual bool Close(file_c *file) = 0;
	virtual bool Copy(const char *dest, const char *src) = 0;
	virtual bool Delete(const char *name) = 0;
	virtual file_c *Open(const char *name, unsigned int flags) = 0;
	virtual bool Rename(const char *oldname, const char *newname) = 0;
};

};
#endif /* __EPI_FILESYSTEM_CLASS__ */
