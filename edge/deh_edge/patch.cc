//------------------------------------------------------------------------
//  PATCH Loading
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
#include "patch.h"

#include "info.h"
#include "mobj.h"
#include "system.h"
#include "things.h"
#include "util.h"
#include "weapons.h"


namespace Patch
{
	FILE *pat_fp;

	int patch_fmt;   /* 1 to 5 */
	int dhe_ver;     /* 12, 13, 20-24, 30 */
	int doom_ver;    /* 12, 16 to 21 */

	void DetectMsg(const char *kind)
	{
		PrintMsg("Detected %s patch file from DEHACKED v%d.%d\n",
			kind, dhe_ver / 10, dhe_ver % 10);
	}

	void VersionMsg(void)
	{
		PrintMsg("Patch format %d, for DOOM EXE %d.%d%s\n",
			patch_fmt, doom_ver / 10, doom_ver % 10,
			(doom_ver == 16) ? "66" : "");
	}

	void LoadReallyOld(void)
	{
		char tempfmt = 0;

		fread(&tempfmt, 1, 1, pat_fp);

		if (tempfmt < 1 || tempfmt > 2)
			FatalError("Bad format byte in DeHackEd patch file.\n"
				"[Really old patch, format byte %d]\n", tempfmt);

		patch_fmt = (int)tempfmt;
		doom_ver = 12;
		dhe_ver = 11 + patch_fmt;

		DetectMsg("really old");
		VersionMsg();
	
		// FIXME...
	}

	void LoadBinary(void)
	{
		char tempdoom = 0;
		char tempfmt  = 0;

		fread(&tempdoom, 1, 1, pat_fp);
		fread(&tempfmt,  1, 1, pat_fp);

		if (tempfmt == 3)
			FatalError("Doom 1.6 beta patches are not supported.\n");
		else if (tempfmt != 4)
			FatalError("Bad format byte in DeHackEd patch file.\n"
				"[Binary patch, format byte %d]\n", tempfmt);

		patch_fmt = 4;

		if (tempdoom != 12 && ! (tempdoom >= 16 && tempdoom <= 21))
			FatalError("Bad Doom release number in patch file !\n"
				"[Binary patch, release number %d]\n", tempdoom);

		doom_ver = (int)tempdoom;

		DetectMsg("binary");
		VersionMsg();

		// FIXME
	}

	void LoadDiff(void)
	{
		DetectMsg("text-based");

		doom_ver = 16;   // defaults
		patch_fmt = 5;  //

		// FIXME

	}

	void LoadNormal(void)
	{
		char idstr[30];

		memset(idstr, 0, sizeof(idstr));

		fread(idstr, 1, 24, pat_fp);

		if (StrCaseCmp(idstr, "atch File for DeHackEd v") != 0)
			FatalError("File is not a DeHackEd patch file !\n");

		memset(idstr, 0, 4);

		fread(idstr, 1, 3, pat_fp);

		if (! isdigit(idstr[0]) || idstr[1] != '.' || ! isdigit(idstr[2]))
			FatalError("Bad version string in DeHackEd patch file.\n"
				"[String %s is not digit . digit]\n", idstr);

		dhe_ver = (idstr[0] - '0') * 10 + (idstr[2] - '0');

		if (dhe_ver < 20 || dhe_ver > 31)
			FatalError("This patch file has an incorrect version number !\n"
				"[Version %s]\n", idstr); 

		if (dhe_ver < 23)
			LoadBinary();
		else
			LoadDiff();
	}
}

void Patch::Load(const char *filename)
{
	// Note: always binary - we'll handle CR/LF ourselves
	pat_fp = fopen(filename, "rb");

	if (! pat_fp)
	{
		int err_num = errno;

		FatalError("Could not open file: %s\n[%s]\n", filename, strerror(err_num));
	}
	
	char tempver = 0;

	fread(&tempver, 1, 1, pat_fp);

	if (tempver == 12)
		LoadReallyOld();
	else if (tempver == 'P')
		LoadNormal();
	else	
		FatalError("File is not a DeHackEd patch file !\n");

	fclose(pat_fp);

	PrintMsg("\n");
}

