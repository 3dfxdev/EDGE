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


static void ExtractMipTex(miptex_database_t& tex_db, int map_idx)
{
  dheader_t bsp;

  if (! PAK_ReadData(map_idx, 0, sizeof(bsp), &bsp))
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

  if (! PAK_ReadData(map_idx, tex_start, sizeof(header), &header))
    FatalError("dmiptexlump_t");

  int num_miptex = LE_S32(header.num_miptex);
  
  for (int i = 0; i < num_miptex; i++)
  {
//fprintf(stderr, "  mip %d/%d\n", i+1, num_miptex);
    u32_t data_ofs;

    if (! PAK_ReadData(map_idx, tex_start + 4 + i*4, 4, &data_ofs))
      FatalError("data_ofs");

    data_ofs = LE_U32(data_ofs);
//fprintf(stderr, "  data_ofs=%d\n", data_ofs);

    // -1 means unused slot
    if (data_ofs & 0x8000000U)
      continue;

    miptex_t mip;

    if (! PAK_ReadData(map_idx, tex_start + data_ofs, sizeof(miptex_t), &mip))
      FatalError("miptex_t");

    mip.width  = LE_U32(mip.width);
    mip.height = LE_U32(mip.height);

    mip.name[15] = 0;

//  DebugPrintf("    mip %2d/%d name:%s size:%dx%d\n",
//              i+1, num_miptex, mip.name, mip.width, mip.height);

    // already seen it?
    if (tex_db.find((const char *)mip.name) != tex_db.end())
      continue;

    // sanity check
    SYS_ASSERT(mip.width  <= 2048);
    SYS_ASSERT(mip.height <= 2048);

    // Quake ignores the offsets, hence we do too!

    int pixels = (mip.width * mip.height) / 64 * 85;

    data_ofs += sizeof(mip);

    // create WAD2 lump and mark database as seen
    tex_db[(const char *)mip.name] = 1;

    WAD2_NewLump(mip.name);

    mip.width  = LE_U32(mip.width);
    mip.height = LE_U32(mip.height);

    WAD2_AppendData(&mip, (int)sizeof(mip));

    // copy the pixel data
    static byte buffer[1024];

    while (pixels > 0)
    {
      int count = MIN(pixels, 1024);

      if (! PAK_ReadData(map_idx, tex_start + data_ofs, count, buffer))
        FatalError("pixels");

      WAD2_AppendData(buffer, count);

      data_ofs += (u32_t)count;
      pixels   -= count;
    }

    WAD2_FinishLump();
  }
}


void TEX_ExtractTextures(const char *dest_file, std::vector<std::string>& src_files)
{
  SYS_ASSERT(src_files.size() > 0);

  if (! WAD2_OpenWrite(dest_file))
  {
    FatalError("Could not create file: %s", dest_file);
  }

  miptex_database_t tex_db;

  for (unsigned int pp = 0; pp < src_files.size(); pp++)
  {
    const char *filename = src_files[pp].c_str();

    LogPrintf("Opening: %s\n", filename);

    if (! PAK_OpenRead(filename))
    {
      // should not happen because we have checked that the files exist
      FatalError("Could not open file: %s", filename);
    }

    std::vector<int> maps;

    PAK_FindMaps(maps);

    for (unsigned int m = 0; m < maps.size(); m++)
    {
//    DebugPrintf("Doing map %d/%d\n", m+1, (int)maps.size());

      ExtractMipTex(tex_db, maps[m]);
    }

    PAK_CloseRead();
  }

  WAD2_CloseWrite();
}

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
