//----------------------------------------------------------------------------
//  EDGE Filesystem Class for Linux Header
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
#ifndef __LINUX_EPI_FILESYSTEM_CLASS__
#define __LINUX_EPI_FILESYSTEM_CLASS__

#include "filesystem.h"

namespace epi
{

class linux_filesystem_c : public filesystem_c
{
public:
	linux_filesystem_c();
	virtual ~linux_filesystem_c();

protected:
    bool ConvertFlagsToMode(int flags, char *mode);
	
	static const int COPY_BUF_SIZE = 1024;
	
public:
	// Directory Functions
	bool GetCurrDir(const char *dir, unsigned int bufsize); 
	bool SetCurrDir(const char *dir);
	bool IsDir(const char *dir);
	bool MakeDir(const char *dir);
	bool RemoveDir(const char *dir);
	bool ReadDir(filesystem_dir_c *fsd, const char *dir, const char *mask);

	// File Functions
	bool Access(const char *name, unsigned int flags);
	bool Close(file_c *file);
	bool Copy(const char *dest, const char *src);
	bool Delete(const char *name);
	file_c *Open(const char *name, unsigned int flags);
	bool Rename(const char *oldname, const char *newname);
};

};
#endif /* __LINUX_EPI_FILESYSTEM_CLASS__ */
