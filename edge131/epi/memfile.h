//----------------------------------------------------------------------------
//  Block of memory with File interface
//----------------------------------------------------------------------------
//
//  Copyright (c) 2005  The EDGE Team.
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
#ifndef __EPI_MEMFILE_H__
#define __EPI_MEMFILE_H__

#include "epi.h"
#include "files.h"

namespace epi
{

class mem_file_c : public file_c
{
public:
    mem_file_c(const byte *_block, int _len, bool copy_it = true);
    ~mem_file_c();

protected:
	byte *data;
	int length;
	int pos;
	bool copied;

public:
    // Overrides
    int GetLength(void) { return length; }
    int GetPosition(void) { return pos; }
    int GetType(void) { return TYPE_MEMORY; }  

    unsigned int Read(void *dest, unsigned int size);
    bool Read16BitInt(void *dest);
    bool Read32BitInt(void *dest);

    bool Seek(int offset, int seekpoint);

    unsigned int Write(const void *src, unsigned int size);
    bool Write16BitInt(void *src);
    bool Write32BitInt(void *src);

public:
	// HACK ALERT !!!
	byte *GetNonCopiedDataPointer() const
	{
		// ASSERT(! copied);
		return data;
	}
};

}; // namespace epi

#endif /* __EPI_MEMFILE_H__ */
