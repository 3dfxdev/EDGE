//------------------------------------------------------------------------
// TEXTBOX : Unix/FLTK Text messages
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


//
// TextBox Constructor
//
Guix_TextBox::Guix_TextBox(int x, int y, int w, int h) :
    Fl_Multi_Browser(x, y, w, h)
{
  // cancel the automatic 'begin' in Fl_Group constructor
  // (it's an ancestor of Fl_Browser).
  end();
 
  textfont(FL_COURIER);
  textsize(14);

#ifdef MACOSX
  // the resize box in the lower right corner is a pain, it ends up
  // covering the text box scroll button.  Luckily when both scrollbars
  // are active, FLTK leaves a square space in that corner and it
  // fits in nicely.
  has_scrollbar(BOTH_ALWAYS);
#endif
}


//
// TextBox Destructor
//
Guix_TextBox::~Guix_TextBox()
{
  // nothing to do
}


void Guix_TextBox::AddMsg(const char *msg, Fl_Color col, // = FL_BLACK,
    boolean_g bold) // = FALSE)
{
  const char *r;

  char buffer[2048];
  int b_idx;

  // setup formatting string
  buffer[0] = 0;

  if (col != FL_BLACK)
    sprintf(buffer, "@C%d", col);

  if (bold)
    strcat(buffer, "@b");
  
  strcat(buffer, "@.");
  b_idx = strlen(buffer);

  while ((r = strchr(msg, '\n')) != NULL)
  {
    strncpy(buffer+b_idx, msg, r - msg);
    buffer[b_idx + r - msg] = 0;

    if (r - msg == 0)
    {
      // workaround for FLTK bug
      strcpy(buffer+b_idx, " ");
    }

    add(buffer);
    msg = r+1;
  }

  if (strlen(msg) > 0)
  {
    strcpy(buffer+b_idx, msg);
    add(buffer);
  }

  // move browser to last line
  if (size() > 0)
    bottomline(size());
}


void Guix_TextBox::AddHorizBar()
{
  add(" ");
  add(" ");
  add("@-");
  add(" ");
  add(" ");
 
  // move browser to last line
  if (size() > 0)
    bottomline(size());
}


void Guix_TextBox::ClearLog()
{
  clear();
}


boolean_g Guix_TextBox::SaveLog(const char *filename)
{
  FILE *fp = fopen(filename, "w");

  if (! fp)
    return FALSE;

  for (int y=1; y <= size(); y++)
  {
    const char *L_txt = text(y);

    if (! L_txt)
    {
      fprintf(fp, "\n");
      continue;
    }
    
    if (L_txt[0] == '@' && L_txt[1] == '-')
    {
      fprintf(fp, "--------------------------------");
      fprintf(fp, "--------------------------------\n");
      continue;
    }

    // remove any '@' formatting info

    while (*L_txt == '@')
    {
      L_txt++;
      
      if (*L_txt == 0)
        break;

      char fmt_ch = *L_txt++;

      if (fmt_ch == '.')
        break;

      // uppercase formatting chars (e.g. @C) have an int argument
      if (isupper(fmt_ch))
      {
        while (isdigit(*L_txt))
          L_txt++;
      }
    }

    fprintf(fp, "%s\n", L_txt);
  }
  
  fclose(fp);

  return TRUE;
}


void Guix_TextBox::LockOut(boolean_g lock_it)
{
  // Don't need to lock the text box.  This routine is for
  // completeness, e.g. in case some aspect of the text box should
  // actually be locked.
}


//------------------------------------------------------------------------

//
// GUI_PrintMsg
//
void GUI_PrintMsg(const char *str, ...)
{
  char buffer[2048];
  
  va_list args;

  va_start(args, str);
  vsprintf(buffer, str, args);
  va_end(args);

  // handle pre-windowing text (ShowOptions and friends)
  if (! guix_win)
  {
    printf("%s", buffer);
    fflush(stdout);
    return;
  }

  if (strncmp(buffer, "ATTENTION", 9) == 0)
    guix_win->text_box->AddMsg(buffer, FL_RED, TRUE);
  else if (strncmp(buffer, "Warning", 7) == 0)
    guix_win->text_box->AddMsg(buffer, FL_RED);
  else
    guix_win->text_box->AddMsg(buffer);
}

