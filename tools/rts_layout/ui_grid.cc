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

#include "headers.h"
#include "hdr_fltk.h"

#include "lib_util.h"
#include "g_level.h"
#include "g_script.h"

#include "ui_window.h"
#include "ui_grid.h"
#include "ui_panel.h"
#include "ui_radius.h"
#include "ui_thing.h"


// the normal 'MOVE' cursor looks like shite on Linux.
// instead we use the X11 'PLUS' cursor.
#ifdef UNIX
#undef  FL_CURSOR_MOVE
#define FL_CURSOR_MOVE  ((Fl_Cursor)46)  // XC_plus
#endif

#ifdef UNIX
#define CURSOR_NO_FOCUS  ((Fl_Cursor)1)  // XC_X_cursor
#endif


//
// UI_Grid Constructor
//
UI_Grid::UI_Grid(int X, int Y, int W, int H, const char *label) : 
    Fl_Widget(X, Y, W, H, label),
    map(NULL), script(NULL),
    zoom(DEF_GRID_ZOOM), zoom_mul(1.0),
    mid_x(0), mid_y(0),
    edit_MODE(EDIT_RadTrig), grid_MODE(1),
    hilite_rad(NULL), hilite_thing(NULL),
    select_rad(NULL), select_thing(NULL),
    dragging(false), drag_dx(0), drag_dy(0)
{ }

//
// UI_Grid Destructor
//
UI_Grid::~UI_Grid()
{ }


void UI_Grid::SetMap(level_c *new_map)
{
  map = new_map;

  if (map)
  {
    double lx,ly, hx,hy;

    map->ComputeBounds(&lx,&ly, &hx,&hy);

    FitBBox(lx,ly, hx,hy);
  }

  redraw();
}

void UI_Grid::SetScript(section_c *new_scr)
{
  script = new_scr;

  // TODO: if (new_scr && !map) FitBBox(...)

  redraw();
}

bool UI_Grid::SetZoom(int new_zoom)
{
  if (new_zoom < MIN_GRID_ZOOM)
    new_zoom = MIN_GRID_ZOOM;

  if (new_zoom > MAX_GRID_ZOOM)
    new_zoom = MAX_GRID_ZOOM;

  if (zoom == new_zoom)
    return false;

  zoom = new_zoom;

  zoom_mul = pow(2.0, (zoom / 2.0 - 9.0));

  main_win->panel->SetZoom(zoom_mul);

  //  fprintf(stderr, "Zoom %d  Mul %1.5f\n", zoom, zoom_mul);

  redraw();

  return true;
}


void UI_Grid::SetPos(double new_x, double new_y)
{
  mid_x = new_x;
  mid_y = new_y;

  redraw();
}

void UI_Grid::FitBBox(double lx, double ly, double hx, double hy)
{
  double dx = hx - lx;
  double dy = hy - ly;

  zoom = MAX_GRID_ZOOM;

  for (; zoom > MIN_GRID_ZOOM; zoom--)
  {
    zoom_mul = pow(2.0, (zoom / 2.0 - 9.0));

    if (dx * zoom_mul < w() && dy * zoom_mul < h())
      break;
  }

  zoom_mul = pow(2.0, (zoom / 2.0 - 9.0));

  main_win->panel->SetZoom(zoom_mul);

  SetPos(lx + dx / 2.0, ly + dy / 2.0);
}

void UI_Grid::MapToWin(double mx, double my, int *X, int *Y) const
{
  double hx = x() + w() / 2.0;
  double hy = y() + h() / 2.0;

  (*X) = I_ROUND(hx + (mx - mid_x) * zoom_mul);
  (*Y) = I_ROUND(hy - (my - mid_y) * zoom_mul);
}

void UI_Grid::WinToMap(int X, int Y, double *mx, double *my) const
{
  double hx = x() + w() / 2.0;
  double hy = y() + h() / 2.0;

  (*mx) = mid_x + (X - hx) / zoom_mul;
  (*my) = mid_y - (Y - hy) / zoom_mul;
}

void UI_Grid::SetEditMode(int new_mode)
{
  edit_MODE = new_mode;

  hilite_rad   = select_rad   = NULL;
  hilite_thing = select_thing = NULL;

  determine_cursor();
  update_active_obj();
 
  redraw();
}


//------------------------------------------------------------------------

void UI_Grid::resize(int X, int Y, int W, int H)
{
  Fl_Widget::resize(X, Y, W, H);
}

void UI_Grid::draw()
{
  fl_push_clip(x(), y(), w(), h());

  fl_color(FL_BLACK);
  fl_rectf(x(), y(), w(), h());

                                //  3456789012345678901234567890
//static const char *ity_2_to_N3 = "-------------------------233";
  static const char *ity_2_to_0  = "-------------------334455555";
  static const char *ity_2_to_3  = "-------------334455667777777";
  static const char *ity_2_to_6  = "-------33445566778899999----";
  static const char *ity_2_to_9  = "-33445566778899999----------";
  static const char *ity_2_to_12 = "566778899999----------------";
  static const char *ity_2_to_15 = "999999----------------------";

//draw_grid(0.125,   ity_2_to_N3[zoom - 3]);
  draw_grid(1.0,     ity_2_to_0 [zoom - 3]);
  draw_grid(8.0,     ity_2_to_3 [zoom - 3]);
  draw_grid(64.0,    ity_2_to_6 [zoom - 3]);
  draw_grid(512.0,   ity_2_to_9 [zoom - 3]);
  draw_grid(4096.0,  ity_2_to_12[zoom - 3]);
  draw_grid(32768.0, ity_2_to_15[zoom - 3]);


  if (map)
    draw_map();

  if (script)
    draw_goodies();


  fl_pop_clip();
}

void UI_Grid::draw_grid(double spacing, int ity)
{
  if (grid_MODE == 0)
    return;

  if (! isdigit(ity))
    return;

  ity = MIN(ity - '0', 9);

  fl_color(fl_rgb_color(0, 0, 224 * ity / 9));

  double mlx = mid_x - w() * 0.5 / zoom_mul;
  double mly = mid_y - h() * 0.5 / zoom_mul;
  double mhx = mid_x + w() * 0.5 / zoom_mul;
  double mhy = mid_y + h() * 0.5 / zoom_mul;

  int gx = GRID_FIND(mid_x, spacing);
  int gy = GRID_FIND(mid_y, spacing);

  int x1 = x();
  int y1 = y();
  int x2 = x() + w();
  int y2 = y() + h();

  int dx, dy;
  int wx, wy;

  for (dx = 0; true; dx++)
  {
    double xx = gx + dx * spacing;

    if (xx > mhx) break;
    if (xx < mlx) continue;

    MapToWin(xx, gy, &wx, &wy);
    fl_yxline(wx, y1, y2);
  }

  for (dx = -1; true; dx--)
  {
    double xx = gx + dx * spacing;

    if (xx < mlx) break;
    if (xx > mhx) continue;

    MapToWin(xx, gy, &wx, &wy);
    fl_yxline(wx, y1, y2);
  }

  for (dy = 0; true; dy++)
  {
    double yy = gy + dy * spacing;

    if (yy > mhy) break;
    if (yy < mly) continue;

    MapToWin(gx, yy, &wy, &wy);
    fl_xyline(x1, wy, x2);
  }

  for (dy = -1; true; dy--)
  {
    double yy = gy + dy * spacing;

    if (yy < mly) break;
    if (yy > mhy) continue;

    MapToWin(gx, yy, &wy, &wy);
    fl_xyline(x1, wy, x2);
  }
}


void UI_Grid::draw_map()
{
  std::vector<linedef_c *>::iterator LI;

  for (LI = map->lines.begin(); LI != map->lines.end(); LI++)
  {
    linedef_c *L = *LI;
    SYS_ASSERT(L);

    draw_linedef(L);
  }
}

void UI_Grid::draw_linedef(const linedef_c *ld)
{
  int bright = 230;
  int middle = 190;
  int dark   = 130;
 
  if (! ld->left  || ! ld->left->sector ||
      ! ld->right || ! ld->right->sector)
  {
    // one-sided line
    fl_color(fl_rgb_color(bright));
  }
  else
  {
    sector_c *front = ld->right->sector;
    sector_c *back  = ld->left->sector;

    int f_min = MIN(front->floor_h, back->floor_h);
    int f_max = MAX(front->floor_h, back->floor_h);

    int c_min = MIN(front->ceil_h, back->ceil_h);

    if (c_min <= f_max)
    {
      // closed door
      fl_color(fl_rgb_color(bright));
    }
    else if (c_min - f_max < 56)
    {
      // narrow vertical gap
      fl_color(fl_rgb_color(middle));
    }
    else if (f_max - f_min > 24)
    {
      // unclimable drop-off
      fl_color(fl_rgb_color(middle));
    }
    else
    {
      // everything else
      fl_color(fl_rgb_color(dark));
    }
  }

  blast_line(ld->start->x, ld->start->y, ld->end->x, ld->end->y);
}


void UI_Grid::draw_goodies()
{
  std::vector<section_c *>::iterator PI;

  for (PI = script->pieces.begin(); PI != script->pieces.end(); PI++)
  {
    if (*PI)
    {
      section_c *piece = *PI;

      if (piece->kind == section_c::RAD_TRIG)
        draw_trigger(piece->trig);
    }
  }
}

void UI_Grid::draw_trigger(rad_trigger_c *RAD)
{
  SYS_ASSERT(RAD);

  if (RAD->worldspawn)
  {
    std::vector<thing_spawn_c *>::iterator TI;

    for (TI = RAD->things.begin(); TI != RAD->things.end(); TI++)
    {
      if (*TI)
        draw_thing(*TI);
    }

    return;
  }

  if (edit_MODE != EDIT_RadTrig)
    return;

  // skip triggers without a definite size
  if (RAD->rx < 0 || RAD->ry < 0)
    return;
 
  bool hilite = (hilite_rad == RAD); 

  if (select_rad == RAD)
    fl_color(FL_YELLOW);
  else if (RAD->is_rect)
    fl_color(fl_rgb_color(hilite?160:0, 255, hilite?160:0));
  else
    fl_color(fl_rgb_color(255, hilite?130:0, hilite?130:0));

  float x1 = RAD->mx - RAD->rx;
  float y1 = RAD->my - RAD->ry;
  float x2 = RAD->mx + RAD->rx;
  float y2 = RAD->my + RAD->ry;

  if (select_rad == RAD && dragging)
  {
    drag_new_rad_coords(RAD, &x1, &y1, &x2, &y2);
  }

  blast_line(x1, y1, x1, y2);
  blast_line(x2, y1, x2, y2);
  blast_line(x1, y1, x2, y1);
  blast_line(x1, y2, x2, y2);

  if (hilite)
  {
    blast_line(x1, y1, x1, y2, -1, +1, -1, -1);
    blast_line(x2, y1, x2, y2, +1, +1, +1, -1);
    blast_line(x1, y1, x2, y1, -1, +1, +1, +1);
    blast_line(x1, y2, x2, y2, -1, -1, +1, -1);
  }
}

void UI_Grid::draw_thing(const thing_spawn_c *TH)
{
  SYS_ASSERT(TH);

  if (edit_MODE != EDIT_Things)
    return;

  float r = 20.0;
  // r = TH->ddf_info->radius;

  bool hilite = (hilite_thing == TH); 

  if (select_thing == TH)
    fl_color(FL_YELLOW);
//else if (TH->ddf_info && TH->ddf_info->is_monster)
//  fl_color(fl_rgb_color(255, hilite?160:0, 255));
//else if (TH->ddf_info && TH->ddf_info->is_pickup)
//  fl_color(fl_rgb_color(hilite?160:0, 255, hilite?160:0));
  else
    fl_color(fl_rgb_color(hilite?160:0, hilite?230:255, 255));

  float x = TH->x;
  float y = TH->y;

  if (select_thing == TH && dragging)
  {
    x = drag_mx;
    y = drag_my;
  }

  float x1 = x - r;
  float y1 = y - r;
  float x2 = x + r;
  float y2 = y + r;

  blast_line(x1, y1, x1, y2);
  blast_line(x2, y1, x2, y2);
  blast_line(x1, y1, x2, y1);
  blast_line(x1, y2, x2, y2);

  if (hilite)
  {
    blast_line(x1, y1, x1, y2, -1, +1, -1, -1);
    blast_line(x2, y1, x2, y2, +1, +1, +1, -1);
    blast_line(x1, y1, x2, y1, -1, +1, +1, +1);
    blast_line(x1, y2, x2, y2, -1, -1, +1, -1);
  }

  // draw direction
 
  float dx = cos(TH->angle * M_PI / 180.0);
  float dy = sin(TH->angle * M_PI / 180.0);

  // convert circle to square
  if (fabs(dx) > fabs(dy))
  {
    dy /= fabs(dx);
    dx /= fabs(dx);
  }
  else
  {
    dx /= fabs(dy);
    dy /= fabs(dy);
  }

  dx *= r; dy *= r;

  blast_line(TH->x, TH->y, TH->x + dx, TH->y + dy);
}


void UI_Grid::blast_line(double x1, double y1, double x2, double y2,
                  int jx1, int jy1, int jx2, int jy2)
{
  double mlx = mid_x - w() * 0.5 / zoom_mul;
  double mly = mid_y - h() * 0.5 / zoom_mul;
  double mhx = mid_x + w() * 0.5 / zoom_mul;
  double mhy = mid_y + h() * 0.5 / zoom_mul;

  // Based on Cohen-Sutherland clipping algorithm

  int out1 = MAP_OUTCODE(x1, y1, mlx, mly, mhx, mhy);
  int out2 = MAP_OUTCODE(x2, y2, mlx, mly, mhx, mhy);

/// PrintDebug("LINE (%1.3f,%1.3f) --> (%1.3f,%1.3f)\n", x1, y1, x2, y2);
/// PrintDebug("RECT (%1.3f,%1.3f) --> (%1.3f,%1.3f)\n", mlx, mly, mhx, mhy);
/// PrintDebug("  out1 = %d  out2 = %d\n", out1, out2);

  while ((out1 & out2) == 0 && (out1 | out2) != 0)
  {
/// PrintDebug("> LINE (%1.3f,%1.3f) --> (%1.3f,%1.3f)\n", x1, y1, x2, y2);
/// PrintDebug(">   out1 = %d  out2 = %d\n", out1, out2);

    // may be partially inside box, find an outside point
    int outside = (out1 ? out1 : out2);

    SYS_ZERO_CHECK(outside);

    double dx = x2 - x1;
    double dy = y2 - y1;

    if (fabs(dx) < 0.1 && fabs(dy) < 0.1)
      break;

    double tmp_x, tmp_y;

    // clip to each side
    if (outside & O_BOTTOM)
    {
      tmp_x = x1 + dx * (mly - y1) / dy;
      tmp_y = mly;
    }
    else if (outside & O_TOP)
    {
      tmp_x = x1 + dx * (mhy - y1) / dy;
      tmp_y = mhy;
    }
    else if (outside & O_LEFT)
    {
      tmp_y = y1 + dy * (mlx - x1) / dx;
      tmp_x = mlx;
    }
    else  /* outside & O_RIGHT */
    {
      SYS_ASSERT(outside & O_RIGHT);

      tmp_y = y1 + dy * (mhx - x1) / dx;
      tmp_x = mhx;
    }

/// PrintDebug(">   outside = %d  temp = (%1.3f, %1.3f)\n", tmp_x, tmp_y);
    SYS_ASSERT(out1 != out2);

    if (outside == out1)
    {
      x1 = tmp_x;
      y1 = tmp_y;

      out1 = MAP_OUTCODE(x1, y1, mlx, mly, mhx, mhy);
    }
    else
    {
      SYS_ASSERT(outside == out2);

      x2 = tmp_x;
      y2 = tmp_y;

      out2 = MAP_OUTCODE(x1, y1, mlx, mly, mhx, mhy);
    }
  }

  if (out1 & out2)
    return;

  int sx, sy;
  int ex, ey;

  MapToWin(x1, y1, &sx, &sy);
  MapToWin(x2, y2, &ex, &ey);

  sx += jx1; sy += jy1;
  ex += jx2; ey += jy2;
  
  fl_line(sx, sy, ex, ey);
}

#if 0
void UI_Grid::draw_path()
{
  int p;

  // first, render the lines
  fl_color(fl_color_cube(0,7,4));

  for (p = 0; p < path->point_num - 1; p++)
  {
    int x1, y1, x2, y2;

    path->GetPoint(p,   &x1, &y1);
    path->GetPoint(p+1, &x2, &y2);

    draw_line(x1, y1, x2, y2);
  }

  // second, render the points themselves
  fl_color(FL_YELLOW);

  for (p = 0; p < path->point_num; p++)
  {
    int mx, my;
    int wx, wy;

    path->GetPoint(p, &mx, &my);

    MapToWin(mx, my, &wx, &wy);

    fl_rect(wx-1, wy-1, 3, 3);
  }
}
#endif

void UI_Grid::scroll(int dx, int dy)
{
  dx = dx * w() / 10;
  dy = dy * h() / 10;

  double mdx = dx / zoom_mul;
  double mdy = dy / zoom_mul;

  mid_x += mdx;
  mid_y += mdy;

// fprintf(stderr, "Scroll pix (%d,%d) map (%1.1f, %1.1f) mid (%1.1f, %1.1f)\n", dx, dy, mdx, mdy, mid_x, mid_y);

  redraw();
}

//------------------------------------------------------------------------

int UI_Grid::handle(int event)
{
  switch (event)
  {
    case FL_FOCUS:
      return 1;

    case FL_KEYDOWN:
    case FL_SHORTCUT:
    {
      int result = handle_key();
      handle_mouse();

      return result;
    }

    case FL_ENTER:
      // we greedily grab the focus
      if (Fl::focus() != this)
        take_focus(); 

      return 1;

    case FL_LEAVE:
      hilite_rad = NULL;
      hilite_thing = NULL;
      dragging = false;
      determine_cursor();
      update_active_obj();
      redraw();
      return 1;

    case FL_MOVE:
      handle_mouse();
      return 1;

    case FL_PUSH:
      handle_push();
      return 1;

    case FL_MOUSEWHEEL:
      handle_wheel(- SGN(Fl::event_dy()));
      handle_mouse();
      return 1;

    case FL_DRAG:
      dragging = true;
      handle_mouse();
      return 1;

    case FL_RELEASE:
      handle_release();
      return 1;

    default:
      break;
  }

  return 0;  // unused
}

int UI_Grid::handle_key()
{
  int key   = Fl::event_key();
  int state = Fl::event_state();

  bool ascii = true;

  if (state & (FL_CTRL | FL_ALT | FL_META))
    ascii = false;

  if (key < 32 || key > 127)
    ascii = false;
  
  if (ascii) switch(tolower(key))
  {
    case '+': case '=':
      handle_wheel(+1);
      return 1;

    case '-': case '_':
      handle_wheel(-1);
      return 1;

    case 'g':
      grid_MODE = (grid_MODE + 1) % 2;
      redraw();
      return 1;

    case 'c':
      scroll_to_mouse();
      return 1;

    case 'f':
      scroll_to_selection();
      return 1;

    case '0': // zoom out
    {
      double lx,ly, hx,hy;
      map->ComputeBounds(&lx,&ly, &hx,&hy);

      FitBBox(lx,ly, hx,hy);
      return 1;
    }

    default:
      // swallow all letters and digits
      return 1;
  }

  switch (key)
  {
    case FL_Escape:
      if (dragging)
      {
        dragging = false;
        select_rad = NULL;
        select_thing = NULL;
        determine_cursor();
        update_active_obj();
        redraw();
      }
      return 1;

    case FL_Left:
      scroll(-1, 0);
      return 1;

    case FL_Right:
      scroll(+1, 0);
      return 1;

    case FL_Up:
      scroll(0, +1);
      return 1;

    case FL_Down:
      scroll(0, -1);
      return 1;

    default: break;
  }

  return 0;
}

void UI_Grid::handle_mouse()
{
  if (! main_win)
    return;

  int wx = Fl::event_x();
  int wy = Fl::event_y();

  double mx, my;

  WinToMap(wx, wy, &mx, &my);

  main_win->panel->SetMouse(mx, my);

  if (dragging)
  {
    drag_mx = mx;
    drag_my = my;

    if (select_rad || select_thing)
      redraw();

    return;
  }

  if (script)
  {
    highlight_nearest(mx, my);
  }

  determine_cursor(mx, my);
}

void UI_Grid::handle_push()
{
  if (Fl::focus() != this)
  {
    take_focus();
    return;
  }

  if (select_rad != hilite_rad || select_thing != hilite_thing)
  {
    select_rad   = hilite_rad;
    select_thing = hilite_thing;

    update_active_obj();
    redraw();
  }
}

void UI_Grid::handle_release()
{
  if (dragging)
  {
    /* DO STUFF */
    dragging = false;
    redraw();
  }
}

void UI_Grid::handle_wheel(int dy)
{
  if (dy == 0)
    return;

  if (Fl::belowmouse() != this)
  {
#if 0
    SetZoom(zoom + dy);
#endif
    return;
  }

  // following code for zooming 'around' the mouse pointer
  // (i.e. map coordinate at pointer remains the same).

  int wx = Fl::event_x();
  int wy = Fl::event_y();

  double mx1, my1;
  WinToMap(wx, wy, &mx1, &my1);

  if (! SetZoom(zoom + dy))
    return;

  double mx2, my2;
  WinToMap(wx, wy, &mx2, &my2);

  mid_x -= mx2 - mx1;
  mid_y -= my2 - my1;
}

void UI_Grid::scroll_to_mouse()
{
  if (Fl::belowmouse() != this)
    return;
 
  int wx = Fl::event_x();
  int wy = Fl::event_y();

  double mx, my;
  WinToMap(wx, wy, &mx, &my);

  SetPos(mx, my);
}

void UI_Grid::scroll_to_selection()
{
  if (select_rad)
  {
    SetPos(select_rad->mx, select_rad->my);
  }
  else if (select_thing)
  {
    SetPos(select_thing->x, select_thing->y);
  }
}

void UI_Grid::highlight_nearest(float mx, float my)
{
  rad_trigger_c *new_rad   = NULL;
  thing_spawn_c *new_thing = NULL;

  float best_dist = 99999.9;

  std::vector<section_c *>::iterator PI;

  for (PI = script->pieces.begin(); PI != script->pieces.end(); PI++)
  {
    section_c *piece = *PI;

    if (! piece)
      continue;

    if (piece->kind == section_c::RAD_TRIG)
    {
      rad_trigger_c *RAD = piece->trig;
      SYS_ASSERT(RAD);

      if (edit_MODE == EDIT_RadTrig && RAD->rx > 0 && inside_RAD(RAD, mx, my))
      {
        float dist = dist_to_RAD(RAD, mx, my);

        if (! new_rad || dist < best_dist)
        {
          new_rad = RAD;
          best_dist = dist;
        }
      }
      else if (edit_MODE == EDIT_Things && RAD->worldspawn)
      {
        std::vector<thing_spawn_c *>::iterator TI;

        for (TI = RAD->things.begin(); TI != RAD->things.end(); TI++)
        {
          thing_spawn_c *thing = *TI;

          if (! thing)
            continue;

          float r = 20.0; // r = TH->ddf_info->radius;

          float dist = dist_to_THING(thing, mx, my);

          if (dist/1.6 > r)
            continue;

          if (! new_thing || dist < best_dist)
          {
            new_thing = thing;
            best_dist = dist;
          }
        }
      }
    }
  }

  /* see if anything changed */

  switch (edit_MODE)
  {
    case EDIT_RadTrig:
      if (new_rad != hilite_rad)
      {
        hilite_rad = new_rad;
        determine_cursor();
        update_active_obj();
        redraw();
      }
      break;

    case EDIT_Things:
      if (new_thing != hilite_thing)
      {
        hilite_thing = new_thing;
        determine_cursor();
        update_active_obj();
        redraw();
      }
      break;

    default: break;
  }
}

bool UI_Grid::inside_RAD(rad_trigger_c *RAD, float mx, float my)
{
  float x1 = RAD->mx - RAD->rx * 1.2;
  float y1 = RAD->my - RAD->ry * 1.2;
  float x2 = RAD->mx + RAD->rx * 1.2;
  float y2 = RAD->my + RAD->ry * 1.2;

  return ! (mx < x1 || mx > x2 || my < y1 || my > y2);
}

float UI_Grid::dist_to_RAD(rad_trigger_c *RAD, float mx, float my)
{
  return MAX(fabs(mx - RAD->mx), fabs(my - RAD->my));
}

float UI_Grid::dist_to_THING(thing_spawn_c *TH,  float mx, float my)
{
  mx -= TH->x;
  my -= TH->y;

  return sqrt(mx*mx + my*my);
}

void UI_Grid::determine_cursor(float mx, float my)
{
  // keep same cursor when dragging
  if (dragging)
    return;

#if 0
  if (hilite_thing)
  {
    main_win->SetCursor(FL_CURSOR_MOVE);
    return;
  }
#endif

  if (! hilite_rad)
  {
    main_win->SetCursor(FL_CURSOR_DEFAULT);
    return;
  }

  rad_trigger_c *RAD = hilite_rad;

  float dx = mx - RAD->mx;
  float dy = my - RAD->my;

  float fx = fabs(dx) / RAD->rx;
  float fy = fabs(dy) / RAD->ry;

  if (fx < 0.66 && fy < 0.66)
  {
    drag_dx = drag_dy = 0;

    main_win->SetCursor(FL_CURSOR_MOVE);
  }
  else
  {
    float slope = MIN(fx,fy) / MAX(fx,fy);

    if (slope > 0.8)
    {
      drag_dx = (dx > 0) ? +1 : -1;
      drag_dy = (dy > 0) ? +1 : -1;

      if (drag_dx * drag_dy > 0)
        main_win->SetCursor(FL_CURSOR_NESW);
      else
        main_win->SetCursor(FL_CURSOR_NWSE);
    }
    else if (fx > fy)
    {
      drag_dx = (dx > 0) ? +1 : -1;
      drag_dy = 0;

      main_win->SetCursor(FL_CURSOR_WE);
    }
    else
    {
      drag_dx = 0;
      drag_dy = (dy > 0) ? +1 : -1;

      main_win->SetCursor(FL_CURSOR_NS);
    }
  }
}

void UI_Grid::drag_new_rad_coords(rad_trigger_c *RAD,
      float *x1, float *y1, float *x2, float *y2)
{
  if (drag_dx == 0 && drag_dy == 0)
  {
    *x1 = drag_mx - RAD->rx;
    *y1 = drag_my - RAD->ry;
    *x2 = drag_mx + RAD->rx;
    *y2 = drag_my + RAD->ry;
    return;
  }

  if (! RAD->is_rect)
  {
    // Radius trigger : all edges act the same
    float dist = MAX(fabs(RAD->mx - drag_mx), fabs(RAD->my - drag_my));

    *x1 = RAD->mx - dist;
    *y1 = RAD->my - dist;
    *x2 = RAD->mx + dist;
    *y2 = RAD->my + dist;
    return;
  }

  // Rectangle trigger
  
  if (drag_dx < 0) *x1 = drag_mx;
  if (drag_dx > 0) *x2 = drag_mx;
  
  if (drag_dy < 0) *y1 = drag_my;
  if (drag_dy > 0) *y2 = drag_my;

  if (*x1 > *x2)
  {
    float tmp = *x1; *x1 = *x2; *x2 = tmp;
  }

  if (*y1 > *y2)
  {
    float tmp = *y1; *y1 = *y2; *y2 = tmp;
  }
}

void UI_Grid::update_active_obj()
{
  if (! main_win)
    return;

  main_win->panel->script_box->SetViewRad(
      select_rad ? select_rad : hilite_rad);

  main_win->panel->thing_box->SetViewThing(
      select_thing ? select_thing : hilite_thing);
}

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
