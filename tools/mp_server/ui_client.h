//------------------------------------------------------------------------
//  Client list widget
//------------------------------------------------------------------------
//
//  Edge MultiPlayer Server (C) 2005  Andrew Apted
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

#ifndef __UI_CLIENT_H__
#define __UI_CLIENT_H__

#include "lib_list.h"

class UI_ClientList : public Fl_Table_Row
{
public:
	UI_ClientList(int x, int y, int w, int h);
	virtual ~UI_ClientList();

protected:
	void draw_cell(TableContext context, int R=0, int C=0,
			int X=0, int Y=0, int W=0, int H=0);

	void resize(int X, int Y, int W, int H);

	int get_column_width(int col, int total_W) const;
	const char *get_column_name(int col) const;

public:
};


#endif /* __UI_CLIENT_H__ */
