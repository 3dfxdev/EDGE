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
#include "ui_grid.h"
#include "ui_panel.h"
#include "ui_radius.h"


//
// UI_RadiusInfo Constructor
//
UI_RadiusInfo::UI_RadiusInfo(int X, int Y, int W, int H, const char *label) : 
    Fl_Group(X, Y, W, H, label),
    view_RAD(NULL)
{
  end();  // cancel begin() in Fl_Group constructor

  box(FL_DOWN_BOX);

  X += 6;
  Y += 6;

  W -= 6;
  H -= 6;

  int MX = X + W/2;


  name = new Fl_Input(X+44, Y, W-48, 22, "Name:");
  name->align(FL_ALIGN_LEFT);
  name->callback(name_callback, this);

  add(name);

  Y += name->h() + 4;


  tag = new Fl_Int_Input(X+44, Y, W-48, 22, "Tag:");
  tag->align(FL_ALIGN_LEFT);
  tag->callback(tag_callback, this);

  add(tag);

  Y += tag->h() + 10;


  Fl_Box *sh_lab = new Fl_Box(FL_NO_BOX, X, Y, W, 22, "Shape:");
  sh_lab->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
  add(sh_lab);

  is_radius = new Fl_Round_Button(X+50, Y, 68, 22, "Radius");
  is_radius->type(FL_RADIO_BUTTON);
  is_radius->value(1);
  is_radius->callback(shape_callback, this);
  add(is_radius);
  
  is_rect = new Fl_Round_Button(X+120, Y, 60, 22, "Rect");
  is_rect->type(FL_RADIO_BUTTON);
  is_rect->value(0);
  is_rect->callback(shape_callback, this);
  add(is_rect);
  
  Y += is_rect->h() + 4;


  pos_x1 = new Fl_Float_Input(X +20, Y, W/2-24, 22, "x1");
  pos_y1 = new Fl_Float_Input(MX+20, Y, W/2-24, 22, "y1");

  pos_x1->align(FL_ALIGN_LEFT);
  pos_y1->align(FL_ALIGN_LEFT);

  pos_x1->callback(pos_callback, this);
  pos_y1->callback(pos_callback, this);

  add(pos_x1);
  add(pos_y1);

  Y += pos_x1->h() + 4;


  radius = new Fl_Float_Input(X+70, Y, W/2-24, 22, "radius");
  radius->align(FL_ALIGN_LEFT);
  radius->callback(pos_callback, this);

  add(radius);


  pos_x2 = new Fl_Float_Input(X +20, Y, W/2-24, 22, "x2");
  pos_y2 = new Fl_Float_Input(MX+20, Y, W/2-24, 22, "y2");

  pos_x2->align(FL_ALIGN_LEFT);
  pos_y2->align(FL_ALIGN_LEFT);

  pos_x2->callback(pos_callback, this);
  pos_y2->callback(pos_callback, this);

  add(pos_x2);
  add(pos_y2);

  pos_x2->hide();
  pos_y2->hide();

  Y += pos_x2->h() + 4;


  pos_z1 = new Fl_Float_Input(X +20, Y, W/2-24, 22, "z1");
  pos_z2 = new Fl_Float_Input(MX+20, Y, W/2-24, 22, "z2");

  pos_z1->align(FL_ALIGN_LEFT);
  pos_z2->align(FL_ALIGN_LEFT);

  pos_z1->callback(height_callback, this);
  pos_z2->callback(height_callback, this);

  add(pos_z1);
  add(pos_z2);

  Y += pos_z1->h() + 6;


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
  
  appear_easy->callback(whenapp_callback, this);
  appear_medium->callback(whenapp_callback, this);
  appear_hard->callback(whenapp_callback, this);

  appear_sp  ->callback(whenapp_callback, this);
  appear_coop->callback(whenapp_callback, this);
  appear_dm  ->callback(whenapp_callback, this);

  add(appear_easy); add(appear_medium); add(appear_hard);
  add(appear_sp);   add(appear_coop);   add(appear_dm);
}

//
// UI_RadiusInfo Destructor
//
UI_RadiusInfo::~UI_RadiusInfo()
{
}


void UI_RadiusInfo::SetViewRad(rad_trigger_c *rad)
{
  if (view_RAD == rad)
    return;

  view_RAD = rad;

  if (view_RAD)
  {
    main_win->panel->SetWhich(rad->rad_Index + 1, active_startmap->rad_Total);

    LoadData(view_RAD);
    activate();
  }
  else
  {
    main_win->panel->SetWhich(-1, -1);

    deactivate();
  }

  redraw();
}

void UI_RadiusInfo::shape_callback(Fl_Widget *w, void *data)
{
  UI_RadiusInfo *info = (UI_RadiusInfo *)data;
  SYS_ASSERT(info);

  if (info->view_RAD)
  {
    if (w == info->is_radius)
      Edit_ChangeShape(info->view_RAD, 0);

    if (w == info->is_rect)
      Edit_ChangeShape(info->view_RAD, 1);
  }
}

void UI_RadiusInfo::pos_callback(Fl_Widget *w, void *data)
{
  UI_RadiusInfo *info = (UI_RadiusInfo *)data;
  SYS_ASSERT(info);

  rad_trigger_c *RAD = info->view_RAD;
  if (! RAD)
    return;

  if (w == info->radius)
  {
    float r = Float_or_Unspec(info->radius->value());

    if (r == FLOAT_UNSPEC || r < 4)
      r = 4;

    Edit_ResizeRadiusOnly(RAD, r);
    return;
  }

  if (! RAD->is_rect)
  {
    if (w == info->pos_x1 || w == info->pos_y1)
    {
      Edit_MoveRad(RAD,
            atof(info->pos_x1->value()),
            atof(info->pos_y1->value()));
      return;
    }
  }

  float x1 = atof(info->pos_x1->value());
  float y1 = atof(info->pos_y1->value());
  float x2 = atof(info->pos_x2->value());
  float y2 = atof(info->pos_y2->value());

  Edit_ResizeRad(RAD, x1, y1, x2, y2);
}

void UI_RadiusInfo::height_callback(Fl_Widget *w, void *data)
{
  UI_RadiusInfo *info = (UI_RadiusInfo *)data;
  SYS_ASSERT(info);

  rad_trigger_c *RAD = info->view_RAD;
  if (! RAD)
    return;

  if (w == info->pos_z1)
  {
    Edit_ChangeFloat(RAD, rad_trigger_c::F_Z1,
          Float_or_Unspec(info->pos_z1->value()));
  }
  else /* (w == info->pos_z2) */
  {
    Edit_ChangeFloat(RAD, rad_trigger_c::F_Z2,
          Float_or_Unspec(info->pos_z2->value()));
  }
}

void UI_RadiusInfo::name_callback(Fl_Widget *w, void *data)
{
  UI_RadiusInfo *info = (UI_RadiusInfo *)data;
  SYS_ASSERT(info);

  rad_trigger_c *RAD = info->view_RAD;
  if (! RAD)
    return;

  Edit_ChangeString(RAD, rad_trigger_c::F_NAME,
        info->name->value());
}

void UI_RadiusInfo::tag_callback(Fl_Widget *w, void *data)
{
  UI_RadiusInfo *info = (UI_RadiusInfo *)data;
  SYS_ASSERT(info);

  rad_trigger_c *RAD = info->view_RAD;
  if (! RAD)
    return;

  Edit_ChangeInt(RAD, rad_trigger_c::F_TAG,
        Int_or_Unspec(info->tag->value()));
}

void UI_RadiusInfo::whenapp_callback(Fl_Widget *w, void *data)
{
  UI_RadiusInfo *info = (UI_RadiusInfo *)data;
  SYS_ASSERT(info);

  rad_trigger_c *RAD = info->view_RAD;
  if (! RAD)
    return;

  int when_appear = info->CalcWhenAppear();

  Edit_ChangeInt(RAD, rad_trigger_c::F_WHEN_APPEAR, when_appear);
}

int UI_RadiusInfo::CalcWhenAppear()
{
  int when_appear = 0;

  if (appear_easy->value())   when_appear |= WNAP_Easy;
  if (appear_medium->value()) when_appear |= WNAP_Medium;
  if (appear_hard->value())   when_appear |= WNAP_Hard;

  if (appear_sp->value())     when_appear |= WNAP_SP;
  if (appear_coop->value())   when_appear |= WNAP_Coop;
  if (appear_dm->value())     when_appear |= WNAP_DM;

  return when_appear;
}


//------------------------------------------------------------------------

void UI_ListenRadTrig(rad_trigger_c *rad, int F)
{
  main_win->panel->script_box->ListenField(rad, F);
}

void UI_RadiusInfo::ListenField(rad_trigger_c *rad, int F)
{
  if (rad != view_RAD)
    return;

  switch (F)
  {
    case rad_trigger_c::F_IS_RECT:
      update_Shape(rad);
      break;

    case rad_trigger_c::F_MX: case rad_trigger_c::F_MY:
    case rad_trigger_c::F_RX: case rad_trigger_c::F_RY:
      update_XY(rad);
      break;

    case rad_trigger_c::F_Z1: case rad_trigger_c::F_Z2:
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

    default: break;
  }

  main_win->grid->redraw();
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
  if (rad->is_rect)
  {
    is_rect  ->value(1);
    is_radius->value(0);

    radius->hide();

    pos_x2->show();
    pos_y2->show();
  }
  else /* radius */
  {
    is_radius->value(1);
    is_rect  ->value(0);

    pos_x2->hide();
    pos_y2->hide();

    radius->show();
  }

  update_XY(rad);
}

void UI_RadiusInfo::update_XY(rad_trigger_c *rad)
{
  char buffer[100];

  if (is_radius->value())
  {
    sprintf(buffer, "%1.1f", rad->mx);
    pos_x1->value(buffer);

    sprintf(buffer, "%1.1f", rad->my);
    pos_y1->value(buffer);

    sprintf(buffer, "%1.1f", rad->rx);
    radius->value(buffer);

    return;
  }

  /* rectangle */

  // X
  {
    sprintf(buffer, "%1.1f", rad->mx - rad->rx);
    pos_x1->value(buffer);

    sprintf(buffer, "%1.1f", rad->mx + rad->rx);
    pos_x2->value(buffer);
  }

  // Y
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

  sprintf(buffer, "%1.1f", rad->z1);
  pos_z1->value( (rad->z1 == FLOAT_UNSPEC) ? "" : buffer);

  sprintf(buffer, "%1.1f", rad->z2);
  pos_z2->value( (rad->z2 == FLOAT_UNSPEC) ? "" : buffer);
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
