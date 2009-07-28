//------------------------------------------------------------------------
//  Main Window
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2006-2008 Andrew Apted
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
#include <math.h>

#include "ui_window.h"

#include "editloop.h"
#include "e_sector.h"
#include "e_things.h"
#include "r_misc.h"
#include "r_grid.h"
#include "im_color.h"
#include "levels.h"
#include "r_render.h"
#include "selectn.h"


extern int active_when;
extern int active_wmask;


//
// UI_Canvas Constructor
//
UI_Canvas::UI_Canvas(int X, int Y, int W, int H, const char *label) : 
    Fl_Widget(X, Y, W, H, label),
    e(NULL), render3d(false),
	highlight(), selbox_active(false)
{ }

//
// UI_Canvas Destructor
//
UI_Canvas::~UI_Canvas()
{ }


void UI_Canvas::resize(int X, int Y, int W, int H)
{
	Fl_Widget::resize(X, Y, W, H);
}


void UI_Canvas::draw()
{
	fl_push_clip(x(), y(), w(), h());

	fl_color(FL_WHITE);
	fl_font(FL_COURIER, 14);

	if (render3d)
		Render3D_Draw(x(), y());
	else if (e)
		DrawEverything();

	fl_pop_clip();
}


int UI_Canvas::handle(int event)
{
	//// fprintf(stderr, "HANDLE EVENT %d\n", event);

	switch (event)
	{
		case FL_FOCUS:
			return 1;

		case FL_KEYDOWN:
		case FL_SHORTCUT:
		{
			//      int result = handle_key();
			//      handle_mouse();

			int key   = Fl::event_key();
			int state = Fl::event_state();

			switch (key)
			{
				case FL_Num_Lock:
				case FL_Caps_Lock:

				case FL_Shift_L: case FL_Control_L:
				case FL_Shift_R: case FL_Control_R:
				case FL_Meta_L:  case FL_Alt_L:
				case FL_Meta_R:  case FL_Alt_R:

					/* IGNORE */
					return 1;

				default:
					/* OK */
					break;
			}

			if (key == FL_Tab || key == '\t')
			{
				render3d = !render3d;
				redraw();
				return 1;
			}

			if (key != 0)
			{
				if (key < 127 && isalpha(key) && (state & FL_SHIFT))
					key = toupper(key);

				if (render3d)
				{
					if (Render3D_Key(key))
						redraw();
				}
				else
					EditorKey(key, (state & FL_SHIFT) ? true : false);
			}

			return 1; // result;
		}

		case FL_ENTER:
			// we greedily grab the focus
			if (Fl::focus() != this)
				take_focus(); 

			return 1;

		case FL_LEAVE:
			//      hilite_rad = NULL;
			//      hilite_thing = NULL;
			//      dragging = false;
			//      determine_cursor();
			//      update_active_obj();
			redraw();
			return 1;

		case FL_MOVE:
		case FL_DRAG:
			EditorMouseMotion(Fl::event_x(), Fl::event_y(),
					MAPX(Fl::event_x()), MAPY(Fl::event_y()), event == FL_DRAG);
			return 1;

		case FL_PUSH:
			{
				int state = Fl::event_state();
				EditorMousePress((state & FL_CTRL) ? true : false);
			}
			return 1;

		case FL_RELEASE:
			EditorMouseRelease();
			return 1;

		case FL_MOUSEWHEEL:
			EditorWheel(0 - Fl::event_dy());
			return 1;

		default:
			break;
	}

	return 0;  // unused
}


//------------------------------------------------------------------------


void UI_Canvas::DrawEverything()
{
	DrawMap(); 

	HighlightSelection(edit.Selected); // FIXME should be widgetized

	if (highlight())
		HighlightObject(highlight.type, highlight.num, YELLOW);

	if (selbox_active)
		SelboxDraw();
}


/*
  draw the actual game map
*/
void UI_Canvas::DrawMap()
{
	int mapx0 = MAPX(x());   // FIXME: cache values
	int mapx9 = MAPX(x()+w());
	int mapy0 = MAPY(y()+h());
	int mapy9 = MAPY(y());
	int n;


	fl_color(FL_BLACK);
	fl_rectf(x(), y(), w(), h());


	// draw the grid first since it's in the background
	if (grid.shown)
		DrawGrid();

	if (e->obj_type != OBJ_THINGS)
		DrawThings();

	DrawLinedefs();

	if (e->obj_type == OBJ_VERTICES)
		DrawVertices();

	if (e->obj_type == OBJ_THINGS)
		DrawThings();

	if (e->obj_type == OBJ_RSCRIPT)
		DrawRTS();


	// Draw the things numbers
	if (e->obj_type == OBJ_THINGS && e->show_object_numbers)
	{
		for (n = 0; n < NumThings; n++)
		{
			int mapx = Things[n].x;
			int mapy = Things[n].y;

			if (mapx >= mapx0 && mapx <= mapx9 && mapy >= mapy0 && mapy <= mapy9)
				DrawObjNum(SCREENX (mapx) + FONTW, SCREENY (mapy) + 2, n, THING_NO);
		}
	}

	// Draw the sector numbers
	if (e->obj_type == OBJ_SECTORS && e->show_object_numbers)
	{
		int xoffset = - FONTW / 2;

		for (n = 0; n < NumSectors; n++)
		{
			int mapx;
			int mapy;

			centre_of_sector (n, &mapx, &mapy);

			if (mapx >= mapx0 && mapx <= mapx9 && mapy >= mapy0 && mapy <= mapy9)
				DrawObjNum(SCREENX (mapx) + xoffset, SCREENY (mapy) - FONTH / 2, n, SECTOR_NO);

			if (n == 10 || n == 100 || n == 1000 || n == 10000)
				xoffset -= FONTW / 2;
		}
	}
}



/*
 *  draw_grid - draw the grid in the background of the edit window
 */
void UI_Canvas::DrawGrid()
{
	int mapx0 = MAPX(x());   // FIXME: cache values
	int mapx9 = MAPX(x()+w());
	int mapy0 = MAPY(y()+h());
	int mapy9 = MAPY(y());

	int grid_step_1 = grid.step; // Map units between dots
	int grid_step_2 = 8 * grid_step_1;  // Map units between dim lines
	int grid_step_3 = 8 * grid_step_2;  // Map units between bright lines

	float pixels_1 = grid.step * grid.Scale;
	float pixels_2 = 8 * pixels_1;


	if (pixels_1 < 1.99)
	{
		fl_color(GRID_DARK);
		fl_rectf(x(), y(), w(), h());
		return;
	}


	fl_color (GRID_BRIGHT);
	{
		int mapx0_3 = (mapx0 / grid_step_3) * grid_step_3;
		if (mapx0_3 < mapx0)
			mapx0_3 += grid_step_3;
		for (int i = mapx0_3; i <= mapx9; i += grid_step_3)
			DrawMapLine (i, mapy0, i, mapy9);
	}

	{
		int mapy0_3 = (mapy0 / grid_step_3) * grid_step_3;
		if (mapy0_3 < mapy0)
			mapy0_3 += grid_step_3;
		for (int j = mapy0_3; j <=  mapy9; j += grid_step_3)
			DrawMapLine (mapx0, j, mapx9, j);
	}


	fl_color (GRID_MEDIUM);

	{
		int mapx0_2 = (mapx0 / grid_step_2) * grid_step_2;
		if (mapx0_2 < mapx0)
			mapx0_2 += grid_step_2;
		for (int i = mapx0_2; i <= mapx9; i += grid_step_2)
			if (i % grid_step_3 != 0)
				DrawMapLine (i, mapy0, i, mapy9);
	}

	{
		int mapy0_2 = (mapy0 / grid_step_2) * grid_step_2;
		if (mapy0_2 < mapy0)
			mapy0_2 += grid_step_2;
		for (int j = mapy0_2; j <=  mapy9; j += grid_step_2)
			if (j % grid_step_3 != 0)
				DrawMapLine (mapx0, j, mapx9, j);
	}


	if (pixels_1 < 3.99)
		fl_color(GRID_MEDIUM);
	//??  else if (pixels_1 > 30.88)
	//??    fl_color(GRID_BRIGHT);
	else
		fl_color(GRID_POINT);

	{
		int mapx0_1 = (mapx0 / grid_step_1) * grid_step_1;
		if (mapx0_1 < mapx0)
			mapx0_1 += grid_step_1;
		int mapy0_1 = (mapy0 / grid_step_1) * grid_step_1;
		if (mapy0_1 < mapy0)
			mapy0_1 += grid_step_1;

		int npoints = (mapx9 - mapx0_1) / grid_step_1 + 1;
		int dispx[npoints];

		for (int n = 0; n < npoints; n++)
			dispx[n] = SCREENX (mapx0_1 + n * grid_step_1);

		for (int j = mapy0_1; j <= mapy9; j += grid_step_1)
		{
			int dispy = SCREENY (j);
			for (int n = 0; n < npoints; n++)
			{
				if (pixels_1 < 30.99)
					fl_point(dispx[n], dispy);
				else
				{
					fl_line(dispx[n]-0, dispy, dispx[n]+1, dispy);
					fl_line(dispx[n], dispy-0, dispx[n], dispy+1);
				}
			}
		}
	}
}


/*
 *  vertex_radius - apparent radius of a vertex, in pixels
 *
 *  Try this in Gnuplot :
 *
 *    plot [0:10] x                                          
 *    replot log(x+1.46)/log(1.5)-log(2.46)/log(1.5)+1
 */
int vertex_radius (double scale)
{
	const int VERTEX_PIXELS = 6;

	return (int) (VERTEX_PIXELS * (0.2 + scale / 2));
}



/*
 *  draw_vertices - draw the vertices, and possibly their numbers
 */
void UI_Canvas::DrawVertices()
{
	int mapx0 = MAPX(x());
	int mapx9 = MAPX(x() + w());
	int mapy0 = MAPY(y() + h());
	int mapy9 = MAPY(y());

	const int r = vertex_radius (grid.Scale);

	fl_color (FL_GREEN);

	for (int n = 0; n < NumVertices; n++)
	{
		int mapx = Vertices[n].x;
		int mapy = Vertices[n].y;
		if (mapx >= mapx0 && mapx <= mapx9 && mapy >= mapy0 && mapy <= mapy9)
		{
			int scrx = SCREENX (mapx);
			int scry = SCREENY (mapy);

			fl_line(scrx - r, scry - r, scrx + r, scry + r);
			fl_line(scrx + r, scry - r, scrx - r, scry + r);
		}
	}

	if (e->show_object_numbers)
	{
		for (int n = 0; n < NumVertices; n++)
		{
			int mapx = Vertices[n].x;
			int mapy = Vertices[n].y;
			if (mapx >= mapx0 && mapx <= mapx9 && mapy >= mapy0 && mapy <= mapy9)
			{
				int x = (int) (SCREENX (mapx) + 2 * r);
				int y = SCREENY (mapy) + 2;
				DrawObjNum(x, y, n, VERTEX_NO);
			}
		}
	}
}


/*
 *  draw_linedefs - draw the linedefs
 */
void UI_Canvas::DrawLinedefs()
{
	int mapx0 = MAPX (x());
	int mapx9 = MAPX (x() + w());
	int mapy0 = MAPY (y() + h());
	int mapy9 = MAPY (y());

	switch (e->obj_type)
	{
		case OBJ_VERTICES:
		{
			fl_color (LIGHTGREY);
			for (int n = 0; n < NumLineDefs; n++)
			{
				int x1 = Vertices[LineDefs[n].start].x;
				int y1 = Vertices[LineDefs[n].start].y;
				int x2 = Vertices[LineDefs[n].end  ].x;
				int y2 = Vertices[LineDefs[n].end  ].y;
				if (x1 < mapx0 && x2 < mapx0
						|| x1 > mapx9 && x2 > mapx9
						|| y1 < mapy0 && y2 < mapy0
						|| y1 > mapy9 && y2 > mapy9)
					continue;
				DrawMapVector (x1, y1, x2, y2);
			}
			break;
		}

		case OBJ_LINEDEFS:
		{
			int current_colour = INT_MIN;  /* Some impossible colour no. */
			int new_colour;

			for (int n = 0; n < NumLineDefs; n++)
			{
				int x1 = Vertices[LineDefs[n].start].x;
				int x2 = Vertices[LineDefs[n].end  ].x;
				int y1 = Vertices[LineDefs[n].start].y;
				int y2 = Vertices[LineDefs[n].end  ].y;
				if (x1 < mapx0 && x2 < mapx0
						|| x1 > mapx9 && x2 > mapx9
						|| y1 < mapy0 && y2 < mapy0
						|| y1 > mapy9 && y2 > mapy9)
					continue;
				if (LineDefs[n].type != 0)  /* AYM 19980207: was "> 0" */
				{
					if (LineDefs[n].tag != 0)  /* AYM 19980207: was "> 0" */
						new_colour = LIGHTMAGENTA;
					else
						new_colour = LIGHTGREEN;
				}
				else if (LineDefs[n].flags & 1)
					new_colour = WHITE;
				else
					new_colour = LIGHTGREY;

				// Signal errors by drawing the linedef in red. Needs work.
				// Tag on a typeless linedef
				if (LineDefs[n].type == 0 && LineDefs[n].tag != 0)
					new_colour = LIGHTRED;
				// No first sidedef
				if (! is_sidedef (LineDefs[n].side_R))
					new_colour = LIGHTRED;
				// Bad second sidedef
				if (! is_sidedef (LineDefs[n].side_L) && LineDefs[n].side_L != -1)
					new_colour = LIGHTRED;

				if (new_colour != current_colour)
					fl_color (current_colour = new_colour);
				DrawMapLine (x1, y1, x2, y2);

				if (e->show_object_numbers)
				{
					int scnx0       = SCREENX (x1);
					int scnx1       = SCREENX (x2);
					int scny0       = SCREENY (y1);
					int scny1       = SCREENY (y2);
					int label_width = 5 * FONTW; ///!!! ((int) log10 (n) + 1) * FONTW;
					if (abs (scnx1 - scnx0) > label_width + 4
							|| abs (scny1 - scny0) > label_width + 4)
					{
						int scnx = (scnx0 + scnx1) / 2 - label_width / 2;
						int scny = (scny0 + scny1) / 2 - FONTH / 2;
						DrawObjNum(scnx, scny, n, LINEDEF_NO);
					}
				}
			}
			break;
		}

		case OBJ_SECTORS:
		{
			int current_colour = INT_MIN;  /* Some impossible colour no. */
			int new_colour;

			for (int n = 0; n < NumLineDefs; n++)
			{
				int x1 = Vertices[LineDefs[n].start].x;
				int x2 = Vertices[LineDefs[n].end  ].x;
				int y1 = Vertices[LineDefs[n].start].y;
				int y2 = Vertices[LineDefs[n].end  ].y;
				if (x1 < mapx0 && x2 < mapx0
						|| x1 > mapx9 && x2 > mapx9
						|| y1 < mapy0 && y2 < mapy0
						|| y1 > mapy9 && y2 > mapy9)
					continue;
				int sd1 = OBJ_NO_NONE;
				int sd2 = OBJ_NO_NONE;
				int s1  = OBJ_NO_NONE;
				int s2  = OBJ_NO_NONE;
				// FIXME should flag negative sidedef numbers as errors
				// FIXME should flag unused tag as errors
				if ((sd1 = LineDefs[n].side_R) < 0 || sd1 >= NumSideDefs
						|| (s1 = SideDefs[sd1].sector) < 0 || s1 >= NumSectors
						|| (sd2 = LineDefs[n].side_L) >= NumSideDefs
						|| sd2 >= 0 && ((s2 = SideDefs[sd2].sector) < 0
							|| s2 >= NumSectors))
				{
					new_colour = LIGHTRED;
				}
				else
				{
					bool have_tag  = false;
					bool have_type = false;
					if (Sectors[s1].tag != 0)
						have_tag = true;
					if (Sectors[s1].type != 0)
						have_type = true;
					if (sd2 >= 0)
					{
						if (Sectors[s2].tag != 0)
							have_tag = true;
						if (Sectors[s2].type != 0)
							have_type = true;
					}
					if (have_tag && have_type)
						new_colour = SECTOR_TAGTYPE;
					else if (have_tag)
						new_colour = SECTOR_TAG;
					else if (have_type)
						new_colour = SECTOR_TYPE;
					else if (LineDefs[n].flags & 1)
						new_colour = WHITE;
					else
						new_colour = LIGHTGREY;
				}
				if (new_colour != current_colour)
					fl_color (current_colour = new_colour);
				DrawMapLine (x1, y1, x2, y2);
			}
			break;
		}

		default:
		{
			int current_colour = INT_MIN;  /* Some impossible colour no. */
			int new_colour;

			for (int n = 0; n < NumLineDefs; n++)
			{
				int x1 = Vertices[LineDefs[n].start].x;
				int x2 = Vertices[LineDefs[n].end  ].x;
				int y1 = Vertices[LineDefs[n].start].y;
				int y2 = Vertices[LineDefs[n].end  ].y;
				if (x1 < mapx0 && x2 < mapx0
						|| x1 > mapx9 && x2 > mapx9
						|| y1 < mapy0 && y2 < mapy0
						|| y1 > mapy9 && y2 > mapy9)
					continue;
				if (LineDefs[n].flags & 1)
					new_colour = WHITE;
				else
					new_colour = LIGHTGREY;
				if (new_colour != current_colour)
					fl_color (current_colour = new_colour);
				DrawMapLine (x1, y1, x2, y2);
			}
			break;
		}
	}
}


/*
 *  draw_things_squares - the obvious
 */
void UI_Canvas::DrawThings()
{
	/* A thing is guaranteed to be totally off-screen
	   if its centre is more than <max_radius> units
	   beyond the edge of the screen. */
	int mapx0 = MAPX(0)   - MAX_RADIUS; //!!!! FIXME
	int mapx9 = MAPX(w()) + MAX_RADIUS;
	int mapy0 = MAPY(h()) - MAX_RADIUS;
	int mapy9 = MAPY(0)   + MAX_RADIUS;

	fl_color(THING_REM);

	for (int n = 0; n < NumThings; n++)
	{
		int mapx = Things[n].x;
		int mapy = Things[n].y;
		int corner_x;
		int corner_y;

		if (mapx < mapx0 || mapx > mapx9 || mapy < mapy0 || mapy > mapy9)
			continue;

		int m = get_thing_radius (Things[n].type);

		if (e->obj_type == OBJ_THINGS)
		{
			fl_color (get_thing_colour (Things[n].type));

			if (active_wmask)
			{
				if (Things[n].options & 1)
					fl_color (YELLOW);
				else if (Things[n].options & 2)
					fl_color (LIGHTGREEN);
				else if (Things[n].options & 4)
					fl_color (LIGHTRED);
				else
					fl_color (THING_REM);
			}
		}
		DrawMapLine (mapx - m, mapy - m, mapx + m, mapy - m);
		DrawMapLine (mapx + m, mapy - m, mapx + m, mapy + m);
		DrawMapLine (mapx + m, mapy + m, mapx - m, mapy + m);
		DrawMapLine (mapx - m, mapy + m, mapx - m, mapy - m);
		{
			size_t direction = angle_to_direction (Things[n].angle);
			static const short xsign[] = {  1,  1,  0, -1, -1, -1,  0,  1,  0 };
			static const short ysign[] = {  0,  1,  1,  1,  0, -1, -1, -1,  0 };
			corner_x = m * xsign[direction];
			corner_y = m * ysign[direction];
		}
		DrawMapLine (mapx, mapy, mapx + corner_x, mapy + corner_y);
	}
}


void UI_Canvas::DrawRTS()
{
  // TODO: DrawRTS
}


/*
 *  draw_obj_no - draw a number at screen coordinates (x, y)
 */
void UI_Canvas::DrawObjNum(int x, int y, int obj_no, Fl_Color c)
{
	fl_color(FL_BLACK);

	DrawScreenText (x - 2, y,     "%d", obj_no);
	DrawScreenText (x - 1, y,     "%d", obj_no);
	DrawScreenText (x + 1, y,     "%d", obj_no);
	DrawScreenText (x + 2, y,     "%d", obj_no);
	DrawScreenText (x,     y + 1, "%d", obj_no);
	DrawScreenText (x,     y - 1, "%d", obj_no);

	fl_color(c);

	DrawScreenText (x,     y,     "%d", obj_no);
}


void UI_Canvas::HighlightSet(Objid& obj)
{
	if (highlight == obj)
		return;
	
	highlight = obj;

	redraw();
}


void UI_Canvas::HighlightForget()
{
	if (highlight.is_nil())
		return;
	
	highlight.nil();

	redraw();
}

/*
   highlight the selected object
*/
void UI_Canvas::HighlightObject (int objtype, int objnum, Fl_Color colour)
{
	int  n, m;

	fl_color (colour);

	// fprintf(stderr, "HighlightObject: %d\n", objnum);

	switch (objtype)
	{
		case OBJ_THINGS:
			m = (get_thing_radius (Things[objnum].type) * 3) / 2;
			DrawMapLine (Things[objnum].x - m, Things[objnum].y - m,
					Things[objnum].x - m, Things[objnum].y + m);
			DrawMapLine (Things[objnum].x - m, Things[objnum].y + m,
					Things[objnum].x + m, Things[objnum].y + m);
			DrawMapLine (Things[objnum].x + m, Things[objnum].y + m,
					Things[objnum].x + m, Things[objnum].y - m);
			DrawMapLine (Things[objnum].x + m, Things[objnum].y - m,
					Things[objnum].x - m, Things[objnum].y - m);
			DrawMapArrow (Things[objnum].x, Things[objnum].y,
					Things[objnum].angle * 182);
			break;

		case OBJ_LINEDEFS:
			n = (Vertices[LineDefs[objnum].start].x
					+ Vertices[LineDefs[objnum].end].x) / 2;
			m = (Vertices[LineDefs[objnum].start].y
					+ Vertices[LineDefs[objnum].end].y) / 2;
			DrawMapLine (n, m, n + (Vertices[LineDefs[objnum].end].y
						- Vertices[LineDefs[objnum].start].y) / 3,
					m + (Vertices[LineDefs[objnum].start].x
						- Vertices[LineDefs[objnum].end].x) / 3);
			
			fl_line_style(FL_SOLID, 2);

			DrawMapVector (Vertices[LineDefs[objnum].start].x,
					Vertices[LineDefs[objnum].start].y,
					Vertices[LineDefs[objnum].end].x,
					Vertices[LineDefs[objnum].end].y);
			if (colour != LIGHTRED && LineDefs[objnum].tag > 0)
			{
				for (m = 0; m < NumSectors; m++)
					if (Sectors[m].tag == LineDefs[objnum].tag)
						HighlightObject (OBJ_SECTORS, m, LIGHTRED);
			}

			fl_line_style(FL_SOLID, 1);
			break;

		case OBJ_VERTICES:
		{
			int r = vertex_radius (grid.Scale) * 3 / 2;
			int scrx0 = SCREENX (Vertices[objnum].x) - r;
			int scrx9 = SCREENX (Vertices[objnum].x) + r;
			int scry0 = SCREENY (Vertices[objnum].y) - r;
			int scry9 = SCREENY (Vertices[objnum].y) + r;

			fl_line(scrx0, scry0, scrx9, scry0);
			fl_line(scrx9, scry0, scrx9, scry9);
			fl_line(scrx9, scry9, scrx0, scry9);
			fl_line(scrx0, scry9, scrx0, scry0);
		}
		break;

		case OBJ_SECTORS:
		{
			fl_line_style(FL_SOLID, 2);

			int mapx0 = MAPX(0);
			int mapy0 = MAPY(w());
			int mapx9 = MAPX(ScrMaxX);
			int mapy9 = MAPY(0);

			for (n = 0; n < NumLineDefs; n++)
				if (LineDefs[n].side_R != -1
						&& SideDefs[LineDefs[n].side_R].sector == objnum
						|| LineDefs[n].side_L != -1
						&& SideDefs[LineDefs[n].side_L].sector == objnum)
				{
					const struct Vertex *v1 = Vertices + LineDefs[n].start;
					const struct Vertex *v2 = Vertices + LineDefs[n].end;

					if (v1->x < mapx0 && v2->x < mapx0
							|| v1->x > mapx9 && v2->x > mapx9
							|| v1->y < mapy0 && v2->y < mapy0
							|| v1->y > mapy9 && v2->y > mapy9)
						continue;  // Off-screen
					DrawMapLine (v1->x, v1->y, v2->x, v2->y);
				}
			if (colour != LIGHTRED && Sectors[objnum].tag > 0)
			{
				for (m = 0; m < NumLineDefs; m++)
					if (LineDefs[m].tag == Sectors[objnum].tag)
						HighlightObject (OBJ_LINEDEFS, m, LIGHTRED);
			}

			fl_line_style(FL_SOLID, 1);
		}
		break;
	}
}

/*
   highlight the selected objects
*/
void UI_Canvas::HighlightSelection(selection_c * list)
{
	if (! list)
		return;

	selection_iterator_c it;

	for (list->begin(&it); !it.at_end(); ++it)
		HighlightObject(list->what_type(), *it, fl_rgb_color(255,192,128));
}


/*
 *  draw_map_point - draw a point at map coordinates
 *
 *  The point is drawn at map coordinates (<mapx>, <mapy>)
 */
void UI_Canvas::DrawMapPoint (int mapx, int mapy)
{
    fl_point(SCREENX(mapx), SCREENY(mapy));
}


/*
 *  DrawMapLine - draw a line on the screen from map coords
 */
void UI_Canvas::DrawMapLine (int mapx1, int mapy1, int mapx2, int mapy2)
{
    fl_line(SCREENX(mapx1), SCREENY(mapy1),
            SCREENX(mapx2), SCREENY(mapy2));
}



/*
 *  DrawMapVector - draw an arrow on the screen from map coords
 */
void UI_Canvas::DrawMapVector (int mapx1, int mapy1, int mapx2, int mapy2)
{
	int scrx1 = SCREENX (mapx1);
	int scry1 = SCREENY (mapy1);
	int scrx2 = SCREENX (mapx2);
	int scry2 = SCREENY (mapy2);

	double r  = hypot ((double) (scrx1 - scrx2), (double) (scry1 - scry2));
#if 0
	/* AYM 19980216 to avoid getting huge arrowheads when zooming in */
	int    scrXoff = (r >= 1.0) ? (int) ((scrx1 - scrx2) * 8.0 / r * (Scale < 1 ? Scale : 1)) : 0;
	int    scrYoff = (r >= 1.0) ? (int) ((scry1 - scry2) * 8.0 / r * (Scale < 1 ? Scale : 1)) : 0;
#else
	int scrXoff = (r >= 1.0) ? (int) ((scrx1 - scrx2) * 8.0 / r * (grid.Scale / 2)) : 0;
	int scrYoff = (r >= 1.0) ? (int) ((scry1 - scry2) * 8.0 / r * (grid.Scale / 2)) : 0;
#endif

	fl_line(scrx1, scry1, scrx2, scry2);

	scrx1 = scrx2 + 2 * scrXoff;
	scry1 = scry2 + 2 * scrYoff;

	fl_line(scrx1 - scrYoff, scry1 + scrXoff, scrx2, scry2);
	fl_line(scrx1 + scrYoff, scry1 - scrXoff, scrx2, scry2);
}


/*
 *  DrawMapArrow - draw an arrow on the screen from map coords and angle (0 - 65535)
 */
void UI_Canvas::DrawMapArrow (int mapx1, int mapy1, unsigned angle)
{
	int mapx2 = mapx1 + (int) (50 * cos (angle / 10430.37835));
	int mapy2 = mapy1 + (int) (50 * sin (angle / 10430.37835));
	int scrx1 = SCREENX (mapx1);
	int scry1 = SCREENY (mapy1);
	int scrx2 = SCREENX (mapx2);
	int scry2 = SCREENY (mapy2);

	double r = hypot (scrx1 - scrx2, scry1 - scry2);
#if 0
	int    scrXoff = (r >= 1.0) ? (int) ((scrx1 - scrx2) * 8.0 / r * (Scale < 1 ? Scale : 1)) : 0;
	int    scrYoff = (r >= 1.0) ? (int) ((scry1 - scry2) * 8.0 / r * (Scale < 1 ? Scale : 1)) : 0;
#else
	int scrXoff = (r >= 1.0) ? (int) ((scrx1 - scrx2) * 8.0 / r * (grid.Scale / 2)) : 0;
	int scrYoff = (r >= 1.0) ? (int) ((scry1 - scry2) * 8.0 / r * (grid.Scale / 2)) : 0;
#endif

	fl_line(scrx1, scry1, scrx2, scry2);

	scrx1 = scrx2 + 2 * scrXoff;
	scry1 = scry2 + 2 * scrYoff;

	fl_line(scrx1 - scrYoff, scry1 + scrXoff, scrx2, scry2);
	fl_line(scrx1 + scrYoff, scry1 - scrXoff, scrx2, scry2);
}


void UI_Canvas::SelboxBegin(int mapx, int mapy)
{
	selbox_active = true;
	selbox_x1 = selbox_x2 = mapx;
	selbox_y1 = selbox_y2 = mapy;
}

void UI_Canvas::SelboxDrag(int mapx, int mapy)
{
	selbox_x2 = mapx;
	selbox_y2 = mapy;

	redraw();
}

void UI_Canvas::SelboxFinish(int *x1, int *y1, int *x2, int *y2)
{
	selbox_active = false;

	*x1 = MIN(selbox_x1, selbox_x2);
	*x2 = MAX(selbox_x1, selbox_x2);

	*y1 = MIN(selbox_y1, selbox_y2);
	*y2 = MAX(selbox_y1, selbox_y2);
}

void UI_Canvas::SelboxDraw()
{
	int x1 = MIN(selbox_x1, selbox_x2);
	int x2 = MAX(selbox_x1, selbox_x2);

	int y1 = MIN(selbox_y1, selbox_y2);
	int y2 = MAX(selbox_y1, selbox_y2);

	fl_color(FL_CYAN);

	DrawMapLine (x1, y1, x2, y1);
	DrawMapLine (x2, y1, x2, y2);
	DrawMapLine (x2, y2, x1, y2);
	DrawMapLine (x1, y2, x1, y1);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
