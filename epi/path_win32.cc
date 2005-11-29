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

namespace epi
{

// Path Manipulation Functions
namespace path
{

//
// bool IsAbsolute()
//
bool IsAbsolute(const char *path)
{
    // FIXME: Catch null parameters

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
// string_c Join()
//
string_c Join(const char *lhs, const char *rhs)
{
    epi::string_c s;

    // FIXME: Catch null parameters

    if (IsAbsolutePath(rhs))
    {
        s.Set(rhs);
        return s;
    }

    s.Set(lhs);

    const char c = s.GetLastChar(); 
    if (c != '/' && c != '\\')
        s.AddChar('\\');

    s.AddString(rhs);

    return s;
}

}; // namespace path

}; // namespace epi
