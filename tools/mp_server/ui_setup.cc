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
 
	/* ----- Server panel ----- */

	int gy = y;

	Fl_Group *sv_info = new Fl_Group(x, gy, w, 160, "\n Server Panel");
	sv_info->box(FL_THIN_UP_BOX);
	sv_info->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT | FL_ALIGN_TOP);
	sv_info->labeltype(FL_NORMAL_LABEL);
	sv_info->labelfont(FL_HELVETICA_BOLD);
	add(sv_info);

	gy += sv_info->h();

	int cy = sv_info->y() + 40;

	address = new Fl_Output(x+100, cy, 160, 24, "Address: ");
	address->align(FL_ALIGN_LEFT);
	address->value("123.45.67.191");  //!!!!!
	sv_info->add(address);

	cy += 32;

	port = new Fl_Output(x+100, cy, 60, 24, "Port: ");
	port->align(FL_ALIGN_LEFT);
	port->value("20702");
	sv_info->add(port);

	cy += 32;

	Fl_Output *status = new Fl_Output(x + 100, cy, 160, 24, "Status: ");
	status->align(FL_ALIGN_LEFT);
	status->value("Not running");
	sv_info->add(status);

	cy = sv_info->y() + 50;

	Fl_Button *go_but = new Fl_Button(x + 340, cy, 120, 28, "START SERVER");
	go_but->box(FL_ROUND_UP_BOX);
	go_but->color(FL_GREEN);
	sv_info->add(go_but);

	cy += 44;

	Fl_Button *stop_but = new Fl_Button(x + 340, cy, 120, 28, "STOP SERVER");
	stop_but->box(FL_ROUND_UP_BOX);
	stop_but->color(FL_RED);
	sv_info->add(stop_but);

	/* ----- Limits panel ----- */
	
	Fl_Group *lim_info = new Fl_Group(x, gy, w, 130, "\n Limits Panel");
	lim_info->box(FL_THIN_UP_BOX);
	lim_info->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT | FL_ALIGN_TOP);
	lim_info->labeltype(FL_NORMAL_LABEL);
	lim_info->labelfont(FL_HELVETICA_BOLD);
	add(lim_info);

	gy += lim_info->h();

	cy = lim_info->y() + 40;

	max_clients = new Fl_Counter(x+150, cy, 140, 24, "Clients: ");
	max_clients->align(FL_ALIGN_LEFT);
	max_clients->type(FL_SIMPLE_COUNTER);
	max_clients->range(16,1024-16);
	max_clients->step(32);
	// max_clients->lstep(50);
	max_clients->value(16);
	lim_info->add(max_clients);

	max_games = new Fl_Counter(x+380, cy, 140, 24, "Games: ");
	max_games->align(FL_ALIGN_LEFT);
	max_games->type(FL_SIMPLE_COUNTER);
	max_games->range(4,200);
	max_games->step(4);
	// max_games->lstep(10);
	max_games->value(4);
	lim_info->add(max_games);

	cy += 32;

	max_plyrs = new Fl_Counter(x+150, cy, 140, 24, "Players per game: ");
	max_plyrs->align(FL_ALIGN_LEFT);
	max_plyrs->type(FL_SIMPLE_COUNTER);
	max_plyrs->range(2, MP_PLAYER_MAX);
	max_plyrs->step(2);
	max_plyrs->value(8);
	lim_info->add(max_plyrs);

	max_bots = new Fl_Counter(x+380, cy, 140, 24, "Bots: ");
	max_bots->align(FL_ALIGN_LEFT);
	max_bots->type(FL_SIMPLE_COUNTER);
	max_bots->range(0,16);
	max_bots->step(2);
	max_bots->value(4);
	lim_info->add(max_bots);

	/* ----- Authorisation panel ----- */

	Fl_Group *act_panel = new Fl_Group(x, gy, w, h - gy, "\n Authorisation Panel");
	act_panel->box(FL_THIN_UP_BOX);
	act_panel->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT | FL_ALIGN_TOP);
	act_panel->labeltype(FL_NORMAL_LABEL);
	act_panel->labelfont(FL_HELVETICA_BOLD);
	add(act_panel);

	gy += act_panel->h();

	// TODO: authorisation stuff (passwords for game creation).

	resizable(act_panel);  // TEMP
}


//
// Setup Destructor
//
UI_Setup::~UI_Setup()
{
}

