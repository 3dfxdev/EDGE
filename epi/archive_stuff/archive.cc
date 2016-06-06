//------------------------------------------------------------------------
//  EPI Archive interface
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

#include "epi.h"
#include "archive.h"

namespace epi
{

/* archive_dir_c helper class */

archive_entry_c* archive_dir_c::operator[](int idx) const
{
	if (idx < 0 || idx >= NumEntries())
		return NULL;
	
	return *(archive_entry_c**)FetchObject(idx);
}

archive_entry_c *archive_dir_c::operator[](const char *name) const
{
	for (int i = 0; i < NumEntries(); i++)
	{
		archive_entry_c *entry = operator[](i);

		if (strcmp(entry->getName(), name) == 0)
			return entry;
	}

	return NULL;  /* not found */
}

bool archive_dir_c::Delete(int idx)
{
	if (idx < 0 || idx >= NumEntries())
		return false;
	
	RemoveObject(idx);
	return true;
}

}  // namespace epi

