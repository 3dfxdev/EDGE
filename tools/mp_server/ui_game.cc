//------------------------------------------------------------------------
//  Game list widget
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

#include "includes.h"
#include "ui_game.h"
#include "ui_window.h"

//
// GameList Constructor
//
UI_GameList::UI_GameList(int x, int y, int w, int h) :
    Fl_Table_Row(x, y, w, h)
{
	// cancel the automatic `begin' in Fl_Group constructor
	end();
 
	type(SELECT_SINGLE);

	cols(5);

	for (int cc = 0; cc < cols(); cc++)
		col_width(cc, get_column_width(cc, w));

	col_header(1);
	col_header_height(20);

	rows(2);
	row_height_all(20);

	row_header(0);
	/* row_header_width(40); */

#if 0 // ifdef MACOSX
	// the resize box in the lower right corner is a pain, it ends up
	// covering the text box scroll button.  Luckily when both scrollbars
	// are active, FLTK leaves a square space in that corner and it
	// fits in nicely.
	has_scrollbar(BOTH_ALWAYS);
#endif

	selection_color(FL_YELLOW);
}

//
// GameList Destructor
//
UI_GameList::~UI_GameList()
{
}

//------------------------------------------------------------------------

int UI_GameList::get_column_width(int col, int total_W) const
{
	SYS_ASSERT(col >= 0);

	if (col == 0)  // ID
		return 80;

	if (col < cols())
	{
		return MAX(100, (total_W - 80) / (cols()-1));
	}

	return 0;  /* dummy */
}

const char * UI_GameList::get_column_name(int col) const
{
	SYS_ASSERT(col >= 0);

	switch (col)
	{
		case 0: return "ID#";
		case 1: return "Level";
		case 2: return "Status";
		case 3: return "Players";
		case 4: return "Bots";

		default:
			break;
	}

	return "Extra";  /* dummy */
}

void UI_GameList::resize(int x, int y, int w, int h)
{
	Fl_Table_Row::resize(x, y, w, h);

	for (int cc = 0; cc < cols(); cc++)
		col_width(cc, get_column_width(cc, w));
}

void UI_GameList::draw_cell(TableContext context, int R, int C,
		int X, int Y, int W, int H)
{
    switch (context)
    {
		case CONTEXT_STARTPAGE:
			// fl_font(FL_HELVETICA, 16);
			break;

		case CONTEXT_ENDPAGE:
			break;

		case CONTEXT_ROW_HEADER:  // unused
			break;

		case CONTEXT_COL_HEADER:
		{
			fl_push_clip(X, Y, W, H);
			{
				fl_draw_box(FL_THIN_UP_BOX, X, Y, W, H, color());
				fl_color(FL_BLACK);
				fl_draw(get_column_name(C), X, Y, W, H, FL_ALIGN_CENTER);
			}
			fl_pop_clip();
			break;
		}

		case CONTEXT_CELL:
		{
			fl_push_clip(X, Y, W, H);
			{
				// BG COLOR
				fl_color(row_selected(R) ? selection_color() : FL_WHITE);
				fl_rectf(X, Y, W, H);

				// TEXT
				fl_color(FL_BLACK);
				fl_draw("Foo", X, Y, W, H, FL_ALIGN_CENTER);

				// BORDER
				fl_color(FL_LIGHT2);
				fl_rect(X, Y, W, H);
			}
			fl_pop_clip();
			break;
		}
                                                                                            
		default:
			break;
    }
}

