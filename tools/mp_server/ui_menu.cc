//------------------------------------------------------------------------
//  Menus
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
#include "ui_menu.h"
#include "ui_window.h"


static bool menu_want_to_quit;


static void menu_quit_CB(Fl_Widget *w, void *data)
{
  menu_want_to_quit = true;
}

#ifndef MACOSX
static void menu_do_exit(Fl_Widget *w, void * data)
{
  main_win->want_quit = true;
}
#endif


//------------------------------------------------------------------------

static const char *about_Info =
  "By Andrew Apted (C) 2004\n"
  "\n"
  "This program is free software, under the terms of\n"
  "the GNU General Public License, and comes with\n"
  "ABSOLUTELY NO WARRANTY.\n"
  "\n";


static void menu_do_about(Fl_Widget *w, void * data)
{
	menu_want_to_quit = false;

	Fl_Window *ab_win = new Fl_Window(600, 340, "About Edge MultiPlayer Server");
	ab_win->end();

	// non-resizable
	ab_win->size_range(ab_win->w(), ab_win->h(), ab_win->w(), ab_win->h());
///	ab_win->position(guix_prefs.manual_x, guix_prefs.manual_y);
	ab_win->callback((Fl_Callback *) menu_quit_CB);

#if 0
	// add the about image
	Fl_Group *group = new Fl_Group(0, 0, 230, ab_win->h());
	group->box(FL_FLAT_BOX);
	group->color(FL_BLACK, FL_BLACK);
	ab_win->add(group);

	Fl_Box *box = new Fl_Box(20, 90, ABOUT_IMG_W+2, ABOUT_IMG_H+2);
	box->image(about_image);
	group->add(box); 


	// nice big logo text
	box = new Fl_Box(240, 5, 350, 50, "glBSPX  " GLBSP_VER);
	box->labelsize(24);
	ab_win->add(box);

	// about text
	box = new Fl_Box(240, 60, 350, 270, about_Info);
	box->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT | FL_ALIGN_TOP);
	ab_win->add(box);
#endif

	// finally add an "OK" button
	Fl_Button *button = new Fl_Button(ab_win->w()-10-60, ab_win->h()-10-30, 
			60, 30, "OK");
	button->callback((Fl_Callback *) menu_quit_CB);
	ab_win->add(button);

///	ab_win->set_modal();
	ab_win->show();

	// capture initial size
	WindowSmallDelay();
	/// int init_x = ab_win->x();
	/// int init_y = ab_win->y();

	// run the GUI until the user closes
	while (! menu_want_to_quit)
		Fl::wait();

#if 0
	// check if the user moved/resized the window
	if (ab_win->x() != init_x || ab_win->y() != init_y)
	{
		guix_prefs.manual_x = ab_win->x();
		guix_prefs.manual_y = ab_win->y();
	}
#endif

	// this deletes all the child widgets too...
	delete ab_win;
}

static void menu_do_save_log(Fl_Widget *w, void * data)
{
	// FIXME
}

//------------------------------------------------------------------------

#undef FCAL
#define FCAL  (Fl_Callback *)

static Fl_Menu_Item menu_items[] = 
{
	{ "&File", 0, 0, 0, FL_SUBMENU },
#ifdef MACOSX
		{ "&Save Log",         0, FCAL menu_do_save_log },
#else
		{ "&Save Log",         0, FCAL menu_do_save_log, 0, FL_MENU_DIVIDER },
		{ "E&xit",  0 /* FL_ALT + 'q' */, FCAL menu_do_exit },
#endif
		{ 0 },

	{ "&Help", 0, 0, 0, FL_SUBMENU },
		{ "&About...",         0,  FCAL menu_do_about },
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

