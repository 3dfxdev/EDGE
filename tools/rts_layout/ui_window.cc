//------------------------------------------------------------------------
//  Main Window
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

#include "ui_menu.h"
#include "ui_window.h"
#include "ui_grid.h"
#include "ui_panel.h"

#ifndef WIN32
#include <unistd.h>
#endif

#if (FL_MAJOR_VERSION != 1 ||  \
     FL_MINOR_VERSION != 1 ||  \
     FL_PATCH_VERSION < 7)
#error "Require FLTK version 1.1.7 or later"
#endif


UI_MainWin *main_win;

#define WINDOW_WIDTH   (600)
#define WINDOW_HEIGHT  (440)

#define MIN_WINDOW_W   (480)
#define MIN_WINDOW_H   WINDOW_HEIGHT


static void main_win_close_CB(Fl_Widget *w, void *data)
{
  if (main_win)
    main_win->action = UI_MainWin::QUIT;
}


//
// MainWin Constructor
//
UI_MainWin::UI_MainWin(const char *title) :
    Fl_Double_Window(WINDOW_WIDTH, WINDOW_HEIGHT, title),
    action(UI_MainWin::NONE)
{
  end(); // cancel begin() in Fl_Group constructor

  size_range(MIN_WINDOW_W, MIN_WINDOW_H);

  callback((Fl_Callback *) main_win_close_CB);

  color(MAIN_BG_COLOR, MAIN_BG_COLOR);
  image(NULL);


  int cy = 0;

  /* ---- Menu bar ---- */
  {
    menu_bar = MenuCreate(0, 0, w()-202, 28);
    add(menu_bar);

#ifndef MACOSX
    cy += menu_bar->h();
#endif
  }

  grid = new UI_Grid(0, cy, w()-200, h()-cy);
  add(grid);

  panel = new UI_Panel(w()-200, 0, 200, h());
  add(panel);

  resizable(grid);

  // show window (pass some dummy arguments)
  int argc = 1;
  char *argv[] = { "RTS_Layout.exe", NULL };

  show(argc, argv);
}

//
// MainWin Destructor
//
UI_MainWin::~UI_MainWin()
{ }


void UI_MainWin::Locked(bool value)
{
}

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
