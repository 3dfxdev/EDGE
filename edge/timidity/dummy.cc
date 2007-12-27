/* 
    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    dummy.c

    Minimal control mode -- no interaction, just stores messages.
*/

#include "config.h"

#include "common.h"
#include "output.h"
#include "ctrlmode.h"
#include "instrum.h"
#include "playmidi.h"

extern ControlMode dummy_ctl;


/* export the playback mode */

static PlayMode dummy_play_mode =
{
	DEFAULT_RATE, PE_16BIT|PE_SIGNED,
	"EPI Audio"
};

PlayMode *play_mode_list[] =
{
	&dummy_play_mode,
	0
};

PlayMode *play_mode = &dummy_play_mode;


static void ctl_refresh(void) { }
static void ctl_total_time(int tt) {}
static void ctl_master_volume(int mv) {}
static void ctl_file_name(char *name) {}
static void ctl_current_time(int ct) {}
static void ctl_note(int v) {}
static void ctl_program(int ch, int val) {}
static void ctl_volume(int channel, int val) {}
static void ctl_expression(int channel, int val) {}
static void ctl_panning(int channel, int val) {}
static void ctl_sustain(int channel, int val) {}
static void ctl_pitch_bend(int channel, int val) {}
static void ctl_reset(void) {}


static int ctl_open(int using_stdin, int using_stdout)
{
	dummy_ctl.opened = 1;
	return 0;
}

static void ctl_close(void)
{ 
	dummy_ctl.opened = 0;
}

static int ctl_read(int32 *valp)
{
	return 0;
}

static void cmsg(int type, int verbosity, char *fmt, ...)
{
	va_list ap;

	if ((type==CMSG_TEXT || type==CMSG_INFO || type==CMSG_WARNING) &&
		(dummy_ctl.verbosity < verbosity))
		return;

	char buffer[2048];

	buffer[2047] = 0;

	va_start(ap, fmt);
	vsprintf(buffer, fmt, ap);
	va_end(ap);

	if (verbosity >= VERB_DEBUG)
		I_Debugf("Timidity: %s\n", buffer);
	else
		I_Printf("Timidity: %s\n", buffer);
}

ControlMode dummy_ctl = 
{
	"dummy interface", 's',
	2,  // verbosity
	0,  // trace_playing
	0,  // opened
	ctl_open, NULL, ctl_close, ctl_read, cmsg,
	ctl_refresh, ctl_reset, ctl_file_name, ctl_total_time,
	ctl_current_time, ctl_note, 
	ctl_master_volume, ctl_program, ctl_volume, 
	ctl_expression, ctl_panning, ctl_sustain, ctl_pitch_bend
};

ControlMode *ctl_list[] = 
{
  &dummy_ctl,
  0
};

ControlMode *ctl = &dummy_ctl;


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
