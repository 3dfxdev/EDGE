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


//------------------------------------------------------------------------
//  FILE MENU
//------------------------------------------------------------------------

static void file_do_quit(Fl_Widget *w, void * data)
{
  application_quit = true;
}

static void file_do_new(Fl_Widget *w, void * data)
{
  // TODO
}

static void file_do_open(Fl_Widget *w, void * data)
{
  // TODO
}

static void file_do_save(Fl_Widget *w, void * data)
{
  // TODO
}

static void file_do_save_as(Fl_Widget *w, void * data)
{
  // TODO
}


//------------------------------------------------------------------------
//  EDIT MENU
//------------------------------------------------------------------------

static void edit_do_undo(Fl_Widget *w, void * data)
{
  // TODO
}

static void edit_do_cut(Fl_Widget *w, void * data)
{
  // TODO
}

static void edit_do_copy(Fl_Widget *w, void * data)
{
  // TODO
}

static void edit_do_paste(Fl_Widget *w, void * data)
{
  // TODO
}

static void edit_do_unselect(Fl_Widget *w, void * data)
{
  // TODO
}


//------------------------------------------------------------------------
//  SCRIPT MENU
//------------------------------------------------------------------------

static void script_do_load_maps(Fl_Widget *w, void * data)
{
  // TODO
}

static void script_do_change_map(Fl_Widget *w, void * data)
{
  // TODO
}

static void script_do_new(Fl_Widget *w, void * data)
{
  // TODO
}

static void script_do_delete(Fl_Widget *w, void * data)
{
  // TODO
}


//------------------------------------------------------------------------
//  THING MENU
//------------------------------------------------------------------------

static void thing_do_load_ddf_file(Fl_Widget *w, void * data)
{
  // TODO
}

static void thing_do_load_ddf_wad(Fl_Widget *w, void * data)
{
  // TODO
}

static void thing_do_new(Fl_Widget *w, void * data)
{
  // TODO
}

static void thing_do_delete(Fl_Widget *w, void * data)
{
  // TODO
}

static void thing_do_find_type(Fl_Widget *w, void * data)
{
  // TODO
}


//------------------------------------------------------------------------
//  HELP MENU
//------------------------------------------------------------------------

static bool about_box_quit;

static void about_quit_CB(Fl_Widget *w, void *data)
{
  about_box_quit = true;
}

static const char *about_Text =
  "RTS Layout Tool allows you to position your\n"
  "EDGE RTS scripts over a map of your level.\n"
  "\n"
  "Copyright (C) 2007 Andrew Apted\n"
  "\n"
  "This program is free software, and may be\n"
  "distributed and modified under the terms of\n"
  "the GNU General Public License\n"
  "\n"
  "There is ABSOLUTELY NO WARRANTY\n"
  "Use at your OWN RISK!";

static const char *about_Web =
  "edge.sourceforge.net";

#define TITLE_COLOR  FL_RED

#define INFO_COLOR  fl_rgb_color(255, 255, 128)
 

void help_do_about(Fl_Widget *w, void * data)
{
  about_box_quit = false;

  Fl_Window *about = new Fl_Window(326, 340, "About RTS_Layout");
  about->end();

  // non-resizable
  about->size_range(about->w(), about->h(), about->w(), about->h());
  about->callback((Fl_Callback *) about_quit_CB);

  int cy = 0;

  // nice big logo text
  Fl_Box *box = new Fl_Box(0, cy, about->w(), 54, RTS_LAYOUT_TITLE " " RTS_LAYOUT_VERSION);
  box->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);
  box->labelcolor(TITLE_COLOR);
  box->labelsize(24);
  about->add(box);


  cy += box->h();
  
  // the very informative text
  box = new Fl_Box(10, cy, about->w()-20, 192, about_Text);
  box->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);
  box->box(FL_UP_BOX);
  box->color(INFO_COLOR);
  about->add(box);

  cy += box->h() + 6;


  // website address
  box = new Fl_Box(0, cy, about->w()-20, 28, about_Web);
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
  button->callback((Fl_Callback *) about_quit_CB);
  darkish->add(button);

/// about->set_modal();

  about->show();

  // run the GUI until the user closes
  while (! about_box_quit)
    Fl::wait();

  // this deletes all the child widgets too...
  delete about;
}


//------------------------------------------------------------------------

#undef FCAL
#define FCAL  (Fl_Callback *)

static Fl_Menu_Item menu_items[] = 
{
  { "&File", 0, 0, 0, FL_SUBMENU },
    { "&New Script",   0,             FCAL file_do_new },
    { "&Open Script",  FL_CTRL + 'o', FCAL file_do_open },
    { "&Save Script",  FL_CTRL + 's', FCAL file_do_save },
    { "Save &As",      0,             FCAL file_do_save_as, 0, FL_MENU_DIVIDER },
    { "&Quit",         FL_CTRL + 'q', FCAL file_do_quit },
    { 0 },

  { "&Edit", 0, 0, 0, FL_SUBMENU },
    { "&Undo",   FL_CTRL + 'z',  FCAL edit_do_undo, 0, FL_MENU_DIVIDER },
    { "Cu&t",    FL_CTRL + 'x',  FCAL edit_do_cut },
    { "&Copy",   FL_CTRL + 'c',  FCAL edit_do_copy },
    { "&Paste",  FL_CTRL + 'v',  FCAL edit_do_paste, 0, FL_MENU_DIVIDER },

    { "Unselect &All", FL_CTRL + 'a', FCAL edit_do_unselect },
    { 0 },

  { "&Script", 0, 0, 0, FL_SUBMENU },
    { "&Load Map Wad",   0, FCAL script_do_load_maps },
    { "&Change Map",     0, FCAL script_do_change_map, 0, FL_MENU_DIVIDER },
    { "&New Trigger",    0, FCAL script_do_new },
    { "&Delete Trigger", 0, FCAL script_do_delete },
    { 0 },

  { "&Thing", 0, 0, 0, FL_SUBMENU },
    { "&Load DDF File", 0, FCAL thing_do_load_ddf_file },
    { "&Load DDF Wad",  0, FCAL thing_do_load_ddf_wad, 0, FL_MENU_DIVIDER },
    { "&New Thing",     0, FCAL thing_do_new },
    { "&Delete Thing",  0, FCAL thing_do_delete },
    { "&Find Type",     0, FCAL thing_do_find_type },
    { 0 },

  { "&Help", 0, 0, 0, FL_SUBMENU },
    { "&About RTS Layout",   0,  FCAL help_do_about },
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
