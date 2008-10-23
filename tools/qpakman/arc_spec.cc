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


bool ARC_IsSpecialInput (const char *lump_name)
{
  // TODO ARC_IsSpecialInput
  return false;
}

bool ARC_StoreSpecial(FILE *fp, const char *path)
{
  // TODO ARC_StoreSpecial
  return false;
}


//------------------------------------------------------------------------

bool ARC_IsSpecialOutput(const char *lump_name)
{
  // TODO ARC_IsSpecialOutput
  return false;
}


bool ARC_ExtractSpecial(int entry, const char *lump_name)
{
  // TODO ARC_ExtractSpecial
  return false;
}


//--- editor settings ---
// vi:ts=2:sw=2:expandtab
