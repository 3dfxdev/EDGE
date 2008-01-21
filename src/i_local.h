//----------------------------------------------------------------------------
//  EDGE System Local definitions
//----------------------------------------------------------------------------
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
//----------------------------------------------------------------------------

#ifndef __SYSTEM_LOCAL_H__
#define __SYSTEM_LOCAL_H__

// I_CD
bool I_StartupCD(void);
bool I_CDStartPlayback(int tracknum, bool loopy, float gain);
bool I_CDPausePlayback(void);
bool I_CDResumePlayback(void);
void I_CDStopPlayback(void);
void I_CDSetVolume(float gain);
bool I_CDFinished(void);
bool I_CDTicker(void);
void I_ShutdownCD();

// I_CTRL
void I_CentreMouse();

// I_MUSIC
extern bool musicpaused;
void I_PostMusicError(const char *message);

extern bool use_grab;
extern bool use_warp_mouse;

#endif /* __SYSTEM_LOCAL_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
