//----------------------------------------------------------------------------
//  EDGE File Class for Win32
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
#include "files_win32.h"
#include "filesystem_win32.h"

#include "types.h"

namespace epi
{

//
// files_win32_c Constructor
//
win32_file_c::win32_file_c() : filesystem(NULL), filehandle(INVALID_HANDLE_VALUE)
{
}

//
// files_win32_c Destructor
//
win32_file_c::~win32_file_c()
{
    if (filesystem)
        filesystem->Close(this);
}

//
// HANDLE win32_file_c::GetDescriptor()
//    
HANDLE win32_file_c::GetDescriptor(void)
{
    // Probably could be inlined. Will change if no addition checking is
    // needed.
    return filehandle;
}

//
// win32_file_c::Setup()
//
void win32_file_c::Setup(filesystem_c* _filesystem, HANDLE _filehandle)
{
    filesystem = _filesystem;
    filehandle = _filehandle;
}

//
// win32_file_c::GetLength()
//
int win32_file_c::GetLength(void)
{
	DWORD size;

	if (filehandle == INVALID_HANDLE_VALUE)
        return -1;

	size = GetFileSize(filehandle, NULL);
	if (size == INVALID_FILE_SIZE)
		return -1;

	return (int)size;
}

//
// win32_file_c::GetPosition()
//
int win32_file_c::GetPosition(void)
{
	DWORD pos;

	if (filehandle == INVALID_HANDLE_VALUE)
        return -1;

	pos = SetFilePointer(filehandle, 0, NULL, FILE_CURRENT);  // provides offset from current position 
	if (pos == INVALID_SET_FILE_POINTER)
		return -1;

	return (int)pos;
}

//
// bool win32_file_c::Read()
//
unsigned int win32_file_c::Read(void *dest, unsigned int size)
{
	DWORD read_len;

    if (!dest || filehandle == INVALID_HANDLE_VALUE)
        return 0;

    if (ReadFile(filehandle, dest, size, &read_len, NULL) == 0)
		return 0;
	
	return read_len;
}

//
// bool win32_file_c::Read16BitInt()
//
bool win32_file_c::Read16BitInt(void *dest)
{
	DWORD read_len;

    if (!dest || filehandle == INVALID_HANDLE_VALUE)
        return false;

    return (ReadFile(filehandle, dest, sizeof(u16_t), &read_len, NULL) != 0);
}

//
// bool win32_file_c::Read32BitInt()
//
bool win32_file_c::Read32BitInt(void *dest)
{
	DWORD read_len;

    if (!dest || filehandle == INVALID_HANDLE_VALUE)
        return false;

    return (ReadFile(filehandle, dest, sizeof(u32_t), &read_len, NULL) != 0);
}

//
// bool win32_file_c::Seek()
//
bool win32_file_c::Seek(int offset, int seekpoint)
{
    DWORD whence;

    switch (seekpoint)
    {
        case SEEKPOINT_START:   { whence = FILE_BEGIN; break;	}
        case SEEKPOINT_CURRENT: { whence = FILE_CURRENT; break;	}
        case SEEKPOINT_END:     { whence = FILE_END; break;	}

        default:
        {
            return false;
            break;
        }
    }

    return (SetFilePointer(filehandle, (LONG)offset, NULL, whence) != INVALID_SET_FILE_POINTER);
}

//
// bool win32_file_c::Write()
//
unsigned int win32_file_c::Write(const void *src, unsigned int size)
{
	DWORD write_len;

    if (!src || filehandle == INVALID_HANDLE_VALUE)
        return 0;

    if (WriteFile(filehandle, src, size, &write_len, NULL) == 0)
		return 0;

	return write_len;
}

//
// bool win32_file_c::Write16BitInt()
//
bool win32_file_c::Write16BitInt(void *src)
{
	DWORD write_len;

    if (!src || filehandle == INVALID_HANDLE_VALUE)
        return false;

    return (WriteFile(filehandle, src, sizeof(u16_t), &write_len, NULL) != 0);
}

//
// bool win32_file_c::Write32BitInt()
//
bool win32_file_c::Write32BitInt(void *src)
{
	DWORD write_len;

    if (!src || filehandle == INVALID_HANDLE_VALUE)
        return false;

    return (WriteFile(filehandle, src, sizeof(u32_t), &write_len, NULL) != 0);
}

};
