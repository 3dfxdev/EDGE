//----------------------------------------------------------------------------
//  EDGE2 WAD Support Code
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2008  The EDGE2 Team.
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
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#ifndef __W_WAD__
#define __W_WAD__

#include "dm_defs.h"

#include "../epi/file.h"
#include "../epi/utility.h"
#include "games/wolf3d/wlf_local.h"
#include "games/wolf3d/wlf_rawdef.h"

#define Debug_Printf I_Debugf

#define DEBUG_LUMPS 0

typedef enum
{
	FLKIND_IWad = 0,  // iwad file
	FLKIND_PWad,      // normal .wad file
	FLKIND_EWad,      // EDGE2.wad
	FLKIND_GWad,      // glbsp node wad
	//FLKIND_SWad,      // startup.wad (from Eternity)
	/*
	FLKIND_WL6,      // .wl6 Wolfenstein datas (needed for mods maybe)
	FLKIND_VGADICT,   // Wolfenstein VGA Dictionary
	FLKIND_VSWAP,     // Wolfenstein VSWAP
	FLKIND_VGAGRAPH,  // Wolfenstein VGRAPH
	FLKIND_AUDIOHED,  // Wolfenstein AUDIOHED
	FKLIND_AUDIOT,    // Wolfenstein AudioT
	FLKIND_GAMEMAPS,  // Wolfenstein GAMEMAPS
	FLKIND_MAPHEAD,   // Wolfenstein MAPHEAD
	FLKIND_RTLMAPS,   // Rise of the Triad DARKWAR.rtl, similar to maphead
	*/
	FLKIND_HWad,      // deHacked wad
	FLKIND_EPK,       // EDGE EPK (zip) file
	FLKIND_PAK,       // Quake PAK
	FLKIND_PK3,       // PK3 zip file
	FLKIND_PK7,       // PK7 7zip file

	FLKIND_Lump,      // raw lump (no extension)
	FLKIND_ROQ,       // ROQ Video Cinematics

	FLKIND_DDF,       // .ddf or .ldf file
	FLKIND_Demo,      // .lmp demo file
	FLKIND_RTS,       // .rts script
	FLKIND_Deh        // .deh or .bex file
}
filekind_e;

// Moved Wolfenstein VSWAP class to global wad header, made no sense to keep it confined to wlf_vswap.
#if 0
class vswap_info_c
{
public:
	epi::file_c *fp;

	int first_wall, num_walls;
	int first_sprite, num_sprites;
	int first_sound, num_sounds;

	std::vector<raw_chunk_t> chunks;

public:
	vswap_info_c() : fp(NULL) { }
	~vswap_info_c() { }
};
#endif // 0


class wadtex_resource_c
{
public:
	wadtex_resource_c() : palette(-1), pnames(-1), texture1(-1), texture2(-1)
	{ }

	// lump numbers, or -1 if nonexistent
	int palette;
	int pnames;
	int texture1;
	int texture2;
};

typedef enum
{
	LMPLST_Sprites,
	LMPLST_Flats,
	LMPLST_Patches,
	LMPLST_LBM, // lbm 320x200
	LMPLST_LPIC, //rott_pic
	LMPLST_RAW //rottraw flats!
}
lumplist_e;

extern int numlumps;
extern int addwadnum;

void W_AddRawFilename(const char *file, int kind);
void W_InitMultipleFiles(void);
void W_ReadDDF(void);
void W_ReadCoalLumps(void);

int W_CheckNumForName2(const char *name);
int W_CheckNumForName_GFX(const char *name);
int W_GetNumForName2(const char *name);
int W_GetNumForName3(const char *name);
int W_CheckNumForName3(const char *name);
int W_GetNumForFullName2(const char *name);
int W_CheckNumForTexPatch(const char *name);
int W_FindNameFromPath(const char *name);
int W_FindLumpFromPath(const std::string &path);

int W_LumpLength(int lump);

void W_DoneWithLump(const void *ptr);
void W_DoneWithLump_Flushable(const void *ptr);
const void *W_CacheLumpNum2(int lump);
const void *W_CacheLumpName2(const char *name);
void W_PreCacheLumpNum(int lump);
void W_PreCacheLumpName(const char *name);
void *W_LoadLumpNum(int lump);
void *W_LoadLumpName(const char *name);
bool W_VerifyLumpName(int lump, const char *name);
const char *W_GetLumpName(int lump);
const char *W_GetLumpFullName(int lump);
int W_CacheInfo(int level);
byte *W_ReadLumpAlloc(int lump, int *length);

epi::file_c *W_OpenLump(int lump);
epi::file_c *W_OpenLump(const char *name);

const char *W_GetFileName(int lump);
int W_GetPaletteForLump(int lump);
int W_FindFlatSequence(const char *start, const char *end,
    int *s_offset, int *e_offset);
epi::u32array_c& W_GetListLumps(int file, lumplist_e which);
void W_GetTextureLumps(int file, wadtex_resource_c *res);
void W_GetWolfTextureLumps(int file, raw_vswap_t *res);
void W_ProcessTX_HI(void);
int W_GetNumFiles(void);
int W_GetFileForLump(int lump);
void W_ShowLumps(int for_file, const char *match);
void W_ShowFiles(void);

static void W_ReadLump(int lump, void *dest);
// Define this only in an emergency.  All these debug printfs quickly
// add up, and it takes only a few seconds to end up with a 40 meg debug file!
#ifdef WAD_CHECK
static int W_CheckNumForName3(const char *x, const char *file, int line)
{
	Debug_Printf("Find '%s' @ %s:%d\n", x, file, line);
	return W_CheckNumForName2(x);
}

static int W_GetNumForName3(const char *x, const char *file, int line)
{
	Debug_Printf("Find '%s' @ %s:%d\n", x, file, line);
	return W_GetNumForName2(x);
}

#if 0
static void *W_CacheLumpNum3(int lump, const char *file, int line)
{
	Debug_Printf("Cache '%d' @ %s:%d\n", lump, file, line);
	return W_CacheLumpNum2(lump, tag);
}

static void *W_CacheLumpName3(const char *name, const char *file, int line)
{
	Debug_Printf("Cache '%s' @ %s:%d\n", name, file, line);
	return W_CacheLumpName2(name, tag);
}
#endif // 0


#define W_CheckNumForName(x) W_CheckNumForName3(x, __FILE__, __LINE__)
#define W_GetNumForName(x) W_GetNumForName3(x, __FILE__, __LINE__)
#define W_GetNumForFullName(x) W_GetNumForFullName2(x, __FILE__, __LINE__)
#define W_CacheLumpNum(x) W_CacheLumpNum3(x, __FILE__, __LINE__)
#define W_CacheLumpName(x) W_CacheLumpName3(x, __FILE__, __LINE__)

#else
#define W_CheckNumForName(x) W_CheckNumForName2(x)
#define W_GetNumForName(x) W_GetNumForName2(x)
#define W_GetNumForFullName(x) W_GetNumForFullName2(x)
#define W_CacheLumpNum(x) W_CacheLumpNum2(x)
#define W_CacheLumpName(x) W_CacheLumpName2(x)
#endif

#endif // __W_WAD__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
