//------------------------------------------------------------------------
//  Common Headers
//------------------------------------------------------------------------
//
//  Copyright (c) 2008  Andrew J Apted
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

#ifndef __COMMON_HEADER__
#define __COMMON_HEADER__


#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

#include <string.h>
#include <ctype.h>
#include <math.h>

#include <string>
#include <vector>

#include "u_type.h"
#include "u_macro.h"
#include "u_endian.h"
#include "u_assert.h"
#include "u_util.h"
#include "u_file.h"


void FatalError(const char *str, ...);


#endif // __COMMON_HEADER__

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
