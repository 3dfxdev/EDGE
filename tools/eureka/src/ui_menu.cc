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

#include "yadex.h"
#include "ui_window.h"
#include "editloop.h"


//------------------------------------------------------------------------
//  FILE MENU
//------------------------------------------------------------------------

static void file_do_quit(Fl_Widget *w, void * data)
{
  main_win->action = UI_MainWin::QUIT;
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
  EditorKey(FL_F+2);
}

static void file_do_save_as(Fl_Widget *w, void * data)
{
  EditorKey(FL_F+3);
}


//------------------------------------------------------------------------
//  EDIT MENU
//------------------------------------------------------------------------

static void edit_do_undo(Fl_Widget *w, void * data)
{
  // TODO
}

static void edit_do_redo(Fl_Widget *w, void * data)
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
  EditorKey('o');
}

static void edit_do_insert(Fl_Widget *w, void * data)
{
  EditorKey(FL_Insert);
}

static void edit_do_delete(Fl_Widget *w, void * data)
{
  EditorKey(FL_Delete);
}

static void edit_do_unselect(Fl_Widget *w, void * data)
{
  // TODO
}


//------------------------------------------------------------------------
//  VIEW MENU
//------------------------------------------------------------------------

static void view_do_zoom_in(Fl_Widget *w, void * data)
{
  EditorKey('+');
}

static void view_do_zoom_out(Fl_Widget *w, void * data)
{
  EditorKey('-');
}

static void view_do_whole_level (Fl_Widget *w, void * data)
{
  // TODO
}


//------------------------------------------------------------------------
//  SEARCH MENU
//------------------------------------------------------------------------

static void search_do_find(Fl_Widget *w, void * data)
{
  EditorKey('f');
}

static void search_do_next(Fl_Widget *w, void * data)
{
  EditorKey('n');
}

static void search_do_prev(Fl_Widget *w, void * data)
{
  EditorKey('p');
}

static void search_do_jump(Fl_Widget *w, void * data)
{
  EditorKey('j');
}


//------------------------------------------------------------------------
//  MISC MENU
//------------------------------------------------------------------------

static void misc_do_offset(Fl_Widget *w, void * data)
{
  // TODO
}

static void misc_do_rotate_scale(Fl_Widget *w, void * data)
{
  // TODO
}

static void misc_do_mirror_horiz(Fl_Widget *w, void * data)
{
  // TODO
}

static void misc_do_mirror_vert(Fl_Widget *w, void * data)
{
  // TODO
}

static void misc_do_other_op(Fl_Widget *w, void * data)
{
  // TODO
}


//------------------------------------------------------------------------
//  HELP MENU
//------------------------------------------------------------------------

#if 0

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
  Fl_Box *box = new Fl_Box(0, cy, about->w(), 54, "EUREKA FTW");
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

#endif

void help_do_about(Fl_Widget *w, void * data) { }


//------------------------------------------------------------------------

#undef FCAL
#define FCAL  (Fl_Callback *)

static Fl_Menu_Item menu_items[] = 
{
  { "&File", 0, 0, 0, FL_SUBMENU },
    { "&New Map",   FL_COMMAND + 'n', FCAL file_do_new },
    { "&Open Map",  FL_COMMAND + 'o', FCAL file_do_open },
    { "&Save Map",  FL_COMMAND + 's', FCAL file_do_save },
    { "Save &As",   0,                FCAL file_do_save_as, 0, FL_MENU_DIVIDER },
    { "&Quit",      FL_COMMAND + 'q', FCAL file_do_quit },
    { 0 },

  { "&Edit", 0, 0, 0, FL_SUBMENU },
    { "&Undo",   FL_COMMAND + 'z',  FCAL edit_do_undo },
    { "&Redo",   FL_COMMAND + 'a',  FCAL edit_do_redo, 0, FL_MENU_DIVIDER },

    { "Cu&t",    FL_COMMAND + 'x',  FCAL edit_do_cut },
    { "&Copy",   FL_COMMAND + 'c',  FCAL edit_do_copy },
    { "&Paste",  FL_COMMAND + 'v',  FCAL edit_do_paste, 0, FL_MENU_DIVIDER },

    { "Insert",  FL_Insert,         FCAL edit_do_insert },
    { "Delete",  FL_Delete,         FCAL edit_do_delete },
    { "Unselect &All", FL_Enter,    FCAL edit_do_unselect, 0, FL_MENU_DIVIDER },

    { "&Move",      0, FCAL misc_do_offset },
    { "&Rotate",    0, FCAL misc_do_rotate_scale },
    { "&Scale",     0, FCAL misc_do_rotate_scale },
    { "Mirror &Horizontally",  0, FCAL misc_do_mirror_horiz },
    { "Mirror &Vertically",    0, FCAL misc_do_mirror_vert },
    { "&Other Operation...",   0, FCAL misc_do_other_op },

//??   "~Exchange object numbers", 24,     0,
//??   "~Preferences...",          FL_F+5,  0,
    { 0 },

  { "&View", 0, 0, 0, FL_SUBMENU },

    { "Zoom &In",     0, FCAL view_do_zoom_in },
    { "Zoom &Out",    0, FCAL view_do_zoom_out },
    { "&Whole Level", 0, FCAL view_do_whole_level },

    { "&Find Object",      0, FCAL search_do_find },
    { "&Next Object",      0, FCAL search_do_next },
    { "&Prev Object",      0, FCAL search_do_prev },
    { "&Jump to Object",   0, FCAL search_do_jump },
    { "&Show Object Nums", 0, FCAL search_do_jump },
    { 0 },

  { "&RTS", 0, 0, 0, FL_SUBMENU },
    { "&Open RTS File",   0, FCAL view_do_zoom_out },
    { "&Import from WAD", 0, FCAL view_do_zoom_out },
    { "&Save RTS File",   0, FCAL view_do_zoom_out },
    { 0 },

  { "&DDF", 0, 0, 0, FL_SUBMENU },
    { "Load DDF &File",    0, FCAL view_do_zoom_out },
    { "Load DDF &WAD",    0, FCAL view_do_zoom_out },
    { 0 },

  { "&Help", 0, 0, 0, FL_SUBMENU },
    { "&About Eureka",   0,  FCAL help_do_about },
    { 0 },

  { 0 }
};


#ifdef MACOSX
Fl_Sys_Menu_Bar * Menu_Create(int x, int y, int w, int h)
{
  Fl_Sys_Menu_Bar *bar = new Fl_Sys_Menu_Bar(x, y, w, h);
  bar->menu(menu_items);
  return bar;
}
#else
Fl_Menu_Bar * Menu_Create(int x, int y, int w, int h)
{
  Fl_Menu_Bar *bar = new Fl_Menu_Bar(x, y, w, h);
  bar->menu(menu_items);
  return bar;
}
#endif


#if 0  // UNUSED

static void Menu_ModeByName(Fl_Menu_Bar *bar, const char *name, int new_mode)
{
  for (int i = bar->size() - 1; i >= 0; i--)
  {
    const char *menu_text = bar->text(i);

    if (! menu_text)
      continue;

    if (menu_text[0] == '&')
      menu_text++;

    if (strcmp(menu_text, name) == 0)
    {
      bar->mode(i, new_mode);
      return;
    }
  }
}

void Menu_SetMode(char mode)
{
  Fl_Menu_Bar *bar = main_win->menu_bar;

  Menu_ModeByName(bar, "Things",   FL_SUBMENU | FL_MENU_INACTIVE);
  Menu_ModeByName(bar, "Linedefs", FL_SUBMENU | FL_MENU_INACTIVE);
  Menu_ModeByName(bar, "Sectors",  FL_SUBMENU | FL_MENU_INACTIVE);
  Menu_ModeByName(bar, "Vertices", FL_SUBMENU | FL_MENU_INACTIVE);
  Menu_ModeByName(bar, "RTS",      FL_SUBMENU | FL_MENU_INACTIVE);

  switch (mode)
  {
    case 't': Menu_ModeByName(bar, "Things",   FL_SUBMENU); break;
    case 'l': Menu_ModeByName(bar, "Linedefs", FL_SUBMENU); break;
    case 's': Menu_ModeByName(bar, "Sectors",  FL_SUBMENU); break;
    case 'v': Menu_ModeByName(bar, "Vertices", FL_SUBMENU); break;
    case 'r': Menu_ModeByName(bar, "RTS",      FL_SUBMENU); break;

    default: break;
  }

  bar->redraw();
}

#endif // UNUSED


//--- editor settings ---
// vi:ts=2:sw=2:expandtab
