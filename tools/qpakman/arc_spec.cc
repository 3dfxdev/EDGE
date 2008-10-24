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
#include "im_color.h"
#include "im_image.h"
#include "im_mip.h"
#include "im_png.h"
#include "q1_structs.h"


extern std::map<std::string, int> all_pak_lumps;


// -AJA- This is not used, because it can produce the wrong data
//       due to duplicate colors in the palette.
bool ARC_StorePOP(FILE *fp, const char *lump)
{
  printf("  Converting POP back to LMP...\n");

  rgb_image_c *img = PNG_Load(fp);

  if (! img)
  {
    printf("FAILURE: failed to load POP image!\n");
    return false;
  }

  const char *new_lump = ReplaceExtension(lump, "lmp");

  PAK_NewLump(new_lump);

  all_pak_lumps[new_lump] = 1;

  // re-encode the POP graphic
  COL_SetFullBright(true);
  COL_SetTransparent(0);

  for (int y = 0; y < img->height; y++)
  for (int x = 0; x < img->width;  x++)
  {
    byte pix = COL_MapColor(img->PixelAt(x, y));

    PAK_AppendData(&pix, 1);
  }

  PAK_FinishLump();

  delete img;

  return true; // OK
}

static bool ExtractPOP(int entry, const char *path)
{
  printf("  Converting POP to PNG...\n");

  const char *png_name = ReplaceExtension(path, "png");

  if (FileExists(png_name) && ! opt_force)
  {
    printf("FAILURE: will not overwrite file: %s\n\n", png_name);
    return false;
  }

  int total = PAK_EntryLen(entry);

  if (total < 16*16)
  {
    printf("FAILURE: invalid length for pop.lmp (%d < 16*16)\n", total);
    return false;
  }


  byte pop[16*16];

  PAK_ReadData(entry, 0, 16*16, pop);

  // create the image for saving.
  COL_SetFullBright(true);

  rgb_image_c *img = new rgb_image_c(16, 16);

  for (int y = 0; y < 16; y++)
  for (int x = 0; x < 16;  x++)
  {
    img->PixelAt(x, y) = COL_ReadPalette(pop[y*16+x]);
  }


  FILE *fp = fopen(png_name, "wb");

  if (! fp)
  {
    printf("FAILURE: cannot create output file: %s\n\n", png_name);
    delete img;
    return false;
  }

  bool result = PNG_Save(fp, img);

  fclose(fp);

  delete img;

  if (! result)
    printf("FAILURE: error while writing PNG file\n\n");

  return result;
}


//------------------------------------------------------------------------

static bool StorePalette(FILE *fp, const char *lump)
{
  printf("  Converting palette back to LMP...\n");

  const char *new_lump = ReplaceExtension(lump, "lmp");

  PAK_NewLump(new_lump);

  all_pak_lumps[new_lump] = 1;

  byte palette[768];

  for (int pix = 0; pix < 256; pix++)
  {
    int R, G, B;

    if (3 != fscanf(fp, " %i %i %i ", &R, &G, &B))
      FatalError("Not enough colors in palette.txt (failed at #%d)\n", pix);

    if (R < 0 || R > 255 || G < 0 || G > 255 || B < 0 || B > 255)
      FatalError("Bad color in palette.txt at #%d : (%d %d %d)\n", pix, R, G, B);

    palette[pix*3+0] = R;
    palette[pix*3+1] = G;
    palette[pix*3+2] = B;
  }

  PAK_AppendData(palette, 768);
  PAK_FinishLump();

  return true; // OK
}

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

  for (int pix = 0; pix < 256; pix++)
  {
    fprintf(fp, "%3d %3d %3d\n",
            palette[pix*3+0], palette[pix*3+1], palette[pix*3+2]);
  }

  fclose(fp);

  return true; // OK
}


//------------------------------------------------------------------------

static bool StoreFontsize(FILE *fp, const char *lump)
{
  printf("  Converting fontsize back to LMP...\n");

  const char *new_lump = ReplaceExtension(lump, "lmp");

  PAK_NewLump(new_lump);

  all_pak_lumps[new_lump] = 1;

  byte fontsize[729];

  for (int i = 0; i < 729; i++)
  {
    int value = 0;

    if (1 != fscanf(fp, " %i ", &value))
      FatalError("Not enough values in fontsize.txt (failed at #%d)\n", i);

    fontsize[i] = value;
  }

  PAK_AppendData(fontsize, 729);
  PAK_FinishLump();

  return true; // OK
}

static bool ExtractFontsize(int entry, const char *path)
{
  printf("  Converting fontsize to TXT...\n");

  const char *filename = ReplaceExtension(path, "txt");

  if (FileExists(filename) && ! opt_force)
  {
    printf("FAILURE: will not overwrite file: %s\n\n", filename);
    return false;
  }

  int total = PAK_EntryLen(entry);

  if (total < 729)
  {
    printf("FAILURE: invalid length for fontsize.lmp (%d < 729)\n", total);
    return false;
  }

  FILE *fp = fopen(filename, "wb");

  if (! fp)
  {
    printf("FAILURE: cannot create output file: %s\n\n", filename);
    return false;
  }

  byte fontsize[729];

  PAK_ReadData(entry, 0, 729, fontsize);

  for (int y = 0; y < 27; y++)
  {
    for (int x = 0; x < 27; x++)
      fprintf(fp, " %2d", fontsize[27*y+x]);

    fprintf(fp, "\n");
  }

  fclose(fp);

  return true; // OK
}


//------------------------------------------------------------------------

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



//------------------------------------------------------------------------

bool ARC_IsSpecialInput (const char *lump)
{
  if (StringCaseCmp(lump, "gfx/palette.txt") == 0)
    return true;

  if (StringCaseCmp(lump, "gfx/menu/fontsize.txt") == 0)
    return true;

  return false;
}

bool ARC_StoreSpecial(FILE *fp, const char *lump, const char *path)
{
  if (StringCaseCmp(lump, "gfx/palette.txt") == 0)
    return StorePalette(fp, lump);

  if (StringCaseCmp(lump, "gfx/menu/fontsize.txt") == 0)
    return StoreFontsize(fp, lump);

///  if (StringCaseCmp(FindBaseName(lump), "pop.png") == 0)
///    return StorePOP(fp, lump);

  return false;
}


bool ARC_IsSpecialOutput(const char *lump)
{
  if (StringCaseCmp(lump, "gfx/palette.lmp") == 0)
    return true;

  if (StringCaseCmp(lump, "gfx/menu/fontsize.lmp") == 0)
    return true;

  if (game_type == GAME_Quake2 && CheckExtension(lump, "WAL"))
    return true;


  return false;
}

bool ARC_ExtractSpecial(int entry, const char *lump, const char *path)
{
  if (StringCaseCmp(lump, "gfx/palette.lmp") == 0)
    return ExtractPalette(entry, path);

  if (StringCaseCmp(lump, "gfx/menu/fontsize.lmp") == 0)
    return ExtractFontsize(entry, path);

  if (game_type == GAME_Quake2 && CheckExtension(lump, "WAL"))
    return ExtractWAL(entry, path);


  return false;
}


//------------------------------------------------------------------------

bool ARC_TryAnalyseSpecial(int entry, const char *lump, const char *path)
{
  if (StringCaseCmp(FindBaseName(lump), "pop.lmp") == 0)
  {
    ExtractPOP(entry, path);
    return true;
  }

  return false;
}


//--- editor settings ---
// vi:ts=2:sw=2:expandtab
