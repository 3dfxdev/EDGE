//------------------------------------------------------------------------
//  Information Panel
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2009 Andrew Apted
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

#include "main.h"
#include "ui_window.h"

#include "levels.h"
#include "w_structs.h"


//
// UI_VertexBox Constructor
//
UI_VertexBox::UI_VertexBox(int X, int Y, int W, int H, const char *label) : 
    Fl_Group(X, Y, W, H, label),
    cur_idx(-1)
{
	end();  // cancel begin() in Fl_Group constructor

	box(FL_FLAT_BOX); // (FL_THIN_UP_BOX);


	X += 6;
	Y += 5;

	W -= 12;
	H -= 10;

	int MX = X + W/2;


	which = new UI_Nombre(X, Y, W-10, 28, "Vertex");

	add(which);

	Y += which->h() + 4;


	pos_x = new Fl_Int_Input(X +28, Y, 80, 24, "x: ");
	pos_y = new Fl_Int_Input(MX +28, Y, 80, 24, "y: ");

	pos_x->align(FL_ALIGN_LEFT);
	pos_y->align(FL_ALIGN_LEFT);

	pos_x->callback(pos_callback, this);
	pos_y->callback(pos_callback, this);

	add(pos_x); add(pos_y);

	Y += pos_y->h() + 4;



	// ---- resizable ----

#if 1
	Fl_Box *resize_control = new Fl_Box(FL_NO_BOX, x(), H-4, w(), 4, NULL);

	add(resize_control);
	resizable(resize_control);
#endif 

}

//
// UI_VertexBox Destructor
//
UI_VertexBox::~UI_VertexBox()
{
}


int UI_VertexBox::handle(int event)
{
	return Fl_Group::handle(event);
}


void UI_VertexBox::pos_callback(Fl_Widget *w, void *data)
{
	UI_VertexBox *box = (UI_VertexBox *)data;

	if (is_vertex(box->cur_idx))
	{
		Vertices[box->cur_idx].x = atoi(box->pos_x->value());
		Vertices[box->cur_idx].y = atoi(box->pos_y->value());

		main_win->canvas->redraw();
	}
}


//------------------------------------------------------------------------

void UI_VertexBox::SetObj(int index)
{
	if (index == cur_idx)
		return;

	cur_idx = index;

	which->SetIndex(index);
	which->SetTotal(NumVertices);

	if (is_vertex(cur_idx))
	{
		pos_x->value(Int_TmpStr(Vertices[cur_idx].x));
		pos_y->value(Int_TmpStr(Vertices[cur_idx].y));
	}
	else
	{
		pos_x->value("");
		pos_y->value("");
	}

	redraw();
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
