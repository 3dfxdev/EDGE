//----------------------------------------------------------------------------
//  EDGE Version Header
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2000  The EDGE Team.
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

#define EDGEVER    0x125
#define EDGEVERFIX 0x100

// -ES- 2000/03/04 The version of EDGE.WAD we require.
#define EDGE_WAD_VERSION 2
#define EDGE_WAD_VERSION_FRAC 1
#define EDGE_WAD_SUB_VERSION 0

// Currently this must be one byte.  Should be removed later (and just
// use EDGEVER directly).

#define DEMOVERSION  (EDGEVER & 0x0FF)
