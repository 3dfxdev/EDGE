//------------------------------------------------------------------------
//  Setup Screen
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

#include "buffer.h"
#include "client.h"
#include "game.h"
#include "packet.h"
#include "ui_setup.h"
#include "ui_window.h"

//
// Setup Constructor
//
UI_Setup::UI_Setup(int x, int y, int w, int h) :
    Fl_Group(x, y, w, h)
{
	// cancel the automatic `begin' in Fl_Group constructor
	end();
 
 	box(FL_THIN_UP_BOX);
	resizable(0);  // don't resize our children

	labeltype(FL_NORMAL_LABEL);
	align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT | FL_ALIGN_TOP);

	int cy = y + 20;

	address = new Fl_Output(x+140, cy, 160, 24, "Server Address: ");
	address->align(FL_ALIGN_LEFT);
	address->value("123.45.67.191");  //!!!!!
	add(address);

	cy += 28;

	port = new Fl_Output(x+140, cy, 60, 24, "Server Port: ");
	port->align(FL_ALIGN_LEFT);
	port->value("20702");
	add(port);

	cy += 28 + 14;

	max_clients = new Fl_Counter(x+140, cy, 140, 24, "Maximum Clients: ");
	max_clients->align(FL_ALIGN_LEFT);
	max_clients->range(4,1024);
	max_clients->step(4);
	max_clients->lstep(50);
	max_clients->value(32);
	add(max_clients);

	cy += 28;

	max_games = new Fl_Counter(x+140, cy, 140, 24, "Maximum Games: ");
	max_games->align(FL_ALIGN_LEFT);
	max_games->range(2,200);
	max_games->step(2);
	max_games->lstep(10);
	max_games->value(8);
	add(max_games);

	cy += 28;

}


//
// Setup Destructor
//
UI_Setup::~UI_Setup()
{
}

