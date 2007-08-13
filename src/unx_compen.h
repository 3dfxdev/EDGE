//----------------------------------------------------------------------------
//  EDGE Linux Compensate Functions
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
// -ACB- 1999/10/10
//

#ifndef O_BINARY
#define O_BINARY  0
#endif

#ifndef D_OK
#define D_OK  X_OK
#endif

void strupr(char *str);

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
