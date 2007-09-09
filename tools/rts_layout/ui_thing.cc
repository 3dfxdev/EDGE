//------------------------------------------------------------------------
//  Spawn-Thing Info
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

#include "ui_thing.h"


//
// UI_ThingInfo Constructor
//
UI_ThingInfo::UI_ThingInfo(int X, int Y, int W, int H, const char *label) : 
    Fl_Group(X, Y, W, H, label)
{
  end();  // cancel begin() in Fl_Group constructor

  box(FL_DOWN_BOX);

  X += 6;
  Y += 6;

  W -= 6;
  H -= 6;

  int MX = X + W/2;


  which = new Fl_Box(FL_NO_BOX, X, Y, W, 22, "Thing #1234  of 7777");
  which->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

  add(which);

  Y += which->h() + 8;


  type = new Fl_Input(X+46, Y, W-50, 24, "Type: ");
  type->align(FL_ALIGN_LEFT);
  type->textsize(12);

  add(type);

  Y += type->h() + 4;


  pos_x = new Fl_Float_Input(X +20, Y, W/2-24, 22, "x");
  pos_y = new Fl_Float_Input(MX+20, Y, W/2-24, 22, "y");

  pos_x->align(FL_ALIGN_LEFT);
  pos_y->align(FL_ALIGN_LEFT);

  add(pos_x);
  add(pos_y);

  Y += pos_x->h() + 4;


  pos_z = new Fl_Float_Input(X +20, Y, W/2-24, 22, "z");
  pos_z->align(FL_ALIGN_LEFT);

  add(pos_z);

  Y += pos_x->h() + 4;


  angle = new Fl_Float_Input(X+54, Y, W/2-14, 22, "Angle:");
  angle->align(FL_ALIGN_LEFT);

  add(angle);

  Y += angle->h() + 4;


  tag = new Fl_Int_Input(X+54, Y, W/2-14, 22, "Tag: ");
  tag->align(FL_ALIGN_LEFT);

  add(tag);

  Y += tag->h() + 8;


  Fl_Box *opt_lab = new Fl_Box(X, Y, W, 22, "Options:");
  opt_lab->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

  add(opt_lab);

  Y += opt_lab->h() + 4;

    
  ambush = new Fl_Check_Button(X+12, Y, W-8, 22, "Ambush");

  add(ambush);

  Y += ambush->h() + 4;


  // when appear: two rows of three on/off buttons
  int AX = X+W/3;
  int BX = X+2*W/3;
  int CW = W/3 - 12;

  appear_easy   = new Fl_Check_Button( X+12, Y, CW, 22, "easy");
  appear_medium = new Fl_Check_Button(AX+8,  Y, CW, 22, "medium");
  appear_hard   = new Fl_Check_Button(BX+10, Y, CW, 22, "hard");

  Y += appear_easy->h() + 2;

#if 0
  appear_sp     = new Fl_Check_Button( X+8, Y, CW, 22, "sp");
  appear_coop   = new Fl_Check_Button(AX+8, Y, CW, 22, "coop");
  appear_dm     = new Fl_Check_Button(BX+8, Y, CW, 22, "dm");
#endif

  appear_easy->value(1);   // appear_sp->value(1);
  appear_medium->value(1); // appear_coop->value(1);
  appear_hard->value(1);   // appear_dm->value(1);

  appear_easy->labelsize(12);   // appear_sp->labelsize(12);
  appear_medium->labelsize(12); // appear_coop->labelsize(12);
  appear_hard->labelsize(12);   // appear_dm->labelsize(12);
  
  add(appear_easy); add(appear_medium); add(appear_hard);
}

//
// UI_ThingInfo Destructor
//
UI_ThingInfo::~UI_ThingInfo()
{
}


void UI_ThingListener(thing_spawn_c *th, int F)
{
  main_win->panel->thing_box->UpdateField(th, F);
}

void UI_ThingInfo::UpdateField(thing_spawn_c *th, int F)
{
  // FIXME!!!  if (th != main_win->grid->active_thing) return;

  if (F == F_AMBUSH || F == F_UPDATE_ALL)

  if (F == F_TYPE   || F == F_UPDATE_ALL)

  if (F == F_X      || F == F_UPDATE_ALL)

  if (F == F_Y      || F == F_UPDATE_ALL)

  if (F == F_Z      || F == F_UPDATE_ALL)

  if (F == F_ANGLE  || F == F_UPDATE_ALL)

  if (F == F_TAG    || F == F_UPDATE_ALL)

  if (F == F_WHEN_APPEAR || F == F_UPDATE_ALL)
}

void UI_ThingInfo::LoadData(thing_spawn_c *th)
{
  UpdateField(th, F_UPDATE_ALL);
}

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
