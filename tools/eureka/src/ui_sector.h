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

#ifndef __UI_SECTOR_H__
#define __UI_SECTOR_H__


class UI_SectorBox : public Fl_Group
{
private:
	int obj;

public:
	UI_Nombre *which;

	Fl_Int_Input *ceil_h;
	Fl_Int_Input *headroom;
	Fl_Int_Input *floor_h;

	Fl_Button *ce_down, *ce_up;
	Fl_Button *fl_down, *fl_up;

	Fl_Input *c_tex;
	UI_Pic   *c_pic;

	Fl_Input *f_tex;
	UI_Pic   *f_pic;

	Fl_Int_Input *type;
	Fl_Output    *desc;
	Fl_Button    *choose;

	Fl_Int_Input *light;
	Fl_Int_Input *tag;

	Fl_Button *lt_down, *lt_up;

public:
	UI_SectorBox(int X, int Y, int W, int H, const char *label = NULL);
	virtual ~UI_SectorBox();

public:
	// a negative value will show 'None Selected'.
	void SetObj(int index);

private:
	void AdjustHeight(s16_t *h, int delta);
	void AdjustLight (s16_t *L, int delta);

	void FlatFromWidget(wad_flat_name_t& fname, Fl_Input *w);
	void FlatToWidget(Fl_Input *w, const wad_flat_name_t& fname);

	static void height_callback(Fl_Widget *, void *);
	static void    tex_callback(Fl_Widget *, void *);
	static void   type_callback(Fl_Widget *, void *);
	static void  light_callback(Fl_Widget *, void *);
	static void    tag_callback(Fl_Widget *, void *);
	static void button_callback(Fl_Widget *, void *);
};

#endif // __UI_SECTOR_H__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
