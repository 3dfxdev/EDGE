//------------------------------------------------------------------------
//  Statistics
//------------------------------------------------------------------------
//
//  Edge MultiPlayer Server (C) 2004  Andrew Apted
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
#include "ui_stats.h"
#include "ui_window.h"

//
// Stats Constructor
//
UI_Stats::UI_Stats(int x, int y, int w, int h) :
    Fl_Group(x, y, w, h)
{
	// cancel the automatic `begin' in Fl_Group constructor
	end();
 
 	box(FL_THIN_UP_BOX);
	resizable(0);  // don't resize our children

	labeltype(FL_NORMAL_LABEL);
	align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT | FL_ALIGN_TOP);

	clients = new Fl_Output(x+120, y+4, 50, 22, "Clients:");
	clients->align(FL_ALIGN_LEFT);
	clients->value("0");
	add(clients);

	queued = new Fl_Output(x+120, y+30, 50, 22, "Games Queued:");
	queued->align(FL_ALIGN_LEFT);
	queued->value("0");
	add(queued);

	played = new Fl_Output(x+120, y+56, 50, 22, "Games Played:");
	played->align(FL_ALIGN_LEFT);
	played->value("0");
	add(played);
}


//
// Stats Destructor
//
UI_Stats::~UI_Stats()
{
}

