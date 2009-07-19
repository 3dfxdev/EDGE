//----------------------------------------------------------------------------
//  EDGE Heads-Up-Display Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2008  The EDGE Team.
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

#include "ddf/language.h"
#include "ddf/level.h"

#include "con_main.h"
#include "g_state.h"
#include "e_input.h"
#include "e_main.h"
#include "f_interm.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "hu_lib.h"
#include "m_menu.h"
#include "n_network.h"
#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"

#include <stdio.h>

#define FONT_HEIGHT  7  //!!!! FIXME temp hack

//
// Locally used constants, shortcuts.
//
// -ACB- 1998/08/09 Removed the HU_TITLE stuff; Use currmap->description.
//
#define HU_TITLEHEIGHT	1
#define HU_TITLEX	0
#define HU_TITLEY	(200 - 32 - 10) 

#define HU_INPUTX	HU_MSGX
#define HU_INPUTY	(HU_MSGY + HU_MSGHEIGHT * (FONT_HEIGHT+1))
#define HU_INPUTWIDTH	64
#define HU_INPUTHEIGHT	1

bool chat_on;

key_binding_c k_talk;

std::string w_map_title;

static std::string w_message;

static bool message_on;
static bool message_no_overwrite;
static int  message_counter;

// 23-6-98 KM Added a line showing the current limits in the
// render code.  Note that these are not really limits,
// just show how many items we have enough memory for.  These
// numbers will increase as needed.  vp = visplanes, vs = vissprites,
static hu_textline_t textlinefps;
static hu_textline_t textlinepos;
static hu_textline_t textlinestats;
static hu_textline_t textlinememory;

static style_c *message_style;
       style_c *automap_style;


//
// Heads-up Init
//
void HU_Init(void)
{
	// should use language["HeadsUpInit"], but LDF hasn't been loaded yet
	E_ProgressMessage("HU_Init: Setting up heads up display.\n");
}

void HU_Stop(void)
{
}

// -ACB- 1998/08/09 Used currmap to set the map name in string
void HU_Start(void)
{
	int i;
	const char *string;

	// find styles
	styledef_c *msg_styledef = styledefs.Lookup("MESSAGES");
	if (! msg_styledef)
		msg_styledef = default_style;
	message_style = HU_LookupStyle(msg_styledef);

	styledef_c *map_styledef = styledefs.Lookup("AUTOMAP");
	if (! map_styledef)
		map_styledef = default_style;
	automap_style = HU_LookupStyle(map_styledef);

	message_on = false;
	message_no_overwrite = false;
	chat_on = false;

	//create stuff for showstats cheat
	// 23-6-98 KM Limits info added.
	HL_InitTextLine(&textlinefps,
		0, 1 * (1 + FONT_HEIGHT), message_style, 0);
	HL_InitTextLine(&textlinestats,
		0, 2 * (1 + FONT_HEIGHT), message_style, 0);
	HL_InitTextLine(&textlinepos,
		0, 3 * (1 + FONT_HEIGHT), message_style, 0);
	HL_InitTextLine(&textlinememory,
		0, 5 * (1 + FONT_HEIGHT), message_style, 0);

	// -ACB- 1998/08/09 Use currmap settings
	if (!currmap->description.empty() &&
		language.IsValidRef(currmap->description.c_str()))
	{
		I_Printf("\n");
		I_Printf("--------------------------------------------------\n");

		CON_MessageColor(RGB_MAKE(0,255,0));

		string = language[currmap->description.c_str()];
		I_Printf("Entering %s\n", string);

		w_map_title = std::string(string);
	}
}

void HU_Drawer(void)
{
	if (message_on)
		HL_WriteText(message_style, 0, HU_MSGX, HU_MSGY, w_message.c_str(), 1.0f);

//TODO	if (chat_on)
//		HL_DrawIText(&w_chat);

	//now, draw stats
	// -ACB- 1998/09/11 Used White Colour Scaling.
	if (showstats)
	{
		static int numframes = 0, lasttime = 0;
		static float fps = 0, mspf = 0;

		char textbuf[100];
		char *s;
		int currtime, timediff;

		numframes++;
		currtime = I_GetTime();
		timediff = currtime - lasttime;

		if (timediff > 70)
		{
			fps  = (float) (numframes * TICRATE) / (float) timediff;
			mspf = (float) timediff * 1000.0f / (float) (numframes * TICRATE);

			lasttime = currtime;
			numframes = 0;
		}

		HL_ClearTextLine(&textlinefps);
		sprintf(textbuf, "fps: %1.1f  ms/f:%1.1f   time:%d:%02d", fps, 
			mspf, (leveltime / TICRATE) / 60, (leveltime / TICRATE) % 60);

		s = textbuf;
		while (*s)
			HL_AddCharToTextLine(&textlinefps, *(s++));
		HL_DrawTextLine(&textlinefps, 0);

#if 1  // don't want non-FPS info causing extra slowdown

		HL_ClearTextLine(&textlinememory);
		sprintf(textbuf, "used cache: %d/%d",
			W_CacheInfo(1),
			W_CacheInfo(2));
		s = textbuf;
		while (*s)
			HL_AddCharToTextLine(&textlinememory, *(s++));
		HL_DrawTextLine(&textlinememory, 0);

		if (!netgame)
		{
			HL_ClearTextLine(&textlinepos);
			HL_ClearTextLine(&textlinestats);

			player_t *p = players[displayplayer];
			SYS_ASSERT(p);

			// Convert angle & x,y co-ordinates so they are easier to read.
			// -KM- 1998/11/25 Added z co-ordinate
			sprintf(textbuf, "LookDir=%1.0f; x,y,z=( %1.0f, %1.0f, %1.0f );"
				" sec=%d/%d", ANG_2_FLOAT(p->mo->angle),
				p->mo->x, p->mo->y,
				p->mo->z,
				(int) (p->mo->subsector->sector - sectors),
				(int) (p->mo->subsector - subsectors));
			s = textbuf;
			while (*s)
				HL_AddCharToTextLine(&textlinepos, *(s++));

			sprintf(textbuf, "Kills:%d/%d   Items:%d/%d   Secrets:%d/%d",
				p->killcount, wi_stats.kills,
				p->itemcount, wi_stats.items,
				p->secretcount, wi_stats.secret);
			s = textbuf;
			while (*s)
				HL_AddCharToTextLine(&textlinestats, *(s++));
			HL_DrawTextLine(&textlinepos, 0);
			HL_DrawTextLine(&textlinestats, 0);
		}
#endif
	}
}

void HU_Erase(void)
{
}

// Starts displaying the message.
void HU_StartMessage(const char *msg)
{
	// only display message if necessary
	if (! message_no_overwrite)
	{
		w_message = std::string(msg);

		message_on = true;
		message_counter = HU_MSGTIMEOUT;
		message_no_overwrite = false;
	}
}

void HU_Ticker(void)
{
	// tick down message counter if message is up
	if (message_counter && !--message_counter)
	{
		message_on = false;
		message_no_overwrite = false;
	}

	// check for incoming chat characters
	if (! netgame)
		return;

#if 0  // OLD STUFF FOR CHAT MESSAGES
	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];
		if (! p) continue;

		if (pnum == consoleplayer)
			continue;

		char c = p->cmd.chatchar;
		p->cmd.chatchar = 0;
		if (! c)
			continue;

		int i = p->pnum;

		if (c <= HU_BROADCAST)
		{
			chat_dest[i] = c;
			continue;
		}

		int rc = HL_KeyInIText(&w_inputbuffer[i], c);

		if (rc && c == KEYD_ENTER)
		{
			if (w_inputbuffer[i].L.len
				&& (chat_dest[i] == consoleplayer + 1
				|| chat_dest[i] == HU_BROADCAST))
			{
				HL_AddMessageToSText(&w_message, p->playername, w_inputbuffer[i].L.ch);

				message_on = true;
				message_no_overwrite = true;
				message_counter = HU_MSGTIMEOUT;

				if (W_CheckNumForName("DSRADIO") >= 0)
					S_StartFX(sfx_radio, SNCAT_UI);
				else
					S_StartFX(sfx_tink, SNCAT_UI);
			}
			HL_ResetIText(&w_inputbuffer[i]);
		}
	}
#endif
}

bool HU_Responder(event_t * ev)
{
	if (ev->type != ev_keydown)
		return false;

	int sym = ev->value.key.sym;

	if (!chat_on && sym == KEYD_ENTER)
	{
		message_on = true;
		message_counter = HU_MSGTIMEOUT;
		return false;
	}

	return false;

#if 0  // OLD CHAT STUFF
	static char lastmessage[HU_MAXLINELENGTH + 1];

	if (!chat_on)
	{
		if (netgame && k_talk.HasKey(ev))
		{
			chat_on = true;
			HL_ResetIText(&w_chat);
			HU_QueueChatChar(HU_BROADCAST);
			return true;
		}
		return false;
	}

	unsigned char c;

	// -ACB- 2008/09/22 Use unicode key if valid
	int unicode = ev->value.key.unicode;
	if (HU_IS_PRINTABLE(unicode))
		c = (char)unicode;
	else
		c = (char)sym;

	bool eatkey = HL_KeyInIText(&w_chat, c);
	if (eatkey)
		HU_QueueChatChar(c);

	if (sym == KEYD_ENTER)
	{
		chat_on = false;

		if (w_chat.L.len)
			CON_PlayerMessage(consoleplayer, lastmessage, w_chat.L.ch);
	}
	else if (sym == KEYD_ESCAPE)
	{
		chat_on = false;
	}

	return eatkey;
#endif
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
