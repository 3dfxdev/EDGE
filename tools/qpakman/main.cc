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
#include "main.h"

#include <time.h>

#include "archive.h"
#include "im_color.h"
#include "im_mip.h"
#include "pakfile.h"


#define VERSION  "0.60"


std::string output_name;

std::vector<std::string> input_names;

std::string color_name;

typedef enum
{
  ACT_None = 0,

  ACT_Create,
  ACT_List,
  ACT_Extract,
  ACT_MakeTex
}
prog_action_type_e;

static prog_action_type_e program_action = ACT_None;


game_kind_e game_type = GAME_Quake1;

bool opt_force   = false;
bool opt_raw     = false;
bool opt_picture = false;


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
  printf("   -l  -list        list contents of PAK/WAD file\n");
  printf("   -e  -extract     extract PAK/WAD contents into current dir\n");
  printf("   -m  -maketex     make a texture WAD from BSP files\n");
  printf("\n");

  printf("   -c  -colors XXX  load color palette from given file\n");
  printf("   -g  -game   XXX  select game (quake1, quake2, hexen2)\n");
  printf("   -f  -force       overwrite existing files when extracting\n");
  printf("   -p  -pic         create PIC format images in the WAD\n");
  printf("   -r  -raw         do not convert anything\n");
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

  if (StringCaseCmp(opt, "-m") == 0 || StringCaseCmp(opt, "-maketex") == 0)
  {
    program_action = ACT_MakeTex;
    return 1;
  }


  if (StringCaseCmp(opt, "-c") == 0 || StringCaseCmp(opt, "-color") == 0 ||
      StringCaseCmp(opt, "-colors") == 0)
  {
    if (argc <= 1 || argv[1][0] == '-')
      FatalError("Missing filename after %s\n", argv[0]);

    color_name = std::string(argv[1]);
    return 2;
  }

  if (StringCaseCmp(opt, "-f") == 0 || StringCaseCmp(opt, "-force") == 0 ||
      StringCaseCmp(opt, "-overwrite") == 0)
  {
    opt_force = true;
    return 1;
  }

  if (StringCaseCmp(opt, "-g") == 0 || StringCaseCmp(opt, "-game") == 0)
  {
    if (argc <= 1 || argv[1][0] == '-')
      FatalError("Missing keyword after %s\n", argv[0]);

    if (StringCaseCmp(argv[1], "q1") == 0 ||
        StringCaseCmp(argv[1], "quake") == 0 ||
        StringCaseCmp(argv[1], "quake1") == 0)
    {
      game_type = GAME_Quake1;
    }
    else if (StringCaseCmp(argv[1], "q2") == 0 ||
             StringCaseCmp(argv[1], "quake2") == 0)
    {
      game_type = GAME_Quake2;
    }
    else if (StringCaseCmp(argv[1], "h2") == 0 ||
             StringCaseCmp(argv[1], "hexen2") == 0)
    {
      game_type = GAME_Hexen2;
    }
    else
      FatalError("Unknown game type: %s\n", argv[1]);

    return 2;
  }

  if (StringCaseCmp(opt, "-p") == 0 || StringCaseCmp(opt, "-pic") == 0)
  {
    opt_picture = true;
    return 1;
  }

  if (StringCaseCmp(opt, "-r") == 0 || StringCaseCmp(opt, "-raw") == 0)
  {
    opt_raw = true;
    return 1;
  }

  FatalError("Unknown option: %s\n", argv[0]);
  return 0; // NOT REACHED
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
    FatalError("Unknown output file format: %s\n", output_name.c_str());
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
    if (! PAK_OpenRead(filename))
      FatalError("Could not open PAK file: %s\n", filename);

    PAK_ListEntries();
    PAK_CloseRead();
  }
  else if (CheckExtension(filename, "wad"))
  {
    if (! WAD2_OpenRead(filename))
      FatalError("Could not open WAD2 file: %s\n", filename);

    WAD2_ListEntries();
    WAD2_CloseRead();
  }
  else
    FatalError("Unknown input file format: %s\n", filename);
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
    FatalError("Unknown input file format: %s\n", filename);
}


void Main_MakeTex(void)
{
  // TODO
    FatalError("MakeTex not implemented!\n");
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

  COL_SetPalette(game_type);

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

    case ACT_MakeTex:
      Main_MakeTex();
      break;

    default:
      FatalError("Nothing to do (missing output file?)\n");
      break; // NOT REACHED
  }

  return 0;
}

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
