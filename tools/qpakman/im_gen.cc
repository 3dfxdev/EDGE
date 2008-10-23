//------------------------------------------------------------------------
//  Generation of Special files
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

#include "pakfile.h"
#include "im_color.h"
#include "im_image.h"
#include "im_gen.h"
#include "im_mip.h"
#include "q1_structs.h"


static void GEN_InversePalette(const char *filename)
{
  // Hexen II lump
  printf("Generating inverse palette: %s\n", filename);

  FILE *fp = fopen(filename, "wb");
  if (! fp)
    FatalError("Cannot create file! %s\n", strerror(errno));

  COL_SetFullBright(true);
  COL_SetTransparent(0);  // will not occur

  char buffer[64];

  for (int r = 0; r < 64; r++)
  for (int g = 0; g < 64; g++)
  {
    for (int b = 0; b < 64; b++)
    {
      buffer[b] = COL_MapColor(MAKE_RGB(r<<2, g<<2, b<<2));
    }

    if (1 != fwrite(buffer, sizeof(buffer), 1, fp))
    {
      fclose(fp);
      FatalError("Error writing file!\n");
    }
  }

  fclose(fp);

  printf("Finished.\n");
}


static void GEN_16to8_Table(const char *filename)
{
  // Quake II lump
  printf("Generating Inverse16 table: %s\n", filename);

  FILE *fp = fopen(filename, "wb");
  if (! fp)
    FatalError("Cannot create file! %s\n", strerror(errno));

  COL_SetFullBright(true);
  COL_SetTransparent(0);  // will not occur

  char buffer[32];

  for (int b = 0; b < 32; b++)
  for (int g = 0; g < 64; g++)
  {
    for (int r = 0; r < 32; r++)
    {
      buffer[r] = COL_MapColor(MAKE_RGB(r<<3, g<<2, b<<3));
    }

    if (1 != fwrite(buffer, sizeof(buffer), 1, fp))
    {
      fclose(fp);
      FatalError("Error writing file!\n");
    }
  }

  fclose(fp);

  printf("Finished.\n");
}


//------------------------------------------------------------------------

static void GEN_TintTable(const char *filename, int x_mul, int y_mul)
{
  // Hexen II lumps
  printf("Generating tint table: %s\n", filename);

  FILE *fp = fopen(filename, "wb");
  if (! fp)
    FatalError("Cannot create file! %s\n", strerror(errno));

  COL_SetFullBright(true);
  COL_SetTransparent(0);  // will not occur

  char buffer[256];  // one row

  for (int y = 0; y < 256; y++)
  {
    for (int x = 0; x < 256; x++)
    {
      u32_t XC = COL_ReadPalette(x);
      u32_t YC = COL_ReadPalette(y);

      int r = (RGB_R(XC) * x_mul + RGB_R(YC) * y_mul) >> 8;
      int g = (RGB_G(XC) * x_mul + RGB_G(YC) * y_mul) >> 8;
      int b = (RGB_B(XC) * x_mul + RGB_B(YC) * y_mul) >> 8;

      // clamp the result
      if (r & 0xF00) r = 255;
      if (g & 0xF00) g = 255;
      if (b & 0xF00) b = 255;

      buffer[x] = COL_MapColor(MAKE_RGB(r, g, b));
    }

    if (1 != fwrite(buffer, sizeof(buffer), 1, fp))
    {
      fclose(fp);
      FatalError("Error writing file!\n");
    }
  }

  fclose(fp);

  printf("Finished.\n");
}


//------------------------------------------------------------------------

bool GEN_TryCreateSpecial(const char *filename)
{
  if (StringCaseCmp(FindBaseName(filename), "invpal.lmp") == 0)
  {
    GEN_InversePalette(filename);
    return true;
  }

  if (StringCaseCmp(FindBaseName(filename), "16to8.dat") == 0)
  {
    GEN_16to8_Table(filename);
    return true;
  }

  if (StringCaseCmp(FindBaseName(filename), "tinttab2.lmp") == 0)
  {
    GEN_TintTable(filename, 165, 91);
    return true;
  }
  else if (StringCaseCmp(FindBaseName(filename), "tinttab.lmp") == 0)
  {
    GEN_TintTable(filename, 128, 200);
    return true;
  }

  return false;
}

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
