//----------------------------------------------------------------------------
//  Win32 EDGE System Specifics
//----------------------------------------------------------------------------
//
//  Copyright (c) 2002-2005  The EDGE Team.
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
#include "memmanager.h"

#include "filesystem_win32.h"

namespace epi
{
	filesystem_c *the_filesystem;
	mem_manager_c *the_mem_manager;

	const unsigned int max_mem_size = 33554432; // 32Mb 
	static bool inited = false;

    //
    // Init
    //
    bool Init(void)
    {
		bool ok;
		mem_manager_c* mm = NULL;
		win32_filesystem_c* fs = NULL;

		if (inited)
			Shutdown();

		ok = true;

		if (ok)
		{
			mm = new mem_manager_c(max_mem_size);
			if (mm)
				the_mem_manager = mm;
			else
				ok = false;
		}

		if (ok)
		{
			fs = new win32_filesystem_c;
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

		if (the_mem_manager)
		{
			delete the_mem_manager;
			the_mem_manager = NULL;
		}

		inited = false;
    }
};
