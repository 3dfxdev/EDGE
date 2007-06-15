//----------------------------------------------------------------------------
//  EDGE Music Handling Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2007  The EDGE Team.
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

#ifndef __S_MUSIC__
#define __S_MUSIC__

// Vars
extern int mus_volume;  // 0 .. SND_SLIDER_NUM-1

void S_ChangeMusic(int entrynum, bool looping);
void S_ResumeMusic(void);
void S_PauseMusic(void);
void S_StopMusic(void);
void S_MusicTicker(void);

void S_ChangeMusicVolume(void);

#endif // __S_MUSIC__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
