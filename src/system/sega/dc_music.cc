//----------------------------------------------------------------------------
//  EDGE2 Dreamcast Music System
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2009  The EDGE2 Team.
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
// -ACB- 2000/03/17 Added CD Music
// -ACB- 2001/01/14 Replaced I_WriteDebug() with I_PostMusicError()
//

#include "system/i_defs.h"

#include "../ddf/main.h"
#include "../ddf/playlist.h"

#include "s_sound.h"
#include "s_music.h"
#include "s_opl.h"
#include "s_timid.h"
#include "dc_sysinc.h"


#define MUSICERRLEN 256
static char errordesc[MUSICERRLEN];

bool musicpaused = false;


extern int dev_freq;
extern bool dev_stereo;


void I_StartupMusic(void)
{
	// Clear the error message
	memset(errordesc, '\0', sizeof(errordesc));

	if (nomusic) return;

#if 1
	if (! nosound)
	{
		if (S_StartupOPL())
		{
			I_Printf("I_StartupMusic: OPL Init OK\n");
		}
		else
		{
			I_Printf("I_StartupMusic: OPL Init FAILED\n");
		}
		
		if (S_StartupTimidity())
		{
			I_Printf("I_StartupMusic: Timidity Init OK\n");
		}
		else
		{
			I_Printf("I_StartupMusic: Timidity Init FAILED\n");
		}
	}
	else
#endif
    {
		I_Printf("I_StartupMusic: Timidity Disabled\n");
    }

	// Music is not paused by default
	musicpaused = false;

	// Nothing went pear shaped
	return;
}


void I_ShutdownMusic(void)
{
	S_ShutdownOPL();
}


void I_PostMusicError(const char *message)
{
	memset(errordesc, 0, MUSICERRLEN);

	if (strlen(message) > MUSICERRLEN-1)
		strncpy(errordesc, message, MUSICERRLEN-1);
	else
		strcpy(errordesc, message);

	return;
}


char *I_MusicReturnError(void)
{
	return errordesc;
}

// -ACB- 2010/12/20 Moved here as Mac OSX does have a MIDI synthesizer
abstract_music_c * I_PlayNativeMusic(const byte *data, int length,
									 float volume, bool loop)
{
	// Linux has no built-in MIDI synthesizer
	return NULL;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
