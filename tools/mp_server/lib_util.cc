//------------------------------------------------------------------------
//  Utility functions
//------------------------------------------------------------------------
//
//  Edge MultiPlayer Server (C) 2004  Andrew Apted
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

#include "includes.h"

#include "lib_util.h"
#include "sys_assert.h"


//
// FileExists
//
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

//
// HasExtension
//
bool HasExtension(const char *filename)
{
	int A = (int)strlen(filename) - 1;

	if (A > 0 && filename[A] == '.')
		return false;

	for (; A >= 0; A--)
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
// CheckExtension
//
// When ext is NULL, checks if the file has no extension.
//
bool CheckExtension(const char *filename, const char *ext)
{
	if (! ext)
		return ! HasExtension(filename);

	int A = (int)strlen(filename) - 1;
	int B = (int)strlen(ext) - 1;

	for (; B >= 0; B--, A--)
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
// NOTE: returned string is static storage.
//
const char *ReplaceExtension(const char *filename, const char *ext)
{
	char *dot_pos;
	static char buffer[1024];

	SYS_ASSERT(strlen(filename)+(ext ? strlen(ext) : 0)+4 < sizeof(buffer));
	SYS_ASSERT(filename[0] != 0);

	strcpy(buffer, filename);

	dot_pos = buffer + strlen(buffer) - 1;

	for (; dot_pos >= buffer && *dot_pos != '.'; dot_pos--)
	{
		if (*dot_pos == '/')
			break;

#ifdef WIN32
		if (*dot_pos == '\\' || *dot_pos == ':')
			break;
#endif
	}

	if (dot_pos < buffer || *dot_pos != '.')
		dot_pos = NULL;

	if (! ext)
	{
		if (dot_pos)
			dot_pos[0] = 0;

		return buffer;
	}

	if (dot_pos)
		dot_pos[1] = 0;
	else
		strcat(buffer, ".");

	strcat(buffer, ext);

	return buffer;
}

//
// FileBaseName
//
// Find the base name of the file (i.e. without any path).
// The result always points within the given string.
//
// Example:  "C:\Foo\Bar.wad"  ->  "Bar.wad"
// 
const char *FileBaseName(const char *filename)
{
	const char *pos = filename + strlen(filename) - 1;

	for (; pos >= filename; pos--)
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

//------------------------------------------------------------------------

//
// StrCaseCmp
//
int StrCaseCmp(const char *A, const char *B)
{
	for (; *A || *B; A++, B++)
	{
		// this test also catches end-of-string conditions
		if (toupper(*A) != toupper(*B))
			return (toupper(*A) - toupper(*B));
	}

	return 0;
}

//
// StrCaseCmpPartial
//
// Checks that the string B occurs at the front of string A.
// NOTE: This function is not symmetric, A can be longer than B and
// still match, but the match always fails if A is shorter than B.
//
int StrCaseCmpPartial(const char *A, const char *B)
{
	for (; *B; A++, B++)
	{
		// this test also catches end-of-string conditions
		if (toupper(*A) != toupper(*B))
			return (toupper(*A) - toupper(*B));
	}

	return 0;
}

//
// StrMaxCopy
//
void StrMaxCopy(char *dest, const char *src, int max)
{
	for (; *src && max > 0; max--)
	{
		*dest++ = *src++;
	}

	*dest = 0;
}

//
// StrUpper
//
// NOTE: returned string is static storage.
//
const char *StrUpper(const char *name)
{
	static char up_buf[1024];

	SYS_ASSERT(strlen(name)+4 < sizeof(up_buf));

	char *dest = up_buf;

	while (*name)
		*dest++ = toupper(*name++);

	*dest = 0;

	return up_buf;
}

//
// StringNew
//
// Length does not include the trailing NUL.
//
char *StringNew(int length)
{
	char *s = (char *) calloc(length + 1, 1);

	if (! s)
		AssertFail("Out of memory (%d bytes for string)\n", length);

	return s;
}

//
// StringDup
//
char *StringDup(const char *orig)
{
	char *s = strdup(orig);

	if (! s)
		AssertFail("Out of memory (copy string)\n");
	
	return s;
}

//
// StringFree
//
void StringFree(const char *str)
{
	free((void*) str);
}

