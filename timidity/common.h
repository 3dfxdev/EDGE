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

	common.h
*/

#ifndef __TIMIDITY_COMMON_H__
#define __TIMIDITY_COMMON_H__

extern char *program_name, current_filename[];

extern FILE *msgfp;

extern int num_ochannels;

#define MULTICHANNEL_OUT
#define MAX_OUT_CHANNELS 6

typedef struct PathList_s
{
  char *path;
  struct PathList_s *next;
}
PathList;

/* Noise modes for open_file */
#define OF_SILENT	0
#define OF_NORMAL	1
#define OF_VERBOSE	2

extern FILE *open_file(const char *name, int decompress, int noise_mode);
extern void add_to_pathlist(char *s);
extern void free_pathlist(void);
extern void close_file(FILE *fp);
extern void skip(FILE *fp, size_t len);
extern void *safe_malloc(size_t count);
extern char *safe_strdup(const char *orig);

#endif /* __TIMIDITY_COMMON_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
