//------------------------------------------------------------------------
//  Main Window
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor (C) 2008-2009 Andrew Apted
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

#ifndef __UI_CANVAS_H__
#define __UI_CANVAS_H__

#include "edit2.h"
#include "grid2.h"


class UI_Canvas : public Fl_Widget
{
private:
	Editor_State_c *e;

	bool render3d;

public:
	UI_Canvas(int X, int Y, int W, int H, const char *label = NULL);
	virtual ~UI_Canvas();

public:
	// FLTK virtual method for handling input events.
	int handle(int event);

	// FLTK virtual method for resizing.
	void resize(int X, int Y, int W, int H);

	void set_edit_info(Editor_State_c *new_e) { e = new_e; redraw(); }

	void DrawMap();

	void DrawMapPoint (int mapx, int mapy);
	void DrawMapLine (int mapx1, int mapy1, int mapx2, int mapy2);
	void DrawMapVector (int mapx1, int mapy1, int mapx2, int mapy2);
	void DrawMapArrow (int mapx1, int mapy1, unsigned angle);

	void HighlightObject (int objtype, int objnum, Fl_Color colour);
	void HighlightSelection (int objtype, SelPtr list);

private:
	// FLTK virtual method for drawing.
	void draw();

	void DrawGrid();
	void DrawVertices();
	void DrawLinedefs();
	void DrawThings();
	void DrawRTS();
	void DrawObjNum(int x, int y, int obj_no, Fl_Color c);

	// convert screen coordinates to map coordinates
	inline int MAPX(int sx) { return (grid.orig_x + I_ROUND((sx - w()/2 - x()) / grid.Scale)); }
	inline int MAPY(int sy) { return (grid.orig_y + I_ROUND((h()/2 - sy + y()) / grid.Scale)); }

	// convert map coordinates to screen coordinates
	inline int SCREENX(int mx) { return (x() + w()/2 + I_ROUND((mx - grid.orig_x) * grid.Scale)); }
	inline int SCREENY(int my) { return (y() + h()/2 + I_ROUND((grid.orig_y - my) * grid.Scale)); }
};


#endif /* __UI_CANVAS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
