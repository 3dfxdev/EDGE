//-----------------------------------------------------------------------------
//  EDGE LINUX CD Handling Code
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
// 	This CD audio module uses the Linux ioctl()
// 	interface, to play audio CD's.
//
//-----------------------------------------------------------------------------
//
// -ACB- 2000/03/17 Based Heavily on Udo Munks CD Handling Code from his
//                  excellent Port of the Doom Engine for X (every flavour
//                  known to man) - XDoom.
//
#include "../i_defs.h"
#include "i_sysinc.h"

#define DRIVE "/dev/cdrom"

static int fd;				// filedescriptor for the CD drive
static struct cdrom_tochdr toc_header;	// TOC header
static struct cdrom_ti ti;		// track/index to play
static struct cdrom_volctrl volume;	// volume

//
// I_StartupCD
//
// This routine checks attempts to init the CDROM device. Returns true is successful
//
bool I_StartupCD(void)
{
  L_WriteDebug("I_StartupCD: Initializing...\n");

  // try to open the CD drive
  if ((fd = open(DRIVE, O_RDONLY)) < 0)
  {
    I_PostMusicError("I_StartupCD: Can't oopen CDROM device");
    return false;
  }

  // turn drives motor on, some drives won't work without
  if (ioctl(fd, CDROMSTART) != 0)
  {
    I_PostMusicError("I_StartupCD: Can't start drive motor\n");
    return false;
  }

  // get the TOC header, so we know how many audio tracks
  if (ioctl(fd, CDROMREADTOCHDR, &toc_header) != 0)
  {
    I_PostMusicError("I_StartupCD: can't read TOC header\n");
    return false;	
  }

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
  struct cdrom_tocentry toc;
	
  // get toc entry for the track
  toc.cdte_track = tracknum;
  toc.cdte_format = CDROM_MSF;
  ioctl(fd, CDROMREADTOCENTRY, &toc);

  // is this an audio track?
  if (toc.cdte_ctrl & CDROM_DATA_TRACK)
  {
    L_WriteDebug("I_CDStartPlayback: Cannot play CD track: %d\n", tracknum);
    return false;
  }

  ti.cdti_trk0 = tracknum;
  ti.cdti_ind0 = 0;
  ti.cdti_trk1 = tracknum;
  ti.cdti_ind1 = 0;
  ioctl(fd, CDROMPLAYTRKIND, &ti);

  return true;
}

//
// I_CDPausePlayback
//
// Paused the playing CD
//
void I_CDPausePlayback(void)
{
  ioctl(fd, CDROMPAUSE);
}

//
// I_CDresumePlayback
//
// Resumes the paused CD
//
void I_CDResumePlayback(void)
{
  ioctl(fd, CDROMRESUME);
}

//
// XDoom calls this function to stop playing the current song.
//
void I_CDStopPlayback(void)
{
  ioctl(fd, CDROMSTOP);
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
    volume.channel0 =  vol * 255 / 15;
    volume.channel1 =  vol * 255 / 15;
    volume.channel2 =  0;
    volume.channel3 =  0;
    ioctl(fd, CDROMVOLCTRL, &volume);
  }
}

//
// I_CDFinished
//
// Has the CD Finished
//
bool I_CDFinished(void)
{
  static struct cdrom_subchnl subchnl;

  subchnl.cdsc_format = CDROM_MSF;	// get result in MSF format
  ioctl(fd, CDROMSUBCHNL, &subchnl);

  if (subchnl.cdsc_audiostatus == CDROM_AUDIO_COMPLETED)
    return true;

  return false;
}

//
// I_ShutdownCD
//
// Closes the cdrom device
//
void I_ShutdownCD()
{
  ioctl(fd, CDROMSTOP);
  close(fd);
}
