//----------------------------------------------------------------------------
//  EDGE File Class for Linux
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
#include "files_linux.h"
#include "filesystem_linux.h"

namespace epi
{

//
// files_linux_c Constructor
//
linux_file_c::linux_file_c() : filesystem(NULL), file(NULL)
{
}

//
// files_linux_c Destructor
//
linux_file_c::~linux_file_c()
{
    if (filesystem)
        filesystem->Close(this);
}

//
// FILE* linux_file_c::GetDescriptor()
//    
FILE* linux_file_c::GetDescriptor(void)
{
    // Probably could be inlined. Will change if no addition checking is
    // needed.
    return file;
}

//
// linux_file_c::Setup()
//
void linux_file_c::Setup(filesystem_c* _filesystem, FILE *_file)
{
    filesystem = _filesystem;
    file = _file;
}

//
// linux_file_c::GetLength()
//
int linux_file_c::GetLength(void)
{
    long currpos;
    long len;

    if (!file)
        return -1;

    currpos = ftell(file);           // Get existing position

    fseek(file, 0, SEEK_END);        // Seek to the end of file
    len = ftell(file);               // Get the position - it our length

    fseek(file, currpos, SEEK_SET);  // Reset existing position
    return (int)len;
}

//
// linux_file_c::GetPosition()
//
int linux_file_c::GetPosition(void)
{
    if (!file)
        return -1;

    return (int)ftell(file);
}

//
// unsigned int linux_file_c::Read()
//
unsigned int linux_file_c::Read(void *dest, unsigned int size)
{
    if (!dest || !file)
        return false;

    return fread(dest, 1, size, file);
}

//
// unsigned int linux_file_c::Read16BitInt()
//
bool linux_file_c::Read16BitInt(void *dest)
{
    if (!dest || !file)
        return false;

    return (fread(dest, sizeof(short), 1, file) == 1);
}

//
// unsigned int linux_file_c::Read32BitInt()
//
bool linux_file_c::Read32BitInt(void *dest)
{
    if (!dest || !file)
        return 0;

    return (fread(dest, sizeof(int), 1, file) == 1);
}

//
// bool linux_file_c::Seek()
//
bool linux_file_c::Seek(int offset, int seekpoint)
{
    int whence;

    switch (seekpoint)
    {
        case SEEKPOINT_START:   { whence = SEEK_SET; break; }
        case SEEKPOINT_CURRENT: { whence = SEEK_CUR; break;  }
        case SEEKPOINT_END:     { whence = SEEK_END; break;  }

        default:
        {
            return false;
            break;
        }
    }

    return (fseek(file, offset, whence) == 0);
}

//
// unsigned int linux_file_c::Write()
//
unsigned int linux_file_c::Write(const void *src, unsigned int size)
{
    if (!src || !file)
        return false;

    return fwrite(src, 1, size, file);
}

//
// unsigned int linux_file_c::Write16BitInt()
//
bool linux_file_c::Write16BitInt(void *src)
{
    if (!src || !file)
        return false;

    return (fwrite(src, sizeof(short), 1, file) == 1);
}

//
// unsigned int linux_file_c::Write32BitInt()
//
bool linux_file_c::Write32BitInt(void *src)
{
    if (!src || !file)
        return false;

    return (fwrite(src, sizeof(int), 1, file) == 1);
}

};
