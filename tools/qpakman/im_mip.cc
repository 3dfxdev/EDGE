//------------------------------------------------------------------------
//  Miptex creation
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

#include <map>

#include "im_image.h"
#include "im_mip.h"
#include "im_png.h"
#include "pakfile.h"
#include "q1_structs.h"

extern std::vector<std::string> input_names;

extern bool opt_overwrite;


std::map<std::string, int> all_lump_names;

std::map<u32_t, byte> color_cache;

int max_color_cache = 1024;

static bool cache_allow_fullbright = false;

static byte transparent_color = 0;


const byte quake1_palette[256*3] =
{
    0,  0,  0,  15, 15, 15,  31, 31, 31,  47, 47, 47,  63, 63, 63,
   75, 75, 75,  91, 91, 91, 107,107,107, 123,123,123, 139,139,139,
  155,155,155, 171,171,171, 187,187,187, 203,203,203, 219,219,219,
  235,235,235,  15, 11,  7,  23, 15, 11,  31, 23, 11,  39, 27, 15,
   47, 35, 19,  55, 43, 23,  63, 47, 23,  75, 55, 27,  83, 59, 27,
   91, 67, 31,  99, 75, 31, 107, 83, 31, 115, 87, 31, 123, 95, 35,
  131,103, 35, 143,111, 35,  11, 11, 15,  19, 19, 27,  27, 27, 39,
   39, 39, 51,  47, 47, 63,  55, 55, 75,  63, 63, 87,  71, 71,103,
   79, 79,115,  91, 91,127,  99, 99,139, 107,107,151, 115,115,163,
  123,123,175, 131,131,187, 139,139,203,   0,  0,  0,   7,  7,  0,
   11, 11,  0,  19, 19,  0,  27, 27,  0,  35, 35,  0,  43, 43,  7,
   47, 47,  7,  55, 55,  7,  63, 63,  7,  71, 71,  7,  75, 75, 11,
   83, 83, 11,  91, 91, 11,  99, 99, 11, 107,107, 15,   7,  0,  0,
   15,  0,  0,  23,  0,  0,  31,  0,  0,  39,  0,  0,  47,  0,  0,
   55,  0,  0,  63,  0,  0,  71,  0,  0,  79,  0,  0,  87,  0,  0,
   95,  0,  0, 103,  0,  0, 111,  0,  0, 119,  0,  0, 127,  0,  0,
   19, 19,  0,  27, 27,  0,  35, 35,  0,  47, 43,  0,  55, 47,  0,
   67, 55,  0,  75, 59,  7,  87, 67,  7,  95, 71,  7, 107, 75, 11,
  119, 83, 15, 131, 87, 19, 139, 91, 19, 151, 95, 27, 163, 99, 31,
  175,103, 35,  35, 19,  7,  47, 23, 11,  59, 31, 15,  75, 35, 19,
   87, 43, 23,  99, 47, 31, 115, 55, 35, 127, 59, 43, 143, 67, 51,
  159, 79, 51, 175, 99, 47, 191,119, 47, 207,143, 43, 223,171, 39,
  239,203, 31, 255,243, 27,  11,  7,  0,  27, 19,  0,  43, 35, 15,
   55, 43, 19,  71, 51, 27,  83, 55, 35,  99, 63, 43, 111, 71, 51,
  127, 83, 63, 139, 95, 71, 155,107, 83, 167,123, 95, 183,135,107,
  195,147,123, 211,163,139, 227,179,151, 171,139,163, 159,127,151,
  147,115,135, 139,103,123, 127, 91,111, 119, 83, 99, 107, 75, 87,
   95, 63, 75,  87, 55, 67,  75, 47, 55,  67, 39, 47,  55, 31, 35,
   43, 23, 27,  35, 19, 19,  23, 11, 11,  15,  7,  7, 187,115,159,
  175,107,143, 163, 95,131, 151, 87,119, 139, 79,107, 127, 75, 95,
  115, 67, 83, 107, 59, 75,  95, 51, 63,  83, 43, 55,  71, 35, 43,
   59, 31, 35,  47, 23, 27,  35, 19, 19,  23, 11, 11,  15,  7,  7,
  219,195,187, 203,179,167, 191,163,155, 175,151,139, 163,135,123,
  151,123,111, 135,111, 95, 123, 99, 83, 107, 87, 71,  95, 75, 59,
   83, 63, 51,  67, 51, 39,  55, 43, 31,  39, 31, 23,  27, 19, 15,
   15, 11,  7, 111,131,123, 103,123,111,  95,115,103,  87,107, 95,
   79, 99, 87,  71, 91, 79,  63, 83, 71,  55, 75, 63,  47, 67, 55,
   43, 59, 47,  35, 51, 39,  31, 43, 31,  23, 35, 23,  15, 27, 19,
   11, 19, 11,   7, 11,  7, 255,243, 27, 239,223, 23, 219,203, 19,
  203,183, 15, 187,167, 15, 171,151, 11, 155,131,  7, 139,115,  7,
  123, 99,  7, 107, 83,  0,  91, 71,  0,  75, 55,  0,  59, 43,  0,
   43, 31,  0,  27, 15,  0,  11,  7,  0,   0,  0,255,  11, 11,239,
   19, 19,223,  27, 27,207,  35, 35,191,  43, 43,175,  47, 47,159,
   47, 47,143,  47, 47,127,  47, 47,111,  47, 47, 95,  43, 43, 79,
   35, 35, 63,  27, 27, 47,  19, 19, 31,  11, 11, 15,  43,  0,  0,
   59,  0,  0,  75,  7,  0,  95,  7,  0, 111, 15,  0, 127, 23,  7,
  147, 31,  7, 163, 39, 11, 183, 51, 15, 195, 75, 27, 207, 99, 43,
  219,127, 59, 227,151, 79, 231,171, 95, 239,191,119, 247,211,139,
  167,123, 59, 183,155, 55, 199,195, 55, 231,227, 87, 127,191,255,
  171,231,255, 215,255,255, 103,  0,  0, 139,  0,  0, 179,  0,  0,
  215,  0,  0, 255,  0,  0, 255,243,147, 255,247,199, 255,255,255,
  159, 91, 83
};


byte MIP_FindColor(const byte *palette, u32_t rgb_col)
{
  int best_idx  = -1;
  int best_dist = (1<<30);

  // Note: we skip index #0 (black), which is used for transparency
  //       in skies.  Black is duplicated at index #48 though.

  for (int i = (cache_allow_fullbright ? 255 : 255-32); i > 0; i--)
  {
    int dr = RGB_R(rgb_col) - palette[i*3+0];
    int dg = RGB_G(rgb_col) - palette[i*3+1];
    int db = RGB_B(rgb_col) - palette[i*3+2];

    int dist = dr*dr + dg*dg + db*db;

    // exact match?
    if (dist == 0)
      return i;

    if (dist < best_dist)
    {
      best_idx  = i;
      best_dist = dist;
    }
  }

  SYS_ASSERT(best_idx >= 0);

  return best_idx;
}


inline byte MIP_MapColor(u32_t rgb_col)
{
  if (RGB_A(rgb_col) <= 128)
    return transparent_color;

  // ignore alpha from now on
  rgb_col &= 0xFFFFFF;

  if (color_cache.find(rgb_col) != color_cache.end())
    return color_cache[rgb_col];

  // don't let the color cache grow without limit
  if ((int)color_cache.size() >= max_color_cache)
  {
    color_cache.clear();
  }

  byte pal_idx = MIP_FindColor(quake1_palette, rgb_col);

  color_cache[rgb_col] = pal_idx;

  return pal_idx;
}


rgb_image_c *MIP_LoadImage(const char *filename)
{
  // Note: extension checks are case-insensitive

  if (CheckExtension(filename, "bmp") || CheckExtension(filename, "tga") ||
      CheckExtension(filename, "gif") || CheckExtension(filename, "pcx") ||
      CheckExtension(filename, "pgm") || CheckExtension(filename, "ppm") ||
      CheckExtension(filename, "tif") || CheckExtension(filename, "tiff"))
  {
    printf("FAILURE: Unsupported image format\n");
    return NULL;
  }

  if (CheckExtension(filename, "jpg") ||
      CheckExtension(filename, "jpeg"))
  {
    // TODO: JPEG
    printf("FAILURE: JPEG image format not supported yet\n");
    return NULL;
  }

  if (! CheckExtension(filename, "png"))
  {
    printf("FAILURE: Not an image file\n");
    return NULL;
  }


  FILE *fp = fopen(filename, "rb");

  if (! fp)
  {
    printf("FAILURE: Cannot open image file: %s\n", filename);
    return NULL;
  }

  rgb_image_c * img = PNG_Load(fp);

  fclose(fp);

  return img;
}


static bool ReplacePrefix(char *name, const char *prefix, char ch)
{
  if (strncmp(name, prefix, strlen(prefix)) != 0)
    return false;

  *name++ = ch;

  int move_chars = strlen(prefix) - 1;

  if (move_chars > 1)
  {
    int new_len = strlen(name) - move_chars;

    // +1 to copy the trailing NUL as well
    memmove(name, name + move_chars, new_len + 1);
  }

  return true;
}

static void ApplyAbbreviations(char *name, bool *fullbright)
{
  // make it lower case
  for (char *u = name; *u; u++)
    *u = tolower(*u);

  int len = strlen(name);

  if (len >= 6)
  {
       ReplacePrefix(name, "star_", '*')
    || ReplacePrefix(name, "plus_", '+')
    || ReplacePrefix(name, "minu_", '-')
    || ReplacePrefix(name, "divd_", '/');
  }

  len = strlen(name);

  if (len >= 5 && memcmp(name+len-4, "_fbr", 4) == 0)
  {
    *fullbright = true;

    name[len-4] = 0;
  }
}


std::string MIP_FileToLumpName(const char *filename, bool * fullbright)
{
  char *base = ReplaceExtension(FindBaseName(filename), NULL);

  if (strlen(base) == 0)
    FatalError("Weird image filename: %s\n", filename);

  ApplyAbbreviations(base, fullbright);

  if (strlen(base) > 15)
  {
    printf("WARNING: Lump name too long, will abbreviate it\n");

    // create new name using first and last 7 characters
    char new_name[20];

    memset(new_name, 0, sizeof(new_name));
    memcpy(new_name, base, 7);
    memcpy(new_name+7, base + strlen(base) - 8, 8);

    StringFree(base);

    base = StringDup(new_name);
  }

  printf("   lump name: %s\n", base);

  // check if already exists
  if (all_lump_names.find(base) != all_lump_names.end())
  {
    printf("FAILURE: Lump already exists, will not duplicate\n");
    return std::string();
  }

  all_lump_names[base] = 1;

  std::string result(base);
  StringFree(base);

  return result;
}


void MIP_ConvertImage(rgb_image_c *img)
{
  byte *line_buf = new byte[img->width];

  for (int y = 0; y < img->height; y++)  
  {
    const u32_t *src   = & img->PixelAt(0, y);
    const u32_t *src_e = src + img->width;

    byte *dest = line_buf;

    for (; src < src_e; src++)
    {
      *dest++ = MIP_MapColor(*src);
    }

    WAD2_AppendData(line_buf, img->width);
  }

  delete[] line_buf;
}


bool MIP_ProcessImage(const char *filename)
{
  bool fullbright = false;

  std::string lump_name = MIP_FileToLumpName(filename, &fullbright);

  if (lump_name.empty())
    return false;

  rgb_image_c * img = MIP_LoadImage(filename);

  if (! img)
    return false;

  if ((img->width & 7) != 0 || (img->height & 7) != 0)
  {
    printf("WARNING: Image size not multiple of 8, will scale up\n");

    int new_w = (img->width  + 7) & ~7;
    int new_h = (img->height + 7) & ~7;

    printf("   new size: %dx%d\n", new_w, new_h);

    rgb_image_c *tmp = img->ScaledDup(new_w, new_h);

    delete img; img = tmp;
  }

  if (StringCaseCmpPartial(lump_name.c_str(), "sky") == 0)
  {
    img->QuakeSkyFix();
  }


  WAD2_NewLump(lump_name.c_str(), TYP_MIPTEX);


  // mip header
  miptex_t mm_tex;

  int offset = sizeof(mm_tex);
  
  memset(mm_tex.name, 0, sizeof(mm_tex.name));
  strcpy(mm_tex.name, lump_name.c_str());
 
  mm_tex.width  = LE_U32(img->width);
  mm_tex.height = LE_U32(img->height);

  for (int i = 0; i < MIP_LEVELS; i++)
  {
    mm_tex.offsets[i] = LE_U32(offset);

    int w = img->width  / (1 << i);
    int h = img->height / (1 << i);

    offset += w * h;
  }

  WAD2_AppendData(&mm_tex, sizeof(mm_tex));


  if (fullbright != cache_allow_fullbright)
  {
    color_cache.clear();
    cache_allow_fullbright = fullbright;
  }


  // now the actual textures
  MIP_ConvertImage(img);

  for (int mip = 1; mip < MIP_LEVELS; mip++)
  {
    rgb_image_c *tmp = img->NiceMip();

    delete img; img = tmp;

    MIP_ConvertImage(img);
  }

  WAD2_FinishLump();

  delete img;
  return true;
}


void MIP_CreateWAD(const char *filename)
{
  if (input_names.size() == 0)
    FatalError("No input images were specified!\n");

  // now make the output WAD2 file!
  if (! WAD2_OpenWrite(filename))
    FatalError("Cannot create WAD2 file: %s\n", filename);

  printf("\n");
  printf("--------------------------------------------------\n");

  int failures = 0;

  for (unsigned int j = 0; j < input_names.size(); j++)
  {
    printf("Processing %d/%d: %s\n", 1+(int)j, (int)input_names.size(),
           input_names[j].c_str());

    if (! MIP_ProcessImage(input_names[j].c_str()))
      failures++;

    printf("\n");
  }

  printf("--------------------------------------------------\n");

  WAD2_CloseWrite();

  printf("Mipped %d images, with %d failures\n",
         (int)input_names.size() - failures, failures);

}


//------------------------------------------------------------------------

static const char * ExpandFileName(const char *lump_name, bool fullbright)
{
  int max_len = strlen(lump_name) + 32;

  char *result = StringNew(max_len);

  // convert any special first character
  if (lump_name[0] == '*')
  {
    strcpy(result, "star_");
  }
  else if (lump_name[0] == '+')
  {
    strcpy(result, "plus_");
  }
  else if (lump_name[0] == '-')
  {
    strcpy(result, "minu_");
  }
  else if (lump_name[0] == '/')
  {
    strcpy(result, "divd_");
  }
  else
  {
    result[0] = lump_name[0];
    result[1] = 0;
  }

  strcat(result, lump_name + 1);

  // sanitize filename (remove problematic characters)
  bool warned = false;

  for (char *p = result; *p; p++)
  {
    if (*p == ' ')
      *p = '_';

    if (*p != '_' && *p != '-' && ! isalnum(*p))
    {
      if (! warned)
      {
        printf("WARNING: removing weird characters from name (\\%03o)\n",
               (unsigned char)*p);
        warned = true;
      }

      *p = '_';
    }
  }

  if (fullbright)
    strcat(result, "_fbr");

  strcat(result, ".png");

  return result;
}


bool MIP_ExtractMipTex(int entry, const char *lump_name)
{
  // mip header
  miptex_t mm_tex;

  if (! WAD2_ReadData(entry, 0, (int)sizeof(mm_tex), &mm_tex))
  {
    printf("FAILURE: could not read miptex header!\n\n");
    return false;
  }

  // (We ignore the internal name and offsets)

  mm_tex.width  = LE_U32(mm_tex.width);
  mm_tex.height = LE_U32(mm_tex.height);

  int width  = mm_tex.width;
  int height = mm_tex.height;

  if (width  < 8 || width  > 4096 ||
      height < 8 || height > 4096)
  {
    printf("FAILURE: weird size of image: %dx%d\n\n", width, height);
    return false;
  }

  byte *pixels = new byte[width * height];

  // NOTE: we assume that the pixels directly follow the miptex header
  
  if (! WAD2_ReadData(entry, (int)sizeof(mm_tex), width * height, pixels))
  {
    printf("FAILURE: could not read %dx%d pixels from miptex\n\n", width, height);
    delete pixels;
    return false;
  }

  // create the image for saving.
  // if the image contains fullbright pixels, the output filename
  // will be given the '_fbr' prefix.
  bool fullbright = false;

  rgb_image_c *img = new rgb_image_c(width, height);

  for (int y = 0; y < height; y++)
  for (int x = 0; x < width;  x++)
  {
    byte pix = pixels[y*width + x];

    if (pix >= 256-32)
      fullbright = true;

    byte R = quake1_palette[pix*3 + 0];
    byte G = quake1_palette[pix*3 + 1];
    byte B = quake1_palette[pix*3 + 2];

    img->PixelAt(x, y) = MAKE_RGB(R, G, B);
  }

  delete pixels;


  if (StringCaseCmpPartial(lump_name, "sky") == 0)
  {
    img->QuakeSkyFix();
  }


  const char *filename = ExpandFileName(lump_name, fullbright);

  if (FileExists(filename) && ! opt_overwrite)
  {
    printf("FAILURE: will not overwrite file: %s\n\n", filename);

    delete img;
    StringFree(filename);
    
    return false;
  }


  bool failed = false;

  FILE *fp = fopen(filename, "wb");
  if (fp)
  {
    if (! PNG_Save(fp, img))
    {
      printf("FAILURE: error while writing PNG file\n\n");
      failed = true;
    }

    fclose(fp);
  }
  else
  {
    printf("FAILURE: cannot create output file: %s\n\n", filename);
    failed = true;
  }

  delete img;
  StringFree(filename);

  return ! failed;
}


void MIP_ExtractWAD(const char *filename)
{
  if (! WAD2_OpenRead(filename))
    FatalError("Cannot open WAD2 file: %s\n", filename);

  printf("\n");
  printf("--------------------------------------------------\n");

  int num_lumps = WAD2_NumEntries();

  int skipped  = 0;
  int failures = 0;

  for (int i = 0; i < num_lumps; i++)
  {
    int type = WAD2_EntryType(i);
    const char *name = WAD2_EntryName(i);

    if (type != TYP_MIPTEX)
    {
      printf("SKIPPING NON-MIPTEX entry %d/%d : %s\n", i+1, num_lumps, name);
      skipped++;
      continue;
    }

    printf("Unpacking entry %d/%d : %s\n", i+1, num_lumps, name);

    if (! MIP_ExtractMipTex(i, name))
      failures++;
  }

  printf("--------------------------------------------------\n");
  printf("\n");

  WAD2_CloseRead();

  if (skipped > 0)
    printf("Skipped %d non-miptex lumps\n", skipped);

  printf("Extracted %d miptex, with %d failures\n",
         num_lumps - failures - skipped, failures);
}

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
