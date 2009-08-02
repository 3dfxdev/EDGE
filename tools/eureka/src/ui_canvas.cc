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
	map_lx = MAPX(x());
	map_ly = MAPY(y()+h());
	map_hx = MAPX(x()+w());
	map_hy = MAPY(y());

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

	if (e->obj_type == OBJ_RADTRIGS)
		DrawRTS();


	int n;

	// Draw the things numbers
	if (e->obj_type == OBJ_THINGS && e->show_object_numbers)
	{
		for (n = 0; n < NumThings; n++)
		{
			int x = Things[n]->x;
			int y = Things[n]->y;

			if (Vis(x, y, MAX_RADIUS))
				DrawObjNum(SCREENX(x) + FONTW, SCREENY(y) + 2, n, THING_NO);
		}
	}

	// Draw the sector numbers
	if (e->obj_type == OBJ_SECTORS && e->show_object_numbers)
	{
		int xoffset = - FONTW / 2;

		for (n = 0; n < NumSectors; n++)
		{
			int x;
			int y;

			centre_of_sector(n, &x, &y);

			if (Vis(x, y, MAX_RADIUS))
				DrawObjNum(SCREENX(x) + xoffset, SCREENY(y) - FONTH / 2, n, SECTOR_NO);

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
	int grid_step_1 = 1 * grid.step;    // Map units between dots
	int grid_step_2 = 8 * grid_step_1;  // Map units between dim lines
	int grid_step_3 = 8 * grid_step_2;  // Map units between bright lines

	float pixels_1 = grid.step * grid.Scale;


	if (pixels_1 < 1.99)
	{
		fl_color(GRID_DARK);
		fl_rectf(x(), y(), w(), h());
		return;
	}


	fl_color (GRID_BRIGHT);
	{
		int gx = (map_lx / grid_step_3) * grid_step_3;

		for (; gx <= map_hx; gx += grid_step_3)
			DrawMapLine(gx, map_ly-2, gx, map_hy+2);

		int gy = (map_ly / grid_step_3) * grid_step_3;

		for (; gy <=  map_hy; gy += grid_step_3)
			DrawMapLine(map_lx, gy, map_hx, gy);
	}


	fl_color (GRID_MEDIUM);
	{
		int gx = (map_lx / grid_step_2) * grid_step_2;

		for (; gx <= map_hx; gx += grid_step_2)
			if (gx % grid_step_3 != 0)
				DrawMapLine(gx, map_ly, gx, map_hy);

		int gy = (map_ly / grid_step_2) * grid_step_2;

		for (; gy <= map_hy; gy += grid_step_2)
			if (gy % grid_step_3 != 0)
				DrawMapLine(map_lx, gy, map_hx, gy);
	}


	// POINTS

	if (pixels_1 < 3.99)
		fl_color(GRID_MEDIUM);
	//??  else if (pixels_1 > 30.88)
	//??    fl_color(GRID_BRIGHT);
	else
		fl_color(GRID_POINT);

	{
		int gx = (map_lx / grid_step_1) * grid_step_1;
		int gy = (map_ly / grid_step_1) * grid_step_1;

		for (int ny = gy; ny <= map_hy; ny += grid_step_1)
		for (int nx = gx; nx <= map_hx; nx += grid_step_1)
		{
			int sx = SCREENX(nx);
			int sy = SCREENY(ny);

			if (pixels_1 < 30.99)
				fl_point(sx, sy);
			else
			{
				fl_line(sx-0, sy, sx+1, sy);
				fl_line(sx, sy-0, sx, sy+1);
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
int vertex_radius(double scale)
{
	const int VERTEX_PIXELS = 6;

	return (int)(VERTEX_PIXELS * (0.2 + scale / 2));
}



/*
 *  draw_vertices - draw the vertices, and possibly their numbers
 */
void UI_Canvas::DrawVertices()
{
	const int r = vertex_radius(grid.Scale);

	fl_color(FL_GREEN);

	for (int n = 0; n < NumVertices; n++)
	{
		int x = Vertices[n]->x;
		int y = Vertices[n]->y;

		if (Vis(x, y, r))
		{
			int scrx = SCREENX(x);
			int scry = SCREENY(y);

			fl_line(scrx - r, scry - r, scrx + r, scry + r);
			fl_line(scrx + r, scry - r, scrx - r, scry + r);
		}
	}

	if (e->show_object_numbers)
	{
		for (int n = 0; n < NumVertices; n++)
		{
			int x = Vertices[n]->x;
			int y = Vertices[n]->y;

			if (Vis(x, y, r))
			{
				int sx = SCREENX(x) + 2 * r;
				int sy = SCREENY(y) + 2;

				DrawObjNum(sx, sy, n, VERTEX_NO);
			}
		}
	}
}


/*
 *  draw_linedefs - draw the linedefs
 */
void UI_Canvas::DrawLinedefs()
{
	int new_colour;

	for (int n = 0; n < NumLineDefs; n++)
	{
		int x1 = LineDefs[n]->Start()->x;
		int y1 = LineDefs[n]->Start()->y;
		int x2 = LineDefs[n]->End  ()->x;
		int y2 = LineDefs[n]->End  ()->y;

		if (! Vis(MIN(x1,x2), MIN(y1,y2), MAX(x1,x2), MAX(y1,y2)))
			continue;

		switch (e->obj_type)
		{
			case OBJ_VERTICES:
			{
				fl_color(LIGHTGREY);

				DrawMapVector(x1, y1, x2, y2);
			}
			break;

			case OBJ_LINEDEFS:
			{
				int new_colour = LIGHTGREY;

				if (LineDefs[n]->type != 0)  /* AYM 19980207: was "> 0" */
				{
					if (LineDefs[n]->tag != 0)  /* AYM 19980207: was "> 0" */
						new_colour = LIGHTMAGENTA;
					else
						new_colour = LIGHTGREEN;
				}
				else if (LineDefs[n]->flags & 1)
					new_colour = WHITE;

				// Signal errors by drawing the linedef in red. Needs work.
				// Tag on a typeless linedef
				if (LineDefs[n]->type == 0 && LineDefs[n]->tag != 0)
					new_colour = LIGHTRED;
				// No first sidedef
				if (! LineDefs[n]->Right())
					new_colour = LIGHTRED;

				fl_color (new_colour);

				DrawMapLine(x1, y1, x2, y2);

///				int mx = (x1 + x2) / 2;
///				int my = (y1 + y2) / 2;
///
///				DrawMapLine(mx, my, mx + (y2 - y1) / 5, my + (x1 - x2) / 5);

				if (e->show_object_numbers)
				{
					int scnx0       = SCREENX (x1);
					int scnx1       = SCREENX (x2);
					int scny0       = SCREENY (y1);
					int scny1       = SCREENY (y2);
					int label_width = 5 * FONTW; ///!!! ((int) log10 (n) + 1) * FONTW;
					if (abs(scnx1 - scnx0) > label_width + 4
							|| abs (scny1 - scny0) > label_width + 4)
					{
						int scnx = (scnx0 + scnx1) / 2 - label_width / 2;
						int scny = (scny0 + scny1) / 2 - FONTH / 2;
						DrawObjNum(scnx, scny, n, LINEDEF_NO);
					}
				}
			}
			break;

			case OBJ_SECTORS:
			{
				int sd1 = OBJ_NO_NONE;
				int sd2 = OBJ_NO_NONE;
				int s1  = OBJ_NO_NONE;
				int s2  = OBJ_NO_NONE;
				// FIXME should flag negative sidedef numbers as errors
				// FIXME should flag unused tag as errors
				if ((sd1 = LineDefs[n]->right) < 0 || sd1 >= NumSideDefs
						|| (s1 = SideDefs[sd1]->sector) < 0 || s1 >= NumSectors
						|| (sd2 = LineDefs[n]->left) >= NumSideDefs
						|| sd2 >= 0 && ((s2 = SideDefs[sd2]->sector) < 0
							|| s2 >= NumSectors))
				{
					new_colour = LIGHTRED;
				}
				else
				{
					bool have_tag  = false;
					bool have_type = false;

					if (Sectors[s1]->tag != 0)
						have_tag = true;
					if (Sectors[s1]->type != 0)
						have_type = true;
					if (sd2 >= 0)
					{
						if (Sectors[s2]->tag != 0)
							have_tag = true;
						if (Sectors[s2]->type != 0)
							have_type = true;
					}
					if (have_tag && have_type)
						new_colour = SECTOR_TAGTYPE;
					else if (have_tag)
						new_colour = SECTOR_TAG;
					else if (have_type)
						new_colour = SECTOR_TYPE;
					else if (LineDefs[n]->flags & 1)
						new_colour = WHITE;
					else
						new_colour = LIGHTGREY;
				}
				
				fl_color(new_colour);

				DrawMapLine(x1, y1, x2, y2);
			}
			break;

			default:
			{
				if (LineDefs[n]->flags & 1)
					new_colour = WHITE;
				else
					new_colour = LIGHTGREY;

				fl_color(new_colour);

				DrawMapLine(x1, y1, x2, y2);
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
	fl_color(THING_REM);

	for (int n = 0; n < NumThings; n++)
	{
		int x = Things[n]->x;
		int y = Things[n]->y;

		if (! Vis(x, y, MAX_RADIUS))
			continue;

		int m = get_thing_radius(Things[n]->type);

		if (e->obj_type == OBJ_THINGS)
		{
			fl_color(get_thing_colour(Things[n]->type));

			if (active_wmask)
			{
				if (Things[n]->options & 1)
					fl_color (YELLOW);
				else if (Things[n]->options & 2)
					fl_color (LIGHTGREEN);
				else if (Things[n]->options & 4)
					fl_color (LIGHTRED);
				else
					fl_color (THING_REM);
			}
		}

		DrawMapLine(x-m, y-m, x+m, y-m);
		DrawMapLine(x+m, y-m, x+m, y+m);
		DrawMapLine(x+m, y+m, x-m, y+m);
		DrawMapLine(x-m, y+m, x-m, y-m);

		{
			size_t direction = angle_to_direction(Things[n]->angle);
			static const short xsign[] = {  1,  1,  0, -1, -1, -1,  0,  1,  0 };
			static const short ysign[] = {  0,  1,  1,  1,  0, -1, -1, -1,  0 };

			int corner_x = m * xsign[direction];
			int corner_y = m * ysign[direction];

			DrawMapLine(x, y, x + corner_x, y + corner_y);
		}
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

	DrawScreenText(x - 2, y,     "%d", obj_no);
	DrawScreenText(x - 1, y,     "%d", obj_no);
	DrawScreenText(x + 1, y,     "%d", obj_no);
	DrawScreenText(x + 2, y,     "%d", obj_no);
	DrawScreenText(x,     y + 1, "%d", obj_no);
	DrawScreenText(x,     y - 1, "%d", obj_no);

	fl_color(c);

	DrawScreenText(x,     y,     "%d", obj_no);
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
void UI_Canvas::HighlightObject(int objtype, int objnum, Fl_Color colour)
{
	fl_color(colour);

	// fprintf(stderr, "HighlightObject: %d\n", objnum);

	switch (objtype)
	{
		case OBJ_THINGS:
		{
			int x = Things[objnum]->x;
			int y = Things[objnum]->y;

			if (! Vis(x, y, MAX_RADIUS))
				return;

			int m = (get_thing_radius(Things[objnum]->type) * 3) / 2;

			DrawMapLine(x - m, y - m, x - m, y + m);
			DrawMapLine(x - m, y + m, x + m, y + m);
			DrawMapLine(x + m, y + m, x + m, y - m);
			DrawMapLine(x + m, y - m, x - m, y - m);

			DrawMapArrow(x, y, Things[objnum]->angle * 182);
		}
		break;

		case OBJ_LINEDEFS:
		{
			int x1 = LineDefs[objnum]->Start()->x;
			int y1 = LineDefs[objnum]->Start()->y;
			int x2 = LineDefs[objnum]->End  ()->x;
			int y2 = LineDefs[objnum]->End  ()->y;

			if (! Vis(MIN(x1,x2), MIN(y1,y2), MAX(x1,x2), MAX(y1,y2)))
				return;

			int mx = (x1 + x2) / 2;
			int my = (y1 + y2) / 2;

			DrawMapLine(mx, my, mx + (y2 - y1) / 5, my + (x1 - x2) / 5);

			fl_line_style(FL_SOLID, 2);

			DrawMapVector(x1, y1, x2, y2);

			if (colour != LIGHTRED && LineDefs[objnum]->tag > 0)
			{
				for (int m = 0; m < NumSectors; m++)
					if (Sectors[m]->tag == LineDefs[objnum]->tag)
						HighlightObject (OBJ_SECTORS, m, LIGHTRED);
			}

			fl_line_style(FL_SOLID, 1);
		}
		break;

		case OBJ_VERTICES:
		{
			int x = Vertices[objnum]->x;
			int y = Vertices[objnum]->y;

			if (! Vis(x, y, MAX_RADIUS))
				return;

			int r = vertex_radius(grid.Scale) * 3 / 2;

			int sx1 = SCREENX(x) - r;
			int sy1 = SCREENY(y) - r;
			int sx2 = SCREENX(x) + r;
			int sy2 = SCREENY(y) + r;

			fl_line(sx1, sy1, sx2, sy1);
			fl_line(sx2, sy1, sx2, sy2);
			fl_line(sx2, sy2, sx1, sy2);
			fl_line(sx1, sy2, sx1, sy1);
		}
		break;

		case OBJ_SECTORS:
		{
			fl_line_style(FL_SOLID, 2);

			for (int n = 0; n < NumLineDefs; n++)
			{
				if (! LineDefs[n]->TouchesSector(Sectors[objnum]))
					continue;

				int x1 = LineDefs[n]->Start()->x;
				int y1 = LineDefs[n]->Start()->y;
				int x2 = LineDefs[n]->End  ()->x;
				int y2 = LineDefs[n]->End  ()->y;

				if (! Vis(MIN(x1,x2), MIN(y1,y2), MAX(x1,x2), MAX(y1,y2)))
					continue;

				DrawMapLine(x1, y1, x2, y2);
			}

			fl_line_style(FL_SOLID, 1);

			if (colour != LIGHTRED && Sectors[objnum]->tag > 0)
			{
				for (int m = 0; m < NumLineDefs; m++)
					if (LineDefs[m]->tag == Sectors[objnum]->tag)
						HighlightObject(OBJ_LINEDEFS, m, LIGHTRED);
			}
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
void UI_Canvas::DrawMapPoint(int mapx, int mapy)
{
    fl_point(SCREENX(mapx), SCREENY(mapy));
}


/*
 *  DrawMapLine - draw a line on the screen from map coords
 */
void UI_Canvas::DrawMapLine(int mapx1, int mapy1, int mapx2, int mapy2)
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

	double r  = hypot((double) (scrx1 - scrx2), (double) (scry1 - scry2));
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
void UI_Canvas::DrawMapArrow(int mapx1, int mapy1, unsigned angle)
{
	int mapx2 = mapx1 + (int) (50 * cos(angle / 10430.37835));
	int mapy2 = mapy1 + (int) (50 * sin(angle / 10430.37835));
	int scrx1 = SCREENX(mapx1);
	int scry1 = SCREENY(mapy1);
	int scrx2 = SCREENX(mapx2);
	int scry2 = SCREENY(mapy2);

	double r = hypot(scrx1 - scrx2, scry1 - scry2);
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

	DrawMapLine(x1, y1, x2, y1);
	DrawMapLine(x2, y1, x2, y2);
	DrawMapLine(x2, y2, x1, y2);
	DrawMapLine(x1, y2, x1, y1);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
