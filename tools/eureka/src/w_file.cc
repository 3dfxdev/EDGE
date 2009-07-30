//------------------------------------------------------------------------
//  WAD FILES
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
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
//------------------------------------------------------------------------
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#include "main.h"
#include "w_file.h"

#define Wad_file  Wad_file0

/*
 *  Wad_file::~Wad_file - dtor
 */
Wad_file::~Wad_file ()
{
	if (directory != 0)
	{
		FreeMemory (directory);
		directory = 0;      // Catch bugs
	}
	if (fp != 0)
	{
		fclose (fp);
		fp = 0;       // Catch bugs
	}
	if (filename != 0)
	{
		FreeMemory (filename);
		filename = 0;     // Catch bugs
	}
}


/*
 *  Wad_file::where - return file(offset) string
 *
 *  Return pointer to a per-Wad_file buffer.
 */
const char *Wad_file::where () const
{
	const unsigned long offset       = ftell (fp);
	const size_t        offset_len   =  + 3;
	const size_t        name_len_max = sizeof where_ - 1 - offset_len;

	if (name_len_max >= strlen (filename)) 
		sprintf (where_, "%s(%lXh)", filename, offset);
	else
	{
		const char  *ellipsis = "...";
		const size_t total    = name_len_max - strlen (ellipsis);
		const size_t left     = total / 2;
		const size_t right    = total - left;
		sprintf (where_, "%*s%s%*s(%lXh)",
				left, filename,
				ellipsis,
				right, filename + total,
				offset);
	}

	return where_;
}


/*
 *  Wad_file::read_vbytes - read bytes from a wad file
 *
 *  Read up to <count> bytes and store them into buffer
 *  <buf>. <count> is _not_ limited to size_t. If an I/O
 *  error occurs, set the error flag. EOF is not considered
 *  an error.
 *
 *  Return the number of bytes read.
 */
long Wad_file::read_vbytes (void *buf, long count) const
{
	long bytes_read_total;
	size_t bytes_read;
	size_t bytes_to_read;

	bytes_read_total = 0;
	bytes_to_read    = 0x8000;
	while (count > 0)
	{
		if (count <= 0x8000)
			bytes_to_read = (size_t) count;
		bytes_read = fread (buf, 1, bytes_to_read, fp);
		bytes_read_total += bytes_read;
		if (bytes_read != bytes_to_read)
			break;
		buf = (char *) buf + bytes_read;
		count -= bytes_read;
	}
	if (ferror (fp))
	{
		if (! error_)
			err ("%s: read error", where ());
		error_ = true;
	}
	return bytes_read_total;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
