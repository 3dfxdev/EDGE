//----------------------------------------------------------------------------
//  EDGE Filesystem Class for Win32 Header
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
//
#ifndef __WIN32_EPI_FILESYSTEM_CLASS__
#define __WIN32_EPI_FILESYSTEM_CLASS__

#include "filesystem.h"

namespace epi
{
	
struct createfile_s;

class win32_filesystem_c : public filesystem_c
{
public:
	win32_filesystem_c();
	virtual ~win32_filesystem_c();

protected:
	bool BuildFileStruct(int epiflags, createfile_s* finfo);
	
public:
	// Basic Directory Functions
	bool GetCurrDir(const char *dir, unsigned int bufsize); 
	bool SetCurrDir(const char *dir);

	bool IsDir(const char *dir);

	bool MakeDir(const char *dir);
	bool RemoveDir(const char *dir);

	bool ReadDir(filesystem_dir_c *fsd, const char *dir, const char *mask);

	// Basic File Functions
	bool Access(const char *name, unsigned int flags);
	bool Close(file_c *file);
	bool Copy(const char *dest, const char *src);
	bool Delete(const char *name);
	file_c *Open(const char *name, unsigned int flags);
	bool Rename(const char *oldname, const char *newname);	

    // Path Manipulation Functions
    bool IsAbsolutePath(const char *path);
    string_c JoinPath(const char *lhs, const char *rhs);
};

};
#endif /* __WIN32_EPI_FILESYSTEM_CLASS__ */

