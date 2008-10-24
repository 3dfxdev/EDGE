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


// -AJA- This is not used, it can produce the wrong data because of
//       duplicate colors in the palette.
#if 0
static int StorePOP(FILE *fp, const char *lump)
{
  printf("  Converting POP back to LMP...\n");

  rgb_image_c *img = PNG_Load(fp);

  if (! img)
  {
    printf("FAILURE: failed to load POP image!\n");
    return ARCSP_Failed;
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

  return ARCSP_Success;
}
#endif

static int AnalysePOP(int entry, const char *path)
{
  printf("  Analysing POP to PNG...\n");

  const char *png_name = ReplaceExtension(path, "png");

  if (FileExists(png_name) && ! opt_force)
  {
    printf("FAILURE: will not overwrite file: %s\n\n", png_name);
    return ARCSP_Failed;
  }

  int length = PAK_EntryLen(entry);
  if (length < 16*16)
  {
    printf("FAILURE: invalid length for pop.lmp (%d < 16*16)\n", length);
    return ARCSP_Failed;
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
    return ARCSP_Failed;
  }

  bool result = PNG_Save(fp, img);

  if (! result)
    printf("FAILURE: error while writing PNG file\n\n");

  fclose(fp);
  delete img;

  // POP image is merely additional to the POP lmp
  return ARCSP_Normal;
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
    {
      printf("FAILURE: Not enough colors in palette.txt (failed at #%d)\n", pix);
      PAK_FinishLump();
      return ARCSP_Failed;
    }

    palette[pix*3+0] = R;
    palette[pix*3+1] = G;
    palette[pix*3+2] = B;
  }

  PAK_AppendData(palette, 768);
  PAK_FinishLump();

  return ARCSP_Success;
}

static int ExtractPalette(int entry, const char *path)
{
  printf("  Converting palette to TXT...\n");

  const char *filename = ReplaceExtension(path, "txt");

  if (FileExists(filename) && ! opt_force)
  {
    printf("FAILURE: will not overwrite file: %s\n\n", filename);
    return ARCSP_Failed;
  }

  int length = PAK_EntryLen(entry);
  if (length < 256*3)
  {
    printf("FAILURE: invalid length for palette.lmp (%d < 256*3)\n", length);
    return ARCSP_Failed;
  }

  FILE *fp = fopen(filename, "w");
  if (! fp)
  {
    printf("FAILURE: cannot create output file: %s\n\n", filename);
    return ARCSP_Failed;
  }

  byte palette[256*3];

  PAK_ReadData(entry, 0, 256*3, palette);

  for (int pix = 0; pix < 256; pix++)
  {
    fprintf(fp, "%3d %3d %3d\n",
            palette[pix*3+0], palette[pix*3+1], palette[pix*3+2]);
  }

  fclose(fp);

  return ARCSP_Success;
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
    int size = 0;

    if (1 != fscanf(fp, " %i ", &size))
    {
      printf("FAILURE: Not enough sizes in fontsize.txt (failed at #%d)\n", i);
      PAK_FinishLump();
      return ARCSP_Failed;
    }

    fontsize[i] = size;
  }

  PAK_AppendData(fontsize, 729);
  PAK_FinishLump();

  return ARCSP_Success;
}

static bool ExtractFontsize(int entry, const char *path)
{
  printf("  Converting fontsize to TXT...\n");

  const char *filename = ReplaceExtension(path, "txt");

  if (FileExists(filename) && ! opt_force)
  {
    printf("FAILURE: will not overwrite file: %s\n\n", filename);
    return ARCSP_Failed;
  }

  int length = PAK_EntryLen(entry);
  if (length < 729)
  {
    printf("FAILURE: invalid length for fontsize.lmp (%d < 729)\n", length);
    return ARCSP_Failed;
  }

  FILE *fp = fopen(filename, "w");
  if (! fp)
  {
    printf("FAILURE: cannot create output file: %s\n\n", filename);
    return ARCSP_Failed;
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

  return ARCSP_Success;
}


//------------------------------------------------------------------------

static int StoreWAL(FILE *fp, const char *lump)
{
  // TODO
  return ARCSP_Failed;
}

static int ExtractWAL(int entry, const char *path)
{
  printf("  Converting WAL texture to PNG...\n");

  const char *png_name = ReplaceExtension(path, "png");

  if (FileExists(png_name) && ! opt_force)
  {
    printf("FAILURE: will not overwrite file: %s\n\n", png_name);
    return ARCSP_Failed;
  }

  bool result = MIP_DecodeWAL(entry, png_name);

  return result ? ARCSP_Success : ARCSP_Failed;
}


//------------------------------------------------------------------------

static int StoreLMP(FILE *fp, const char *lump)
{
  // TODO
  return ARCSP_Failed;
}

static int ExtractLMP(int entry, const char *path)
{
  printf("  Converting LMP graphic to PNG...\n");

  const char *png_name = ReplaceExtension(path, "png");

  if (FileExists(png_name) && ! opt_force)
  {
    printf("FAILURE: will not overwrite file: %s\n\n", png_name);
    return ARCSP_Failed;
  }

  int length = PAK_EntryLen(entry);

  // determine size of image
  int W = 0, H = 0;

  if (length >= 8)
  {
    pic_header_t pic;

    PAK_ReadData(entry, 0, sizeof(pic_header_t), &pic);

    W = LE_U32(pic.width);
    H = LE_U32(pic.height);
  }

  length -= 8;

  if (W <= 0 || H <= 0 || W*H > length)
  {
    printf("FAILURE: invalid size in LMP graphic (%dx%d)\n", W, H);
    return ARCSP_Failed;
  }


  COL_SetFullBright(true);
  COL_SetTransparent(255);

  rgb_image_c *img = new rgb_image_c(W, H);

  byte *pixels = new byte[W];

  for (int y = 0; y < H; y++)
  {
    PAK_ReadData(entry, 8+y*W, W, pixels);

    // TODO: optimise this with a new COL_xxxx function
    for (int x = 0; x < W; x++)
    {
      img->PixelAt(x, y) = COL_ReadPalWithTrans(pixels[x]);
    }
  }

  delete[] pixels;


  FILE *fp = fopen(png_name, "wb");
  if (! fp)
  {
    printf("FAILURE: cannot create output file: %s\n\n", png_name);
    delete img;
    return ARCSP_Failed;
  }

  bool result = PNG_Save(fp, img);

  if (! result)
    printf("FAILURE: error while writing PNG file\n\n");

  fclose(fp);
  delete img;
 

  return result ? ARCSP_Success : ARCSP_Failed;
}


//------------------------------------------------------------------------

static int StoreConchars(FILE *fp, const char *lump)
{
  // TODO
  return ARCSP_Failed;
}

static int ExtractConchars(int entry, const char *path)
{
  // TODO
  return ARCSP_Failed;
}


//------------------------------------------------------------------------

static int AnalyseQ1Colormap(int entry, const char *path)
{
  // TODO
  // unpack the LMP file as well
  return ARCSP_Normal;
}

static int AnalyseQ2Colormap(int entry, const char *path)
{
  // TODO
  return ARCSP_Normal;
}

static int AnalysePlayermap(int entry, const char *path)
{
  // TODO
  // unpack the LMP file as well
  return ARCSP_Normal;
}


//------------------------------------------------------------------------

int ARC_TryStoreSpecial(FILE *fp, const char *lump, const char *path)
{
  if (StringCaseCmp(lump, "gfx/palette.txt") == 0)
    return StorePalette(fp, lump);

  if (StringCaseCmp(lump, "gfx/menu/fontsize.txt") == 0)
    return StoreFontsize(fp, lump);

  if (StringCaseCmp(lump, "gfx/menu/conchars.png") == 0)
    return StoreConchars(fp, lump);

  if (StringCaseCmp(FindBaseName(lump), "pop.png") == 0)
    return ARCSP_Ignored;

  if (game_type != GAME_Quake2 && CheckExtension(lump, "PNG") &&
      StringCaseCmpPartial(lump, "gfx/") == 0)
  {
    return StoreLMP(fp, lump);
  }

  if (game_type == GAME_Quake2 && CheckExtension(lump, "PNG") &&
      StringCaseCmpPartial(lump, "textures/") == 0)
  {
    return StoreWAL(fp, lump);
  }

  // just a normal file
  return ARCSP_Normal;
}


int ARC_TryExtractSpecial(int entry, const char *lump, const char *path)
{
  if (StringCaseCmp(lump, "gfx/palette.lmp") == 0)
    return ExtractPalette(entry, path);

  if (StringCaseCmp(lump, "gfx/menu/fontsize.lmp") == 0)
    return ExtractFontsize(entry, path);

  if (StringCaseCmp(FindBaseName(lump), "pop.lmp") == 0)
    return AnalysePOP(entry, path);

  if (StringCaseCmp(lump, "gfx/menu/conchars.lmp") == 0)
    return ExtractConchars(entry, path);

  if (StringCaseCmp(lump, "gfx/colormap.lmp") == 0)
    return AnalyseQ1Colormap(entry, path);

  if (StringCaseCmp(lump, "pics/colormap.pcx") == 0)
    return AnalyseQ2Colormap(entry, path);

  if (StringCaseCmp(lump, "gfx/player.lmp") == 0)
    return AnalysePlayermap(entry, path);

  if (game_type != GAME_Quake2 && CheckExtension(lump, "LMP") &&
      StringCaseCmpPartial(lump, "gfx/") == 0)
  {
    return ExtractLMP(entry, path);
  }

  if (game_type == GAME_Quake2 && CheckExtension(lump, "WAL"))
    return ExtractWAL(entry, path);

  // just a normal lump
  return ARCSP_Normal;
}


//--- editor settings ---
// vi:ts=2:sw=2:expandtab
