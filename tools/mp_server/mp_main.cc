//------------------------------------------------------------------------
//  Main program
//------------------------------------------------------------------------
//
//  Edge MultiPlayer Server (C) 2005  Andrew Apted
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

#include "includes.h"

#include "client.h"
#include "game.h"
#include "lib_argv.h"
#include "mp_main.h"
#include "network.h"
#include "packet.h"
#include "protocol.h"
#include "ui_log.h"
#include "ui_window.h"


#define MY_TITLE  "Edge MultiPlayer Server " MPSERVER_VERSION


// GUI-specific Globals

#if 0
guix_preferences_t guix_prefs;

const guix_preferences_t default_guiprefs =
{
#ifdef WIN32
  40,  20,     // win_x, win_y
#else
  40,  50,
#endif
  550, 520,    // win_w, win_h;

  120, 200,    // progress_x, progress_y
  90, 200,     // dialog_x, dialog_y
  80, 100,     // other_x, other_y

  20, 50,      // manual_x, manual_y
  610, 520,    // manual_w, manual_h
  0,           // manual_page

  TRUE,        // overwrite_warn
  TRUE,        // same_file_warn
  TRUE,        // lack_ext_warn
  NULL         // save_log_file
};
#endif


/* ----- user information ----------------------------- */

static void ShowInfo(void)
{
	printf(
		"\n"
		"** " MY_TITLE " (C) Andrew Apted 2005 **\n"
		"\n"
	);

	printf(
		"Usage: mp_server [options...]\n"
		"\n"
		"Available options:\n"
		"  -l  -local [ADDR]      Set local computer's IP address\n"
		"  -p  -port  [PORT]      Set server's port number\n"
		"  -d  -debug             Enable debugging\n"
		"  -h  -help              Show this help message\n"
		"\n"
	);

	printf(
		"This program is free software, under the terms of the GNU General\n"
		"Public License, and comes with ABSOLUTELY NO WARRANTY.  See the\n"
		"documentation for more details, or visit this web page:\n"
		"    http://www.gnu.org/licenses/licenses.html\n"
		"\n"
	);
}


void MainSetDefaults(void)
{
#if 0
  memcpy((guix_preferences_t *) &guix_prefs, &default_guiprefs,
      sizeof(guix_prefs));

  // set default filename for saving the log
  guix_prefs.save_log_file = GlbspStrDup("glbsp.log");
#endif
}


/* ----- main program ----------------------------- */

int main(int argc, char **argv)
{
	// skip program name
	argv++, argc--;

	ArgvInit(argc, (const char **)argv);

	if (ArgvFind('?', NULL) >= 0 || ArgvFind('h', "help") >= 0)
	{
		ShowInfo();
		exit(1);
	}

	DebugInit(ArgvFind('d', "debug") >= 0);

	// set defaults, also initializes the nodebuildxxxx stuff
	MainSetDefaults();

#if 0
	// read persistent data
	CookieSetPath(argv[0]);

	cookie_status_t cookie_ret = CookieReadAll();
#endif

	// FIXME: handle arguments

	Fl::scheme(NULL);
	fl_message_font(FL_HELVETICA /* _BOLD */, 18);

	// load icons for file chooser
	Fl_File_Icon::load_system_icons();

//	LogInit();

    if (! nlInit())
	{
		fl_alert("Hawk network library failed to initialise.");
        exit(5); //!!!!
	}

	main_win = new UI_MainWin(MY_TITLE);

	LogPrintf(0,
		"\n*** " MY_TITLE " (C) 2005 Andrew Apted ***\n\n"
	);

	NetInit();

	NLthreadID net_thread = nlThreadCreate(NetRun, NULL, NL_TRUE);

	if (net_thread == (NLthreadID) NL_INVALID)
	{
		fl_alert("Unable to create networking thread.\n");
		exit(5); //!!!!
	}

	try
	{
		// run the GUI until the user quits
		while (! main_win->want_quit && ! net_failure)
		{
			Fl::wait(0.1f);

			main_win->Update();
		}

		if (net_failure)
			throw assert_fail_c(GetNetFailureMsg());
	}
	catch (assert_fail_c err)
	{
		fl_alert("Sorry, an internal error occurred:\n%s", err.GetMessage());
	}
	catch (...)
	{
		fl_alert("An unknown problem occurred (UI code)");
	}

	net_quit = true;

	if (! net_failure)
		nlThreadJoin(net_thread, NULL);

	delete main_win;
	main_win = NULL;

	DebugTerm();
	ArgvTerm();

	return 0;
}

