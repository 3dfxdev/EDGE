//-----------------------------------------------------------------------------
//  EDGE SDL CD Handling Code
//-----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2008  The EDGE Team.
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
//-----------------------------------------------------------------------------

#include "i_defs.h"
#include "i_sdlinc.h"

#include <stdarg.h>

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
// This routine checks attempts to init the CDROM device.
// Returns true is successful
//
bool I_StartupCD(void)
{
	I_Printf("I_StartupCD: Initializing...\n");

	cd_dev = NULL;
	cd_playing = false;

	// try to open the CD drive

	if (SDL_InitSubSystem(SDL_INIT_CDROM) != 0)
	{
		I_Printf("I_StartupCD: Can't init CDROM system in SDL\n");
		return false;
	}

	cd_num_drives = SDL_CDNumDrives();

	if (cd_num_drives <= 0)
	{
		I_Printf("I_StartupCD: no CDROM drives found\n");
		return false;
	}

	I_Printf("I_StartupCD: found %d CDROM drive%s\n", cd_num_drives,
		(cd_num_drives == 1) ? "" : "s");

	cd_drive_idx = 0;  // FIXME: use CheckParm() ?

	cd_dev = SDL_CDOpen(cd_drive_idx);

	if (! cd_dev)
	{
		I_Printf("I_StartupCD: Can't open CDROM drive: %s", SDL_GetError());
		return false;
	}

	I_Printf("I_StartupCD: Init OK, using drive #%d: %s\n", cd_drive_idx,
		SDL_CDName(cd_drive_idx));

	return true;
}


void I_ShutdownCD()
{
	/* Closes the cdrom device */

	if (cd_dev)
	{
		I_CDStopPlayback();

		SDL_CDClose(cd_dev);
		cd_dev = NULL;
	}
}


abstract_music_c * I_PlayCDMusic(int tracknum, float volume, bool loop)
{
	/* Attempts to play CD track 'tracknum' */

	if (cd_playing)
		I_CDStopPlayback();

	// this call also updates the SDL_CD information
	CDstatus st = SDL_CDStatus(cd_dev);

	if (! CD_INDRIVE(st))
	{
		MusicErrorPrintf("I_CDStartPlayback: no CD in drive.");
		return NULL;
	}

	if (tracknum >= cd_dev->numtracks)
	{
		MusicErrorPrintf("I_CDStartPlayback: no such track #%d.", tracknum);
		return NULL;
	}

	if (SDL_CDPlayTracks(cd_dev, tracknum, 0, 1, 0) < 0)
	{
		MusicErrorPrintf("I_CDStartPlayback: unable to play track #%d: %s",
			tracknum, SDL_GetError());
		return NULL;
	}

	currcd_track = tracknum;
	currcd_loop  = loop;

	I_CDSetVolume(volume);

	cd_playing = true;
	
	return NULL;  // FIXME !!!!!!
}

//
// Paused the playing CD
//
bool I_CDPausePlayback(void)
{
	return (SDL_CDPause(cd_dev) == 0);
}

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


void I_CDSetVolume(float gain)
{
	// FIXME: no SDL function for changing CD volume

	currcd_gain = gain;
}


// Has the CD Finished?
//
bool I_CDFinished(void)
{
	CDstatus st = SDL_CDStatus(cd_dev);

	return (st == CD_STOPPED);  // what about CD_PAUSED ??
}

bool I_CDTicker(void)
{
	if (currcd_loop && I_CDFinished())
	{
		I_CDStopPlayback();  //???

		if (SDL_CDPlayTracks(cd_dev, currcd_track, 0, 1, 0) < 0)
			return false;
	}

	return true;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
