//----------------------------------------------------------------------------
//  EDGE WAD Support Code
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
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#ifndef __W_WAD__
#define __W_WAD__

#include "dm_defs.h"
#include "z_zone.h"

typedef struct wadtex_resource_s
{
  // lump numbers, or -1 if nonexistent
  int palette;
  int pnames;
  int texture1;
  int texture2;
}
wadtex_resource_t;

typedef enum
{
  LMPLST_Sprites,
  LMPLST_Flats,
}
lumplist_e;

extern int numlumps;
extern int addwadnum;

void W_AddRawFilename(const char *file, boolean_t allow_ddf);
boolean_t W_InitMultipleFiles(void);

int W_CheckNumForName2(const char *name);
int W_GetNumForName2(const char *name);

int W_LumpLength(int lump);
void W_ReadLump(int lump, void *dest);
boolean_t W_LumpRawInfo(int lump, int *handle, int *pos, int *size);

void W_DoneWithLump(const void *ptr);
void W_DoneWithLump_Flushable(const void *ptr);
const void *W_CacheLumpNum2(int lump);
const void *W_CacheLumpName2(const char *name);
void W_PreCacheLumpNum(int lump);
void W_PreCacheLumpName(const char *name);
void *W_LoadLumpNum(int lump);
void *W_LoadLumpName(const char *name);
boolean_t W_VerifyLumpName(int lump, const char *name);
const char *W_GetLumpName(int lump);
int W_CacheInfo(int level);

const char *W_GetFileName(int lump);
int W_GetPaletteForLump(int lump);
void W_AddDynamicGWA(const char *filename, int map_lump);
int W_FindFlatSequence(const char *start, const char *end, 
    int *s_offset, int *e_offset);
const int *W_GetListLumps(int file, lumplist_e which, int *count);
void W_GetTextureLumps(int file, wadtex_resource_t *res);
int W_GetNumFiles(void);

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

#define W_CheckNumForName(x) W_CheckNumForName3(x, __FILE__, __LINE__)
#define W_GetNumForName(x) W_GetNumForName3(x, __FILE__, __LINE__)
#define W_CacheLumpNum(x) W_CacheLumpNum3(x, __FILE__, __LINE__)
#define W_CacheLumpName(x) W_CacheLumpName3(x, __FILE__, __LINE__)

#else
#define W_CheckNumForName(x) W_CheckNumForName2(x)
#define W_GetNumForName(x) W_GetNumForName2(x)
#define W_CacheLumpNum(x) W_CacheLumpNum2(x)
#define W_CacheLumpName(x) W_CacheLumpName2(x)
#endif
#endif
