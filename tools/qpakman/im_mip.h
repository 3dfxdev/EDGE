//------------------------------------------------------------------------
//  Basic image storage
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

#ifndef __IMAGE_MIP_H__
#define __IMAGE_MIP_H__

bool MIP_ProcessImage(const char *filename);
void MIP_CreateWAD(const char *filename);
void MIP_ExtractWAD(const char *filename);

bool MIP_DecodeWAL(int entry, const char *filename);
void MIP_EncodeWAL(const char *filename);

/* ----- PIC structure ---------------------- */

typedef struct
{
  u32_t width;
  u32_t height;

//  byte pixels[width * height];
}
pic_header_t;


/* ----- WAL structure ---------------------- */

typedef struct
{
  char name[32];

  u32_t width;
  u32_t height;
  u32_t offsets[4];

  char anim_next[32];

  u32_t flags;
  s32_t contents;
  s32_t value;
}
wal_header_t;


#endif  /* __IMAGE_MIP_H__ */

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
