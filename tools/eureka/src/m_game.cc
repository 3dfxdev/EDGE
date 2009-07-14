//------------------------------------------------------------------------
//  GAME HANDLING
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor (C) 2001-2009 Andrew Apted
//                     (C) 1997-2003 André Majorel et al
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

#include "im_color.h"
#include "m_game.h"
#include "levels.h"
#include "e_things.h"


std::vector<ldtdef_t *> ldtdef;
std::vector<ldtgroup_t *> ldtgroup;
std::vector<stdef_t *> stdef;
std::vector<thingdef_t *> thingdef;
std::vector<thinggroup_t *> thinggroup;


/*
 *  InitGameDefs
 *  Create empty lists for game definitions
 */
void InitGameDefs (void)
{
    ldtdef.clear();
    ldtgroup.clear();
    stdef.clear();
    thingdef.clear();
    thinggroup.clear();
}


/*
 *  LoadGameDefs
 *  Looks for file <game>.ugh in various directories and reads it.
 *  Builds list ThingsDefs.
 *  A totally boring piece of code.
 */
void LoadGameDefs (const char *game)
{
	FILE *ygdfile = 0;    /* YGD file descriptor */
#define YGD_BUF 200   /* max. line length + 2 */
	char readbuf[YGD_BUF];    /* buffer the line is read into */
#define MAX_TOKENS 10   /* tokens per line */
	int lineno;     /* current line of file */
	char filename[1025];
	char basename[256];

	strcpy (basename, game  );
	strcat (basename, ".ugh");

	strcpy (filename, basename);


	ygdfile = fopen (filename, "r");
	if (ygdfile == NULL)
		FatalError ("%s: %s", filename, strerror (errno));

	/* Read the game definition
	   file, line by line. */
	for (lineno = 2; fgets (readbuf, sizeof readbuf, ygdfile); lineno++)
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
		buf = (char *) malloc (strlen (readbuf) + 1);
		if (! buf)
			FatalError ("not enough memory");

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
			else if (! in_token && (quoted || ! isspace (*iptr)))
			{
				if (ntoks >= (int) (sizeof token / sizeof *token))
					FatalError ("%s(%d): more than %d tokens",
							filename, lineno, sizeof token / sizeof *token);
				token[ntoks] = optr;
				ntoks++;
				in_token = 1;
				*optr++ = *iptr;
			}

			// First space between two tokens
			else if (in_token && ! quoted && isspace (*iptr))
			{
				*optr++ = '\0';
				in_token = 0;
			}

			// Character in the middle of a token
			else if (in_token)
				*optr++ = *iptr;
		}
		if (quoted)
			FatalError ("%s(%d): unmatched double quote", filename, lineno);

		/* process line */
		if (ntoks == 0)
		{
			free (buf);
			continue;
		}
		if (! strcmp (token[0], "ldt"))
		{
			ldtdef_t *buf = new ldtdef_t;

			if (ntoks != 5)
				FatalError (bad_arg_count, filename, lineno, token[0], 4);

			buf->number    = atoi (token[1]);
			buf->ldtgroup  = *token[2];
			buf->shortdesc = token[3];  /* FIXME: trunc to 16 char. */
			buf->longdesc  = token[4];  /* FIXME: trunc reasonably */

			ldtdef.push_back(buf);
		}
		else if (! strcmp (token[0], "ldtgroup"))
		{
			if (ntoks != 3)
				FatalError (bad_arg_count, filename, lineno, token[0], 2);

			ldtgroup_t *buf = new ldtgroup_t;

			buf->ldtgroup = *token[1];
			buf->desc     = token[2];

			ldtgroup.push_back(buf);
		}
		else if (! strcmp (token[0], "level_format"))
		{
			if (ntoks != 2)
				FatalError (bad_arg_count, filename, lineno, token[0], 1);
			if (! strcmp (token[1], "alpha"))
				yg_level_format = YGLF_ALPHA;
			else if (! strcmp (token[1], "doom"))
				yg_level_format = YGLF_DOOM;
			else if (! strcmp (token[1], "hexen"))
				yg_level_format = YGLF_HEXEN;
			else
				FatalError ("%s(%d): invalid argument \"%.32s\" (alpha|doom|hexen)",
						filename, lineno, token[1]);
			free (buf);
		}
		else if (! strcmp (token[0], "level_name"))
		{
			if (ntoks != 2)
				FatalError (bad_arg_count, filename, lineno, token[0], 1);
			if (! strcmp (token[1], "e1m1"))
				yg_level_name = YGLN_E1M1;
			else if (! strcmp (token[1], "e1m10"))
				yg_level_name = YGLN_E1M10;
			else if (! strcmp (token[1], "map01"))
				yg_level_name = YGLN_MAP01;
			else
				FatalError ("%s(%d): invalid argument \"%.32s\" (e1m1|e1m10|map01)",
						filename, lineno, token[1]);
			free (buf);
		}
		else if (! strcmp (token[0], "picture_format"))
		{
			if (ntoks != 2)
				FatalError (bad_arg_count, filename, lineno, token[0], 1);
			if (! strcmp (token[1], "alpha"))
				yg_picture_format = YGPF_ALPHA;
			else if (! strcmp (token[1], "pr"))
				yg_picture_format = YGPF_PR;
			else if (! strcmp (token[1], "normal"))
				yg_picture_format = YGPF_NORMAL;
			else
				FatalError ("%s(%d): invalid argument \"%.32s\" (alpha|pr|normal)",
						filename, lineno, token[1]);
			free (buf);
		}
		else if (! strcmp (token[0], "sky_flat"))
		{
			if (ntoks != 2)
				FatalError (bad_arg_count, filename, lineno, token[0], 1);
			sky_flat = token[1];
		}
		else if (! strcmp (token[0], "st"))
		{
			if (ntoks != 4)
				FatalError (bad_arg_count, filename, lineno, token[0], 3);

			stdef_t *buf = new stdef_t;

			buf->number    = atoi (token[1]);
			buf->shortdesc = token[2];  /* FIXME: trunc to 14 char. */
			buf->longdesc  = token[3];  /* FIXME: trunc reasonably */

			stdef.push_back(buf);
		}
		else if (! strcmp (token[0], "texture_format"))
		{
			if (ntoks != 2)
				FatalError (bad_arg_count, filename, lineno, token[0], 1);
			if (! strcmp (token[1], "nameless"))
				yg_texture_format = YGTF_NAMELESS;
			else if (! strcmp (token[1], "normal"))
				yg_texture_format = YGTF_NORMAL;
			else if (! strcmp (token[1], "strife11"))
				yg_texture_format = YGTF_STRIFE11;
			else
				FatalError (
						"%s(%d): invalid argument \"%.32s\" (normal|nameless|strife11)",
						filename, lineno, token[1]);
			free (buf);
		}
		else if (! strcmp (token[0], "texture_lumps"))
		{
			if (ntoks != 2)
				FatalError (bad_arg_count, filename, lineno, token[0], 1);
			if (! strcmp (token[1], "textures"))
				yg_texture_lumps = YGTL_TEXTURES;
			else if (! strcmp (token[1], "normal"))
				yg_texture_lumps = YGTL_NORMAL;
			else if (! strcmp (token[1], "none"))
				yg_texture_lumps = YGTL_NONE;
			else
				FatalError (
						"%s(%d): invalid argument \"%.32s\" (normal|textures|none)",
						filename, lineno, token[1]);
			free (buf);
		}
		else if (! strcmp (token[0], "thing"))
		{
			if (ntoks < 6 || ntoks > 7)
				FatalError (
						"%s(d%): directive \"%s\" takes between 5 and 6 parameters",
						filename, lineno, token[0]);

			thingdef_t *buf = new thingdef_t;

			buf->number     = atoi (token[1]);
			buf->thinggroup = *token[2];
			buf->flags      = *token[3] == 's' ? THINGDEF_SPECTRAL : 0;  // FIXME!
			buf->radius     = atoi (token[4]);
			buf->desc       = token[5];
			buf->sprite     = ntoks >= 7 ? token[6] : 0;

			thingdef.push_back(buf);
		}
		else if (! strcmp (token[0], "thinggroup"))
		{
			if (ntoks != 4)
				FatalError (bad_arg_count, filename, lineno, token[0], 3);

			thinggroup_t *buf = new thinggroup_t;

			buf->thinggroup = *token[1];
			if (getcolour (token[2], &buf->rgb))
				FatalError ("%s(%d): bad colour spec \"%.32s\"",
						filename, lineno, token[2]);

			buf->desc = token[3];

			thinggroup.push_back(buf);
		}
		else
		{
			free (buf);
			FatalError ("%s(%d): unknown directive \"%.32s\"",
					filename, lineno, token[0]);
		}
	}

	fclose (ygdfile);

	/* Verify that all the mandatory directives are present. */
	{
		bool abort = false;
		if (yg_level_format == YGLF__)
		{
			err ("%s: Missing \"level_format\" directive.", filename);
			abort = true;
		}
		if (yg_level_name == YGLN__)
		{
			err ("%s: Missing \"level_name\" directive.", filename);
			abort = true;
		}
		// FIXME perhaps print a warning message if picture_format
		// is missing ("assuming picture_format=normal").
		// FIXME and same thing for texture_format and texture_lumps ?
		if (abort)
			exit (2);
	}

#if 0

#endif

	/*
	 *  Second pass
	 */

	/* Speed optimization : build the table of things attributes
	   that get_thing_*() use. */
	create_things_table ();

	///???  /* KLUDGE: Add bogus ldtgroup LDT_FREE. InputLinedefType()
	///???     knows that it means "let the user enter a number". */
	///???  {
	///???  ldtgroup_t * buf = new ldtgroup_t;
	///???  
	///???  buf->ldtgroup = LDT_FREE;  /* that is '\0' */
	///???  buf->desc     = "Other (enter number)";
	///???  
	///???  buf->push_back(buf);
	///???  if (al_lwrite (ldtgroup, &buf))
	///???    FatalError ("LGD90 (%s)", al_astrerror (al_aerrno));
	///???  }
	///???  
	///???  /* KLUDGE: Add bogus thinggroup THING_FREE.
	///???     InputThingType() knows that it means "let the user enter a number". */
	///???  {
	///???  thinggroup_t buf;
	///???  
	///???  buf.thinggroup = THING_FREE;  /* that is '\0' */
	///???  buf.desc       = "Other (enter number)";
	///???  al_lseek (thinggroup, 0, SEEK_END);
	///???  if (al_lwrite (thinggroup, &buf))
	///???    FatalError ("LGD91 (%s)", al_astrerror (al_aerrno));
	///???  }
	///???  
	///???  /* KLUDGE: Add bogus sector type at the end of stdef.
	///???     SectorProperties() knows that it means "let the user enter a number". */
	///???  {
	///???  stdef_t buf;
	///???  
	///???  buf.number    = 0;     /* not significant */
	///???  buf.shortdesc = 0;     /* not significant */
	///???  buf.longdesc  = "Other (enter number)";
	///???  al_lseek (stdef, 0, SEEK_END);
	///???  if (al_lwrite (stdef, &buf))
	///???    FatalError ("LGD92 (%s)", al_astrerror (al_aerrno));
	///???  }

}


/*
 *  FreeGameDefs
 *  Free all memory allocated to game definitions
 */
void FreeGameDefs (void)
{
	delete_things_table ();
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
