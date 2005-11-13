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
//
#include "filesystem.h"
#include "strings.h"

namespace epi
{

// A Filesystem Directory

//
// filesystem_dir_c Constructor
//
filesystem_dir_c::filesystem_dir_c() : array_c(sizeof(filesystem_direntry_s)) 
{
}

// 
// filesystem_dir_c Deconstructor
//
filesystem_dir_c::~filesystem_dir_c()
{
}

//
// bool filesystem_dir_c::AddEntry()
//
bool filesystem_dir_c::AddEntry(filesystem_direntry_s *fs_entry)
{
	if (InsertObject(fs_entry)<0)
        return false;

	return true;
}

//
// filesystem_dir_c::CleanupObject
//
void filesystem_dir_c::CleanupObject(void *obj)
{
	filesystem_direntry_s *d = (filesystem_direntry_s*)obj;
	if (d->name)  { delete d->name; }
}

//
// filesystem_dir_c::operator[]
//
filesystem_direntry_s* filesystem_dir_c::operator[](int idx)
{
	return (filesystem_direntry_s*)FetchObject(idx);
}


// The Filesystem

//
// filesystem_c Constructor
//
filesystem_c::filesystem_c()
{
}

//
// filesystem_c Deconstructor
//
filesystem_c::~filesystem_c()
{
}

};

