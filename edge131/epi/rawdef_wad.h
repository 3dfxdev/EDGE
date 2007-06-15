//----------------------------------------------------------------------------
//  WAD structures, raw on-disk layout
//----------------------------------------------------------------------------
//
//  Copyright (c) 2002-2005  The EDGE Team.
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

#ifndef __EPI_RAWDEF_WAD__
#define __EPI_RAWDEF_WAD__

#include "types.h"

namespace epi
{

// wad header
typedef struct raw_wad_header_s
{
	char type[4];

	u32_t num_entries;
	u32_t dir_start;
}
raw_wad_header_t;
 
// directory entry
typedef struct raw_wad_entry_s
{
	u32_t start;
	u32_t length;

	char name[8];
}
raw_wad_entry_t;

}  // namespace epi

#endif  // __EPI_RAWDEF_WAD__
