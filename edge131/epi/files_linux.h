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
#ifndef __LINUX_EPI_FILE_CLASS__
#define __LINUX_EPI_FILE_CLASS__

#include "epi.h"
#include "files.h"

#include <stdio.h>

namespace epi
{

class linux_file_c: public file_c
{
public:
    linux_file_c();
    ~linux_file_c();

protected:
    filesystem_c *filesystem;
    FILE *file;
    
public:
    // Config
    FILE *GetDescriptor(void);
    void Setup(filesystem_c* _filesystem, FILE * _file);

    // Overrides
    int GetLength(void);
    int GetPosition(void);
    int GetType(void) { return TYPE_DISK; }  

    unsigned int Read(void *dest, unsigned int size);
    bool Read16BitInt(void *dest);
    bool Read32BitInt(void *dest);

    bool Seek(int offset, int seekpoint);

    unsigned int Write(const void *src, unsigned int size);
    bool Write16BitInt(void *src);
    bool Write32BitInt(void *src);
};

};
#endif
