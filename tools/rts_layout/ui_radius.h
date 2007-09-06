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
  Fl_Box    *which;  // e.g. 'Trig 1234/7777'

  Fl_Choice *shape;

  Fl_Float_Input *pos_x;
  Fl_Float_Input *pos_y;

  // for radius shape, pos_w is hidden and pos_h contains radius
  Fl_Float_Input *pos_w;
  Fl_Float_Input *pos_h;

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


private:

};

#endif // __UI_RADIUS_H__

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
