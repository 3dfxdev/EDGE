//----------------------------------------------------------------------------
//  EDGE Wipe Main
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
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
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------
//
// DESCRIPTION:
//   Mission start screen wipe/melt, special effects.
// 

#ifndef __F_WIPE_H__
#define __F_WIPE_H__

#include "m_fixed.h"

//
//                       SCREEN WIPE PACKAGE
//

typedef enum
{
  // no wiping
  WIPE_None,
  // weird screen melt
  WIPE_Melt,
  // cross-fading
  WIPE_Crossfade,
  // pixel fading
  WIPE_Pixelfade,

  // new screen simply scrolls in from the given side of the screen
  // (or if reversed, the old one scrolls out to the given side)
  WIPE_Top,
  WIPE_Bottom,
  WIPE_Left,
  WIPE_Right,

  // Scrolls in from all corners
  WIPE_Corners,

  // Opens like doors
  WIPE_Doors,

  WIPE_NUMWIPES
}
wipetype_e;

extern wipetype_e wipe_method;
extern int wipe_reverse;

// for enum cvars
extern const char WIPE_EnumStr[];

#endif  // __F_WIPE_H__
