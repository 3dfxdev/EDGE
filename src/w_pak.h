//------------------------------------------------------------------------
//  3DGE PAK HANDLER - Quake1/2 PAK files
//------------------------------------------------------------------------
//
//  Copyright (C) 2009 EDGE Team
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

#ifndef __PAK_FILES_H__
#define __PAK_FILES_H__


/* PAK reading */

bool PAK_OpenRead(const char *filename);
void PAK_CloseRead(void);

int  PAK_NumEntries(void);
int  PAK_FindEntry(const char *name);
void PAK_FindMaps(std::vector<int>& entries);

int  PAK_EntryLen(int entry);
const char * PAK_EntryName(int entry);

bool PAK_ReadData(int entry, int offset, int length, void *buffer);

void PAK_ListEntries(void);


#endif /* __PAK_FILES_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
