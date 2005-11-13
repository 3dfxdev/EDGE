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
#ifndef __EPI_FILE_CLASS__
#define __EPI_FILE_CLASS__ 

namespace epi
{

// Base class of the EDGE Platform Inferface File
class file_c
{
public:
	file_c() {};
	virtual ~file_c() {};

    // Access Types
    enum access_e
    {
        ACCESS_READ   = 0x1,
        ACCESS_WRITE  = 0x2,
        ACCESS_APPEND = 0x4,
        ACCESS_BINARY = 0x8
    };

	// Seek reference points
    enum seek_e
	{
		SEEKPOINT_START,
		SEEKPOINT_CURRENT,
		SEEKPOINT_END,
		SEEKPOINT_NUMTYPES
	};

    // Type
    enum type_e
    {
        TYPE_DISK,
        TYPE_MEMORY,
        TYPE_NUMTYPES
    };

protected:

public:
	virtual int GetLength(void) = 0;
	virtual int GetPosition(void) = 0;
    virtual int GetType(void) = 0;

	virtual unsigned int Read(void *dest, unsigned int size) = 0;
	virtual bool Read16BitInt(void *dest) = 0;
	virtual bool Read32BitInt(void *dest) = 0;

	virtual bool Seek(int offset, int seekpoint) = 0;

	virtual unsigned int Write(const void *src, unsigned int size) = 0;
	virtual bool Write16BitInt(void *src) = 0;
	virtual bool Write32BitInt(void *src) = 0;
};

};

#endif /* __EPI_FILE_CLASS__ */
