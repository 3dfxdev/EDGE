//----------------------------------------------------------------------------
//  EDGE Console Main
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

#include "i_defs.h"
#include "con_main.h"

#include "con_defs.h"
#include "g_game.h"
#include "gui_gui.h"
#include "gui_ctls.h"
#include "m_menu.h"
#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"

#include "epi/math_crc.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREENROWS 100
#define SCREENCOLS 80
#define BACKBUFFER 10

typedef struct
{
	char *name;
	int flags;
	int (*cmd) (const char *arg);
}
con_cmd_t;

int CON_CMDToggleMouse(const char *args);
int CON_CMDHelloWorld(const char *args);
int CON_CMDEat(const char *args);
int CON_CMDExec(const char *args);
int CON_CMDArgText(const char *args);
int CON_CMDType(const char *args);
int CON_CMDTypeOf(const char *args);
int CON_CMDScreenShot(const char *args);
int CON_CMDSet(const char *args);
int CON_CMDWatch(const char *args);
int CON_CMDQuitEDGE(const char *args);
int CON_CMDCrc(const char *args);
int CON_CMDPlaySound(const char *args);

bool CON_CMDHelloWorldResponder(gui_t * gui, guievent_t * ev);

// Current console commands.  Needs extending badly.
// 'Builtin' commands should be added here
// TODO: add another file (i_exec.c) that can load in
//   'external' commands.  On a real operating system,
//   these 'external' commands could be loaded in using
//   dynamic linking, on DOS they must be linked statically.
con_cmd_t consolecommands[] =
{
	{"args", 0, CON_CMDArgText},
	{"crc", 0, CON_CMDCrc},
	{"playsound", 0, CON_CMDPlaySound},
	{"eat", 0, CON_CMDEat},
	{"exec", 0, CON_CMDExec},
	{"mouse", 0, CON_CMDToggleMouse},
	{"screenshot", 0, CON_CMDScreenShot},
	{"set", 0, CON_CMDSet},
	{"test", 0, CON_CMDHelloWorld},
	{"type", 0, CON_CMDType},
	{"typeof", 0, CON_CMDTypeOf},
	{"quit", 0, CON_CMDQuitEDGE},
	{"watch", 0, CON_CMDWatch},
};

static int GetArgs(const char *args, int argc, char **argv)
{
	int i, j, k = 0, m;
	const int len = (const int)strlen(args);
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

static void KillArgs(int argc, char *(argv[]))
{
	int i;

	for (i = 0; i < argc; i++)
		Z_Free(argv[i]);
}

void CON_TryCommand(const char *cmd)
{
	int i, j, e;
	char *s;
	const char *c;

	while (isspace(*cmd))
		cmd++;

	if (strlen(cmd) == 0)
		return;

	for (i = sizeof(consolecommands) / sizeof(consolecommands[0]); i--;)
	{
		s = consolecommands[i].name;
		c = cmd;
		for (j = strlen(consolecommands[i].name); j--; s++, c++)
			if (*s != *c)
				break;
		if (j == -1 && (!*c || *c == ' '))
		{
			e = consolecommands[i].cmd(cmd);
			if (e)
				CON_Printf("Error %d\n", e);
			return;
		}
	}
	CON_Printf("Bad Command.\n");
}

int CON_CMDToggleMouse(const char *args)
{
	bool vis = !GUI_MainGetMouseVisibility();

	GUI_MainSetMouseVisibility(vis);
	CON_Printf("Mouse: %s\n", vis ? "on" : "off");
	return 0;
}

bool CON_CMDHelloWorldResponder(gui_t * gui, guievent_t * ev)
{
	switch (ev->type)
	{
	case 112:
		GUI_Destroy(gui);
		return true;
	default:
		return false;
	}
}

int CON_CMDHelloWorld(const char *args)
{
	gui_t *gui = Z_ClearNew(gui_t, 1);
	gui->Responder = &CON_CMDHelloWorldResponder;
	GUI_Start(GUI_NULL(), gui);
	gui->process = GUI_MSGStart(GUI_NULL(), gui, 112, 0, "Hello World");
	return 0;
}

int CON_CMDExec(const char *args)
{
	FILE *script;
	int argc;
	char *argv[2];
	char buffer[SCREENCOLS];

	argc = GetArgs(args, 2, argv);

	if (!argc)
		return 1;

	if (argc != 2)
	{
		CON_Printf("Usage: exec <script name.cfg>\n");
		KillArgs(argc, argv);
		return 1;
	}

	script = fopen(argv[1], "rb");
	if (!script)
	{
		CON_Printf("Unable to open \'%s\'!\n", argv[1]);
		KillArgs(argc, argv);
		return 1;
	}

	while (fgets(buffer, SCREENCOLS - 1, script))
	{
		CON_TryCommand(buffer);
	}

	fclose(script);
	KillArgs(argc, argv);
	return 0;
}

int CON_CMDType(const char *args)
{
	FILE *script;
	int argc;
	char *argv[2];
	char buffer[SCREENCOLS];

	argc = GetArgs(args, 2, argv);

	if (!argc)
		return 1;

	if (argc != 2)
	{
		CON_Printf("Usage: type <file name.txt>\n");
		KillArgs(argc, argv);
		return 2;
	}

	script = fopen(argv[1], "r");
	if (!script)
	{
		CON_Printf("Unable to open \'%s\'!\n", argv[1]);
		KillArgs(argc, argv);
		return 3;
	}
	while (fgets(buffer, SCREENCOLS - 1, script))
	{
		CON_Printf(buffer);
	}
	fclose(script);
	KillArgs(argc, argv);
	return 0;
}

int CON_CMDArgText(const char *args)
{
	int argc;
	int i;
	char *(argv[10]);

	argc = GetArgs(args, 10, argv);

	for (i = 0; i < argc; i++)
		CON_Printf("%d:(%d) \"%s\"\n", i, (int)strlen(argv[i]), argv[i]);

	KillArgs(argc, argv);
	return 0;
}

//
// Eats memory.
//
int CON_CMDEat(const char *args)
{
	int argc;
	char *argv[2];
	int bytes;
	static char *p = NULL;

	argc = GetArgs(args, 2, argv);
	if (!argc)
		return 1;

	if (argc != 2 || 1 != sscanf(argv[1], "%d", &bytes))
	{
		CON_Printf("Eat memory. Usage: eat <size>\n");
		KillArgs(argc, argv);
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

int CON_CMDScreenShot(const char *args)
{
	G_DeferredScreenShot();

	return 0;
}

int CON_CMDSet(const char *args)
{
	int argc;
	cvar_t *var;
	// temp argv: Just to handle 'const' keyword properly
	char *tmpargv[3];
	const char *argv[3];
	int i;
	int e = 0;
	char buf[1025];

	argc = GetArgs(args, 3, tmpargv);

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

	KillArgs(argc, tmpargv);
	return e;
}

int CON_CMDTypeOf(const char *args)
{
	cvar_t *v;
	char *argv[2];
	int argc;
	int e;

	argc = GetArgs(args, 2, argv);

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
			CON_Printf("No console variable called \"%s\"!\n", args);
			e = 2;
		}
	}

	KillArgs(argc, argv);

	return e;
}

int CON_CMDWatch(const char *args)
{
	int argc;
	char *argv[3];

	argc = GetArgs(args, 3, argv);

	if (argc != 3)
	{
		CON_Printf("Usage: watch <cvar> <max>\n");
		return 1;
	}

	GUI_BARStart(GUI_NULL(), argv[1], strtol(argv[2], NULL, 0));

	KillArgs(argc, argv);
	return 0;
}

int CON_CMDQuitEDGE(const char *args)
{
	M_QuitEDGE(0);

	return 0;
}


int CON_CMDCrc(const char *args)
{
	int argc;
	char *argv[3];

	int lump, length;
	const byte *data;

	argc = GetArgs(args, 2, argv);

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

	KillArgs(argc, argv);
	return 0;
}

int CON_CMDPlaySound(const char *args)
{
	int argc;
	char *argv[3];

	sfx_t *sfx;

	argc = GetArgs(args, 2, argv);

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
		sound::StartFX(sfx, SNCAT_UI);
	}

	KillArgs(argc, argv);
	return 0;
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
