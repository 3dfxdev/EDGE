//------------------------------------------------------------------------
//  Main Window
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

#ifndef __UI_WINDOW_H__
#define __UI_WINDOW_H__

#include "ui_menu.h"

#define MAIN_BG_COLOR  fl_gray_ramp(FL_NUM_GRAY * 7 / 24)

class UI_Panel;

class UI_MainWin : public Fl_Double_Window
{
public:
  UI_MainWin(const char *title);
  virtual ~UI_MainWin();

public:
  // main child widgets

#ifdef MACOSX
  Fl_Sys_Menu_Bar *menu_bar;
#else
  Fl_Menu_Bar *menu_bar;
#endif

//  UI_Grid *grid;

  UI_Panel *panel;

  enum  // actions
  {
    NONE = 0,
    BUILD,
    ABORT,
    QUIT
  };
 
  int action;

public:
  void Locked(bool value);
};

extern UI_MainWin * main_win;


#endif // __UI_WINDOW_H__

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
