//----------------------------------------------------------------------------
//  EDGE Filesystem Class for Linux
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
//
#include "epi.h"
#include "strings.h"

#include "filesystem.h"

#include <dirent.h>
#include <fnmatch.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_MODE_CHARS 3

#define COPY_BUF_SIZE  1024


namespace epi
{

static bool ConvertFlagsToMode(int flags, char *mode)
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


bool FS_GetCurrDir(const char *dir, unsigned int bufsize)
{
	SYS_ASSERT(dir);

	return (getcwd((char *)dir, bufsize) != NULL);
}

bool FS_SetCurrDir(const char *dir)
{
	SYS_ASSERT(dir);

	return (chdir(dir) == 0);
}

bool FS_IsDir(const char *dir)
{
	SYS_ASSERT(dir);

	DIR *result = opendir(dir);

	if (result == NULL)
		return false;

	closedir(result);
	return true;
}

bool FS_MakeDir(const char *dir)
{
	SYS_ASSERT(dir);

	return (mkdir(dir, 0775) == 0);
}

bool FS_RemoveDir(const char *dir)
{
	SYS_ASSERT(dir);

	return (rmdir(dir) == 0);
}

bool FS_ReadDir(filesystem_dir_c *fsd, const char *dir, const char *mask)
{
	SYS_ASSERT(fsd);
	SYS_ASSERT(dir);
	SYS_ASSERT(mask);

	DIR *handle;
	char olddir[PATH_MAX];
	filesystem_direntry_s tmp_entry;
	struct dirent *fdata;

	handle = opendir(dir);
	if (handle == NULL)
		return false;

	FS_GetCurrDir(olddir, PATH_MAX);
	FS_SetCurrDir(dir);

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
			FS_SetCurrDir(olddir);
			return false;
		}
	}

	FS_SetCurrDir(olddir);
	closedir(handle);

	return true;
}

bool FS_Access(const char *name, unsigned int flags)
{
	SYS_ASSERT(name);

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


bool FS_Copy(const char *dest, const char *src)
{
	SYS_ASSERT(dest);
	SYS_ASSERT(src);

	bool ok = false;

	file_c *dest_file = NULL;
	file_c *src_file  = NULL;

	unsigned char *buf = NULL;

	int size;
	int pkt_len;

	src_file = FS_Open(src, file_c::ACCESS_READ);
	if (! src_file)
		goto error_occurred;

	dest_file = FS_Open(dest, file_c::ACCESS_WRITE);
	if (! dest_file)
		goto error_occurred;

	buf = new unsigned char[COPY_BUF_SIZE];
	SYS_ASSERT(buf);

	size = src_file->GetLength();

	while (size > 0)
	{
		pkt_len = MIN(size, COPY_BUF_SIZE);

		if (src_file->Read(buf, pkt_len) != (unsigned int)pkt_len)
			goto error_occurred;

		if (dest_file->Write(buf, pkt_len) != (unsigned int)pkt_len)
			goto error_occurred;

		size -= pkt_len;
	}

	ok = true;

error_occurred:

	if (src_file)
		FS_Close(src_file);

	if (dest_file)
		FS_Close(dest_file);

	if (buf)
		delete[] buf;

	return ok;
}

bool FS_Delete(const char *name)
{
	SYS_ASSERT(name);

    return (unlink(name) == 0);
}

file_c* FS_Open(const char *name, 
                                 unsigned int flags)
{
	SYS_ASSERT(name);

    char mode[MAX_MODE_CHARS];
    linux_file_c *file;
    FILE *fp;

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

bool FS_Rename(const char *oldname, 
                                        const char *newname)
{
	SYS_ASSERT(oldname);
	SYS_ASSERT(newname);

	return (rename(oldname, newname) != -1);
}

} // namespace epi

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
