//------------------------------------------------------------------------
//  LUA INCLUDES
//------------------------------------------------------------------------
//
//  Copyright (c) 2005-2008  The EDGE Team.
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

#ifndef __EDGE_LUA_INC__
#define __EDGE_LUA_INC__

/* LUA Scripting Language */

#if defined(HAVE_LUA_51_H)

#include <lua5.1/lua.h>
#include <lua5.1/lauxlib.h>
#include <lua5.1/lualib.h>

#elif defined(HAVE_LUA_H)

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#else // local copy

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#endif

#endif // __EDGE_LUA_INC__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
