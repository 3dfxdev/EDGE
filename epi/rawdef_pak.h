//----------------------------------------------------------------------------
//  PAK defines, raw on-disk layout
//----------------------------------------------------------------------------
//
//  Copyright (c) 2016  Isotope SoftWorks.
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
//  Based on "qfiles.h" from the GPL'd quake 2 source release.
//  Copyright (C) 1997-2001 Id Software, Inc.
//
//  EDIT: Now we can remove these from /src and use namespaces.
//----------------------------------------------------------------------------

#ifndef __EPI_RAWDEF_PAK__
#define __EPI_RAWDEF_PAK__

#include <vector> ///rip

/// OBLIGE HOLDOVERS:
#define ALIGN_LEN(x)  (((x) + 3) & ~3)

namespace epi
{
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


/* PAK writing */

bool PAK_OpenWrite(const char *filename);
void PAK_CloseWrite(void);

void PAK_NewLump(const char *name);
bool PAK_AppendData(const void *data, int length);
void PAK_FinishLump(void);


/* ----- PAK structures ---------------------- */

typedef struct raw_pak_header_s
{
	char magic[4];

	u32_t dir_start;
	u32_t entry_num;

}
raw_pak_header_t;

#define PAK_MAGIC  "PACK"


typedef struct raw_pak_entry_s
{
	char name[56];

	u32_t offset;
	u32_t length;

}
raw_pak_entry_t;


}  // namespace epi

#endif  /* __EPI_RAWDEF_PAK__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
