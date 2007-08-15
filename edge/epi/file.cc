//----------------------------------------------------------------------------
//  EDGE File Class for Linux
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

#include "epi.h"

#include "file.h"

namespace epi
{

ansi_file_c::ansi_file_c(FILE *_filep) : fp(_filep)
{ }

ansi_file_c::~ansi_file_c()
{
    if (fp)
	{
		fclose(fp);
		fp = NULL;
    }
}

int ansi_file_c::GetLength()
{
	SYS_ASSERT(fp);

    long currpos = ftell(fp);      // Get existing position

    fseek(fp, 0, SEEK_END);        // Seek to the end of file
    long len = ftell(fp);          // Get the position - it our length

    fseek(fp, currpos, SEEK_SET);  // Reset existing position
    return (int)len;
}

int ansi_file_c::GetPosition()
{
	SYS_ASSERT(fp);

    return (int)ftell(fp);
}

unsigned int ansi_file_c::Read(void *dest, unsigned int size)
{
	SYS_ASSERT(fp);
	SYS_ASSERT(dest);

    return fread(dest, 1, size, fp);
}

unsigned int ansi_file_c::Write(const void *src, unsigned int size)
{
	SYS_ASSERT(fp);
	SYS_ASSERT(src);

    return fwrite(src, 1, size, fp);
}

bool ansi_file_c::Seek(int offset, int seekpoint)
{
    int whence;

    switch (seekpoint)
    {
        case SEEKPOINT_START:   { whence = SEEK_SET; break; }
        case SEEKPOINT_CURRENT: { whence = SEEK_CUR; break; }
        case SEEKPOINT_END:     { whence = SEEK_END; break; }

        default:
			I_Error("ansi_file_c::Seek : illegal seekpoint value.\n");
            return false; /* NOT REACHED */
    }

    return (fseek(fp, offset, whence) == 0);
}


// utility functions...

bool FS_FlagsToAnsiMode(int flags, char *mode)
{
    // Must have some value in epiflags
    if (flags == 0)
        return false;

    // Check for any invalid combinations
    if ((flags & file_c::ACCESS_WRITE) && (flags & file_c::ACCESS_APPEND))
        return false;

    if (flags & file_c::ACCESS_READ)
    {
        if (flags & file_c::ACCESS_WRITE) 
            strcpy(mode, "w+");                        // Read/Write
        else if (flags & file_c::ACCESS_APPEND)
            strcpy(mode, "a+");                        // Read/Append
        else
            strcpy(mode, "r");                         // Read
    }
    else
    {
        if (flags & file_c::ACCESS_WRITE)       
            strcpy(mode, "w");                         // Write
        else if (flags & file_c::ACCESS_APPEND) 
            strcpy(mode, "a");                         // Append
        else                                         
            return false;                              // Invalid
    }
    
    return true;
}

} // namespace epi

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
