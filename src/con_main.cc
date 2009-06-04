//----------------------------------------------------------------------------
//  EDGE Console Main
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

#include "i_defs.h"

#include "epi/math_crc.h"

#include "con_main.h"
#include "con_defs.h"
#include "con_var.h"

#include "g_game.h"
#include "m_menu.h"
#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"


#define SCREENROWS 100
#define SCREENCOLS 80
#define BACKBUFFER 10


#define MAX_CON_ARGS  64




int CMD_Exec(char **argv, int argc)
{
	FILE *script;
	char buffer[SCREENCOLS];

	if (argc != 2)
	{
		CON_Printf("Usage: exec <script name.cfg>\n");
		return 1;
	}

	script = fopen(argv[1], "rb");
	if (!script)
	{
		CON_Printf("Unable to open \'%s\'!\n", argv[1]);
		return 1;
	}

	while (fgets(buffer, SCREENCOLS - 1, script))
	{
		CON_TryCommand(buffer);
	}

	fclose(script);
	return 0;
}

int CMD_Type(char **argv, int argc)
{
	FILE *script;
	char buffer[SCREENCOLS];

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
	while (fgets(buffer, SCREENCOLS - 1, script))
	{
		CON_Printf("%s", buffer);
	}
	fclose(script);

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
			data = (const byte*)W_CacheLumpNum(lump);
			length = W_LumpLength(lump);

			epi::crc32_c result;

			result.Reset();
			result.AddBlock(data, length);

			W_DoneWithLump(data);

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
	char *match = NULL;

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

		I_Printf("  %-15s \"%s\"\n", all_cvars[i].name, var->str);
		total++;
	}

	if (total == 0)
		I_Printf("Nothing matched.\n");

	return 0;
}


//----------------------------------------------------------------------------

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


typedef struct
{
	const char *name;

	int (* func) (char **argv, int argc);
}
con_cmd_t;

//
// Current console commands:
//
static const con_cmd_t builtin_commands[] =
{
	{ "args",           CMD_ArgList },
	{ "crc",            CMD_Crc },
	{ "playsound",      CMD_PlaySound },
	{ "exec",           CMD_Exec },
	{ "showfiles",      CMD_ShowFiles },
	{ "showlumps",      CMD_ShowLumps },
	{ "showvars",       CMD_ShowVars },
	{ "screenshot",     CMD_ScreenShot },
	{ "type",           CMD_Type },
	{ "quit",           CMD_QuitEDGE },
	{ "exit",           CMD_QuitEDGE },

	// end of list
	{ NULL, NULL }
};


static int FindCommand(const char *name)
{
	for (int i = 0; builtin_commands[i].name; i++)
	{
		if (strcmp(name, builtin_commands[i].name) == 0)
			return i;
	}

	return -1;  // not found
}

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
	}
	else
	{
		cvar_link_t *link = CON_FindVar(argv[0]);

		if (! link)
			I_Printf("Unknown Command or Variable.\n");
		else if (argc <= 1)
			I_Printf("%s \"%s\"\n", argv[0], link->var->str);
		else if (argc >= 3)
			I_Printf("Can only assign one value (%d given).\n", argc-1);
		else if (strchr(link->flags, 'r'))
			I_Printf("That cvar is read only.\n");
		else
			*link->var = argv[1];
	}

	KillArgs(argv, argc);
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

	if (consoleplayer != plyr)
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

	if (consoleplayer != plyr)
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
