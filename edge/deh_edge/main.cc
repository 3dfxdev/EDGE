//------------------------------------------------------------------------
//  MAIN Program
//------------------------------------------------------------------------
//
//  DEH_EDGE  Copyright (C) 2004  The EDGE Team
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License (in COPYING.txt) for more details.
//
//------------------------------------------------------------------------
//
//  DEH_EDGE is based on:
//
//  +  DeHackEd source code, by Greg Lewis.
//  -  DOOM source code (C) 1993-1996 id Software, Inc.
//  -  Linux DOOM Hack Editor, by Sam Lantinga.
//  -  PrBoom's DEH/BEX code, by Ty Halderman, TeamTNT.
//
//------------------------------------------------------------------------

#include "i_defs.h"

#include "ammo.h"
#include "attacks.h"
#include "frames.h"
#include "info.h"
#include "patch.h"
#include "rscript.h"
#include "sounds.h"
#include "storage.h"
#include "system.h"
#include "things.h"
#include "text.h"
#include "util.h"
#include "wad.h"
#include "weapons.h"


#define MAX_INPUTS  32


const char *input_files[MAX_INPUTS];
const char *output_file = NULL;

int num_inputs = 0;

int target_version = 128;  // EDGE 1.28

bool quiet_mode = false;
bool all_mode   = false;


/* ----- user information ----------------------------- */

static void ShowTitle(void)
{
  PrintMsg(
    "\n"
	"=================================================\n"
    "|    DeHackEd -> EDGE Conversion Tool  V" VERSION "     |\n"
	"|                                               |\n"
	"|  The EDGE Team.  http://edge.sourceforge.net  |\n"
	"=================================================\n"
	"\n"
  );
}

static void ShowInfo(void)
{
  PrintMsg(
    "USAGE:  deh_edge  (Options...)  input.deh (...)  (-o output.wad)\n"
	"\n"
	"Available options:\n"
	"   -o --output      Output filename override.\n"
	"   -e --edge #.##   Specify EDGE version to target.\n"
	"   -q --quiet       Quiet mode, suppress warnings.\n"
	"   -a --all         Convert all DDF (much bigger !).\n"
	"\n"
  );
}


/* ----- main program ----------------------------- */

static void ParseArgs(int argc, char **argv)
{
	while (argc > 0)
	{
		const char *opt = *argv;

		argv++, argc--;

		// input filename ?
		if (*opt != '-')
		{
			if (num_inputs >= MAX_INPUTS)
				FatalError("Too many input files !!\n");

			input_files[num_inputs++] = StringDup(opt);
			continue;
		}

		if (opt[1] == '-') opt++;

		if (! opt[1])
			FatalError("Illegal option %s\n", opt);

		// output filename ?
		if (StrCaseCmp(opt, "-o") == 0 || StrCaseCmp(opt, "-output") == 0)
		{
			if (argc == 0 || argv[0][0] == '-')
				FatalError("Missing output filename !\n");

			if (output_file)
				FatalError("Output file already given.\n");

			output_file = StringDup(*argv);

			argv++, argc--;
			continue;
		}

		// version number
		if (StrCaseCmp(opt, "-e") == 0 || StrCaseCmp(opt, "-edge") == 0)
		{
			const char *ver = argv[0];

			if (argc == 0 || ver[0] == '-')
				FatalError("Missing version number !\n");

			if (isdigit(ver[0]) && ver[1] == '.' &&
				isdigit(ver[2]) && isdigit(ver[3]))
			{
				target_version = (ver[0] - '0') * 100 +
					(ver[2] - '0') * 10 + (ver[3] - '0');
			}
			else
				FatalError("Badly formed version number.\n");

			argv++, argc--;
			continue;
		}

		if (StrCaseCmp(opt, "-q") == 0 || StrCaseCmp(opt, "-quiet") == 0)
		{
			quiet_mode = true;
			continue;
		}
		if (StrCaseCmp(opt, "-a") == 0 || StrCaseCmp(opt, "-all") == 0)
		{
			all_mode = true;
			continue;
		}

		FatalError("Unknown option: %s\n", opt);
	}
}

static void ValidateArgs(void)
{
	int j;

	if (num_inputs == 0)
		FatalError("Missing input filename !\n");

	if (target_version < 123 || target_version >= 300)
		FatalError("Illegal version number: %d.%02d\n", target_version / 100,
			target_version % 100);

	for (j = 0; j < num_inputs; j++)
	{
		if (strlen(ReplaceExtension(input_files[j], NULL)) == 0)
			FatalError("Illegal input filename: %s\n", input_files[j]);

		if (CheckExtension(input_files[j], "wad") ||
		    CheckExtension(input_files[j], "hwa"))
			FatalError("Input filename cannot be a WAD file.\n");
	}

	if (! output_file)
	{
		// default output filename, add ".hwa" or "_deh.wad" to base

		const char *base_name = ReplaceExtension(input_files[0], NULL);
		
		char *new_file = StringNew(strlen(base_name) + 16);

		strcpy(new_file, base_name);
		strcat(new_file, (target_version >= 128) ? ".hwa" : "_deh.wad");

		output_file = new_file;
	}

	if (CheckExtension(output_file, "deh") ||
	    CheckExtension(output_file, "bex"))
		FatalError("Output filename cannot be a DEH file.\n");

	for (j = 0; j < num_inputs; j++)
	{
		if (StrCaseCmp(input_files[j], output_file) == 0)
			FatalError("Output filename is same as input filename.\n");
		
		if (CheckExtension(input_files[j], NULL) &&
			! FileExists(input_files[j]))
		{
			// FIXME: for BEX support, check for BEX here

			input_files[j] = StringDup(ReplaceExtension(input_files[j], "deh"));
		}
	}

	if (CheckExtension(output_file, NULL))
	{
		output_file = StringDup(ReplaceExtension(output_file, "wad"));
	}
}

int main(int argc, char **argv)
{
	System_Startup();

	ShowTitle();
	
	// skip program name itself
	argv++, argc--;

	if (argc <= 0)
	{
		ShowInfo();
		System_Shutdown();
		exit(1);
	}

	if (StrCaseCmp(argv[0], "/?") == 0 || StrCaseCmp(argv[0], "-h") == 0 ||
		StrCaseCmp(argv[0], "-help") == 0 || StrCaseCmp(argv[0], "--help") == 0)
	{
		ShowInfo();
		System_Shutdown();
		exit(1);
	}

	Frames::Startup();
	Things::Startup();
	Weapons::Startup();

	Storage::Startup();
	WAD::Startup();

	ParseArgs(argc, argv);
	ValidateArgs();

	// load DEH patch file(s)
	for (int j = 0; j < num_inputs; j++)
		Patch::Load(input_files[j]);

	Storage::ApplyAll();

	// do conversions into DDF...
	PrintMsg("Converting data into EDGE %d.%02d DDF...\n",
		target_version / 100, target_version % 100);

	TextStr::SpriteDependencies();
	Frames::StateDependencies();
	Ammo::AmmoDependencies();

	Things::ConvertTHING();
	Attacks::ConvertATK();
	Weapons::ConvertWEAP();
	Sounds::ConvertSFX();
	Sounds::ConvertMUS();
	TextStr::ConvertLDF();
	Rscript::ConvertRAD();

	PrintMsg("\n");
	PrintMsg("Writing WAD file: %s\n", output_file);

	WAD::WriteFile(output_file);
	PrintMsg("\n");

	WAD::Shutdown();
	System_Shutdown();

	return 0;
}

