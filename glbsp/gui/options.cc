//------------------------------------------------------------------------
// Options : Unix/FLTK Option boxes
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


#define BM_BUTTONTYPE  FL_ROUND_DOWN_BOX
#define BM_BUTTONSIZE  30


static void build_mode_radio_CB(Fl_Widget *w, void *data)
{
  boolean_g old_gwa = guix_info.gwa_mode;
 
  guix_win->build_mode->WriteInfo();

  // communicate with output file widget, for GWA mode
  if (old_gwa != guix_info.gwa_mode)
  {
    guix_win->files->GWA_Changed();
  }

  guix_win->misc_opts->GWA_Changed();
}


//
// BuildMode Constructor
//
Guix_BuildMode::Guix_BuildMode(int x, int y, int w, int h) :
    Fl_Group(x, y, w, h, "Build Mode")
{
  // cancel the automatic 'begin' in Fl_Group constructor
  end();
  
  box(FL_THIN_UP_BOX);
  resizable(0);  // no resizing the kiddies, please

  labelfont(FL_HELVETICA | FL_BOLD);
  labeltype(FL_NORMAL_LABEL);
  align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT | FL_ALIGN_TOP);

  // create the children

  int CX = x+12;
  int CY = y+16;

  gwa = new Fl_Check_Button(CX, CY, 
      BM_BUTTONSIZE, BM_BUTTONSIZE, "GWA Mode");
  gwa->down_box(BM_BUTTONTYPE);
  gwa->type(FL_RADIO_BUTTON);
  gwa->align(FL_ALIGN_RIGHT);
  gwa->callback((Fl_Callback *) build_mode_radio_CB);
  add(gwa);

  CY += 24;

  maybe_normal = new Fl_Check_Button(CX, CY,
      BM_BUTTONSIZE, BM_BUTTONSIZE, "GL, Normal if missing");
  maybe_normal->down_box(BM_BUTTONTYPE);
  maybe_normal->type(FL_RADIO_BUTTON);
  maybe_normal->align(FL_ALIGN_RIGHT);
  maybe_normal->callback((Fl_Callback *) build_mode_radio_CB);
  add(maybe_normal);

  CY += 24;

  both = new Fl_Check_Button(CX, CY,
      BM_BUTTONSIZE, BM_BUTTONSIZE, "GL and Normal nodes");
  both->down_box(BM_BUTTONTYPE);
  both->type(FL_RADIO_BUTTON);
  both->align(FL_ALIGN_RIGHT);
  both->callback((Fl_Callback *) build_mode_radio_CB);
  add(both);

  CY += 24;

  gl_only = new Fl_Check_Button(CX, CY,
      BM_BUTTONSIZE, BM_BUTTONSIZE, "GL nodes only");
  gl_only->down_box(BM_BUTTONTYPE);
  gl_only->type(FL_RADIO_BUTTON);
  gl_only->align(FL_ALIGN_RIGHT);
  gl_only->callback((Fl_Callback *) build_mode_radio_CB);
  add(gl_only);

  CY += 24;

  ReadInfo();
}


//
// BuildMode Destructor
//
Guix_BuildMode::~Guix_BuildMode()
{
  WriteInfo();
}


void Guix_BuildMode::ReadInfo()
{
  if (guix_info.gwa_mode)
    gwa->setonly();
  
  else if (guix_info.no_normal)
    gl_only->setonly();
  
  else if (guix_info.force_normal)
    both->setonly();

  else
    maybe_normal->setonly();

  // redraw them all (just to be safe)
  gwa->redraw();
  gl_only->redraw();
  both->redraw();
  maybe_normal->redraw();
}


void Guix_BuildMode::WriteInfo()
{
  // default: everything false
  guix_info.gwa_mode = FALSE;
  guix_info.no_normal = FALSE;
  guix_info.force_normal = FALSE;

  if (gwa->value())
  {
    guix_info.gwa_mode = TRUE;
  }
  else if (gl_only->value())
  {
    guix_info.no_normal = TRUE;
  }
  else if (both->value())
  {
    guix_info.force_normal = TRUE;
  }
}


void Guix_BuildMode::LockOut(boolean_g lock_it)
{
  if (lock_it)
  {
    gwa->set_output();
    maybe_normal->set_output();
    both->set_output();
    gl_only->set_output();
  }
  else
  {
    gwa->clear_output();
    maybe_normal->clear_output();
    both->clear_output();
    gl_only->clear_output();
  }
}


//------------------------------------------------------------------------


static void misc_opts_check_CB(Fl_Widget *w, void *data)
{
  guix_win->misc_opts->WriteInfo();
}


//
// MiscOptions Constructor
//
Guix_MiscOptions::Guix_MiscOptions(int x, int y, int w, int h) :
    Fl_Group(x, y, w, h, "Misc Options")
{
  // cancel the automatic 'begin' in Fl_Group constructor
  end();
  
  box(FL_THIN_UP_BOX);
  resizable(0);  // no resizing the kiddies, please

  labelfont(FL_HELVETICA | FL_BOLD);
  labeltype(FL_NORMAL_LABEL);
  align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT | FL_ALIGN_TOP);

  // create children

  int CX = x+12;
  int CY = y+20;

  warnings = new Fl_Check_Button(CX, CY, 22, 22, "Extra Warnings");
  warnings->down_box(FL_DOWN_BOX);
  warnings->align(FL_ALIGN_RIGHT);
  warnings->callback((Fl_Callback *) misc_opts_check_CB);
  add(warnings);

  CY += 24;

  no_reject = new Fl_Check_Button(CX, CY, 22, 22, "Don't clobber REJECT");
  no_reject->down_box(FL_DOWN_BOX);
  no_reject->align(FL_ALIGN_RIGHT);
  no_reject->callback((Fl_Callback *) misc_opts_check_CB);
  add(no_reject);

  CY += 24;

  pack_sides = new Fl_Check_Button(CX, CY, 22, 22, "Pack Sidedefs");
  pack_sides->down_box(FL_DOWN_BOX);
  pack_sides->align(FL_ALIGN_RIGHT);
  pack_sides->callback((Fl_Callback *) misc_opts_check_CB);
  add(pack_sides);

  CY += 24;

  choose_fresh = new Fl_Check_Button(CX, CY, 22, 22, "Fresh Partition Lines");
  choose_fresh->down_box(FL_DOWN_BOX);
  choose_fresh->align(FL_ALIGN_RIGHT);
  choose_fresh->callback((Fl_Callback *) misc_opts_check_CB);
  add(choose_fresh);

  CY += 24;

  ReadInfo();
}


//
// MiscOptions Destructor
//
Guix_MiscOptions::~Guix_MiscOptions()
{
  WriteInfo();
}


void Guix_MiscOptions::ReadInfo()
{
  choose_fresh->value(guix_info.fast ? 0 : 1);  // API change
  choose_fresh->redraw();

  warnings->value(guix_info.mini_warnings ? 1 : 0);
  warnings->redraw();

  no_reject->value(guix_info.no_reject ? 1 : 0);
  no_reject->redraw();

  pack_sides->value(guix_info.pack_sides ? 1 : 0);
  pack_sides->redraw();

  GWA_Changed();
}


void Guix_MiscOptions::WriteInfo()
{
  guix_info.fast = choose_fresh->value() ? FALSE : TRUE;  // API change
  guix_info.no_reject = no_reject->value() ? TRUE : FALSE;
  guix_info.mini_warnings = warnings->value() ? TRUE : FALSE;
  guix_info.pack_sides = pack_sides->value() ? TRUE : FALSE;
}


void Guix_MiscOptions::GWA_Changed()
{
  if (guix_info.gwa_mode)
  {
    no_reject->deactivate();
    pack_sides->deactivate();
  }
  else
  {
    no_reject->activate();
    pack_sides->activate();
  }

  if (guix_info.force_normal)
    choose_fresh->deactivate();
  else
    choose_fresh->activate();
}


void Guix_MiscOptions::LockOut(boolean_g lock_it)
{
  if (lock_it)
  {
    choose_fresh->set_output();
    warnings->set_output();
    no_reject->set_output();
    pack_sides->set_output();
  }
  else
  {
    choose_fresh->clear_output();
    warnings->clear_output();
    no_reject->clear_output();
    pack_sides->clear_output();
  }
} 
