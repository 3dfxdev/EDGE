//------------------------------------------------------------------------
//  MAIN PROGRAM
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
#include <time.h>

#include "im_color.h"
#include "cfgfile.h"
#include "editloop.h"
#include "game.h"
#include "gfx.h"
#include "levels.h"    /* Because of "viewtex" */
#include "w_patches.h"  /* Because of "p" */
#include "w_file.h"
#include "w_list.h"
#include "w_name.h"
#include "w_wads.h"


#define VERSION  "0.42"


/*
 *  Constants (declared in main.h)
 */
const char *const log_file       = "eureka.log";
const char *const msg_unexpected = "unexpected error";
const char *const msg_nomem      = "Not enough memory";


/*
 *  Not real variables -- just unique pointer values
 *  used by functions that return pointers to 
 */
char error_non_unique[1];  // Found more than one
char error_none[1];        // Found none
char error_invalid[1];     // Invalid parameter


/*
 *  Global variables
 */
FILE *      logfile     = NULL;   // Filepointer to the error log
bool        Registered  = false;  // Registered or shareware game?
int         remind_to_build_nodes = 0;  // Remind user to build nodes

// Set from command line and/or config file
bool      autoscroll      = 0;
unsigned long autoscroll_amp    = 10;
unsigned long autoscroll_edge   = 30;
const char *config_file                 = NULL;
int       copy_linedef_reuse_sidedefs = 0;
bool      Debug       = false;
int       default_ceiling_height  = 128;
char      default_ceiling_texture[WAD_FLAT_NAME + 1]  = "CEIL3_5";
int       default_floor_height        = 0;
char      default_floor_texture[WAD_FLAT_NAME + 1]  = "FLOOR4_8";
int       default_light_level       = 144;
char      default_lower_texture[WAD_TEX_NAME + 1] = "STARTAN3";
char      default_middle_texture[WAD_TEX_NAME + 1]  = "STARTAN3";
char      default_upper_texture[WAD_TEX_NAME + 1] = "STARTAN3";
int       default_thing     = 3004;
int       double_click_timeout    = 200;
bool      Expert      = false;
const char *Game      = NULL;
const char *Warp      = NULL;

int       idle_sleep_ms     = 50;
bool      InfoShown     = true;
int       zoom_default      = 0;  // 0 means fit
int       zoom_step     = 0;  // 0 means sqrt(2)
int       digit_zoom_base               = 100;
int       digit_zoom_step               = 0;  // 0 means sqrt(2)

confirm_t insert_vertex_split_linedef = YC_ASK_ONCE;
confirm_t insert_vertex_merge_vertices  = YC_ASK_ONCE;

bool      blindly_swap_sidedefs         = false;

const char *Iwad1     = NULL;
const char *Iwad2     = NULL;
const char *Iwad3     = NULL;
const char *Iwad4     = NULL;
const char *Iwad5     = NULL;
const char *Iwad6     = NULL;
const char *Iwad7     = NULL;
const char *Iwad8     = NULL;
const char *Iwad9     = NULL;
const char *Iwad10      = NULL;
const char *MainWad     = NULL;
#ifdef AYM_MOUSE_HACKS
int       MouseMickeysH     = 5;
int       MouseMickeysV     = 5;
#endif
char **   PatchWads     = NULL;
bool      Quiet       = false;
bool      Quieter     = false;
unsigned long scroll_less   = 10;
unsigned long scroll_more   = 90;
bool      Select0     = false;
int       show_help     = 0;
int       sprite_scale                  = 100;
bool      SwapButtons     = false;
int       verbose     = 0;
int       welcome_message   = 1;
const char *bench     = 0;

// Global variables declared in game.h
yglf_t yg_level_format   = YGLF__;
ygln_t yg_level_name     = YGLN__;
ygpf_t yg_picture_format = YGPF_NORMAL;
ygtf_t yg_texture_format = YGTF_NORMAL;
ygtl_t yg_texture_lumps  = YGTL_NORMAL;

Wad_name sky_flat;


/*
 *  Prototypes of private functions
 */
static int  parse_environment_vars ();
static void print_error_message (const char *fmt, va_list args);


/*
 *  parse_environment_vars
 *  Check certain environment variables.
 *  Returns 0 on success, <>0 on error.
 */
static int parse_environment_vars ()
{
	char *value;

	value = getenv ("YADEX_GAME");
	if (value != NULL)
		Game = value;
	return 0;
}


/*
   play a fascinating tune
*/

void Beep ()
{
	fl_beep();
}


/*
 *  warn
 *  printf a warning message to stdout.
 *  If the format string of the previous invocation did not
 *  end with a '\n', do not prepend the "Warning: " string.
 *
 *  FIXME should handle cases where stdout is not available
 *  (BGI version when in graphics mode).
 */
void warn (const char *fmt, ...)
{
	static bool start_of_line = true;
	va_list args;

	if (start_of_line)
		fputs ("Warning: ", stdout);
	va_start (args, fmt);
	vprintf (fmt, args);
	size_t len = strlen (fmt);
	start_of_line = len > 0 && fmt[len - 1] == '\n';
}


/*
 *  fatal_error
 *  Print an error message and terminate the program with code 2.
 */
void FatalError(const char *fmt, ...)
{
	// With BGI, we have to switch back to text mode
	// before printing the error message so do it now...

	va_list args;
	va_start (args, fmt);
	print_error_message (fmt, args);

	// ... on the other hand, with X, we don't have to
	// call TermGfx() before printing so we do it last so
	// that a segfault occuring in TermGfx() does not
	// prevent us from seeing the stderr message.

	TermGfx ();  // Don't need to sleep (1) either.


	// Clean up things and free swap space
	ForgetLevelData ();
	ForgetWTextureNames ();
	ForgetFTextureNames ();
	CloseWadFiles ();
	exit (2);
}


/*
 *  err
 *  Print an error message but do not terminate the program.
 */
void err (const char *fmt, ...)
{
	va_list args;

	va_start (args, fmt);
	print_error_message (fmt, args);
}


/*
 *  print_error_message
 *  Print an error message to stderr.
 */
static void print_error_message (const char *fmt, va_list args)
{
	fflush (stdout);
	fputs ("Error: ", stderr);
	vfprintf (stderr, fmt, args);
	fputc ('\n', stderr);
	fflush (stderr);
	if (Debug && logfile != NULL)
	{
		fputs ("Error: ", logfile);
		vfprintf (logfile, fmt, args);
		fputc ('\n', logfile);
		fflush (logfile);
	}
}


/*
 *  nf_bug
 *  Report about a non-fatal bug to stderr. The message
 *  should not expand to more than 80 characters.
 */
void nf_bug (const char *fmt, ...)
{
	static bool first_time = 1;
	static int repeats = 0;
	static char msg_prev[81];
	char msg[81];

	va_list args;
	va_start (args, fmt);
	y_vsnprintf (msg, sizeof msg, fmt, args);
	if (first_time || strncmp (msg, msg_prev, sizeof msg))
	{
		fflush (stdout);
		if (repeats)
		{
			fprintf (stderr, "Bug: Previous message repeated %d times\n",
					repeats);
			repeats = 0;
		}

		fprintf (stderr, "Bug: %s\n", msg);
		fflush (stderr);
		if (first_time)
		{
			fputs ("REPORT ALL \"Bug:\" MESSAGES TO THE MAINTAINER !\n", stderr);
			first_time = 0;
		}
		strncpy (msg_prev, msg, sizeof msg_prev);
	}
	else
	{
		repeats++;  // Same message as above
		if (repeats == 10)
		{
			fflush (stdout);
			fprintf (stderr, "Bug: Previous message repeated %d times\n",
					repeats);
			fflush (stderr);
			repeats = 0;
		}
	}
}


/*
   write a message in the log file
*/

void LogMessage (const char *logstr, ...)
{
  va_list  args;
  time_t   tval;
  char    *tstr;

  if (Debug && logfile != NULL)
  {
    va_start (args, logstr);
    /* if the message begins with ":", output the current date & time first */
    if (logstr[0] == ':')
    {
      time (&tval);
      tstr = ctime (&tval);
      tstr[strlen (tstr) - 1] = '\0';
      fprintf (logfile, "%s", tstr);
    }
    vfprintf (logfile, logstr, args);
    fflush (logfile);  /* AYM 19971031 */
  }
}


/*
 *  find_level
 *  Look in the master directory for levels that match
 *  the name in <name_given>.
 *
 *  <name_given> can have one of the following formats :
 *  
 *    [Ee]n[Mm]m      EnMm
 *    [Mm][Aa][Pp]nm  MAPnm
 *    n               MAP0n
 *    nm              Either EnMn or MAPnm
 *    ijk             EiMjk (Doom alpha 0.4 and 0.5)
 *
 *  Return:
 *  - If <name_given> is either [Ee]n[Mm]m or [Mm][Aa][Pp]nm,
 *    - if the level was found, its canonical (uppercased)
 *      name in a freshly malloc'd buffer,
 *    - else, NULL.
 *  - If <name_given> is either n or nm,
 *    - if either EnMn or MAPnm was found, the canonical name
 *      of the level found, in a freshly malloc'd buffer,
 *    - if none was found, <error_none>,
 *    - if the <name_given> is invalid, <error_invalid>,
 *    - if several were found, <error_non_unique>.
 */
static char *find_level (const char *name_given)
{
	// Is it a shorthand name ? ("1", "23", ...)
	if (isdigit(name_given[0])
			&& (atoi (name_given) <= 99
				|| atoi (name_given) <= 999 && yg_level_name == YGLN_E1M10))
	{
		int n = atoi (name_given);
		char *name1 = (char *) malloc (7);
		char *name2 = (char *) malloc (6);
		if (n > 99)
			sprintf (name1, "E%dM%02d", n / 100, n % 100);
		else
			sprintf (name1, "E%dM%d", n / 10, n % 10);
		sprintf (name2, "MAP%02d", n);
		int match1 = FindMasterDir (MasterDir, name1) != NULL;
		int match2 = FindMasterDir (MasterDir, name2) != NULL;
		if (match1 && ! match2)  // Found only ExMy
		{
			free (name2);
			return name1;
		}
		else if (match2 && ! match1) // Found only MAPxy
		{
			free (name1);
			return name2;
		}
		else if (match1 && match2) // Found both
		{
			free (name1);
			free (name2);
			return error_non_unique;
		}
		else       // Found none
		{
			free (name1);
			free (name2);
			return error_none;
		}
	}

#if 1
	// Else look for <name_given>
	if (FindMasterDir (MasterDir, name_given))
		return strdup (name_given);
	else
	{
		if (levelname2levelno (name_given))
			return NULL;
		else
			return error_invalid;
	}
#else
	// If <name_given> is "[Ee]n[Mm]m" or "[Mm][Aa][Pp]nm", look for that
	if (levelname2levelno (name_given))
	{
		char *canonical_name = strdup (name_given);
		for (char *p = canonical_name; *p; p++)
			*p = toupper (*p);  // But shouldn't FindMasterDir() be case-insensitive ?
		if (FindMasterDir (MasterDir, canonical_name))
			return canonical_name;
		else
		{
			free (canonical_name);
			return NULL;
		}
	}
	return error_invalid;
#endif
}


/*
   the driving program
*/

void EditLevel (const char *levelname, bool newlevel)
{
    if (InitGfx())
        return;

    if (newlevel)
    {
        EmptyLevelData (levelname);

        MapMinX = -2000;
        MapMinY = -2000;
        MapMaxX = 2000;
        MapMaxY = 2000;

        Level = 0;
    }
    else
    {
        if (ReadLevelData (levelname))
        {
            goto done;  // Failure!
        }
    }

    LogMessage (": Editing %s...\n", levelname ? levelname : "new level");

	EditorLoop (levelname);

	LogMessage (": Finished editing %s...\n", levelname ? levelname : "new level");

	if (Level && Level->wadfile)
	{
		const char *const file_name =
			Level->wadfile ? Level->wadfile->pathname () : "(New level)";
	}

done :
    TermGfx ();

    ForgetLevelData ();

    /* forget the level pointer */
    Level = 0;

    ForgetWTextureNames ();
    ForgetFTextureNames ();
}



int main (int argc, char *argv[])
{
	int r;

	// First detect manually --help and --version
	// because parse_command_line_options() cannot.
	if (argc == 2 && strcmp (argv[1], "--help") == 0)
	{
		//   print_usage (stdout);
		if (fflush (stdout) != 0)
			FatalError("stdout: %s", strerror (errno));
		exit (0);
	}
	if (argc == 2 && strcmp (argv[1], "--version") == 0)
	{
		//   puts (what ());
		puts ("# Eureka fluff\n");
		if (fflush (stdout) != 0)
			FatalError("stdout: %s", strerror (errno));
		exit (0);
	}

	// Second a quick pass through the command line
	// arguments to detect -?, -f and -help.
	r = parse_command_line_options (argc - 1, argv + 1, 1);
	if (r)
		exit(1);

	if (show_help)
	{
		// FIXME
		return 0;
	}

	//printf ("%s\n", what ());


	// The config file provides some values.
	if (config_file != NULL)
		r = parse_config_file_user (config_file);
	else
		r = parse_config_file_default ();

	if (r == 0)
	{
		// Environment variables can override them.
		r = parse_environment_vars ();
		if (r == 0)
		{
			// And the command line argument can override both.
			r = parse_command_line_options (argc - 1, argv + 1, 2);
		}
	}
	if (r != 0)
	{
		exit (1);
	}

	if (Game != NULL && strcmp (Game, "doom") == 0)
	{
		if (Iwad1 == NULL)
		{
			err ("You have to tell me where doom.wad is.");
			exit (1);
		}
		MainWad = Iwad1;
	}
	else if (Game != NULL && strcmp (Game, "doom2") == 0)
	{
		if (Iwad2 == NULL)
		{
			err ("You have to tell me where doom2.wad is.");
			exit (1);
		}
		MainWad = Iwad2;
	}
	else if (Game != NULL && strcmp (Game, "heretic") == 0)
	{
		if (Iwad3 == NULL)
		{
			err ("You have to tell me where heretic.wad is.");
			exit (1);
		}
		MainWad = Iwad3;
	}
	else if (Game != NULL && strcmp (Game, "hexen") == 0)
	{
		if (Iwad4 == NULL)
		{
			err ("You have to tell me where hexen.wad is.");
			exit (1);
		}
		MainWad = Iwad4;
	}
	else if (Game != NULL && strcmp (Game, "strife") == 0)
	{
		if (Iwad5 == NULL)
		{
			err ("You have to tell me where strife1.wad is.");
			exit (1);
		}
		MainWad = Iwad5;
	}
	else if (Game != NULL && strcmp (Game, "doom02") == 0)
	{
		if (Iwad6 == NULL)
		{
			err ("You have to tell me where the Doom alpha 0.2 iwad is.");
			exit (1);
		}
		MainWad = Iwad6;
	}
	else if (Game != NULL && strcmp (Game, "doom04") == 0)
	{
		if (Iwad7 == NULL)
		{
			err ("You have to tell me where the Doom alpha 0.4 iwad is.");
			exit (1);
		}
		MainWad = Iwad7;
	}
	else if (Game != NULL && strcmp (Game, "doom05") == 0)
	{
		if (Iwad8 == NULL)
		{
			err ("You have to tell me where the Doom alpha 0.5 iwad is.");
			exit (1);
		}
		MainWad = Iwad8;
	}
	else if (Game != NULL && strcmp (Game, "doompr") == 0)
	{
		if (Iwad9 == NULL)
		{
			err ("You have to tell me where the Doom press release iwad is.");
			exit (1);
		}
		MainWad = Iwad9;
	}
	else if (Game != NULL && strcmp (Game, "strife10") == 0)
	{
		if (Iwad10 == NULL)
		{
			err ("You have to tell me where strife1.wad is.");
			exit (1);
		}
		MainWad = Iwad10;
	}
	else
	{
		if (Game == NULL)
			err ("You didn't say for which game you want to edit.");
		else
			err ("Unknown game \"%s\"", Game);
		fprintf (stderr,
				"Use \"-g <game>\" on the command line or put \"game=<game>\" in config file\n"
				"where <game> is one of \"doom\", \"doom02\", \"doom04\", \"doom05\","
				" \"doom2\",\n\"doompr\", \"heretic\", \"hexen\", \"strife\" and "
				"\"strife10\".\n");
		exit (1);
	}
	if (Debug)
	{
		logfile = fopen (log_file, "a");
		if (logfile == NULL)
			warn ("can't open log file \"%s\" (%s)", log_file, strerror (errno));
	}
	if (Quieter)
		Quiet = true;

	// Sanity checks (useful when porting).
	check_types();

	// Load game definitions (*.ygd).
	InitGameDefs();
	LoadGameDefs(Game);

	// Load the iwad and the pwads.
	if (OpenMainWad(MainWad))
		FatalError("If you don't give me an iwad, I'll quit. I'm serious.");

	if (PatchWads)
	{
		const char * const *pwad_name;

		for (pwad_name = PatchWads; *pwad_name; pwad_name++)
			OpenPatchWad (*pwad_name);
	}

	/* sanity check */
	CloseUnusedWadFiles();

	/* all systems go! */
	if (! Warp || ! Warp[0])
		Warp = "MAP01";

    ReadWTextureNames();
    ReadFTextureNames();

    patch_dir.refresh(MasterDir);

	EditLevel(Warp, 0);

	/* that's all, folks! */
	CloseWadFiles();
	FreeGameDefs();
	LogMessage(": The end!\n\n\n");

	if (logfile != NULL)
		fclose (logfile);
	if (remind_to_build_nodes)
		printf ("\n"
				"** You have made changes to one or more wads. Don't forget to pass\n"
				"** them through a nodes builder (E.G. BSP) before running them.\n"
				"** Like this: \"ybsp foo.wad -o tmp.wad; doom -file tmp.wad\"\n\n");
	return 0;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
