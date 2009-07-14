//------------------------------------------------------------------------
//  Information Bar (bottom of window)
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

#ifndef __UI_NOMBRE_H__
#define __UI_NOMBRE_H__


class UI_Nombre : public Fl_Box
{
private:
	int index;
	int total;

	const char *type_name;

	void Update();

public:
	UI_Nombre(int X, int Y, int W, int H, const char *what = NULL);
	virtual ~UI_Nombre();

public:
	void SetIndex(int _idx);  // _idx < 0 means "no index"
	void SetTotal(int _tot);
};


#endif // __UI_NOMBRE_H__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
