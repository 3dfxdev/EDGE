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

#include "im_mip.h"
#include "pakfile.h"


std::string output_name;

std::vector<std::string> input_names;


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
  printf("**** QPAKMAN v0.07  (C) 2008 Andrew Apted ****\n");
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
  if (StringCaseCmp(argv[0], "-o") == 0 ||
      StringCaseCmp(argv[0], "-output")  == 0 ||
      StringCaseCmp(argv[0], "--output") == 0)
  {
    if (argc <= 1 || argv[1][0] == '-')
      FatalError("Missing output filename after %s\n", argv[0]);

    output_name = std::string(argv[1]);

    return 2;
  }

  FatalError("Unknown option: %s\n", argv[0]);
  return 1; // NOT REACHED
}


void HandleFile(const char *filename)
{
  input_names.push_back(std::string(filename));
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

    HandleFile(argv[0]);

    argv++;
    argc--;
  }

  // validate stuff
  if (strlen(output_name.c_str()) == 0)
    FatalError("No output file was specified (use: -o filename)\n");

  if (input_names.size() == 0)
    FatalError("No input images were specified!\n");


  // now make the output WAD2 file!
  if (! WAD2_OpenWrite(output_name.c_str()))
    FatalError("Cannot create WAD2 file: %s\n", output_name.c_str());

  printf("\n");
  printf("--------------------------------------------------\n");

  int failures = 0;

  for (unsigned int j = 0; j < input_names.size(); j++)
  {
    printf("Processing %d/%d: %s\n", 1+(int)j, (int)input_names.size(),
           input_names[j].c_str());

    if (! MIP_ProcessImage(input_names[j].c_str()))
      failures++;

    printf("\n");
  }

  printf("--------------------------------------------------\n");

  WAD2_CloseWrite();

  printf("Mipped %d images, with %d failures\n",
         (int)input_names.size() - failures, failures);

  return 0;
}

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
