//------------------------------------------------------------------------
//  GRID : Draws everything on the map
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

#ifndef __UI_GRID_H__
#define __UI_GRID_H__


class linedef_c;
class level_c;

class rad_trigger_c;
class thing_spawn_c;
class section_c;
class script_c;


class UI_Grid : public Fl_Widget
{
public:
  UI_Grid(int X, int Y, int W, int H, const char *label = NULL);
  virtual ~UI_Grid();

  void SetMap(level_c *new_map);
  // set or change the current map we are viewing.  The 'new_map'
  // parameter can be NULL to disable map view (which is the
  // default state).  Calling this will cause the grid to zoom
  // so that it fits the whole map (i.e. FitBBox is invoked).

  void SetScript(section_c *new_scr);
  // set or change the current script we are viewing.
  // The 'new_scr' can be NULL to disable script display
  // (triggers and spawn-things).

  void SetZoom(int new_zoom);
  // changes the current zoom factor.

  void SetPos(double new_x, double new_y);
  // changes the current position.

  void FitBBox(double lx, double ly, double hx, double hy);
  // set zoom and position so that the bounding area fits.

  void MapToWin(double mx, double my, int *X, int *Y) const;
  // convert a map coordinate into a window coordinate, using
  // current grid position and zoom factor.

  void WinToMap(int X, int Y, double *mx, double *my) const;
  // convert a map coordinate into a window coordinate, using
  // current grid position and zoom factor.

  enum edit_mode_kind_e
  {
    EDIT_RadTrig = 0,
    EDIT_Things  = 1,
  };

  void SetEditMode(int new_mode);
 
public:
  int handle(int event);
  // FLTK virtual method for handling input events.

  void resize(int X, int Y, int W, int H);
  // FLTK virtual method for resizing.

private:
  void draw();
  // FLTK virtual method for drawing.

  void draw_grid(double spacing, int ity);

  void draw_map();
  void draw_linedef(const linedef_c *ld);

  enum state_foo_e
  {
    STATE_Off = 0,
    STATE_Normal,
    STATE_Highlighted,
    STATE_Selected
  };
 
  void draw_goodies();
  void draw_trigger(rad_trigger_c *RAD);
  void draw_thing(const thing_spawn_c *TH);

  void blast_line(double x1, double y1, double x2, double y2);

  void scroll(int dx, int dy);

  void new_node_or_sub(void);

public:
  int handle_key(int key);

  void handle_mouse(int wx, int wy);

private:
  level_c *map;

  section_c *script;
 
  int zoom;
  // zoom factor: (2 ^ (zoom/2)) pixels per 512 units on the map

  double zoom_mul;
  // derived from 'zoom'.

  static const int MIN_GRID_ZOOM = 3;
  static const int DEF_GRID_ZOOM = 18;  // 1:1 ratio
  static const int MAX_GRID_ZOOM = 26;

  double mid_x;
  double mid_y;

  int edit_MODE;
  int grid_MODE;


  static inline int GRID_FIND(double x, double y)
  {
    return int(x - fmod(x,y) + (x < 0) ? y : 0);
  }

  static const int O_TOP    = 1;
  static const int O_BOTTOM = 2;
  static const int O_LEFT   = 4;
  static const int O_RIGHT  = 8;

  static int MAP_OUTCODE(double x, double y,
      double lx, double ly, double hx, double hy)
  {
    return
      ((y < ly) ? O_BOTTOM : 0) |
      ((y > hy) ? O_TOP    : 0) |
      ((x < lx) ? O_LEFT   : 0) |
      ((x > hx) ? O_RIGHT  : 0);
  }

};

#endif // __UI_GRID_H__

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
