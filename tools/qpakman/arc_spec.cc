//------------------------------------------------------------------------
//  Special Archiving
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
#include <algorithm>

#include "archive.h"
#include "arc_spec.h"
#include "pakfile.h"
#include "im_mip.h"
#include "q1_structs.h"


bool ARC_IsSpecialInput (const char *lump)
{
  // TODO ARC_IsSpecialInput
  return false;
}

bool ARC_StoreSpecial(FILE *fp, const char *lump, const char *path)
{
  // TODO ARC_StoreSpecial
  return false;
}


//------------------------------------------------------------------------

static bool ExtractPalette(int entry, const char *path)
{
  printf("  Converting palette to TXT...\n");

  const char *filename = ReplaceExtension(path, "txt");

  if (FileExists(filename) && ! opt_force)
  {
    printf("FAILURE: will not overwrite file: %s\n\n", filename);
    return false;
  }

  int total = PAK_EntryLen(entry);

  if (total < 768)
  {
    printf("FAILURE: invalid length for palette.lmp (%d < 768)\n", total);
    return false;
  }

  FILE *fp = fopen(filename, "wb");

  if (! fp)
  {
    printf("FAILURE: cannot create output file: %s\n\n", filename);
    return false;
  }

  byte palette[768];

  PAK_ReadData(entry, 0, 768, palette);

  for (int i = 0; i < 256; i++)
  {
    fprintf(fp, "%3d %3d %3d\n",
            palette[i*3+0], palette[i*3+1], palette[i*3+2]);
  }

  fclose(fp);

  return true; // OK
}

static bool ExtractFontSize(int entry, const char *path)
{
  // TODO
}

static bool ExtractWAL(int entry, const char *path)
{
  printf("  Converting WAL texture to PNG...\n");

  const char *png_name = ReplaceExtension(path, "png");

  if (FileExists(png_name) && ! opt_force)
  {
    printf("FAILURE: will not overwrite file: %s\n\n", png_name);
    return false;
  }

  return MIP_DecodeWAL(entry, png_name);
}


bool ARC_IsSpecialOutput(const char *lump)
{
  if (StringCaseCmp(lump, "gfx/palette.lmp") == 0)
    return true;

  if (game_type == GAME_Quake2 && CheckExtension(lump, "WAL"))
    return true;

  // TODO ARC_IsSpecialOutput
  return false;
}


bool ARC_ExtractSpecial(int entry, const char *lump, const char *path)
{
  if (StringCaseCmp(lump, "gfx/palette.lmp") == 0)
    return ExtractPalette(entry, path);

  if (StringCaseCmp(lump, "gfx/menu/fontsize.lmp") == 0)
    return ExtractFontSize(entry, path);

  if (game_type == GAME_Quake2 && CheckExtension(lump, "WAL"))
    return ExtractWAL(entry, path);


  // TODO ARC_ExtractSpecial
  return false;
}


//------------------------------------------------------------------------

bool ARC_IsAnalyseOutput(const char *lump)
{
  // TODO ARC_IsAnalyseOutput
  return false;
}

bool ARC_AnalyseSpecial(int entry, const char *lump, const char *path)
{
  // TODO ARC_AnalyseSpecial
  return false;
}


//--- editor settings ---
// vi:ts=2:sw=2:expandtab
