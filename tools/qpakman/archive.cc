//------------------------------------------------------------------------
//  Archiving files to/from a PAK
//------------------------------------------------------------------------
// 
//  Copyright (c) 2008  Andrew J. Apted
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

#include "archive.h"
#include "pakfile.h"
#include "q1_structs.h"

extern std::vector<std::string> input_names;

extern bool opt_recursive;
extern bool opt_overwrite;


void ARC_CreatePAK(const char *filename)
{
  // TODO: ARC_CreatePAK
  FatalError("PAK creation not yet implemented\n");
}


void ARC_ExtractPAK(const char *filename)
{
  // TODO: ARC_ExtractPAK
  FatalError("PAK extraction not yet implemented\n");
}


//--- editor settings ---
// vi:ts=2:sw=2:expandtab
