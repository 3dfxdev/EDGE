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

#include "g_level.h"
#include "g_script.h"

#include "ui_window.h"
#include "ui_grid.h"
#include "ui_panel.h"


//
// UI_Grid Constructor
//
UI_Grid::UI_Grid(int X, int Y, int W, int H, const char *label) : 
    Fl_Widget(X, Y, W, H, label),
    map(NULL), script(NULL),
    zoom(DEF_GRID_ZOOM), zoom_mul(1.0),
    mid_x(0), mid_y(0),
    grid_MODE(1), shade_MODE(1)
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

void UI_Grid::SetZoom(int new_zoom)
{
  if (new_zoom < MIN_GRID_ZOOM)
    new_zoom = MIN_GRID_ZOOM;

  if (new_zoom > MAX_GRID_ZOOM)
    new_zoom = MAX_GRID_ZOOM;

  if (zoom == new_zoom)
    return;

  zoom = new_zoom;

  zoom_mul = pow(2.0, (zoom / 2.0 - 9.0));

  main_win->panel->SetZoom(zoom_mul);

  //  fprintf(stderr, "Zoom %d  Mul %1.5f\n", zoom, zoom_mul);

  redraw();
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

  new_node_or_sub();  // bit hackish (calling it here)
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


#if 0
void UI_Grid::draw_partition(const node_c *nd, int ity)
{
  double mlx = mid_x - w() * 0.5 / zoom_mul;
  double mly = mid_y - h() * 0.5 / zoom_mul;
  double mhx = mid_x + w() * 0.5 / zoom_mul;
  double mhy = mid_y + h() * 0.5 / zoom_mul;

  double tlx, tly;
  double thx, thy;

  // intersect the partition line (which extends to infinity) with
  // the sides of the screen (in map coords).  Whether we use the
  // left/right sides to top/bottom depends on the angle of the
  // partition line.

  if (ABS(nd->dx) > ABS(nd->dy))
  {
    tlx = mlx;
    thx = mhx;
    tly = nd->y + nd->dy * (mlx - nd->x) / double(nd->dx);
    thy = nd->y + nd->dy * (mhx - nd->x) / double(nd->dx);

    if (MAX(tly, thy) < mly || MIN(tly, thy) > mhy)
      return;
  }
  else
  {
    tlx = nd->x + nd->dx * (mly - nd->y) / double(nd->dy);
    thx = nd->x + nd->dx * (mhy - nd->y) / double(nd->dy);
    tly = mly;
    thy = mhy;

    if (MAX(tlx, thx) < mlx || MIN(tlx, thx) > mhx)
      return;
  }

  int sx, sy;
  int ex, ey;

  MapToWin(tlx, tly, &sx, &sy);
  MapToWin(thx, thy, &ex, &ey);

  if (partition_MODE < 2)
  {
    // move vertical or horizontal lines by one pixel
    // (to prevent being clobbered by segs)
    if (ABS(nd->dx) < ABS(nd->dy))
        sx++, ex++;
    else
        sy++, ey++;
  }

  fl_color(fl_rgb_color(ity*80-70, 0, ity*80-70));
  fl_line(sx, sy, ex, ey);

  // draw arrow heads along it
  float pd_len = sqrt(nd->dx*nd->dx + nd->dy*nd->dy);
  float pdx =  nd->dx / pd_len;
  float pdy = -nd->dy / pd_len;

  for (float u=0.1; u <= 0.91; u += 0.16)
  {
    int ax = int(sx + (ex-sx) * u);
    int ay = int(sy + (ey-sy) * u);

    fl_line(ax, ay, ax + int((-pdy-pdx*1.0)*8), ay + int(( pdx-pdy*1.0)*8) );
    fl_line(ax, ay, ax + int(( pdy-pdx*1.0)*8), ay + int((-pdx-pdy*1.0)*8) );
  }
}
#endif


#if 0 // TODO
void UI_Grid::draw_script(const script_t *scr, int ity)
{
  double mlx = mid_x - w() * 0.5 / zoom_mul;
  double mly = mid_y - h() * 0.5 / zoom_mul;
  double mhx = mid_x + w() * 0.5 / zoom_mul;
  double mhy = mid_y + h() * 0.5 / zoom_mul;

  // check if bounding box is off screen

  if (bbox->maxx < mlx || bbox->minx > mhx ||
      bbox->maxy < mly || bbox->miny > mhy)
  {
      return;
  }

  int sx, sy;
  int ex, ey;

  MapToWin(bbox->minx, bbox->maxy, &sx, &sy);
  MapToWin(bbox->maxx, bbox->miny, &ex, &ey);

  if (partition_MODE < 2)
  {
    // make one pixel bigger (to prevent being clobbered by segs)
    sx--; sy--; ex++; ey++;
  }

  fl_color(fl_rgb_color(ity*50, 0, 0));

  fl_line(sx, sy, sx, ey);
  fl_line(sx, sy, ex, sy);
  fl_line(sx, ey, ex, ey);
  fl_line(ex, sy, ex, ey);
}
#endif


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
  int bright = 240;
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

  // skip triggers without a definite size
  if (RAD->rx < 0 || RAD->ry < 0)
    return;
 
  if (RAD->is_rect)
    fl_color(FL_RED);
  else
    fl_color(FL_GREEN);

  float x1 = RAD->mx - RAD->rx;
  float y1 = RAD->my - RAD->ry;
  float x2 = RAD->mx + RAD->rx;
  float y2 = RAD->my + RAD->ry;

  blast_line(x1, y1, x1, y2);
  blast_line(x2, y1, x2, y2);
  blast_line(x1, y1, x2, y1);
  blast_line(x1, y2, x2, y2);
}

void UI_Grid::draw_thing(const thing_spawn_c *TH)
{
  SYS_ASSERT(TH);

  // TODO
}


void UI_Grid::blast_line(double x1, double y1, double x2, double y2)
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
      int result = handle_key(Fl::event_key());
      handle_mouse(Fl::event_x(), Fl::event_y());
      return result;
    }

    case FL_ENTER:
    case FL_LEAVE:
      return 1;

    case FL_MOVE:
      handle_mouse(Fl::event_x(), Fl::event_y());
      return 1;

    case FL_PUSH:
      if (Fl::focus() != this)
      {
        Fl::focus(this);
        handle(FL_FOCUS);
        return 1;
      }

      // TODO
      // redraw();
      return 1;

    case FL_MOUSEWHEEL:
      if (Fl::event_dy() < 0)
        SetZoom(zoom + 1);
      else if (Fl::event_dy() > 0)
        SetZoom(zoom - 1);

      handle_mouse(Fl::event_x(), Fl::event_y());
      return 1;

    case FL_DRAG:
    case FL_RELEASE:
      // these are currently ignored.
      return 1;

    default:
      break;
  }

  return 0;  // unused
}

int UI_Grid::handle_key(int key)
{
  if (key == 0)
    return 0;

  switch (key)
  {
    case '+': case '=':
      SetZoom(zoom + 1);
      return 1;

    case '-': case '_':
      SetZoom(zoom - 1);
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

    case 'g': case 'G':
      grid_MODE = (grid_MODE + 1) % 2;
      redraw();
      return 1;

    case 's': case 'S':
      shade_MODE = (shade_MODE + 1) % 2;
      redraw();
      return 1;

    default:
      break;
  }

  return 0;  // unused
}

void UI_Grid::handle_mouse(int wx, int wy)
{
  if (! main_win)
    return;

  double mx, my;

  WinToMap(wx, wy, &mx, &my);

  main_win->panel->SetMouse(mx, my);
}

void UI_Grid::new_node_or_sub(void)
{
#if 0
  node_c *cur_nd;
  subsec_c *cur_sub;
  bbox_t *cur_bbox;
  
  lowest_node(&cur_nd, &cur_sub, &cur_bbox);

  if (cur_sub)
  {
    guix_win->info->BeginSegList();
    
    for (seg_c *seg = cur_sub->seg_list; seg; seg = seg->next)
      guix_win->info->AddSeg(seg);
    
    guix_win->info->EndSegList();

    guix_win->info->SetSubsectorIndex(cur_sub->index);
    guix_win->info->SetPartition(NULL);
  }
  else
  {
    guix_win->info->SetNodeIndex(cur_nd->index);
    guix_win->info->SetPartition(cur_nd);
  }

  guix_win->info->SetCurBBox(cur_bbox); // NULL is OK
#endif
}


//--- editor settings ---
// vi:ts=2:sw=2:expandtab
