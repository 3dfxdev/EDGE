//----------------------------------------------------------------------------
//  EDGE Sound FX Handling Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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

#include "epi/math_vector.h"

// Forward declarations
typedef struct mobj_s mobj_t;
typedef struct sec_sfxorig_s sec_sfxorig_t;
typedef struct sfx_s sfx_t;

// Sound Categories
// ----------------
//
// Each category has a minimum number of channels (say N).
// Sounds of a category are GUARANTEED to play when there
// are less than N sounds of that category already playing.
//
// So while more than N sounds of a category can be active at
// a time, the extra ones are "hogging" channels belonging to
// other categories, and will be kicked out (trumped) if there
// are no other free channels.
//
// The order here is significant, if the channel limit for a
// category is set to zero, then NEXT category is tried.
//
typedef enum
{
	SNCAT_UI,         // for the user interface (menus, tips)
	SNCAT_Music,      // for OGG music and MIDI synthesis
	SNCAT_Player,     // for console player (pain, death, pickup)
	SNCAT_Weapon,     // for console player's weapon
	SNCAT_Opponent,   // for all other players (DM or COOP)
	SNCAT_Monster,    // for all monster sounds
	SNCAT_Object,     // for all other objects
	SNCAT_Level,      // for doors, lifts and map scripts
	SNCAT_NUMTYPES
}
sound_category_e;

// for the sliders
#define SND_SLIDER_NUM  20

extern float slider_to_gain[SND_SLIDER_NUM];

// S_MUSIC.C
void S_ChangeMusic(int entrynum, bool looping);
void S_ResumeMusic(void);
void S_PauseMusic(void);
void S_StopMusic(void);
void S_MusicTicker(void);
int S_GetMusicVolume(void);
void S_SetMusicVolume(int volume);

// S_SOUND.C
namespace sound
{
    // FX Flags
    typedef enum
    {
        FXFLAG_SYMBOLIC    = 0x1,
        FXFLAG_IGNOREPAUSE = 0x2,
        FXFLAG_IGNORELOOP  = 0x4
    }
    fx_flag_t;

    // Init/Shutdown
    void Init(void);
    void Shutdown(void);

    void Reset(void);

    // FX Control
    void SetFXFlags(int handle, int flags);

    void StartFX(sfx_t *sfx, int category, epi::vec3_c pos, int flags = 0); 
    void StartFX(sfx_t *sfx, int category, mobj_t *mo, int flags = 0); 
    void StartFX(sfx_t *sfx, int category, sec_sfxorig_t *orig, int flags = 0); 
    void StartFX(sfx_t *sfx, int category = SNCAT_UI, int flags = 0);

    void StopFX(int handle);
    void StopFX(sec_sfxorig_t *orig);
    void StopFX(mobj_t *mo);

    void StopLoopingFX(int handle);
    void StopLoopingFX(sec_sfxorig_t *orig);
    void StopLoopingFX(mobj_t *mo);
    
    void ResumeAllFX();
    void PauseAllFX();

    bool IsFXPlaying(int handle); 
    bool IsFXPlaying(sec_sfxorig_t *orig); 
    bool IsFXPlaying(mobj_t *mo); 

    // Your effect reservation, sir...
    int ReserveFX(int category);
    void UnreserveFX(int handle);

    // Ticker
    void Ticker();

    // Playsim Object <-> Effect Linkage
    void UnlinkFX(mobj_t *mo);
    void UnlinkFX(sec_sfxorig_t *orig);

    // Volume Control
    int GetVolume();
    void SetVolume(int volume);

    // Effect lookup
	int LookupEffectDef(const sfx_t *s);
};

#endif // __S_SOUND__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
