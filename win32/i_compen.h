//----------------------------------------------------------------------------
//  EDGE Win32 Compensate Functions
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2001  The EDGE Team.
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
// DJGPP, but not included in MSVC.
//
// -ACB- 1999/09/22
//

int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, int max);
int strncasecmpwild(const char *s1, const char *s2, int n);
