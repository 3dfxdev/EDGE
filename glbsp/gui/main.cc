//------------------------------------------------------------------------
// MAIN : Unix/FLTK Main program
//------------------------------------------------------------------------
//
//  GL-Friendly Node Builder (C) 2000-2007 Andrew Apted
//
//  Based on 'BSP 2.3' by Colin Reed, Lee Killough and others.
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

// this includes everything we need
#include "local.h"


#define MY_TITLE  ("glBSP Node Builder " GLBSP_VER)


// Node building info

nodebuildinfo_t guix_info;

volatile nodebuildcomms_t guix_comms;

const nodebuildfuncs_t guix_funcs =
{
  GUI_FatalError,
  GUI_PrintMsg,
  GUI_Ticker,

  GUI_DisplayOpen,
  GUI_DisplaySetTitle,
  GUI_DisplaySetBar,
  GUI_DisplaySetBarLimit,
  GUI_DisplaySetBarText,
  GUI_DisplayClose
};


// GUI-specific Globals

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


/* ----- user information ----------------------------- */


static void ShowTitle(void)
{
  GUI_PrintMsg(
    "\n"
    "**** GLBSP Node Builder " GLBSP_VER " (C) 2007 Andrew Apted ****\n\n"
  );
}

static void ShowInfo(void)
{
  GUI_PrintMsg(
    "This GL node builder was originally based on BSP 2.3, which was\n"
    "created from the basic theory stated in DEU5 (OBJECTS.C)\n"
    "\n"
    "Credits should go to :-\n"
    "  Andy Baker & Marc Pullen   for their invaluable help\n"
    "  Janis Legzdinsh            for fixing up Hexen support\n"
    "  Colin Reed & Lee Killough  for creating the original BSP\n"
    "  Matt Fell                  for the Doom Specs\n"
    "  Raphael Quinet             for DEU and the original idea\n"
    "  ... and everyone who helped with the original BSP.\n"
    "\n"
    "This program is free software, under the terms of the GNU General\n"
    "Public License, and comes with ABSOLUTELY NO WARRANTY.  See the\n"
    "accompanying documentation for more details.\n"
    "\n"
    "Note: glBSPX is the GUI (graphical user interface) version.\n"
    "Try plain \"glbsp\" if you want the command-line version.\n"
  );
}


void MainSetDefaults(void)
{
  // this is more messy than it was in C
  memcpy((nodebuildinfo_t  *) &guix_info,  &default_buildinfo,  
      sizeof(guix_info));

  memcpy((nodebuildcomms_t *) &guix_comms, &default_buildcomms, 
      sizeof(guix_comms));

  memcpy((guix_preferences_t *) &guix_prefs, &default_guiprefs,
      sizeof(guix_prefs));

  // set default filename for saving the log
  guix_prefs.save_log_file = GlbspStrDup("glbsp.log");
}


/* ----- main program ----------------------------- */



int main(int argc, char **argv)
{
  if (argc > 1 &&
      (strcmp(argv[1], "/?") == 0 || strcmp(argv[1], "-h") == 0 ||
       strcmp(argv[1], "-help") == 0 || strcmp(argv[1], "--help") == 0 ||
       strcmp(argv[1], "-HELP") == 0 || strcmp(argv[1], "--HELP") == 0))
  {
    ShowTitle();
    ShowInfo();
    exit(1);
  }

  int first_arg = 1;

#ifdef MACOSX
  if (first_arg < argc && (strncmp(argv[first_arg], "-psn", 4) == 0))
    first_arg++;
#endif

  // set defaults, also initializes the nodebuildxxxx stuff
  MainSetDefaults();

  // read persistent data
  CookieSetPath(argv[0]);

  cookie_status_t cookie_ret = CookieReadAll();

  // handle drag and drop: a single non-option argument
  //
  // NOTE: there is no support for giving options to glBSPX via the
  // command line.  Plain 'glbsp' should be used if this is desired.
  // The difficult here lies in possible conflicts between given
  // options and those already set from within the GUI.  Plus we may
  // want to handle a drag-n-drop of multiple files later on.
  //
  boolean_g unused_args = FALSE;

  if (first_arg < argc && argv[first_arg][0] != '-')
  {
    GlbspFree(guix_info.input_file);
    GlbspFree(guix_info.output_file);

    guix_info.input_file = GlbspStrDup(argv[first_arg]);

    // guess an output name too
    
    guix_info.output_file = GlbspStrDup(
        HelperGuessOutput(guix_info.input_file));
 
    first_arg++;
  }

  if (first_arg < argc)
    unused_args = TRUE;


  // load icons for file chooser
  Fl_File_Icon::load_system_icons();

  guix_win = new Guix_MainWin(MY_TITLE);
   
  DialogLoadImages();

  ShowTitle();

  switch (cookie_ret)
  {
    case COOKIE_E_OK:
      break;

    case COOKIE_E_NO_FILE:
      guix_win->text_box->AddMsg(
          "** Missing INI file -- Using defaults **\n\n", FL_RED, TRUE);
      break;

    case COOKIE_E_PARSE_ERRORS:
    case COOKIE_E_CHECK_ERRORS:
      guix_win->text_box->AddMsg(
          "** Warning: Errors found in INI file **\n\n", FL_RED, TRUE);
      break;
  }

  if (unused_args)
    guix_win->text_box->AddMsg(
        "** Warning: Ignoring extra arguments to glBSPX **\n\n", FL_RED, TRUE);

  // run the GUI until the user quits
  while (! guix_win->want_quit)
    Fl::wait();

  delete guix_win;
  guix_win = NULL;

  CookieWriteAll();

  DialogFreeImages();

  return 0;
}

