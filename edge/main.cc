//----------------------------------------------------------------------------
//  EDGE Main
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2006  The EDGE Team.
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

#include "src/i_sdlinc.h"  // needed for proper SDL main linkage

extern "C" {

extern int I_Main(int argc, char *argv[]);

int main(int argc, char *argv[])
{
	return I_Main(argc, argv);
}

} // extern "C"


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
