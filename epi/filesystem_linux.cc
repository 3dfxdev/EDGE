//----------------------------------------------------------------------------
//  EDGE Filesystem Class for Linux
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
#include "errors.h"
#include "strings.h"

#include "files_linux.h"
#include "filesystem_linux.h"

#include <dirent.h>
#include <fnmatch.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_MODE_CHARS 3

namespace epi
{

//
// epi_linux_filesystem_c Constructor
//
linux_filesystem_c::linux_filesystem_c()
{
}

//
// linux_filesystem_c Destructor
//
linux_filesystem_c::~linux_filesystem_c()
{
}

//
// linux_filesystem_c::ConvertFlagsToMode()
//
bool linux_filesystem_c::ConvertFlagsToMode(int flags, char *mode)
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

//
// bool linux_filesystem_c::GetCurrDir()
//
bool linux_filesystem_c::GetCurrDir(const char *dir, unsigned int bufsize)
{
	if (!dir)
		return false;

	return (getcwd((char *)dir, bufsize) != NULL);
}

//
// bool linux_filesystem_c::SetCurrDir()
//
bool linux_filesystem_c::SetCurrDir(const char *dir)
{
	if (!dir)
		return false;

	return (chdir(dir) == 0);
}

//
// bool linux_filesystem_c::IsDir()
// 
bool linux_filesystem_c::IsDir(const char *dir)
{
	if (!dir)
		return false;

	DIR *result = opendir(dir);

	if (result == NULL)
		return false;

	closedir(result);
	return true;
}

//
// bool linux_filesystem_c::MakeDir()
//
bool linux_filesystem_c::MakeDir(const char *dir)
{
	if (!dir)
		return false;

	return (mkdir(dir, 0775) == 0);
}

//
// bool linux_filesystem_c::RemoveDir()
//
bool linux_filesystem_c::RemoveDir(const char *dir)
{
	if (!dir)
		return false;

	return (rmdir(dir) == 0);
}

//
// bool linux_filesystem_c::ReadDir()
//
bool linux_filesystem_c::ReadDir(filesystem_dir_c *fsd, const char *dir, const char *mask)
{
	if (!dir || !fsd || !mask)
		return false;

	DIR *handle;
	char olddir[PATH_MAX];
	filesystem_direntry_s tmp_entry;
	struct dirent *fdata;

	handle = opendir(dir);
	if (handle == NULL)
		return false;

	GetCurrDir(olddir, PATH_MAX);
	SetCurrDir(dir);

	// Ensure the container is empty
	fsd->Clear();

	for (;;)
	{
		fdata = readdir(handle);
		if (fdata == NULL)
			break;

		if (fnmatch(mask, fdata->d_name, 0) != 0)
			continue;

		struct stat finfo;

		if (stat(fdata->d_name, &finfo) != 0)
			continue;
		
		tmp_entry.name = new string_c(fdata->d_name);
		tmp_entry.size = finfo.st_size;
		tmp_entry.dir = S_ISDIR(finfo.st_mode) ?true:false;

		if (!fsd->AddEntry(&tmp_entry))
		{
			closedir(handle);
			delete tmp_entry.name;
			SetCurrDir(olddir);
			return false;
		}
	}

	SetCurrDir(olddir);
	closedir(handle);

	return true;
}

//
// bool linux_filesystem_c::Access()
//
bool linux_filesystem_c::Access(const char *name, unsigned int flags)
{
    char mode[MAX_MODE_CHARS];
    FILE *fp;

    if (!ConvertFlagsToMode(flags, mode))
        return false;

    fp = fopen(name, mode);
    if (!fp)
        return false;

    fclose(fp);
    return true;
}

//
// bool linux_filesystem_c::Close()
//
bool linux_filesystem_c::Close(file_c *file)
{
    FILE *fp;

    if (!file)
        return false;

    if (file->GetType() == file_c::TYPE_DISK)
    {
        linux_file_c *diskfile = (linux_file_c*)file;

        fp = diskfile->GetDescriptor();
        if (fp)
            fclose(fp);

		//
		// We nullify the filesystem and filehandle to 
		// NULL: the prevents recursive Close() calls
		// since the diskfile deconstructor will
		// attempt to close the file if the filesystem
		// exists.
		// 
		diskfile->Setup(NULL, NULL);
    }

    delete file;
	return true;
}

//
// bool linux_filesystem_c::Copy()
//
bool linux_filesystem_c::Copy(const char *dest, const char *src)
{
	file_c *dest_file;
	file_c *src_file;
	bool ok;
	
	unsigned char *buf;
	int cpsize, size;
	
	dest_file = NULL;
	src_file = NULL;
	buf = NULL;
	ok = true;
	
	try
	{
		src_file = Open(src, file_c::ACCESS_READ);
		if (!src_file)
		{
			throw error_c(EPI_ERRGEN_FOOBAR);
		}	
	
		dest_file = Open(dest, file_c::ACCESS_WRITE);
		if (!dest_file)
		{
			throw error_c (EPI_ERRGEN_FOOBAR);
		}
		
		buf = new unsigned char[COPY_BUF_SIZE];
		if (!buf)
		{
        	throw error_c(EPI_ERRGEN_FOOBAR);
		}

		size = src_file->GetLength();
		while (size > 0)
		{
			if (size > COPY_BUF_SIZE)
				cpsize = COPY_BUF_SIZE;
			else
				cpsize = size;
			
			if (src_file->Read(buf, cpsize) != (unsigned int)cpsize)
			{
				throw error_c(EPI_ERRGEN_FOOBAR);
			}
	
			if (dest_file->Write(buf, cpsize) != (unsigned int)cpsize)
			{
				throw error_c(EPI_ERRGEN_FOOBAR);
			}
	
			size -= cpsize;
		}
	}
	catch(epi::error_c err)
	{
		ok = false;
	}

	if (src_file)
		Close(src_file);

	if (dest_file)
		Close(dest_file);

	if (buf)
		delete [] buf;

	return ok;
}

//
// bool linux_filesystem_c::Delete()
//
bool linux_filesystem_c::Delete(const char *name)
{
    if (!name)
        return false;

    return !(unlink(name))?true:false;
}

//
// file_c* linux_filesystem_c::Open()
//
file_c* linux_filesystem_c::Open(const char *name, 
                                 unsigned int flags)
{
    char mode[MAX_MODE_CHARS];
    linux_file_c *file;
    FILE *fp;

    if (!name)
        return NULL;

    if (!ConvertFlagsToMode(flags, mode))
        return NULL;

    fp = fopen(name, mode);
    if (!fp)
        return NULL;

    file = new linux_file_c;
    if (!file)
    {
        fclose(fp);
        return NULL;
    }

    file->Setup(this, fp);
    return file;
}

//
// bool linux_filesystem_c::Rename()
//
bool linux_filesystem_c::Rename(const char *oldname, 
                                        const char *newname)
{
	if (!oldname || !newname)
	{
		return false;
	}

	return (bool)(rename(oldname, newname) != -1);
}

// Path Manipulation Functions

//
// bool linux_filesystem_c::IsAbsolutePath()
//
bool linux_filesystem_c::IsAbsolutePath(const char *path)
{
    // FIXME: Catch null parameters

	if (path[0] == '/')
		return true;

    return false;
}

//
// string_c linux_filesystem_c::JoinPath()
//
string_c linux_filesystem_c::JoinPath(const char *lhs, const char *rhs)
{
    epi::string_c s;

    // FIXME: Catch null parameters

    if (IsAbsolutePath(rhs))
    {
        s.Set(rhs);
        return s;
    }

    s.Set(lhs);

    if (s.GetLastChar() != '/')
        s.AddChar('/');

    s.AddString(rhs);

    return s;
}

};
