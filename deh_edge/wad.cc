//----------------------------------------------------------------------------
//  EDGE Tool WAD I/O
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
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

#include "i_defs.h"
#include "wad.h"

#include "system.h"

#define PWAD_HEADER  "PWAD"

#define MAX_LUMPS  2000

//
// --- TYPES ---
//

//
// Wad Info
//
typedef struct
{
	char id[4];        // IWAD (whole) or PWAD (part)
	int numlumps;      // Number of Lumps
	int infotableofs;  // Info table offset
}
wadinfo_t;

//
// Lump stuff
//
typedef struct lump_s
{
	byte *data;          // Data
	int filepos;         // Position in file
	int size;            // Size
	char name[8];        // Name
}
lump_t;

// Lump list
static lump_t **lumplist = NULL;
static int num_lumps;

static lump_t *cur_lump = NULL;
static int cur_max_size;

static char wad_msg_buf[1024];

//
// PadFile
//
// Pads a file to the nearest 4 bytes.
//
static void PadFile(FILE *fp)
{
	unsigned char zeros[4] = { 0, 0, 0, 0 };
	
	int num = ftell(fp) % 4;

	if (num != 0)
	{
		fwrite(&zeros, 1, 4 - num, fp);
	}
}

//
// WAD_LumpExists
//
int WAD_LumpExists(const char *name)
{
	int i;

	for (i = 0; i < num_lumps; i++)
	{
		if (strncmp(lumplist[i]->name, name, 8) == 0)
			return i;
	}

	return -1;
}

//
// WAD_NewLump
//
void WAD_NewLump(const char *name)
{
	if (cur_lump)
		InternalError("WAD_NewLump: current lump not finished");

	// Check for existing lump, overwrite if need be.
	int i = WAD_LumpExists(name);

	if (i >= 0)
	{
		free(lumplist[i]->data);
	}
	else
	{
		i = num_lumps;

		num_lumps++;

		if (num_lumps > MAX_LUMPS)
			FatalError("Too many lumps ! (%d)", MAX_LUMPS);

		lumplist[i] = (lump_t *) calloc(sizeof(lump_t), 1);

		if (! lumplist[i])
			FatalError("Out of memory (adding %dth lump)", num_lumps);
	}

	cur_lump = lumplist[i];
	cur_max_size = 4;

	cur_lump->size = 0;
	cur_lump->data = (byte *) malloc(cur_max_size);

	if (! cur_lump->data)
		FatalError("Out of memory (New lump data)");

	strncpy(cur_lump->name, name, 8);
}

//
// WAD_AddData
//
void WAD_AddData(const byte *data, int size)
{
	if (! cur_lump)
		InternalError("WAD_AddData: no current lump");
	
	if (cur_lump->size + size > cur_max_size)
	{
		while (cur_lump->size + size > cur_max_size)
			cur_max_size *= 2;

		cur_lump->data = (byte *) realloc(cur_lump->data, cur_max_size);

		if (! cur_lump->data)
			FatalError("Out of memory (adding %d bytes)", size);
	}

	memcpy(cur_lump->data + cur_lump->size, data, size);

	cur_lump->size += size;
}

//
// WAD_Printf
//
void WAD_Printf(const char *str, ...)
{
	va_list args;

	va_start(args, str);
	vsprintf(wad_msg_buf, str, args);
	va_end(args);

	WAD_AddData((byte *) wad_msg_buf, strlen(wad_msg_buf));
}

//
// WAD_FinishLump
//
void WAD_FinishLump(void)
{
	if (! cur_lump)
		InternalError("WAD_FinishLump: not started");

	cur_lump = NULL;
	cur_max_size = 0;
}

//
// WAD_WriteFile
//
void WAD_WriteFile(const char *name)
{
	if (cur_lump)
		InternalError("WAD_WriteFile: lump not finished");

	FILE *fp;
	const char *pwadstr = PWAD_HEADER;

	// Open File
	fp = fopen(name, "wb");

	if (!fp)
		FatalError("Cannot create output file: %s", name);

	// Write Header
	int raw_num = Endian_U32(num_lumps);

	fwrite((void*)pwadstr, 1, 4, fp);
	fwrite((void*)&raw_num, sizeof(int), 1, fp);
	fwrite((void*)&raw_num, sizeof(int), 1, fp); // dummy - write later

	// Write Lumps
	int i;

	for (i = 0; i < num_lumps; i++)
	{
		PadFile(fp);
		lumplist[i]->filepos = ftell(fp);
		fwrite((void*)lumplist[i]->data, 1, lumplist[i]->size, fp);
	}

	PadFile(fp);
	int raw_offset = Endian_U32(ftell(fp));

	// Write Lumpinfos (directory table)
	for (i = 0; i < num_lumps; i++)
	{
		int raw_pos  = Endian_U32(lumplist[i]->filepos);
		int raw_size = Endian_U32(lumplist[i]->size);

		fwrite((void*)&raw_pos, sizeof(int), 1, fp);
		fwrite((void*)&raw_size, sizeof(int), 1, fp);
		fwrite((void*)lumplist[i]->name, 1, 8, fp);
	}

	// Write table offset
	fseek(fp, 8, SEEK_SET);
	fwrite((void*)&raw_offset, sizeof(int), 1, fp);

	// Close File
	fclose(fp);
}

//
// WAD_Startup
//
void WAD_Startup(void)
{
	lumplist = (lump_t **) calloc(sizeof(lump_t *), MAX_LUMPS);

	if (! lumplist)
		FatalError("Out of memory (WAD_Startup)");
	
	num_lumps = 0;
}

//
// WAD_Shutdown
//
void WAD_Shutdown(void)
{
	int i;

	// go through lump list and free lumps
	for (i = 0; i < num_lumps; i++)
	{
		free(lumplist[i]->data);
		free(lumplist[i]);
	}

	num_lumps = 0;

	if (lumplist)
	{
		free(lumplist);
		lumplist = 0;
	}
}
