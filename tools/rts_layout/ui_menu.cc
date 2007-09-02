//------------------------------------------------------------------------
//  Menus
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
#include "main.h"


static bool menu_want_to_quit;


static void menu_quit_CB(Fl_Widget *w, void *data)
{
  menu_want_to_quit = true;
}

#ifndef MACOSX
static void menu_do_exit(Fl_Widget *w, void * data)
{
  main_win->action = UI_MainWin::QUIT;
}
#endif


//------------------------------------------------------------------------

static const char *about_Text =
  "This program allows you to position your\n"
  "EDGE RTS scripts on a map of the level.\n"
  "\n"
  "Copyright (C) 2007 Andrew Apted\n"
  "\n"
  "This program is free software, and may be\n"
  "distributed and modified under the terms of\n"
  "the GNU General Public License\n"
  "\n"
  "There is ABSOLUTELY NO WARRANTY\n"
  "Use at your OWN RISK";

static const char *about_Web =
  "http://edge.sourceforge.net";

#define TITLE_COLOR  FL_BLUE

#define INFO_COLOR  fl_color_cube(4,2,0)
  

void menu_do_about(Fl_Widget *w, void * data)
{
  menu_want_to_quit = false;

  Fl_Window *about = new Fl_Window(340, 364, "About RTS_Layout");
  about->end();

  // non-resizable
  about->size_range(about->w(), about->h(), about->w(), about->h());
  about->callback((Fl_Callback *) menu_quit_CB);

  int cy = 0;

  // nice big logo text
  Fl_Box *box = new Fl_Box(0, cy, about->w(), 50, RTS_LAYOUT_TITLE " " RTS_LAYOUT_VERSION);
  box->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);
  box->labelcolor(TITLE_COLOR);
  box->labelsize(24);
  about->add(box);


  cy += box->h() + 10;
  
  // the very informative text
  box = new Fl_Box(10, cy, about->w()-20, 192, about_Text);
  box->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);
  box->box(FL_UP_BOX);
  box->color(INFO_COLOR);
  about->add(box);

  cy += box->h() + 10;


  // website address
  box = new Fl_Box(10, cy, about->w()-20, 30, about_Web);
  box->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);
  box->labelsize(20);
  about->add(box);

  cy += box->h() + 10;


  SYS_ASSERT(cy < about->h());

  Fl_Group *darkish = new Fl_Group(0, cy, about->w(), about->h()-cy);
  darkish->end();
  darkish->box(FL_FLAT_BOX);
  darkish->color(FL_DARK3, FL_DARK3);
  about->add(darkish);

  // finally add an "OK" button
  Fl_Button *button = new Fl_Button(about->w()-10-60, about->h()-10-30, 
      60, 30, "OK");
  button->callback((Fl_Callback *) menu_quit_CB);
  darkish->add(button);

/// about->set_modal();

  about->show();

  // run the GUI until the user closes
  while (! menu_want_to_quit)
    Fl::wait();

  // this deletes all the child widgets too...
  delete about;
}

static void menu_do_save_log(Fl_Widget *w, void * data)
{
  // TODO
}

//------------------------------------------------------------------------

#undef FCAL
#define FCAL  (Fl_Callback *)

static Fl_Menu_Item menu_items[] = 
{
  { "&File", 0, 0, 0, FL_SUBMENU },
    { "&New Script",   0,             FCAL menu_do_save_log },
    { "&Open Script",  FL_CTRL + 'o', FCAL menu_do_save_log },
//  { "&Open WAD",     0,             FCAL menu_do_save_log },
    { "&Save",         FL_CTRL + 's', FCAL menu_do_save_log },
    { "Save &As",      0,             FCAL menu_do_save_log, 0, FL_MENU_DIVIDER },
    { "&Quit",   FL_CTRL + 'q', FCAL menu_do_exit },
    { 0 },

  { "&Edit", 0, 0, 0, FL_SUBMENU },
    { "&Undo",   FL_CTRL + 'z',  FCAL menu_do_about, 0, FL_MENU_DIVIDER },
    { "Cu&t",    FL_CTRL + 'x',  FCAL menu_do_about },
    { "&Copy",   FL_CTRL + 'c',  FCAL menu_do_about },
    { "&Paste",  FL_CTRL + 'v',  FCAL menu_do_about, 0, FL_MENU_DIVIDER },
    { "Unselect All",  0,        FCAL menu_do_about },
    { 0 },

  { "&Script", 0, 0, 0, FL_SUBMENU },
    { "&Change Map...",  0,  FCAL menu_do_about, 0, FL_MENU_DIVIDER },
    { "&New Trigger...", 0,  FCAL menu_do_about },
    { "&Delete",         0,  FCAL menu_do_about },
    { 0 },

  { "&Thing", 0, 0, 0, FL_SUBMENU },
    { "&Load DDF...",  0,  FCAL menu_do_about, 0, FL_MENU_DIVIDER },
    { "&New Thing...", 0,  FCAL menu_do_about },
    { "&Delete",       0,  FCAL menu_do_about },
    { 0 },

  { "&Help", 0, 0, 0, FL_SUBMENU },
    { "&About...",   0,  FCAL menu_do_about },
    { 0 },

  { 0 }
};


//
// MenuCreate
//
#ifdef MACOSX
Fl_Sys_Menu_Bar * MenuCreate(int x, int y, int w, int h)
{
  Fl_Sys_Menu_Bar *bar = new Fl_Sys_Menu_Bar(x, y, w, h);
  bar->menu(menu_items);
  return bar;
}
#else
Fl_Menu_Bar * MenuCreate(int x, int y, int w, int h)
{
  Fl_Menu_Bar *bar = new Fl_Menu_Bar(x, y, w, h);
  bar->menu(menu_items);
  return bar;
}
#endif


//--- editor settings ---
// vi:ts=2:sw=2:expandtab
