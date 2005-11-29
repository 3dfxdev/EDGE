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
    // Returns true if the given is an absolute path
    bool IsAbsolute(const char *path);

    // Join two paths together
    string_c Join(const char *lhs, const char *rhs);
}; 

}; // namespace epi

#endif /* __EPI_PATH_MODULE__ */
