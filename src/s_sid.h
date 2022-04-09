//----------------------------------------------------------------------------
//  EDGE TinySID Music Player (HEADER)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2022 - The EDGE-Classic Community
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

#ifndef __SIDPLAYER_H__
#define __SIDPLAYER_H__

#include "system\i_defs.h"

#include "epi\sound_data.h"

class pl_entry_c;

/* FUNCTIONS */

abstract_music_c * S_PlaySIDMusic(const pl_entry_c *musdat, float volume, bool looping);

bool S_CheckSID(byte *data, int length);

#endif  /* __SIDPLAYER_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
