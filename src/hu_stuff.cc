//----------------------------------------------------------------------------
//  EDGE Heads-Up-Display Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2018  The EDGE Team.
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

#include "system/i_defs.h"

#include "hu_stuff.h"
#include "hu_style.h"
#include "hu_draw.h"


#include "con_var.h"
#include "con_main.h"
#include "con_gui.h"
#include "dm_state.h"
#include "e_input.h"
#include "e_main.h"
#include "g_game.h"
#include "f_interm.h"
#include "m_menu.h"
#include "n_network.h"
#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"

DEF_CVAR(r_textscale, float, "c", 0.7f);
DEF_CVAR(r_text_x, int, "c", 160);
DEF_CVAR(r_text_y, int, "c", 3);
//extern cvar_c r_textscale;
//
// Locally used constants, shortcuts.
//
// -ACB- 1998/08/09 Removed the HU_TITLE stuff; Use currmap->description.
//
#define HU_TITLEHEIGHT	1
#define HU_TITLEX	0
#define HU_TITLEY	(200 - 32 - 10) 

#define HU_INPUTX	(HU_MSGX)
#define HU_INPUTY	(HU_MSGY + HU_MSGHEIGHT * 8)
#define HU_INPUTWIDTH	64
#define HU_INPUTHEIGHT	1

bool chat_on;

std::string w_map_title;

extern bool message_on; //switched static for extern . . .
static bool message_center;
static bool message_no_overwrite;

std::string w_message;
static int message_counter;

style_c *automap_style;

float fade_time;
//
// Heads-up Init
//
void HU_Init(void)
{
	// should use language["HeadsUpInit"], but LDF hasn't been loaded yet
	E_ProgressMessage("HU_Init: Setting up heads up display.\n");
}



// -ACB- 1998/08/09 Used currmap to set the map name in string
void HU_Start(void)
{
	const char *string;

	SYS_ASSERT(currmap);

	styledef_c *map_styledef = styledefs.Lookup("AUTOMAP");
	if (! map_styledef)
		map_styledef = default_style;
	automap_style = hu_styles.Lookup(map_styledef);

	message_on = false;
	message_center = false;
	message_no_overwrite = false;

	// -ACB- 1998/08/09 Use currmap settings
	if (currmap->description &&
		language.IsValidRef(currmap->description))
	{
		I_Printf("\n");
		I_Printf("--------------------------------------------------\n");

		CON_MessageColor(RGB_MAKE(0,255,0));

		string = language[currmap->description];
		I_Printf("Entering %s\n", string);

		w_map_title = std::string(string);
	}
}


void HU_Drawer(void)
{
	//cvar_c m_centerem;

	
	CON_ShowFPS();


	if (message_on)
	{
		HUD_SetAlpha(1.0f); //r_textalpha, defaults to "1.0f";
		HUD_SetScale(r_textscale);	 //TODO: Should make this user-definable in the Options Menu.
		HUD_SetAlignment(0, 0); //use this to set alignment?
		//OLD. NON CENTERED. HUD_DrawText(HU_MSGX, HU_MSGY, w_message.c_str());
		HUD_DrawText(r_text_x, r_text_y, w_message.c_str()); //r_text_x = 160 - 3/ 2 (keep this 160 int), r_text_y = 3;
		HUD_SetScale();
		HUD_SetAlignment();
		HUD_SetAlpha();
	}

// TODO: chat messages
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
	
	//TODO: HUD Message Fading
/* 	if(message_counter >= HU_MSGFADESTART) 
	{
       alpha = MAX((alpha -= HU_MSGFADETIME), 0);
    } */

	
	// check for incoming chat characters
	if (! netgame)
		return;

	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];
		if (! p) continue;

		if (p->playerflags & PFL_Console)
			continue;

		char c = p->cmd.chatchar;

		p->cmd.chatchar = 0;

		if (c)
		{
			/* TODO: chat stuff */
		}
	}
}


void HU_QueueChatChar(char c)
{
	// TODO
}


char HU_DequeueChatChar(void)
{
	return 0;  // TODO
}


bool HU_Responder(event_t * ev)
{
	if (ev->type != ev_keyup && ev->type != ev_keydown)
		return false;

	int c = ev->data1;

	if (ev->type != ev_keydown)
		return false;

	// TODO: chat stuff
	if (false)
	{
		//...
		return true;
	}

	if (c == KEYD_ENTER)
	{
		message_on = true;
		message_counter = HU_MSGTIMEOUT;
	}

	return false;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
