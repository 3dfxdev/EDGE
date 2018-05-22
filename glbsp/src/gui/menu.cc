//------------------------------------------------------------------------
// MENU : Unix/FLTK Menu handling
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


static boolean_g menu_want_to_quit;


static void menu_quit_CB(Fl_Widget *w, void *data)
{
  menu_want_to_quit = TRUE;
}

static void menu_do_clear_log(Fl_Widget *w, void * data)
{
  guix_win->text_box->ClearLog();
}

#ifndef MACOSX
static void menu_do_exit(Fl_Widget *w, void * data)
{
  guix_win->want_quit = TRUE;
}
#endif


//------------------------------------------------------------------------

static void menu_do_prefs(Fl_Widget *w, void * data)
{
  guix_pref_win = new Guix_PrefWin();

  // run the GUI until the user closes
  while (! guix_pref_win->want_quit)
    Fl::wait();

  delete guix_pref_win;
}


//------------------------------------------------------------------------

static const char *about_Info =
  "By Andrew Apted (C) 2000-2007\n"
  "\n"
  "Based on BSP 2.3 (C) 1998 Colin Reed, Lee Killough\n"
  "\n"
  "Additional credits to...\n"
  "    Andy Baker & Marc Pullen, for invaluable help\n"
  "    Janis Legzdinsh, for fixing up Hexen support\n"
  "    Matt Fell, for the Doom Specs\n"
  "    Raphael Quinet, for DEU and the original idea\n"
  "    ... and everyone else who deserves it !\n"
  "\n"
  "This program is free software, under the terms of\n"
  "the GNU General Public License, and comes with\n"
  "ABSOLUTELY NO WARRANTY.\n"
  "\n"
  "Website:  http://glbsp.sourceforge.net";


static void menu_do_about(Fl_Widget *w, void * data)
{
  menu_want_to_quit = FALSE;

  Fl_Window *ab_win = new Fl_Window(600, 340, "About glBSP");
  ab_win->end();

  // non-resizable
  ab_win->size_range(ab_win->w(), ab_win->h(), ab_win->w(), ab_win->h());
  ab_win->position(guix_prefs.manual_x, guix_prefs.manual_y);
  ab_win->callback((Fl_Callback *) menu_quit_CB);

  // add the about image
  Fl_Group *group = new Fl_Group(0, 0, 230, ab_win->h());
  group->box(FL_FLAT_BOX);
  group->color(FL_BLACK, FL_BLACK);
  ab_win->add(group);
  
  Fl_Box *box = new Fl_Box(20, 90, ABOUT_IMG_W+2, ABOUT_IMG_H+2);
  box->image(about_image);
  group->add(box); 


  // nice big logo text
  box = new Fl_Box(240, 5, 350, 50, "glBSPX  " GLBSP_VER);
  box->labelsize(24);
  ab_win->add(box);

  // about text
  box = new Fl_Box(240, 60, 350, 270, about_Info);
  box->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT | FL_ALIGN_TOP);
  ab_win->add(box);
   
 
  // finally add an "OK" button
  Fl_Button *button = new Fl_Button(ab_win->w()-10-60, ab_win->h()-10-30, 
      60, 30, "OK");
  button->callback((Fl_Callback *) menu_quit_CB);
  ab_win->add(button);
    
  ab_win->set_modal();
  ab_win->show();
  
  // capture initial size
  WindowSmallDelay();
  int init_x = ab_win->x();
  int init_y = ab_win->y();
  
  // run the GUI until the user closes
  while (! menu_want_to_quit)
    Fl::wait();

  // check if the user moved/resized the window
  if (ab_win->x() != init_x || ab_win->y() != init_y)
  {
    guix_prefs.manual_x = ab_win->x();
    guix_prefs.manual_y = ab_win->y();
  }
 
  // this deletes all the child widgets too...
  delete ab_win;
}


//------------------------------------------------------------------------

static void menu_do_manual(Fl_Widget *w, void * data)
{
  guix_book_win = new Guix_Book();

  guix_book_win->LoadPage(guix_prefs.manual_page);

  // run the GUI until the user closes
  while (! guix_book_win->want_quit)
  {
    guix_book_win->Update();

    Fl::wait();
  }

  delete guix_book_win;
}


//------------------------------------------------------------------------

static void menu_do_license(Fl_Widget *w, void * data)
{
  guix_lic_win = new Guix_License();

  // run the GUI until the user closes
  while (! guix_lic_win->want_quit)
  {
    Fl::wait();
  }

  delete guix_lic_win;
}


//------------------------------------------------------------------------

static void menu_do_save_log(Fl_Widget *w, void * data)
{
  char guess_name[512];
    
  // compute the "guess" filename
  
  if (! guix_info.output_file ||
      ! HelperFilenameValid(guix_info.output_file) ||
      strlen(guix_info.output_file) > 500)
  {
    strcpy(guess_name, "glbsp.log");
  }
  else
  {
    strcpy(guess_name, HelperReplaceExt(guix_info.output_file, "log"));
  }

  int choice;
  char buffer[1024];

  for (;;)
  {
    choice = DialogQueryFilename(
        "Please select the file to save the log into:",
        & guix_prefs.save_log_file, guess_name);

    if (choice != 0)
      return;

    if (!guix_prefs.save_log_file || guix_prefs.save_log_file[0] == 0)
    {
      DialogShowAndGetChoice(ALERT_TXT, skull_image, 
          "Please choose a Log filename.");
      continue;
    }

    if (! HelperFilenameValid(guix_prefs.save_log_file))
    {
      sprintf(buffer,
          "Invalid Log filename:\n"
          "\n"
          "      %s\n"
          "\n"
          "Please check the filename and try again.",
          guix_prefs.save_log_file);

      DialogShowAndGetChoice(ALERT_TXT, skull_image, buffer);
      continue;
    }

    // check for missing extension

    if (! HelperHasExt(guix_prefs.save_log_file))
    {
      if (guix_prefs.lack_ext_warn)
      {
        sprintf(buffer,
            "The Log file you selected has no extension.\n"
            "\n"
            "Do you want to add \".LOG\" and continue ?");

        choice = DialogShowAndGetChoice(ALERT_TXT, skull_image,
            buffer, "OK", "Re-Select", "Cancel");

        if (choice < 0 || choice == 2)
          return;

        if (choice == 1)
          continue;
      }

      // choice == 0
      {
        char *new_log = HelperReplaceExt(guix_prefs.save_log_file, "log");

        GlbspFree(guix_prefs.save_log_file);
        guix_prefs.save_log_file = GlbspStrDup(new_log);
      }
    }

    // check if file already exists...
    
    if (guix_prefs.overwrite_warn && 
        HelperFileExists(guix_prefs.save_log_file))
    {
      sprintf(buffer,
          "Warning: the chosen Log file already exists:\n"
          "\n"
          "      %s\n"
          "\n"
          "Do you want to overwrite it ?",
          guix_prefs.save_log_file);

      choice = DialogShowAndGetChoice(ALERT_TXT, skull_image,
          buffer, "OK", "Re-Select", "Cancel");

      if (choice < 0 || choice == 2)
        return;

      if (choice == 1)
        continue;
    }

    guix_win->text_box->SaveLog(guix_prefs.save_log_file);
    return;
  }
}


//------------------------------------------------------------------------

#undef FCAL
#define FCAL  (Fl_Callback *)

static Fl_Menu_Item menu_items[] = 
{
  { "&File", 0, 0, 0, FL_SUBMENU },
    { "&Preferences...",    0, FCAL menu_do_prefs },
    { "&Save Log...",       0, FCAL menu_do_save_log },
#ifdef MACOSX
    { "&Clear Log",         0, FCAL menu_do_clear_log },
#else
    { "&Clear Log",         0, FCAL menu_do_clear_log, 0, FL_MENU_DIVIDER },
    { "E&xit",   FL_ALT + 'q', FCAL menu_do_exit },
#endif
    { 0 },

  { "&Help", 0, 0, 0, FL_SUBMENU },
    { "&About...",         0,  FCAL menu_do_about },
    { "&License...",       0,  FCAL menu_do_license },
    { "&Manual...",   FL_F+1,  FCAL menu_do_manual },
    { 0 },

  { 0 }
};


//
// MenuCreate
//
#ifdef MACOSX
Fl_Sys_Menu_Bar * MenuCreate(int x, int y, int w, int h)
{
  Fl_Sys_Menu_Bar *bar = new Fl_Sys_Menu_Bar(x, y, w, h);
  bar->menu(menu_items);
  return bar;
}
#else
Fl_Menu_Bar * MenuCreate(int x, int y, int w, int h)
{
  Fl_Menu_Bar *bar = new Fl_Menu_Bar(x, y, w, h);
  bar->menu(menu_items);
  return bar;
}
#endif

