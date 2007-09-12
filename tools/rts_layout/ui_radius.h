//------------------------------------------------------------------------
//  Radius Trigger Info
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

#ifndef __UI_RADIUS_H__
#define __UI_RADIUS_H__


class UI_RadiusInfo : public Fl_Group
{
public:
  UI_RadiusInfo(int X, int Y, int W, int H, const char *label = NULL);
  virtual ~UI_RadiusInfo();

private:
  rad_trigger_c *view_RAD;

  // current shape (radio buttons)
  Fl_Round_Button *is_radius;
  Fl_Round_Button *is_rect;

  Fl_Float_Input *pos_x1;
  Fl_Float_Input *pos_y1;

  // for rectangle shape, 'radius' widget is hidden
  // for radius shape, 'x2/y2' widgets are hidden
  Fl_Float_Input *radius;

  Fl_Float_Input *pos_x2;
  Fl_Float_Input *pos_y2;

  Fl_Float_Input *pos_z1;
  Fl_Float_Input *pos_z2;

  Fl_Input     *name;
  Fl_Int_Input *tag;

  // when appear: two rows of three on/off buttons
  Fl_Check_Button *appear_easy;
  Fl_Check_Button *appear_medium;
  Fl_Check_Button *appear_hard;

  Fl_Check_Button *appear_sp;
  Fl_Check_Button *appear_coop;
  Fl_Check_Button *appear_dm;

public:
  void LoadData(rad_trigger_c *rad);

  void UpdateField(rad_trigger_c *rad, int F);

  void SetViewRad(rad_trigger_c *rad);

private:
  void ConvertToRadius();
  void ConvertToRectangle();

  void update_Shape(rad_trigger_c *rad);
  void update_XY(rad_trigger_c *rad);
  void update_Z(rad_trigger_c *rad);
  void update_Name(rad_trigger_c *rad);
  void update_Tag(rad_trigger_c *rad);
  void update_WhenAppear(rad_trigger_c *rad);

  static void shape_callback(Fl_Widget *, void *);
};

#endif // __UI_RADIUS_H__

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
