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
#include "g_edit.h"

#include "ui_window.h"
#include "ui_panel.h"
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


  which = new Fl_Box(FL_NO_BOX, X, Y, W, 22, "Trigger #1234  of 7777");
  which->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

  add(which);

  Y += which->h() + 8;


  shape = new Fl_Choice(X+54, Y, W-58, 24, "Shape: ");
  shape->align(FL_ALIGN_LEFT);
  shape->add("Radius|Rectangle");
  shape->value(0);
  shape->callback(shape_callback, this);

  add(shape);
  
  Y += shape->h() + 4;


  pos_x1 = new Fl_Float_Input(X +20, Y, W/2-24, 22, "x1");
  pos_y1 = new Fl_Float_Input(MX+20, Y, W/2-24, 22, "y1");

  pos_x1->align(FL_ALIGN_LEFT);
  pos_y1->align(FL_ALIGN_LEFT);

  add(pos_x1);
  add(pos_y1);

  Y += pos_x1->h() + 4;


  radius = new Fl_Float_Input(MX+20, Y, W/2-24, 22, "radius");
  radius->align(FL_ALIGN_LEFT);

  add(radius);


  pos_x2 = new Fl_Float_Input(X +20, Y, W/2-24, 22, "x2");
  pos_y2 = new Fl_Float_Input(MX+20, Y, W/2-24, 22, "y2");

  pos_x2->align(FL_ALIGN_LEFT);
  pos_y2->align(FL_ALIGN_LEFT);

  add(pos_x2);
  add(pos_y2);

  pos_x2->hide();
  pos_y2->hide();

  Y += pos_x2->h() + 4;


  pos_z1 = new Fl_Float_Input(X +20, Y, W/2-24, 22, "z1");
  pos_z2 = new Fl_Float_Input(MX+20, Y, W/2-24, 22, "z2");

  pos_z1->align(FL_ALIGN_LEFT);
  pos_z1->align(FL_ALIGN_LEFT);

  add(pos_z1);
  add(pos_z2);

  Y += pos_z1->h() + 12;


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


void UI_RadiusInfo::shape_callback(Fl_Widget *w, void *data)
{
  UI_RadiusInfo *rad = (UI_RadiusInfo *)data;

  SYS_ASSERT(rad);

  if (rad->shape->value() == 0)
    rad->ConvertToRadius();
  else
    rad->ConvertToRectangle();
}

void UI_RadiusInfo::ConvertToRadius()
{

  // FIXME: do conversion
}

void UI_RadiusInfo::ConvertToRectangle()
{

  // FIXME: do conversion
}


//------------------------------------------------------------------------

void UI_RadTrigListener(rad_trigger_c *rad, int F)
{
  main_win->panel->script_box->UpdateField(rad, F);
}

void UI_RadiusInfo::UpdateField(rad_trigger_c *rad, int F)
{
  // FIXME!!!  if (rad != main_win->grid->active_radtrig) return;

  switch (F)
  {
    case rad_trigger_c::F_IS_RECT:
      update_Shape(rad);
      break;

    case rad_trigger_c::F_MX: case rad_trigger_c::F_MY:
    case rad_trigger_c::F_RX: case rad_trigger_c::F_RY:
      update_XY(rad);
      break;

    case rad_trigger_c::F_MZ: case rad_trigger_c::F_RZ:
      update_Z(rad);
      break;

    case rad_trigger_c::F_NAME:
      update_Name(rad);
      break;

    case rad_trigger_c::F_TAG:
      update_Tag(rad);
      break;

    case rad_trigger_c::F_WHEN_APPEAR:
      update_WhenAppear(rad);
      break;

    default:
      break;
  }
}

void UI_RadiusInfo::LoadData(rad_trigger_c *rad)
{
  update_Shape(rad);
  update_XY(rad);
  update_Z(rad);
  update_Name(rad);
  update_Tag(rad);
  update_WhenAppear(rad);
}

void UI_RadiusInfo::update_Shape(rad_trigger_c *rad)
{
  shape->value(rad->is_rect ? 1 : 0);

  if (rad->is_rect)
  {
    radius->hide();

    pos_x2->show();
    pos_y2->show();
  }
  else /* radius */
  {
    pos_x2->hide();
    pos_y2->hide();

    radius->show();
  }

  update_XY(rad);
}

void UI_RadiusInfo::update_XY(rad_trigger_c *rad)
{
  char buffer[100];

  if (shape->value() == 0) /* radius */
  {
    sprintf(buffer, "%1.1f", rad->mx);
    pos_x1->value( (rad->mx == FLOAT_UNSPEC) ? "" : buffer);

    sprintf(buffer, "%1.1f", rad->my);
    pos_y1->value( (rad->my == FLOAT_UNSPEC) ? "" : buffer);

    return;
  }

  /* rectangle */

  if (rad->mx == FLOAT_UNSPEC)
  {
    pos_x1->value("");
    pos_x2->value("");
  }
  else if (rad->rx == FLOAT_UNSPEC)
  {
    sprintf(buffer, "%1.1f", rad->mx);
    pos_x1->value(buffer);
    pos_x2->value("");
  }
  else
  {
    sprintf(buffer, "%1.1f", rad->mx - rad->rx);
    pos_x1->value(buffer);

    sprintf(buffer, "%1.1f", rad->mx + rad->rx);
    pos_x2->value(buffer);
  }

  if (rad->my == FLOAT_UNSPEC)
  {
    pos_y1->value("");
    pos_y2->value("");
  }
  else if (rad->ry == FLOAT_UNSPEC)
  {
    sprintf(buffer, "%1.1f", rad->my);
    pos_y1->value(buffer);
    pos_y2->value("");
  }
  else
  {
    sprintf(buffer, "%1.1f", rad->my - rad->ry);
    pos_y1->value(buffer);

    sprintf(buffer, "%1.1f", rad->my + rad->ry);
    pos_y2->value(buffer);
  }
}

void UI_RadiusInfo::update_Z(rad_trigger_c *rad)
{
  char buffer[100];

  if (rad->mz == FLOAT_UNSPEC)
  {
    pos_z1->value("");
    pos_z2->value("");
  }
  else if (rad->rz == FLOAT_UNSPEC)
  {
    sprintf(buffer, "%1.1f", rad->mz);
    pos_z1->value(buffer);
    pos_z2->value("");
  }
  else
  {
    sprintf(buffer, "%1.1f", rad->mz - rad->rz);
    pos_z1->value(buffer);

    sprintf(buffer, "%1.1f", rad->mz + rad->rz);
    pos_z2->value(buffer);
  }
}

void UI_RadiusInfo::update_Name(rad_trigger_c *rad)
{
  name->value(rad->name.c_str());
}

void UI_RadiusInfo::update_Tag(rad_trigger_c *rad)
{
  char buffer[100];

  sprintf(buffer, "%d", rad->tag);
  tag->value( (rad->tag == INT_UNSPEC) ? "" : buffer);
}

void UI_RadiusInfo::update_WhenAppear(rad_trigger_c *rad)
{
  appear_easy->value(   (rad->when_appear & WNAP_Easy)  ? 1 : 0);
  appear_medium->value( (rad->when_appear & WNAP_Medium)? 1 : 0);
  appear_hard->value(   (rad->when_appear & WNAP_Hard)  ? 1 : 0);

  appear_sp->value(   (rad->when_appear & WNAP_SP)  ? 1 : 0);
  appear_coop->value( (rad->when_appear & WNAP_Coop)? 1 : 0);
  appear_dm->value(   (rad->when_appear & WNAP_DM)  ? 1 : 0);
}


//--- editor settings ---
// vi:ts=2:sw=2:expandtab
