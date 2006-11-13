//----------------------------------------------------------------------------
//  EDGE Network Menu Code
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
#include "m_netgame.h"

#include "dm_defs.h"
#include "dm_state.h"

#include "con_main.h"
#include "ddf_main.h"
#include "dstrings.h"
#include "e_main.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "hu_style.h"
#include "m_argv.h"
#include "m_inline.h"
#include "m_menu.h"
#include "m_misc.h"
#include "m_option.h"
#include "m_random.h"
#include "n_network.h"
#include "p_setup.h"
#include "r_local.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "sv_chunk.h"
#include "sv_main.h"
#include "v_ctx.h"
#include "v_res.h"
#include "v_colour.h"
#include "w_wad.h"
#include "wi_stuff.h"
#include "z_zone.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>


int netgame_menuon;
bool netgame_we_host;

static style_c *ng_host_style;
static style_c *ng_join_style;
static style_c *ng_list_style;


void M_DrawHostMenu(void)
{
	netgame_we_host = true;

	ng_host_style->DrawBackground();

	HL_WriteText(ng_host_style,2, 80, 10, "HOST NET GAME");

	// FIXME
}

bool M_NetHostResponder(event_t * ev, int ch)
{
	if (ch == KEYD_ENTER)
	{
		// FIXME: setup broadcast port

		sound::StartFX(sfx_swtchn, SNCAT_UI);

		netgame_menuon = 3;
		return true;
	}

	return false;
}


//----------------------------------------------------------------------------

void M_DrawJoinMenu(void)
{
	netgame_we_host = false;

	ng_join_style->DrawBackground();

	HL_WriteText(ng_join_style,2, 80, 10, "JOIN NET GAME");

	// FIXME
}

bool M_NetJoinResponder(event_t * ev, int ch)
{
	if (ch == KEYD_ENTER)
	{
		sound::StartFX(sfx_swtchn, SNCAT_UI);

		netgame_menuon = 3;
		return true;
	}

	return false;
}


//----------------------------------------------------------------------------

static void NetGameStartLevel(void)
{
	skill_t skill = sk_medium;  // FIXME: from packet

	gamedef_c *g = gamedefs.Lookup("HELL ON EARTH"); // FIXME: from packet

	DEV_ASSERT2(g);


	// -KM- 1998/12/17 Clear the intermission.
	WI_Clear();


	// create parameters

	newgame_params_c params;

	params.skill = skill;
	params.deathmatch = 2;

	params.random_seed = 1; // FIXME: from packet

	params.SinglePlayer(3);  // FIXME!!!!

	params.game = g;
	params.map = G_LookupMap(g->firstmap.GetString());

	if (! params.map || ! G_DeferredInitNew(params))
	{
		M_StartMessage(language["EpisodeNonExist"], NULL, false);
		return;
	}
}

void M_DrawPlayerList(void)
{
	ng_list_style->DrawBackground();

	HL_WriteText(ng_list_style,2, 80, 10, "PLAYER LIST");

	if (netgame_we_host)
	{
		HL_WriteText(ng_list_style,2, 40, 140, "Press <ENTER> to start game");
	}

}

bool M_NetListResponder(event_t * ev, int ch)
{
	if (ch == KEYD_ENTER && netgame_we_host)
	{
		sound::StartFX(sfx_swtchn, SNCAT_UI);

		netgame_menuon = 0;
		M_ClearMenus();

		NetGameStartLevel();

		return true;
	}

	return false;
}


//----------------------------------------------------------------------------

void M_NetGameInit(void)
{
	netgame_menuon = 0;

	// load styles
	styledef_c *def;
	style_c *ng_default;

	def = styledefs.Lookup("OPTIONS");
	if (! def) def = default_style;
	ng_default = hu_styles.Lookup(def);

	def = styledefs.Lookup("HOST NETGAME");
	ng_host_style = def ? hu_styles.Lookup(def) : ng_default;

	def = styledefs.Lookup("JOIN NETGAME");
	ng_join_style = def ? hu_styles.Lookup(def) : ng_default;

	def = styledefs.Lookup("NET PLAYER LIST");
	ng_list_style = def ? hu_styles.Lookup(def) : ng_default;
}

void M_NetGameDrawer(void)
{
	switch (netgame_menuon)
	{
		case 1: M_DrawHostMenu(); return;
		case 2: M_DrawJoinMenu(); return;
		case 3: M_DrawPlayerList(); return;
	}

	I_Error("INTERNAL ERROR: netgame_menuon=%d\n", netgame_menuon);
}

bool M_NetGameResponder(event_t * ev, int ch)
{
	switch (ch)
	{
		case KEYD_ESCAPE:
		{
			// FIXME: send packets (Unjoin / Unhost)

			netgame_menuon = 0;
			M_ClearMenus();

			sound::StartFX(sfx_swtchx, SNCAT_UI);
			return true;
		}
	}

	switch (netgame_menuon)
	{
		case 1: return M_NetHostResponder(ev, ch);
		case 2: return M_NetJoinResponder(ev, ch);
		case 3: return M_NetListResponder(ev, ch);
	}

	return false;
}

void M_NetGameTicker(void)
{
	// FIXME
}

