//----------------------------------------------------------------------------
//  EDGE Console Main
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

#include "system/i_defs.h"

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
#include "p_cheats.h"
// [SP] Externals
extern int debug_fps, debug_pos;

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
		for(size_t i=0; i < strlen(match); i++) {
			match[i] = toupper(match[i]);
		}
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

	cvar_link_t *link = all_cvars_list;

	while(link) {
		if (match && *match)
			if (! strstr(link->name, match))
				continue;

		cvar_c *var = link->var;

		if (show_defaults)
			I_Printf("  %-15s \"%s\" (%s)\n", link->name, var->get_casted_string(), var->get_casted_default_string()); // TODO: def_val
		else
			I_Printf("  %-15s \"%s\"\n", link->name, var->get_casted_string());


		total++;

		link = link->next;
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
	I_Printf("Welcome to the EDGE Console.\n");
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
	I_Printf("EDGE v" EDGEVERSTR "\n");
	//I_Printf("Platform " EDGEPRINTBIT "\n");
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

int CMD_ActivateCameraManSystem(char **argv, int argc)
{
	if (argc == 2)
	{
		int flag = atoi(argv[1]);
		cameraman::Activate(flag > 0 ? 1 : 0);
	}
	else
	{
		I_Printf("Usage: %s 1|0\n\tActivates/deactivates the camera-man system. By default it is deactivated.\n", argv[0]);
	}
	return 0;
}

int CMD_ResetCameraManSystem(char **argv, int argc)
{
	cameraman::Reset();
	I_Printf("Reset camera-man system to default values...\n");
	return 0;
}

int CMD_AddCameraMan(char **argv, int argc)
{
	if (argc >= 2)
	{		
		if (player_t *hero = players[0])
		{
			float x = hero->mo->x;
			float y = hero->mo->y;
			float z = hero->mo->z + hero->viewz;
			float ax = hero->mo->GetInterpolatedVertAngle();
			float ay = hero->mo->GetInterpolatedAngle();
			float fov = atof(argv[1]);

			int id = cameraman::Add(x, y, z, ax, ay, fov, (argc == 3) ? argv[2] : NULL);
			if (id > -1)
				I_Printf("Successfully added camera-man of ID: %d...\n", id);
			return id;
		}
		else
		{
			I_Printf("NO PLAYER!\n");
		}
	}
	return 0;
}

int CMD_RemoveCameraMan(char **argv, int argc)
{
	if (argc >= 2)
	{
		int id = atoi(argv[1]);

		if (argc == 3 && id < 0)
			id = cameraman::GetId(argv[2]);

		int state = cameraman::Remove(id);
		if (state > -1)
			CON_Printf("Successfully removed camera-man of ID: %d...\n", id);
		return state;
	}
	return 0;
}

int CMD_SwitchToCameraMan(char **argv, int argc)
{
	if (argc >= 2)
	{
		int id = atoi(argv[1]);

		if (argc == 3 && id < 0)
			id = cameraman::GetId(argv[2]);

		cameraman::SetStartId(id);
		cameraman::SetEndId(id);
	}
	return 0;
}

int CMD_SetStartCameraMan(char **argv, int argc)
{
	if (argc >= 2)
	{
		int id = atoi(argv[1]);

		if (argc == 3 && id < 0)
			id = cameraman::GetId(argv[2]);

		cameraman::SetStartId(id);
	}
	return 0;
}

int CMD_SetEndCameraMan(char **argv, int argc)
{
	if (argc >= 2)
	{
		int id = atoi(argv[1]);

		if (argc == 3 && id < 0)
			id = cameraman::GetId(argv[2]);

		cameraman::SetEndId(id);
	}
	return 0;
}

int CMD_SetCameraManPosition(char **argv, int argc)
{
	if (argc >= 2)
	{
		if (player_t *hero = players[0])
		{
			int id = atoi(argv[1]);

			if (argc == 3 && id < 0)
				id = cameraman::GetId(argv[2]);

			float x = hero->mo->x;
			float y = hero->mo->y;
			float z = hero->mo->z + hero->viewz;
			return cameraman::SetPosition(id, x, y, z);
		}
	}
	return 0;
}

int CMD_SetCameraManAngles(char **argv, int argc)
{
	if (argc >= 2)
	{
		if (player_t *hero = players[0])
		{
			int id = atoi(argv[1]);

			if (argc == 3 && id < 0)
				id = cameraman::GetId(argv[2]);

			float ax = hero->mo->GetInterpolatedVertAngle();
			float ay = hero->mo->GetInterpolatedAngle();
			return cameraman::SetAngles(id, ax, ay);
		}
	}
	return 0;
}

int CMD_SetCameraManFov(char **argv, int argc)
{
	if (argc >= 3)
	{
		int id = atoi(argv[1]);
		float fov = 0.0f;

		if (argc == 4 && id < 0)
		{
			id = cameraman::GetId(argv[2]);
			fov = atof(argv[3]);
		}
		else
		{
			fov = atof(argv[2]);
		}

		return cameraman::SetFov(id, fov);
	}
	return 0;
}

int CMD_SaveCameraManSystem(char **argv, int argc)
{
	cameraman::Serialize(0);
	return 0;
}

int CMD_LoadCameraManSystem(char **argv, int argc)
{
	cameraman::Serialize(1);
	return 0;
}

int CMD_PrintCameraManSystem(char **argv, int argc)
{
	cameraman::Print();
	return 0;
}

int CMD_ToggleHelpers(char **argv, int argc)
{
	cameraman::ToggleHelpers();
	return 0;
}

int CMD_SetCameraBobbing(char **argv, int argc)
{
	if (argc >= 2)
	{
		int flag = atoi(argv[1]);

		extern bool disable_bob;
		extern float bob_z_scale;
		extern float bob_r_scale;

		disable_bob = !(flag > 0);
		bob_z_scale = (disable_bob) ? 0.0f : 0.5f;
		bob_r_scale = (disable_bob) ? 0.0f : 0.2f;

		if (argc == 4)
		{
			bob_z_scale = atof(argv[2]);
			bob_r_scale = atof(argv[3]);
		}
	}
	return 0;
}

// [SP] --- BEGIN CHEAT CODES ---

int CMD_CheatGod(char **argv, int argc)
{
	if (!CheckCheats())
		return 0;

	Cheat_ToggleGodMode();

	return 0;
}

int CMD_CheatNoClip(char **argv, int argc)
{
	if (!CheckCheats())
		return 0;

	Cheat_NoClip();

	return 0;
}

int CMD_CheatAmmoNoKey(char **argv, int argc)
{
	if (!CheckCheats())
		return 0;

	Cheat_IDFA();

	return 0;
}

int CMD_CheatVeryHappyAmmo(char **argv, int argc)
{
	if (!CheckCheats())
		return 0;

	Cheat_IDKFA();

	return 0;
}

int CMD_CheatChainSaw(char **argv, int argc)
{
	if (!CheckCheats())
		return 0;

	Cheat_Choppers();

	return 0;
}

int CMD_CheatUnlockDoors(char **argv, int argc)
{
	if (!CheckCheats())
		return 0;

	Cheat_Unlock();

	return 0;
}

int CMD_CheatShowHOM(char **argv, int argc)
{
	if (!CheckCheats())
		return 0;

	Cheat_HOM();

	return 0;
}

int CMD_CheatShowPos(char **argv, int argc)
{
	if (!CheckCheats())
		return 0;

	Cheat_ShowPos();

	return 0;
}

int CMD_CheatKillAll(char **argv, int argc)
{
	if (!CheckCheats())
		return 0;

	Cheat_KillAll();

	return 0;
}

int CMD_CheatSuicide(char **argv, int argc)
{
	if (!CheckCheats())
		return 0;

	Cheat_Suicide();

	return 0;
}

int CMD_CheatInfo(char **argv, int argc)
{
	if (!CheckCheats())
		return 0;

	Cheat_Info();

	return 0;
}

int CMD_CheatLoaded(char **argv, int argc)
{
	if (!CheckCheats())
		return 0;

	Cheat_Loaded();

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

	// Camera-man system
	{ "actcam",   CMD_ActivateCameraManSystem },
	{ "rstcam",   CMD_ResetCameraManSystem },
	{ "addcam",   CMD_AddCameraMan },
	{ "remcam",   CMD_RemoveCameraMan },
	{ "swtcam",   CMD_SwitchToCameraMan },
	{ "strcam",   CMD_SetStartCameraMan },
	{ "endcam",   CMD_SetEndCameraMan },
	{ "poscam",   CMD_SetCameraManPosition },
	{ "angcam",   CMD_SetCameraManAngles },
	{ "fovcam",   CMD_SetCameraManFov },
	{ "savcam",   CMD_SaveCameraManSystem },
	{ "lddcam",	  CMD_LoadCameraManSystem },
	{ "prtcam",	  CMD_PrintCameraManSystem },

	{ "tghelp",   CMD_ToggleHelpers },

	{ "setbob",   CMD_SetCameraBobbing },

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
			I_Printf("%s \"%s\"\n", argv[0], link->var->get_casted_string());
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

	Z_Clear(buffer, char, 256); //TODO: V782 https://www.viva64.com/en/w/v782/ There is no sense in evaluating the distance between elements from different arrays: '(buffer) - ((char *)(buffer))'.

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

	Z_Clear(buffer, char, 256); //TODO: V782 https://www.viva64.com/en/w/v782/ There is no sense in evaluating the distance between elements from different arrays: '(buffer) - ((char *)(buffer))'.

	va_start(argptr, lookup);
	vsprintf(buffer, lookup, argptr);
	va_end(argptr);

	CON_Message("%s", buffer);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
