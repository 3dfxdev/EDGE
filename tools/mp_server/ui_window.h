//------------------------------------------------------------------------
//  Main Window
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

#ifndef __UI_WINDOW_H__
#define __UI_WINDOW_H__

#include "ui_client.h"
#include "ui_game.h"
#include "ui_log.h"
#include "ui_menu.h"
#include "ui_setup.h"
#include "ui_stats.h"

#define MAIN_BG_COLOR  fl_gray_ramp(FL_NUM_GRAY * 9 / 24)

#define MAIN_WINDOW_MIN_W  560
#define MAIN_WINDOW_MIN_H  440

class UI_LogBox;

class UI_MainWin : public Fl_Window
{
public:
	UI_MainWin(const char *title);
	virtual ~UI_MainWin();

	// main child widgets

#ifdef MACOSX
	Fl_Sys_Menu_Bar *menu_bar;
#else
	Fl_Menu_Bar *menu_bar;
#endif

	Fl_Tabs *tabs;

	UI_Setup *setup_box;
	UI_ClientList *client_list;
	UI_GameList *game_list;

	UI_Stats *stat_box;
	UI_LogBox *log_box;

	// user closed the window
	bool want_quit;

	void Update()
	{
		stat_box->Update();
		log_box->Update();
	}

	// routine to capture the current main window state into the
	// guix_preferences_t structure.
	// 
	void WritePrefs();

protected:

	// initial window size, read after the window manager has had a
	// chance to move the window somewhere else.  If the window is still
	// there when CaptureState() is called, we don't need to update the
	// coords in the cookie file.
	// 
	int init_x, init_y, init_w, init_h;
};

extern UI_MainWin * main_win;

void WindowSmallDelay(void);


#endif /* __UI_WINDOW_H__ */
