//----------------------------------------------------------------------------
//  EDGE<->GLBSP Bridging code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2003  The EDGE Team.
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

#include "dm_type.h"
#include "e_main.h"
#include "hu_lib.h"
#include "m_menu.h"
#include "v_ctx.h"
#include "v_colour.h"
#include "v_res.h"
#include "w_image.h"
#include "w_wad.h"
#include "z_zone.h"

#include "glbsp-2.05/glbsp.h"


bool gb_draw_progress = false;

static char message_buf[1024];
static int ticker_time;

// time between redrawing screen (bit under 1/11 second)
#define REDRAW_TICS  3

static void GB_InitProgress(void);
static void GB_TermProgress(void);

static const image_t *gb_background = NULL;


// -AJA- FIXME: put this in LANGUAGE.LDF sometime

#define MESSAGE1  "EDGE IS NOW CREATING THE GWA FILE..."
#define MESSAGE2  "THIS ONLY HAS TO BE DONE ONCE FOR THIS WAD"


#define MAXBARTEXT  100

typedef struct
{
	// current limit
	int limit;

	// current position (0.0 to 1.0)
	float pos;

	char text[MAXBARTEXT];
}
gb_bar_t;

static const gb_bar_t default_bar = { 0, 0.0f, { 0, }};

static gb_bar_t bars[2];
static displaytype_e cur_disp = DIS_INVALID;

static bool gb_refresh;


//
// GB_PrintMsg
//
void GB_PrintMsg(const char *str, ...)
{
	va_list args;

	va_start(args, str);
	vsprintf(message_buf, str, args);
	va_end(args);

	I_Printf("GB: %s", message_buf);
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
	int cur_time;

	if (! e_display_OK)
		return;

	cur_time = I_GetTime();

	if (ticker_time != 0 && cur_time < ticker_time + REDRAW_TICS)
		return;

	E_Display();

	// Note: get the time _after_ rendering the frame.  This handles the
	// situation of very low framerates better, as we can at least
	// perform REDRAW_TICS amount of work for each frame.

	ticker_time = I_GetTime();
}

//
// GB_DisplayOpen
//
boolean_g GB_DisplayOpen(displaytype_e type)
{
	cur_disp = type;

	bars[0] = default_bar;
	bars[1] = default_bar;

	gb_refresh = true;

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
	DEV_ASSERT2(1 <= barnum && barnum <= 2);

	Z_StrNCpy(bars[barnum - 1].text, str, MAXBARTEXT-1);

	// only need to refresh on text changes (not on bar changes)
	gb_refresh = true;
}

//
// GB_DisplaySetBarLimit
//
void GB_DisplaySetBarLimit(int barnum, int limit)
{
	DEV_ASSERT2(1 <= barnum && barnum <= 2);

	bars[barnum - 1].limit = limit;
}

//
// GB_DisplaySetBar
//
void GB_DisplaySetBar(int barnum, int count)
{
	DEV_ASSERT2(1 <= barnum && barnum <= 2);

	if (count < 0 || bars[barnum - 1].limit <= 0 ||
		count > bars[barnum - 1].limit)
	{
		return;
	}

	// compute fractional position
	bars[barnum - 1].pos = (float)count / bars[barnum - 1].limit;
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
// map_lump (a lump number from w_wad for the start marker, e.g.
// "MAP01").  Returns true if successful, false if it failed.
//
bool GB_BuildNodes(int map_lump)
{
	nodebuildinfo_t nb_info;
	volatile nodebuildcomms_t nb_comms;
	glbsp_ret_e ret;

	nb_info  = default_buildinfo;

	memcpy((void *)&nb_comms, (void *)&default_buildcomms, sizeof(nodebuildcomms_t));
	//  nb_comms = default_buildcomms;

  nb_info.input_file = GlbspStrDup(W_GetFileName(map_lump));

	// FIXME: check parm "-node-factor"

	if (GLBSP_E_OK != GlbspCheckInfo(&nb_info, &nb_comms))
		return false;

	GB_InitProgress();

	ret = GlbspBuildNodes(&nb_info, &edge_build_funcs, &nb_comms);

	GB_TermProgress();

	if (ret != GLBSP_E_OK)
		return false;

	DEV_ASSERT2(nb_info.output_file);
	DEV_ASSERT2(nb_info.gwa_mode);

	W_AddDynamicGWA(nb_info.output_file, map_lump);

	return true;
}

//
// GB_InitProgress
//
void GB_InitProgress(void)
{
	ticker_time = 0;
	cur_disp = DIS_INVALID;
	gb_draw_progress = true;
	gb_refresh = true;

	if (! e_display_OK)
		return;

	//...
}

//
// GB_TermProgress
//
void GB_TermProgress(void)
{
	gb_draw_progress = false;

	if (! e_display_OK)
		return;

	//...
}

//
// GB_DrawProgress
//
void GB_DrawProgress(void)
{
	int i;

	// -AJA- funky maths for "scaled resolution" thing
	int lx = 32;
	int rx = 240;

	int x1 = (int)(X_OFFSET + lx * DX);
	int x2 = (int)(X_OFFSET + rx * DX);

	int num_bars, dy;
	int filecol, nodecol;

	DEV_ASSERT2(e_display_OK);

	if (! gb_background)
		gb_background = W_ImageFromFlat("FLAT10");

	if (gb_refresh)
	{
		vctx.DrawImage(0, 0, SCREENWIDTH, SCREENHEIGHT, gb_background,
			0.0, 0.0, IM_RIGHT(gb_background) * 5.0f, 
			IM_BOTTOM(gb_background) * 5.0f, NULL, 1.0);

		gb_refresh = false;
	}

	HL_WriteTextTrans(12, 10, text_green_map, MESSAGE1);
	HL_WriteTextTrans(12, 24, text_green_map, MESSAGE2);

	switch (cur_disp)
	{
		case DIS_BUILDPROGRESS:
			num_bars = 2; dy = 0; 
			filecol = RED; nodecol = (CYAN + CYAN_LEN/2);
			break;

		case DIS_FILEPROGRESS:
			num_bars = 1; dy = 0; 
			filecol = nodecol = YELLOW;
			break;

		default:
			return;
	}

	for (i=0; i < num_bars; i++)
	{
		int x3 = x1 + (int)(bars[i].pos * (x2 - x1));

		int ty = 60 + i * 30 + dy;
		int uy = 70 + i * 30 + dy;
		int by = 80 + i * 30 + dy;

		int y1 = (int)(Y_OFFSET + uy * DY);
		int y2 = (int)(Y_OFFSET + by * DY);

		int bg = DBLUE;
		int fg = (i == num_bars-1) ? filecol : nodecol;

		HL_WriteTextTrans(lx, ty, text_white_map, bars[i].text);

		if (x3 > x1)
			vctx.SolidBox(x1, y1, x3-x1, y2-y1, fg, 1.0);

		if (x3 < x2)
			vctx.SolidBox(x3, y1, x2-x3, y2-y1, bg, 1.0);
	}
}

