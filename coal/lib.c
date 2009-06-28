/*  Copyright (C) 1996-1997  Id Software, Inc.

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

    See file, 'COPYING', for details.
*/

// cmdlib.c

#include "lib.h"
#include <sys/types.h>
#include <sys/stat.h>

#ifdef WIN32
#include <direct.h>
#endif

#ifdef NeXT
#include <libc.h>
#endif



/*
=================
Error

For abnormal program terminations
=================
*/
void Error (char *error, ...)
{
	va_list argptr;

	printf ("************ ERROR ************\n");

	va_start (argptr,error);
	vprintf (error,argptr);
	va_end (argptr);
	printf ("\n");
	exit (1);
}



/*
=============================================================================

						MISC FUNCTIONS

=============================================================================
*/


static int filelength (FILE *f)
{
	int pos = ftell (f);

	fseek (f, 0, SEEK_END);

	int end = ftell (f);

	fseek (f, pos, SEEK_SET);

	return end;
}


int LoadFile (char *filename, void **bufferptr)
{
	FILE *f = fopen(filename, "rb");

	if (!f)
		Error ("Error opening %s: %s", filename, strerror(errno));

	int length = filelength (f);
	void *buffer = malloc (length+1);

	if ( fread (buffer, 1, length, f) != (size_t)length)
		Error ("File read failure");

	fclose (f);

	((char *)buffer)[length] = 0;
	
	*bufferptr = buffer;
	return length;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
