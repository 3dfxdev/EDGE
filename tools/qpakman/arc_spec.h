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

#ifndef __ARCHIVE_SPECIAL_H__
#define __ARCHIVE_SPECIAL_H__

typedef enum
{
  ARCSP_Normal  = 0,  // lump should be handled as normal
  ARCSP_Success,      // lump was special, successfully (un)packed
  ARCSP_Failed,       // lump was special, failed to (un)pack
  ARCSP_Ignored,      // lump was special and was ignored
}
arc_special_result_e;

int ARC_TryStoreSpecial(FILE *fp, const char *lump, const char *path);
int ARC_TryExtractSpecial(int entry, const char *lump, const char *path);

#endif  /* __ARCHIVE_SPECIAL_H__ */

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
