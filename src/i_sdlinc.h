//----------------------------------------------------------------------------
//  EDGE2 SDL System Internal header
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2005-2009  The EDGE2 Team.
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

#ifndef __SDL_SYSTEM_INTERNAL_H__
#define __SDL_SYSTEM_INTERNAL_H__

#ifdef MACOSX
#include <SDL2/SDL.h>
#include <SDL2/SDL_keycode.h>
#else
#include <SDL.h>
#include <SDL_keycode.h> /// Keys
#endif

#include "i_local.h"  // FIXME: remove

#endif /* __SDL_SYSTEM_INTERNAL_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
