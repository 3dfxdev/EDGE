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

#ifndef __UI_VERTEX_H__
#define __UI_VERTEX_H__

class UI_ThingInfo;
class UI_LineInfo;
class UI_SectorInfo;
class UI_VertexInfo;
class UI_RadiusInfo;


class UI_VertexBox : public Fl_Group
{
private:
  int cur_idx;

public: //???

  UI_Nombre *which;

  Fl_Int_Input *pos_x;
  Fl_Int_Input *pos_y;


public:
  UI_VertexBox(int X, int Y, int W, int H, const char *label = NULL);
  virtual ~UI_VertexBox();

public:
  int handle(int event);
  // FLTK virtual method for handling input events.

public:
  // a negative value will show 'None Selected'.
  void SetObj(int index);

private:
  static void pos_callback(Fl_Widget *, void *);
};

#endif // __UI_VERTEX_H__

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
