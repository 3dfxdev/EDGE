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
#define EXIT_WINDOW_H  220


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
	prog_val[0] = prog_val[1] = 0;

	int cy = 10;

	Fl_Box *top_msg = new Fl_Box(10, cy, w() - 20, 30, "Performing shutdown sequence...");
	top_msg->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
	add(top_msg);

	cy += top_msg->h() + 15;
	
	light_but[0] = new Fl_Light_Button(50, cy, 20, 20, "Terminating all games");
	light_but[0]->box(FL_NO_BOX);
	light_but[0]->set_output();
	light_but[0]->align(FL_ALIGN_RIGHT);
	add(light_but[0]);

	cy += light_but[0]->h() + 15;

	light_but[1] = new Fl_Light_Button(50, cy, 20, 20, "Booting off all clients");
	light_but[1]->box(FL_NO_BOX);
	light_but[1]->set_output();
	light_but[1]->align(FL_ALIGN_RIGHT);
	add(light_but[1]);

	cy += light_but[1]->h() + 20;

	prog_bar = new Fl_Progress(50, cy, w()-100, 20);
	prog_bar->minimum(0);
	prog_bar->maximum(100);
	add(prog_bar);

	cy += prog_bar->h() + 20;

	// finally add the "Quit Now" button
	Fl_Button *quit_now = new Fl_Button(w()/2-35, cy,
			70, 30, "Quit Now");

//FIXME !!!!	button->callback((Fl_Callback *) menu_quit_CB);
	add(quit_now);
}

UI_ExitProgress::~UI_ExitProgress()
{
}

void UI_ExitProgress::Update(int item, int progress)
{
	SYS_ASSERT(0 <= item && item <= 1);
	SYS_ASSERT(0 <= progress && progress <= 100);

	prog_val[item] = progress;

	if (progress == 100)
		light_but[item]->set();

	int total = (prog_val[0] + prog_val[1]) / 2;

	prog_bar->value(total);
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

