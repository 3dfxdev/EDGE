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

extern bool opt_force;


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

const byte quake2_palette[256*3] =
{
    0,  0,  0,  15, 15, 15,  31, 31, 31,  47, 47, 47,  63, 63, 63,
   75, 75, 75,  91, 91, 91, 107,107,107, 123,123,123, 139,139,139,
  155,155,155, 171,171,171, 187,187,187, 203,203,203, 219,219,219,
  235,235,235,  99, 75, 35,  91, 67, 31,  83, 63, 31,  79, 59, 27,
   71, 55, 27,  63, 47, 23,  59, 43, 23,  51, 39, 19,  47, 35, 19,
   43, 31, 19,  39, 27, 15,  35, 23, 15,  27, 19, 11,  23, 15, 11,
   19, 15,  7,  15, 11,  7,  95, 95,111,  91, 91,103,  91, 83, 95,
   87, 79, 91,  83, 75, 83,  79, 71, 75,  71, 63, 67,  63, 59, 59,
   59, 55, 55,  51, 47, 47,  47, 43, 43,  39, 39, 39,  35, 35, 35,
   27, 27, 27,  23, 23, 23,  19, 19, 19, 143,119, 83, 123, 99, 67,
  115, 91, 59, 103, 79, 47, 207,151, 75, 167,123, 59, 139,103, 47,
  111, 83, 39, 235,159, 39, 203,139, 35, 175,119, 31, 147, 99, 27,
  119, 79, 23,  91, 59, 15,  63, 39, 11,  35, 23,  7, 167, 59, 43,
  159, 47, 35, 151, 43, 27, 139, 39, 19, 127, 31, 15, 115, 23, 11,
  103, 23,  7,  87, 19,  0,  75, 15,  0,  67, 15,  0,  59, 15,  0,
   51, 11,  0,  43, 11,  0,  35, 11,  0,  27,  7,  0,  19,  7,  0,
  123, 95, 75, 115, 87, 67, 107, 83, 63, 103, 79, 59,  95, 71, 55,
   87, 67, 51,  83, 63, 47,  75, 55, 43,  67, 51, 39,  63, 47, 35,
   55, 39, 27,  47, 35, 23,  39, 27, 19,  31, 23, 15,  23, 15, 11,
   15, 11,  7, 111, 59, 23,  95, 55, 23,  83, 47, 23,  67, 43, 23,
   55, 35, 19,  39, 27, 15,  27, 19, 11,  15, 11,  7, 179, 91, 79,
  191,123,111, 203,155,147, 215,187,183, 203,215,223, 179,199,211,
  159,183,195, 135,167,183, 115,151,167,  91,135,155,  71,119,139,
   47,103,127,  23, 83,111,  19, 75,103,  15, 67, 91,  11, 63, 83,
    7, 55, 75,   7, 47, 63,   7, 39, 51,   0, 31, 43,   0, 23, 31,
    0, 15, 19,   0,  7, 11,   0,  0,  0, 139, 87, 87, 131, 79, 79,
  123, 71, 71, 115, 67, 67, 107, 59, 59,  99, 51, 51,  91, 47, 47,
   87, 43, 43,  75, 35, 35,  63, 31, 31,  51, 27, 27,  43, 19, 19,
   31, 15, 15,  19, 11, 11,  11,  7,  7,   0,  0,  0, 151,159,123,
  143,151,115, 135,139,107, 127,131, 99, 119,123, 95, 115,115, 87,
  107,107, 79,  99, 99, 71,  91, 91, 67,  79, 79, 59,  67, 67, 51,
   55, 55, 43,  47, 47, 35,  35, 35, 27,  23, 23, 19,  15, 15, 11,
  159, 75, 63, 147, 67, 55, 139, 59, 47, 127, 55, 39, 119, 47, 35,
  107, 43, 27,  99, 35, 23,  87, 31, 19,  79, 27, 15,  67, 23, 11,
   55, 19, 11,  43, 15,  7,  31, 11,  7,  23,  7,  0,  11,  0,  0,
    0,  0,  0, 119,123,207, 111,115,195, 103,107,183,  99, 99,167,
   91, 91,155,  83, 87,143,  75, 79,127,  71, 71,115,  63, 63,103,
   55, 55, 87,  47, 47, 75,  39, 39, 63,  35, 31, 47,  27, 23, 35,
   19, 15, 23,  11,  7,  7, 155,171,123, 143,159,111, 135,151, 99,
  123,139, 87, 115,131, 75, 103,119, 67,  95,111, 59,  87,103, 51,
   75, 91, 39,  63, 79, 27,  55, 67, 19,  47, 59, 11,  35, 47,  7,
   27, 35,  0,  19, 23,  0,  11, 15,  0,   0,255,  0,  35,231, 15,
   63,211, 27,  83,187, 39,  95,167, 47,  95,143, 51,  95,123, 51,
  255,255,255, 255,255,211, 255,255,167, 255,255,127, 255,255, 83,
  255,255, 39, 255,235, 31, 255,215, 23, 255,191, 15, 255,171,  7,
  255,147,  0, 239,127,  0, 227,107,  0, 211, 87,  0, 199, 71,  0,
  183, 59,  0, 171, 43,  0, 155, 31,  0, 143, 23,  0, 127, 15,  0,
  115,  7,  0,  95,  0,  0,  71,  0,  0,  47,  0,  0,  27,  0,  0,
  239,  0,  0,  55, 55,255, 255,  0,  0,   0,  0,255,  43, 43, 35,
   27, 27, 23,  19, 19, 15, 235,151,127, 195,115, 83, 159, 87, 51,
  123, 63, 27, 235,211,199, 199,171,155, 167,139,119, 135,107, 87,
  159, 91, 83                                         
};

const byte hexen2_palette[256*3] =
{
    0,  0,  0,   0,  0,  0,   8,  8,  8,  16, 16, 16,  24, 24, 24,
   32, 32, 32,  40, 40, 40,  48, 48, 48,  56, 56, 56,  64, 64, 64,
   72, 72, 72,  80, 80, 80,  84, 84, 84,  88, 88, 88,  96, 96, 96,
  104,104,104, 112,112,112, 120,120,120, 128,128,128, 136,136,136,
  148,148,148, 156,156,156, 168,168,168, 180,180,180, 184,184,184,
  196,196,196, 204,204,204, 212,212,212, 224,224,224, 232,232,232,
  240,240,240, 252,252,252,   8,  8, 12,  16, 16, 20,  24, 24, 28,
   28, 32, 36,  36, 36, 44,  44, 44, 52,  48, 52, 60,  56, 56, 68,
   64, 64, 72,  76, 76, 88,  92, 92,104, 108,112,128, 128,132,152,
  152,156,176, 168,172,196, 188,196,220,  32, 24, 20,  40, 32, 28,
   48, 36, 32,  52, 44, 40,  60, 52, 44,  68, 56, 52,  76, 64, 56,
   84, 72, 64,  92, 76, 72, 100, 84, 76, 108, 92, 84, 112, 96, 88,
  120,104, 96, 128,112,100, 136,116,108, 144,124,112,  20, 24, 20,
   28, 32, 28,  32, 36, 32,  40, 44, 40,  44, 48, 44,  48, 56, 48,
   56, 64, 56,  64, 68, 64,  68, 76, 68,  84, 92, 84, 104,112,104,
  120,128,120, 140,148,136, 156,164,152, 172,180,168, 188,196,184,
   48, 32,  8,  60, 40,  8,  72, 48, 16,  84, 56, 20,  92, 64, 28,
  100, 72, 36, 108, 80, 44, 120, 92, 52, 136,104, 60, 148,116, 72,
  160,128, 84, 168,136, 92, 180,144,100, 188,152,108, 196,160,116,
  204,168,124,  16, 20, 16,  20, 28, 20,  24, 32, 24,  28, 36, 28,
   32, 44, 32,  36, 48, 36,  40, 56, 40,  44, 60, 44,  48, 68, 48,
   52, 76, 52,  60, 84, 60,  68, 92, 64,  76,100, 72,  84,108, 76,
   92,116, 84, 100,128, 92,  24, 12,  8,  32, 16,  8,  40, 20,  8,
   52, 24, 12,  60, 28, 12,  68, 32, 12,  76, 36, 16,  84, 44, 20,
   92, 48, 24, 100, 56, 28, 112, 64, 32, 120, 72, 36, 128, 80, 44,
  144, 92, 56, 168,112, 72, 192,132, 88,  24,  4,  4,  36,  4,  4,
   48,  0,  0,  60,  0,  0,  68,  0,  0,  80,  0,  0,  88,  0,  0,
  100,  0,  0, 112,  0,  0, 132,  0,  0, 152,  0,  0, 172,  0,  0,
  192,  0,  0, 212,  0,  0, 232,  0,  0, 252,  0,  0,  16, 12, 32,
   28, 20, 48,  32, 28, 56,  40, 36, 68,  52, 44, 80,  60, 56, 92,
   68, 64,104,  80, 72,116,  88, 84,128, 100, 96,140, 108,108,152,
  120,116,164, 132,132,176, 144,144,188, 156,156,200, 172,172,212,
   36, 20,  4,  52, 24,  4,  68, 32,  4,  80, 40,  0, 100, 48,  4,
  124, 60,  4, 140, 72,  4, 156, 88,  8, 172,100,  8, 188,116, 12,
  204,128, 12, 220,144, 16, 236,160, 20, 252,184, 56, 248,200, 80,
  248,220,120,  20, 16,  4,  28, 24,  8,  36, 32,  8,  44, 40, 12,
   52, 48, 16,  56, 56, 16,  64, 64, 20,  68, 72, 24,  72, 80, 28,
   80, 92, 32,  84,104, 40,  88,116, 44,  92,128, 52,  92,140, 52,
   92,148, 56,  96,160, 64,  60, 16, 16,  72, 24, 24,  84, 28, 28,
  100, 36, 36, 112, 44, 44, 124, 52, 48, 140, 64, 56, 152, 76, 64,
   44, 20,  8,  56, 28, 12,  72, 32, 16,  84, 40, 20,  96, 44, 28,
  112, 52, 32, 124, 56, 40, 140, 64, 48,  24, 20, 16,  36, 28, 20,
   44, 36, 28,  56, 44, 32,  64, 52, 36,  72, 60, 44,  80, 68, 48,
   92, 76, 52, 100, 84, 60, 112, 92, 68, 120,100, 72, 132,112, 80,
  144,120, 88, 152,128, 96, 160,136,104, 168,148,112,  36, 24, 12,
   44, 32, 16,  52, 40, 20,  60, 44, 20,  72, 52, 24,  80, 60, 28,
   88, 68, 28, 104, 76, 32, 148, 96, 56, 160,108, 64, 172,116, 72,
  180,124, 80, 192,132, 88, 204,140, 92, 216,156,108,  60, 20, 92,
  100, 36,116, 168, 72,164, 204,108,192,   4, 84,  4,   4,132,  4,
    0,180,  0,   0,216,  0,   4,  4,144,  16, 68,204,  36,132,224,
   88,168,232, 216,  4,  4, 244, 72,  0, 252,128,  0, 252,172, 24,
  252,252,252                                         
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

  if (FileExists(filename) && ! opt_force)
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
