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

#include "g_script.h"

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

  bool SetZoom(int new_zoom);
  // changes the current zoom factor.
  // Returns true if actually changed, otherwise false.

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
  int  handle_key();
  void handle_mouse();
  void handle_push();
  void handle_release();
  void handle_wheel(int dy);

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

  void blast_line(double x1, double y1, double x2, double y2,
                  int jx1=0, int jy1=0, int jx2=0, int jy2=0);

  void scroll(int dx, int dy);
  void scroll_to_mouse();
  void scroll_to_selection();

  void highlight_nearest(float mx, float my);

  bool inside_RAD(rad_trigger_c *RAD, float mx, float my);

  float dist_to_RAD  (rad_trigger_c *RAD, float mx, float my);
  float dist_to_THING(thing_spawn_c *TH,  float mx, float my);

  void determine_cursor(float mx = 0, float my = 0);

  void drag_new_rad_coords(rad_trigger_c *rad, float *x1, float *y1,
                           float *x2, float *y2);

private:
  level_c *map;

  section_c *script;
 
  // zoom factor: (2 ^ (zoom/2)) pixels per 512 units on the map
  int zoom;

  // derived from 'zoom'.
  double zoom_mul;

  static const int MIN_GRID_ZOOM = 3;
  static const int DEF_GRID_ZOOM = 18;  // 1:1 ratio
  static const int MAX_GRID_ZOOM = 26;

  double mid_x;
  double mid_y;

  int edit_MODE;
  int grid_MODE;

  rad_trigger_c *hilite_rad;
  thing_spawn_c *hilite_thing;

  rad_trigger_c *select_rad;
  thing_spawn_c *select_thing;

  bool dragging;

  // specify which part of the radius trigger we will drag:
  // Edges have one of dx/dy as zero. Corners have both non-zero.
  // When dx==0 and dy==0, we simply move the whole trigger.
  int drag_dx;
  int drag_dy;

  // point in world space for dragging
  float drag_mx;
  float drag_my;


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
