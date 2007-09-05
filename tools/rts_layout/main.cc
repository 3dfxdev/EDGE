//------------------------------------------------------------------------
//  Main program
//------------------------------------------------------------------------
//
//  RTS Layout Tool (C) 2007 Andrew Apted
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

#include "headers.h"
#include "hdr_fltk.h"

#include "lib_argv.h"
#include "lib_util.h"

#include "main.h"
#include "g_level.h"
#include "g_wad.h"

#include "ui_chooser.h"
#include "ui_dialog.h"
#include "ui_grid.h"
#include "ui_menu.h"
#include "ui_window.h"


#define TICKER_TIME  20 /* ms */

#define LOG_FILENAME     "LOGS.txt"


const char *working_path = NULL;
// const char *install_path = NULL;


/* ----- user information ----------------------------- */

static void ShowInfo(void)
{
  printf(
    "\n"
    "** " RTS_LAYOUT_TITLE " " RTS_LAYOUT_VERSION " (C) 2007 Andrew Apted **\n"
    "\n"
  );

  printf(
    "Usage: RTS_Layout [options...]\n"
    "\n"
    "Available options:\n"
    "  -h  -help         Show this help message\n"
    "\n"
  );

  printf(
    "This program is free software, under the terms of the GNU General\n"
    "Public License, and comes with ABSOLUTELY NO WARRANTY.  See the\n"
    "documentation for more details, or visit this web page:\n"
    "http://www.gnu.org/licenses/gpl.html\n"
    "\n"
  );
}

void Determine_WorkingPath(const char *argv0)
{
  // firstly find the "Working directory", and set it as the
  // current directory.  That's the place where the CONFIG.cfg
  // and LOGS.txt files are, as well the temp files.

#ifndef FHS_INSTALL
  working_path = GetExecutablePath(argv0);

#else
  working_path = StringNew(FL_PATH_MAX + 4);

  fl_filename_expand(working_path, "$HOME/.rts_layout");

  // try to create it (doesn't matter if it already exists)
  FileMakeDir(working_path);
#endif

  if (! working_path)
    working_path = StringDup(".");
}

#if 0 // CRUD??
void Determine_InstallPath(const char *argv0)
{
  // secondly find the "Install directory", and store the
  // result in the global variable 'install_path'.  This is
  // where all the LUA scripts and other data files are.

#ifndef FHS_INSTALL
  install_path = StringDup(working_path);

#else
  static const char *prefixes[] =
  {
    "/usr/local", "/usr", "/opt", NULL
  };

  for (int i = 0; prefixes[i]; i++)
  {
    install_path = StringPrintf("%s/share/rts_layout_%s",
        prefixes[i], RTS_LAYOUT_VERSION);

    const char *filename = StringPrintf("%s/scripts/oblige.lua", install_path);

fprintf(stderr, "Trying install path: [%s]\n  with file: [%s]\n\n",
install_path, filename)

    bool exists = FileExists(filename);

    StringFree(filename);

    if (exists)
      break;

    StringFree(install_path);
    install_path = NULL;
  }
#endif

  if (! install_path)
    Main_FatalError("Unable to find LUA script folder!\n");
}
#endif

void Main_Ticker()
{
  // This function is called very frequently.
  // To prevent a slow-down, we only call Fl::check()
  // after a certain time has elapsed.

  static u32_t last_millis = 0;

  u32_t cur_millis = TimeGetMillies();

  if ((cur_millis - last_millis) >= TICKER_TIME)
  {
    Fl::check();

    last_millis = cur_millis;
  }
}

void Main_Shutdown()
{
  if (main_win)
  {
//  Cookie_Save(CONFIG_FILENAME);

    delete main_win;
    main_win = NULL;
  }

  LogClose();
  ArgvClose();
}

void Main_FatalError(const char *msg, ...)
{
  static char buffer[MSG_BUF_LEN];

  va_list arg_pt;

  va_start(arg_pt, msg);
  vsnprintf(buffer, MSG_BUF_LEN-1, msg, arg_pt);
  va_end(arg_pt);

  buffer[MSG_BUF_LEN-2] = 0;

  DLG_ShowError("%s", buffer);

  Main_Shutdown();

  exit(9);
}


/* ----- main program ----------------------------- */

int main(int argc, char **argv)
{
  // initialise argument parser (skipping program name)
  ArgvInit(argc-1, (const char **)(argv+1));

  if (ArgvFind('?', NULL) >= 0 || ArgvFind('h', "help") >= 0)
  {
    ShowInfo();
    exit(1);
  }

  Fl::scheme("plastic");

  fl_message_font(FL_HELVETICA /* _BOLD */, 18);

  Determine_WorkingPath(argv[0]);
//  Determine_InstallPath(argv[0]);

  FileChangeDir(working_path);

  LogInit(LOG_FILENAME);

  if (ArgvFind('d', "debug") >= 0)
    LogEnableDebug();

  if (ArgvFind('t', "terminal") >= 0)
    LogEnableTerminal();

  LogPrintf(RTS_LAYOUT_TITLE " " RTS_LAYOUT_VERSION " (C) 2007 Andrew Apted\n\n");

  LogPrintf("working_path: [%s]\n",   working_path);
//  LogPrintf("install_path: [%s]\n\n", install_path);

  // load icons for file chooser
  Fl_File_Icon::load_system_icons();

  Default_Location();

  main_win = new UI_MainWin(RTS_LAYOUT_TITLE);

  // load config after creating window (set widget values)
//  Cookie_Load(CONFIG_FILENAME);

#if 1  // TEST CODE for MAP DRAWING
  wad_c *wad = wad_c::Load("PAR.wad");
  SYS_ASSERT(wad);

  SYS_ASSERT(wad->FindLevel("E1M2"));

  level_c *lev = level_c::LoadLevel(wad);
  SYS_ASSERT(lev);
  
  main_win->grid->SetMap(lev);
#endif 

  try
  {
    // run the GUI until the user quits
    while (! application_quit)
    {
      Fl::wait(0.2f);
    }
  }
  catch (...)
  {
    Main_FatalError("An unknown problem occurred (UI code)");
  }

  Main_Shutdown();

  return 0;
}

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
