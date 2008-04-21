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


std::map<std::string, int> all_created_dirs;


void ARC_CreatePAK(const char *filename)
{
  // TODO: ARC_CreatePAK
  FatalError("PAK creation not yet implemented\n");
}


//------------------------------------------------------------------------

const char *SanitizeOutputName(const char *name)
{
  // Sanitize the output filename as follows:
  //
  // (a) replace spaces and weird chars with '_' (show WARNING)
  // (b) replace \ with /
  // (c) strip leading / characters
  // (d) disallow .. and //

  while (*name == '/' || *name == '\\' || *name == '.')
    name++;

  int name_len = strlen(name);

  char *filename = StringNew(name_len + 32);
  char *pos = filename;

  bool warned = false;

  for (; *name; name++)
  {
    char ch = *name;

    if (ch == ' ')  ch = '_';
    if (ch == '\\') ch = '/';

    if ((ch == '.' && name[1] == '.') ||
        (ch == '/' && name[1] == '/'))
    {
      continue;
    }

    if (! (isalnum(ch) || ch == '_' || ch == '-' ||
           ch == '/'   || ch == '.'))
    {
      if (! warned)
      {
        printf("WARNING: removing weird characters from name (\\%03o)\n",
               (unsigned char)ch);
        warned = true;
      }

      ch = '_';
    }

    *pos++ = ch;
  }

  *pos = 0;

  if (strlen(filename) == 0)
  {
    printf("FAILURE: illegal filename\n");
    return NULL;
  }
  
  return filename;
}


static bool ARC_CreateNeededDirs(const char *filename)
{
  for (int level = 0; level < 60; level++)
  {
    const char *end_p = filename;

    for (int i = 0; i <= level; i++)
    {
      end_p = strchr(end_p, '/');

      if (! end_p)
        return true;

      if (i < level)
        end_p++;
    }

    char *dir_name = StringNew(end_p - filename + 4);

    StringMaxCopy(dir_name, filename, end_p - filename);

    // check whether we made the directory before
    if (all_created_dirs.find(dir_name) == all_created_dirs.end())
    {
      // add it to created-list anyway, to prevent more warnings
      all_created_dirs[dir_name] = 1;

      if (! FileMakeDir(dir_name))
      {
        printf("WARNING: could not create directory: %s\n", dir_name);
#if 0
        StringFree(dir_name);
        return false;
#endif
      }
    }

    StringFree(dir_name);
  }

  // should never get here (but no biggie if we do)
  return false;
}


bool ARC_ExtractOneFile(int entry, const char *name)
{
  const char * filename = SanitizeOutputName(name);
  if (! filename)
    return false;


  if (FileExists(filename) && ! opt_overwrite)
  {
    printf("FAILURE: will not overwrite file: %s\n\n", filename);

    StringFree(filename);
    return false;
  }

  if (! ARC_CreateNeededDirs(filename))
  {
#if 0
    // error message displayed by ARC_CreateNeededDirs()
    StringFree(filename);
    return false;
#endif
  }


  int entry_len = PAK_EntryLen(entry);

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
