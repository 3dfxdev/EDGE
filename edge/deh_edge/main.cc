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
#include "sounds.h"
#include "system.h"
#include "things.h"
#include "text.h"
#include "util.h"
#include "wad.h"
#include "weapons.h"


//  DEH 1.2 (patch v1) no frames
//  DEH 1.3 (patch v2) frames, no code pointers
//  DEH X.X (patch vX) code pointers
//  DEH Y.Y (patch vY) text format
//  DEH 3.0 has ^Misc section


const char *input_file;
const char *output_file;


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
	"    -a  --all       Convert all (even if unmodified) into DDF.\n"
	"\n"
  );
}


/* ----- main program ----------------------------- */

static void ParseArgs(int argc, char **argv)
{
	assert(argc >= 1);

	input_file = strdup(*argv);

	argv++, argc--;

	if (argc >= 1)
	{
		output_file = strdup(*argv);

		argv++, argc--;
	}
	else
	{
		output_file = strdup("zz_deh.wad");  //!!!! FIXME
	}

	// !!!! AddMissingExtension(&input_file,  ".deh");
	// !!!! AddMissingExtension(&output_file, ".wad");

	if (StrCaseCmp(input_file, output_file) == 0)
		FatalError("Output filename is same as input filename.\n");

	if (argc > 0)
		FatalError("Too many filenames.\n");
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

	if (strcmp(argv[0], "/?") == 0 || strcmp(argv[0], "-h") == 0 ||
			strcmp(argv[0], "-help") == 0 || strcmp(argv[0], "--help") == 0 ||
			strcmp(argv[0], "-HELP") == 0 || strcmp(argv[0], "--HELP") == 0)
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

	PrintMsg("Loading patch file: %s\n", input_file);

	// DO STUFF !!
		Things::ConvertTHING();
		Attacks::ConvertATK();
		Weapons::ConvertWEAP();
		Sounds::ConvertSFX();
		Sounds::ConvertMUS();
		TextStr::ConvertLDF();

	PrintMsg("Writing WAD file: %s\n", output_file);
	WAD::WriteFile(output_file);

	PrintMsg("Finished.\n\n");

	WAD::Shutdown();
	System_Shutdown();

	return 0;
}

