//------------------------------------------------------------------------
//  Exit Progress window
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
#include "ui_exit.h"
#include "ui_window.h"


#if 0
	ab_win->show();

	// capture initial size
	WindowSmallDelay();
	/// int init_x = ab_win->x();
	/// int init_y = ab_win->y();

	// run the GUI until the user closes
	while (! menu_want_to_quit)
		Fl::wait();

#  if 0
	// check if the user moved/resized the window
	if (ab_win->x() != init_x || ab_win->y() != init_y)
	{
		guix_prefs.manual_x = ab_win->x();
		guix_prefs.manual_y = ab_win->y();
	}
#  endif

#endif


#define EXIT_WINDOW_W  400
#define EXIT_WINDOW_H  300


UI_ExitProgress::UI_ExitProgress(const char *title) :
	Fl_Window(/* !!!! */ 50, 50, EXIT_WINDOW_W, EXIT_WINDOW_H, title)
{
	// turn off auto-add-widget mode
	end();

	// non-resizable
	size_range(w(), h(), w(), h());

	//FIXME !!!!! callback((Fl_Callback *) exit_win_close_CB);

	set_modal();

	want_quit = false;

	// finally add the "Quit Now" button
	Fl_Button *quit_now = new Fl_Button(w()/2-30, h()-10-30, 
			60, 30, "Quit Now");

//FIXME !!!!	button->callback((Fl_Callback *) menu_quit_CB);
	add(quit_now);
}

UI_ExitProgress::~UI_ExitProgress()
{
}

void UI_ExitProgress::Update(int item, int progress)
{
	/// !!!
}

//------------------------------------------------------------------------

#if 0  // ????
static UI_ExitProgress *exit_prog;

//
// ExitProgressShow
//
void ExitProgressShow()
{
	exit_prog = new UI_ExitProgress();
}

//
// ExitProgressUpdate
//
// 'item' is 0 for booting games, 1 for booting clients.
// 'progress' ranges from 0 to 100 (percentage).
//
void ExitProgressUpdate(int item, int progress)
{
}

//
// ExitProgressRemove
//
void ExitProgressRemove()
{
}
#endif

