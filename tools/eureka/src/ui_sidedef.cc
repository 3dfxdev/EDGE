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
#include "w_rawdef.h"
#include "w_structs.h"


//
// Constructor
//
UI_SideBox::UI_SideBox(int X, int Y, int W, int H, int _side) : 
    Fl_Group(X, Y, W, H),
    obj(-1), is_front(_side == 0)
{
	end();  // cancel begin() in Fl_Group constructor

	box(FL_FLAT_BOX); // FL_UP_BOX

	align(FL_ALIGN_INSIDE | FL_ALIGN_TOP | FL_ALIGN_LEFT);

	if (is_front)
		labelcolor(FL_BLUE);
	else
		labelcolor(fl_rgb_color(224,64,0));


	X += 6;
	Y += 6 + 12;  // space for label

	W -= 12;
	H -= 12;

	int MX = X + W/2;


	x_ofs = new Fl_Int_Input(X+20,   Y, 52, 24, "x:");
	y_ofs = new Fl_Int_Input(MX-20,  Y, 52, 24, "y:");
	sec = new Fl_Int_Input(X+W-54, Y, 52, 24, "sec:");

	x_ofs->align(FL_ALIGN_LEFT);
	y_ofs->align(FL_ALIGN_LEFT);
	sec  ->align(FL_ALIGN_LEFT);

	x_ofs->callback(offset_callback, this);
	y_ofs->callback(offset_callback, this);
	sec  ->callback(sector_callback, this);

	add(x_ofs); add(y_ofs); add(sec);


	Y += x_ofs->h() + 4;


	l_pic = new UI_Pic(X+4,      Y, 64, 64);
	m_pic = new UI_Pic(MX-32,    Y, 64, 64);
	u_pic = new UI_Pic(X+W-64-4, Y, 64, 64);

	l_pic->callback(tex_callback, this);
	m_pic->callback(tex_callback, this);
	u_pic->callback(tex_callback, this);

	Y += 65;


	l_tex = new Fl_Input(X,      Y, 80, 20);
	m_tex = new Fl_Input(MX-40,  Y, 80, 20);
	u_tex = new Fl_Input(X+W-80, Y, 80, 20);

	l_tex->textsize(12);
	m_tex->textsize(12);
	u_tex->textsize(12);

	l_tex->callback(tex_callback, this);
	m_tex->callback(tex_callback, this);
	u_tex->callback(tex_callback, this);


	add(l_pic); add(m_pic); add(u_pic);
	add(l_tex); add(m_tex); add(u_tex);


	Y += 24;


	UpdateHiding(true);
	UpdateLabel();
}

//
// Destructor
//
UI_SideBox::~UI_SideBox()
{
}


void UI_SideBox::tex_callback(Fl_Widget *w, void *data)
{
	UI_SideBox *box = (UI_SideBox *)data;

	int n = box->obj;
	if (! is_sidedef(n))
		return;


	if (w == box->l_pic)
	{
		// TODO: texture selection
	}
	if (w == box->m_pic)
	{
		// TODO: texture selection
	}
	if (w == box->u_pic)
	{
		// TODO: texture selection
	}


	if (w == box->l_tex)
	{
//@@@@		box->TexFromWidget (SideDefs[n].lower_tex, box->l_tex);
//@@@@		box->l_pic->GetTex(SideDefs[n].lower_tex);
	}

	if (w == box->m_tex)
	{
//@@@@		box->TexFromWidget (SideDefs[n].mid_tex, box->m_tex);
//@@@@		box->m_pic->GetTex(SideDefs[n].mid_tex);
	}

	if (w == box->u_tex)
	{
//@@@@		box->TexFromWidget (SideDefs[n].upper_tex, box->u_tex);
//@@@@		box->u_pic->GetTex(SideDefs[n].upper_tex);
	}
}


void UI_SideBox::offset_callback(Fl_Widget *w, void *data)
{
	UI_SideBox *box = (UI_SideBox *)data;

	if (! is_sidedef(box->obj))
		return;

//@@@@@@	SideDefs[box->obj]->x_offset = atoi(box->x_ofs->value());
//@@@@@@	SideDefs[box->obj]->y_offset = atoi(box->y_ofs->value());
}


void UI_SideBox::sector_callback(Fl_Widget *w, void *data)
{
	UI_SideBox *box = (UI_SideBox *)data;

	if (! is_sidedef(box->obj))
		return;

	int new_sec = atoi(box->sec->value());

//@@@@@@	if (new_sec >= 0)
//@@@@@@		SideDefs[box->obj].sector = new_sec;
}


//------------------------------------------------------------------------


void UI_SideBox::SetObj(int index)
{
	if (obj == index)
		return;

	if (obj < 0)
		UpdateHiding(false);
	else if (index < 0)
		UpdateHiding(true);

	obj = index;

	if (is_sidedef(obj))
	{
		x_ofs->value(Int_TmpStr(SideDefs[obj]->x_offset));
		y_ofs->value(Int_TmpStr(SideDefs[obj]->y_offset));
		sec->value(Int_TmpStr(SideDefs[obj]->sector));

		l_tex->value(SideDefs[obj]->LowerTex());
		m_tex->value(SideDefs[obj]->MidTex());
		u_tex->value(SideDefs[obj]->UpperTex());

		l_pic->GetTex(SideDefs[obj]->LowerTex());
		m_pic->GetTex(SideDefs[obj]->MidTex());
		u_pic->GetTex(SideDefs[obj]->UpperTex());
	}
	else
	{
		x_ofs->value("");
		y_ofs->value("");
		sec->value("");

		l_tex->value("");
		m_tex->value("");
		u_tex->value("");

		l_pic->Nil();
		m_pic->Nil();
		u_pic->Nil();
	}

	UpdateLabel();

	redraw();
}


void UI_SideBox::UpdateLabel()
{
	if (! is_sidedef(obj))
	{
		label(is_front ? " No Front Sidedef" : " No Back Sidedef");
		return;
	}

	char buffer[200];

	sprintf(buffer, " %s Sidedef: #%d\n",
			is_front ? "Front" : "Back", obj);

	copy_label(buffer);
}


void UI_SideBox::UpdateHiding(bool hide)
{
	if (hide)
	{
		x_ofs->hide();
		y_ofs->hide();
		sec->hide();

		l_tex->hide();
		m_tex->hide();
		u_tex->hide();

		l_pic->hide();
		m_pic->hide();
		u_pic->hide();
	}
	else
	{
		x_ofs->show();
		y_ofs->show();
		sec->show();

		l_tex->show();
		m_tex->show();
		u_tex->show();

		l_pic->show();
		m_pic->show();
		u_pic->show();
	}
}


void UI_SideBox::TexFromWidget(wad_tex_name_t& tname, Fl_Input *w)
{
	memset(tname, 0, WAD_TEX_NAME);

	strncpy(tname, w->value(), WAD_TEX_NAME);

	for (int i = 0; i < WAD_TEX_NAME; i++)
		tname[i] = toupper(tname[i]);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
