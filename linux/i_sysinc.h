//----------------------------------------------------------------------------
//  EDGE LINUX System Specific header
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

#ifndef __SYSTEM_INTERNAL_H__
#define __SYSTEM_INTERNAL_H__

#include "../i_defs.h"

#include <sys/ioctl.h>
#include <linux/cdrom.h>

// I_CD.C
boolean_t I_StartupCD(void);
boolean_t I_CDStartPlayback(int tracknum);
void I_CDPausePlayback(void);
void I_CDResumePlayback(void);
void I_CDStopPlayback(void);
void I_CDSetVolume(int vol);
boolean_t I_CDFinished(void);
void I_ShutdownCD(void);

// I_MUSIC.C
extern boolean_t musicpaused;
void I_PostMusicError(const char *message);

// I_MUSSRV.C
boolean_t I_StartupMusserv(void);
boolean_t I_MusservStartPlayback(const char *data, int len);
void I_MusservPausePlayback(void);
void I_MusservResumePlayback(void);
void I_MusservStopPlayback(void);
void I_MusservSetVolume(int vol);
void I_ShutdownMusserv(void);

// MP3 stuff
boolean_t I_StartupMP3(void);
int I_MP3PlayTrack(const char *filename, boolean_t looping);
void I_MP3StopTrack(int track);
void I_MP3SetVolume(int vol);
void I_ShutdownMP3(void);

// I_FMPAT.C
void I_CreateGENMIDI(byte *dest);

#endif // __SYSTEM_INTERNAL_H__
//----------------------------------------------------------------------------
//  EDGE LINUX System Specific header
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

#ifndef __SYSTEM_INTERNAL_H__
#define __SYSTEM_INTERNAL_H__

#include "../i_defs.h"

#include <sys/ioctl.h>
#include <linux/cdrom.h>

// I_CD.C
boolean_t I_StartupCD(void);
boolean_t I_CDStartPlayback(int tracknum);
void I_CDPausePlayback(void);
void I_CDResumePlayback(void);
void I_CDStopPlayback(void);
void I_CDSetVolume(int vol);
boolean_t I_CDFinished(void);
void I_ShutdownCD(void);

// I_MUSIC.C
extern boolean_t musicpaused;
void I_PostMusicError(const char *message);

// I_MUSSRV.C
boolean_t I_StartupMusserv(void);
boolean_t I_MusservStartPlayback(const char *data, int len);
void I_MusservPausePlayback(void);
void I_MusservResumePlayback(void);
void I_MusservStopPlayback(void);
void I_MusservSetVolume(int vol);
void I_ShutdownMusserv(void);

// MP3 stuff
boolean_t I_StartupMP3(void);
int I_MP3PlayTrack(const char *filename, boolean_t looping);
void I_MP3StopTrack(int track);
void I_MP3SetVolume(int vol);
void I_ShutdownMP3(void);

// I_FMPAT.C
void I_CreateGENMIDI(byte *dest);

#endif // __SYSTEM_INTERNAL_H__
