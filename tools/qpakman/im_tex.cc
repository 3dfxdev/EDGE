//------------------------------------------------------------------------
//  Texture Extraction
//------------------------------------------------------------------------
// 
//  Copyright (c) 2008  Andrew J Apted
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

#include "headers.h"
#include "main.h"

#include <map>

#include "im_color.h"
#include "im_tex.h"
#include "pakfile.h"
#include "q1_structs.h"


// WAD2_OpenWrite
// {
//   for each pakfile do
//   {
//     PAK_OpenRead
//
//     for each map do
//       for each miptex do
//         if not already seen
//           copy miptex into WAD2 file
// 
//     PAK_CloseRead
//   }
// }
// WAD2_CloseWrite


#define LogPrintf  printf

typedef std::map<std::string, int> miptex_database_t;

static miptex_database_t tex_db;


typedef bool (* read_func_F)(int offset, int length, void *buffer);


static int  pak_entry;
static bool pak_reader(int offset, int length, void *buffer)
{
  return PAK_ReadData(pak_entry, offset, length, buffer);
}

static FILE *bsp_fp;
static bool bsp_reader(int offset, int length, void *buffer)
{
  if (fseek(bsp_fp, offset, SEEK_SET) != 0)
      return false;

  int res = fread(buffer, length, 1, bsp_fp);

  return (res == 1);
}


static void ExtractMipTex(read_func_F read_func)
{
  dheader_t bsp;

  if (! read_func(0, sizeof(bsp), &bsp))
    FatalError("dheader_t");

  bsp.version = LE_S32(bsp.version);

  u32_t tex_start = LE_U32(bsp.lumps[LUMP_TEXTURES].start);
  u32_t tex_total = LE_U32(bsp.lumps[LUMP_TEXTURES].length);

//  DebugPrintf("BSP: version:0x%08x tex:0x%08x len:0x%08x\n",
//              bsp.version, tex_start, tex_total);

  // check version in header (for sanity)
  if (bsp.version < 0x17 || bsp.version > 0x1F)
    FatalError("bad version in BSP");

  // no textures?
  if (tex_total == 0)
    return;

  dmiptexlump_t header;

  if (! read_func(tex_start, sizeof(header), &header))
    FatalError("dmiptexlump_t");

  int num_miptex = LE_S32(header.num_miptex);

  for (int i = 0; i < num_miptex; i++)
  {
    u32_t data_ofs;

    if (! read_func(tex_start + 4 + i*4, 4, &data_ofs))
      FatalError("data_ofs");

    data_ofs = LE_U32(data_ofs);

    // -1 means unused slot
    if (data_ofs & 0x8000000U)
      continue;

    miptex_t mip;

    if (! read_func(tex_start + data_ofs, sizeof(miptex_t), &mip))
      FatalError("miptex_t");

    mip.width  = LE_U32(mip.width);
    mip.height = LE_U32(mip.height);

    mip.name[15] = 0;

//  DebugPrintf("    mip %2d/%d name:%s size:%dx%d\n",
//              i+1, num_miptex, mip.name, mip.width, mip.height);

    // already seen it?
    if (tex_db.find((const char *)mip.name) != tex_db.end())
      continue;

    printf("  Copying %d/%d : %s\n", i+1, num_miptex, mip.name);

    // sanity check
    SYS_ASSERT(mip.width  <= 2048);
    SYS_ASSERT(mip.height <= 2048);

    // Quake ignores the offsets, hence we do too!

    int pixels = (mip.width * mip.height) / 64 * 85;

    data_ofs += sizeof(mip);

    // create WAD2 lump and mark database as seen
    tex_db[(const char *)mip.name] = 1;

    WAD2_NewLump(mip.name, TYP_MIPTEX);

    mip.width  = LE_U32(mip.width);
    mip.height = LE_U32(mip.height);

    WAD2_AppendData(&mip, (int)sizeof(mip));

    // copy the pixel data
    static byte buffer[1024];

    while (pixels > 0)
    {
      int count = MIN(pixels, 1024);

      if (! read_func(tex_start + data_ofs, count, buffer))
        FatalError("pixels");

      WAD2_AppendData(buffer, count);

      data_ofs += (u32_t)count;
      pixels   -= count;
    }

    WAD2_FinishLump();
  }
}


void TEX_ExtractStart()
{
  tex_db.clear();
}

void TEX_ExtractDone()
{
  // nothing needed
}


void TEX_ExtractFromPAK(const char *filename)
{
  if (! PAK_OpenRead(filename))
  {
    FatalError("Could not open pak file: %s", filename);
  }

  std::vector<int> maps;

  PAK_FindMaps(maps);

  for (unsigned int m = 0; m < maps.size(); m++)
  {
    pak_entry = maps[m];

    printf("\n");
    printf("Processing map %d/%d : %s\n", m+1, (int)maps.size(),
           PAK_EntryName(pak_entry));

    ExtractMipTex(pak_reader);
  }

  PAK_CloseRead();

  printf("\n");
}


void TEX_ExtractFromBSP(const char *filename)
{
  bsp_fp = fopen(filename, "rb");

  if (! bsp_fp)
    FatalError("Could not open file: %s", filename);

  printf("Opened BSP file: %s\n", filename);

  ExtractMipTex(bsp_reader);

  fclose(bsp_fp);

  printf("Closed BSP file.\n");
  printf("\n");
}


void TEX_ExtractFromWAD(const char *filename)
{
  if (! WAD2_OpenRead(filename))
    FatalError("Could not open wad file: %s", filename);

  int num_entires = WAD2_NumEntries();

  for (int i = 0; i < num_entires; i++)
  {
    const char *name = WAD2_EntryName(i);

    // skip PICs
    if (WAD2_EntryType(i) != TYP_MIPTEX)
      continue;

    // already seen it?
    if (tex_db.find((const char *)name) != tex_db.end())
      continue;
    
    printf("  Copying %d/%d : %s\n", i+1, num_entires, name);

    WAD2_NewLump(name, TYP_MIPTEX);

    // copy the data
    int offset = 0;
    int total  = WAD2_EntryLen(i);

    while (offset < total)
    {
      byte buffer[1000];

      int want_len = MIN((int)sizeof(buffer), total - offset);
    
      if (! WAD2_ReadData(i, offset, want_len, buffer))
        FatalError("wad copy");

      WAD2_AppendData(buffer, want_len);

      offset += want_len;
    }

    WAD2_FinishLump();

    tex_db[name] = 1;
  }

  WAD2_CloseRead();

  printf("\n");
}

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
