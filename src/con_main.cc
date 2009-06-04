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




int CMD_Exec(const char **argv, int argc)
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

int CMD_Type(const char **argv, int argc)
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

int CMD_DumpArgs(const char **argv, int argc)
{
	I_Printf("Arguments:\n");

	for (int i = 0; i < argc; i++)
		I_Printf(" %2d len:%d text:\"%s\"\n", i, (int)strlen(argv[i]), argv[i]);

	return 0;
}

//
// Eats memory.
//
int CMD_Eat(const char **argv, int argc)
{
	int bytes;
	static char *p = NULL;

	if (argc != 2 || 1 != sscanf(argv[1], "%d", &bytes))
	{
		CON_Printf("Eat memory. Usage: eat <size>\n");
		return 2;
	}

	if (!bytes)
	{
		Z_Free(p);
		p = NULL;
	}
	else
		Z_Resize(p, char, bytes);

	return 0;
}

int CMD_ScreenShot(const char **argv, int argc)
{
	G_DeferredScreenShot();

	return 0;
}

int CMD_Set(const char **argv, int argc)
{
	cvar_t *var;
	// temp argv: Just to handle 'const' keyword properly
	char *tmpargv[3];
	int i;
	int e = 0;
	char buf[1025];

	for (i = 0; i < argc; i++)
		argv[i] = tmpargv[i];

	switch (argc)
	{
	case 2:
	case 1:
		for (i = 0; i < num_cvars; i++)
		{
			if (argc == 2)
				if (stricmp(argv[1], cvars[i]->name))
					continue;
			if (cvars[i]->flags & cf_read)
			{
				cvars[i]->type->get_value_str(cvars[i]->value, buf);
				CON_Printf("%s = %s\n", cvars[i]->name, buf);
			}
			else
				CON_Printf("%s (Write Only)\n", cvars[i]->name);
			if (argc == 2)
				break;
		}
		if (i == num_cvars && argc == 2)
		{  // not found

			CON_Printf("No cvar '%s'!\n", argv[1]);
			e = 3;
		}
		break;
	default:
		var = CON_CVarPtrFromName(argv[1]);
		if (var)
		{
			if (!strlen(argv[2]) && var->flags & cf_delete)
				CON_DeleteCVar(argv[1]);
			else
			{
				if (var->flags & cf_write)
				{
					if (!var->type->set_value(var, argc - 2, &argv[2]))
						e = 1, CON_Printf("\"%s\" is not a valid value for type %s\n", argv[2], var->type->get_name(var));
				}
				else
					e = 2, CON_Printf("\"%s\" is read only\n", var->name);
			}
		}
		else
		{
			if (argv[2][0] >= '0' && argv[2][0] <= '9')
			{
				int *x = Z_New(int, 1);

				*x = strtol(argv[2], NULL, 0);
				CON_CreateCVarInt(argv[1], (cflag_t) (cf_delete|cf_mem|cf_normal), x);
			}
			else if (!strcmp(argv[2], "true") || !strcmp(argv[2], "false"))
			{
				bool *b = Z_New(bool, 1);

				*b = !strcmp(argv[2], "true");
				CON_CreateCVarBool(argv[1], (cflag_t) (cf_delete|cf_mem|cf_normal), b);
			}
			else
			{
				char *s = Z_New(char, 256);

				strcpy(s, argv[2]);
				CON_CreateCVarStr(argv[1], (cflag_t) (cf_delete|cf_mem|cf_normal), s, 255);
			}
		}
		break;
	}

	return e;
}

int CMD_TypeOf(const char **argv, int argc)
{
	cvar_t *v;
	int e;

	if (argc < 2)
	{
		CON_Printf("Usage: Typeof <cvar>\n");
		e = 1;
	}
	else
	{
		v = CON_CVarPtrFromName(argv[1]);

		if (v)
		{
			CON_Printf("The type of \"%s\" is \"%s\"\n", v->name, v->type->get_name(v));
			e = 0;
		}
		else
		{
			CON_Printf("No console variable called \"%s\"!\n", argv[1]);
			e = 2;
		}
	}

	return e;
}


int CMD_QuitEDGE(const char **argv, int argc)
{
	M_QuitEDGE(0);

	return 0;
}


int CMD_Crc(const char **argv, int argc)
{
	int lump, length;
	const byte *data;

	if (argc != 2)
	{
		CON_Printf("Usage: crc <lump>\n");
		return 1;
	}

	lump = W_CheckNumForName(argv[1]);

	if (lump == -1)
	{
		CON_Printf("No such lump: %s\n", argv[1]);
	}
	else
	{
		data = (const byte*)W_CacheLumpNum(lump);
		length = W_LumpLength(lump);

		epi::crc32_c result;

		result.Reset();
		result.AddBlock(data, length);

		W_DoneWithLump(data);

		CON_Printf("  %s  %d bytes  crc = %08x\n", argv[1], length, result.crc);
	}

	return 0;
}

int CMD_PlaySound(const char **argv, int argc)
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

int CMD_ShowFiles(const char **argv, int argc)
{
	W_ShowFiles();
	return 0;
}

int CMD_ShowLumps(const char **argv, int argc)
{
	int for_file = -1;  // all files

	char *match = NULL;

	if (argc >= 2 && isdigit(argv[1][0]))
		for_file = atoi(argv[1]);

	if (argc >= 3)
	{
		match = strdup(argv[2]);
		strupr(match);
	}

	W_ShowLumps(for_file, match);

	if (match)
		free(match);

	return 0;
}

int CMD_ShowVars(const char **argv, int argc)
{
	I_Printf("All console vars:\n");

	for (int i = 0; all_cvars[i].name; i++)
	{
		cvar_c *var = all_cvars[i].var;

		I_Printf("  %-15s \"%s\"\n", all_cvars[i].name, var->str);
	}

	return 0;
}


//----------------------------------------------------------------------------


static int GetArgs(const char *line, char **argv, int max_argc)
{
	while (isspace(*line))
		line++;

	int i, j, k = 0, m;
	const int len = (const int)strlen(line);
	char *s;
	int quote;

	// skip any leading spaces
	for (i = 0; args[i] == ' '; i++)
		;

	for (; i < len; i = j)
	{
		// find end of arg
		if (args[i] == '\"')
		{
			quote = 1;
			i++;
			for (j = i; args[j] != '\"' && j < len; j++)
				;
		} else
		{
			quote = 0;
			for (j = i; args[j] != ' ' && j < len; j++)
				;
		}

		s = Z_New(char, j - i + 1);
		for (m = 0; m < j - i; m++)
		{
			// -ES- 1999/07/25 Convert to lowercase, to avoid case sensitivity
			s[m] = tolower(args[i + m]);
		}
		s[j - i] = '\0';

		argv[k] = s;

		if (++k == argc)
			break;

		if (quote)
			// skip the ending quote
			j++;

		// skip whitespace until the next arg
		while (args[j] == ' ' && j < len)
			j++;
	}

	return k;
}

static void KillArgs(char **argv, int argc)
{
	for (int i = 0; i < argc; i++)
		free(argv[i]);
}


typedef struct
{
	const char *name;

	int (* func) (const char **argv, int argc);
}
con_cmd_t;

//
// Current console commands:
//
static const con_cmd_t builtin_commands[] =
{
	{ "args",           CMD_DumpArgs },
	{ "crc",            CMD_Crc },
	{ "playsound",      CMD_PlaySound },
	{ "eat",            CMD_Eat },
	{ "exec",           CMD_Exec },
	{ "show_files",     CMD_ShowFiles },
	{ "show_lumps",     CMD_ShowLumps },
	{ "show_vars",      CMD_ShowVars },
	{ "screenshot",     CMD_ScreenShot },
	{ "set",            CMD_Set },
	{ "type",           CMD_Type },
	{ "typeof",         CMD_TypeOf },
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

	int cmd = FindCommand(argv[0]);

	if (cmd >= 0)
	{
		int err = (* builtin_commands[cmd].func)(argv, argc);
	}
	else
	{
		CON_Printf("Unknown Command.\n");
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
