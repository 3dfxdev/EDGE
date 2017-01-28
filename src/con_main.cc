//----------------------------------------------------------------------------
//  EDGE2 Console Main
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2009  The EDGE2 Team.
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

#include "i_defs.h"

#include "../epi/filesystem.h"
#include "../epi/math_crc.h"

#include "../ddf/language.h"
#include "../ddf/sfx.h"

#include "con_main.h"
#include "con_var.h"

#include "e_input.h"
#include "g_game.h"
#include "m_menu.h"
#include "m_misc.h"
#include "s_sound.h"
#include "w_wad.h"
#include "version.h"
#include "z_zone.h"

// [SP] Headers for cheat codes (need access to player and mobjs)
#include "p_local.h"
#include "p_mobj.h"
#include "p_bot.h"
#include "dm_state.h"
// [SP] Externals
extern cvar_c debug_fps, debug_pos;

#define MAX_CON_ARGS  64


typedef struct
{
	const char *name;

	int (* func)(char **argv, int argc);
}
con_cmd_t;


// forward decl.
extern const con_cmd_t builtin_commands[];

extern void M_ChangeLevelCheat(const char *string);
extern void I_ShowJoysticks(void);
extern void M_QuitFinally(void);


int CMD_Exec(char **argv, int argc)
{
	if (argc != 2)
	{
		CON_Printf("Usage: exec <script.cfg>\n");
		return 1;
	}

	FILE *script = fopen(argv[1], "rb");
	if (!script)
	{
		CON_Printf("Unable to open file: %s\n", argv[1]);
		return 1;
	}

	char buffer[200];

	while (fgets(buffer, sizeof(buffer)-1, script))
	{
		CON_TryCommand(buffer);
	}

	fclose(script);
	return 0;
}

int CMD_Type(char **argv, int argc)
{
	FILE *script;
	char buffer[200];

	if (argc != 2)
	{
		CON_Printf("Usage: type <filename.txt>\n");
		return 2;
	}

	script = fopen(argv[1], "r");
	if (!script)
	{
		CON_Printf("Unable to open \'%s\'!\n", argv[1]);
		return 3;
	}
	while (fgets(buffer, sizeof(buffer)-1, script))
	{
		CON_Printf("%s", buffer);
	}
	fclose(script);

	return 0;
}

int CMD_Dir(char **argv, int argc)
{
	const char *path = ".";
	const char *mask = "*.*";

	if (argc >= 2)
		path = argv[1];

	if (argc >= 3)
		mask = argv[2];

	I_Printf("Directory contents matching %s\n", mask);

	epi::filesystem_dir_c fsd;

	if (! FS_ReadDir(&fsd, path, mask))
	{
		I_Printf("Failed to read dir: %s\n", path);
		return 1;
	}

	for (int i = 0; i < fsd.GetSize(); i++)
	{
		epi::filesys_direntry_c *entry = fsd[i];

		I_Printf("  %2d: %10d %s '%s'\n",
				 i+1, entry->size,
				 entry->is_dir ? "DIR" : "   ",
				 entry->name.c_str());
	}

	return 0;
}

int CMD_ArgList(char **argv, int argc)
{
	I_Printf("Arguments:\n");

	for (int i = 0; i < argc; i++)
		I_Printf(" %2d len:%d text:\"%s\"\n", i, (int)strlen(argv[i]), argv[i]);

	return 0;
}

int CMD_ScreenShot(char **argv, int argc)
{
	G_DeferredScreenShot();

	return 0;
}

int CMD_QuitEDGE(char **argv, int argc)
{
#if 0
	if (argc >= 2 && stricmp(argv[1], "now") == 0)
	{
		// this never returns
		M_QuitFinally();
	}
#endif
	M_QuitEDGE(0);

	return 0;
}


int CMD_Crc(char **argv, int argc)
{
	int lump, length;
	const byte *data;

	if (argc < 2)
	{
		CON_Printf("Usage: crc <lump>\n");
		return 1;
	}

	for (int i = 1; i < argc; i++)
	{
		lump = W_CheckNumForName(argv[i]);

		if (lump == -1)
		{
			CON_Printf("No such lump: %s\n", argv[i]);
		}
		else
		{
			length = W_LumpLength(lump);
			data = (const byte*)W_LoadLumpNum(lump);

			epi::crc32_c result;

			result.Reset();
			result.AddBlock(data, length);

			Z_Free((void*)data);

			CON_Printf("  %s  %d bytes  crc = %08x\n", argv[i], length, result.crc);
		}
	}

	return 0;
}

int CMD_PlaySound(char **argv, int argc)
{
	sfx_t *sfx;

	if (argc != 2)
	{
		CON_Printf("Usage: playsound <name>\n");
		return 1;
	}

	sfx = sfxdefs.GetEffect(argv[1], false);
	if (! sfx)
	{
		CON_Printf("No such sound: %s\n", argv[1]);
	}
	else
	{
		S_StartFX(sfx, SNCAT_UI);
	}

	return 0;
}

int CMD_ResetVars(char **argv, int argc)
{
	CON_ResetAllVars();
	M_ResetDefaults(0);
	return 0;
}

int CMD_ShowFiles(char **argv, int argc)
{
	W_ShowFiles();
	return 0;
}

int CMD_ShowLumps(char **argv, int argc)
{
	int for_file = -1;  // all files

	char *match = NULL;

	if (argc >= 2 && isdigit(argv[1][0]))
		for_file = atoi(argv[1]);

	if (argc >= 3)
	{
		match = argv[2];
		strupr(match);
	}

	W_ShowLumps(for_file, match);
	return 0;
}

int CMD_ShowVars(char **argv, int argc)
{
	bool show_defaults = false;

	char *match = NULL;

	if (argc >= 2 && stricmp(argv[1], "-l") == 0)
	{
		show_defaults = true;
		argv++; argc--;
	}

	if (argc >= 2)
		match = argv[1];

	I_Printf("Console Variables:\n");

	int total = 0;

	for (int i = 0; all_cvars[i].name; i++)
	{
		if (match && *match)
			if (! strstr(all_cvars[i].name, match))
				continue;

		cvar_c *var = all_cvars[i].var;

		if (show_defaults)
			I_Printf("  %-15s \"%s\" (%s)\n", all_cvars[i].name, var->str, all_cvars[i].def_val);
		else
			I_Printf("  %-15s \"%s\"\n", all_cvars[i].name, var->str);

		total++;
	}

	if (total == 0)
		I_Printf("Nothing matched.\n");

	return 0;
}

int CMD_ShowCmds(char **argv, int argc)
{
	char *match = NULL;

	if (argc >= 2)
		match = argv[1];

	I_Printf("Console Commands:\n");

	int total = 0;

	for (int i = 0; builtin_commands[i].name; i++)
	{
		if (match && *match)
			if (! strstr(builtin_commands[i].name, match))
				continue;

		I_Printf("  %-15s\n", builtin_commands[i].name);
		total++;
	}

	if (total == 0)
		I_Printf("Nothing matched.\n");

	return 0;
}

int CMD_ShowKeys(char **argv, int argc)
{
#if 0  // TODO
	char *match = NULL;

	if (argc >= 2)
		match = argv[1];

	I_Printf("Key Bindings:\n");

	int total = 0;

	for (int i = 0; all_binds[i].name; i++)
	{
		if (match && *match)
			if (! strstr(all_binds[i].name, match))
				continue;

		std::string keylist = all_binds[i].bind->FormatKeyList();

		I_Printf("  %-15s %s\n", all_binds[i].name, keylist.c_str());
		total++;
	}

	if (total == 0)
		I_Printf("Nothing matched.\n");
#endif
	return 0;
}

int CMD_ShowJoysticks(char **argv, int argc)
{
	I_ShowJoysticks();
	return 0;
}


int CMD_Help(char **argv, int argc)
{
	I_Printf("Welcome to the EDGE2 Console.\n");
	I_Printf("\n");
	I_Printf("Use the 'showcmds' command to list all commands.\n");
	I_Printf("The 'showvars' command will list all variables.\n");
	I_Printf("Both of these can take a keyword to match the names with.\n");
	I_Printf("\n");
	I_Printf("To show the value of a variable, just type its name.\n");
	I_Printf("To change it, follow the name with a space and the new value.\n");
	I_Printf("\n");
	I_Printf("Press ESC key to close the console.\n");
	I_Printf("The PGUP and PGDN keys scroll the console up and down.\n");
	I_Printf("The UP and DOWN arrow keys let you recall previous commands.\n");
	I_Printf("\n");
	I_Printf("Have a nice day!\n");

	return 0;
}

int CMD_Version(char **argv, int argc)
{
	I_Printf("3DGE v" EDGEVERSTR "\n");
	return 0;
}


int CMD_Map(char **argv, int argc)
{
	if (argc <= 1)
	{
		CON_Printf("Usage: map <level>\n");
		return 0;
	}

	M_ChangeLevelCheat(argv[1]);
	return 0;
}


// [SP] --- BEGIN CHEAT CODES ---
// TODO: There's *WAY* too much code duplication here. The code in both the menu and the console need to be exported to
// a single file and called appropriately.
bool CheckCheats()
{
#ifdef NOCHEATS
	// we should never see this message, since this define removes the console entries anyway...
	I_Printf("Cheats are disabled in this build!\n");
	return false;
#endif

	// disable cheats while in RTS menu, or demos
	if (rts_menuactive)
		return false;

	// no cheating in netgames or if disallowed in levels.ddf
	if (!level_flags.cheats)
	{
		I_Printf("Cheats are currently disabled here!\n");
		return false;
	}

#if 0 //!!!! TEMP DISABLED, NETWORK DEBUGGING
	if (netgame)
	{
		I_Printf("Cheats are disabled in net game!\n");
		return false;
	}
#endif
	return true;
}

static void CheatGiveWeapons(player_t *pl, int key = -2)
{
	epi::array_iterator_c it;
	
	for (it = weapondefs.GetIterator(0); it.IsValid(); it++)
	{
		weapondef_c *info = ITERATOR_TO_TYPE(it, weapondef_c*);

		if (info && !info->no_cheat && (key<0 || info->bind_key==key))
		{
			P_AddWeapon(pl, info, NULL);
		}
	}

	if (key < 0)
	{
		for (int slot=0; slot < MAXWEAPONS; slot++)
		{
			if (pl->weapons[slot].info)
				P_FillWeapon(pl, slot);
		}
	}

	P_UpdateAvailWeapons(pl);
}

int CMD_CheatGod(char **argv, int argc)
{
	if (!CheckCheats())
		return 0;

	player_t *pl = players[consoleplayer1];

	pl->cheats ^= CF_GODMODE;
	if (pl->cheats & CF_GODMODE)
	{
		if (pl->mo)
		{
			pl->health = pl->mo->health = pl->mo->info->spawnhealth;
		}
		CON_MessageLDF("GodModeOn");
	}
	else
		CON_MessageLDF("GodModeOff");
	return 0;
}

int CMD_CheatNoClip(char **argv, int argc)
{
	if (!CheckCheats())
		return 0;

	player_t *pl = players[consoleplayer1];

	pl->cheats ^= CF_NOCLIP;

	if (pl->cheats & CF_NOCLIP)
		CON_MessageLDF("ClipOn");
	else
		CON_MessageLDF("ClipOff");
	return 0;
}

int CMD_CheatAmmoNoKey(char **argv, int argc)
{
	if (!CheckCheats())
		return 0;

	player_t *pl = players[consoleplayer1];

	pl->armours[CHEATARMOURTYPE] = CHEATARMOUR;

	P_UpdateTotalArmour(pl);

	for (int i = 0; i < NUMAMMO; i++)
		pl->ammo[i].num = pl->ammo[i].max;

	CheatGiveWeapons(pl);

	CON_MessageLDF("AmmoAdded");

	return 0;
}

int CMD_CheatVeryHappyAmmo(char **argv, int argc)
{
	if (!CheckCheats())
		return 0;

	player_t *pl = players[consoleplayer1];

	pl->armours[CHEATARMOURTYPE] = CHEATARMOUR;

	P_UpdateTotalArmour(pl);

	for (int i = 0; i < NUMAMMO; i++)
		pl->ammo[i].num = pl->ammo[i].max;

	pl->cards = KF_MASK;

	CheatGiveWeapons(pl);

	CON_MessageLDF("VeryHappyAmmo");

	return 0;
}

int CMD_CheatChainSaw(char **argv, int argc)
{
	if (!CheckCheats())
		return 0;

	player_t *pl = players[consoleplayer1];
	weapondef_c *w = weapondefs.Lookup("CHAINSAW");

	if (w)
	{
		P_AddWeapon(pl, w, NULL);
		pl->powers[PW_Invulnerable] = 1;
		CON_MessageLDF("CHOPPERSNote");
	}

	return 0;
}

int CMD_CheatUnlockDoors(char **argv, int argc)
{
	if (!CheckCheats())
		return 0;

	player_t *pl = players[consoleplayer1];

	pl->cards = KF_MASK;

	CON_MessageLDF("UnlockCheat");

	return 0;
}

int CMD_CheatShowHOM(char **argv, int argc)
{
	if (!CheckCheats())
		return 0;

	debug_hom = debug_hom.d ? 0 : 1;

	if (debug_hom.d)
		CON_MessageLDF("HomDetectOn");
	else
		CON_MessageLDF("HomDetectOff");

	return 0;
}

int CMD_CheatShowPos(char **argv, int argc)
{
	if (!CheckCheats())
		return 0;

	player_t *pl = players[consoleplayer1];

	CON_Message("ang=%f;x,y=(%f,%f)",
		ANG_2_FLOAT(pl->mo->angle), pl->mo->x, pl->mo->y);

	return 0;
}

int CMD_CheatKillAll(char **argv, int argc)
{
	if (!CheckCheats())
		return 0;

	int killcount = 0;

	mobj_t *mo;
	mobj_t *next;

	for (mo = mobjlisthead; mo; mo = next)
	{
		next = mo->next;

		if ((mo->extendedflags & EF_MONSTER) && (mo->health > 0))
		{
			P_TelefragMobj(mo, NULL, NULL);
			killcount++;
		}
	}

	CON_MessageLDF("MonstersKilled", killcount);

	return 0;
}

int CMD_CheatSuicide(char **argv, int argc)
{
	if (!CheckCheats())
		return 0;

	player_t *pl = players[consoleplayer1];

	P_TelefragMobj(pl->mo, pl->mo, NULL);

	CON_MessageLDF("SuicideCheat");

	return 0;
}

int CMD_CheatInfo(char **argv, int argc)
{
	if (!CheckCheats())
		return 0;

	debug_fps = debug_fps.d ? 0 : 1;
	debug_pos = debug_fps.d;

	return 0;
}

int CMD_CheatLoaded(char **argv, int argc)
{
	if (!CheckCheats())
		return 0;

	player_t *pl = players[consoleplayer1];

	for (int i = 0; i < NUMAMMO; i++)
		pl->ammo[i].num = pl->ammo[i].max;

	CON_MessageLDF("LoadedCheat");

	return 0;
}



//----------------------------------------------------------------------------

// oh lordy....
static char *StrDup(const char *start, int len)
{
	char *buf = new char[len + 2];

	memcpy(buf, start, len);
	buf[len] = 0;

	return buf;
}

static int GetArgs(const char *line, char **argv, int max_argc)
{
	int argc = 0;

	for (;;)
	{
		while (isspace(*line))
			line++;

		if (! *line)
			break;

		// silent truncation (bad?)
		if (argc >= max_argc)
			break;

		const char *start = line;

		if (*line == '"')
		{
			start++; line++;

			while (*line && *line != '"')
				line++;
		}
		else
		{
			while (*line && !isspace(*line))
				line++;
		}

		// ignore empty strings at beginning of the line
		if (! (argc == 0 && start == line))
		{
			argv[argc++] = StrDup(start, line - start);
		}

		if (*line)
			line++;
	}

	return argc;
}

static void KillArgs(char **argv, int argc)
{
	for (int i = 0; i < argc; i++)
		delete[] argv[i];
}


//
// Current console commands:
//
const con_cmd_t builtin_commands[] =
{
	{ "args",           CMD_ArgList },
	{ "crc",            CMD_Crc },
	{ "dir",            CMD_Dir },
	{ "exec",           CMD_Exec },
	{ "help",           CMD_Help },
	{ "map",            CMD_Map },
	{ "warp",           CMD_Map },  // compatibility
	{ "playsound",      CMD_PlaySound },
//	{ "resetkeys",      CMD_ResetKeys },
	{ "resetvars",      CMD_ResetVars },
	{ "showfiles",      CMD_ShowFiles },
  	{ "showjoysticks",  CMD_ShowJoysticks },
//	{ "showkeys",       CMD_ShowKeys },
	{ "showlumps",      CMD_ShowLumps },
	{ "showcmds",       CMD_ShowCmds },
	{ "showvars",       CMD_ShowVars },
	{ "screenshot",     CMD_ScreenShot },
	{ "type",           CMD_Type },
	{ "version",        CMD_Version },
	{ "quit",           CMD_QuitEDGE },
	{ "exit",           CMD_QuitEDGE },

#ifndef NOCHEATS
	// [SP] Cheats
	{ "god",            CMD_CheatGod },
	{ "noclip",         CMD_CheatNoClip },
	{ "idfa",           CMD_CheatAmmoNoKey },
	{ "idkfa",          CMD_CheatVeryHappyAmmo },
	{ "idchoppers",     CMD_CheatChainSaw },
	{ "idunlock",       CMD_CheatUnlockDoors },
	{ "showhom",        CMD_CheatShowHOM },
	{ "showpos",        CMD_CheatShowPos },
	{ "killall",        CMD_CheatKillAll },
	{ "massacre",       CMD_CheatKillAll },
	{ "kill",           CMD_CheatSuicide },
	{ "suicide",        CMD_CheatSuicide },
	{ "info",           CMD_CheatInfo },
	{ "loaded",         CMD_CheatLoaded },
#endif

	// end of list
	{ NULL, NULL }
};


static int FindCommand(const char *name)
{
	for (int i = 0; builtin_commands[i].name; i++)
	{
		if (stricmp(name, builtin_commands[i].name) == 0)
			return i;
	}

	return -1;  // not found
}

#if 0
static void ProcessBind(key_link_t *link, char **argv, int argc)
{
	for (int i = 1; i < argc; i++)
	{
		if (stricmp(argv[i], "-c") == 0)
		{
			link->bind->Clear();
			continue;
		}

		int keyd = E_KeyFromName(argv[i]);
		if (keyd == 0)
		{
			CON_Printf("Invalid key name: %s\n", argv[i]);
			continue;
		}

		link->bind->Toggle(keyd);
	}
}
#endif

void CON_TryCommand(const char *cmd)
{
	char *argv[MAX_CON_ARGS];
	int argc = GetArgs(cmd, argv, MAX_CON_ARGS);

	if (argc == 0)
		return;

	int index = FindCommand(argv[0]);
	if (index >= 0)
	{
		(* builtin_commands[index].func)(argv, argc);

		KillArgs(argv, argc);
		return;
	}

	cvar_link_t *link = CON_FindVar(argv[0]);
	if (link)
	{
		if (argc <= 1)
			I_Printf("%s \"%s\"\n", argv[0], link->var->str);
		else if (argc >= 3)
			I_Printf("Can only assign one value (%d given).\n", argc-1);
		else if (strchr(link->flags, 'r'))
			I_Printf("That cvar is read only.\n");
		else
			*link->var = argv[1];

		KillArgs(argv, argc);
		return;
	}

#if 0
	// hmmm I like it kinky...
	key_link_t *kink = E_FindKeyBinding(argv[0]);
	if (kink)
	{
		if (argc <= 1)
		{
			std::string keylist = kink->bind->FormatKeyList();

			I_Printf("%s %s\n", argv[0], keylist.c_str());
		}
		else
		{
			ProcessBind(kink, argv, argc);
		}

		KillArgs(argv, argc);
		return;
	}
#endif

	I_Printf("Unknown console command: %s\n", argv[0]);

	KillArgs(argv, argc);
	return;
}


int CON_MatchAllCmds(std::vector<const char *>& list,
                     const char *pattern)
{
	list.clear();

	for (int i = 0; builtin_commands[i].name; i++)
	{
		if (! CON_MatchPattern(builtin_commands[i].name, pattern))
			continue;

		list.push_back(builtin_commands[i].name);
	}

	return (int)list.size();
}


//
// CON_PlayerMessage
//
// -ACB- 1999/09/22 Console Player Message Only. Changed from
//                  #define to procedure because of compiler
//                  differences.
//
void CON_PlayerMessage(int plyr, const char *message, ...)
{
	va_list argptr;
	char buffer[256];

	if (consoleplayer1 != plyr)
		return;

	Z_Clear(buffer, char, 256);

	va_start(argptr, message);
	vsprintf(buffer, message, argptr);
	va_end(argptr);

	CON_Message("%s", buffer);
}

//
// CON_PlayerMessageLDF
//
// -ACB- 1999/09/22 Console Player Message Only. Changed from
//                  #define to procedure because of compiler
//                  differences.
//
void CON_PlayerMessageLDF(int plyr, const char *lookup, ...)
{
	va_list argptr;
	char buffer[256];

	if (consoleplayer1 != plyr)
		return;

	lookup = language[lookup];

	Z_Clear(buffer, char, 256);

	va_start(argptr, lookup);
	vsprintf(buffer, lookup, argptr);
	va_end(argptr);

	CON_Message("%s", buffer);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
