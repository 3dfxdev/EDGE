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

#include "attacks.h"
#include "frames.h"
#include "patch.h"
#include "sounds.h"
#include "system.h"
#include "things.h"
#include "text.h"
#include "util.h"
#include "wad.h"
#include "weapons.h"


const char *input_file  = NULL;
const char *output_file = NULL;

bool quiet_mode = false;
bool all_mode   = true;   // !!!!! FIXME

int edge_version = 127;


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
    "USAGE:  deh_edge  (Options...)  input.deh  (-o  output.wad)\n"
	"\n"
	"Available options.\n"
	"    -o  --output    Output filename override.\n"
	"    -q  --quiet     Quiet mode, suppress warnings.\n"
	"    -a  --all       Convert all (even unmodified) DDF.\n"
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
			if (input_file)
				FatalError("Too many filenames (use -o for output).\n");

			input_file = strdup(opt);
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

			output_file = strdup(*argv);

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
		// -v -version

		FatalError("Unknown option: %s\n", opt);
	}
}

static void ValidateArgs(void)
{
	if (! input_file)
		FatalError("Missing input filename !\n");

	if (strlen(ReplaceExtension(input_file, NULL)) == 0)
		FatalError("Illegal input filename: %s\n", input_file);

	if (CheckExtension(input_file, "wad"))
		FatalError("Input filename cannot be a WAD file.\n");

	if (! output_file)
	{
		// default output filename, add "_deh.wad" to base

		const char *base_name = ReplaceExtension(input_file, NULL);
		
		char *new_file = (char *) malloc(strlen(base_name) + 1 + 16); // FIXME
		assert(new_file);

		strcpy(new_file, base_name);
		strcat(new_file, "_deh.wad");

		output_file = new_file;
	}

	if (CheckExtension(output_file, "deh"))
		FatalError("Output filename cannot be a DEH file.\n");

	if (StrCaseCmp(input_file, output_file) == 0)
		FatalError("Output filename is same as input filename.\n");
	
	if (CheckExtension(input_file, NULL) && ! FileExists(input_file))
	{
		input_file = strdup(ReplaceExtension(input_file, "deh"));
	}

	if (CheckExtension(output_file, NULL))
	{
		output_file = strdup(ReplaceExtension(output_file, "wad"));
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

	WAD::Startup();

	ParseArgs(argc, argv);
	ValidateArgs();

	// load DEH patch file
	PrintMsg("Loading patch file: %s\n", input_file);

	Patch::Load(input_file);

	// do conversions into DDF...
	PrintMsg("Converting data into DDF...\n");

	Things::ConvertTHING();
	Attacks::ConvertATK();
	Weapons::ConvertWEAP();
	Sounds::ConvertSFX();
	Sounds::ConvertMUS();
	TextStr::ConvertLDF();

	PrintMsg("\n");
	PrintMsg("Writing WAD file: %s\n", output_file);

	WAD::WriteFile(output_file);

	PrintMsg("Finished.\n\n");

	WAD::Shutdown();
	System_Shutdown();

	return 0;
}

