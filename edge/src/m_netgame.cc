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
bool netgame_we_host;

static style_c *ng_host_style;
static style_c *ng_join_style;
static style_c *ng_list_style;

static newgame_params_c *ng_params;


//
// WELCOME ("we")
//
typedef struct welcome_proto_s  // FIXME: TEMP CRAP
{
	u16_t host_ver;  // MP_PROTOCOL_VER from host

	enum
	{
		GAME_STR_MAX = 32
	};

	char game_name[GAME_STR_MAX];     // e.g. HELL_ON_EARTH

	enum
	{
		LEVEL_STR_MAX = 8
	};

	char level_name[LEVEL_STR_MAX];   // e.g. MAP01

	enum  // mode values:
	{
		MODE_Coop       = 'C',
		MODE_New_DM     = 'N',
		MODE_Old_DM     = 'O',  // weapons stay on map (etc)
		MODE_LastMan    = 'L',
		MODE_CTF        = 'F',
	};

	byte mode;
	byte skill;  // 1 to 5

	byte bots;
	byte teams;  // 0 = no teams

	enum  // gameplay bitflags:
	{
		GP_NoMonsters = (1 << 0),
		GP_FastMon    = (1 << 1),

		GP_True3D     = (1 << 3),
		GP_Jumping    = (1 << 4),
		GP_Crouching  = (1 << 5),

		GP_AutoAim    = (1 << 7),
		GP_MLook      = (1 << 8),
	};

	u32_t gameplay;
	u32_t random_seed;

	u16_t wad_checksum;  // checksum over all loaded wads
	u16_t def_checksum;  // checksum over all definitions

	byte reserved[16];  // (future expansion)

	void ByteSwap();
}
welcome_proto_t;


static int host_pos;

static int hosting_port;

static welcome_proto_t host_welcome;

#define HOST_OPTIONS  6
#define JOIN_OPTIONS  4


static int join_pos;

static net_address_c * join_addr;
static int joining_port;

static int join_discover_timer;

static welcome_proto_t join_welcome;


static void CreateHostWelcome(welcome_proto_t *we)
{
	we->host_ver = 1; //!!! MP_PROTOCOL_VER;

	strcpy(we->game_name, "HELL ON EARTH");

	strcpy(we->level_name, "MAP01");  // FIXME: game->firstmap

	we->mode  = welcome_proto_t::MODE_New_DM;
	we->skill = sk_medium;

	we->bots  = 0;
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

#if 0  // OLD CRUD
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
		case 'C': return "CO-OPERATIVE";
		case 'O': return "OLD DEATHMATCH";
		case 'N': return "NEW DEATHMATCH";
		case 'L': return "LAST MAN STANDING";
		case 'F': return "CATCH THE FLAG";

		default: return "????";
	}
}

static const char *GetSkillName(byte skill)
{
	// FIXME: LDF these up, OR BETTER: use shrunken images
	switch (skill)
	{
		case 1: return "I'm Too Young To Die";
		case 2: return "Hey, Not Too Rough";
		case 3: return "Hurt Me Plenty";
		case 4: return "Ultra-Violence";
		case 5: return "Nightmare!";

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
	netgame_we_host = true;

	host_pos = 0;
}

static const char * MODE_LIST_STR = "CNO"; // "CNOLF"

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
			const char *pos = strchr(MODE_LIST_STR, host_welcome.mode);
			if (! pos) return;

			pos += dir;

			if (pos < MODE_LIST_STR)
				pos = MODE_LIST_STR + strlen(MODE_LIST_STR) - 1;
			else if (*pos == 0)
				pos = MODE_LIST_STR;

			host_welcome.mode = *pos;
			break;
		}

		case 3: // Skill
			host_welcome.skill += dir;
			if (host_welcome.skill < 1 || host_welcome.skill > 250)
				host_welcome.skill = 5;
			else if (host_welcome.skill > 5)
				host_welcome.skill = 1;

			break;

		case 4: // Bots
			host_welcome.bots += dir;
///---		if (host_welcome.bots >= 5 && (host_welcome.bots & 1))
///---			host_welcome.bots += dir;

			if (host_welcome.bots & 0x80)
				host_welcome.bots = 15;
			else if (host_welcome.bots > 15)
				host_welcome.bots = 0;

			break;

		default:
			break;
	}
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


	DrawKeyword(idx, ng_host_style, y, "GAME", host_welcome.game_name);
	y += 10; idx++,

	DrawKeyword(idx, ng_host_style, y, "LEVEL", host_welcome.level_name);
	y += 20; idx++,

	DrawKeyword(idx, ng_host_style, y, "MODE", GetModeName(host_welcome.mode));
	y += 10; idx++,

	DrawKeyword(idx, ng_host_style, y, "SKILL", GetSkillName(host_welcome.skill));
	y += 10; idx++,

	DrawKeyword(idx, ng_host_style, y, "BOTS",
			LocalPrintf(buffer, sizeof(buffer), "%d", host_welcome.bots));
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
			// FIXME: !!!! check for error
			N_OpenBroadcastSocket(true);

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


//----------------------------------------------------------------------------

void M_NetJoinBegun(void)
{
	netgame_we_host = true;

	// FIXME: check for -host option (short-circuit the BD spiel)

	join_pos = 0;
	join_discover_timer = 10 * TICRATE;
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

			S_StartFX(sfx_swtchn);
			netgame_menuon = 3;
			break;

		default:
			break;
	}
}


void M_DrawJoinMenu(void)
{
	netgame_we_host = false;

	ng_join_style->DrawBackground();

	HL_WriteText(ng_join_style,2, 80, 10, "JOIN NET GAME");

	char buffer[200];

	if (join_discover_timer > 0)
	{
		HL_WriteText(ng_join_style,3,  30, 160, "Looking for Host on LAN...");
		HL_WriteText(ng_join_style,1, 240, 160,
				LocalPrintf(buffer, sizeof(buffer), "%d",
					(join_discover_timer+TICRATE-1) / TICRATE));

		join_discover_timer--;
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

	HL_WriteText(ng_join_style,(join_pos==idx)?2:0, 30, y, "Connect to Host now");
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
	}

	return false;
}


//----------------------------------------------------------------------------

static void NetGameStartLevel(void)
{
	// -KM- 1998/12/17 Clear the intermission.
	WI_Clear();

	// create parameters

	if (! ParseWelcomePacket(ng_params, netgame_we_host ? &host_welcome : &join_welcome))
	{
		M_StartMessage(language["EpisodeNonExist"], NULL, false);
		return;
	}

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
		if (ng_params->players[i] & PFL_Bot)
			continue;

		DrawKeyword(-1, ng_list_style, y,
				LocalPrintf(buffer, sizeof(buffer), "Player%d", i),
				(ng_params->players[i] & PFL_Network) ? "IP 11.22.33.44" : "Local");
		y += 10;
	}
	
	if (netgame_we_host)
	{
		HL_WriteText(ng_list_style,2, 40, 140, "Press <ENTER> to Start Game");
	}
}

bool M_NetListResponder(event_t * ev, int ch)
{
	if (ch == KEYD_ENTER && netgame_we_host)
	{
		S_StartFX(sfx_swtchn);

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

	ng_params = new newgame_params_c();

	host_pos = 0;
	join_pos = 0;

	CreateHostWelcome(&host_welcome);

	hosting_port = MP_EDGE_PORT;
	joining_port = MP_EDGE_PORT;

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
			N_CloseBroadcastSocket();

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
	// FIXME: N_DoPacketStuff()

#if 0
	switch (netgame_menuon)
	{
		case 1: return M_NetHostTicker();
		case 2: return M_NetJoinTicker();
		case 3: return M_NetListTicker();
	}
#endif
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
