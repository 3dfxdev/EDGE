//----------------------------------------------------------------------------
//  EDGE2 Sound System for SDL
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

#include <kos.h>

#include "i_defs.h"

#include <fcntl.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/time.h>

#include "m_argv.h"
#include "m_misc.h"
#include "m_random.h"
#include "s_music.h"
#include "s_sound.h"
#include "w_wad.h"

#include <dc/sound/sound.h>
#include <dc/sound/sfxmgr.h>
#include <dc/sound/stream.h>

extern "C" {
#include "libWildMidi/wildmidi_lib.h"
}

// If true, sound system is off/not working. Changed to false if sound init ok.
bool nosound = false;
bool nomidi = false;

static mutex_t l = MUTEX_INITIALIZER;

static uint32 midi_status = 0;
static kthread_t * midi_thread = NULL;
extern snd_stream_hnd_t stream_hnd;


static void *do_midi(void *argp)
{
    while (midi_status != 0xDEADBEEF)
    {
		thd_sleep(100);
		if (stream_hnd != SND_STREAM_INVALID)
        {
            I_LockAudio();
			snd_stream_poll(stream_hnd);
            I_UnlockAudio();
        }
    }
    return NULL;
}

void I_StartupSound(void)
{
	if (nosound)
	{
		I_Printf("I_StartupSound: %s\n", "no sound");
		return;
	}

	I_Printf("I_StartupSound: %s\n", "sound enabled");
}


void I_ShutdownSound(void)
{
	if (nosound)
		return;

	S_Shutdown();

    mutex_destroy(&l);

	nosound = true;
}

void I_LockAudio(void)
{
    if (mutex_lock(&l) < 0)
        I_Error("I_LockAudio: mutex_lock failed!\n");
}

void I_UnlockAudio(void)
{
    mutex_unlock(&l);
}

void I_ShutdownMusic(void)
{
	if (nomusic)
		return;

	if (!nomidi)
	{
		midi_status = 0xDEADBEEF; // DIE, DAEMON! DIE!!
		thd_sleep(200);
		WildMidi_Shutdown();
	}

	snd_stream_shutdown();
}

void I_StartupMusic(void)
{
	if (nomusic)
	{
		I_Printf("I_StartupMusic: %s\n", "no music");
		return;
	}

	I_Printf("I_StartupMusic: %s\n", "snd_stream_init()");
	snd_stream_init();

	I_Printf("I_StartupMusic: %s\n", "WildMidi_Init()");
    if (WildMidi_Init("/cd/timidity/timidity.cfg", 44100, WM_MO_LINEAR_VOLUME) == -1)
    {
		I_Printf(" %s\n", "MIDI not available");
		nomidi = true;
	}
    //Start midi polling thread
    midi_status = 0;
    midi_thread = thd_create(0, do_midi, NULL);
    if(!midi_thread)
    {
        I_Printf ("I_StartupMusic: Cannot create midi daemon, no midi music.\n");
        nomidi = true;
        WildMidi_Shutdown();
    }
    thd_sleep(100);

	I_Printf("I_StartupMusic: %s\n", "music enabled");
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
