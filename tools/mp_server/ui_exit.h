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

#ifndef __UI_EXIT_H__
#define __UI_EXIT_H__

class UI_ExitProgress : public Fl_Window
{
public:
	UI_ExitProgress(const char *title);
	virtual ~UI_ExitProgress();

private:
	bool want_quit;

	int prog_val[2];

	Fl_Light_Button *light_but[2];

	Fl_Progress *prog_bar;

public:
	void Update(int item, int progress);
	// 'item' is 0 for terminating games, 1 for booting clients.
	// 'progress' ranges from 0 to 100 (percentage).

	bool WantQuit() const { return want_quit; }
};

#if 0 // ????
void ExitProgressShow();
void ExitProgressUpdate(int item, int progress);
void ExitProgressRemove();
#endif

#endif /* __UI_EXIT_H__ */
