//----------------------------------------------------------------------------
//  EDGE<->GLBSP Bridging code
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
#include "l_glbsp.h"

#include "e_main.h"
#include "version.h"

#include "glbsp/glbsp.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define PROGRESS_STEP  2  // percent

static void GB_InitProgress(void);
static void GB_TermProgress(void);

static char message_buf[1024];

/// #define MESSAGE1  "EDGE IS NOW CREATING THE GWA FILE..."
/// #define MESSAGE2  "THIS ONLY HAS TO BE DONE ONCE FOR THIS WAD"

static char wad_name_buf[256];


static int display_mode = DIS_INVALID;

static int build_progress = 0;
static int build_limit = 0;
static int build_perc = -999;

//
// GB_PrintMsg
//
void GB_PrintMsg(const char *str, ...)
{
	va_list args;

	va_start(args, str);
	vsprintf(message_buf, str, args);
	va_end(args);

	I_Printf("GLBSP: %s", message_buf);
}

//
// GB_FatalError
//
// Terminates the program reporting an error.
//
void GB_FatalError(const char *str, ...)
{
	va_list args;

	va_start(args, str);
	vsprintf(message_buf, str, args);
	va_end(args);

	I_Error("Builing nodes failed: %s\n", message_buf);
}

//
// GB_Ticker
//
void GB_Ticker(void)
{
	// Maybe do something here, e.g. check for 'ESCAPE' key press
}

//
// GB_DisplayOpen
//
boolean_g GB_DisplayOpen(displaytype_e type)
{
	display_mode = type;

	build_progress = 0;
	build_limit = 0;
	build_perc = -999;

	return TRUE;
}

//
// GB_DisplaySetTitle
//
void GB_DisplaySetTitle(const char *str)
{
	/* does nothing */
}

//
// GB_DisplaySetText
//
void GB_DisplaySetText(const char *str)
{
	/* does nothing */
}

//
// GB_DisplaySetBarText
//
void GB_DisplaySetBarText(int barnum, const char *str)
{
	if (display_mode != DIS_BUILDPROGRESS)
	{
		E_NodeMessage(str);
		return;
	}

	if (barnum == 1)
	{
		sprintf(message_buf, "%s: %s\n", wad_name_buf, str);

		E_NodeMessage(message_buf);
	}
	else
	{
		// determine wad filename

		const char *pos = str + strlen(str) - 1;

		for (; pos > str; pos--)
		{
			char ch = pos[-1];

			if (ch == '/' || ch == '\\' || ch == ':')
				break;
		}

		strcpy(wad_name_buf, (pos > str) ? pos : str);
	}
}

//
// GB_DisplaySetBarLimit
//
void GB_DisplaySetBarLimit(int barnum, int limit)
{
	DEV_ASSERT2(1 <= barnum && barnum <= 2);

	if (display_mode == DIS_BUILDPROGRESS && barnum == 2)
	{
		build_limit = limit;
		build_progress = 0;
		build_perc = 0;
	}
}

//
// GB_DisplaySetBar
//
void GB_DisplaySetBar(int barnum, int count)
{
	DEV_ASSERT2(1 <= barnum && barnum <= 2);

	if (display_mode == DIS_BUILDPROGRESS && barnum == 2)
	{
		if (count < 0 || count > build_limit || build_limit <= 0)
			return;

		build_progress = count;

		int perc = build_progress * 100 / build_limit;

		if (perc >= build_perc + PROGRESS_STEP)
		{
			build_perc = perc;
			E_NodeProgress(build_perc);
		}
	}
}

//
// GB_DisplayClose
//
void GB_DisplayClose(void)
{
	/* does nothing */
}

const nodebuildfuncs_t edge_build_funcs =
{
	GB_FatalError,
	GB_PrintMsg,
	GB_Ticker,

	GB_DisplayOpen,
	GB_DisplaySetTitle,
	GB_DisplaySetBar,
	GB_DisplaySetBarLimit,
	GB_DisplaySetBarText,
	GB_DisplayClose
};


//
// GB_BuildNodes
//
// Attempt to build nodes for the WAD file containing the given
// WAD file.  Returns true if successful, otherwise false.
//
bool GB_BuildNodes(const char *filename, const char *outname)
{
	nodebuildinfo_t nb_info;
	volatile nodebuildcomms_t nb_comms;

	nb_info = default_buildinfo;

	// nb_comms = default_buildcomms;
	memcpy((void *)&nb_comms, (void *)&default_buildcomms, sizeof(nodebuildcomms_t));

	nb_info.input_file  = GlbspStrDup(filename);
	nb_info.output_file = GlbspStrDup(outname);
	nb_info.quiet = true;

	// FIXME: user-controllable build options (factor, fresh, etc).

	if (GLBSP_E_OK != GlbspCheckInfo(&nb_info, &nb_comms))
	{
		GB_PrintMsg("Param Check FAILED:\n");
		GB_PrintMsg("- %s\n", nb_comms.message);

		return false;
	}

	GB_InitProgress();

	glbsp_ret_e ret = GlbspBuildNodes(&nb_info, &edge_build_funcs, &nb_comms);

	GB_TermProgress();

	if (ret != GLBSP_E_OK)
	{
		GB_PrintMsg("Building FAILED:\n");
		GB_PrintMsg("- %s\n\n", nb_comms.message);

		return false;
	}

	DEV_ASSERT2(nb_info.gwa_mode);

	return true;
}

void GB_InitProgress(void)
{
	display_mode = DIS_INVALID;
	
	wad_name_buf[0] = 0;
}

void GB_TermProgress(void)
{
}

