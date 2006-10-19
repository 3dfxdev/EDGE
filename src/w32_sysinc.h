//----------------------------------------------------------------------------
//  EDGE Win32 System Internal header
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

#ifndef __SYSTEM_INTERNAL_H__
#define __SYSTEM_INTERNAL_H__

#define DUMMYUNIONNAMEN(n)

#include "i_defs.h"

#include <objbase.h>
#include <wchar.h>
#include <winbase.h>
#include <winnls.h>
#include <mmsystem.h>
#include <signal.h>
#include <windowsx.h> // -ACB- 2000/07/19 Cracker API

#include <sys/types.h> // Required for _stat()

#include "edge32.rh" // Resources

// Win32 Mixer
typedef struct
{
	HMIXER handle;				// Handle
	UINT id;					// ID

	int channels;				// Channel count
	DWORD volctrlid;			// Volume control ID
	DWORD minvol;				// Min Volume
	DWORD maxvol;				// Max Volume

	DWORD originalvol;          // Original volume 
}
win32_mixer_t;

// I_MUS.C - Win32 MUS Handling
bool I_StartupMUS();
int I_MUSPlayTrack(byte *data, int length, bool loopy, float gain);
void I_MUSPause(void);
void I_MUSResume(void);
void I_MUSStop(void);
void I_MUSTicker(void);
bool I_MUSPlaying(void);
void I_MUSSetVolume(float gain);
void I_ShutdownMUS(void);

// I_MUSIC.C
win32_mixer_t *I_MusicLoadMixer(DWORD type);
void I_MusicReleaseMixer(win32_mixer_t* mixer);
bool I_MusicGetMixerVol(win32_mixer_t* mixer, DWORD *vol);
bool I_MusicSetMixerVol(win32_mixer_t* mixer, DWORD vol);

#endif /* __SYSTEM_INTERNAL_H__ */


