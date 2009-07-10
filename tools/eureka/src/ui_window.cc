//------------------------------------------------------------------------
//  Main Window
//------------------------------------------------------------------------
//
//  Oblige Level Maker (C) 2006-2008 Andrew Apted
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

#include "main.h"
#include "ui_window.h"

#ifndef WIN32
#include <unistd.h>
#endif

#if (FL_MAJOR_VERSION != 1 ||  \
     FL_MINOR_VERSION != 1 ||  \
     FL_PATCH_VERSION < 7)
#error "Require FLTK version 1.1.7 or later"
#endif



UI_MainWin *main_win;

#define MAIN_WINDOW_W  (800-32+QF*60)
#define MAIN_WINDOW_H  (600-98+QF*40)

#define MAX_WINDOW_W  MAIN_WINDOW_W
#define MAX_WINDOW_H  MAIN_WINDOW_H


static void main_win_close_CB(Fl_Widget *w, void *data)
{
  if (main_win)
    main_win->action = UI_MainWin::QUIT;
}


//
// MainWin Constructor
//
UI_MainWin::UI_MainWin(const char *title) :
    Fl_Double_Window(MAIN_WINDOW_W, MAIN_WINDOW_H, title),
    action(UI_MainWin::NONE),
    cursor_shape(FL_CURSOR_DEFAULT)
{
  end(); // cancel begin() in Fl_Group constructor

  size_range(MAIN_WINDOW_W, MAIN_WINDOW_H);

  callback((Fl_Callback *) main_win_close_CB);

  color(WINDOW_BG, WINDOW_BG);

  int cy = 0;
  int ey = h();


  /* ---- Menu bar ---- */
  {
    menu_bar = Menu_Create(0, 0, w() - 262, 28+QF*3);
    add(menu_bar);

#ifndef MACOSX
    cy += menu_bar->h();
#endif
  }


  info_bar = new UI_InfoBar(0, ey - (28+QF*3), w(), 28+QF*3);
  add(info_bar);

  ey = ey - info_bar->h();

  
  int panel_W = 260;

  canvas = new UI_Canvas(0, cy, w()-panel_W, ey - cy);
  add(canvas);

  resizable(canvas);


  int BY = 0;     // cy+2
  int BH = ey-2;  // ey-BY-2

  thing_box = new UI_ThingBox(w() - panel_W, BY, panel_W, BH);
  add(thing_box);

  line_box = new UI_LineBox(w() - panel_W, BY, panel_W, BH);
  line_box->hide();
  add(line_box);

  sec_box = new UI_SectorBox(w() - panel_W, BY, panel_W, BH);
  sec_box->hide();
  add(sec_box);

  vert_box = new UI_VertexBox(w() - panel_W, BY, panel_W, BH);
  vert_box->hide();
  add(vert_box);

  rad_box = new UI_RadiusBox(w() - panel_W, BY, panel_W, BH);
  rad_box->hide();
  add(rad_box);

}

//
// MainWin Destructor
//
UI_MainWin::~UI_MainWin()
{ }


void UI_MainWin::SetMode(char mode)
{
  // TODO: if mode == cur_mode then return end

  thing_box->hide();
   line_box->hide();
    sec_box->hide();
   vert_box->hide();
    rad_box->hide();

  switch (mode)
  {
    case 't': thing_box->show(); break;
    case 'l':  line_box->show(); break;
    case 's':   sec_box->show(); break;
    case 'v':  vert_box->show(); break;
    case 'r':   rad_box->show(); break;

    default: break;
  }

  info_bar->SetMode(mode);

  redraw();
}


void UI_MainWin::SetCursor(Fl_Cursor shape)
{
  if (shape == cursor_shape)
    return;

  cursor_shape = shape;

  cursor(shape);
}


const char *Int_TmpStr(int value)
{
  static char buffer[200];

  sprintf(buffer, "%d", value);

  return buffer;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
