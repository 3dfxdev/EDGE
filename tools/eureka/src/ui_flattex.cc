//------------------------------------------------------------------------
//  Flat/Texture List
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
#include "ui_flattex.h"

#include "im_img.h"
#include "im_color.h"
#include "m_game.h"
#include "e_things.h"
#include "w_structs.h"
#include "r_images.h"
#include "levels.h"


FlatTex_Choice::FlatTex_Choice(int X, int Y, int W, int H) :
	Fl_Box(FL_DOWN_BOX, X, Y, W, H, "")
{
	align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
	labelfont(FL_COURIER);
	labelsize(KF_fonth);
	color(FL_DARK3, FL_DARK3);

	// TODO
}

FlatTex_Choice::~FlatTex_Choice()
{
	// TODO
}

void FlatTex_Choice::draw()
{
	Fl_Box::draw();
}


int FlatTex_Choice::CalcHeight() const
{
	return h();
}


class My_ClipGroup : public Fl_Group
{
	public:
		My_ClipGroup(int X, int Y, int W, int H, const char *L = NULL) :
			Fl_Group(X, Y, W, H, L)
	{
		clip_children(1);
	}

	virtual ~My_ClipGroup()
	{ }
};

//------------------------------------------------------------------------


UI_FlatTexList::UI_FlatTexList(int X, int Y, int W, int H, const char *label) :
    Fl_Group(X, Y, W, H, label)
{
	end(); // cancel begin() in Fl_Group constructor

	box(FL_FLAT_BOX);  // box(FL_DOWN_BOX);

	color(FL_DARK3, FL_DARK3); // FL_GRAY0+1, FL_GRAY0+1);


///---	Fl_Group *top = new Fl_Group(X, Y, W, 100);
///---	top->end();
///---	top->box(FL_DOWN_BOX);
///---	top->color(FL_GRAY0+1, FL_GRAY0+1);
///---
///---	add(top);

	title_lab = new Fl_Box(X, Y+10, W, 22+KF*4, "Texture List");
	title_lab->labelsize(20+KF*4);
	add(title_lab);

	mode = new Fl_Choice(X+50, Y+36, W-60, 24, "Mode:");
	mode->align(FL_ALIGN_LEFT);
	mode->color(FL_DARK3, FL_DARK3);
	mode->add("Images|Names");
	mode->value(0);
//	mode->callback(mode_callback, this);
	mode->labelsize(KF_fonth);
	mode->textsize(KF_fonth);

	add(mode);


	mx = X + Fl::scrollbar_size();
	my = Y + 100;
	mw = W-0 - Fl::scrollbar_size();
	mh = H - 100;

	offset_y = 0;
	total_h  = 0;


	sbar = new Fl_Scrollbar(X, my, Fl::scrollbar_size(), mh);
  	sbar->callback(callback_Scroll, this);
	sbar->color(FL_DARK2, FL_DARK2);
	sbar->selection_color(FL_GRAY0);

	add(sbar);


	pack = new My_ClipGroup(mx, my, mw, mh);
	pack->end();

	add(pack);


	Populate();

	PositionAll();
}

UI_FlatTexList::~UI_FlatTexList()
{
}


void UI_FlatTexList::Populate()
{
	int y = pack->y();

	int mx = pack->x();
	int mw = pack->w();

	for (int i = 0; i < NumWTexture; i++)
	{
		FlatTex_Choice *ftc = new FlatTex_Choice(mx, y, mw, 24);

		ftc->name = std::string(WTexture[i]);
		ftc->label(ftc->name.c_str());

		pack->add(ftc);

		y += ftc->h();
	}
}


void UI_FlatTexList::PositionAll(FlatTex_Choice *focus)
{
	// determine focus [closest to top without going past it]
	if (! focus)
	{
		int best_dist = 9999;

		for (int j = 0; j < pack->children(); j++)
		{
			FlatTex_Choice *M = (FlatTex_Choice *) pack->child(j);
			SYS_ASSERT(M);

			if (!M->visible() || M->y() < my || M->y() >= my+mh)
				continue;

			int dist = M->y() - my;

			if (dist < best_dist)
			{
				focus = M;
				best_dist = dist;
			}
		}
	}


	// calculate new total height
	int new_height = 0;
	int spacing = 0;

	for (int k = 0; k < pack->children(); k++)
	{
		FlatTex_Choice *M = (FlatTex_Choice *) pack->child(k);
		SYS_ASSERT(M);

		if (M->visible())
			new_height += M->CalcHeight() + spacing;
	}


	// determine new offset_y
	if (new_height <= mh)
	{
		offset_y = 0;
	}
	else if (focus)
	{
		int focus_oy = focus->y() - my;

		int above_h = 0;
		for (int k = 0; k < pack->children(); k++)
		{
			FlatTex_Choice *M = (FlatTex_Choice *) pack->child(k);

			if (M->visible() && M->y() < focus->y())
			{
				above_h += M->CalcHeight() + spacing;
			}
		}

		offset_y = above_h - focus_oy;

		offset_y = MAX(offset_y, 0);
		offset_y = MIN(offset_y, new_height - mh);
	}
	else
	{
		// when not shrinking, offset_y will remain valid
		if (new_height < total_h)
			offset_y = 0;
	}

	total_h = new_height;

	SYS_ASSERT(offset_y >= 0);
	SYS_ASSERT(offset_y <= total_h);


	// reposition all the choices
	int ny = my - offset_y;

	for (int j = 0; j < pack->children(); j++)
	{
		FlatTex_Choice *M = (FlatTex_Choice *) pack->child(j);
		SYS_ASSERT(M);

		int nh = M->visible() ? M->CalcHeight() : 1;

		if (ny != M->y() || nh != M->h())
		{
			M->resize(M->x(), ny, M->w(), nh);
		}

		if (M->visible())
			ny += M->CalcHeight() + spacing;
	}


	// p = position, first line displayed
	// w = window, number of lines displayed
	// t = top, number of first line
	// l = length, total number of lines
	sbar->value(offset_y, mh, 0, total_h);

	if (sbar->slider_size() < 0.06)
		sbar->slider_size(0.06);

	pack->redraw();
}


void UI_FlatTexList::callback_Scroll(Fl_Widget *w, void *data)
{
	Fl_Scrollbar *sbar = (Fl_Scrollbar *)w;
	UI_FlatTexList *that = main_win->tex_list;

	int previous_y = that->offset_y;

	that->offset_y = sbar->value();

	int dy = that->offset_y - previous_y;

	// simply reposition all the UI_Module widgets
	for (int j = 0; j < that->pack->children(); j++)
	{
		Fl_Widget *F = that->pack->child(j);
		SYS_ASSERT(F);

		F->resize(F->x(), F->y() - dy, F->w(), F->h());
	}

	that->redraw();
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
