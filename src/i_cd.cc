//-----------------------------------------------------------------------------
//  EDGE MACOSX CD Handling Code (Empty Stubs)
//-----------------------------------------------------------------------------
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
//  DESCRIPTION:
// 	-AJA- Empty stubs, to get MacOSX version to compile.
// 	      Using SDL's CD functions might be the way to go...
//

#include "../i_defs.h"
#include "i_sysinc.h"

//
// I_StartupCD
//
// This routine checks attempts to init the CDROM device. Returns true is successful
//
bool I_StartupCD(void)
{
  L_WriteDebug("I_StartupCD: Initializing...\n");

  // try to open the CD drive
  if (1)
  {
    I_PostMusicError("I_StartupCD: Can't oopen CDROM device");
    return false;
  }

  // FIXME

  L_WriteDebug("I_StartupCD: CD Init OK\n");
  return true;	
}

//
// I_CDStartPlayback
//
// Attempts to play CD track 'tracknum', returns true on success.
//
bool I_CDStartPlayback(int tracknum)
{
  // FIXME
  return true;
}

//
// I_CDPausePlayback
//
// Paused the playing CD
//
void I_CDPausePlayback(void)
{
  // FIXME
}

//
// I_CDresumePlayback
//
// Resumes the paused CD
//
void I_CDResumePlayback(void)
{
  // FIXME
}

//
// XDoom calls this function to stop playing the current song.
//
void I_CDStopPlayback(void)
{
  // FIXME
}

//
// I_CDSetVolume
//
// Sets the CD Volume
//
void I_CDSetVolume(int vol)
{
  if ((vol >= 0) && (vol <= 15))
  {
    // FIXME
  }
}

//
// I_CDFinished
//
// Has the CD Finished
//
bool I_CDFinished(void)
{
  // FIXME
  return false;
}

//
// I_ShutdownCD
//
// Closes the cdrom device
//
void I_ShutdownCD()
{
  // FIXME
}
