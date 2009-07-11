//------------------------------------------------------------------------
//  Flat/Texture List
//------------------------------------------------------------------------
//
//  RTS Layout Tool (C) 2007 Andrew Apted
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
#include "game.h"
#include "things.h"
#include "w_structs.h"
#include "r_images.h"


FlatTex_Choice::FlatTex_Choice(int X, int Y, int W, int H) :
	Fl_Box(FL_BORDER_BOX, X, Y, W, H, "")
{
	// TODO
}

FlatTex_Choice::~FlatTex_Choice()
{
	// TODO
}

void FlatTex_Choice::draw()
{
	// TODO
}


//------------------------------------------------------------------------


UI_FlatTexList::UI_FlatTexList(int X, int Y, int W, int H, const char *label) :
    Fl_Group(X, Y, W, H, label)
{
	end(); // cancel begin() in Fl_Group constructor

	box(FL_FLAT_BOX);

	color(FL_DARK3, FL_DARK3);

	// TODO
}

UI_FlatTexList::~UI_FlatTexList()
{
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
