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

#include "system.h"
#include "wad.h"


#define VERSION  "0.3"


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

	WAD_Startup();

	// PARSE ARGS

	// DO STUFF !!

	WAD_Shutdown();
	System_Shutdown();

	return 0;
}

