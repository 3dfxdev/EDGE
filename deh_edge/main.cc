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
#include "buffer.h"
#include "dh_embed.h"
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


namespace Deh_Edge
{

class input_buffer_c
{
public:
	parse_buffer_api *buf;
	const char *infoname;
	bool is_lump;

	input_buffer_c(parse_buffer_api *_buf, const char *_info, bool _lump) :
		buf(_buf), is_lump(_lump)
	{
		infoname = StringDup(_info);
	}

	~input_buffer_c()
	{
		delete buf;
		buf = NULL;

		free((void*)infoname); 
	}
};

#define MAX_INPUTS  32

input_buffer_c *input_bufs[MAX_INPUTS];
int num_inputs = 0;

const char *output_file = NULL;

#define DEFAULT_TARGET  129

int target_version;
bool quiet_mode;
bool all_mode;

const dehconvfuncs_t *cur_funcs = NULL;


/* ----- user information ----------------------------- */

#ifndef DEH_EDGE_PLUGIN

void ShowTitle(void)
{
  PrintMsg(
    "\n"
	"=================================================\n"
    "|    DeHackEd -> EDGE Conversion Tool  V" DEH_EDGE_VERS "     |\n"
	"|                                               |\n"
	"|  The EDGE Team.  http://edge.sourceforge.net  |\n"
	"=================================================\n"
	"\n"
  );
}

void ShowInfo(void)
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

#endif

void Startup(void)
{
	System_Startup();

	Frames::Startup();
	Things::Startup();
	Weapons::Startup();

	Storage::Startup();
	WAD::Startup();

	/* reset parameters */

	num_inputs = 0;
	output_file = NULL;

	target_version = DEFAULT_TARGET;
	quiet_mode = false;
	all_mode = false;
}

void AddFile(const char *filename)
{
	if (num_inputs >= MAX_INPUTS)
		FatalError("Too many input files !!\n");

	if (strlen(ReplaceExtension(filename, NULL)) == 0)
		FatalError("Illegal input filename: %s\n", filename);

	if (CheckExtension(filename, "wad") || CheckExtension(filename, "hwa"))
		FatalError("Input filename cannot be a WAD file.\n");

	if (CheckExtension(filename, NULL))
	{
		// memory management here is "optimised" (i.e. a bit dodgy),
		// since the result of ReplaceExtension() is an internal static
		// buffer.

		const char *bex_name = ReplaceExtension(filename, "bex");

		if (FileExists(bex_name))
		{
			input_bufs[num_inputs++] = new input_buffer_c(
				Buffer::OpenFile(bex_name), FileBaseName(bex_name), false);
			return;
		}

		const char *deh_name = ReplaceExtension(filename, "deh");

		if (FileExists(deh_name))
		{
			input_bufs[num_inputs++] = new input_buffer_c(
				Buffer::OpenFile(deh_name), FileBaseName(deh_name), false);
			return;
		}
	}

	input_bufs[num_inputs++] = new input_buffer_c(
		Buffer::OpenFile(filename), FileBaseName(filename), false);
}

void AddLump(const char *data, int length, const char *infoname)
{
	if (num_inputs >= MAX_INPUTS)
		FatalError("Too many input lumps !!\n");

	input_bufs[num_inputs++] = new input_buffer_c(
		Buffer::OpenLump(data, length), infoname, true);
}

void FreeBuffers(void)
{
	for (int j = 0; j < num_inputs; j++)
	{
		if (input_bufs[j])
		{
			delete input_bufs[j];
			input_bufs[j] = NULL;
		}
	}
}

void Convert(void)
{
	// load DEH patch file(s)
	for (int j = 0; j < num_inputs; j++)
	{
		char temp_text[256];
		sprintf(temp_text, "Parsing %s", input_bufs[j]->infoname);

		ProgressText(temp_text);
		ProgressMajor(j * 70 / num_inputs, (j+1) * 70 / num_inputs);

		PrintMsg("Loading patch file: %s\n", input_bufs[j]->infoname);

		Patch::Load(input_bufs[j]->buf);
	}

	FreeBuffers();

	ProgressText("Converting DEH");
	ProgressMajor(70, 80);

	Storage::ApplyAll();
	ProgressMinor(1, 6);

	// do conversions into DDF...
	PrintMsg("Converting data into EDGE %d.%02d DDF...\n",
		target_version / 100, target_version % 100);

	TextStr::SpriteDependencies();
	Frames::StateDependencies();
	Ammo::AmmoDependencies();
	ProgressMinor(2, 6);

	Things::ConvertTHING();
	Attacks::ConvertATK();
	ProgressMinor(3, 6);

	Weapons::ConvertWEAP();
	Sounds::ConvertSFX();
	Sounds::ConvertMUS();
	ProgressMinor(4, 6);

	TextStr::ConvertLDF();
	Rscript::ConvertRAD();
	ProgressMinor(5, 6);

	PrintMsg("\n");
	PrintMsg("Writing WAD file: %s\n", output_file);

	ProgressText("Writing HWA file");
	ProgressMajor(80, 100);

	WAD::WriteFile(output_file);
	PrintMsg("\n");

	ProgressMajor(100, 100);
}

void Shutdown(void)
{
	WAD::Shutdown();
	System_Shutdown();
}


/* ----- main program ----------------------------- */

#ifndef DEH_EDGE_PLUGIN

void ParseArgs(int argc, char **argv)
{
	const char *first_input = NULL;

	while (argc > 0)
	{
		const char *opt = *argv;

		argv++, argc--;

		// input filename ?
		if (*opt != '-')
		{
			if (! first_input)
				first_input = opt;

			AddFile(opt);
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

	// choose output filename when not specified

	if (! output_file && first_input)
	{
		const char *base_name = ReplaceExtension(first_input, NULL);

		output_file = StringNew(strlen(base_name) + 16);

		strcpy((char *)output_file, base_name);
		strcat((char *)output_file, (target_version >= 128) ? ".hwa" : "_deh.wad");
	}
}

#endif  // DEH_EDGE_PLUGIN

void ValidateArgs(void)
{
	if (num_inputs == 0)
		FatalError("Missing input filename !\n");

	if (target_version < 123 || target_version >= 300)
		FatalError("Illegal version number: %d.%02d\n", target_version / 100,
			target_version % 100);

	if (! output_file)
		FatalError("Missing output filename !\n");

	if (CheckExtension(output_file, "deh") ||
	    CheckExtension(output_file, "bex"))
		FatalError("Output filename cannot be a DEH file.\n");

	if (CheckExtension(output_file, NULL))
	{
		output_file = StringDup(ReplaceExtension(output_file,
			(target_version >= 128) ? "hwa" : "wad"));
	}

///---	for (j = 0; j < num_inputs; j++)
///---	{
///---		if (input_bufs[j]->is_lump)
///---			continue;
///---
///---		if (StrCaseCmp(input_bufs[j]->infoname, output_file) == 0)
///---			FatalError("Output filename is same as input filename.\n");
///---	}
}

}  // Deh_Edge

//------------------------------------------------------------------------

#ifndef DEH_EDGE_PLUGIN

int main(int argc, char **argv)
{
	Deh_Edge::Startup();
	Deh_Edge::ShowTitle();
	
	// skip program name itself
	argv++, argc--;

	if (argc <= 0)
	{
		Deh_Edge::ShowInfo();
		Deh_Edge::System_Shutdown();

		exit(1);
	}

	if (Deh_Edge::StrCaseCmp(argv[0], "/?") == 0 ||
		Deh_Edge::StrCaseCmp(argv[0], "-h") == 0 ||
		Deh_Edge::StrCaseCmp(argv[0], "-help") == 0 ||
		Deh_Edge::StrCaseCmp(argv[0], "--help") == 0)
	{
		Deh_Edge::ShowInfo();
		Deh_Edge::System_Shutdown();

		exit(1);
	}

	Deh_Edge::ParseArgs(argc, argv);
	Deh_Edge::ValidateArgs();

	Deh_Edge::Convert();
	Deh_Edge::Shutdown();

	return 0;
}

#endif  // DEH_EDGE_PLUGIN

//------------------------------------------------------------------------

#ifdef DEH_EDGE_PLUGIN

dehret_e DehEdgeStartup(const dehconvfuncs_t *funcs)
{
	Deh_Edge::Startup();
	Deh_Edge::cur_funcs = funcs;

	Deh_Edge::PrintMsg("DeHackEd -> EDGE Conversion Tool V" DEH_EDGE_VERS "\n");

	return DEH_OK;
}

const char *DehEdgeGetError(void)
{
	return "(Unknown Error)"; //!!!! FIXME
}

dehret_e DehEdgeSetVersion(int version)
{
	Deh_Edge::target_version = version;  // validated later

	return DEH_OK;
}

dehret_e DehEdgeSetQuiet(int quiet)
{
	Deh_Edge::quiet_mode = (quiet != 0);

	return DEH_OK;
}

dehret_e DehEdgeAddFile(const char *filename)
{
	// FIXME: if not exists, return DEH_E_XXX
	Deh_Edge::AddFile(filename);

	return DEH_OK;
}

dehret_e DehEdgeAddLump(const char *data, int length, const char *infoname)
{
	Deh_Edge::AddLump(data, length, infoname);

	return DEH_OK;
}

dehret_e DehEdgeRunConversion(const char *out_name)
{
	// if (! out_name)
	//	return DEH_E_BadArgs;

	Deh_Edge::output_file = out_name;

	Deh_Edge::ValidateArgs();
	Deh_Edge::Convert();

	return DEH_OK;
}

dehret_e DehEdgeShutdown(void)
{
	Deh_Edge::Shutdown();
	Deh_Edge::cur_funcs = NULL;

	return DEH_OK;
}

#endif  // DEH_EDGE_PLUGIN
