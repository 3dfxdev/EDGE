//------------------------------------------------------------------------
//  UTILITIES
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor (C) 2001-2009 Andrew Apted
//                     (C) 1997-2003 André Majorel et al
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

// #include "m_game.h"

#include <math.h>
#include <sys/time.h>
#include <time.h>



/*
 *  spec_path
 *  Extract the path of a spec
 */
const char *spec_path (const char *spec)
{
	static char path[Y_PATH + 1];
	size_t n;

	*path = '\0';
	strncat (path, spec, sizeof path - 1);
	for (n = strlen (path); n > 0 && (path[n-1] != '/'); n--)
		;
	path[n] = '\0';
	return path;
}


/*
 *  fncmp
 *  Compare two filenames
 *  For Unix, it's a simple strcmp.
 *  For DOS, it's case insensitive and "/" and "\" are equivalent.
 *  FIXME: should canonicalize both names and compare that.
 */
int fncmp (const char *name1, const char *name2)
{
#if defined Y_DOS
	char c1, c2;
	for (;;)
	{
		c1 = tolower ((unsigned char) *name1++);
		c2 = tolower ((unsigned char) *name2++);
		if (c1=='\\')
			c1 = '/';
		if (c2=='\\')
			c2 = '/';
		if (c1 != c2)
			return c1-c2;
		if (!c1)
			return 0;
	}
#elif defined Y_UNIX
	return strcmp (name1, name2);
#endif
}


/*
 *  is_absolute
 *  Tell whether a file name is absolute or relative.
 *
 *  Note: for DOS, a filename of the form "d:foo" is
 *  considered absolute, even though it's technically
 *  relative to the current working directory of "d:".
 *  My reasoning is that someone who wants to specify a
 *  name that's relative to one of the standard
 *  directories is not going to put a "d:" in front of it.
 */
int is_absolute (const char *filename)
{
#if defined Y_UNIX
	return *filename == '/';
#elif defined Y_DOS
	return *filename == '/'
		|| *filename == '\\'
		|| isalpha (*filename) && filename[1] == ':';
#endif
}


/*
 *  y_stricmp
 *  A case-insensitive strcmp()
 *  (same thing as DOS stricmp() or GNU strcasecmp())
 */
int y_stricmp (const char *s1, const char *s2)
{
	for (;;)
	{
		if (tolower (*s1) != tolower (*s2))
			return (unsigned char) *s1 - (unsigned char) *s2;
		if (! *s1)
			if (! *s2)
				return 0;
			else
				return -1;
		if (! *s2)
			return 1;
		s1++;
		s2++;
	}
}


/*
 *  y_strnicmp
 *  A case-insensitive strncmp()
 *  (same thing as DOS strnicmp() or GNU strncasecmp())
 */
int y_strnicmp (const char *s1, const char *s2, size_t len)
{
	while (len-- > 0)
	{
		if (tolower (*s1) != tolower (*s2))
			return (unsigned char) *s1 - (unsigned char) *s2;
		if (! *s1)
			if (! *s2)
				return 0;
			else
				return -1;
		if (! *s2)
			return 1;
		s1++;
		s2++;
	}
	return 0;
}


/*
 *  y_snprintf
 *  If available, snprintf(). Else sprintf().
 */
int y_snprintf (char *buf, size_t size, const char *fmt, ...)
{
	va_list args;
	va_start (args, fmt);
#ifdef Y_SNPRINTF
	return vsnprintf (buf, size, fmt, args);
#else
	return vsprintf (buf, fmt, args);
#endif
}


/*
 *  y_vsnprintf
 *  If available, vsnprintf(). Else vsprintf().
 */
int y_vsnprintf (char *buf, size_t size, const char *fmt, va_list args)
{
#ifdef Y_SNPRINTF
	return vsnprintf (buf, size, fmt, args);
#else
	return vsprintf (buf, fmt, args);
#endif
}


/*
 *  y_strupr
 *  Upper-case a string
 */
void y_strupr (char *string)
{
	while (*string)
	{
		*string = toupper (*string);
		string++;
	}
}



/*
 *  file_exists
 *  Check whether a file exists and is readable.
 *  Returns true if it is, false if it isn't.
 */
bool file_exists (const char *filename)
{
	FILE *test;

	if ((test = fopen (filename, "rb")) == NULL)
		return 0;
	fclose (test);
	return 1;
}


/*
 *  y_filename
 *  Copies into <buf> a string that is a close as possible
 *  to <filename> but is guaranteed to be no longer than
 *  <size> - 1 and contain only printable characters. Non
 *  printable characters are replaced by question marks.
 *  Excess characters are replaced by an ellipsis.
 */
void y_filename (char *buf, size_t size, const char *filename)
{
	if (size == 0)
		return;
	if (size == 1)
	{
		*buf = 0;
		return;
	}
	size_t len    = strlen (filename);
	size_t maxlen = size - 1;

	if (len > 3 && maxlen <= 3)  // Pathological case, fill with dots
	{
		memset (buf, '.', maxlen);
		buf[maxlen] = '\0';
		return;
	}

	size_t len1 = len;
	size_t len2 = 0;
	if (len > maxlen)
	{
		len1 = (maxlen - 3) / 2;
		len2 = maxlen - 3 - len1;
	}
	char *p = buf;
	for (size_t n = 0; n < len1; n++)
	{
		*p++ = y_isprint (*filename) ? *filename : '?';
		filename++;
	}
	if (len2 > 0)
	{
		*p++ = '.';
		*p++ = '.';
		*p++ = '.';
		filename += len - len1 - len2;
		for (size_t n = 0; n < len2; n++)
		{
			*p++ = y_isprint (*filename) ? *filename : '?';
			filename++;
		}
	}
	*p++ = 0;
}
 

unsigned long y_milliseconds()
{
#ifdef WIN32
	return GetTickCount();
#else
	struct timeval tv;
	struct timezone tz;

	gettimeofday (&tv, &tz);

	return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
}


/*
 *  check_types
 *
 *  Sanity checks about the sizes and properties of certain types.
 *  Useful when porting.
 */

#define assert_size(type,size)            \
  do                  \
  {                 \
    if (sizeof (type) != size)            \
      FatalError("sizeof " #type " is %d (should be " #size ")",  \
  (int) sizeof (type));           \
  }                 \
  while (0)
   
#define assert_wrap(type,high,low)          \
  do                  \
  {                 \
    type n = high;              \
    if (++n != low)             \
      FatalError("Type " #type " wraps around to %lu (should be " #low ")",\
  (unsigned long) n);           \
  }                 \
  while (0)

void check_types ()
{
	assert_size (u8_t,  1);
	assert_size (s8_t,  1);
	assert_size (u16_t, 2);
	assert_size (s16_t, 2);
	assert_size (u32_t, 4);
	assert_size (s32_t, 4);

	assert_size (struct LineDef, 14);
	assert_size (struct Sector,  26);
	assert_size (struct SideDef, 30);
	assert_size (struct Thing,   10);
	assert_size (struct Vertex,   4);
}


/*
   translate (dx, dy) into an integer angle value (0-65535)
*/

unsigned ComputeAngle (int dx, int dy)
{
	return (unsigned) (atan2 ((double) dy, (double) dx) * 10430.37835 + 0.5);
}



/*
   compute the distance from (0, 0) to (dx, dy)
*/

unsigned ComputeDist (int dx, int dy)
{
	return (unsigned) (hypot ((double) dx, (double) dy) + 0.5);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
