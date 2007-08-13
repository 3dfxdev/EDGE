//----------------------------------------------------------------------------
//  EDGE Win32 Compensate Functions
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2007  The EDGE Team.
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
// This file accounts for functions in
// that are part of some standard libraries
// but not others.
//
// -ACB- 1999/09/22
//
#include "i_defs.h"

#include <ctype.h>

#if 0  // NOT NEEDED
int strcasecmp(const char *s1, const char *s2)
{
	int count;

	count = 0;

	while(!(s1[count] == '\0' && s2[count] == '\0'))
	{
		if ((s1[count]&0xDF) != (s2[count]&0xDF))
			return ((s1[count]&0xDF)-(s2[count]&0xDF));

		count++;
	}

	return 0;
}

int strncasecmp(const char *s1, const char *s2, int max)
{
	int count;

	count = 0;

	while(count < max)
	{
		if ((s1[count]&0xDF) != (s2[count]&0xDF))
			return ((s1[count]&0xDF)-(s2[count]&0xDF));

		if (s1[count] == '\0' && s2[count] == '\0')
			return 0;

		count++;
	}

	return 0;
}
#endif

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
