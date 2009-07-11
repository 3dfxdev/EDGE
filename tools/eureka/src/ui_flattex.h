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

#ifndef __UI_FLATTEX_H__
#define __UI_FLATTEX_H__

#include <string>


class FlatTex_Choice : public Fl_Box
{
private:
	bool filtered;  // won't be shown
	bool pic_mode;  // show as picture

	std::string name;
	std::string size;

	int type_id;
	int category;

	UI_Pic *pic;

	int pic_size;
	int name_w;

public:
	FlatTex_Choice(int X, int Y, int W, int H);
	virtual ~FlatTex_Choice();

public:


private:
	// FLTK method for drawing this widget
	void draw();

};


class UI_FlatTexList : public Fl_Group
{
private:
	Fl_Box *title_lab;

	Fl_Choice *mode;  // Names vs Images
	Fl_Choice *category;

	Fl_Group *pack;
	Fl_Scrollbar *sbar;

public:
	UI_FlatTexList(int X, int Y, int W, int H, const char *label = NULL);
	virtual ~UI_FlatTexList();

public:
	void SetCategories(const char *cats);


private:

};

#endif /* __UI_FLATTEX_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
