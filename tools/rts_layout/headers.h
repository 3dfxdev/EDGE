//------------------------------------------------------------------------
//  INCLUDES
//------------------------------------------------------------------------
//
//  RTS Layout Tool (C) 2007 Andrew Apted
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

#ifndef __RTS_HEADERS_H__
#define __RTS_HEADERS_H__

/* OS specifics */
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

/* C library */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <errno.h>

/* STL goodies */

#include <string>
#include <vector>
#include <list>

/* Our own system defs */

#include "sys_type.h"
#include "sys_macro.h"
#include "sys_assert.h"
#include "sys_debug.h"
#include "sys_endian.h"

#define MSG_BUF_LEN  2000

#endif // __RTS_HEADERS_H__

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
