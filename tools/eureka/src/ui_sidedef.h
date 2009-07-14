//------------------------------------------------------------------------
//  Information Panel
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

#ifndef __UI_SIDEDEF_H__
#define __UI_SIDEDEF_H__


class UI_SideBox : public Fl_Group
{
private:
	int  obj;
	bool is_front;

public:
	Fl_Int_Input *x_ofs;
	Fl_Int_Input *y_ofs;
	Fl_Int_Input *sec;

	UI_Pic   *l_pic;
	UI_Pic   *m_pic;
	UI_Pic   *u_pic;

	Fl_Input *l_tex;
	Fl_Input *m_tex;
	Fl_Input *u_tex;

public:
	UI_SideBox(int X, int Y, int W, int H, int _side);
	virtual ~UI_SideBox();

public:
	void SetObj(int index);

private:
	void UpdateLabel();
	void UpdateHiding(bool hide);

	void TexFromWidget(wad_tex_name_t& tname, Fl_Input *w);
	void TexToWidget(Fl_Input *w, const wad_tex_name_t& tname);

	static void    tex_callback(Fl_Widget *, void *);
	static void offset_callback(Fl_Widget *, void *);
	static void sector_callback(Fl_Widget *, void *);
};

#endif // __UI_SIDEDEF_H__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
