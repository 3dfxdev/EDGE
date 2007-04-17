//----------------------------------------------------------------------------
//  EDGE GUI Controls
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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

#include "i_defs.h"
#include "gui_ctls.h"

#include "con_main.h"
#include "con_cvar.h"
#include "dm_state.h"
#include "gui_gui.h"
#include "hu_stuff.h"
#include "m_menu.h"
#include "m_misc.h"
#include "r_modes.h"
#include "v_colour.h"
#include "z_zone.h"

typedef struct button_s
{
	bool status;
	char *string;
	gui_t *parent;
}
button_t;

typedef struct msgbox_s
{
	int msg;
	char *string;
	gui_t *gui;
	gui_t *parent;
}
msgbox_t;

typedef struct drag_s
{
	bool mouse;
	gui_t *parent;
}
drag_t;

void BT_Drawer(gui_t * g);
bool BT_Responder(gui_t * g, guievent_t * ev);
void MSG_Drawer(gui_t * g);
bool MSG_Responder(gui_t * g, guievent_t * ev);

gui_t *GUI_BTStart(gui_t ** g, gui_t * parent, int id, int x, int y, char *string)
{
	gui_t *gui;
	button_t *bt;

	gui = Z_ClearNew(gui_t, 1);

	bt = (button_t*)Z_New(button_t, 1);
	bt->string = Z_StrDup(string);

	gui->process = bt;
	gui->Responder = &BT_Responder;
	gui->Drawer = &BT_Drawer;

	gui->id = id;
	bt->parent = parent;

	gui->left = x;
	gui->top = y;
	gui->right = x ; //!!! + HL_StringWidth(string);
	gui->bottom = y ; ///!!! + HL_StringHeight(string);

	GUI_Start(g, gui);
	return gui;
}

bool BT_Responder(gui_t * g, guievent_t * ev)
{
	button_t *bt = (button_t*)g->process;
	guievent_t click;

	// Proper Init under ANSI C++
	click.type  = gev_bnclick;
	click.data1 = g->id;

	switch (ev->type)
	{
		case gev_keydown:
			if (ev->data1 == KEYD_MOUSE1)
				bt->status = true;
			break;

		case gev_keyup:
			if (ev->data1 == KEYD_MOUSE1 && bt->status)
			{
				bt->status = false;
				return GUI_Responder(&bt->parent, &click);
			}
			bt->status = false;
			break;

		case gev_destroy:
			Z_Free(bt->string);
			Z_Free(bt);
			return false;

		default:
			break;
	}

	return false;
}

void BT_Drawer(gui_t * g)
{
	button_t *bt = (button_t*)g->process;

	GUI_WriteText(g->left, g->top, bt->string);
}

gui_t *GUI_MSGStart(gui_t ** g, gui_t * parent, int msg_id, int id, char *string)
{
	gui_t *gui;
	msgbox_t *msg;

	gui = Z_ClearNew(gui_t, 1);
	msg = (msgbox_t*)Z_New(msgbox_t, 1);

	gui->process = msg;
	gui->Responder = &MSG_Responder;
	gui->Drawer = &MSG_Drawer;

	msg->string = Z_StrDup(string);
	msg->parent = parent;
	msg->msg = msg_id;
	gui->id = id;

	gui->top = SCREENHEIGHT / 2 - 30;
	gui->bottom = SCREENHEIGHT / 2 + 30;
	gui->left = SCREENWIDTH / 4;
	gui->right = 3 * SCREENWIDTH / 4;

	GUI_Init(&msg->gui);
	GUI_BTStart(&msg->gui, gui, 0, SCREENWIDTH / 2 - 4, SCREENHEIGHT / 2 + 16, "OK");

	GUI_Start(g, gui);
	return gui;
}

bool MSG_Responder(gui_t * g, guievent_t * ev)
{
	int r;
	msgbox_t *msg = (msgbox_t*)g->process;
	guievent_t ok;

	switch (ev->type)
	{
		case gev_bnclick:
		{
			// Proper init under ANSI C++
			ok.type  = (guievtype_t)msg->msg;
			ok.data1 = g->id;

			r = GUI_Responder(&msg->parent, &ok);
			GUI_Destroy(g);
			return r?true:false;
		}

		case gev_destroy:
			GUI_Destroy(msg->gui);
			Z_Free(msg->string);
			Z_Free(msg->gui);
			Z_Free(msg);
			break;

		default:
			return GUI_Responder(&msg->gui, ev);
	}

	return false;
}

void MSG_Drawer(gui_t * g)
{
	msgbox_t *msg = (msgbox_t*)g->process;

	///!!! GUI_WriteText((g->left + g->right - HL_StringWidth(msg->string)) / 2, (g->top + 12), msg->string);
	GUI_Drawer(&msg->gui);
}

static bool DRAG_Responder(gui_t * g, guievent_t * ev)
{
	drag_t *drag = (drag_t *) g->process;
	guievent_t event;

	switch (ev->type)
	{
		case gev_hover:
			if (drag->mouse)
			{
				event = *ev;
				event.type = gev_drag;
				GUI_Responder(&drag->parent, &event);
				return true;
			}
			break;

		case gev_keydown:
			if (ev->data1 == KEYD_MOUSE1)
			{
				drag->mouse = true;
				return true;
			}
			return false;

		case gev_keyup:
			if (drag->mouse && ev->data1 == KEYD_MOUSE1)
				drag->mouse = false;
			return false;

		case gev_destroy:
			Z_Free(drag);
			return false;

		default:
			return false;
	}
	return false;
}

gui_t *GUI_DRAGStart(gui_t ** g, gui_t * parent, int id)
{
	gui_t *gui;
	drag_t *drag;

	gui = Z_ClearNew(gui_t, 1);
	drag = (drag_t*)Z_New(drag_t, 1);

	gui->process = drag;
	gui->Responder = &DRAG_Responder;

	drag->parent = parent;
	drag->mouse = 0;
	gui->id = id;

	GUI_Start(g, gui);
	return gui;
}

void GUI_WriteText(int x, int y, char *string)
{
	///!!! HL_WriteText(x, y, string);
}

typedef struct
{
	char *watch;
	gui_t *gui;
	int max;
}
bar_t;

static bool BAR_Responder(gui_t * g, guievent_t * ev)
{
	bar_t *bar = (bar_t*)g->process;

	switch (ev->type)
	{
		case gev_drag:
			ev->type = gev_move;
			GUI_Responder(&g, ev);
			return true;

		case gev_destroy:
			GUI_Destroy(bar->gui);
			Z_Free(bar->watch);
			Z_Free(bar);
			break;

		case gev_keydown:
			if (ev->data1 == KEYD_MOUSE2)
			{
				GUI_Destroy(g);
				return true;
			}

		default:
			return GUI_Responder(&bar->gui, ev);

		case gev_move:
		case gev_size:
			GUI_Responder(&bar->gui, ev);
			break;
	}

	return false;
}

//
// BAR_Drawer
//
// This does?
//
static void BAR_Drawer(gui_t * gui)
{
	bar_t *bar = (bar_t*)gui->process;
	const int *valp;
	unsigned int val;
	int r, g, b;

	if (!CON_GetCVar(bar->watch, (const void **)&valp))
		return;

	if (!valp)
		return;

	val = *valp;
	val = val * 100 / bar->max;

	r = val / 2;
	if (val >= 31)
		r = 31 - r;
	if (r < 0)
		r = 0;

	g = val / 3;
	if (g > 31)
		g = 31;

	b = 0;
}

gui_t *GUI_BARStart(gui_t ** g, char *watch, int max)
{
	gui_t *gui;
	bar_t *bar;
	guievent_t ev;

	gui = Z_ClearNew(gui_t, 1);
	gui->process = bar = Z_New(bar_t, 1);

	bar->watch = Z_StrDup(watch);
	bar->max = max;

	gui->Responder = &BAR_Responder;
	gui->Drawer = &BAR_Drawer;

	gui->id = 0;

	GUI_Init(&bar->gui);
	GUI_DRAGStart(&bar->gui, gui, 0);
	ev.type = gev_size;
	ev.data1 = 100;
	ev.data2 = 16;
	GUI_Responder(&bar->gui, &ev);

	gui->right = 100;
	gui->bottom = 16;

	GUI_Start(g, gui);
	return gui;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
