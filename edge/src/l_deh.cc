//----------------------------------------------------------------------------
//  EDGE DEH Interface
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
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

#include "dm_type.h"
#include "e_main.h"
#include "hu_lib.h"
#include "m_menu.h"
#include "v_ctx.h"
#include "v_colour.h"
#include "v_res.h"
#include "version.h"
#include "w_image.h"
#include "w_wad.h"

#include "dehedge-1.2/dh_plugin.h"

bool dh_draw_progress = false;

static bool dh_refresh;
static int dh_ticker_time;

static char dh_msg_buf[1024];

// time between redrawing screen (bit under 1/11 second)
#define REDRAW_TICS  3

//
// DH_PrintMsg
//
void DH_PrintMsg(const char *str, ...)
{
	va_list args;

	va_start(args, str);
	vsprintf(dh_msg_buf, str, args);
	va_end(args);

	I_Printf("DEH_EDGE: %s", dh_msg_buf);
}

//
// DH_FatalError
//
// Terminates the program reporting an error.
//
void DH_FatalError(const char *str, ...)
{
	va_list args;

	va_start(args, str);
	vsprintf(dh_msg_buf, str, args);
	va_end(args);

	I_Error("Converting DEH patch failed: %s\n", dh_msg_buf);
}

//
// DH_Ticker
//
void DH_Ticker(void)
{
	int cur_time;

	if (! e_display_OK)
		return;

	cur_time = I_GetTime();

	if (dh_ticker_time != 0 && cur_time < dh_ticker_time + REDRAW_TICS)
		return;

	///!!! FIXME E_Display();

	// Note: get the time _after_ rendering the frame.  This handles the
	// situation of very low framerates better, as we can at least
	// perform REDRAW_TICS amount of work for each frame.

	dh_ticker_time = I_GetTime();
}

//
// DH_ProgressText
//
void DH_ProgressText(const char *str)
{
	// FIXME: remember text, draw it

	// only need to refresh on text changes (not on bar changes)
	dh_refresh = true;
}

//
// DH_ProgressBar
//
void DH_ProgressBar(int percentage)
{
	// FIXME: do progress bar stuff
}

const dehconvfuncs_t edge_dehconv_funcs =
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
		10 * ((EDGEVER / 0x10) % 0x10) +
	    100 * (EDGEVER / 0x100);

	dh_refresh = true;

	dehret_e ret;

	DehEdgeStartup(&edge_dehconv_funcs);
	DehEdgeSetVersion(deci_ver);

	ret = DehEdgeAddFile(filename);

	if (ret == DEH_OK)
		ret = DehEdgeRunConversion(outname);

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
		10 * ((EDGEVER / 0x10) % 0x10) +
	    100 * (EDGEVER / 0x100);

	dh_refresh = true;

	dehret_e ret;

	DehEdgeStartup(&edge_dehconv_funcs);
	DehEdgeSetVersion(deci_ver);

	ret = DehEdgeAddLump((const char *)data, length, info_name);

	if (ret == DEH_OK)
		ret = DehEdgeRunConversion(outname);

	DehEdgeShutdown();

	return (ret == DEH_OK);
}

//
// DH_DrawProgress
//
void DH_DrawProgress(void)
{
	// NOT YET IMPLEMENTED
}
