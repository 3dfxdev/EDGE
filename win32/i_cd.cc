//----------------------------------------------------------------------------
//  EDGE WIN32 CD Handling Code
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
//
// -ACB- 1999/11/09 Written
//

#include "..\i_defs.h"
#include "i_sysinc.h"

#define CDDEVICE "cdaudio"

typedef struct
{
	WORD id;
	int currenttrack;
	int startpos;
	int finishpos;
	int pausedpos;
}
cdinfo_t;

static win32_mixer_t *mixer = NULL;
static cdinfo_t *currcd = NULL;
static MCIERROR errorcode;

static int currcd_track = 0;
static bool currcd_loop = false;
static float currcd_gain = 0.0f;

//
// I_StartupCD
//
bool I_StartupCD()
{
	// Insert code here...
	mixer = I_MusicLoadMixer(MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC);
	if (!mixer)
	{
		I_PostMusicError("I_StartupCD: Couldn't open the midi device");
		return false;
	}

	if (!I_MusicGetMixerVol(mixer, &mixer->originalvol))
	{
		I_MusicReleaseMixer(mixer);
		mixer = NULL;
		
		I_PostMusicError("I_StartupCD: Unable to get original volume");
		return false;
	}

	// Ensure all is quiet when we start
	I_MusicSetMixerVol(mixer, 0);

	return true;
}

//
// I_ShutdownCD
//
void I_ShutdownCD()
{
	// Close all our handled objects
	if (mixer)
	{
		I_MusicSetMixerVol(mixer, mixer->originalvol);
		I_MusicReleaseMixer(mixer);
		mixer = NULL;
	}

	I_CDStopPlayback();
	return;
}

//
// I_CDStartPlayback
//
// Initialises playing a CD-Audio Track, returns false on failure.
//
bool I_CDStartPlayback(int tracknum, bool loopy, float gain)
{
	MCI_OPEN_PARMS openparm;
	MCI_PLAY_PARMS playparm;
	MCI_SET_PARMS setparm;
	MCI_STATUS_PARMS statusparm;
	int numoftracks;
	char errordesc[256];

	// clear error description
	memset(errordesc, 0, sizeof(char)*256);

	if (currcd)
		I_CDStopPlayback();

	// create cd info object
	currcd = new cdinfo_t;
	if (!currcd)
	{
		I_PostMusicError("I_CDPlayTrack: Unable to open CD-Audio device\n");
		return false;
	}

	// open parameters
	openparm.dwCallback      = (DWORD_PTR)mainwindow;
	openparm.lpstrDeviceType = CDDEVICE;

	// Open MCI CD-Audio
	errorcode = mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE, (DWORD_PTR)&openparm);
	if (errorcode)
	{
		if (!mciGetErrorString(errorcode, errordesc, 128))
			I_PostMusicError("Unknown Error");
		else
			I_PostMusicError(errordesc);

		delete currcd;
		currcd = NULL;
		return false;
	}

	// Set global deviceid
	currcd->id = openparm.wDeviceID;

	// Get the status of MCI CD-Audio
	statusparm.dwCallback = (DWORD_PTR)mainwindow;
	statusparm.dwItem     = MCI_STATUS_MEDIA_PRESENT;

	errorcode = mciSendCommand(currcd->id, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&statusparm);
	if (errorcode || !statusparm.dwReturn)
	{
		if (!mciGetErrorString(errorcode, errordesc, 128))
			I_PostMusicError("Unknown Error");
		else
			I_PostMusicError(errordesc);

		mciSendCommand(currcd->id, MCI_CLOSE, 0, (DWORD)NULL);
		delete currcd;
		currcd = NULL;
		return false;
	}

	// Get the number of CD Tracks
	statusparm.dwCallback = (DWORD_PTR)mainwindow;
	statusparm.dwItem     = MCI_STATUS_NUMBER_OF_TRACKS;

	errorcode = mciSendCommand(currcd->id, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&statusparm);
	if (errorcode)
	{
		if (!mciGetErrorString(errorcode, errordesc, 128))
			I_PostMusicError("Unknown Error");
		else
			I_PostMusicError(errordesc);

		mciSendCommand(currcd->id, MCI_CLOSE, 0, (DWORD)NULL);
		delete currcd;
		currcd = NULL;
		return false;
	}

	numoftracks = (DWORD)statusparm.dwReturn;
	if (tracknum >= numoftracks)
	{
		mciSendCommand(currcd->id, MCI_CLOSE, 0, (DWORD)NULL);
		I_PostMusicError("Track exceeds available tracks");
		delete currcd;
		currcd = NULL;
		return false;
	}

	// Get the status of the CD Track
	statusparm.dwCallback = (DWORD_PTR)mainwindow;
	statusparm.dwItem     = MCI_CDA_STATUS_TYPE_TRACK;
	currcd->currenttrack  = statusparm.dwTrack    = tracknum;

	errorcode = mciSendCommand(currcd->id, MCI_STATUS, MCI_STATUS_ITEM|MCI_TRACK, (DWORD_PTR)&statusparm);
	if (errorcode)
	{
		if (!mciGetErrorString(errorcode, errordesc, 128))
			I_PostMusicError("Unknown Error");
		else
			I_PostMusicError(errordesc);

		mciSendCommand(currcd->id, MCI_CLOSE, 0, (DWORD)NULL);
		delete currcd;
		currcd = NULL;
		return false;
	}

	// Check its an audio track....
	if (statusparm.dwReturn != MCI_CDA_TRACK_AUDIO)
	{
		mciSendCommand(currcd->id, MCI_CLOSE, 0, (DWORD)NULL);
		I_PostMusicError("Track is not Audio\n");
		delete currcd;
		currcd = NULL;
		return false;
	}

	// Setup time format
	setparm.dwTimeFormat = MCI_FORMAT_TMSF;
	mciSendCommand(currcd->id, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD_PTR)&setparm);

	// Setup play parameters
	playparm.dwCallback = (DWORD_PTR)mainwindow;
	currcd->startpos = playparm.dwFrom = MCI_MAKE_TMSF(tracknum, 0, 0, 0);

	// Check if last track...
	if (tracknum == (numoftracks-1))
	{
		currcd->finishpos = playparm.dwTo = 0L;
		errorcode = mciSendCommand(currcd->id, MCI_PLAY, MCI_NOTIFY|MCI_FROM, (DWORD_PTR)&playparm);
	}
	else
	{
		currcd->finishpos = playparm.dwTo = MCI_MAKE_TMSF(tracknum+1, 0, 0, 0);
		errorcode = mciSendCommand(currcd->id, MCI_PLAY, MCI_NOTIFY|MCI_FROM|MCI_TO, (DWORD_PTR)&playparm);
	}

	if (errorcode)
	{
		if (!mciGetErrorString(errorcode, errordesc, 128))
			I_PostMusicError("Unknown Error");
		else
			I_PostMusicError(errordesc);

		mciSendCommand(currcd->id, MCI_CLOSE, 0, (DWORD)NULL);
		delete currcd;
		currcd = NULL;
		return false;
	}

	currcd_track = tracknum;
	currcd_loop = loopy;
	I_CDSetVolume(gain);

	return true;
}

//
// I_CDPausePlayback
//
bool I_CDPausePlayback(void)
{
	MCI_SET_PARMS setparm;
	MCI_STATUS_PARMS statusparm;
	char errordesc[256];

	// clear error description
	memset(errordesc, 0, sizeof(char)*256);

	if (!currcd)
	{
		I_PostMusicError("CD has not been started\n");
		return false;
	}

	setparm.dwTimeFormat = MCI_FORMAT_TMSF;
	mciSendCommand(currcd->id, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD_PTR)&setparm);

	statusparm.dwCallback  = (DWORD_PTR)mainwindow;
	statusparm.dwItem = MCI_STATUS_POSITION;

	errorcode = mciSendCommand(currcd->id, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&statusparm);
	if (errorcode)
	{
		if (!mciGetErrorString(errorcode, errordesc, 128))
			I_PostMusicError("Unknown Error");
		else
			I_PostMusicError(errordesc);

		return false;
	}

	currcd->pausedpos = (DWORD)statusparm.dwReturn;

	mciSendCommand(currcd->id, MCI_STOP, 0, (DWORD)NULL);

	return true;
}

//
// I_CDResumePlayback
//
bool I_CDResumePlayback(void)
{
	MCI_SET_PARMS setparm;
	MCI_PLAY_PARMS playparm;
	char errordesc[256];

	// clear error description
	memset(errordesc, 0, sizeof(char)*256);

	if (!currcd)
	{
		I_PostMusicError( "CD has not been started\n");
		return false;
	}

	setparm.dwTimeFormat = MCI_FORMAT_TMSF;
	mciSendCommand(currcd->id, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD_PTR) &setparm);

	playparm.dwCallback = (DWORD_PTR)mainwindow;
	playparm.dwFrom     = currcd->pausedpos;
	playparm.dwTo       = currcd->finishpos;

	if (!currcd->finishpos)
		errorcode = mciSendCommand(currcd->id, MCI_PLAY, MCI_NOTIFY|MCI_FROM, (DWORD_PTR)&playparm);
	else
		errorcode = mciSendCommand(currcd->id, MCI_PLAY, MCI_NOTIFY|MCI_FROM|MCI_TO, (DWORD_PTR)&playparm);

	if (errorcode)
	{
		if (!mciGetErrorString(errorcode, errordesc, 128))
			I_PostMusicError("Unknown Error");
		else
			I_PostMusicError(errordesc);

		delete currcd;
		currcd = NULL;
		return false;
	}

	return true;
}

//
// I_CDStopPlayback
//
void I_CDStopPlayback(void)
{
	if (!currcd)
		return;

	mciSendCommand(currcd->id, MCI_STOP, 0, (DWORD)NULL);
	mciSendCommand(currcd->id, MCI_CLOSE, 0, (DWORD)NULL);
	delete currcd;
	currcd = NULL;
	return;
}

//
// I_CDFinished
//
// Has the CD Finished playing
//
bool I_CDFinished(void)
{
	MCI_STATUS_PARMS statusparm;

	if (!currcd)
		return false;

	// Get the status of MCI CD-Audio
	statusparm.dwCallback = (DWORD_PTR)mainwindow;
	statusparm.dwItem     = MCI_STATUS_MODE;

	mciSendCommand(currcd->id, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&statusparm);

	switch(statusparm.dwReturn)
	{
		case MCI_MODE_NOT_READY:
		case MCI_MODE_PLAY:
		case MCI_MODE_SEEK:
			return false;
	}

	return true;
}

//
// I_CDSetVolume
//
void I_CDSetVolume(float gain)
{
	if (!mixer)
		return;

	DWORD actualvol;

	// Too small...
	if (gain < 0)
		gain = 0;

	// Too big...
	if (gain > 1)
		gain = 1;

	// Calculate a given value in range
	actualvol = int(gain * (mixer->maxvol - mixer->minvol));
	actualvol += mixer->minvol;

	I_MusicSetMixerVol(mixer, actualvol);

	currcd_gain = gain;
}

//
// I_CDTicker
//
bool I_CDTicker(void)
{
	if (currcd_loop && I_CDFinished())
	{
		I_CDStopPlayback();

		if (! I_CDStartPlayback(currcd_track, currcd_loop, currcd_gain))
			return false;
	}

	return true;
}
