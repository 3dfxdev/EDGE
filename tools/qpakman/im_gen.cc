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
  printf("Generating inverse palette: %s\n", filename);

  FILE *fp = fopen(filename, "wb");
  if (! fp)
    FatalError("Cannot create file! %s\n", strerror(errno));

  COL_SetFullBright(true);
  COL_SetTransparent(255);

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


//------------------------------------------------------------------------

bool GEN_TryCreateSpecial(const char *filename)
{
  if (StringCaseCmp(FindBaseName(filename), "invpal.lmp") == 0)
  {
    GEN_InversePalette(filename);
    return true;
  }

  // 16to8.dat

  // tinttab.lmp
  // tinttab2.lmp

  return false;
}

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
