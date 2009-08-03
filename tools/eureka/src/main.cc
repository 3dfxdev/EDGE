//------------------------------------------------------------------------
//  MAIN PROGRAM
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

#include <time.h>

#include "e_loadsave.h"
#include "im_color.h"
#include "m_config.h"
#include "editloop.h"
#include "m_game.h"
#include "r_misc.h"
#include "levels.h"    /* Because of "viewtex" */
#include "w_file.h"
#include "w_list.h"
#include "w_name.h"
#include "w_wads.h"

#include "w_flats.h"
#include "w_texture.h"
#include "w_wad.h"

#include "ui_window.h"


#define VERSION  "0.52"


/*
 *  Constants (declared in main.h)
 */
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

char **   PatchWads     = NULL;
bool      Quiet       = false;
bool      Quieter     = false;
unsigned long scroll_less   = 10;
unsigned long scroll_more   = 90;
int       show_help     = 0;
int       sprite_scale                  = 100;
bool      SwapButtons     = false;
int       verbose     = 0;
int       welcome_message   = 1;
const char *bench     = 0;

ygln_t yg_level_name = YGLN__;

Wad_name sky_flat;
int sky_color = 207; // FIXME: move into UGH file


/*
 *  Prototypes of private functions
 */
static int  parse_environment_vars ();
static void print_error_message (const char *fmt, va_list args);

static void TermFLTK();


/*
 *  parse_environment_vars
 *  Check certain environment variables.
 *  Returns 0 on success, <>0 on error.
 */
static int parse_environment_vars ()
{
	char *value;

	value = getenv ("EUREKA_GAME");
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
 *  fatal_error
 *  Print an error message and terminate the program with code 2.
 */
void FatalError(const char *fmt, ...)
{
	va_list args;
	va_start (args, fmt);
	print_error_message (fmt, args);

	TermFLTK ();

	CloseWadFiles ();
	exit(2);
}


void BugError(const char *fmt, ...)
{
	va_list args;
	va_start (args, fmt);
	print_error_message (fmt, args);  // FUCKEN FIXME QUICK

	TermFLTK ();

	CloseWadFiles ();
	exit(9);
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
}


int InitFLTK(void)  // returns 0 on success
{
	/*
	 *  Create the window
	 */
	Fl::visual(FL_RGB);

	Fl::scheme("plastic");

	int screen_w = Fl::w();
	int screen_h = Fl::h();

	fprintf(stderr, "-- SCREEN SIZE %dx%d\n", screen_w, screen_h);

	KF = 0;
	if (screen_w >= 1000) KF++;
	if (screen_w >= 1240) KF++;

	KF_fonth = (14 + KF * 2);


	main_win = new UI_MainWin("Eureka DOOM Editor " VERSION);

	// kill the stupid bright background of the "plastic" scheme
	delete Fl::scheme_bg_;
	Fl::scheme_bg_ = NULL;

	main_win->image(NULL);

	// show window (pass some dummy arguments)
	{
		int argc = 1;
		char *argv[] = { "Goobers.exe", NULL };

		main_win->show(argc, argv);
	}

	SetWindowSize (main_win->canvas->w(), main_win->canvas->h());

	return 0;
}


void TermFLTK()
{
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
int main(int argc, char *argv[])
{
	LogInit("LOG.txt");
	LogPrintf("Eureka DOOM Editor v%s\n\n", VERSION);
	LogEnableDebug();

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

	if (Quieter)
		Quiet = true;

	// Sanity checks (useful when porting).
	check_types();

	// Load game definitions (*.ygd).
	InitGameDefs();
	LoadGameDefs(Game);

	// Load the iwad and the pwads.
	if (OpenMainWad("doom2.wad"))
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

	Wad_file * iwad = Wad_file::Open("doom2.wad");
	if (! iwad)
		FatalError("Cannot find IWAD!\n");
	master_dir.push_back(iwad);

	W_LoadPalette();

    if (InitFLTK())
        exit(9);

	const char *levelname = Warp;

	bool newlevel = false;

    if (newlevel)
		FreshLevel();
    else
        LoadLevel(levelname);

	W_LoadFlats();
	W_LoadTextures();

    LogPrintf(": Editing %s...\n", levelname ? levelname : "new level");

	EditorLoop (levelname);

	LogPrintf(": Finished editing %s...\n", levelname ? levelname : "new level");

    TermFLTK ();

	BA_ClearAll();

	// clear textures??

	/* that's all, folks! */
	CloseWadFiles();
	FreeGameDefs();

	if (remind_to_build_nodes)
		printf ("\n"
				"** You have made changes to one or more wads. Don't forget to pass\n"
				"** them through a nodes builder (E.G. BSP) before running them.\n"
				"** Like this: \"ybsp foo.wad -o tmp.wad; doom -file tmp.wad\"\n\n");

	LogClose();

	return 0;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
