//----------------------------------------------------------------------------
//  EDGE Heads-Up-Display Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2007  The EDGE Team.
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
#include "hu_stuff.h"

#include "con_main.h"
#include "dm_state.h"
#include "e_input.h"
#include "e_main.h"
#include "g_game.h"
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
#define HU_INPUTTOGGLE	key_talk
#define HU_INPUTX	HU_MSGX
#define HU_INPUTY	(HU_MSGY + HU_MSGHEIGHT * (FONT_HEIGHT+1))
#define HU_INPUTWIDTH	64
#define HU_INPUTHEIGHT	1

bool chat_on;
static hu_textline_t w_title;
static hu_itext_t w_chat;

static char *chat_dest;
static hu_itext_t *w_inputbuffer;

static bool message_on;
static bool message_no_overwrite;

static hu_stext_t w_message;
static int message_counter;

static bool headsupactive = false;

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
static style_c *chat_style;


// -ACB- 1999/09/28 was english_shiftxform. Only one used.
static const unsigned char shiftxform[] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
	11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
	21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,

	' ', '!', '"', '#', '$', '%', '&',
	'"',  // shift-'
	'(', ')', '*', '+',
	'<',  // shift-,
	'_',  // shift--
	'>',  // shift-.
	'?',  // shift-/
	')',  // shift-0
	'!',  // shift-1
	'@',  // shift-2
	'#',  // shift-3
	'$',  // shift-4
	'%',  // shift-5
	'^',  // shift-6
	'&',  // shift-7
	'*',  // shift-8
	'(',  // shift-9
	':',
	':',  // shift-;
	'<',
	'+',  // shift-=
	'>', '?', '@',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'[',  // shift-[
	'!',  // shift-backslash
	']',  // shift-]
	'"', '_',
	'\'',  // shift-`
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'{', '|', '}', '~', 127,

	128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139,
	140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151,
	152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163,
	164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
	176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187,
	188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 
	200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 
	212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
	224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 
	236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 
	248, 249, 250, 251, 252, 253, 254, 255
};

//
// Heads-up Init
//
void HU_Init(void)
{
	// should use language["HeadsUpInit"], but LDF hasn't been loaded yet
	E_ProgressMessage("HU_Init: Setting up heads up display.\n");

	chat_dest = Z_New(char, MAXPLAYERS);

	w_inputbuffer = Z_New(hu_itext_t, MAXPLAYERS);
}

static void HU_Stop(void)
{
	headsupactive = false;
}

// -ACB- 1998/08/09 Used currmap to set the map name in string
void HU_Start(void)
{
	int i;
	const char *string;

	if (headsupactive)
		HU_Stop();

	// find styles
	styledef_c *msg_styledef = styledefs.Lookup("MESSAGES");
	if (! msg_styledef)
		msg_styledef = default_style;
	message_style = hu_styles.Lookup(msg_styledef);

	styledef_c *map_styledef = styledefs.Lookup("AUTOMAP");
	if (! map_styledef)
		map_styledef = default_style;
	automap_style = hu_styles.Lookup(map_styledef);

	styledef_c *chat_styledef = styledefs.Lookup("CHAT");
	if (! chat_styledef)
		chat_styledef = default_style;
	chat_style = hu_styles.Lookup(chat_styledef);

	message_on = false;
	message_no_overwrite = false;
	chat_on = false;

	// create the message widget
	HL_InitSText(&w_message, HU_MSGX, HU_MSGY, HU_MSGHEIGHT, message_style, 0);

	// create the map title widget
	HL_InitTextLine(&w_title, HU_TITLEX, HU_TITLEY, automap_style, 0);

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
	if (currmap->description &&
		language.IsValidRef(currmap->description))
	{
		string = language[currmap->description];
		I_Printf("Entering %s\n", string);

		for (; *string; string++)
			HL_AddCharToTextLine(&w_title, *string);
	}

	// create the chat widget
	HL_InitIText(&w_chat, HU_INPUTX, HU_INPUTY, chat_style, 0);

	// create the inputbuffer widgets
	for (i = 0; i < MAXPLAYERS; i++)
		HL_InitIText(&w_inputbuffer[i], 0, 0, NULL, 0);

	headsupactive = true;
}

void HU_Drawer(void)
{
	if (message_on)
		HL_DrawSText(&w_message);

	if (chat_on)
		HL_DrawIText(&w_chat);

	if (automapactive)
		HL_DrawTextLine(&w_title, false);

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
	if (!headsupactive)
		return;

	HL_EraseSText(&w_message);
	HL_EraseIText(&w_chat);
	HL_EraseTextLine(&w_title);
}

// Starts displaying the message.
void HU_StartMessage(const char *msg)
{
	// only display message if necessary
	if (! message_no_overwrite)
	{
		HL_AddMessageToSText(&w_message, 0, msg);
		message_on = true;
		message_counter = HU_MSGTIMEOUT;
		message_no_overwrite = false;
	}
}

void HU_Ticker(void)
{
	int i,rc;
	char c;

	// tick down message counter if message is up
	if (message_counter && !--message_counter)
	{
		message_on = false;
		message_no_overwrite = false;
	}

	// check for incoming chat characters
	if (! netgame)
		return;

	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];
		if (! p) continue;

		if (pnum == consoleplayer)
			continue;

		c = p->cmd.chatchar;
		i = p->pnum;
		if (c)
		{
			if (c <= HU_BROADCAST)
				chat_dest[i] = c;
			else
			{
				if (c >= 'a' && c <= 'z')
					c = (char)shiftxform[(unsigned char)c];
				rc = HL_KeyInIText(&w_inputbuffer[i], c);
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
			p->cmd.chatchar = 0;
		}
	}
}

#define QUEUESIZE 128

static char chatchars[QUEUESIZE];
static int head = 0;
static int tail = 0;

void HU_QueueChatChar(char c)
{
	if (((head + 1) & (QUEUESIZE - 1)) == tail)
	{
		CON_PlayerMessageLDF(consoleplayer, "UnsentMsg");
	}
	else
	{
		chatchars[head] = c;
		head = (head + 1) & (QUEUESIZE - 1);
	}
}

char HU_DequeueChatChar(void)
{
	char c;

	if (head != tail)
	{
		c = chatchars[tail];
		tail = (tail + 1) & (QUEUESIZE - 1);
	}
	else
	{
		c = 0;
	}

	return c;
}

bool HU_Responder(event_t * ev)
{
	static char lastmessage[HU_MAXLINELENGTH + 1];
	bool eatkey = false;
	static bool shiftdown = false;
	static bool altdown = false;
	unsigned char c;

	if (ev->type == ev_analogue)
		return false;

	c = ev->value.key;

	if (c == KEYD_RSHIFT)
	{
		shiftdown = (ev->type == ev_keydown);
		return false;
	}
	else if (c == KEYD_RALT || c == KEYD_LALT)
	{
		altdown = (ev->type == ev_keydown);
		return false;
	}

	if (ev->type != ev_keydown)
		return false;

	if (!chat_on)
	{
		if (c == KEYD_ENTER)
		{
			message_on = true;
			message_counter = HU_MSGTIMEOUT;
			eatkey = false;
		}
		else if (netgame && c && ((c == (HU_INPUTTOGGLE >> 16)) || (c == (HU_INPUTTOGGLE & 0xffff))))
		{
			eatkey = chat_on = true;
			HL_ResetIText(&w_chat);
			HU_QueueChatChar(HU_BROADCAST);
		}
	}
	else
	{
		if (shiftdown || (c >= 'a' && c <= 'z'))
			c = shiftxform[c];

		eatkey = HL_KeyInIText(&w_chat, c);
		if (eatkey)
		{
			// static unsigned char buf[20]; // DEBUG
			HU_QueueChatChar(c);

			// sprintf(buf, "KEY: %d => %d", ev->data1, c);
			//      consoleplayer->message = buf;
		}
		if (c == KEYD_ENTER)
		{
			chat_on = false;

			if (w_chat.L.len)
				CON_PlayerMessage(consoleplayer, lastmessage, w_chat.L.ch);
		}
		else if (c == KEYD_ESCAPE)
			chat_on = false;
	}

	return eatkey;

}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
