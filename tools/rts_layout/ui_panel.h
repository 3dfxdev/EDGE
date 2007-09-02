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

#ifndef __UI_PANEL_H__
#define __UI_PANEL_H__

class UI_ScriptInfo;
class UI_ThingInfo;


class UI_Panel : public Fl_Group
{
public:
  UI_Panel(int X, int Y, int W, int H, const char *label = NULL);
  virtual ~UI_Panel();

private:
  Fl_Output *map_name;

  Fl_Choice *mode;

  /*-----------------------*/

  UI_ScriptInfo *script_box;
  UI_ThingInfo  *thing_box;
  
  /*-----------------------*/

  Fl_Box    *m_label;
  Fl_Output *mouse_x;
  Fl_Output *mouse_y;
  Fl_Output *grid_size;

public:
  int handle(int event);
  // FLTK virtual method for handling input events.

public:
  void SetMap(const char *name);

  void SetZoom(float zoom_mul);

  void SetNodeIndex(int index);

  void SetMouse(double mx, double my);

private:
  static void mode_callback(Fl_Widget *, void *);
};

#endif // __UI_PANEL_H__

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
