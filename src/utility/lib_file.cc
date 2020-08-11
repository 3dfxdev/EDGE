//------------------------------------------------------------------------
//  File Utilities
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2006-2016 Andrew Apted
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
//------------------------------------------------------------------------

#include <functional>
#include <iostream>

#include "system/i_defs.h"

#ifdef WIN32
#include <io.h>
#include "m_strings.h"
#else // UNIX or MACOSX
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#ifdef __APPLE__
#include <sys/param.h>
#include <mach-o/dyld.h> // _NSGetExecutablePath
#endif

#ifndef PATH_MAX
#define PATH_MAX  2048
#endif

#include "../hdr_fltk.h"

#define FatalError I_Error


bool FileExists(const char *filename)
{
	FILE *fp = fopen(filename, "rb");

	if (fp)
	{
		fclose(fp);
		return true;
	}

	return false;
}


bool HasExtension(const char *filename)
{
	int A = (int)strlen(filename) - 1;

	if (A > 0 && filename[A] == '.')
		return false;

	for (; A >= 0 ; A--)
	{
		if (filename[A] == '.')
			return true;

		if (filename[A] == '/')
			break;

#ifdef WIN32
		if (filename[A] == '\\' || filename[A] == ':')
			break;
#endif
	}

	return false;
}

//
// MatchExtension
//
// When ext is NULL, checks if the file has no extension.
//
bool MatchExtension(const char *filename, const char *ext)
{
	if (! ext)
		return ! HasExtension(filename);

	int A = (int)strlen(filename) - 1;
	int B = (int)strlen(ext) - 1;

	for (; B >= 0 ; B--, A--)
	{
		if (A < 0)
			return false;

		if (toupper(filename[A]) != toupper(ext[B]))
			return false;
	}

	return (A >= 1) && (filename[A] == '.');
}


//
// ReplaceExtension
//
// When ext is NULL, any existing extension is removed.
//
// Returned string is a COPY.
//
SString ReplaceExtension(const char *filename, const char *ext)
{
	SYS_ASSERT(filename[0] != 0);

	size_t total_len = strlen(filename) + (ext ? strlen(ext) : 0);

	SString buffer;
	buffer.reserve(total_len + 10);
	buffer = filename;

	size_t dot_pos;
#ifdef WIN32
	dot_pos = buffer.find_last_of("./\\:");
#else
	dot_pos = buffer.find_last_of("./");
#endif

	if(dot_pos != SString::npos && buffer[(int)dot_pos] != '.')
		dot_pos = SString::npos;
	if (! ext)
	{
		if(dot_pos != SString::npos)
			buffer.erase(dot_pos);
		return buffer;
	}

	if (dot_pos != SString::npos && dot_pos != buffer.length() - 1)
		buffer.erase(dot_pos + 1);
	else
		buffer += '.';
	buffer += ext;
	return buffer;
}

const char *FindBaseName(const char *filename)
{
	// Find the base name of the file (i.e. without any path).
	// The result always points within the given string.
	//
	// Example:  "C:\Foo\Bar.wad"  ->  "Bar.wad"

	const char *pos = filename + strlen(filename) - 1;

	for (; pos >= filename ; pos--)
	{
		if (*pos == '/')
			return pos + 1;

#ifdef WIN32
		if (*pos == '\\' || *pos == ':')
			return pos + 1;
#endif
	}

	return filename;
}


bool FilenameIsBare(const char *filename)
{
	if (strchr(filename, '.')) return false;
	if (strchr(filename, '/')) return false;
	if (strchr(filename, '\\')) return false;
	if (strchr(filename, ':')) return false;

	return true;
}


static void FilenameStripBase(char *buffer)
{
	if(!*buffer)
	{
		return;	// empty buffer, can't do much
	}
	char *pos = buffer + strlen(buffer) - 1;

	for (; pos > buffer ; pos--)
	{
		if (*pos == '/')
			break;

#ifdef WIN32
		if (*pos == '\\')
			break;

		if (*pos == ':')
		{
			pos[1] = 0;
			return;
		}
#endif
	}

	if (pos > buffer)
		*pos = 0;
	else
	{
		// At this point it's guaranteed not to be empty
		buffer[0] = '.';
		buffer[1] = 0;
	}
}

//
// Clears the basename
//
static void FilenameStripBase(SString &path)
{
	if(!path)
	{
		path = ".";
		return;
	}

#ifdef _WIN32
	size_t seppos = path.find_last_of("\\/");
	size_t colonpos = path.rfind(':');
	if(seppos != SString::npos)
	{
		if(colonpos != SString::npos && colonpos > seppos)
		{
			if(colonpos < path.size() - 1)
				path.erase(colonpos + 1);
			return;
		}
		if(seppos == 0)
		{
			path = "\\";
			return;
		}
		path.erase(seppos);
		return;
	}
	path = ".";
	return;
#else
	size_t seppos = path.find_last_of("/");
	if(seppos != SString::npos)
	{
		if(seppos == 0)
		{
			path = "/";
			return;
		}
		path.erase(seppos);
		return;
	}
	path = ".";
	return;
#endif
}


//
// takes the basename in 'filename' and prepends the path from 'othername'.
// returns a newly allocated string.
//
SString FilenameReposition(const char *filename, const char *othername)
{
	filename = fl_filename_name(filename);

	const char *op = fl_filename_name(othername);

	if (op <= othername)
		return filename;

	size_t dir_len = op - othername;
	size_t len = strlen(filename) + dir_len;

	SString result;
	result.reserve(len + 10);
	result.assign(othername, dir_len);
	result += filename;
	return result;
}

//
// Get path
//
SString FilenameGetPath(const char *filename)
{
	size_t len = (size_t)(FindBaseName(filename) - filename);

	// remove trailing slash (except when following "C:" or similar)
	if(len >= 1 &&
		(filename[len - 1] == '/' || filename[len - 1] == '\\') &&
		!(len >= 2 && filename[len - 2] == ':'))
	{
		len--;
	}

	if(len == 0)
		return ".";
	
	return SString(filename, len);
}


bool FileCopy(const char *src_name, const char *dest_name)
{
	char buffer[1024];

	FILE *src = fopen(src_name, "rb");
	if (! src)
		return false;

	FILE *dest = fopen(dest_name, "wb");
	if (! dest)
	{
		fclose(src);
		return false;
	}

	while (true)
	{
		size_t rlen = fread(buffer, 1, sizeof(buffer), src);
		if (rlen == 0)
			break;

		size_t wlen = fwrite(buffer, 1, rlen, dest);
		if (wlen != rlen)
			break;
	}

	bool was_OK = !ferror(src) && !ferror(dest);

	fclose(dest);
	fclose(src);

	return was_OK;
}


bool FileRename(const char *old_name, const char *new_name)
{
#ifdef WIN32
	return (::MoveFile(old_name, new_name) != 0);

#else // UNIX or MACOSX

	return (rename(old_name, new_name) == 0);
#endif
}


bool FileDelete(const char *filename)
{
#ifdef WIN32
	// TODO: set wide character here
	return (::DeleteFile(filename) != 0);

#else // UNIX or MACOSX

	return (remove(filename) == 0);
#endif
}


bool FileChangeDir(const char *dir_name)
{
#ifdef WIN32
	// TODO: set wide character here
	return (::SetCurrentDirectory(dir_name) != 0);

#else // UNIX or MACOSX

	return (chdir(dir_name) == 0);
#endif
}


bool FileMakeDir(const char *dir_name)
{
#ifdef WIN32
	return (::CreateDirectory(dir_name, NULL) != 0);

#else // UNIX or MACOSX

	return (mkdir(dir_name, 0775) == 0);
#endif
}


u8_t * FileLoad(const char *filename, int *length)
{
	*length = 0;

	FILE *fp = fopen(filename, "rb");

	if (! fp)
		return NULL;

	// determine size of file (via seeking)
	fseek(fp, 0, SEEK_END);
	{
		(*length) = (int)ftell(fp);
	}
	fseek(fp, 0, SEEK_SET);

	if (ferror(fp) || *length < 0)
	{
		fclose(fp);
		return NULL;
	}

	u8_t *data = (u8_t *) malloc(*length + 1);

	if (! data)
		FatalError("Out of memory (%d bytes for FileLoad)\n", *length);

	// ensure buffer is NUL-terminated
	data[*length] = 0;

	if (*length > 0 && 1 != fread(data, *length, 1, fp))
	{
		FileFree(data);
		fclose(fp);
		return NULL;
	}

	fclose(fp);

	return data;
}


void FileFree(u8_t *mem)
{
	free((void*) mem);
}


//
// Note: returns false when the path doesn't exist.
//
bool PathIsDirectory(const char *path)
{
#ifdef WIN32
	char old_dir[MAX_PATH+1];

	if (GetCurrentDirectory(MAX_PATH, (LPSTR)old_dir) == FALSE)
		return false;

	bool result = SetCurrentDirectory(path);

	SetCurrentDirectory(old_dir);

	return result;

#else // UNIX or MACOSX

	struct stat finfo;

	if (stat(path, &finfo) != 0)
		return false;

	return (S_ISDIR(finfo.st_mode)) ? true : false;
#endif
}

//------------------------------------------------------------------------

//
// Scan a directory
//
int ScanDirectory(const char *path, const std::function<void(const char *, int)> &func)
{
	SYS_ASSERT(!!path);
	int count = 0;

#ifdef WIN32

	// this is a bit clunky.  We set the current directory to the
	// target and use FindFirstFile with "*.*" as the pattern.
	// Afterwards we restore the current directory.

	char old_dir[MAX_PATH+1];

	if (GetCurrentDirectory(MAX_PATH, (LPSTR)old_dir) == FALSE)
		return SCAN_ERROR;

	if (SetCurrentDirectory(path) == FALSE)
		return SCAN_ERR_NoExist;

	WIN32_FIND_DATA fdata;

	HANDLE handle = FindFirstFile("*.*", &fdata);
	if (handle == INVALID_HANDLE_VALUE)
	{
		SetCurrentDirectory(old_dir);

		return 0;  //??? (GetLastError() == ERROR_FILE_NOT_FOUND) ? 0 : SCAN_ERROR;
	}

	do
	{
		int flags = 0;

		if (fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			flags |= SCAN_F_IsDir;

		if (fdata.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
			flags |= SCAN_F_ReadOnly;

		if (fdata.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
			flags |= SCAN_F_Hidden;

		// minor kludge for consistency with Unix
		if (fdata.cFileName[0] == '.' && isalpha(fdata.cFileName[1]))
			flags |= SCAN_F_Hidden;

		if (strcmp(fdata.cFileName, ".")  == 0 ||
				strcmp(fdata.cFileName, "..") == 0)
		{
			// skip the funky "." and ".." dirs
		}
		else
		{
			func(fdata.cFileName, flags);

			count++;
		}
	}
	while (FindNextFile(handle, &fdata) != FALSE);

	FindClose(handle);

	SetCurrentDirectory(old_dir);


#else // ---- UNIX ------------------------------------------------

	DIR *handle = opendir(path);
	if (handle == NULL)
		return SCAN_ERR_NoExist;

	for (;;)
	{
		const struct dirent *fdata = readdir(handle);
		if (fdata == NULL)
			break;

		if (strlen(fdata->d_name) == 0)
			continue;

		// skip the funky "." and ".." dirs
		if (strcmp(fdata->d_name, ".")  == 0 ||
				strcmp(fdata->d_name, "..") == 0)
			continue;

		SString full_name = StringPrintf("%s/%s", path, fdata->d_name);

		struct stat finfo;

		if (stat(full_name.c_str(), &finfo) != 0)
		{
			DebugPrintf(".... stat failed: %s\n", strerror(errno));
			continue;
		}


		int flags = 0;

		if (S_ISDIR(finfo.st_mode))
			flags |= SCAN_F_IsDir;

		if ((finfo.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)) == 0)
			flags |= SCAN_F_ReadOnly;

		if (fdata->d_name[0] == '.' && isalpha(fdata->d_name[1]))
			flags |= SCAN_F_Hidden;

		func(fdata->d_name, flags);

		count++;
	}

	closedir(handle);
#endif

	return count;
}

int ScanDirectory(const char *path, directory_iter_f func, void *priv_dat)
{
	return ScanDirectory(path, [func, priv_dat](const char *name, int flags)
						 {
		func(name, flags, priv_dat);
	});
}

//------------------------------------------------------------------------

SString GetExecutablePath(const char *argv0)
{
	SString path;

#ifdef WIN32
	wchar_t wpath[PATH_MAX / 2 + 1];
	DWORD length = GetModuleFileNameW(GetModuleHandleW(nullptr), wpath, _countof(wpath));
	if(length > 0 && length < PATH_MAX / 2)
	{
		if(_waccess(wpath, 0) == 0)
		{
			SString retpath = WideToUTF8(wpath);
			FilenameStripBase(retpath);
			return retpath;
		}
	}

#elif !defined(__APPLE__) // UNIX
	char rawpath[PATH_MAX+2];

	int length = readlink("/proc/self/exe", rawpath, PATH_MAX);

	if (length > 0)
	{
		rawpath[length] = 0; // add the missing NUL

		if (access(rawpath, 0) == 0)  // sanity check
		{
			FilenameStripBase(rawpath);
			return rawpath;
		}
	}

#else
	/*
	   from http://www.hmug.org/man/3/NSModule.html

	   extern int _NSGetExecutablePath(char *buf, uint32_t *bufsize);

	   _NSGetExecutablePath copies the path of the executable
	   into the buffer and returns 0 if the path was successfully
	   copied in the provided buffer. If the buffer is not large
	   enough, -1 is returned and the expected buffer size is
	   copied in *bufsize.
	 */
	uint32_t pathlen = PATH_MAX * 2;
	char rawpath[2 * PATH_MAX + 2];

	if (0 == _NSGetExecutablePath(rawpath, &pathlen))
	{
		// FIXME: will this be _inside_ the .app folder???
		FilenameStripBase(rawpath);
		return rawpath;
	}
#endif

	// fallback method: use argv[0]
	path = argv0;

#ifdef __APPLE__
	// FIXME: check if _inside_ the .app folder
#endif

	FilenameStripBase(path);
	return path;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
