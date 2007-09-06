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

#include "headers.h"
#include "hdr_fltk.h"

#include "lib_util.h"

#include "ui_panel.h"
#include "ui_radius.h"


#define INFO_BG_COLOR  fl_rgb_color(96)


//
// UI_Panel Constructor
//
UI_Panel::UI_Panel(int X, int Y, int W, int H, const char *label) : 
    Fl_Group(X, Y, W, H, label)
{
  end();  // cancel begin() in Fl_Group constructor

  box(FL_FLAT_BOX);

//  color(INFO_BG_COLOR, INFO_BG_COLOR);

  X += 6;
  Y += 6;

  W -= 12;
  H -= 12;

  // ---- top section ----
 
  map_name = new Fl_Output(X+84, Y, W-84, 22, "Map Name:");
  map_name->align(FL_ALIGN_LEFT);

  add(map_name);

  Y += map_name->h() + 8;


  mode = new Fl_Choice(X+52, Y, W-64, 22, "Mode:");
  mode->align(FL_ALIGN_LEFT);
  mode->add("Scripts|Things");
  mode->value(0);
  mode->color(FL_RED);
  mode->callback(mode_callback, this);

  add(mode);

  Y += mode->h() + 16;

  int TY = Y;
  
#if 1
  // resize control:
  Fl_Box *resize_control = new Fl_Box(FL_NO_BOX, x(), Y, w(), 4, NULL);

  add(resize_control);
  resizable(resize_control);
#endif 

  
  // ---- bottom section ----

  Y = y() + H - 22;

  mouse_x = new Fl_Output(X+28,   Y, 72, 22, "x");
  mouse_y = new Fl_Output(X+W-72, Y, 72, 22, "y");

  mouse_x->align(FL_ALIGN_LEFT);
  mouse_y->align(FL_ALIGN_LEFT);

  add(mouse_x);
  add(mouse_y);

  Y -= mouse_x->h() + 4;

  m_label = new Fl_Box(FL_NO_BOX, X, Y, W, 22, "Mouse Coords:");
  m_label->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  add(m_label);
  
  Y -= m_label->h() + 4;

  grid_size = new Fl_Output(X+50, Y, 80, 22, "Scale:");
  grid_size->align(FL_ALIGN_LEFT);
  add(grid_size);

  Y -= grid_size->h() + 4;

  int BY = Y;


  // ---- middle section ----
 
  script_box = new UI_RadiusInfo(X-2, TY, W+4, BY-TY);

  add(script_box);
    

  // FIXME: thing_box
  

}

//
// UI_Panel Destructor
//
UI_Panel::~UI_Panel()
{
}

int UI_Panel::handle(int event)
{
  return Fl_Group::handle(event);
}

void UI_Panel::mode_callback(Fl_Widget *w, void *data)
{
  UI_Panel *me = (UI_Panel *)data;

  SYS_ASSERT(me);

  me->mode->color(me->mode->value() ? FL_CYAN : FL_RED);
}

//------------------------------------------------------------------------

void UI_Panel::SetMap(const char *name)
{
  char *upper = StrUpper(name);

  map_name->value(upper);

  StringFree(upper);
}

void UI_Panel::SetZoom(float zoom_mul)
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


void UI_Panel::SetNodeIndex(int index)
{
#if 0
  char buffer[60];

  sprintf(buffer, "%d", index);
  
  ns_index->label("Node #");
  ns_index->value(buffer);

  redraw();
#endif
}


void UI_Panel::SetMouse(double mx, double my)
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

  sprintf(x_buffer, "%1.1f", mx);
  sprintf(y_buffer, "%1.1f", my);

  mouse_x->value(x_buffer);
  mouse_y->value(y_buffer);
}


//--- editor settings ---
// vi:ts=2:sw=2:expandtab
