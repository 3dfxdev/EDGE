//------------------------------------------------------------------------
// LOCAL : Unix/FLTK local definitions
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

#ifndef __GLBSP_LOCAL_H__
#define __GLBSP_LOCAL_H__


// use this for inlining.  Usually defined in the makefile.
#ifndef INLINE_G
#define INLINE_G  /* nothing */
#endif


//
//  INCLUDES
//

#include "glbsp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <assert.h>

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Counter.H>
#include <FL/Fl_File_Icon.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Image.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/Fl_Multi_Browser.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Pixmap.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/Fl_Slider.H>
#ifdef MACOSX
#include <FL/Fl_Sys_Menu_Bar.H>
#endif
#include <FL/Fl_Value_Input.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Window.H>

#include <FL/fl_ask.H>
#include <FL/fl_draw.H>
#include <FL/Fl_File_Chooser.H>


//
//  MAIN
//

typedef struct guix_preferences_s
{
  // main window position & size
  int win_x, win_y;
  int win_w, win_h;
  
  // positions & sizes of other windows
  int progress_x, progress_y;
  int dialog_x, dialog_y;
  int other_x, other_y;

  int manual_x, manual_y;
  int manual_w, manual_h;
  int manual_page;
  
  // warn about overwriting files
  boolean_g overwrite_warn;

  // warn about input file == output file
  boolean_g same_file_warn;

  // warn about missing extensions
  boolean_g lack_ext_warn;

  // filename for saving the log
  const char *save_log_file;
}
guix_preferences_t;

extern guix_preferences_t guix_prefs;

extern nodebuildinfo_t guix_info;
extern volatile nodebuildcomms_t guix_comms;
extern const nodebuildfuncs_t guix_funcs;

void MainSetDefaults(void);


//
//  ABOUT
//

extern const unsigned char about_image_pal[256 * 3];
extern const unsigned char about_image_data[];

#define ABOUT_IMG_W  190
#define ABOUT_IMG_H  190


//
//  BOOK
//

typedef struct book_page_s
{
  // array of lines, ends with NULL.
  // Link lines start with '#L' and two decimal digits.
  // Paragraphcs start with '#P' and two digits, first digit is major
  // indent level, second digit is minor indent level.
  // 
  const char ** text;
}
book_page_t;

class Guix_Book : public Fl_Window
{
public:
  Guix_Book();
  virtual ~Guix_Book();

  // override resize method, reformat text
  virtual FL_EXPORT void resize(int,int,int,int);
 
  // child widgets
  Fl_Group *group;
    
  Fl_Button *quit;
  Fl_Button *contents;
  Fl_Button *next;
  Fl_Button *prev;
  
  Fl_Hold_Browser *browser;

  boolean_g want_quit;

  int cur_page;

  // total number of pages ?
  int PageCount();

  // load a new page into the browser.  Updates 'page_num'.  This
  // routine should be called shortly after construction, to display
  // the initial page.  Zero will get the contents page.  Invalid
  // page numbers will be safely ignored.
  //
  void LoadPage(int new_num);

  // take the appropriate action for the given line number.
  void FollowLink(int line);

  // handle various internal "to do" items.  This is here since
  // modifying widgets (esp. clearing and adding lines to the hold
  // browser) from within their callbacks seems like a risky business
  // to me.  Should be called in the Fl::wait() loop.
  //
  void Update();

  // page to change to, otherwise BOOK_NO_PAGE
  int want_page;

protected:

  boolean_g want_reformat;

  // initial window position and size
  int init_x, init_y;
  int init_w, init_h;

  // -- paragraph code --
  
  boolean_g in_paragraph;
  boolean_g first_line;
  
  int major_indent;
  int minor_indent;

  char para_buf[1024];
  int para_width;

  void ParaStart();
  void ParaEnd();

  void ParaAddWord(const char *word);
  void ParaAddLine(const char *line);
};

#define BOOK_NO_PAGE  -50

extern const book_page_t book_pages[];
extern Guix_Book * guix_book_win;


class Guix_License : public Fl_Window
{
public:
  Guix_License();
  virtual ~Guix_License();

  // child widgets
  Fl_Group *group;
  Fl_Button *quit;
  Fl_Browser *browser;

  boolean_g want_quit;

protected:

  // initial window position and size
  int init_x, init_y;
  int init_w, init_h;
};

extern Guix_License * guix_lic_win;


//
//  COOKIE
//

typedef enum
{
  COOKIE_E_OK = 0,
  COOKIE_E_NO_FILE = 1,
  COOKIE_E_PARSE_ERRORS = 2,
  COOKIE_E_CHECK_ERRORS = 3
}
cookie_status_t;

void CookieSetPath(const char *argv0);
cookie_status_t CookieReadAll(void);
cookie_status_t CookieWriteAll(void);


//
//  DIALOG
//

void DialogLoadImages(void);
void DialogFreeImages(void);

int DialogShowAndGetChoice(const char *title, Fl_Pixmap *pic, 
    const char *message, const char *left = "OK", 
    const char *middle = NULL, const char *right = NULL);

int DialogQueryFilename(const char *message,
    const char ** name_ptr, const char *guess_name);

void GUI_FatalError(const char *str, ...);


//
//  FILES
//

class Guix_FileBox : public Fl_Group
{
public:
  Guix_FileBox(int x, int y, int w, int h);
  virtual ~Guix_FileBox();

  // child widgets in File area
  Fl_Box *in_label;
  Fl_Box *out_label;

  Fl_Input *in_file;
  Fl_Input *out_file;
  Fl_Output *out_gwa_file;
 
  Fl_Button *in_browse;
  Fl_Button *out_browse;
  Fl_Button *out_guess;

  // filename to produce when in GWA mode.
  const char *gwa_filename;
  
  // group for file boxes, to handle resizing properly.
  Fl_Group *file_group;
 
  // routine to set the input widgets based on the build-info.
  void ReadInfo();

  // routine to change the build-info to match the widgets.
  void WriteInfo();

  // routine to call when GWA mode changes state.
  void GWA_Changed();

  // routine to call when the in_file changes.  It should compute
  // a new value for 'gwa_filename', and update the out_gwa_file
  // widget.
  //
  void InFileChanged();

  // locking routine (see Guix_MainWin)
  void LockOut(boolean_g lock_it);
};

class Guix_FactorBox : public Fl_Group
{
public:
  Guix_FactorBox(int x, int y, int w, int h);
  virtual ~Guix_FactorBox();

  // child widget
  Fl_Counter *factor;

  // routine to set the input widget based on the build-info.
  void ReadInfo();

  // routine to change the build-info to match the input.
  void WriteInfo();

  // locking routine (see Guix_MainWin)
  void LockOut(boolean_g lock_it);
};

class Guix_BuildButton : public Fl_Group
{
public:
  Guix_BuildButton(int x, int y, int w, int h);
  virtual ~Guix_BuildButton();

  // child widget
  Fl_Button *build;
  Fl_Button *stopper;

  // locking routine (see Guix_MainWin)
  void LockOut(boolean_g lock_it);
};

#define ALERT_TXT  "glBSP Alert"
#define MISSING_COMMS  "(Not Specified)"


//
//  HELPER
//

int HelperCaseCmp(const char *A, const char *B);
int HelperCaseCmpLen(const char *A, const char *B, int len);

unsigned int HelperGetMillis();

boolean_g HelperFilenameValid(const char *filename);
boolean_g HelperHasExt(const char *filename);
boolean_g HelperCheckExt(const char *filename, const char *ext);
char *HelperReplaceExt(const char *filename, const char *ext);
char *HelperGuessOutput(const char *filename);
boolean_g HelperFileExists(const char *filename);


//
//  IMAGES
//

extern const char * pldie_image_data[];
extern const char * skull_image_data[];

extern Fl_Image * about_image;
extern Fl_Pixmap * pldie_image;
extern Fl_Pixmap * skull_image;


//
//  LICENSE
//

extern const char *license_text[];


//
//  MENU
//

#define MANUAL_WINDOW_MIN_W  500
#define MANUAL_WINDOW_MIN_H  200

#ifdef MACOSX
Fl_Sys_Menu_Bar
#else
Fl_Menu_Bar
#endif
* MenuCreate(int x, int y, int w, int h);


//
//  OPTIONS
//

class Guix_BuildMode : public Fl_Group
{
public:
  Guix_BuildMode(int x, int y, int w, int h);
  virtual ~Guix_BuildMode();

  // child widgets: a set of radio buttons
  Fl_Button *gwa;
  Fl_Button *maybe_normal;
  Fl_Button *both;
  Fl_Button *gl_only;

  // this routine sets one of the radio buttons on, based on the given
  // build-information.
  //
  void ReadInfo();

  // this routine does the reverse, setting the 'gwa_mode', 'no_normal'
  // and 'force_normal' fields based on which radio button is currently
  // active.
  //
  void WriteInfo();

  // locking routine (see Guix_MainWin)
  void LockOut(boolean_g lock_it);
};

class Guix_MiscOptions : public Fl_Group
{
public:
  Guix_MiscOptions(int x, int y, int w, int h);
  virtual ~Guix_MiscOptions();

  // child widgets: a set of toggle buttons
  Fl_Button *choose_fresh;
  Fl_Button *warnings;
  Fl_Button *no_reject;
  Fl_Button *pack_sides;
 
  // routine to set the buttons based on the build-info.
  void ReadInfo();

  // routine to change the build-info to match the buttons.
  void WriteInfo();
  
  // routine to call when GWA mode changes state.
  void GWA_Changed();

  // locking routine (see Guix_MainWin)
  void LockOut(boolean_g lock_it);
};


//
//  PREFS
//

class Guix_PrefWin : public Fl_Window
{
public:
  // constructor reads the guix_pref values.
  // destructor writes them.
 
  Guix_PrefWin();
  virtual ~Guix_PrefWin();

  boolean_g want_quit;

  // child widgets
  Fl_Group *groups[3];

  Fl_Button *overwrite;
  Fl_Button *same_file;
  Fl_Button *lack_ext;

  // color stuff ??
 
  Fl_Button *reset_all;
  Fl_Button *quit;

  // routine called by "reset all" button
  void PrefsChanged();
 
protected:

  // initial window position
  int init_x, init_y;
};

extern Guix_PrefWin * guix_pref_win;


//
//  PROGRESS
//

typedef struct guix_bar_s
{
  // widgets
  Fl_Box *label;
  Fl_Slider *slide;
  Fl_Box *perc;

  // current string for bar
  const char *lab_str;

  // string buffer for percentage
  char perc_buf[16];

  int perc_shown;
  // current percentage shown (avoid updating widget)
}
guix_bar_t;

class Guix_ProgressBox : public Fl_Group
{
public:
  Guix_ProgressBox(int x, int y, int w, int h);
  virtual ~Guix_ProgressBox();

  // child widgets
  guix_bar_t bars[2];

  Fl_Group *group;

  // current message strings
  const char *title_str;

  // current number of bars (READ ONLY from outside)
  int curr_bars;

  // sets the number of active bars.  Must be 1 or 2.
  void SetBars(int num);
  
  // clear the progress bars (e.g. when stopped by user)
  void ClearBars(void);
  
  // set the short name of the bar
  void SetBarName(int which, const char *label_short);
   
protected:

  // initial window position
  int init_x, init_y;

  void CreateOneBar(guix_bar_t& bar, int x, int y, int w, int h);

  void SetupOneBar(guix_bar_t& bar, int y, const char *label_short, 
      Fl_Color col);
};

void GUI_Ticker(void);

boolean_g GUI_DisplayOpen(displaytype_e type);
void GUI_DisplaySetTitle(const char *str);
void GUI_DisplaySetBar(int barnum, int count);
void GUI_DisplaySetBarLimit(int barnum, int limit);
void GUI_DisplaySetBarText(int barnum, const char *str);
void GUI_DisplayClose(void);


//
//  TEXTBOX
//

class Guix_TextBox : public Fl_Multi_Browser
{
public:
  Guix_TextBox(int x, int y, int w, int h);
  virtual ~Guix_TextBox();

  // adds the message to the text box.  The message may contain
  // newlines ('\n').  The message doesn't need a trailing '\n', i.e.
  // it is implied.
  //
  void AddMsg(const char *msg, Fl_Color col = FL_BLACK, 
      boolean_g bold = FALSE);
 
  // add a horizontal dividing bar
  void AddHorizBar();
  
  // routine to clear the text box
  void ClearLog();

  // routine to save the log to a file.  Will overwrite the file if
  // already exists.  Returns TRUE if successful or FALSE on error.
  //
  boolean_g SaveLog(const char *filename);

  // locking routine (see Guix_MainWin)
  void LockOut(boolean_g lock_it);
};

void GUI_PrintMsg(const char *str, ...);


//
//  WINDOW
//

#define MAIN_BG_COLOR  fl_gray_ramp(FL_NUM_GRAY * 9 / 24)

#define MAIN_WINDOW_MIN_W  540
#define MAIN_WINDOW_MIN_H  450

class Guix_MainWin : public Fl_Window
{
public:
  Guix_MainWin(const char *title);
  virtual ~Guix_MainWin();

  // main child widgets
  
#ifdef MACOSX
  Fl_Sys_Menu_Bar *menu_bar;
#else
  Fl_Menu_Bar *menu_bar;
#endif

  Guix_BuildMode *build_mode;
  Guix_MiscOptions *misc_opts;
  Guix_FactorBox *factor;

  Guix_FileBox *files;
  Guix_ProgressBox *progress;
  Guix_BuildButton *builder;
  Guix_TextBox *text_box;
 
  // user closed the window
  boolean_g want_quit;
  
  // routine to capture the current main window state into the
  // guix_preferences_t structure.
  // 
  void WritePrefs();

  // this routine is useful if the nodebuildinfo has changed.  It
  // causes all the widgets to update themselves using the new
  // parameters.
  // 
  void ReadAllInfo();
  
  // routine to capture all of the nodebuildinfo state from the
  // various widgets.  Note: don't need to call this before
  // destructing everything -- the widget destructors will do it
  // automatically.
  // 
  void WriteAllInfo();

  // this routine will update the user interface to prevent the user
  // from modifying most of the widgets (used during building).  When
  // 'lock_it' is FALSE, we are actually unlocking a previous lock.
  // 
  void LockOut(boolean_g lock_it);
   
protected:
  
  // initial window size, read after the window manager has had a
  // chance to move the window somewhere else.  If the window is still
  // there when CaptureState() is called, we don't need to update the
  // coords in the cookie file.
  // 
  int init_x, init_y, init_w, init_h;
};

extern Guix_MainWin * guix_win;

void WindowSmallDelay(void);


#endif /* __GLBSP_LOCAL_H__ */
