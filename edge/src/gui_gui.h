//----------------------------------------------------------------------------
//  EDGE GUI Definitions
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
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
//----------------------------------------------------------------------------

#ifndef __GUI_GUI_H__
#define __GUI_GUI_H__

#include "dm_type.h"

// This header should be included by all gui apps

// Input event types.
typedef enum
{
	// Base events (see d_event.h) so guievent_t can be used
	// as an extension of event_t
	gev_keydown,
	gev_keyup,
	gev_analogue,

	// Creation events
	gev_spawn,
	gev_destroy,
	gev_losefocus,

	// If a responder returns false in relation to these two
	// events, the system will handle them.
	gev_move,
	gev_size,

	// Sent to do stuff like drag
	gev_hover,

	// All gui controls send messages >= 1000. So it is extendable
	gev_ctrl = 1000,
	gev_bnclick,
	gev_drag,

	// Events greater than gev_user are free for user applications
	gev_user = 10000
}
guievtype_t;

// Event structure.
typedef struct
{
	guievtype_t type;
	int data1;  // buttons (keys) / data2 axis
	int data2;  // analogue axis 1
	int data3;  // data4 axis
	int data4;  // analogue axis 2
}
guievent_t;

typedef struct gui_s
{
	// The id is essentially a user field.  Usually used to identify
	// controls in sub windows.  eg you create buttons with ids 0, 1...
	// and the buttons send their id numbers back when they are pushed,
	// so you know which button is clicked.
	int id;
	// The ticker function is where you do the main thinking of the application
	// If there is no ticker, set this field to NULL.  Remember that this is
	// not a real multitasking environment, so no big for loops please!
	// The parameter passed is a pointer to the structure, so it is called
	// (by the system as gui_t *gui;  gui->Ticker(gui);
	// Tickers are called 35 times per second.
	void (*Ticker) (struct gui_s *gui);
	// This function is used to respond to events like key presses and spawn
	// events.  Passes a pointer to the structure (see Ticker) and a pointer
	// to the event to process.  (Don't modify the event.) Set to NULL to
	// ignore all events.
	bool(*Responder) (struct gui_s *gui, guievent_t * event);
	// Should draw the app to the screen.  Once again optional and passed
	// a pointer to the structure.
	void (*Drawer) (struct gui_s * gui);
	// This is a user pointer.  To make apps "thread safe" in this mock
	// gui, store all 'global' variables in a structure pointed to by this.
	// Otherwise, you can only have one instance of your app open at any one
	// time.
	void *process;
	// These variables store the position of your app on the screen.  You should
	// not draw outside these coordinates.
	int left, right, top, bottom;
	// The gui is stored as a circular linked list.
	struct gui_s *prev, *next;
	// The entire gui system is stored in a single gui_t*, which points to the
	// forground application.  This field points to that variable.  You can create
	// other guis, eg dialog boxes which consist of gui's (buttons, check boxes etc)
	// that are sub windows your window.  (Pheww!) (See message box?)
	struct gui_s **gui;
}
gui_t;

// This returns a pointer to the top level gui system.
gui_t **GUI_NULL(void);

// This initialises a gui_t * to be used as a gui.
// eg:  gui_t *gui;
//      GUI_Init(&gui);
void GUI_Init(gui_t ** gui);

// Adds an application g to the gui.
void GUI_Start(gui_t ** gui, gui_t * g);

// Removes an application from it's gui
void GUI_Destroy(gui_t * g);

// Sets the foreground application
void GUI_SetFocus(gui_t ** gui, gui_t * g);

// Calls all the Tickers for the specified gui
void GUI_Ticker(gui_t ** gui);

// Sends an event to the specified gui.  Usually only the foreground
// application's responder is called.
bool GUI_Responder(gui_t ** gui, guievent_t * e);

// Draws all the windows in the gui, using Painters' Algorithm
void GUI_Drawer(gui_t ** gui);

// Enables disables the mouse cursor.
bool GUI_MainGetMouseVisibility(void);
void GUI_MainSetMouseVisibility(bool visible);

// Sets the mouse lump
bool GUI_SetMouse(char *name);

#ifdef CON_MAIN_H
// include the gui parts of con_main.h if con_main earlier has been included.
#include "con_main.h"
#endif

#endif
