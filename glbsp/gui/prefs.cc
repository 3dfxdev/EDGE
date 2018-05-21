//------------------------------------------------------------------------
// PREFS : Unix/FLTK Preference Window
//------------------------------------------------------------------------
//
//  GL-Friendly Node Builder (C) 2000-2007 Andrew Apted
//
//  Based on 'BSP 2.3' by Colin Reed, Lee Killough and others.
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

// this includes everything we need
#include "local.h"


Guix_PrefWin *guix_pref_win;


static void prefs_quit_CB(Fl_Widget *w, void *data)
{
  if (guix_pref_win)
    guix_pref_win->want_quit = TRUE;
}

static void prefs_reset_all_CB(Fl_Widget *w, void *data)
{
  MainSetDefaults();

  if (guix_pref_win)
    guix_pref_win->PrefsChanged();

  guix_win->ReadAllInfo();
}


//------------------------------------------------------------------------

//
// PrefWin Constructor
//
Guix_PrefWin::Guix_PrefWin() : Fl_Window(480, 290, "glBSP Preferences")
{
  // cancel the automatic 'begin' in Fl_Group constructor
  end();
 
  // non-resizable window
  size_range(w(), h(), w(), h());

  position(guix_prefs.other_x, guix_prefs.other_y);
 
  want_quit = FALSE;

  // allow manual closing of window
  callback((Fl_Callback *) prefs_quit_CB);
  
  // create buttons in top row

  groups[0] = new Fl_Group(0, 0, w(), 110, "Pop-up dialog options:");
  groups[0]->box(FL_THIN_UP_BOX);
  groups[0]->resizable(0);
  groups[0]->labelfont(FL_HELVETICA | FL_BOLD);
  groups[0]->labeltype(FL_NORMAL_LABEL);
  groups[0]->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT | FL_ALIGN_TOP);
  add(groups[0]);
 
  int CX = 20;
  int CY = 24;
   
  overwrite = new Fl_Check_Button(CX, CY, 22, 22,
      "Warn before overwriting files");
  overwrite->down_box(FL_DOWN_BOX);
  overwrite->align(FL_ALIGN_RIGHT);
  overwrite->value(guix_prefs.overwrite_warn ? 1 : 0);
  groups[0]->add(overwrite);

  CY += 22;

  same_file = new Fl_Check_Button(CX, CY, 22, 22,
      "Warn if Input and Output files are the same");
  same_file->down_box(FL_DOWN_BOX);
  same_file->align(FL_ALIGN_RIGHT);
  same_file->value(guix_prefs.same_file_warn ? 1 : 0);
  groups[0]->add(same_file);

  CY += 22;

  lack_ext = new Fl_Check_Button(CX, CY, 22, 22,
      "Warn about missing extensions (otherwise add the default)");
  lack_ext->down_box(FL_DOWN_BOX);
  lack_ext->align(FL_ALIGN_RIGHT);
  lack_ext->value(guix_prefs.lack_ext_warn ? 1 : 0);
  groups[0]->add(lack_ext);

  CY += 22;

  // create reset button
  
  groups[1] = new Fl_Group(0, 110, w(), 120);
  groups[1]->box(FL_THIN_UP_BOX);
  groups[1]->resizable(0);
  add(groups[1]);

  reset_all = new Fl_Button((w() - 240) / 2, 180, 240, 26,
      "Reset All To Defaults");
  reset_all->labelfont(FL_HELVETICA | FL_BOLD);
  reset_all->callback((Fl_Callback *) prefs_reset_all_CB);
  groups[1]->add(reset_all);

  Fl_Box *reset_txt = new Fl_Box(FL_FLAT_BOX, 20, 130, w() - 40, 46,
      "This button resets all user-changeable options to default "
      "values, including everything in the main window.");

  reset_txt->align(FL_ALIGN_LEFT | FL_ALIGN_TOP | 
      FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
  groups[1]->add(reset_txt);
  
  // create quit button
  
  groups[2] = new Fl_Group(0, 230, w(), 60);
  groups[2]->box(FL_THIN_UP_BOX);
  groups[2]->resizable(0);
  add(groups[2]);
  
  quit = new Fl_Button(w() - 100, h() - 40, 80, 26, "Close");
  quit->callback((Fl_Callback *) prefs_quit_CB);
  groups[2]->add(quit);

  // show the window
  set_modal();
  show();

  // read initial pos (same logic as in Guix_MainWin)
  WindowSmallDelay();
  
  init_x = x(); init_y = y();
}


//
// PrefWin Destructor
//
Guix_PrefWin::~Guix_PrefWin()
{
  // write new preferences
  guix_prefs.overwrite_warn = overwrite->value() ? TRUE : FALSE;
  guix_prefs.same_file_warn = same_file->value() ? TRUE : FALSE;
  guix_prefs.lack_ext_warn  = lack_ext->value()  ? TRUE : FALSE;
 
  // update window_pos if user moved the window
  if (x() != init_x || y() != init_y)
  {
    guix_prefs.other_x = x();
    guix_prefs.other_y = y();
  }
}


void Guix_PrefWin::PrefsChanged()
{
  overwrite->value(guix_prefs.overwrite_warn ? 1 : 0);
  overwrite->redraw();

  same_file->value(guix_prefs.same_file_warn ? 1 : 0);
  same_file->redraw();

  lack_ext->value(guix_prefs.lack_ext_warn ? 1 : 0);
  lack_ext->redraw();
}

