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
#include "main.h"

#include <map>
#include <algorithm>

#include "archive.h"
#include "arc_spec.h"
#include "pakfile.h"
#include "im_mip.h"
#include "q1_structs.h"


#define opt_recursive  true


extern std::vector<std::string> input_names;
extern std::string output_name;


std::map<std::string, int> all_created_dirs;
std::map<std::string, int> all_pak_lumps;

static int total_packed;


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

  char *filename = StringNew(strlen(name) + 32);
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
      // add it to created-list anyway, to prevent future warnings
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


  ARC_CreateNeededDirs(filename);


  if (! opt_raw && ARC_IsSpecialOutput(name))
  {
    bool result = ARC_ExtractSpecial(entry, name, filename);

    StringFree(filename);

    return result;
  }


  if (FileExists(filename) && ! opt_force)
  {
    printf("FAILURE: will not overwrite file: %s\n\n", filename);

    StringFree(filename);
    return false;
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


  if (! opt_raw)
  {
    ARC_TryAnalyseSpecial(entry, name, filename);
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

    printf("Unpacking %d/%d : %s\n", i+1, num_files, name);

    if (! ARC_ExtractOneFile(i, name))
      failures++;
  }

  printf("--------------------------------------------------\n");
  printf("\n");

  PAK_CloseRead();

// printf("Skipped %d entries\n", skipped);

  printf("Extracted %d entries, with %d failures\n",
         num_files - failures - skipped, failures);
}


//------------------------------------------------------------------------


void Spaces(int depth)
{
  for (; depth > 0; depth--)
    printf("  ");
}


const char * SanitizeInputName(const char *name)
{
  // Sanitize the input filename as follows:
  //
  // (a) check if name is too long
  // (b) strip leading / characters
  // (c) replace spaces and weird chars with '_' (show WARNING)
  // (d) replace \ with /

  while (*name == '/' || *name == '\\' || *name == '.')
    name++;

  if (strlen(name) > 55)
  {
    printf("FAILURE: filename is too long for PAK format.\n");
    return NULL;
  }

  char *lumpname = StringNew(strlen(name) + 32);
  char *pos = lumpname;

  bool warned = false;

  for (; *name; name++)
  {
    int ch = (unsigned char) *name;

    if (isspace(ch)) ch = '_';
    if (ch == '\\')  ch = '/';

    if ((ch == '.' && name[1] == '.') ||
        (ch == '/' && name[1] == '/'))
    {
      continue;
    }

    if (ch < 32 || ch > 126)
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

  if (strlen(lumpname) == 0)
  {
    printf("FAILURE: illegal filename\n");
    return NULL;
  }
  
  return lumpname;
}


void ARC_StoreFile(const char *path,
                   int *num_pack, int *failures, int *skipped)
{
  printf("Storing: %s\n", path);

  const char *lump_name = SanitizeInputName(path);
  if (! lump_name)
  {
    (*failures) += 1;
    return;
  }

  if (all_pak_lumps.find(lump_name) != all_pak_lumps.end())
  {
    printf("FAILURE: Lump already exists, will not duplicate\n\n");

    StringFree(lump_name);

    (*failures) += 1;
    return;
  }


  FILE *fp = fopen(path, "rb");
  if (! fp)
  {
    int what_error = errno;

    printf("FAILURE: could not open: %s\n\n", strerror(what_error));

    StringFree(lump_name);

    (*failures) += 1;
    return;
  }


  if (! opt_raw && ARC_IsSpecialInput(lump_name))
  {
    if (ARC_StoreSpecial(fp, lump_name, path))
      (*num_pack) += 1;
    else
      (*failures) += 1;

    StringFree(lump_name);

    fclose(fp);
    return;
  }


  PAK_NewLump(lump_name);

  all_pak_lumps[lump_name] = 1;

  StringFree(lump_name);

  (*num_pack) += 1;


  // transfer data
  bool read_error = false;

  static byte buffer[1024];

  for (;;)
  {
    int count = fread(buffer, 1, (size_t)sizeof(buffer), fp);

    if (count == 0)  // EOF
      break;

    if (count < 0)  // Error
    {
      int what_error = errno;

      printf("FAILURE: error reading file: %s\n\n",
             (what_error == 0) ? "Unknown error" : strerror(what_error));

      read_error = true;
      break;
    }

    PAK_AppendData(buffer, count);

    total_packed += count;
  }

  PAK_FinishLump();

  fclose(fp);

  if (read_error)
    (*failures) += 1;
}


static void PakDirScanner2(const char *name, int flags, void *priv_dat)
{
  if (flags & SCAN_F_Hidden)
    return;

  if (StringCaseCmp(name, "CVS") == 0)
    return;

  const char *prefix = (const char *)priv_dat;

  std::string full_name(prefix);

  full_name += DIR_SEP_STR;
  full_name += std::string(name);

  input_names.push_back(full_name);
}


void ARC_ProcessPath(const char *path,
                     int *num_pack, int *failures, int *skipped)
{
  // absolute filenames are not allowed
  if (path[0] == '/' || path[0] == '\\' ||
      (isalpha(path[0]) && path[1] == ':'))
  {
    printf("SKIPPING ABSOLUTE PATH: %s\n", path);
    (*skipped) += 1;
    return;
  }

  if (path[0] == '.' || strlen(path) == 0)
  {
    printf("SKIPPING BAD PATH: %s\n", path);
    (*skipped) += 1;
    return;
  }

  if (StringCaseCmp(path, "qpakman")     == 0 ||
      StringCaseCmp(path, "qpakman.exe") == 0)
  {
    printf("SKIPPING MYSELF: %s\n", path);
    (*skipped) += 1;
    return;
  }

  if (StringCaseCmp(path, output_name.c_str()) == 0)
  {
    printf("SKIPPING OUTPUT FILE: %s\n", path);
    (*skipped) += 1;
    return;
  }


  if (PathIsDirectory(path))
  {
    if (! opt_recursive)
    {
      printf("SKIPPING DIRECTORY: %s\n", path);
      (*skipped) += 1;
      return;
    }

    if (StringCaseCmp(FindBaseName(path), ".svn") == 0 ||
        StringCaseCmp(FindBaseName(path), "CVS")  == 0)
    {
      printf("SKIPPING REPOSITORY CRAP: %s\n", path);
      (*skipped) += 1;
      return;
    }

    printf("\n");
    printf("Processing directory: %s\n", path);

    int result = ScanDirectory(path, PakDirScanner2, (void*)path);

    if (result < 0)
    {
      printf("FAILURE: error scanning directory\n\n");
      (*failures) += 1;
    }
    else if (result == 0)
    {
      printf("(empty directory)\n\n");
    }

    return;
  }


  if (CheckExtension(path, "pak"))
  {
    // TODO
    printf("MERGING PAKS NOT IMPLEMENTED: %s\n", path);
    (*failures) += 1;
    return;
  }

  if (CheckExtension(path, "ana") ||
      CheckExtension(path, "gen"))
  {
    printf("SKIPPING ANALYSIS or GENERATOR: %s\n", path);
    (*skipped) += 1;
    return;
  }


  ARC_StoreFile(path, num_pack, failures, skipped);
}


void ARC_CreatePAK(const char *filename)
{
  if (input_names.size() == 0)
    FatalError("No input files were specified!\n");

  if (! PAK_OpenWrite(filename))
    FatalError("Cannot create PAK file: %s\n", filename);

  printf("\n");
  printf("--------------------------------------------------\n");

  int num_pack = 0;
  int failures = 0;
  int skipped  = 0;

  total_packed = 0;

  // ALGORITHM
  //
  // The basic approach is to treat the 'input_names' vector as a
  // stack, and we pop each path_name off the top to process it.
  //
  // When the path is a file, we copy it into the output PAK, but
  // for directories we scan it and push all the names in that
  // directory onto the stack.

  // reverse out stack, so we process entries in the same order as
  // given on the command line.
  std::reverse(input_names.begin(), input_names.end());

  while (! input_names.empty())
  {
    const char *path = StringDup(input_names.back().c_str());

    input_names.pop_back();

    ARC_ProcessPath(path, &num_pack, &failures, &skipped);

    StringFree(path);
  }

  printf("--------------------------------------------------\n");
  printf("\n");

  PAK_CloseWrite();

  if (skipped > 0)
    printf("Skipped %d pathnames\n", skipped);

  printf("Packed %d files (%1.2f MB), with %d failures\n",
         num_pack, (total_packed+9999) / 1000000.0, failures);
}

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
