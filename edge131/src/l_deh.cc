//----------------------------------------------------------------------------
//  EDGE DEH Interface
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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
//----------------------------------------------------------------------------
//
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#include "i_defs.h"
#include "l_deh.h"

#include "version.h"

#include "epi/epi.h"
#include "deh_edge/dh_plugin.h"

#include <stdio.h>
#include <stdarg.h>

static char dh_message[1024];

//
// DH_PrintMsg
//
static void GCCATTR((format (printf,1,2)))
	DH_PrintMsg(const char *str, ...)
{
	va_list args;

	va_start(args, str);
	vsprintf(dh_message, str, args);
	va_end(args);

	I_Printf("DEH_EDGE: %s", dh_message);
}

//
// DH_FatalError
//
// Terminates the program reporting an error.
//
static void GCCATTR((format (printf,1,2)))
	DH_FatalError(const char *str, ...)
{
	va_list args;

	va_start(args, str);
	vsprintf(dh_message, str, args);
	va_end(args);

	I_Error("Converting DEH patch failed: %s\n", dh_message);
}

//
// DH_ProgressText
//
static void DH_ProgressText(const char *str)
{
	/* nothing needed */
}

//
// DH_ProgressBar
//
static void DH_ProgressBar(int percentage)
{
	/* nothing needed */
}

static const dehconvfuncs_t edge_dehconv_funcs =
{
	DH_FatalError,
	DH_PrintMsg,
	DH_ProgressBar,
	DH_ProgressText
};


//
// DH_ConvertFile
//
bool DH_ConvertFile(const char *filename, const char *outname)
{
	int deci_ver = (EDGEVER % 0x10) +
				   ((EDGEVER / 0x10) % 0x10) * 10 +
				   (EDGEVER / 0x100) * 100;

	DehEdgeStartup(&edge_dehconv_funcs);
	DehEdgeSetVersion(deci_ver);

	dehret_e ret = DehEdgeAddFile(filename);

	if (ret != DEH_OK)
	{
		DH_PrintMsg("FAILED to add file:\n");
		DH_PrintMsg("- %s\n", DehEdgeGetError());
	}
	else
	{
		ret = DehEdgeRunConversion(outname);

		if (ret != DEH_OK)
		{
			DH_PrintMsg("CONVERSION FAILED:\n");
			DH_PrintMsg("- %s\n", DehEdgeGetError());
		}
	}

	DehEdgeShutdown();

	return (ret == DEH_OK);
}

//
// DH_ConvertLump
//
bool DH_ConvertLump(const byte *data, int length, const char *lumpname,
	const char *outname)
{
	char info_name[100];

	sprintf(info_name, "%s.LMP", lumpname);

	int deci_ver = (EDGEVER % 0x10) +
				   ((EDGEVER / 0x10) % 0x10) * 10 +
				   (EDGEVER / 0x100) * 100;

	DehEdgeStartup(&edge_dehconv_funcs);
	DehEdgeSetVersion(deci_ver);

	dehret_e ret = DehEdgeAddLump((const char *)data, length, info_name);

	if (ret != DEH_OK)
	{
		DH_PrintMsg("FAILED to add lump:\n");
		DH_PrintMsg("- %s\n", DehEdgeGetError());
	}
	else
	{
		ret = DehEdgeRunConversion(outname);

		if (ret != DEH_OK)
		{
			DH_PrintMsg("CONVERSION FAILED:\n");
			DH_PrintMsg("- %s\n", DehEdgeGetError());
		}
	}

	DehEdgeShutdown();

	return (ret == DEH_OK);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
