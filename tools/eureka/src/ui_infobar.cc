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

#include "main.h"
#include "ui_window.h"

#include "grid2.h"


//
// UI_InfoBar Constructor
//
UI_InfoBar::UI_InfoBar(int X, int Y, int W, int H, const char *label) : 
    Fl_Group(X, Y, W, H, label)
{
	end();  // cancel begin() in Fl_Group constructor

	box(FL_FLAT_BOX);


	mode = new Fl_Choice(X+50, Y+4, 88, H-4, "Mode:");
	mode->align(FL_ALIGN_LEFT);
	mode->add("Things|Linedefs|Sectors|Vertices|RTS");
	mode->value(0);
	mode->callback(mode_callback, this);
	mode->labelsize(QF_F);
	mode->textsize(QF_F);

	add(mode);

	X = mode->x() + mode->w() + 8;


	scale = new Fl_Choice(X+50, Y+4, 78, H-4, "Scale:");
	scale->align(FL_ALIGN_LEFT);
	scale->add(Grid_State_c::scale_options());
	scale->value(8);
	scale->callback(scale_callback, this);
	scale->labelsize(QF_F);
	scale->textsize(QF_F);

	add(scale);

	X = scale->x() + scale->w() + 4;


	grid_size = new Fl_Choice(X+50, Y+4, 56, H-4, "Grid:");

	grid_size->align(FL_ALIGN_LEFT);
	grid_size->add(Grid_State_c::grid_options());
	grid_size->value(1);
	grid_size->callback(grid_callback, this);
	grid_size->labelsize(QF_F);
	grid_size->textsize(QF_F);

	add(grid_size);

	X = grid_size->x() + grid_size->w() + 6;


	grid_lock = new Fl_Choice(X, Y+4, 72, H-4);
	grid_lock->add("LOCK|Snap|FREE");
	grid_lock->value(1);
	grid_lock->callback(lock_callback, this);
	grid_lock->labelsize(QF_F);
	grid_lock->textsize(QF_F);

	add(grid_lock);

	X = grid_lock->x() + grid_lock->w() + 10;


	mouse_x = new Fl_Output(X+28,       Y+4, 64, H-4, "x");
	mouse_y = new Fl_Output(X+28+72+10, Y+4, 64, H-4, "y");

	mouse_x->align(FL_ALIGN_LEFT);
	mouse_y->align(FL_ALIGN_LEFT);

	mouse_x->labelsize(QF_F); mouse_y->labelsize(QF_F);
	mouse_x->textsize(QF_F);  mouse_y->textsize(QF_F);

	add(mouse_x);
	add(mouse_y);

	X = mouse_y->x() + mouse_y->w() + 12;


	map_name = new Fl_Box(FL_FLAT_BOX, X, Y+4, 80, H-4, "");
	map_name->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
	map_name->label("MAP01");
	map_name->labelsize(QF_F);

	add(map_name);


	// ---- resizable ----
 
}

//
// UI_InfoBar Destructor
//
UI_InfoBar::~UI_InfoBar()
{
}

int UI_InfoBar::handle(int event)
{
  return Fl_Group::handle(event);
}


void UI_InfoBar::mode_callback(Fl_Widget *w, void *data)
{
  Fl_Choice *mode = (Fl_Choice *)w;

  static const char *mode_keys = "tlsvr";

  main_win->SetMode(mode_keys[mode->value()]);

  UI_InfoBar *bar = (UI_InfoBar *)data;

  bar->UpdateModeColor();
}


void UI_InfoBar::scale_callback(Fl_Widget *w, void *data)
{
  Fl_Choice *choice = (Fl_Choice *)w;

  grid.ScaleFromWidget(choice->value());
}


void UI_InfoBar::grid_callback(Fl_Widget *w, void *data)
{
  Fl_Choice *choice = (Fl_Choice *)w;

  grid.StepFromWidget(choice->value());
}


void UI_InfoBar::lock_callback(Fl_Widget *w, void *data)
{
  UI_InfoBar *bar = (UI_InfoBar *)data;

  // FIXME: editor state!!!!

  bar->UpdateLockColor();
}



//------------------------------------------------------------------------

void UI_InfoBar::SetMode(char mode_ch)
{
  switch (mode_ch)
  {
    case 't': mode->value(0); break;
    case 'l': mode->value(1); break;
    case 's': mode->value(2); break;
    case 'v': mode->value(3); break;
    case 'r': mode->value(4); break;

    default: break;
  }

  UpdateModeColor();
}


void UI_InfoBar::SetMouse(double mx, double my)
{
  if (mx < -32767.0 || mx > 32767.0 ||
      my < -32767.0 || my > 32767.0)
  {
    mouse_x->value("off map");
    mouse_y->value("off map");

    return;
  }

  char x_buffer[60];
  char y_buffer[60];

  sprintf(x_buffer, "%d", I_ROUND(mx));
  sprintf(y_buffer, "%d", I_ROUND(my));

  mouse_x->value(x_buffer);
  mouse_y->value(y_buffer);
}


void UI_InfoBar::SetMap(const char *name, const char *wad_name)
{
  char buffer[512];

  sprintf(buffer, "%s  %s\n", name, wad_name);

  map_name->copy_label(buffer);
}


void UI_InfoBar::SetChanged(bool is_changed)
{
  map_name->labelcolor(is_changed ? FL_RED : FL_BLACK);

  redraw();
}


void UI_InfoBar::SetScale(int i)
{
  scale->value(i);
}

void UI_InfoBar::SetGrid(int i)
{
  grid_size->value(i);
}


void UI_InfoBar::UpdateLock()
{
  if (grid.locked)
    grid_lock->value(0);
  else if (grid.snap)
    grid_lock->value(1);
  else
    grid_lock->value(2);

  UpdateLockColor();
}


#if 0

void UI_InfoBar::SetZoom(float zoom_mul)
{
  char buffer[60];

///  if (0.99 < zoom_mul && zoom_mul < 1.01)
///  {
///    grid_size->value("1:1");
///    return;
///  }

  if (zoom_mul < 0.99)
  {
    sprintf(buffer, "/ %1.3f", 1.0/zoom_mul);
  }
  else // zoom_mul > 1
  {
    sprintf(buffer, "x %1.3f", zoom_mul);
  }

  grid_size->value(buffer);
}


void UI_InfoBar::SetNodeIndex(int index)
{
#if 0
  char buffer[60];

  sprintf(buffer, "%d", index);
  
  ns_index->label("Node #");
  ns_index->value(buffer);

  redraw();
#endif
}


void UI_InfoBar::SetWhich(int index, int total)
{
  if (index < 0)
  {
    which->label(INDEX_NONE_STR);
  }
  else
  {
    char buffer[200];
    sprintf(buffer, "Index: #%d of %d", index, total);
 
    which->copy_label(buffer);
  }

  redraw();
}
#endif


void UI_InfoBar::UpdateModeColor()
{
  switch (mode->value())
  {
    case 0: /* Things   */ mode->color(FL_MAGENTA); break;
    case 1: /* Linedefs */ mode->color(fl_rgb_color(0,128,255)); break;
    case 2: /* Sectors  */ mode->color(FL_YELLOW); break;
    case 3: /* Vertices */ mode->color(fl_rgb_color(0,192,96));  break;
    case 4: /* RTS      */ mode->color(FL_RED); break;
  }
}


void UI_InfoBar::UpdateLockColor()
{
  switch (grid_lock->value())
  {
    case 0: /* Lock */ grid_lock->color(fl_rgb_color(255,128,128)); break;
    case 1: /* Snap */ grid_lock->color(FL_BACKGROUND_COLOR); break;
    case 2: /* Free */ grid_lock->color(fl_rgb_color(128,255,128)); break;
  }
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
