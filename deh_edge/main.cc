//------------------------------------------------------------------------
// DEH_EDGE Main Program
//------------------------------------------------------------------------
//
//  GL-Friendly Node Builder (C) 2000-2004 Andrew Apted
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

#include "i_defs.h"

#include "attacks.h"
#include "system.h"
#include "things.h"
#include "wad.h"


//  DEH 1.2 (patch v1) no frames
//  DEH 1.3 (patch v2) frames, no code pointers
//  DEH X.X (patch vX) code pointers
//  DEH Y.Y (patch vY) text format


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
    "USAGE:  deh_edge  (Options...)  input.deh  (output.wad)\n"
	"\n"
	"Currently there are no options.\n"
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
		output_file = strdup("bar_deh.wad");  //!!!! FIXME
	}

	// !!!! AddMissingExtension(&input_file,  ".deh");
	// !!!! AddMissingExtension(&output_file, ".wad");

	// !!!! if (StrCaseCmp(input_file, output_file) == 0) FatalError

	if (argc > 0)
		FatalError("Too many filenames.");
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

	memset(state_dyn, 0, sizeof(state_dyn));
	memset(mobj_dyn,  0, sizeof(mobj_dyn));

	WAD::Startup();

	ParseArgs(argc, argv);

	PrintMsg("Loading patch file: %s\n", input_file);

	// DO STUFF !!
		Things::ConvertAll();
		Attacks::ConvertAll();

	PrintMsg("Writing WAD file: %s\n", output_file);
	WAD::WriteFile(output_file);

	PrintMsg("Finished.\n\n");

	WAD::Shutdown();
	System_Shutdown();

	return 0;
}

