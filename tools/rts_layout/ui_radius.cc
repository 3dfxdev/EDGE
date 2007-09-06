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

#include "headers.h"
#include "hdr_fltk.h"

#include "lib_util.h"

#include "ui_radius.h"


#define INFO_BG_COLOR  fl_rgb_color(96)


//
// UI_RadiusInfo Constructor
//
UI_RadiusInfo::UI_RadiusInfo(int X, int Y, int W, int H, const char *label) : 
    Fl_Group(X, Y, W, H, label)
{
  end();  // cancel begin() in Fl_Group constructor

  box(FL_DOWN_BOX);

  X += 6;
  Y += 6;

  W -= 6;
  H -= 6;

  int MX = X + W/2;


  which = new Fl_Box(FL_NO_BOX, X, Y, W, 22, "Trigger #1234 (of 7777)");
  which->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

  add(which);

  Y += which->h() + 8;


  shape = new Fl_Choice(X+54, Y, W-58, 24, "Shape: ");
  shape->align(FL_ALIGN_LEFT);
  shape->add("Radius|Rectangle");
  shape->value(1);

  add(shape);
  
  Y += shape->h() + 4;


  pos_x = new Fl_Float_Input(X +20, Y, W/2-24, 22, "x:");
  pos_y = new Fl_Float_Input(MX+20, Y, W/2-24, 22, "y:");

  pos_x->align(FL_ALIGN_LEFT);
  pos_y->align(FL_ALIGN_LEFT);

  add(pos_x);
  add(pos_y);

  Y += pos_x->h() + 4;


  pos_w = new Fl_Float_Input(X +20, Y, W/2-24, 22, "w:");
  pos_h = new Fl_Float_Input(MX+20, Y, W/2-24, 22, "h:");

  pos_w->align(FL_ALIGN_LEFT);
  pos_h->align(FL_ALIGN_LEFT);

  add(pos_w);
  add(pos_h);

  Y += pos_x->h() + 4;


  pos_z1 = new Fl_Float_Input(X +20, Y, W/2-24, 22, "z1:");
  pos_z2 = new Fl_Float_Input(MX+20, Y, W/2-24, 22, "z2:");

  pos_z1->align(FL_ALIGN_LEFT);
  pos_z1->align(FL_ALIGN_LEFT);

  add(pos_z1);
  add(pos_z2);

  Y += pos_x->h() + 12;


  name = new Fl_Input(X+44, Y, W-48, 22, "Name:");
  name->align(FL_ALIGN_LEFT);

  add(name);

  Y += name->h() + 4;


  tag = new Fl_Int_Input(X+44, Y, W-48, 22, "Tag:");
  tag->align(FL_ALIGN_LEFT);

  add(tag);

  Y += tag->h() + 4;


  // when appear: two rows of three on/off buttons
#if 0
  appear_easy;
  appear_medium;
  appear_hard;

  appear_sp;
  appear_coop;
  appear_dm;
#endif
}

//
// UI_RadiusInfo Destructor
//
UI_RadiusInfo::~UI_RadiusInfo()
{
}


//--- editor settings ---
// vi:ts=2:sw=2:expandtab
