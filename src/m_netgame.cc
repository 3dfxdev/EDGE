//----------------------------------------------------------------------------
//  EDGE Network Menu Code
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
#include "i_net.h"

#include "ddf/main.h"

#include "con_main.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "dstrings.h"
#include "e_main.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "hu_style.h"
#include "m_argv.h"
#include "m_menu.h"
#include "m_misc.h"
#include "m_option.h"
#include "m_random.h"
#include "m_netgame.h"
#include "n_network.h"
#include "n_reliable.h"
#include "p_setup.h"
#include "r_local.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "sv_chunk.h"
#include "sv_main.h"
#include "r_draw.h"
#include "r_modes.h"
#include "r_colormap.h"
#include "w_wad.h"
#include "f_interm.h"
#include "z_zone.h"


extern gameflags_t default_gameflags;


int netgame_menuon;
bool netgame_we_are_host;

static style_c *ng_host_style;
static style_c *ng_join_style;
static style_c *ng_list_style;

static newgame_params_c *ng_params;


static int host_pos;

static int hosting_port;

static int host_want_bots;

#define HOST_OPTIONS  6
#define JOIN_OPTIONS  4


static int join_pos;

static net_address_c * join_addr;
static int joining_port;

static int join_discover_timer;


#if (0 == 1)
static void CreateHostWelcome(welcome_proto_t *we)
{
	we->host_ver = 1; //!!! MP_PROTOCOL_VER;

	strcpy(we->game_name, "HELL ON EARTH");

	strcpy(we->level_name, "MAP01");  // FIXME: game->firstmap

	we->mode  = welcome_proto_t::MODE_New_DM;
	we->skill = sk_medium;

	we->teams = 0;

	we->gameplay = welcome_proto_t::GP_True3D |
		           welcome_proto_t::GP_MLook;

	we->random_seed = I_PureRandom();

	we->wad_checksum = 0; // FIXME
	we->def_checksum = 0; // FIXME

	memset(we->reserved, 0, sizeof(we->reserved));
}

void CreateHostPlayerList(newgame_params_c *par, int humans, int bots)
{
	int total = humans + bots;

	SYS_ASSERT(humans > 0);
	SYS_ASSERT(total <= MAXPLAYERS);

	int bots_per_human = bots / humans;
	int host_bots = bots - (bots_per_human * (humans - 1));

	int p = 0;

	par->players[p++] = PFL_Zero;

	for (; host_bots > 0; host_bots--)
		par->players[p++] = PFL_Bot;

	for (int h = 1; h < humans; h++)
	{
		par->players[p++] = PFL_Network;

		for (int b = bots_per_human; b > 0; b--)
			par->players[p++] = (playerflag_e)(PFL_Bot | PFL_Network);
	}

	par->total_players = p;

	SYS_ASSERT(p == humans + bots);
}

static bool ParseWelcomePacket(newgame_params_c *par, welcome_proto_t *we)
{
	gamedef_c *game = gamedefs.Lookup(we->game_name);

	if (! game)
	{
		I_Printf("Unknown game in welcome packet: %s\n", we->game_name);
		return false;
	}

	par->map = G_LookupMap(we->level_name);

	if (! par->map)
	{
		I_Printf("Unknown level in welcome packet: %s\n", we->level_name);
		return false;
	}

	par->skill = (skill_t)we->skill;

	switch (we->mode)
	{
		case welcome_proto_t::MODE_Coop:
			par->deathmatch = 0;
			break;
	
		case welcome_proto_t::MODE_Old_DM:
			par->deathmatch = 1;
			break;
	
		case welcome_proto_t::MODE_New_DM:
			par->deathmatch = 2;
			break;
	
		default:
			I_Error("Unknown game mode in welcome packet: [%c]\n", we->mode);
			return false; /* NOT REACHED */
	}

	par->random_seed = we->random_seed;
	par->total_players = 0;  // updated later

	/* FIXME: we->bots, we->teams */

	gameflags_t flags = default_gameflags;

#define HANDLE_FLAG(field, testbit)  \
	(flags.field) = (we->gameplay & (testbit)) ? true : false;

	HANDLE_FLAG(nomonsters,     welcome_proto_t::GP_NoMonsters);
	HANDLE_FLAG(fastparm,       welcome_proto_t::GP_FastMon);

	HANDLE_FLAG(true3dgameplay, welcome_proto_t::GP_True3D);
	HANDLE_FLAG(jump,           welcome_proto_t::GP_Jumping);
	HANDLE_FLAG(crouch,         welcome_proto_t::GP_Crouching);
	HANDLE_FLAG(mlook,          welcome_proto_t::GP_MLook);

	flags.autoaim =
		(we->gameplay & welcome_proto_t::GP_AutoAim) ? AA_ON : AA_OFF;

	return true;
}

void ParsePlayerList(newgame_params_c *par, player_list_proto_t *li)
{
	SYS_ASSERT(li->real_players > 0);
	SYS_ASSERT(li->real_players <= li->total_players);
	SYS_ASSERT(li->total_players <= MAXPLAYERS);

	par->total_players = li->total_players;

	for (int p = li->first_player; p <= li->last_player; p++)
	{
		par->players[p] = (playerflag_e) li->players[p].player_flags;
	}
}
#endif


static void DrawKeyword(int index, style_c *style, int y,
		const char *keyword, const char *value)
{
	int x = 120;

	HL_WriteText(style,(index<0)?3:0, x - 10 - style->fonts[0]->StringWidth(keyword), y, keyword);
	HL_WriteText(style,1, x + 10, y, value);

	if ((netgame_menuon == 1 && index == host_pos) ||
	    (netgame_menuon == 2 && index == join_pos))
	{
		HL_WriteText(style,2, x - style->fonts[2]->StringWidth("*")/2, y, "*");
	}
}

static const char *GetModeName(char mode)
{
	// FIXME: LDF these up
	switch (mode)
	{
		case 0: return "CO-OPERATIVE";
		case 1: return "OLD DEATHMATCH";
		case 2: return "NEW DEATHMATCH";
		case 3: return "LAST MAN STANDING";
		case 4: return "CATCH THE FLAG";

		default: return "????";
	}
}

static const char *GetSkillName(skill_t skill)
{
	// FIXME: LDF these up, OR BETTER: use shrunken images
	switch (skill)
	{
		case sk_baby: return "I'm Too Young To Die";
		case sk_easy: return "Hey, Not Too Rough";
		case sk_medium: return "Hurt Me Plenty";
		case sk_hard: return "Ultra-Violence";
		case sk_nightmare: return "Nightmare!";

		default: return "????";
	}
}

static const char *LocalPrintf(char *buf, int max_len, const char *str, ...)
{
	va_list argptr;

	va_start (argptr, str);
	vsnprintf (buf, max_len, str, argptr);
	va_end (argptr);

	return buf;
}


//----------------------------------------------------------------------------

void M_NetHostBegun(void)
{
	netgame_we_are_host = true;

	if (hosting_port <= 0)
		hosting_port = base_port;

	host_pos = 0;

	if (ng_params)
		delete ng_params;

	ng_params = new newgame_params_c;

	ng_params->map = G_LookupMap("1");

	host_want_bots = 0;
}


static void HostChangeOption(int opt, int key)
{
	int dir = (key == KEYD_LEFTARROW) ? -1 : +1;

	switch (opt)
	{
		case 0: // Game
			// FIXME
			break;

		case 1: // Level
			// FIXME !!!!
			break;

		case 2: // Mode
		{
			ng_params->deathmatch += dir;

			if (ng_params->deathmatch < 0)
				ng_params->deathmatch = 2;
			else if (ng_params->deathmatch > 2)
				ng_params->deathmatch = 0;

			break;
		}

		case 3: // Skill
///!!!!			ng_params->skill += dir;
///!!!!			if (ng_params->skill < 0 || ng_params->skill > 250)
///!!!!				ng_params->skill = 4;
///!!!!			else if (ng_params->skill > 4)
///!!!!				ng_params->skill = 0;

			break;

		case 4: // Bots
			host_want_bots += dir;

			if (host_want_bots < 0)
				host_want_bots = 15;
			else if (host_want_bots > 15)
				host_want_bots = 0;

			break;

		default:
			break;
	}
}

static void HostAccept(void)
{
	// create local player and bots
	ng_params->SinglePlayer(host_want_bots);
}

void M_DrawHostMenu(void)
{
	ng_host_style->DrawBackground();

	HL_WriteText(ng_host_style,2, 80, 10, "HOST NET GAME");

	char buffer[200];

	int y = 30;
	int idx = 0;


	DrawKeyword(-1, ng_host_style, y, "LOCAL ADDRESS", n_local_addr.TempString(false));
	y += 10;

	DrawKeyword(-1, ng_join_style, y, "LOCAL PORT",
			LocalPrintf(buffer, sizeof(buffer), "%d", hosting_port));
	y += 20;


	DrawKeyword(idx, ng_host_style, y, "GAME", ng_params->map->episode->ddf.name.c_str());
	y += 20; idx++,

	DrawKeyword(idx, ng_host_style, y, "LEVEL", ng_params->map->ddf.name.c_str());
	y += 10; idx++,

	DrawKeyword(idx, ng_host_style, y, "MODE", GetModeName(ng_params->deathmatch));
	y += 10; idx++,

	DrawKeyword(idx, ng_host_style, y, "SKILL", GetSkillName(ng_params->skill));
	y += 10; idx++,

	DrawKeyword(idx, ng_host_style, y, "BOTS",
			LocalPrintf(buffer, sizeof(buffer), "%d", host_want_bots));
	y += 20; idx++,

	// etc

	HL_WriteText(ng_host_style,(host_pos==idx) ? 2:0, 20,  y, "Begin Accepting Connections");
}

bool M_NetHostResponder(event_t * ev, int ch)
{
	if (ch == KEYD_ENTER)
	{
		if (host_pos == (HOST_OPTIONS-1))
		{
			HostAccept();
			S_StartFX(sfx_swtchn);
			netgame_menuon = 3;
		}
		return true;
	}

	if (ch == KEYD_DOWNARROW || ch == KEYD_MWHEEL_DN)
	{
		host_pos = (host_pos + 1) % HOST_OPTIONS;
		return true;
	}
	else if (ch == KEYD_UPARROW || ch == KEYD_MWHEEL_UP)
	{
		host_pos = (host_pos + HOST_OPTIONS - 1) % HOST_OPTIONS;
		return true;
	}

	if (ch == KEYD_LEFTARROW || ch == KEYD_RIGHTARROW ||
		ch == KEYD_ENTER)
	{
		HostChangeOption(host_pos, ch);
		return true;
	}

	return false;
}

void M_NetHostTicker(void)
{
	// Note: broadcast queries are handled by the general net stuff  [???]
}


//----------------------------------------------------------------------------

void M_NetJoinBegun(void)
{
	netgame_we_are_host = false;

	if (joining_port <= 0)
		joining_port = base_port;

	join_pos = 0;
	join_discover_timer = 10 * TICRATE;

	if (ng_params)
		delete ng_params;

	ng_params = new newgame_params_c;
}

static void JoinConnect(void)
{
	// ??
}

static void JoinChangeOption(int opt, int key)
{
	int dir = (key == KEYD_LEFTARROW) ? -1 : +1;

	switch (opt)
	{
		case 0: // Host Addr
			if (key == KEYD_ENTER)
			{ // pop up input box !!!!
			// FIXME !!!!
			}
			break;

		case 1: // Host Port
			if (key == KEYD_ENTER)
			{ // pop up input box !!!!
			// FIXME !!!!
			}
			else
				joining_port += dir;
			break;

		case 2: // Search again
		{
			join_discover_timer = 10 * TICRATE;
			break;
		}

		case 3: // Connect
			if (! join_addr)
			{
				S_StartFX(sfx_oof);
				return;
			}

			JoinConnect();

			S_StartFX(sfx_swtchn);
			netgame_menuon = 3;
			break;

		default:
			break;
	}
}


void M_DrawJoinMenu(void)
{
	ng_join_style->DrawBackground();

	HL_WriteText(ng_join_style,2, 80, 10, "JOIN NET GAME");

	char buffer[200];

	if (join_discover_timer > 0)
	{
		HL_WriteText(ng_join_style,3,  30, 160, "Looking for Host on LAN...");
		HL_WriteText(ng_join_style,1, 240, 160,
				LocalPrintf(buffer, sizeof(buffer), "%d",
					(join_discover_timer+TICRATE-1) / TICRATE));
	}

	int y = 30;
  	int idx = 0;
	
	DrawKeyword(idx, ng_join_style, y, "HOST ADDRESS",
				join_addr ? join_addr->TempString(false) : "???");
	y += 10; idx++;

	DrawKeyword(idx, ng_join_style, y, "HOST PORT",
			LocalPrintf(buffer, sizeof(buffer), "%d", joining_port));
	y += 20; idx++;

	// FIXME....


	HL_WriteText(ng_join_style,(join_pos==idx)?2:0, 30, y, "Search LAN again");
	y += 20; idx++;

	HL_WriteText(ng_join_style,(join_pos==idx)?2:0, 30, y, "Connect to Host");
	y += 20; idx++;
}

bool M_NetJoinResponder(event_t * ev, int ch)
{
	if (ch == KEYD_DOWNARROW || ch == KEYD_MWHEEL_DN)
	{
		join_pos = (join_pos + 1) % JOIN_OPTIONS;
		return true;
	}
	else if (ch == KEYD_UPARROW || ch == KEYD_MWHEEL_UP)
	{
		join_pos = (join_pos + JOIN_OPTIONS - 1) % JOIN_OPTIONS;
		return true;
	}

	if (ch == KEYD_LEFTARROW || ch == KEYD_RIGHTARROW ||
		ch == KEYD_ENTER)
	{
		JoinChangeOption(join_pos, ch);
		return true;
	}

	return false;
}

void M_NetJoinTicker(void)
{
	if (join_discover_timer > 0)
	{
		if ((join_discover_timer % TICRATE) == 0)
		{
			// FIXME: send broadcast packet
		}

		join_discover_timer--;
	}

	// FIXME: check for broadcast reply 
}


//----------------------------------------------------------------------------

static void NetGameStartLevel(void)
{
	// -KM- 1998/12/17 Clear the intermission.
	WI_Clear();

	G_DeferredNewGame(*ng_params);
}

void M_DrawPlayerList(void)
{
	ng_list_style->DrawBackground();

	HL_WriteText(ng_list_style,2, 80, 10, "PLAYER LIST");

	char buffer[200];

	int y = 30;

	for (int i = 0; i < ng_params->total_players; i++)
	{
		DrawKeyword(-1, ng_list_style, y,
				LocalPrintf(buffer, sizeof(buffer), "%s%d", 
					(ng_params->players[i] & PFL_Bot) ? "Bot" : "Player", i),
				ng_params->nodes[i] ? ng_params->nodes[i]->remote.TempString(false) : "Local");
		y += 10;
	}

	if (netgame_we_are_host)
	{
		HL_WriteText(ng_list_style,2, 40, 140, "Press <ENTER> to Start Game");
	}
}

bool M_NetListResponder(event_t * ev, int ch)
{
	if (ch == KEYD_ENTER && netgame_we_are_host)
	{
		S_StartFX(sfx_swtchn);

		netgame_menuon = 0;
		M_ClearMenus();

		NetGameStartLevel();

		return true;
	}

	return false;
}

void M_NetListTicker(void)
{
	// FIXME: handle player update commands, BEGIN command, etc
}


//----------------------------------------------------------------------------

void M_NetGameInit(void)
{
	netgame_menuon = 0;

	host_pos = 0;
	join_pos = 0;

	hosting_port = joining_port = 0;  // set later

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


	const char *str = M_GetParm("-connect");
	if (str)
	{
		join_addr = new net_address_c();
		if (! join_addr->FromString(str))
		{
			I_Error("Bad IP address for -connect: %s\n", str);
		}
	}
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
			netgame_menuon = 0;
			M_ClearMenus();

			S_StartFX(sfx_swtchx);
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
	switch (netgame_menuon)
	{
		case 1: return M_NetHostTicker();
		case 2: return M_NetJoinTicker();
		case 3: return M_NetListTicker();
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
