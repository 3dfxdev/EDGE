//------------------------------------------------------------------------
//  Information Panel
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

#ifndef __UI_LINEDEF_H__
#define __UI_LINEDEF_H__


class UI_LineBox : public Fl_Group
{
private:
  int obj;

public:
  UI_Nombre *which;

  Fl_Int_Input *type;
  Fl_Output    *desc;
  Fl_Button    *choose;

  Fl_Output    *length;
  Fl_Int_Input *tag;

  UI_SideBox *front;
  UI_SideBox *back;

  // Flags
  Fl_Choice *f_automap;

  Fl_Check_Button *f_upper;
  Fl_Check_Button *f_lower;
  Fl_Check_Button *f_passthru;

  Fl_Check_Button *f_walk;
  Fl_Check_Button *f_mons;
  Fl_Check_Button *f_sound;

public:
  UI_LineBox(int X, int Y, int W, int H, const char *label = NULL);
  virtual ~UI_LineBox();

public:
  void SetObj(int index);

private:
  void CalcLength();

  int  CalcFlags() const;
  void FlagsFromInt(int flags);

  static void flags_callback(Fl_Widget *, void *);
};

#endif // __UI_LINEDEF_H__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
