//-----------------------------------------------------------------------------
//  EDGE MACOSX CD Handling Code (Empty Stubs)
//-----------------------------------------------------------------------------
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
//  DESCRIPTION:
// 	-AJA- Empty stubs, to get MacOSX version to compile.
// 	      Using SDL's CD functions might be the way to go...
//

#include "../i_defs.h"
#include "i_sdlinc.h"


static SDL_CD *cd_dev;  // NULL when CD system is inactive

static int cd_num_drives;
static int cd_drive_idx;
static bool cd_playing;

static int currcd_track = 0;
static bool currcd_loop = false;
static float currcd_gain = 0.0f;

static void MusicErrorPrintf(const char *fmt, ...)
{
	static char errmsg[1024];

	va_list argptr;

	va_start (argptr, fmt);
	vsprintf (errmsg, fmt, argptr);
	va_end (argptr);

	I_PostMusicError(errmsg);
}

//
// I_StartupCD
//
// This routine checks attempts to init the CDROM device.
// Returns true is successful
//
bool I_StartupCD(void)
{
	I_Printf("I_StartupCD: Initializing...\n");

	cd_dev = NULL;
	cd_playing = false;

	// try to open the CD drive

	if (SDL_Init(SDL_INIT_CDROM) != 0)
	{
		MusicErrorPrintf("I_StartupCD: Can't init CDROM system in SDL");
		return false;
	}

	cd_num_drives = SDL_CDNumDrives();

	if (cd_num_drives <= 0)
	{
		MusicErrorPrintf("I_StartupCD: no CDROM drives found");
		return false;
	}

	I_Printf("I_StartupCD: found %d CDROM drive%s\n", cd_num_drives,
		(cd_num_drives == 1) ? "" : "s");

	cd_drive_idx = 0;  // FIXME: use CheckParm() ?

	cd_dev = SDL_CDOpen(cd_drive_idx);

	if (! cd_dev)
	{
		MusicErrorPrintf("I_StartupCD: Can't open CDROM drive: %s", SDL_GetError());
		return false;
	}

	I_Printf("I_StartupCD: Init OK, using drive #%d: %s\n", cd_drive_idx,
		SDL_CDName(cd_drive_idx));
	return true;	
}

//
// I_ShutdownCD
//
// Closes the cdrom device
//
void I_ShutdownCD()
{
	if (cd_dev)
	{
		I_CDStopPlayback();

		SDL_CDClose(cd_dev);
		cd_dev = NULL;
	}
}

//
// I_CDStartPlayback
//
// Attempts to play CD track 'tracknum', returns true on success.
//
bool I_CDStartPlayback(int tracknum, bool loop, float gain)
{
	if (cd_playing)
		I_CDStopPlayback();

	// this call also updates the SDL_CD information
	CDstatus st = SDL_CDStatus(cd_dev);

	if (! CD_INDRIVE(st))
	{
		MusicErrorPrintf("I_CDStartPlayback: no CD in drive.");
		return false;
	}

	if (tracknum >= cd_dev->numtracks)
	{
		MusicErrorPrintf("I_CDStartPlayback: no such track #%d.", tracknum);
		return false;
	}

	if (SDL_CDPlayTracks(cd_dev, tracknum, 0, 1, 0) < 0)
	{
		MusicErrorPrintf("I_CDStartPlayback: unable to play track #%d: %s",
			tracknum, SDL_GetError());
		return false;
	}

	currcd_track = tracknum;
	currcd_loop  = loop;
	I_CDSetVolume(gain);

	cd_playing = true;
	return true;
}

//
// I_CDPausePlayback
//
// Paused the playing CD
//
bool I_CDPausePlayback(void)
{
	return (SDL_CDPause(cd_dev) == 0);
}

//
// I_CDresumePlayback
//
// Resumes the paused CD
//
bool I_CDResumePlayback(void)
{
	return (SDL_CDResume(cd_dev) == 0);
}

//
// XDoom calls this function to stop playing the current song.
//
void I_CDStopPlayback(void)
{
	SDL_CDStop(cd_dev);
	cd_playing = false;
}

//
// I_CDSetVolume
//
void I_CDSetVolume(float gain)
{
	// FIXME: no SDL function for changing CD volume

	currcd_gain = gain;
}

//
// I_CDFinished
//
// Has the CD Finished?
//
bool I_CDFinished(void)
{
	CDstatus st = SDL_CDStatus(cd_dev);

	return (st == CD_STOPPED);  // what about CD_PAUSED ??
}

//
// I_CDTicker
//
bool I_CDTicker(void)
{
	if (currcd_loop && I_CDFinished())
	{
		I_CDStopPlayback();  //???

		if (! I_CDStartPlayback(currcd_track, currcd_loop, currcd_gain))
			return false;
	}

	return true;
}
