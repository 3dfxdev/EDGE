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


/* export the playback mode */

int play_mode_rate = DEFAULT_RATE;
int play_mode_encoding = PE_16BIT|PE_SIGNED;


int ctl_verbosity = 2;


void ctl_msg(int type, int verbosity, char *fmt, ...)
{
	va_list ap;

	if ((type==CMSG_TEXT || type==CMSG_INFO || type==CMSG_WARNING) &&
		(verbosity > ctl_verbosity))
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


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
