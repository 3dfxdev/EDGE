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


  pos_x = new Fl_Float_Input(X +20, Y, W/2-24, 22, "x");
  pos_y = new Fl_Float_Input(MX+20, Y, W/2-24, 22, "y");

  pos_x->align(FL_ALIGN_LEFT);
  pos_y->align(FL_ALIGN_LEFT);

  add(pos_x);
  add(pos_y);

  Y += pos_x->h() + 4;


  pos_w = new Fl_Float_Input(X +20, Y, W/2-24, 22, "w");
  pos_h = new Fl_Float_Input(MX+20, Y, W/2-24, 22, "h");

  pos_w->align(FL_ALIGN_LEFT);
  pos_h->align(FL_ALIGN_LEFT);

  add(pos_w);
  add(pos_h);

  Y += pos_x->h() + 4;


  pos_z1 = new Fl_Float_Input(X +20, Y, W/2-24, 22, "z1");
  pos_z2 = new Fl_Float_Input(MX+20, Y, W/2-24, 22, "z2");

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
  int AX = X+W/3;
  int BX = X+2*W/3;
  int CW = W/3 - 12;

  appear_easy   = new Fl_Check_Button( X+8, Y, CW, 22, "easy");
  appear_medium = new Fl_Check_Button(AX+8, Y, CW, 22, "medium");
  appear_hard   = new Fl_Check_Button(BX+8, Y, CW, 22, "hard");

  Y += appear_easy->h() + 2;

  appear_sp     = new Fl_Check_Button( X+8, Y, CW, 22, "sp");
  appear_coop   = new Fl_Check_Button(AX+8, Y, CW, 22, "coop");
  appear_dm     = new Fl_Check_Button(BX+8, Y, CW, 22, "dm");

  appear_easy->value(1);   appear_sp->value(1);
  appear_medium->value(1); appear_coop->value(1);
  appear_hard->value(1);   appear_dm->value(1);

  appear_easy->labelsize(12);   appear_sp->labelsize(12);
  appear_medium->labelsize(12); appear_coop->labelsize(12);
  appear_hard->labelsize(12);   appear_dm->labelsize(12);
  
  add(appear_easy); add(appear_medium); add(appear_hard);
  add(appear_sp);   add(appear_coop);   add(appear_dm);
}

//
// UI_RadiusInfo Destructor
//
UI_RadiusInfo::~UI_RadiusInfo()
{
}


//--- editor settings ---
// vi:ts=2:sw=2:expandtab
