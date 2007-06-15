//----------------------------------------------------------------------------
//  EDGE Path Handling Methods
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
#ifndef __EPI_PATH_MODULE__
#define __EPI_PATH_MODULE__

#include "strings.h"

namespace epi
{

// Path Manipulation Functions
namespace path
{
    // Returns the basename (filename minus extension) if it exists
    string_c GetBasename(const char *path);

    // Returns the directory from the path if it exists
    string_c GetDir(const char *path);

    // Returns a filename extension from the path if it exists
    string_c GetExtension(const char *path);

    // Returns a filename from the path if it exists
    string_c GetFilename(const char *path);

    // Returns true if the given is an absolute path
    bool IsAbsolute(const char *path);

    // Returns true if the character could act as a seperator
    bool IsDirSeperator(const char c);

    // Join two paths together
    string_c Join(const char *lhs, const char *rhs);
}; 

}; // namespace epi

#endif /* __EPI_PATH_MODULE__ */
