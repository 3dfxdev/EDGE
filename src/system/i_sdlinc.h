//----------------------------------------------------------------------------
//  EDGE SDL System Internal header
//----------------------------------------------------------------------------
//
//  Copyright (c) 2005-2018  The EDGE Team.
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

// -CA- 2018/11/30:
// Fix below so autobuild systems can find SDL.h. If you are using a system that supports
// GCC/Linux/OSX and this does not work or invalidates your build, please promptly report
// it so we can come up with another solution. Just open up an ISSUE or a PR via Github if
// it does not work and/or you have a better way.

#ifndef WIN32
#include <SDL2/SDL.h>
#include <SDL2/SDL_keycode.h> /// Keys
#include <SDL2/SDL_opengl.h> // opengl
#include <SDL2/SDL_render.h> // kitchensink
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_mutex.h>
#include <SDL2/SDL_cpuinfo.h>

#else

#include <SDL.h>
#include <SDL_keycode.h> /// Keys
#include <SDL_opengl.h> // opengl
#include <SDL_render.h> // kitchensink
#include <SDL_thread.h>
#include <SDL_surface.h>
#include <SDL_mutex.h>

#endif



#include "i_local.h"  // FIXME: remove

#endif /* __SDL_SYSTEM_INTERNAL_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
