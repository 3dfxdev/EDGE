//----------------------------------------------------------------------------
//  Linux EPI System Specifics
//----------------------------------------------------------------------------
//
//  Copyright (c) 2002-2007  The EDGE Team.
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
#include "epi.h"
#include "filesystem.h"

#include "filesystem_linux.h"

namespace epi
{
	filesystem_c *the_filesystem = NULL;

	static bool inited = false;

    //
    // Init
    //
    bool Init(void)
    {
		bool ok;

		linux_filesystem_c* fs;

		if (inited)
			Shutdown();

		ok = true;


		if (ok)
		{
			fs = new linux_filesystem_c;
			if (fs) 
				the_filesystem = fs;
			else
				ok = false;
		}

		inited = ok;
        return ok;
    }

    //
    // Shutdown
    //
    void Shutdown(void)
    {
		if (the_filesystem)
		{
			delete the_filesystem;
			the_filesystem = NULL;
		}

		inited = false;
    }

} // namespace epi

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
