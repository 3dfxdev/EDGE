//------------------------------------------------------------------------
//  QPAKMAN Main program
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

#include <time.h>

#include "archive.h"
#include "im_mip.h"
#include "pakfile.h"

#define VERSION  "0.30"


std::string output_name;

std::vector<std::string> input_names;

typedef enum
{
  ACT_None = 0,

  ACT_Create,
  ACT_List,
  ACT_Extract
}
prog_action_type_e;

static prog_action_type_e program_action = ACT_None;

bool opt_recursive = false;
bool opt_overwrite = false;


void FatalError(const char *message, ...)
{
  fprintf(stdout, "FATAL ERROR: ");

  va_list argptr;

  va_start(argptr, message);
  vfprintf(stdout, message, argptr);
  va_end(argptr);

  fprintf(stdout, "\n");
  fflush(stdout);

  exit(9);
}


void ShowTitle(void)
{
  printf("\n");
  printf("**** QPAKMAN v" VERSION "  (C) 2008 Andrew Apted ****\n");
  printf("\n");
}


void ShowUsage(void)
{
  printf("USAGE: qpakman  [OPTIONS...]  FILES...  -o OUTPUT.pak\n");
  printf("       qpakman  [OPTIONS...]  IMAGES..  -o OUTPUT.wad\n");
  printf("       qpakman  [OPTIONS...]  INPUT.pak/wad\n");
  printf("\n");

  printf("OPTIONS:\n");
  printf("   -r  -recursive   descend into directories (PAK creation)\n");
  printf("   -l  -list        list contents of PAK/WAD file\n");
  printf("   -e  -extract     extract PAK/WAD contents into current dir\n");
  printf("       -overwrite   overwrite existing files when extracting\n");
  printf("\n");

  printf("This program is free software, under the terms of the GNU General\n");
  printf("Public License, and comes with ABSOLUTELY NO WARRANTY.  See the\n");
  printf("accompanying documentation for more details.  USE AT OWN RISK.\n");
  printf("\n");
}


/* returns number of arguments used, at least 1 */
int HandleOption(int argc, char **argv)
{
  const char *opt = argv[0];

  // GNU style options begin with two dashes
  if (opt[0] == '-' && opt[1] == '-')
    opt++;

  if (StringCaseCmp(opt, "-o") == 0 || StringCaseCmp(opt, "-output") == 0)
  {
    if (argc <= 1 || argv[1][0] == '-')
      FatalError("Missing output filename after %s\n", argv[0]);

    output_name = std::string(argv[1]);

    program_action = ACT_Create;
    return 2;
  }

  if (StringCaseCmp(opt, "-l") == 0 || StringCaseCmp(opt, "-list") == 0)
  {
    program_action = ACT_List;
    return 1;
  }

  if (StringCaseCmp(opt, "-e") == 0 || StringCaseCmp(opt, "-extract") == 0)
  {
    program_action = ACT_Extract;
    return 1;
  }

  if (StringCaseCmp(opt, "-r") == 0 || StringCaseCmp(opt, "-recursive") == 0)
  {
    opt_recursive = true;
    return 1;
  }

  // no short version because this option is dangerous
  if (StringCaseCmp(opt, "-overwrite") == 0)
  {
    opt_overwrite = true;
    return 1;
  }

  FatalError("Unknown option: %s\n", argv[0]);
  return 1; // NOT REACHED
}


void AddInputFile(const char *filename)
{
  input_names.push_back(std::string(filename));
}



void Main_Create(void)
{
  const char *filename = output_name.c_str();

  if (CheckExtension(filename, "wad"))
    MIP_CreateWAD(filename);
  else if (CheckExtension(filename, "pak"))
    ARC_CreatePAK(filename);
  else
    FatalError("Unknown output file format: ^s\n", output_name.c_str());
}


void Main_List(void)
{
  if (input_names.size() == 0)
    FatalError("Missing input file to list\n");

  if (input_names.size() > 1)
    FatalError("Can only list one input file\n");

  const char *filename = input_names[0].c_str();

  if (CheckExtension(filename, "pak"))
  {
    // FIXME: check for error !!!!!!!
    PAK_OpenRead(filename);
    PAK_ListEntries();
    PAK_CloseRead();
  }
  else if (CheckExtension(filename, "wad"))
  {
    // FIXME: check for error !!!!!!!
    WAD2_OpenRead(filename);
    WAD2_ListEntries();
    WAD2_CloseRead();
  }
  else
    FatalError("Unknown input file format: ^s\n", filename);
}


void Main_Extract(void)
{
  if (input_names.size() == 0)
    FatalError("Missing input file to extract\n");

  if (input_names.size() > 1)
    FatalError("Can only extract one input file\n");

  const char *filename = input_names[0].c_str();

  if (CheckExtension(filename, "wad"))
    MIP_ExtractWAD(filename);
  else if (CheckExtension(filename, "pak"))
    ARC_ExtractPAK(filename);
  else
    FatalError("Unknown input file format: ^s\n", filename);
}


int main(int argc, char **argv)
{
  // skip program name itself
  argv++, argc--;

  if (argc <= 0 ||
      StringCaseCmp(argv[0], "/?") == 0 ||
      StringCaseCmp(argv[0], "-h") == 0 ||
      StringCaseCmp(argv[0], "-help") == 0 ||
      StringCaseCmp(argv[0], "--help") == 0)
  {
    ShowTitle();
    ShowUsage();
    exit(1);
  }

  ShowTitle();

  // handle command-line arguments
  while (argc > 0)
  {
    if (argv[0][0] == '-')
    {
      int num = HandleOption(argc, argv);

      argv += num;
      argc -= num;

      continue;
    }

    AddInputFile(argv[0]);

    argv++;
    argc--;
  }

  switch (program_action)
  {
    case ACT_Create:
      Main_Create();
      break;

    case ACT_List:
      Main_List();
      break;

    case ACT_Extract:
      Main_Extract();
      break;

    default:
      FatalError("Nothing to do (missing output file?)\n");
      break; // NOT REACHED
  }

  return 0;
}

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
