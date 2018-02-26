//------------------------------------------------------------------------
// MAIN : Main program
//------------------------------------------------------------------------
//
//  LevNamer (C) 2001 Andrew Apted
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
 
#include "system.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <assert.h>

#include "structs.h"
#include "wad.h"


static char *input_wad  = NULL;
static char *output_wad = NULL;

static int num_files = 0;


#define MAX_LEV_NAMES  200

typedef struct name_set_s
{
  int size;

  struct
  {
    char s[10];
  }
  names[MAX_LEV_NAMES];
}
name_set_t;

static name_set_t source_set = { 0, };
static name_set_t dest_set   = { 0, };


//
// ReadNameIndex
//
// Returns -1 if invalid.
//
int ReadNameIndex(const char *buffer)
{
  const char *d1, *d2;
  int v1, v2;

  for (d1=buffer; *d1; d1++)
    if (isdigit(*d1))
      break;

  if (*d1 == 0)  // no digits at all
    return 0;

  v1 = *d1 - '0';
  
  for (d2 = d1+1; *d2; d2++)
    if (isdigit(*d2))
      break;

  if (*d2 == 0)  // only one digit
    return v1;

  v2 = *d2 - '0';

  // special handling for episode/mission style
  if (d2 > (d1+1))
  {
    if (v1 == 0 || v2 == 0)
      return -1;

    return (v1 - 1) * 9 + (v2 - 1);
  }

  return v1 * 10 + v2;
}


//
// WriteNameIndex
//
// Returns -1 if failed (no more names left).
//
int WriteNameIndex(char *buffer, int index)
{
  char *d1, *d2;

  if (index < 0)
    return -1;

  for (d1=buffer; *d1; d1++)
    if (isdigit(*d1))
      break;

  if (*d1 == 0)  // no digits at all
    return (index == 0) ? 0 : -1;

  for (d2 = d1+1; *d2; d2++)
    if (isdigit(*d2))
      break;

  if (*d2 == 0)  // only one digit
  {
    if (index > 9)
      return -1;

    *d1 = '0' + index;
    return 0;
  }

  // special handling for episode/mission style
  if (d2 > (d1+1))
  {
    if (index > 80)
      return -1;

    *d1 = '1' + (index / 9);
    *d2 = '1' + (index % 9);

    return 0;
  }

  if (index > 99)
    return -1;

  *d1 = '0' + (index / 10);
  *d2 = '0' + (index % 10);

  return 0;
}

// 
// FindNameInSet
//
// Returns the index value, or -1 if not found.
// 
int FindNameInSet(const name_set_t *set, const char *name)
{
  int i;

  for (i=0; i < set->size; i++)
    if (strcmp(set->names[i].s, name) == 0)
      return i;

  return -1;
}


//
// CheckNameValid
//
// Does a fatal error if not valid.
//
static void CheckNameValid(const char *name)
{
  const char *pos;
  int digit_num;

  if (strlen(name) == 0)
    FatalError("Bad level name: zero length !");
  
  if (strlen(name) > 5)
    FatalError("Level name `%s' too long - limit is 5 chars", name);
        
  if (! isalpha(name[0]))
    FatalError("Bad level name `%s' - must start with letter", name);

  // test on valid chars
  for (pos = name; *pos; pos++)
  {
    if (! isalnum(*pos) && (*pos) != '_')
      FatalError("Bad level name `%s' - illegal char `%c'", name, *pos);
  }

  // test on number of digits
  for (pos = name, digit_num = 0; *pos; pos++)
    if (isdigit(*pos))
      digit_num++;

  if (digit_num > 2)
    FatalError("Bad level name `%s' - too many digits", name);
  
  // do a check on episode/mission style names
  if (ReadNameIndex(name) < 0)
    FatalError("Level name `%s' has bad episode/mission", name);
}

//
// CheckRangeLetters
//
// Prevents things like E1M1-MAP37.
// 
static void CheckRangeLetters(const char *name1, const char *name2)
{
  if (strlen(name1) < strlen(name2))
    FatalError("Bad range: %s shorter than %s", name1, name2);

  if (strlen(name1) > strlen(name2))
    FatalError("Bad range: %s longer than %s", name1, name2);

  for (; *name1; name1++, name2++)
  {
    assert(*name2);

    if (isdigit(*name1) && isdigit(*name2))
      continue;

    if (*name1 != *name2)
      FatalError("Bad range: %s doesn't match %s", name1, name2);
  }
}

//
//  ParseNameRange
//  
void ParseNameRange(name_set_t *set, const char *arg, const char *pos)
{
  char name1[32];
  char name2[32];
  
  int len1, len2;
  int idx1, idx2;
  int i;

  // Get names and check if valid

  len1 = pos - arg;
  len2 = strlen(arg) - 1 - len1;
    
  if (len1 <= 0 || len2 <= 0)
    FatalError("Bad name range `%s'", arg);

  if (len1 > 30 || len2 > 30)
    FatalError("Invalid name range `%s' (too long)", arg);

  strncpy(name1, arg, len1);
  name1[len1] = 0;

  strncpy(name2, pos+1, len2);
  name2[len2] = 0;

  StrUpperCase(name1);
  StrUpperCase(name2);

  CheckNameValid(name1);
  CheckNameValid(name2);

  CheckRangeLetters(name1, name2);

  // OK, we have two valid names -- compute index values

  idx1 = ReadNameIndex(name1);
  if (idx1 < 0)
    FatalError("Level name `%s' has bad episode/mission", name1);

  idx2 = ReadNameIndex(name2);
  if (idx2 < 0)
    FatalError("Level name `%s' has bad episode/mission", name2);

  if (idx1 > idx2)
    FatalError("Level range `%s' is reversed !", arg);

  // Note: this should never happen (due to 2-digit limit)
  if (idx2 - idx1 + 1 > 110)
    FatalError("Range `%s' too large somehow !", arg);

  // Fill in the range...
  
  set->size = idx2 - idx1 + 1;

  for (i=0; i < set->size; i++)
  {
    strcpy(set->names[i].s, name1);

    // this shouldn't fail either...
    if (WriteNameIndex(set->names[i].s, idx1 + i) < 0)
      FatalError("Range `%s' overflowed somehow !", arg);
  }
}

//
//  ParseNameSet
//  
void ParseNameSet(name_set_t *set, const char *arg)
{
  const char *pos;
  const char *next = NULL;

  if (set->size > 0)
    FatalError("The -convert option can only be used once");
   
  // check for ranges first

  pos = strchr(arg, '-');
  if (pos != NULL)
  {
    ParseNameRange(set, arg, pos);
    return;
  }

  // handle comma-separated names, possibly just one.
  // Note: destructive to `arg' pointer.
  
  for (pos = arg; *pos; pos = next)
  {
    char name[32];
    int len;

    next = strchr(pos, ',');

    if (next)
    {
      len = next - pos;
      next++;
    }
    else
    {
      len = strlen(pos);
      next = pos + len;
    }

    if (len <= 0)
      FatalError("Bad level name in `%s' - zero length", arg);

    if (len > 30)
      FatalError("Bad level name in `%s' - way too long", arg);

    // check for overflow (REAL unlikely)
    if (set->size > 110)
      FatalError("Too many level names specified !");

    strncpy(name, pos, len);
    name[len] = 0;

    StrUpperCase(name);
    CheckNameValid(name);

    // check if already specified
    if (FindNameInSet(set, name) >= 0)
      FatalError("Level name `%s' specified twice !", name);

    strcpy(set->names[set->size].s, name);
    set->size++;
  }
}


/* ----- user information ----------------------------- */

static void ShowTitle(void)
{
  PrintMsg(
    "\n"
    "****  LevNamer  V1.01        ****\n"
    "****  (C) 2001 Andrew Apted  ****\n"
    "\n");
}

static void ShowUsage(void)
{
  PrintMsg(
    "This program is free software, under the terms of the GNU General\n"
    "Public License, and comes with ABSOLUTELY NO WARRANTY.  See the\n"
    "accompanying documentation for more details.\n"
    "\n"
    "Usage:\n"
    "   levnamer  input.wad  -o output.wad  -convert source dest\n"
    "\n"
    "Options:\n"
    "   -loadall       Loads all data from source wad (don't copy)\n"
    "\n"
  );

  exit(1);
}


/* ----- option parsing ----------------------------- */

typedef struct option_s
{
  char *name;
  void *var;
  enum { BOOL, INT, STRING } kind;
}
option_t;

static option_t option_list[] =
{
  { "o",           &output_wad,    STRING },
  { "noprog",      &no_progress,   BOOL   },
  { "loadall",     &load_all,      BOOL   },
  { "hexen",       &hexen_mode,    BOOL   },

  { NULL, NULL, BOOL }
};

static void ParseOptions(int argc, char **argv)
{
  char *opt_str;
  option_t *cur;

  int *temp_int;
  char **temp_str;

  // skip program name
  argv++; argc--;

  if (argc == 0)
    ShowUsage();
 
  if (strcmp(argv[0], "/?") == 0)
    ShowUsage();

  while (argc > 0)
  {
    if (argv[0][0] != '-')
    {
      // --- ORDINARY FILENAME ---

      if (num_files >= 1)
        FatalError("Too many filenames.  Use the -o option", argv[0]);

      input_wad = argv[0];
      num_files++;

      argv++; argc--;
      continue;
    }

    // --- AN OPTION ---

    opt_str = & argv[0][1];

    // handle GNU style options beginning with `--'
    if (opt_str[0] == '-')
      opt_str++;

    // handle help option
    if (StrCaseCmp(opt_str, "help") == 0 ||
        StrCaseCmp(opt_str, "h") == 0)
    {
      ShowUsage();
    }

    // handle -convert option
    if (StrCaseCmp(opt_str, "convert") == 0)
    {
      // need two extra arguments
      if (argc < 3)
        FatalError("The %s option needs source and dest names", argv[0]);

      ParseNameSet(&source_set, argv[1]);
      ParseNameSet(&dest_set,   argv[2]);

      argv += 3; argc -= 3;
      continue;
    }

    // find option in list
    for (cur = option_list; cur->name; cur++)
    {
      if (StrCaseCmp(opt_str, cur->name) == 0)
        break;
    }

    if (! cur->name)
      FatalError("Unknown option: %s", argv[0]);

    switch (cur->kind)
    {
      case BOOL:
        temp_int = (int *) cur->var;
        (*temp_int)++;
        argv++; argc--;
        break;

      case INT:
        // need an extra argument
        if (argc < 2)
          FatalError("Missing number for the %s option", argv[0]);

        temp_int = (int *) cur->var;
        (*temp_int) = (int) strtol(argv[1], NULL, 10);
        argv += 2; argc -= 2;
        break;

      case STRING:
        // need an extra argument
        if (argc < 2)
          FatalError("Missing name for the %s option", argv[0]);

        temp_str = (char **) cur->var;

        if (*temp_str != NULL)
          FatalError("The %s option cannot be used twice", argv[0]);
          
        *temp_str = argv[1];
        argv += 2; argc -= 2;
        break;
    }
  }
}

static void CheckOptions(void)
{
  if (!input_wad)
    FatalError("Missing input filename");

  if (!output_wad)
    FatalError("Missing output filename");

  if (StrCaseCmp(input_wad, output_wad) == 0)
  {
    PrintMsg("* Output file is same as input file.  Using -loadall\n\n");
    load_all = 1;
  }

  if (source_set.size == 0 || dest_set.size == 0)
    FatalError("Missing conversion info");

  // Convenience feature -- allow sole destination to imply range

  if (source_set.size > 1 && dest_set.size == 1)
  {
    int i;
    int base_idx;

    dest_set.size = source_set.size;
    
    base_idx = ReadNameIndex(dest_set.names[0].s);
    assert(base_idx >= 0);
    
    for (i=1; i < dest_set.size; i++)
    {
      strcpy(dest_set.names[i].s, dest_set.names[0].s);

      if (WriteNameIndex(dest_set.names[i].s, base_idx + i) < 0)
        FatalError("Dest name %s overflowed when expanding",
            dest_set.names[i-1].s);
    }
    
    PrintMsg("* Destination names expanded to %s-%s\n\n",
      dest_set.names[0].s, dest_set.names[dest_set.size-1].s);
  }

  if (dest_set.size > source_set.size)
    FatalError("Destination has more names than source");

  if (dest_set.size < source_set.size)
    FatalError("Destination has fewer names than source");
}


/* ----- scan the directory and convert names ----------- */

static void ConvertNames(void)
{
  lump_t *L;
  int idx;
  int num_on_line = 0;
  int levels_converted = 0;

  for (L=wad.dir_head; L; L=L->next)
  {
    idx = FindNameInSet(&source_set, L->name);

    if (idx >= 0)
    {
      PrintMsg("%s -> %s   ", L->name, dest_set.names[idx].s);

      num_on_line++;
      if (num_on_line % 2 == 0)
        PrintMsg("\n");

      SysFree(L->name);
      L->name = SysStrdup(dest_set.names[idx].s);

      levels_converted++;
      continue;
    }

    // handle GL Nodes

    if (strlen(L->name) >= 3 && L->name[0] == 'G' &&
        L->name[1] == 'L' && L->name[2] == '_')
    {
      idx = FindNameInSet(&source_set, L->name + 3);
      if (idx >= 0)
      {
        char buffer[10];

        sprintf(buffer, "GL_%s", dest_set.names[idx].s);

        PrintMsg("%s -> %s   ", L->name, buffer);

        num_on_line++;
        if (num_on_line % 2 == 0)
          PrintMsg("\n");

        SysFree(L->name);
        L->name = SysStrdup(buffer);

        levels_converted++;
        continue;
      }
    }
  }

  if (num_on_line % 2 != 0)
    PrintMsg("\n");

  if (levels_converted == 0)
    PrintMsg("WARNING: No level names converted !\n");
}


/* ----- Main Program -------------------------------------- */

int main(int argc, char **argv)
{
  InitProgress();

  ShowTitle();

  ParseOptions(argc, argv);
  CheckOptions();

  // opens and reads directory from the input wad
  ReadWadFile(input_wad);

  PrintMsg("\n");
  ConvertNames();

  // writes all the lumps to the output wad
  WriteWadFile(output_wad);

  // close wads and free memory
  CloseWads();

  TermProgress();

  return 0;
}

