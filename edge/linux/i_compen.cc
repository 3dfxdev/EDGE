//----------------------------------------------------------------------------
//  EDGE License Compensate Functions
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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
// -ACB- 1999/10/10
//

#include "../i_defs.h"
#include "../dm_defs.h"

#include <ctype.h>
#include <stdlib.h>

void strupr(char *str)
{
	if (str == NULL)
		return;

	while (*str)
	{
		*str = toupper(*str);
		str++;
	}
}

int strncasecmpwild(const char *s1, const char *s2, int n)
{
	int i = 0;

	for (i = 0; s1[i] && s2[i] && i < n; i++)
	{
		if ((toupper(s1[i]) != toupper(s2[i])) && (s1[i] != '?') && (s2[i] != '?'))
			break;
	}
	// -KM- 1999/01/29 If strings are equal return equal.
	if (i == n)
		return 0;

	if (s1[i] == '?' || s2[i] == '?')
		return 0;

	return s1[i] - s2[i];
}

