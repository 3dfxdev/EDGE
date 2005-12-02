//----------------------------------------------------------------------------
//  EDGE Path Handling Methods for Win32
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
#include "path.h"

#include <string.h>

namespace epi
{

// Path Manipulation Functions
namespace path
{

//
// string_c GetBasename
//
string_c GetBasename(const char *path) 
{
    epi::string_c s;

    // FIXME: Throw exception?
    if (!path)
        return s;

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

    return s;
}

//
// string_c GetDir
//
string_c GetDir(const char *path)
{
    epi::string_c s;

    // FIXME: Throw exception?
    if (!path)
        return s;

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

    return s;
}

//
// string_c GetExtension
//
string_c GetExtension(const char *path)
{
    epi::string_c s;

    // FIXME: Throw exception?
    if (!path)
        return s;

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

    return s;
}

//
// string_c GetFilename
//
string_c GetFilename(const char *path)
{
    epi::string_c s;

    // FIXME: Throw exception?
    if (!path)
        return s;

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

    return s;
}


//
// bool IsAbsolute()
//
bool IsAbsolute(const char *path)
{
    // FIXME: Throw error on null parameters?
    if (!path)
        return false;

    // Check for Drive letter, colon and slash...
    if (strlen(path) > 2 && path[1] == ':' && 
        (path[2] == '\\' || path[2] == '/') && 
        isalpha(path[0]))
    {
        return true;
    }

    // Check for share name...
    if (strlen(path) > 1 && path[0] == '\\' && path[1] == '\\')
    {
        return true;
    }

    return false;
}

//
// bool IsDirSeperator
//
bool IsDirSeperator(const char c)
{
    return (c == '\\' || c == '/' || c == ':'); // Kester added ':'
}

//
// string_c Join()
//
string_c Join(const char *lhs, const char *rhs)
{
    epi::string_c s;

    if (lhs && rhs)
    {
        if (IsAbsolute(rhs))
        {
            s.Set(rhs);
            return s;
        }

        s.Set(lhs);

        if (s.GetLength())
        {
            const char c = s.GetLastChar();
            if (c != '\\' && c != '/')
                s.AddChar('\\');
        }

        s.AddString(rhs);
    }

    return s;
}

}; // namespace path

}; // namespace epi
