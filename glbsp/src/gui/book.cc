//------------------------------------------------------------------------
// BOOK : Unix/FLTK Manual Code
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


Guix_Book *guix_book_win;


static void book_quit_CB(Fl_Widget *w, void *data)
{
  if (guix_book_win)
    guix_book_win->want_quit = TRUE;
}

static void book_contents_CB(Fl_Widget *w, void *data)
{
  if (guix_book_win)
    guix_book_win->want_page = 0;
}

static void book_prev_CB(Fl_Widget *w, void *data)
{
  if (guix_book_win)
    guix_book_win->want_page = guix_book_win->cur_page - 1;
}

static void book_next_CB(Fl_Widget *w, void *data)
{
  if (guix_book_win)
    guix_book_win->want_page = guix_book_win->cur_page + 1;
}

static void book_selector_CB(Fl_Widget *w, void *data)
{
  if (guix_book_win)
    guix_book_win->FollowLink(guix_book_win->browser->value() - 1);
}


//------------------------------------------------------------------------

//
// Book Constructor
//
Guix_Book::Guix_Book() : Fl_Window(guix_prefs.manual_w,
    guix_prefs.manual_h, "glBSP Manual")
{
  // cancel the automatic 'begin' in Fl_Group constructor
  end();
 
  size_range(MANUAL_WINDOW_MIN_W, MANUAL_WINDOW_MIN_H);
  position(guix_prefs.manual_x, guix_prefs.manual_y);
  color(MAIN_BG_COLOR, MAIN_BG_COLOR);
 
  // allow manual closing of window
  callback((Fl_Callback *) book_quit_CB);
  
  want_quit = FALSE;
  want_page = BOOK_NO_PAGE;
  want_reformat = FALSE;
  cur_page = 0;

  // create buttons in top row

  group = new Fl_Group(0, h() - 34, w(), 34);
  add(group);
  
  int CX = 10;
  int CY = h() - 30;

  contents = new Fl_Button(CX, CY, 96, 26, "&Contents");
  contents->box(FL_ROUND_UP_BOX);
  contents->callback((Fl_Callback *) book_contents_CB);
  group->add(contents);

  CX += 126;

  prev = new Fl_Button(CX, CY, 96, 26, "<<  &Prev");
  prev->box(FL_ROUND_UP_BOX);
  prev->callback((Fl_Callback *) book_prev_CB);
  group->add(prev);

  CX += 106;

  next = new Fl_Button(CX, CY, 96, 26, "&Next  >>");
  next->box(FL_ROUND_UP_BOX);
  next->callback((Fl_Callback *) book_next_CB);
  group->add(next);

  CX += 106;

  quit = new Fl_Button(w() - 96 - 24, CY, 96, 26, "Close");
  quit->box(FL_ROUND_UP_BOX);
  quit->callback((Fl_Callback *) book_quit_CB);
  group->add(quit);

  Fl_Box *invis_2 = new Fl_Box(CX, 0, w() - 96 - 10 - CX, 20);
  add(invis_2);

  group->resizable(invis_2);

  // create the browser

  browser = new Fl_Hold_Browser(0, 0, w(), h() - 34);
  browser->callback((Fl_Callback *) book_selector_CB);
 
  add(browser);
  resizable(browser);

  // show the window
  set_modal();
  show();

  // read initial pos (same logic as in Guix_MainWin)
  WindowSmallDelay();
  
  init_x = x(); init_y = y();
  init_w = w(); init_h = h();
}


//
// Book Destructor
//
Guix_Book::~Guix_Book()
{
  // update preferences if user moved the window
  if (x() != init_x || y() != init_y)
  {
    guix_prefs.manual_x = x();
    guix_prefs.manual_y = y();
  }

  if (w() != init_w || h() != init_h)
  {
    guix_prefs.manual_w = w();
    guix_prefs.manual_h = h();
  }

  guix_prefs.manual_page = cur_page;
}


void Guix_Book::resize(int X, int Y, int W, int H)
{
  if (W != w() || H != h())
    want_reformat = TRUE;

  Fl_Window::resize(X, Y, W, H);
}


int Guix_Book::PageCount()
{
  // we don't need anything super efficient

  int count;
  
  for (count=0; book_pages[count].text; count++)
  { /* nothing */ }

  return count;
}


void Guix_Book::LoadPage(int new_num)
{
  if (new_num < 0 || new_num >= PageCount())
    return;

  cur_page = new_num;

  want_page = BOOK_NO_PAGE;
  want_reformat = FALSE;

  if (cur_page == 0)
    prev->deactivate();
  else
    prev->activate();

  if (cur_page == PageCount() - 1)
    next->deactivate();
  else
    next->activate();

  // -- create browser text --

  browser->clear();

  int i;
  const char ** lines = book_pages[cur_page].text;

  ParaStart();

  for (i=0; lines[i]; i++)
  {
    ParaAddLine(lines[i]);
  }

  ParaEnd();

  browser->position(0);
}


void Guix_Book::FollowLink(int line)
{
  if (line < 0)
  {
    browser->deselect();
    return;
  }

  const char * L = book_pages[cur_page].text[line];
  
  if (L[0] == '#' && L[1] == 'L')
  {
    want_page = (L[2] - '0') * 10 + (L[3] - '0');
  }
  else
  {
    // for lines without links, the best we can do is just clear the
    // hold selection.
    //
    browser->deselect();
  }
}


void Guix_Book::Update()
{
  if (want_page != BOOK_NO_PAGE)
    LoadPage(want_page);
  else if (want_reformat)
    LoadPage(cur_page);
}


//
//  PARAGRAPH CODE
//

void Guix_Book::ParaStart()
{
  in_paragraph = FALSE;
}


void Guix_Book::ParaEnd()
{
  if (in_paragraph && para_buf[0] != 0)
  {
    browser->add(para_buf);
  }

  in_paragraph = FALSE;
}


void Guix_Book::ParaAddWord(const char *word)
{
  // set current font for fl_width()
  fl_font(browser->textfont(), browser->textsize());
 
  // need to wrap ?
  if (para_buf[0] != 0 && (fl_width(para_buf) + fl_width(' ') +
      fl_width(word) > para_width))
  {
    browser->add(para_buf);
    para_buf[0] = 0;
    first_line = FALSE;
  }

  if (para_buf[0] == 0)
  {
    // prefix with spaces to indent paragraph
    int count = major_indent + (first_line ? 0 : minor_indent);

    para_buf[count] = 0;

    for (count--; count >= 0; count--)
      para_buf[count] = ' ';
  }
  else
    strcat(para_buf, " ");

  strcat(para_buf, word);
}


void Guix_Book::ParaAddLine(const char *line)
{
  if (line[0] == 0 || line[0] == '#' || line[0] == '@')
    ParaEnd();

  if (line[0] == '#' && line[1] == 'L')
  {
    browser->add(line + 4);
    return;
  }

  if (! in_paragraph)
  {
    if (! (line[0] == '#' && line[1] == 'P'))
    {
      browser->add(line);
      return;
    }
    
    major_indent = (line[2] - '0') * 6 + 1;
    minor_indent = (line[3] - '0');

    line += 4;

    in_paragraph = TRUE;
    first_line = TRUE;

    para_buf[0] = 0;
    para_width = browser->w() - 24 - (int)fl_width(' ') * major_indent;
  }
 
  // OK, we must be in paragraph mode here

  for (;;)
  {
    while (isspace(*line))
      line++;

    if (line[0] == 0)
      return;

    char word_buf[100];
    int word_len = 0;

    while (*line && word_len < 98)
    {
      // handle escapes:
      //  '#-'  : non-break space.
      //  '. '  : add a non-break space after dot

      if (line[0] == '#' && line[1] == '-')
      {
        word_buf[word_len++] = ' ';
        line += 2;
        continue;
      }
      else if (line[0] == '.' && line[1] == ' ')
      {
        word_buf[word_len++] = '.';
        word_buf[word_len++] = ' ';
        word_buf[word_len++] = ' ';
        line += 2;
        continue;
      }

      if (isspace(*line))
        break;

      word_buf[word_len++] = *line++;
    }

    word_buf[word_len] = 0;
    
    ParaAddWord(word_buf);
  }
}


//------------------------------------------------------------------------


Guix_License *guix_lic_win;

static void license_quit_CB(Fl_Widget *w, void *data)
{
  if (guix_lic_win)
    guix_lic_win->want_quit = TRUE;
}

//
// License Constructor
//
Guix_License::Guix_License() : Fl_Window(guix_prefs.manual_w,
    guix_prefs.manual_h, "glBSP License")
{
  // cancel the automatic 'begin' in Fl_Group constructor
  end();
 
  size_range(MANUAL_WINDOW_MIN_W, MANUAL_WINDOW_MIN_H);
  position(guix_prefs.manual_x, guix_prefs.manual_y);
  color(MAIN_BG_COLOR, MAIN_BG_COLOR);
 
  // allow manual closing of window
  callback((Fl_Callback *) license_quit_CB);

  want_quit = FALSE;

  // create close button in bottom row

  Fl_Box *invis_1 = new Fl_Box(0, 0, 30, 30);
  add(invis_1);
  
  group = new Fl_Group(0, h() - 34, w(), 34);
  group->resizable(invis_1);
  add(group);
  
  quit = new Fl_Button(w() - 96 - 24, h() - 30, 96, 26, "Close");
  quit->box(FL_ROUND_UP_BOX);
  quit->callback((Fl_Callback *) license_quit_CB);
  group->add(quit);

  // create the browser

  int i;

  browser = new Fl_Browser(0, 0, w(), h() - 34);
 
  for (i=0; license_text[i]; i++)
    browser->add(license_text[i]);

  browser->position(0);

  add(browser);
  resizable(browser);

  // show the window
  set_modal();
  show();

  // read initial pos (same logic as in Guix_MainWin)
  WindowSmallDelay();
  
  init_x = x(); init_y = y();
  init_w = w(); init_h = h();
}


//
// License Destructor
//
Guix_License::~Guix_License()
{
  // update preferences if user moved the window
  if (x() != init_x || y() != init_y)
  {
    guix_prefs.manual_x = x();
    guix_prefs.manual_y = y();
  }

  if (w() != init_w || h() != init_h)
  {
    guix_prefs.manual_w = w();
    guix_prefs.manual_h = h();
  }
}

