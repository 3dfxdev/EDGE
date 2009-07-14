//------------------------------------------------------------------------
//  WAD SPRITE MANAGEMENT
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

#include <functional>
#include "w_sprites.h"
#include "w_name.h"


/*
 *  Sprite_dir::loc_by_root - find sprite by prefix
 *  
 *      Return the (wad, offset, length) location of the first
 *      lump by alphabetical order whose name begins with
 *      <name>. If not found, set loc.wad to 0.
 */
void Sprite_loc_by_root (const char *name, Lump_loc& loc)
{
	char buffer[16];

	strcpy(buffer, name);

	if (strlen(buffer) == 4)
		strcat(buffer, "A");

	if (strlen(buffer) == 5)
		strcat(buffer, "0");

	MDirPtr m = FindMasterDir(MasterDir, buffer);

	if (! m)
	{
		buffer[5] = '1';
		m = FindMasterDir(MasterDir, buffer);
	}

	if (! m)
	{
		strcat(buffer, "C1");
		m = FindMasterDir(MasterDir, buffer);
	}

	if (! m)
	{
		buffer[6] = 'D';
		m = FindMasterDir(MasterDir, buffer);
	}

	if (! m)
	{
		loc.wad = NULL;
		loc.ofs = loc.len = 0;
		return;
	}

	loc.wad = m->wadfile;
	loc.ofs = m->dir.start;
	loc.len = m->dir.size;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
