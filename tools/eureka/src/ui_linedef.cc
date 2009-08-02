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
#include "e_things.h"
#include "m_game.h"
#include "w_rawdef.h"
#include "w_structs.h"


//
// UI_LineBox Constructor
//
UI_LineBox::UI_LineBox(int X, int Y, int W, int H, const char *label) : 
    Fl_Group(X, Y, W, H, label),
    obj(-1)
{
	end();  // cancel begin() in Fl_Group constructor

	box(FL_FLAT_BOX); // (FL_THIN_UP_BOX);


	X += 6;
	Y += 5;

	W -= 12;
	H -= 10;


	which = new UI_Nombre(X, Y, W-10, 28, "Linedef");

	add(which);

	Y += which->h() + 4;


	type = new Fl_Int_Input(X+58, Y, 64, 24, "Type: ");
	type->align(FL_ALIGN_LEFT);

	add(type);

	choose = new Fl_Button(X+W/2+24, Y, 80, 24, "Choose");
	//!!!!  choose->callback(button_callback, this);

	add(choose);

	Y += type->h() + 2;


	desc = new Fl_Output(X+58, Y, W-66, 24, "Desc: ");
	desc->align(FL_ALIGN_LEFT);

	add(desc);

	Y += desc->h() + 2;


	tag = new Fl_Int_Input(X+W/2+52, Y, 64, 24, "Tag: ");
	tag->align(FL_ALIGN_LEFT);

	add(tag);


	length = new Fl_Output(X+58, Y, 64, 24, "Length: ");
	length->align(FL_ALIGN_LEFT);

	add(length);

	Y += tag->h() + 2;

	Y += 3;


	Fl_Box *flags = new Fl_Box(FL_FLAT_BOX, X, Y, 64, 24, "Flags: ");
	flags->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

	add(flags);


	f_automap = new Fl_Choice(X+W-104, Y, 104, 22);
	f_automap->add("Normal|Invisible|Mapped|Secret");
	f_automap->value(0);
	f_automap->callback(flags_callback, this);

	add(f_automap);


	Y += flags->h() - 1;



	f_upper = new Fl_Check_Button(X+12, Y+2, 20, 20, "upper unpeg");
	f_upper->align(FL_ALIGN_RIGHT);
	f_upper->labelsize(12);
	f_upper->callback(flags_callback, this);

	add(f_upper);


	f_walk = new Fl_Check_Button(X+W-120, Y+2, 20, 20, "block walk");
	f_walk->align(FL_ALIGN_RIGHT);
	f_walk->labelsize(12);
	f_walk->callback(flags_callback, this);

	add(f_walk);



	Y += 19;


	f_lower = new Fl_Check_Button(X+12, Y+2, 20, 20, "lower unpeg");
	f_lower->align(FL_ALIGN_RIGHT);
	f_lower->labelsize(12);
	f_lower->callback(flags_callback, this);

	add(f_lower);


	f_mons = new Fl_Check_Button(X+W-120, Y+2, 20, 20, "block mons");
	f_mons->align(FL_ALIGN_RIGHT);
	f_mons->labelsize(12);
	f_mons->callback(flags_callback, this);

	add(f_mons);


	Y += 19;


	f_passthru = new Fl_Check_Button(X+12, Y+2, 20, 20, "pass use");
	f_passthru->align(FL_ALIGN_RIGHT);
	f_passthru->labelsize(12);
	f_passthru->callback(flags_callback, this);

	add(f_passthru);


	f_sound = new Fl_Check_Button(X+W-120, Y+2, 20, 20, "block sound");
	f_sound->align(FL_ALIGN_RIGHT);
	f_sound->labelsize(12);
	f_sound->callback(flags_callback, this);

	add(f_sound);


	//  Fl_Choice *f_block = new Fl_Choice(X+16, Y, 104, 22);
	//  f_block->add("Walkable|Block Mons|Impassible");
	//  f_block->value(0);
	//
	//  add(f_block);

	Y += 29;


	front = new UI_SideBox(x(), Y, w(), 136, 0);

	add(front);

	Y += front->h();


	back = new UI_SideBox(x(), Y, w(), 130, 1);

	add(back);
}

//
// UI_LineBox Destructor
//
UI_LineBox::~UI_LineBox()
{
}


void UI_LineBox::flags_callback(Fl_Widget *w, void *data)
{
	UI_LineBox *box = (UI_LineBox *)data;

	if (! is_linedef(box->obj))
		return;

//@@@@@@	LineDefs[box->obj].flags = box->CalcFlags();
}



//------------------------------------------------------------------------


void UI_LineBox::SetObj(int index)
{
	if (obj == index)
		return;

	obj = index;

	which->SetIndex(index);
	which->SetTotal(NumLineDefs);

	if (is_linedef(obj))
	{
		front->SetObj(LineDefs[obj]->right);
		back->SetObj(LineDefs[obj]->left);

		type->value(Int_TmpStr(LineDefs[obj]->type));
		desc->value(GetLineDefTypeName(LineDefs[obj]->type));
		tag->value(Int_TmpStr(LineDefs[obj]->tag));

		CalcLength();

		FlagsFromInt(LineDefs[obj]->flags);
	}
	else
	{
		front->SetObj(-1);
		back->SetObj(-1);

		type->value("");
		desc->value("");
		tag->value("");

		length->value("");

		FlagsFromInt(0);
	}

	//  redraw();
}


void UI_LineBox::CalcLength()
{
	// ASSERT(obj >= 0);

	int n = obj;

	int dx = LineDefs[n]->Start()->x - LineDefs[n]->End()->x;
	int dy = LineDefs[n]->Start()->y - LineDefs[n]->End()->y;

	float len_f = sqrt(dx*dx + dy*dy);

	char buffer[300];

	if (int(len_f) >= 10000)
		sprintf(buffer, "%1.0f", len_f);
	else if (int(len_f) >= 100)
		sprintf(buffer, "%1.1f", len_f);
	else
		sprintf(buffer, "%1.2f", len_f);

	length->value(buffer);
}


void UI_LineBox::FlagsFromInt(int lineflags)
{
	if (lineflags & MLF_Secret)
		f_automap->value(3);
	else if (lineflags & MLF_DontDraw)
		f_automap->value(1);
	else if (lineflags & MLF_Mapped)
		f_automap->value(2);
	else
		f_automap->value(0);

	f_upper   ->value((lineflags & MLF_UpperUnpegged) ? 1 : 0);
	f_lower   ->value((lineflags & MLF_LowerUnpegged) ? 1 : 0);
	f_passthru->value((lineflags & MLF_PassThru)      ? 1 : 0);

	f_walk ->value((lineflags & MLF_Blocking)      ? 1 : 0);
	f_mons ->value((lineflags & MLF_BlockMonsters) ? 1 : 0);
	f_sound->value((lineflags & MLF_SoundBlock)    ? 1 : 0);
}


int UI_LineBox::CalcFlags() const
{
	// ASSERT(is_linedef(obj))

	int lineflags = 0;

	// FIXME: not sure if this belongs here
	if (LineDefs[obj]->left >= 0)
		lineflags |= MLF_TwoSided;

	switch (f_automap->value())
	{
		case 0: /* Normal    */; break;
		case 1: /* Invisible */ lineflags |= MLF_DontDraw; break;
		case 2: /* Mapped    */ lineflags |= MLF_Mapped; break;
		case 3: /* Secret    */ lineflags |= MLF_Secret; break;
	}

	if (f_upper->value())    lineflags |= MLF_UpperUnpegged;
	if (f_lower->value())    lineflags |= MLF_LowerUnpegged;
	if (f_passthru->value()) lineflags |= MLF_PassThru;

	if (f_walk->value())  lineflags |= MLF_Blocking;
	if (f_mons->value())  lineflags |= MLF_BlockMonsters;
	if (f_sound->value()) lineflags |= MLF_SoundBlock;

	return lineflags;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
