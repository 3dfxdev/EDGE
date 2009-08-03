//------------------------------------------------------------------------
//  GAME HANDLING
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
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
//------------------------------------------------------------------------
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#include "main.h"

#include <map>

#include "im_color.h"
#include "m_game.h"
#include "levels.h"
#include "e_things.h"


#define UNKNOWN_THING_RADIUS  16
#define UNKNOWN_THING_COLOR   fl_rgb_color(0,255,255)


std::map<char, linegroup_t *>  line_groups;
std::map<char, thinggroup_t *> thing_groups;

std::map<int, linetype_t *>   line_types;
std::map<int, sectortype_t *> sector_types;
std::map<int, thingtype_t *>  thing_types;

/*
 *  InitGameDefs
 *  Create empty lists for game definitions
 */
void InitGameDefs(void)
{
	// TODO: delete the contents

    line_groups.clear();
    line_types.clear();
    sector_types.clear();
    thing_groups.clear();
    thing_types.clear();
}


static pcolour_t ParseHexColor(const char *str)
{
	int number = strtol(str, NULL, 16);

	int r = (number & 0xF00) >> 8;
	int g = (number & 0x0F0) >> 4;
	int b = (number & 0x00F);

	return fl_rgb_color(r*17, g*17, b*17);
}


/*
 *  LoadGameDefs
 *  Looks for file <game>.ugh in various directories and reads it.
 *  Builds list ThingsDefs.
 *  A totally boring piece of code.
 */
void LoadGameDefs(const char *game)
{
	FILE *ygdfile = 0;    /* YGD file descriptor */
#define YGD_BUF 200   /* max. line length + 2 */
	char readbuf[YGD_BUF];    /* buffer the line is read into */
#define MAX_TOKENS 10   /* tokens per line */
	int lineno;     /* current line of file */
	char filename[1025];
	char basename[256];

	strcpy(basename, game  );
	strcat(basename, ".ugh");

	strcpy(filename, basename);


	ygdfile = fopen(filename, "r");
	if (ygdfile == NULL)
		FatalError("%s: %s", filename, strerror (errno));

	/* Read the game definition
	   file, line by line. */
	for (lineno = 2; fgets(readbuf, sizeof readbuf, ygdfile); lineno++)
	{
		int         ntoks;
		char       *token[MAX_TOKENS];
		int         quoted;
		int         in_token;
		const char *iptr;
		char       *optr;
		char       *buf;
		const char *const bad_arg_count =
			"%s(%d): directive \"%s\" takes %d parameters";

		/* duplicate the buffer */
		buf = (char *) malloc(strlen(readbuf) + 1);
		if (! buf)
			FatalError("not enough memory");

		/* break the line into whitespace-separated tokens.
		   whitespace can be enclosed in double quotes. */
		for (in_token = 0, quoted = 0, iptr = readbuf, optr = buf, ntoks = 0;
				; iptr++)
		{
			if (*iptr == '\n' || *iptr == '\0')
			{
				if (in_token)
					*optr = '\0';
				break;
			}

			else if (*iptr == '"')
				quoted ^= 1;

			// "#" at the beginning of a token
			else if (! in_token && ! quoted && *iptr == '#')
				break;

			// First character of token
			else if (! in_token && (quoted || ! isspace(*iptr)))
			{
				if (ntoks >= (int) (sizeof token / sizeof *token))
					FatalError("%s(%d): more than %d tokens",
							filename, lineno, sizeof token / sizeof *token);
				token[ntoks] = optr;
				ntoks++;
				in_token = 1;
				*optr++ = *iptr;
			}

			// First space between two tokens
			else if (in_token && ! quoted && isspace(*iptr))
			{
				*optr++ = '\0';
				in_token = 0;
			}

			// Character in the middle of a token
			else if (in_token)
				*optr++ = *iptr;
		}
		if (quoted)
			FatalError("%s(%d): unmatched double quote", filename, lineno);

		/* process line */
		if (ntoks == 0)
		{
			free(buf);
			continue;
		}

		if (y_stricmp(token[0], "linegroup") == 0)
		{
			if (ntoks != 3)
				FatalError(bad_arg_count, filename, lineno, token[0], 2);

			linegroup_t *buf = new linegroup_t;

			buf->group = token[1][0];
			buf->desc  = token[2];

			line_groups[buf->group] = buf;
		}
		else if (y_stricmp(token[0], "line") == 0)
		{
			linetype_t *buf = new linetype_t;

			if (ntoks < 4)  //!!!!!! FIXME != 4
				FatalError(bad_arg_count, filename, lineno, token[0], 3);

			int number = atoi(token[1]);

			buf->group = token[2][0];
			buf->desc  = token[3];

			if (line_groups.find(buf->group) == line_groups.end())
				FatalError("%s(%d): unknown line group '%c'.\n",
						filename, lineno, buf->group);

			line_types[number] = buf;
		}
		else if (y_stricmp(token[0], "level_name") == 0)
		{
			if (ntoks != 2)
				FatalError(bad_arg_count, filename, lineno, token[0], 1);

			if (! strcmp(token[1], "e1m1"))
				yg_level_name = YGLN_E1M1;
			else if (! strcmp(token[1], "e1m10"))
				yg_level_name = YGLN_E1M10;
			else if (! strcmp(token[1], "map01"))
				yg_level_name = YGLN_MAP01;
			else
				FatalError("%s(%d): invalid argument \"%.32s\" (e1m1|e1m10|map01)",
						filename, lineno, token[1]);
			free(buf);
		}
		else if (y_stricmp(token[0], "sky_flat") == 0)
		{
			if (ntoks != 2)
				FatalError(bad_arg_count, filename, lineno, token[0], 1);

			sky_flat = token[1];
		}
		else if (y_stricmp(token[0], "sector") == 0)
		{
			if (ntoks != 3)
				FatalError(bad_arg_count, filename, lineno, token[0], 2);

			int number = atoi(token[1]);

			sectortype_t *buf = new sectortype_t;
			buf->desc = token[2];

			sector_types[number] = buf;
		}
		else if (y_stricmp(token[0], "thinggroup") == 0)
		{
			if (ntoks != 4)
				FatalError(bad_arg_count, filename, lineno, token[0], 3);

			thinggroup_t *buf = new thinggroup_t;

			buf->group = token[1][0];
			buf->color = ParseHexColor(token[2]);
			buf->desc  = token[3];

			thing_groups[buf->group] = buf;
		}
		else if (y_stricmp(token[0], "thing") == 0)
		{
			if (ntoks != 7)
				FatalError(bad_arg_count, filename, lineno, token[0], 7);

			thingtype_t *buf = new thingtype_t;

			int number = atoi(token[1]);

			buf->group  = token[2][0];
			buf->flags  = (token[3][0] == 's') ? THINGDEF_SPECTRAL : 0;  // FIXME!
			buf->radius = atoi(token[4]);
			buf->sprite = token[5];
			buf->desc   = token[6];

			if (thing_groups.find(buf->group) == thing_groups.end())
				FatalError("%s(%d): unknown thing group '%c'.\n",
						filename, lineno, buf->group);

			buf->color  = thing_groups[buf->group]->color;

			thing_types[number] = buf;
		}
		else
		{
			FatalError("%s(%d): unknown directive \"%.32s\"",
					filename, lineno, token[0]);
		}
	}

	fclose(ygdfile);

	/* Verify that all the mandatory directives are present. */
	{
		bool abort = false;
		if (yg_level_name == YGLN__)
		{
			err ("%s: Missing \"level_name\" directive.", filename);
			abort = true;
		}

		if (abort)
			exit(2);
	}
}


/*
 *  FreeGameDefs
 *  Free all memory allocated to game definitions
 */
void FreeGameDefs(void)
{
}


const sectortype_t * M_GetSectorType(int type)
{
	std::map<int, sectortype_t *>::iterator SI;

	SI = sector_types.find(type);

	if (SI != sector_types.end())
		return SI->second;

	static sectortype_t dummy_type =
	{
		"UNKNOWN TYPE"
	};

	return &dummy_type;
}


const linetype_t * M_GetLineType(int type)
{
	std::map<int, linetype_t *>::iterator LI;

	LI = line_types.find(type);

	if (LI != line_types.end())
		return LI->second;

	static linetype_t dummy_type =
	{
		0, "UNKNOWN TYPE"
	};

	return &dummy_type;
}


const thingtype_t * M_GetThingType(int type)
{
	std::map<int, thingtype_t *>::iterator TI;

	TI = thing_types.find(type);

	if (TI != thing_types.end())
		return TI->second;

	static thingtype_t dummy_type =
	{
		0, 0, UNKNOWN_THING_RADIUS,
		"UNKNOWN TYPE", "NULL",
		UNKNOWN_THING_COLOR
	};

	return &dummy_type;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
