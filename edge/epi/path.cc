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

std::string GetBasename(const char *path) 
{
	SYS_ASSERT(path);

    epi::string_c s;
	
#ifdef WIN32
    const int len = strlen(path);
    if (len)
    {
        const char *start = path + (len - 1);

        // back up until a slash or the start
        while (start != path && !IsDirSeperator(*(start-1)))  
            start--;

        const char *end = start;
        if (*end != '.') // Not a un*x style hidden file
        {
            // move forward until extension or end
            while (*end && *end != '.')
                end++;

            // Generate the result
            if (start != end)
                s.AddChars(start, 0, end - start);
        }
        else 
        {
            s.AddString(start);
        }
    }

#else // LINUX

    const int len = strlen(path);
    if (len)
    {
        const char *start = path + (len - 1);

        // back up until a slash or the start
        while (start != path && !IsDirSeperator(*(start-1)))  
            start--;

        const char *end = start;
        if (*end != '.') // Not a un*x style hidden file
        {
            // move forward until extension or end
            while (*end && *end != '.')
                end++;

            // Generate the result
            if (start != end)
                s.AddChars(start, 0, end - start);
        }
        else 
        {
            s.AddString(start);
        }
    }

#endif

    return s;
}


std::string GetDir(const char *path)
{
	SYS_ASSERT(path);

    epi::string_c s;

#ifdef WIN32
    const int len = strlen(path);
    if (len)
    {
        const char *end = path + len; // point at the '\0' initially

        // back up until a slash or the start
        while (end != path) 
        {
            end--;

            if (IsDirSeperator(*end))
            {
                end++;
                s.AddChars(path, 0, end - path); 
                break;
            }
        }
    }

#else // LINUX
	
    const int len = strlen(path);
    if (len)
    {
        const char *end = path + len; // point at the '\0' initially

        // back up until a slash or the start
        while (end != path) 
        {
            end--;

            if (IsDirSeperator(*end))
            {
                end++;
                s.AddChars(path, 0, end - path); 
                break;
            }
        }
    }
#endif

    return s;
}


std::string GetExtension(const char *path)
{
	SYS_ASSERT(path);

    epi::string_c s;


#ifdef WIN32
    const int len = strlen(path);
    if (len)
    {
        const char *start = path + (len-1);
        const char *end = start;

        // back up until a slash or the start
        while (start != path && !IsDirSeperator(*start))
        {
            start--;
        }

        // Now step through to find the first period
        while (start < end && *start != '.')
            start++;

        if (start != path && start < end)
        {
            // Handle filenames that being with a 
            // fullstop (un*x style hidden files)
            if (!IsDirSeperator(*(start-1)))
            {
                start++; // Step over period

                // Double check we're not at the end 
                if (start != end)
                    s.AddString(start);
            }
        }
    }

#else // LINUX

    const int len = strlen(path);
    if (len)
    {
        const char *start = path + (len-1);
        const char *end = start;

        // back up until a slash or the start
        while (start != path && !IsDirSeperator(*start))
        {
            start--;
        }

        // Now step through to find the first period
        while (start < end && *start != '.')
            start++;

        if (start != path && start < end)
        {
            // Handle filenames that being with a 
            // fullstop (un*x style hidden files)
            if (!IsDirSeperator(*(start-1)))
            {
                start++; // Step over period

                // Double check we're not at the end 
                if (start != end)
                    s.AddString(start);
            }
        }
    }
#endif

    return s;
}


std::string GetFilename(const char *path)
{
	SYS_ASSERT(path);

    epi::string_c s;

#ifdef WIN32
    const int len = strlen(path);
    if (len)
    {
        const char *start = path + (len - 1);

        // back up until a slash or the start
        while (start != path && !IsDirSeperator(*start))
            start--;

        if (start != path || IsDirSeperator(*start))
            start++; // Step over directory seperator
        
        s.AddString(start);
    }

#else // LINUX

    const int len = strlen(path);
    if (len)
    {
        const char *start = path + (len - 1);

        // back up until a slash or the start
        while (start != path && !IsDirSeperator(*start))
            start--;

        if (start != path || IsDirSeperator(*start))
            start++; // Step over directory seperator
        
        s.AddString(start);
    }
#endif

    return s;
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

	if (path && IsDirSeperator(path[0]))
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
	SYS_ASESRT(lhs && rhs);

	if (IsAbsolute(rhs))
		return std::string(rhs);

    epi::string_c result(lhs);

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
