//----------------------------------------------------------------------------
//  EDGE Sound FX Handling Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2001  The EDGE Team.
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

#ifndef __S_SOUND__
#define __S_SOUND__

#include "p_mobj.h"
#include "z_zone.h"

// for the sliders
#define S_MIN_VOLUME 0
#define S_MAX_VOLUME 15

#define S_CLOSE_DIST    160.0
#define S_CLIPPING_DIST 1600.0

// S_MUSIC.C
void S_ChangeMusic(int entrynum, boolean_t looping);
void S_ResumeMusic(void);
void S_PauseMusic(void);
void S_StopMusic(void);
void S_MusicTicker(void);
int S_GetMusicVolume(void);
void S_SetMusicVolume(int volume);

// S_SOUND.C
boolean_t S_Init(void);
void S_SoundLevelInit(void);
int S_StartSound(mobj_t *origin, sfx_t *sound_id);
void S_ResumeSounds(void);
void S_PauseSounds(void);
void S_RemoveSoundOrigin(mobj_t *origin);
void S_AddToFreeQueue(mobj_t *origin, void *block);
void S_StopSound(mobj_t *origin);
void S_StopLoopingSound(mobj_t *origin); // -ACB- 2001/01/09 Addition Functionality
void S_StopChannel(int cnum);
void S_UpdateSounds(mobj_t *listener);
void S_SoundTicker(void);
int S_GetSfxVolume(void);
void S_SetSfxVolume(int volume);

// S_UTIL.C
byte *S_UtilConvertMUStoMIDI(byte *data);

#endif
