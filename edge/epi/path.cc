//----------------------------------------------------------------------------
//  EDGE Path Handling Methods for Win32
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
#include "path.h"

namespace epi
{

// Path Manipulation Functions
namespace path
{

std::string GetDir(const char *path)
{
	SYS_ASSERT(path);

	const char *p = path + strlen(path) - 1;

	// back up until a slash or the start
	for (; p >= path; p--)
		if (IsDirSeperator(*p))
			return std::string(path, (p - path) + 1);

    return std::string();  // nothing
}


std::string GetFilename(const char *path)
{
	SYS_ASSERT(path);

	const char *p = path + strlen(path) - 1;

	// back up until a slash or the start
	for (; p >= path; p--)
		if (IsDirSeperator(*p))
			return std::string(p + 1);

    return std::string(path);
}


std::string GetExtension(const char *path)
{
	SYS_ASSERT(path);

	const char *p = path + strlen(path) - 1;
	
	// back up until a dot
	for (; p >= path; p--)
	{
		if (IsDirSeperator(*p))
			break;

		if (*p == '.')
		{
            // handle filenames that being with a dot
            // (un*x style hidden files)
            if (p == path || IsDirSeperator(p[-1]))
				break;

			return std::string(p + 1);
		}
	}

    return std::string();  // nothing
}


std::string GetBasename(const char *path) 
{
	SYS_ASSERT(path);

	const char *p = path + strlen(path) - 1;
	const char *r = p;

	// back up until a slash or the start
	for (; p > path; p--)
	{
		if (IsDirSeperator(*p))
		{
			p++; break;
		}
	}

	SYS_ASSERT(p >= path);
	
	// back up until a dot
	for (; r >= p; r--)
	{
		if (*r == '.')
		{
            // handle filenames that being with a dot
            // (un*x style hidden files)
            if (r == p || IsDirSeperator(r[-1]))
				break;

			return std::string(p, r - p);
		}
	}

    return std::string(p);
}



bool IsAbsolute(const char *path)
{
	SYS_ASSERT(path);

#ifdef WIN32
    // Check for Drive letter, colon and slash...
    if (strlen(path) > 2 && path[1] == ':' && 
        (path[2] == '\\' || path[2] == '/') && 
        isalpha(path[0]))
    {
        return true;
    }

    // Check for share name...
    if (strlen(path) > 1 && path[0] == '\\' && path[1] == '\\')
        return true;

#else // LINUX

	if (IsDirSeperator(path[0]))
		return true;
#endif

    return false;
}


bool IsDirSeperator(const char c)
{
#ifdef WIN32
    return (c == '\\' || c == '/' || c == ':'); // Kester added ':'
#else // LINUX
    return (c == '\\' || c == '/');
#endif
}


std::string Join(const char *lhs, const char *rhs)
{
	SYS_ASSERT(lhs && rhs);

	if (IsAbsolute(rhs))
		return std::string(rhs);

    std::string result(lhs);

	if (result.size() > 0)
	{
		const char c = result[result.size()-1];

#ifdef WIN32
		if (c != '\\' && c != '/')
			result += '\\';
#else // LINUX
		if (c != '/')
			result += '/';
#endif
	}

	result += rhs;

    return result;
}

}; // namespace path

} // namespace epi

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
