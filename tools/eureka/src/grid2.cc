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
#include "grid2.h"

#include "ui_window.h"


Grid_State_c grid;


Grid_State_c::Grid_State_c() :
     shown(true), snap(true), locked(false),
     pixels_min(10),
     orig_x(0), orig_y(0), Scale(1.0), step(128)
{
}

Grid_State_c::~Grid_State_c()
{
}


void Grid_State_c::CenterMapAt(int x, int y)
{
  if (orig_x != x || orig_y != y)
  {
    orig_x = x;
    orig_y = y;

    if (main_win)
      main_win->canvas->redraw();
  }
}


int Grid_State_c::SnapX(int mapx) const
{
  if (! snap || grid.step == 0)
    return mapx;

  if (mapx >= 0)
    return grid.step * ((mapx + grid.step / 2) / grid.step);
  else
    return grid.step * ((mapx - grid.step / 2) / grid.step);
}


int Grid_State_c::SnapY(int mapy) const
{
  if (! snap || grid.step == 0)
    return mapy;

  if (mapy >= 0)
    return grid.step * ((mapy + grid.step / 2) / grid.step);
  else
    return grid.step * ((mapy - grid.step / 2) / grid.step);
}


void Grid_State_c::RefocusZoom(int map_x, int map_y, float before_Scale)
{
  float dist_factor = (1.0 - before_Scale / Scale);

  orig_x += I_ROUND((map_x - orig_x) * dist_factor);
  orig_y += I_ROUND((map_y - orig_y) * dist_factor);

  if (main_win)
    main_win->canvas->redraw();
}


const double Grid_State_c::scale_values[] =
{
  16.0, 8.0, 6.0, 4.0, 3.0, 2.0, 1.5, 1.0,

  1.0 / 1.5, 1.0 / 2.0, 1.0 / 3.0,  1.0 / 4.0,
  1.0 / 6.0, 1.0 / 8.0, 1.0 / 16.0, 1.0 / 32.0,
  1.0 / 64.0
};

const int Grid_State_c::digit_scales[] =
{
  1, 3, 5, 7, 9, 11, 13, 14, 15  /* index into scale_values[] */
};

const int Grid_State_c::grid_values[] =
{
  -1 /* OFF */,

  512, 256, 128, 64, 32, 16, 8, 4, 2
};

#define NUM_SCALE_VALUES  17
#define NUM_GRID_VALUES   10


const char *Grid_State_c::scale_options()
{
  return "x 16|x 8.0|x 6.0|x 4.0|x 3.0|x 2.0|x 1.5|"
         "-100%-|"
         "/ 1.5|/ 2.0|/ 3.0|/ 4.0|/ 6.0|/ 8.0|/ 16|/ 32|/ 64"; 
}

const char *Grid_State_c::grid_options()
{
  return "OFF|"
         "512|256|128| 64| 32| 16|  8|  4|  2";
}


void Grid_State_c::ScaleFromWidget(int i)
{
//!!!!  SYS_ASSERT(0 <= i && i < NUM_SCALE_VALUES)

  Scale = scale_values[i];

  if (main_win)
    main_win->canvas->redraw();
}

void Grid_State_c::StepFromWidget(int i)
{
//!!!!  SYS_ASSERT(0 <= i && i < NUM_GRID_VALUES)

  step = grid_values[i];

  if (shown)
    if (main_win)
      main_win->canvas->redraw();
}


void Grid_State_c::StepFromScale()
{
  if (locked)
    return;

  int result = 1;

  for (int i = 1; i < NUM_GRID_VALUES; i++)
  {
    result = i;

    if (grid_values[i] * Scale / 2 < pixels_min)
      break;
  }

  if (step == grid_values[result])
    return; // no change


  step = grid_values[result];

  if (main_win)
  {
    main_win->info_bar->SetGrid(result);
    main_win->canvas->redraw();
  }
}


void Grid_State_c::AdjustStep(int delta)
{
  int result = -1;

  if (delta > 0)
  {
    for (int i = NUM_GRID_VALUES-1; i >= 1; i--)
    {
      if (grid_values[i] > step)
      {
        result = i;
        break;
      }
    }
  }
  else // (delta < 0)
  {
    for (int i = 1; i < NUM_GRID_VALUES; i++)
    {
      if (grid_values[i] < step)
      {
        result = i;
        break;
      }
    }
  }

  // already at the extreme end?
  if (result < 0)
    return;

  StepFromWidget(result);

  if (main_win)
    main_win->info_bar->SetGrid(result);
}


void Grid_State_c::AdjustScale(int delta)
{
  int result = -1;

  if (delta > 0)
  {
    for (int i = NUM_SCALE_VALUES-1; i >= 0; i--)
    {
      if (scale_values[i] > Scale*1.01)
      {
        result = i;
        break;
      }
    }
  }
  else // (delta < 0)
  {
    for (int i = 0; i < NUM_SCALE_VALUES; i++)
    {
      if (scale_values[i] < Scale*0.99)
      {
        result = i;
        break;
      }
    }
  }

  // already at the extreme end?
  if (result < 0)
    return;

  ScaleFromWidget(result);

  if (main_win)
    main_win->info_bar->SetScale(result);
}


void Grid_State_c::NearestScale(double want_scale)
{
  int result = 0;

  for (int i = 0; i < NUM_SCALE_VALUES; i++)
  {
    result = i;

    if (scale_values[i] < want_scale)
      break;
  }

  ScaleFromWidget(result);

  if (main_win)
    main_win->info_bar->SetScale(result);
}


void Grid_State_c::ScaleFromDigit(int digit)
{
  // digit must be 1 to 9

  int result = digit_scales[digit - 1];

  ScaleFromWidget(result);

  if (main_win)
    main_win->info_bar->SetScale(result);
}


//--- editor settings ---
// vi:ts=2:sw=2:expandtab
