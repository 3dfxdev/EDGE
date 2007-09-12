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

#ifndef __UI_THING_H__
#define __UI_THING_H__


class UI_ThingInfo : public Fl_Group
{
public:
  UI_ThingInfo(int X, int Y, int W, int H, const char *label = NULL);
  virtual ~UI_ThingInfo();

private:
  thing_spawn_c *view_TH;

///---  Fl_Box *which;  // e.g. 'Thing #1234'

  Fl_Input *type;

  Fl_Float_Input *pos_x;
  Fl_Float_Input *pos_y;
  Fl_Float_Input *pos_z;

  Fl_Float_Input *angle;

  Fl_Check_Button *ambush;

  Fl_Int_Input *tag;

  // when appear: two rows of three on/off buttons
  Fl_Check_Button *appear_easy;
  Fl_Check_Button *appear_medium;
  Fl_Check_Button *appear_hard;

#if 0
  Fl_Check_Button *appear_sp;
  Fl_Check_Button *appear_coop;
  Fl_Check_Button *appear_dm;
#endif

public:
  void LoadData(thing_spawn_c *th);

  void SetViewThing(thing_spawn_c *th);

  void ListenField(thing_spawn_c *th, int F);

private:
  void update_Ambush(thing_spawn_c *th);
  void update_Type(thing_spawn_c *th);
  void update_Loc(thing_spawn_c *th);
  void update_Angle(thing_spawn_c *th);
  void update_Tag(thing_spawn_c *th);
  void update_WhenAppear(thing_spawn_c *th);

};

#endif // __UI_THING_H__

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
