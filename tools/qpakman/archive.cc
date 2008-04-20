//------------------------------------------------------------------------
//  Archiving files to/from a PAK
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


//------------------------------------------------------------------------

bool ARC_ExtractOneFile(int entry, const char *name)
{
  int entry_len = PAK_EntryLen(entry);


  // FIXME: validate/sanitize entry name
  const char * filename = StringDup(name);


  if (FileExists(filename) && ! opt_overwrite)
  {
    printf("FAILURE: will not overwrite file: %s\n\n", filename);

    StringFree(filename);
    return false;
  }


  bool failed = false;

  FILE *fp = fopen(filename, "wb");
  if (fp)
  {
    static byte buffer[2048];

    // transfer data from PAK into new file
    int position = 0;

    while (position < entry_len)
    {
      int count = MIN((int)sizeof(buffer), entry_len - position);

      if (! PAK_ReadData(entry, position, count, buffer))
      {
        printf("FAILURE: read error on %d bytes\n\n", count);
        failed = true;
        break;
      }

      if (1 != fwrite(buffer, count, 1, fp))
      {
        printf("FAILURE: write error on %d bytes\n\n", count);
        failed = true;
        break;
      }

      position += count;
    }

    fclose(fp);
  }
  else
  {
    printf("FAILURE: cannot create output file: %s\n\n", filename);
    failed = true;
  }

  StringFree(filename);

  return ! failed;
}


void ARC_ExtractPAK(const char *filename)
{
  if (! PAK_OpenRead(filename))
    FatalError("Cannot open PAK file: %s\n", filename);

  printf("\n");
  printf("--------------------------------------------------\n");

  int num_files = PAK_NumEntries();

  int skipped  = 0;
  int failures = 0;

  for (int i = 0; i < num_files; i++)
  {
    const char *name = PAK_EntryName(i);

    printf("Unpacking entry %d/%d : %s\n", i+1, num_files, name);

    if (! ARC_ExtractOneFile(i, name))
      failures++;
  }

  printf("\n");
  printf("--------------------------------------------------\n");

  PAK_CloseRead();

// printf("Skipped %d entries\n", skipped);

  printf("Extracted %d entries, with %d failures\n",
         num_files - failures - skipped, failures);
}

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
