//----------------------------------------------------------------------------
//  EDGE LINUX System Specific header
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

#ifndef __SYSTEM_INTERNAL_H__
#define __SYSTEM_INTERNAL_H__

#include "../i_defs.h"

#include <sys/ioctl.h>

#ifndef MACOSX
#include <linux/cdrom.h>
#endif

// I_CD.C
bool I_StartupCD(void);
bool I_CDStartPlayback(int tracknum);
void I_CDPausePlayback(void);
void I_CDResumePlayback(void);
void I_CDStopPlayback(void);
void I_CDSetVolume(int vol);
bool I_CDFinished(void);
void I_ShutdownCD(void);

// I_MUSIC.C
extern bool musicpaused;
void I_PostMusicError(const char *message);

// I_MUSSRV.C
bool I_StartupMusserv(void);
bool I_MusservStartPlayback(const char *data, int len);
void I_MusservPausePlayback(void);
void I_MusservResumePlayback(void);
void I_MusservStopPlayback(void);
void I_MusservSetVolume(int vol);
void I_ShutdownMusserv(void);

// I_FMPAT.C
void I_CreateGENMIDI(byte *dest);

// I_USER.C
void I_MessageBox(const char *message, const char *title, int mode);

#endif // __SYSTEM_INTERNAL_H__
