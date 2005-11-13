//----------------------------------------------------------------------------
//  EDGE Filesystem Class for Win32
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
#include "epi.h"
#include "strings.h"

#include "files_win32.h"
#include "filesystem_win32.h"

#define MAX_MODE_CHARS 4

namespace epi
{

struct createfile_s
{
	DWORD dwDesiredAccess;                      // access mode
	DWORD dwShareMode;                          // share mode
	DWORD dwCreationDisposition;                // how to create
};

//
// win32_filesystem_c Constructor
//
win32_filesystem_c::win32_filesystem_c()
{
}

//
// win32_filesystem_c Deconstructor
//
win32_filesystem_c::~win32_filesystem_c()
{
}

//
// win32_filesystem_c::BuildFileStruct()
//
bool win32_filesystem_c::BuildFileStruct(int epiflags, createfile_s* finfo)
{
    // Must have some value in epiflags
    if (epiflags == 0)
        return false;

	// Sanity checking...
	if (!finfo)
		return false;			// Clearly mad

    // Check for any invalid combinations
    if ((epiflags & file_c::ACCESS_WRITE) && (epiflags & file_c::ACCESS_APPEND))
        return false;


    if (epiflags & file_c::ACCESS_READ)
    {
		if (epiflags & file_c::ACCESS_WRITE)
		{
			finfo->dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
			finfo->dwShareMode = 0;
			finfo->dwCreationDisposition = OPEN_ALWAYS;
		}
		else if (epiflags & file_c::ACCESS_APPEND)
		{
			finfo->dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
			finfo->dwShareMode = 0;
			finfo->dwCreationDisposition = OPEN_EXISTING;
		}
		else
		{
			finfo->dwDesiredAccess = GENERIC_READ;
			finfo->dwShareMode = FILE_SHARE_READ;
			finfo->dwCreationDisposition = OPEN_EXISTING;
		}
    }
    else
    {
		if (epiflags & file_c::ACCESS_WRITE)
		{
			finfo->dwDesiredAccess = GENERIC_WRITE;
			finfo->dwShareMode = 0;
			finfo->dwCreationDisposition = OPEN_ALWAYS;
		}
		else if (epiflags & file_c::ACCESS_APPEND)
		{
			finfo->dwDesiredAccess = GENERIC_WRITE;
			finfo->dwShareMode = 0;
			finfo->dwCreationDisposition = OPEN_EXISTING;
		}
		else
		{
			return false;
		}
	}
    
    return true;
}


//
// bool win32_filesystem_c::GetCurrDir()
//
bool win32_filesystem_c::GetCurrDir(const char *dir, unsigned int bufsize)
{
	if (!dir)
		return false;

	return (::GetCurrentDirectory(bufsize, (LPSTR)dir) != FALSE);
}

//
// bool win32_filesystem_c::SetCurrDir()
//
bool win32_filesystem_c::SetCurrDir(const char *dir)
{
	if (!dir)
		return false;

	return (::SetCurrentDirectory(dir) != FALSE);
}

//
// bool win32_filesystem_c::IsDir()
// 
bool win32_filesystem_c::IsDir(const char *dir)
{
	if (!dir)
		return false;

	bool result;
	char curpath[MAX_PATH];

	GetCurrDir(curpath, MAX_PATH);

	result = SetCurrDir(dir);

	SetCurrDir(curpath);
	return result;
}

//
// bool win32_filesystem_c::MakeDir()
//
bool win32_filesystem_c::MakeDir(const char *dir)
{
	if (!dir)
		return false;

	return (::CreateDirectory(dir, NULL) != FALSE);
}

//
// bool win32_filesystem_c::RemoveDir()
//
bool win32_filesystem_c::RemoveDir(const char *dir)
{
	if (!dir)
		return false;

	return (::RemoveDirectory(dir) != FALSE);
}

//
// bool win32_filesystem_c::ReadDir()
//
bool win32_filesystem_c::ReadDir(filesystem_dir_c *fsd, const char *dir, const char *mask)
{
	if (!dir || !fsd || !mask)
		return false;

	filesystem_direntry_s tmp_entry;
	string_c curr_dir;
	HANDLE handle;
	WIN32_FIND_DATA fdata;

	// Bit of scope for the dir buffer to be held on stack 
	// for the minimum amount of time
	{
		char tmp[MAX_PATH+1];
		
		if (!GetCurrDir(tmp, MAX_PATH))
			return false;

		curr_dir = tmp;
	}

	if (!SetCurrDir(dir))
		return false;

	handle = FindFirstFile(mask, &fdata);
	if (handle == INVALID_HANDLE_VALUE)
		return false;

	// Ensure the container is empty
	fsd->Clear();

	do
	{
		tmp_entry.name = new string_c(fdata.cFileName);
		if (!tmp_entry.name)
			return false;

		tmp_entry.size = fdata.nFileSizeLow;
		tmp_entry.dir = (fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)?true:false; 

		if (!fsd->AddEntry(&tmp_entry))
		{
			delete tmp_entry.name;
			return false;
		}
	}
	while(FindNextFile(handle, &fdata));

	FindClose(handle);

	SetCurrDir(curr_dir.GetString());
	return true;
}

//
// bool win32_filesystem_c::Access()
//
bool win32_filesystem_c::Access(const char *name, unsigned int flags)
{
	createfile_s fs;
	HANDLE handle;

	if (!BuildFileStruct(flags, &fs))
		return false;

	handle = CreateFile(name, fs.dwDesiredAccess, fs.dwShareMode, 
							NULL, fs.dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, NULL);

	if (handle == INVALID_HANDLE_VALUE)	
		return false;

	CloseHandle(handle);
	return true;
}

//
// bool win32_filesystem_c::Close()
//
bool win32_filesystem_c::Close(file_c *file)
{
    if (!file)
        return false;

    if (file->GetType() == file_c::TYPE_DISK)
    {
		HANDLE handle;

        win32_file_c *diskfile = (win32_file_c*)file;

        handle = diskfile->GetDescriptor();
        if (handle != INVALID_HANDLE_VALUE)
            CloseHandle(handle);

        diskfile->Setup(NULL, INVALID_HANDLE_VALUE);
    }

    delete file;
	return true;
}

//
// bool win32_filesystem_c::Copy()
//
bool win32_filesystem_c::Copy(const char *dest, const char *src)
{
	if (!dest || !src)
	{
		return false;
	}

	// Copy src to dest overwriting dest if it exists
	return (::CopyFile(src, dest, FALSE) != FALSE);
}

//
// bool win32_filesystem_c::Delete()
//
bool win32_filesystem_c::Delete(const char *name)
{
	if (!name)
        return false;

	return (::DeleteFile(name) != FALSE);
}

//
// file_c* win32_filesystem_c::Open()
//
file_c* win32_filesystem_c::Open(const char *name, unsigned int flags)
{
	createfile_s fs;
	win32_file_c *file;
	HANDLE handle;

    if (!name)
        return NULL;

	if (!BuildFileStruct(flags, &fs))
		return false;

	handle = CreateFile(name, fs.dwDesiredAccess, fs.dwShareMode, 
							NULL, fs.dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, NULL);

	if (handle == INVALID_HANDLE_VALUE)	
		return false;

    file = new win32_file_c;
    if (!file)
    {
        CloseHandle(handle);
        return NULL;
    }

    file->Setup(this, handle);
    return file;
}

//
// bool win32_filesystem_c::Rename()
//
bool win32_filesystem_c::Rename(const char *oldname, const char *newname)
{
	if (!oldname || !newname)
	{
		return false;
	}

	return (::MoveFile(oldname, newname) != FALSE);
}

};
