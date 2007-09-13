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
#include "g_edit.h"

#include "ui_window.h"
#include "ui_grid.h"
#include "ui_panel.h"
#include "ui_thing.h"


//
// UI_ThingInfo Constructor
//
UI_ThingInfo::UI_ThingInfo(int X, int Y, int W, int H, const char *label) : 
    Fl_Group(X, Y, W, H, label),
    view_TH(NULL)
{
  end();  // cancel begin() in Fl_Group constructor

  box(FL_DOWN_BOX);

  X += 6;
  Y += 6;

  W -= 6;
  H -= 6;

  int MX = X + W/2;


  type = new Fl_Input(X+50, Y, W-54, 24, "Type: ");
  type->align(FL_ALIGN_LEFT);
  type->textsize(12);
  type->callback(type_callback, this);

  add(type);

  Y += type->h() + 4;


  tag = new Fl_Int_Input(X+50, Y, W/2-14, 22, "Tag: ");
  tag->align(FL_ALIGN_LEFT);
  tag->callback(tag_callback, this);

  add(tag);

  Y += tag->h() + 4;


  angle = new Fl_Float_Input(X+50, Y, W/2-14, 22, "Angle:");
  angle->align(FL_ALIGN_LEFT);
  angle->callback(angle_callback, this);

  add(angle);

  Y += angle->h() + 10;


  pos_x = new Fl_Float_Input(X +20, Y, W/2-24, 22, "x");
  pos_y = new Fl_Float_Input(MX+20, Y, W/2-24, 22, "y");

  Y += pos_x->h() + 4;

  pos_z = new Fl_Float_Input(X +20, Y, W/2-24, 22, "z");

  pos_x->align(FL_ALIGN_LEFT);
  pos_y->align(FL_ALIGN_LEFT);
  pos_z->align(FL_ALIGN_LEFT);

  pos_x->callback(pos_callback, this);
  pos_y->callback(pos_callback, this);
  pos_z->callback(pos_callback, this);

  add(pos_x);
  add(pos_y);
  add(pos_z);

  Y += pos_x->h() + 8;


  Fl_Box *opt_lab = new Fl_Box(X, Y, W, 22, "Options:");
  opt_lab->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

  add(opt_lab);

  Y += opt_lab->h() + 2;

    
  ambush = new Fl_Check_Button(X+12, Y, W-8, 22, "Ambush");
  ambush->callback(option_callback, this);

  add(ambush);

  Y += ambush->h() + 2;


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

  appear_easy  ->value(1);
  appear_medium->value(1);
  appear_hard  ->value(1);

  appear_easy  ->labelsize(12);
  appear_medium->labelsize(12);
  appear_hard  ->labelsize(12);
  
  appear_easy  ->callback(option_callback, this);
  appear_medium->callback(option_callback, this);
  appear_hard  ->callback(option_callback, this);
  
  add(appear_easy); add(appear_medium); add(appear_hard);
}

//
// UI_ThingInfo Destructor
//
UI_ThingInfo::~UI_ThingInfo()
{
}

void UI_ThingInfo::SetViewThing(thing_spawn_c *th)
{
  if (view_TH == th)
    return;

  view_TH = th;

  if (view_TH)
  {
    SYS_ASSERT(th->parent);
    main_win->panel->SetWhich(th->th_Index + 1, th->parent->th_Total);

    LoadData(view_TH);
    activate();
  }
  else
  {
    main_win->panel->SetWhich(-1, -1);

    deactivate();
  }

  redraw();
}

void UI_ThingInfo::type_callback(Fl_Widget *w, void *data)
{
  UI_ThingInfo *info = (UI_ThingInfo *)data;
  SYS_ASSERT(info);

  thing_spawn_c *TH = info->view_TH;
  if (! TH)
    return;

  const char *str = info->type->value();
  while (isspace(*str))
    str++;

  // make sure type name is at least something valid
  Edit_ChangeString(TH, thing_spawn_c::F_TYPE, (*str) ? str : "XXXX");
}

void UI_ThingInfo::tag_callback(Fl_Widget *w, void *data)
{
  UI_ThingInfo *info = (UI_ThingInfo *)data;
  SYS_ASSERT(info);

  thing_spawn_c *TH = info->view_TH;
  if (! TH)
    return;

  Edit_ChangeInt(TH, thing_spawn_c::F_TAG,
        Int_or_Unspec(info->tag->value()));
}

void UI_ThingInfo::angle_callback(Fl_Widget *w, void *data)
{
  UI_ThingInfo *info = (UI_ThingInfo *)data;
  SYS_ASSERT(info);

  thing_spawn_c *TH = info->view_TH;
  if (! TH)
    return;

  Edit_ChangeFloat(TH, thing_spawn_c::F_ANGLE,
        Float_or_Unspec(info->angle->value()));
}

void UI_ThingInfo::pos_callback(Fl_Widget *w, void *data)
{
  UI_ThingInfo *info = (UI_ThingInfo *)data;
  SYS_ASSERT(info);

  thing_spawn_c *TH = info->view_TH;
  if (! TH)
    return;

  if (w == info->pos_z)
  {
    Edit_ChangeFloat(TH, thing_spawn_c::F_Z,
          Float_or_Unspec(info->pos_z->value()));
    return;
  }

  Edit_MoveThing(TH,
      atof(info->pos_x->value()),
      atof(info->pos_y->value()) );
}

void UI_ThingInfo::option_callback(Fl_Widget *w, void *data)
{
  UI_ThingInfo *info = (UI_ThingInfo *)data;
  SYS_ASSERT(info);

  thing_spawn_c *TH = info->view_TH;
  if (! TH)
    return;

  if (w == info->ambush)
  {
    Edit_ChangeInt(TH, thing_spawn_c::F_AMBUSH,
         info->ambush->value() ? 1 : 0);
    return;
  }
 
  Edit_ChangeInt(TH, thing_spawn_c::F_WHEN_APPEAR,
        info->CalcWhenAppear());
}

int UI_ThingInfo::CalcWhenAppear()
{
  int when_appear = 0;

  if (appear_easy  ->value()) when_appear |= WNAP_Easy;
  if (appear_medium->value()) when_appear |= WNAP_Medium;
  if (appear_hard  ->value()) when_appear |= WNAP_Hard;

#if 0
  if (appear_sp->value())     when_appear |= WNAP_SP;
  if (appear_coop->value())   when_appear |= WNAP_Coop;
  if (appear_dm->value())     when_appear |= WNAP_DM;
#endif

  return when_appear;
}



//------------------------------------------------------------------------

void UI_ListenThing(thing_spawn_c *th, int F)
{
  main_win->panel->thing_box->ListenField(th, F);
}

void UI_ThingInfo::ListenField(thing_spawn_c *th, int F)
{
  if (th != view_TH)
    return;

  switch (F)
  {
    case thing_spawn_c::F_AMBUSH:
      update_Ambush(th);
      break;

    case thing_spawn_c::F_TYPE:
      update_Type(th);
      break;

    case thing_spawn_c::F_X:
    case thing_spawn_c::F_Y:
    case thing_spawn_c::F_Z:
      update_Pos(th);
      break;

    case thing_spawn_c::F_ANGLE:
      update_Angle(th);
      break;

    case thing_spawn_c::F_TAG:
      update_Tag(th);
      break;

    case thing_spawn_c::F_WHEN_APPEAR:
      update_WhenAppear(th);
      break;

    default: break;
  }

  main_win->grid->redraw();
}

void UI_ThingInfo::LoadData(thing_spawn_c *th)
{
  update_Ambush(th);
  update_Type(th);
  update_Pos(th);
  update_Angle(th);
  update_Tag(th);
  update_WhenAppear(th);
}

void UI_ThingInfo::update_Ambush(thing_spawn_c *th)
{
  ambush->value(th->ambush);
}

void UI_ThingInfo::update_Type(thing_spawn_c *th)
{
  type->value(th->type.c_str());
}

void UI_ThingInfo::update_Pos(thing_spawn_c *th)
{
  pos_x->value(Float_TmpStr(th->x));
  pos_y->value(Float_TmpStr(th->y));
  pos_z->value(Float_TmpStr(th->z));
}

void UI_ThingInfo::update_Angle(thing_spawn_c *th)
{
  angle->value(Float_TmpStr(th->angle));
}

void UI_ThingInfo::update_Tag(thing_spawn_c *th)
{
  tag->value(Int_TmpStr(th->tag));
}

void UI_ThingInfo::update_WhenAppear(thing_spawn_c *th)
{
  appear_easy->value(   (th->when_appear & WNAP_Easy)  ? 1 : 0);
  appear_medium->value( (th->when_appear & WNAP_Medium)? 1 : 0);
  appear_hard->value(   (th->when_appear & WNAP_Hard)  ? 1 : 0);
}


//--- editor settings ---
// vi:ts=2:sw=2:expandtab
