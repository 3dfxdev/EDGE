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


class thing_spawn_c;
class Sticker;


class UI_ThingBox : public Fl_Group
{
private:
  int obj;

  thing_spawn_c *view_TH;

  UI_Nombre *which;

  Fl_Int_Input *type;
  Fl_Output    *desc;
  Fl_Button    *choose;

  Fl_Int_Input *angle;
  Fl_Button    *ang_left;
  Fl_Button    *ang_right;

  Fl_Int_Input *exfloor;
  Fl_Button    *efl_down;
  Fl_Button    *efl_up;

  Fl_Int_Input *pos_x;
  Fl_Int_Input *pos_y;

  // Options
  Fl_Check_Button *o_easy;
  Fl_Check_Button *o_medium;
  Fl_Check_Button *o_hard;

  Fl_Check_Button *o_sp;
  Fl_Check_Button *o_coop;
  Fl_Check_Button *o_dm;

  Fl_Check_Button *o_ambush;
  Fl_Check_Button *o_friend;

  UI_Pic *sprite;

public:
  UI_ThingBox(int X, int Y, int W, int H, const char *label = NULL);
  virtual ~UI_ThingBox();

public:
  void LoadData(thing_spawn_c *th);

  void SetViewThing(thing_spawn_c *th);

  void ListenField(thing_spawn_c *th, int F);

public:
  // a negative value will show 'None Selected'.
  void SetObj(int index);

private:
  void update_Ambush(thing_spawn_c *th);
  void update_Type  (thing_spawn_c *th);
  void update_Pos   (thing_spawn_c *th);
  void update_Angle (thing_spawn_c *th);
  void update_WhenAppear(thing_spawn_c *th);

  int CalcWhenAppear();

  static void   type_callback(Fl_Widget *w, void *data);
  static void  angle_callback(Fl_Widget *w, void *data);
  static void    pos_callback(Fl_Widget *w, void *data);
  static void option_callback(Fl_Widget *w, void *data);
  static void button_callback(Fl_Widget *w, void *data);

  void Rotate(int dir);  // -1 for anticlockwise, +1 for clockwise
  void AdjustExtraFloor(int dir);

  int  CalcOptions() const;
  void OptionsFromInt(int options);

};

#endif // __UI_THING_H__

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
