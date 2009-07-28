//------------------------------------------------------------------------
//  Spawn-Thing Info
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
// UI_ThingBox Constructor
//
UI_ThingBox::UI_ThingBox(int X, int Y, int W, int H, const char *label) : 
    Fl_Group(X, Y, W, H, label),
    obj(-1),
    view_TH(NULL)
{
	end();  // cancel begin() in Fl_Group constructor

	box(FL_FLAT_BOX);

	X += 6;
	Y += 5;

	W -= 12;
	H -= 10;

	int MX = X + W/2;


	which = new UI_Nombre(X, Y, W-10, 28, "Thing");

	add(which);

	Y += which->h() + 4;


	type = new Fl_Int_Input(X+70, Y, 64, 24, "Type: ");
	type->align(FL_ALIGN_LEFT);
	type->callback(type_callback, this);

	add(type);

	choose = new Fl_Button(X+W/2+24, Y, 80, 24, "Choose");
	choose->callback(button_callback, this);

	add(choose);


	Y += type->h() + 3;

	desc = new Fl_Output(X+70, Y, W-76, 24, "Desc: ");
	desc->align(FL_ALIGN_LEFT);

	add(desc);

	Y += desc->h() + 3;


	angle = new Fl_Int_Input(X+70, Y, 64, 24, "Angle: ");
	angle->align(FL_ALIGN_LEFT);
	angle->callback(angle_callback, this);

	add(angle);

	ang_left  = new Fl_Button(X+W-100, Y+1, 30, 22, "@<");
	ang_right = new Fl_Button(X+W- 60, Y+1, 30, 22, "@>");

	//  ang_left ->labelfont(FL_HELVETICA_BOLD);
	//  ang_right->labelfont(FL_HELVETICA_BOLD);
	//  ang_left ->labelsize(16);
	//  ang_right->labelsize(16);

	ang_left ->callback(button_callback, this);
	ang_right->callback(button_callback, this);

	add(ang_left); add(ang_right);

	Y += angle->h() + 3;


	exfloor = new Fl_Int_Input(X+70, Y, 64, 24, "ExFloor: ");
	exfloor->align(FL_ALIGN_LEFT);
	exfloor->callback(option_callback, this);

	add(exfloor);

	efl_down = new Fl_Button(X+W-100, Y+1, 30, 22, "-");
	efl_up   = new Fl_Button(X+W- 60, Y+1, 30, 22, "+");

	efl_down->labelfont(FL_HELVETICA_BOLD);
	efl_up  ->labelfont(FL_HELVETICA_BOLD);
	efl_down->labelsize(16);
	efl_up  ->labelsize(16);

	efl_down->callback(button_callback, this);
	efl_up  ->callback(button_callback, this);

	add(efl_down); add(efl_up);

	Y += exfloor->h() + 10;


	pos_x = new Fl_Int_Input(X +28, Y, 80, 24, "x: ");
	pos_y = new Fl_Int_Input(MX+28, Y, 80, 24, "y: ");

	pos_x->align(FL_ALIGN_LEFT);
	pos_y->align(FL_ALIGN_LEFT);

	pos_x->callback(pos_callback, this);
	pos_y->callback(pos_callback, this);

	add(pos_x); add(pos_y);

	Y += pos_x->h() + 4;


	Fl_Box *opt_lab = new Fl_Box(X, Y, W, 22, "Options:");
	opt_lab->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

	add(opt_lab);

	Y += opt_lab->h() + 2;


	// when appear: two rows of three on/off buttons
	int AX = X+W/3+4;
	int BX = X+2*W/3-16;
	int FW = W/3 - 12;

	int AY = Y+22;
	int BY = Y+22*2;

	o_easy   = new Fl_Check_Button( X+12,  Y, FW, 22, "easy");
	o_medium = new Fl_Check_Button( X+12, AY, FW, 22, "medium");
	o_hard   = new Fl_Check_Button( X+12, BY, FW, 22, "hard");

	o_sp     = new Fl_Check_Button(AX+12,  Y, FW, 22, "sp");
	o_coop   = new Fl_Check_Button(AX+12, AY, FW, 22, "coop");
	o_dm     = new Fl_Check_Button(AX+12, BY, FW, 22, "dm");

	o_easy  ->value(1);  o_sp  ->value(1);
	o_medium->value(1);  o_coop->value(1);
	o_hard  ->value(1);  o_dm  ->value(1);

#if 0
	o_easy  ->labelsize(12);  o_sp  ->labelsize(12);
	o_medium->labelsize(12);  o_coop->labelsize(12);
	o_hard  ->labelsize(12);  o_dm  ->labelsize(12);
#endif

	o_easy  ->callback(option_callback, this);
	o_medium->callback(option_callback, this);
	o_hard  ->callback(option_callback, this);

	o_sp    ->callback(option_callback, this);
	o_coop  ->callback(option_callback, this);
	o_dm    ->callback(option_callback, this);

	add(o_easy); add(o_medium); add(o_hard);
	add(o_sp);   add(o_coop);   add(o_dm);


	o_ambush = new Fl_Check_Button(BX+12,  Y, FW, 22, "ambush");
	o_friend = new Fl_Check_Button(BX+12, AY, FW, 22, "friend");

	o_ambush->callback(option_callback, this);
	o_friend->callback(option_callback, this);

	add(o_ambush); add(o_friend);

	Y = BY + 40;


	sprite = new UI_Pic(X + (W-100)/2, Y, 100,100);
	sprite->callback(button_callback, this);

	add(sprite);
}

//
// UI_ThingBox Destructor
//
UI_ThingBox::~UI_ThingBox()
{
}


void UI_ThingBox::SetObj(int index)
{
	if (index == obj)
		return;

	obj = index;

	which->SetIndex(index);
	which->SetTotal(NumVertices);

	if (is_thing(obj))
	{
		type ->value(Int_TmpStr(Things[obj]->type));
		desc ->value(get_thing_name(Things[obj]->type));
		sprite->GetSprite(Things[obj]->type);

		angle->value(Int_TmpStr(Things[obj]->angle));
		pos_x->value(Int_TmpStr(Things[obj]->x));
		pos_y->value(Int_TmpStr(Things[obj]->y));

		OptionsFromInt(Things[obj]->options);
	}
	else
	{
		type ->value("");
		desc ->value("");
		sprite->Nil();

		angle->value("");
		pos_x->value("");
		pos_y->value("");

		OptionsFromInt(0);
	}

	redraw();
}


void UI_ThingBox::SetViewThing(thing_spawn_c *th)
{
#if 0  // TODO
	if (view_TH == th)
		return;

	view_TH = th;

	if (view_TH)
	{
		SYS_ASSERT(th->parent);
		main_win->panel->SetWhich(th->th_Index + 1, th->parent->th_Total);

		LoadData(view_TH);
		activate();
	}
	else
	{
		main_win->panel->SetWhich(-1, -1);

		deactivate();
	}

	redraw();
#endif
}


void UI_ThingBox::type_callback(Fl_Widget *w, void *data)
{
	UI_ThingBox *box = (UI_ThingBox *)data;

	if (! is_thing(box->obj))
		return;

	int new_type = atoi(box->type->value());

//@@@@@@	Things[box->obj].type = new_type;

	box->desc->value(get_thing_name(new_type));
	box->sprite->GetSprite(new_type);

	main_win->canvas->redraw();
}


void UI_ThingBox::angle_callback(Fl_Widget *w, void *data)
{
	UI_ThingBox *box = (UI_ThingBox *)data;

	if (! is_thing(box->obj))
		return;

//@@@@@@	Things[box->obj].angle = atoi(box->angle->value());

	main_win->canvas->redraw();
}


void UI_ThingBox::pos_callback(Fl_Widget *w, void *data)
{
	UI_ThingBox *box = (UI_ThingBox *)data;

	if (! is_thing(box->obj))
		return;

//@@@@@@	Things[box->obj].x = atoi(box->pos_x->value());
//@@@@@@	Things[box->obj].y = atoi(box->pos_y->value());

	main_win->canvas->redraw();
}


void UI_ThingBox::option_callback(Fl_Widget *w, void *data)
{
	UI_ThingBox *box = (UI_ThingBox *)data;

	if (! is_thing(box->obj))
		return;

//@@@@@@	Things[box->obj].options = box->CalcOptions();
}


void UI_ThingBox::button_callback(Fl_Widget *w, void *data)
{
	UI_ThingBox *box = (UI_ThingBox *)data;

	int n = box->obj;
	if (! is_thing(n))
		return;

//@@@@	if (w == box->ang_left)
//@@@@		box->Rotate(+1);
//@@@@
//@@@@	if (w == box->ang_right)
//@@@@		box->Rotate(-1);
//@@@@
//@@@@	if (w == box->efl_down)
//@@@@		box->AdjustExtraFloor(-1);
//@@@@
//@@@@	if (w == box->efl_up)
//@@@@		box->AdjustExtraFloor(+1);

	if (w == box->choose || w == box->sprite)
	{
		// FIXME: thing selection dialog
	}
}


void UI_ThingBox::Rotate(int dir)
{
	if (! is_thing(obj))
		return;

	int old_ang = (Things[obj]->angle / 45) & 7;
	int new_ang = ((old_ang + dir) & 7) * 45;

//@@@@	Things[obj].angle = new_ang;

	angle->value(Int_TmpStr(new_ang));

	main_win->canvas->redraw();
}


void UI_ThingBox::AdjustExtraFloor(int dir)
{
	if (! is_thing(obj))
		return;

	int old_fl = (Things[obj]->options & MTF_EXFLOOR_MASK) >> MTF_EXFLOOR_SHIFT;
	int new_fl = ((old_fl + dir) << MTF_EXFLOOR_SHIFT) & MTF_EXFLOOR_MASK;

//@@@@	Things[obj].options &= ~MTF_EXFLOOR_MASK;
//@@@@	Things[obj].options |= new_fl;

	if (new_fl)
		exfloor->value(Int_TmpStr(new_fl >> MTF_EXFLOOR_SHIFT));
	else
		exfloor->value("");
}


void UI_ThingBox::OptionsFromInt(int options)
{
	o_easy  ->value((options & MTF_Easy)   ? 1 : 0);
	o_medium->value((options & MTF_Medium) ? 1 : 0);
	o_hard  ->value((options & MTF_Hard)   ? 1 : 0);

	o_sp  ->value((options & MTF_Not_SP)   ? 0 : 1);
	o_coop->value((options & MTF_Not_COOP) ? 0 : 1);
	o_dm  ->value((options & MTF_Not_DM)   ? 0 : 1);

	o_ambush->value((options & MTF_Ambush) ? 1 : 0);
	o_friend->value((options & MTF_Friend) ? 1 : 0);

	if (options & MTF_EXFLOOR_MASK)
		exfloor->value(Int_TmpStr((options & MTF_EXFLOOR_MASK) >> MTF_EXFLOOR_SHIFT));
	else
		exfloor->value("");
}


int UI_ThingBox::CalcOptions() const
{
	int options = 0;

	if (o_easy  ->value()) options |= MTF_Easy;
	if (o_medium->value()) options |= MTF_Medium;
	if (o_hard  ->value()) options |= MTF_Hard;

	if (0 == o_sp  ->value()) options |= MTF_Not_SP;
	if (0 == o_coop->value()) options |= MTF_Not_COOP;
	if (0 == o_dm  ->value()) options |= MTF_Not_DM;

	if (o_ambush->value()) options |= MTF_Ambush;
	if (o_friend->value()) options |= MTF_Friend;

	int exfl_num = atoi(exfloor->value());

	if (exfl_num > 0)
	{
		options |= (exfl_num << MTF_EXFLOOR_SHIFT) & MTF_EXFLOOR_MASK;
	}

	return options;
}



//------------------------------------------------------------------------

void UI_ListenThing(thing_spawn_c *th, int F)
{
#if 0  // TODO
	main_win->panel->thing_box->ListenField(th, F);
#endif
}

void UI_ThingBox::ListenField(thing_spawn_c *th, int F)
{
#if 0  // TODO
	if (th != view_TH)
		return;

	switch (F)
	{
		case thing_spawn_c::F_AMBUSH:
			update_Ambush(th);
			break;

		case thing_spawn_c::F_TYPE:
			update_Type(th);
			break;

		case thing_spawn_c::F_X:
		case thing_spawn_c::F_Y:
		case thing_spawn_c::F_Z:
			update_Pos(th);
			break;

		case thing_spawn_c::F_ANGLE:
			update_Angle(th);
			break;

		case thing_spawn_c::F_TAG:
			update_Tag(th);
			break;

		case thing_spawn_c::F_WHEN_APPEAR:
			update_WhenAppear(th);
			break;

		default: break;
	}

	main_win->grid->redraw();
#endif
}

void UI_ThingBox::LoadData(thing_spawn_c *th)
{
#if 0  // TODO
	update_Ambush(th);
	update_Type(th);
	update_Pos(th);
	update_Angle(th);
	update_Tag(th);
	update_WhenAppear(th);
#endif
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
