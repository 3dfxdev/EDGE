//----------------------------------------------------------------------------
//  WIN32 GUI Controls
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2000  Andy Baker
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
//----------------------------------------------------------------------------
//
#include "gui.h"

#define MAXFILENAME 512

//
// BUTTON
//
typedef struct
{
  HWND buttonhwnd;
  void (*action)(HWND wnd, DWORD msg);   // Action Procedure
}
_guibutt_t;

//
// EDIT BOX
//
typedef struct
{
  HWND boxhwnd;                          // Box Handle
  void (*action)(HWND wnd, DWORD msg);   // Action Procedure
}
_editbox_t;

//
// PROGRESS BAR
//
typedef struct
{
  HWND barhwnd;
  int minrange;
  int maxrange;
}
_guipgrsbar_t;

//
// TEXT BAR
//
typedef struct
{
  HWND txthwnd;
}
_guitext_t;

// Button List
static _guibutt_t **guibuttlist;
static int numofguibutts = 0;

// EditBox List
static _editbox_t **editboxlist;
static int numofeditboxs = 0;

// ProgressBar List
static _guipgrsbar_t **pgrsbarlist;
static int numofpgrsbars = 0;

// Text List
static _guitext_t **txtlist;
static int numoftxts = 0;

// ============================== BUTTON ==============================

//
// I_GUICreateButton
//
int I_GUICreateButton(guibutton_t* button)
{
  _guibutt_t **oldguibuttlist;
  HWND butthandle;
  int i;

  if (numofguibutts && guibuttlist)
  {
    i=0;
    while (i<numofguibutts && guibuttlist[i])
      i++;

    if (i==numofguibutts)
    {
      oldguibuttlist = guibuttlist;

      guibuttlist = malloc(sizeof(_guibutt_t*)*(numofguibutts+1));
      if (!guibuttlist)
        return 0;

      memset(guibuttlist, 0, sizeof(_guibutt_t*)*(numofguibutts+1));
      memcpy(guibuttlist, oldguibuttlist, sizeof(_guibutt_t*)*(numofguibutts));

      free(oldguibuttlist);

      i = numofguibutts; 
      numofguibutts++;
    }
  }
  else
  {
    guibuttlist = malloc(sizeof(_guibutt_t*));

    if (!guibuttlist)
      return 0;

    numofguibutts = 1;
    i = 0;
  }

  guibuttlist[i] = malloc(sizeof(_guibutt_t));
  if (!guibuttlist[i])
    return 0;

  butthandle = CreateWindowEx(0, "BUTTON",
                             (LPSTR) NULL,
                             BS_VCENTER|BS_DEFPUSHBUTTON|WS_CHILD|WS_VISIBLE,
                             button->x,
                             button->y,
                             button->width,
                             button->height,
                             button->parentwindow,
                             (HMENU) NULL,
                             button->parentinst,
                             NULL);

  guibuttlist[i]->action = button->action;

  if (button->text)
    SetWindowText(butthandle, button->text);

  guibuttlist[i]->buttonhwnd = butthandle;

  return i;
}

//
// I_GUIButtonMsghandler
//
boolean_g I_GUIButtonMsghandler(HWND window, DWORD message)
{
  int i;

  i=0;
  while (i<numofguibutts)
  {
    if (guibuttlist[i] && (guibuttlist[i]->buttonhwnd == window))
    {
      if (guibuttlist[i]->action)
        guibuttlist[i]->action(window, message);

      return true;
    }

    i++;
  }

  return false;
}

//
// I_GUIDestroyButton
//
void I_GUIDestroyButton(int handle)
{
  if (handle<0 || handle>=numofguibutts)
    return;

  if (!guibuttlist[handle])
    return;

  DestroyWindow(guibuttlist[handle]->buttonhwnd);

  free(guibuttlist[handle]);
  guibuttlist[handle] = NULL;
}

// ============================== EDIT BOX ==============================

//
// I_GUICreateEditbox
//
int I_GUICreateEditbox(editbox_t* editbox)
{
  _editbox_t **oldeditboxlist;
  HWND boxhandle;
  int i;

  if (numofeditboxs && editboxlist)
  {
    i=0;
    while (i<numofeditboxs && editboxlist[i])
      i++;

    if (i==numofeditboxs)
    {
      oldeditboxlist = editboxlist;

      editboxlist = malloc(sizeof(_editbox_t*)*(numofeditboxs+1));
      if (!editboxlist)
        return 0;

      memset(editboxlist, 0, sizeof(_editbox_t*)*(numofeditboxs+1));
      memcpy(editboxlist, oldeditboxlist, sizeof(_editbox_t*)*(numofeditboxs));

      free(oldeditboxlist);

      i = numofeditboxs; 
      numofeditboxs++;
    }
  }
  else
  {
    editboxlist = malloc(sizeof(_editbox_t*));

    if (!editboxlist)
      return 0;

    numofeditboxs = 1;
    i = 0;
  }

  editboxlist[i] = malloc(sizeof(_editbox_t));
  if (!editboxlist[i])
    return 0;

  boxhandle = CreateWindowEx(0, "EDIT",
                             (LPSTR) NULL,
                             WS_BORDER|WS_CHILD|WS_VISIBLE,
                             editbox->x,
                             editbox->y,
                             editbox->width,
                             editbox->height,
                             editbox->parentwindow,
                             (HMENU) NULL,
                             editbox->parentinst,
                             NULL);

  editboxlist[i]->action = editbox->action;
  editboxlist[i]->boxhwnd = boxhandle;

  return i;
}

//
// I_GUIEnableEditbox
//
boolean_g I_GUIEnableEditbox(int handle, boolean_g enable)
{
  if (handle<0 || handle>=numofeditboxs)
    return false;

  if (!editboxlist[handle])
    return false;

  if (enable)
    EnableWindow(editboxlist[handle]->boxhwnd, TRUE);
  else
    EnableWindow(editboxlist[handle]->boxhwnd, FALSE);

  return true;
}

//
// I_GUISetReadOnlyEditbox
//
boolean_g I_GUISetReadOnlyEditbox(int handle, boolean_g enable)
{
  if (handle<0 || handle>=numofeditboxs)
    return false;

  if (!editboxlist[handle])
    return false;

  if (enable)
    SendMessage(editboxlist[handle]->boxhwnd, EM_SETREADONLY, (WPARAM)(BOOL)TRUE, 0L);
  else
    SendMessage(editboxlist[handle]->boxhwnd, EM_SETREADONLY, (WPARAM)(BOOL)FALSE, 0L);

  return true;
}

//
// I_GUIGetEditboxText
//
boolean_g I_GUIGetEditboxText(int handle, char *str, int strsize)
{
  if (handle<0 || handle>=numofeditboxs)
    return false;

  if (!editboxlist[handle])
    return false;

  // No I don't like this either: Windozes for you
  *(int*)str = strsize;
  SendMessage(editboxlist[handle]->boxhwnd, EM_GETLINE, 0, (LPARAM)str); 
  return true;
}

//
// I_GUISetEditboxText
//
boolean_g I_GUISetEditboxText(int handle, const char *str)
{
  if (handle<0 || handle>=numofeditboxs)
    return false;

  if (!editboxlist[handle])
    return false;

  SendMessage(editboxlist[handle]->boxhwnd, WM_SETTEXT, 0, (LPARAM)str); 
  return true;
}

//
// I_GUIEditboxMsghandler
//
boolean_g I_GUIEditboxMsghandler(HWND window, DWORD message)
{
  int i;

  i=0;
  while (i<numofeditboxs)
  {
    if (editboxlist[i] && (editboxlist[i]->boxhwnd == window))
    {
      if (editboxlist[i]->action)
        editboxlist[i]->action(window, message);

      return true;
    }

    i++;
  }

  return false;
}

//
// I_GUIDestroyEditbox
//
void I_GUIDestroyEditbox(int handle)
{
  if (handle<0 || handle>=numofeditboxs)
    return;

  if (!editboxlist[handle])
    return;

  DestroyWindow(editboxlist[handle]->boxhwnd);

  free(editboxlist[handle]);
  editboxlist[handle] = NULL;
}

// ======================== OPENFILE DIALOG ========================

//
// I_GUIOpenFileDialog
//
const char *I_GUIOpenFileDialog(filedialog_t *filedlg)
{
  OPENFILENAME opendialog;
  char *filename;
  char *filter;
  int fpos, fnpos;
  int flen, fnlen;
  int filterlen, filterpos;
  int i;

  if (!filedlg)
    return NULL;

  // Construct filter
  flen = strlen(filedlg->filters);
  fnlen = strlen(filedlg->filternames);

  filterlen = fnlen+flen+2;
  filter = malloc(sizeof(char)*(filterlen+1)); // include treble NULL
  if (!filter)
    return NULL;
  memset(filter, 0, sizeof(char)*(filterlen+1));

  filterpos = 0; // Construct Filter Position
  fpos      = 0; // Filters Position
  fnpos     = 0; // Filter Names Position

  // Add Filter Name
  while (filterpos<filterlen)
  {
    while (filedlg->filternames[fnpos] != ',' &&
                               filedlg->filternames[fnpos] != '\0')
    {
      filter[filterpos] = filedlg->filternames[fnpos];
      filterpos++;
      fnpos++;
    }
  
    // Skip Comma
    if (filedlg->filternames[fnpos] == ',')
      fnpos++;

    // Add NULL Split
    filter[filterpos] = '\0';
    filterpos++; 

    // Add Filter Type
    while (filedlg->filters[fpos] != ',' && filedlg->filters[fpos] != '\0')
    {
      filter[filterpos] = filedlg->filters[fpos];
      filterpos++;
      fpos++;
    }

    // Skip Comma
    if (filedlg->filters[fpos] == ',')
      fpos++;

    // Add NULL Split
    filter[filterpos] = '\0';
    filterpos++; 
  }

  filename = malloc(sizeof(char)*(MAXFILENAME+1));
  if (!filename)
    return NULL;
  memset(filename, 0, sizeof(char)*(MAXFILENAME+1));

  opendialog.lStructSize       = OPENFILENAME_SIZE_VERSION_400;
  opendialog.hwndOwner         = filedlg->parentwindow; 
  opendialog.hInstance         = NULL;
  opendialog.lpstrFilter       = filter; 
  opendialog.lpstrCustomFilter = (LPTSTR) NULL; 
  opendialog.nMaxCustFilter    = 0L; 
  opendialog.nFilterIndex      = 1L; 
  opendialog.lpstrFile         = filename; 
  opendialog.nMaxFile          = sizeof(char)*MAXFILENAME; 
  opendialog.lpstrFileTitle    = NULL; 
  opendialog.nMaxFileTitle     = 0; 
  opendialog.lpstrInitialDir   = filedlg->startdir; 
  opendialog.lpstrTitle        = filedlg->title;
  opendialog.nFileOffset       = 0; 
  opendialog.nFileExtension    = 0; 
  opendialog.lpstrDefExt       = NULL;  // No default extension 
  opendialog.lCustData         = 0; 
 
  opendialog.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | 
                     OFN_HIDEREADONLY | OFN_LONGNAMES; 
  
  if (!GetOpenFileName(&opendialog))
    return NULL;

  return filename;
}

// ======================== PROGRESS BAR ========================

//
// I_GUICreateBar
//
int I_GUICreateBar(progressbar_t* prgrbar)
{
  _guipgrsbar_t **oldpgrsbarlist;
  HWND barhandle;
  COLORREF crf;
  int i;

  if (numofpgrsbars && pgrsbarlist)
  {
    i=0;
    while (i<numofpgrsbars && pgrsbarlist[i])
      i++;

    if (i==numofpgrsbars)
    {
      oldpgrsbarlist = pgrsbarlist;

      pgrsbarlist = malloc(sizeof(_guipgrsbar_t*)*(numofpgrsbars+1));
      if (!pgrsbarlist)
        return 0;

      memset(pgrsbarlist, 0, sizeof(_guipgrsbar_t*)*(numofpgrsbars+1));
      memcpy(pgrsbarlist, oldpgrsbarlist, sizeof(_guipgrsbar_t*)*(numofpgrsbars));

      free(oldpgrsbarlist);

      i = numofpgrsbars; 
      numofpgrsbars++;
    }
  }
  else
  {
    pgrsbarlist = malloc(sizeof(_guipgrsbar_t*));

    if (!pgrsbarlist)
      return 0;

    numofpgrsbars = 1;
    i = 0;
  }

  pgrsbarlist[i] = malloc(sizeof(_guipgrsbar_t));
  if (!pgrsbarlist[i])
    return 0;

  barhandle = CreateWindowEx(0, PROGRESS_CLASS,
                             (LPSTR) NULL,
                             PBS_SMOOTH|WS_CHILD|WS_VISIBLE,
                             prgrbar->x,
                             prgrbar->y,
                             prgrbar->width,
                             prgrbar->height,
                             prgrbar->parentwindow,
                             (HMENU) NULL,
                             prgrbar->parentinst,
                             NULL);

  if (!barhandle)
  {
    //
    // No need to make the pointer table any smaller - just
    // free the bar struct
    //
    free(pgrsbarlist[i]);
    return 0;
  }

  // Set the range
  SendMessage(barhandle,
              PBM_SETRANGE32,
              prgrbar->minrange,
              prgrbar->maxrange);

  // Set the initial position
  SendMessage(barhandle, PBM_SETPOS, prgrbar->initpos, 0);

  // Set the step
  SendMessage(barhandle, PBM_SETSTEP, prgrbar->step, 0);

  // Set the bar colour
  crf = RGB(prgrbar->barcolour.red,
            prgrbar->barcolour.green,
            prgrbar->barcolour.blue);

  SendMessage(barhandle, PBM_SETBARCOLOR, 0, crf);

  // Set the background colour
  crf = RGB(prgrbar->backcolour.red,
            prgrbar->backcolour.green,
            prgrbar->backcolour.blue);

  SendMessage(barhandle, PBM_SETBKCOLOR, 0, crf);

  // Setup structure
  pgrsbarlist[i]->barhwnd = barhandle;
  pgrsbarlist[i]->minrange = prgrbar->minrange;
  pgrsbarlist[i]->maxrange = prgrbar->maxrange;

  return i;
}

//
// I_GUIStepBar
//
boolean_g I_GUIStepBar(int handle)
{
  if (handle<0 || handle>=numofpgrsbars)
    return false;

  if (!pgrsbarlist[handle])
    return false;

  SendMessage(pgrsbarlist[handle]->barhwnd, PBM_STEPIT, 0, 0);

  return true;
}

//
// I_GUIUpdateBar
//
boolean_g I_GUIUpdateBar(int handle, int pos)
{
  if (handle<0 || handle>=numofpgrsbars)
    return false;

  if (!pgrsbarlist[handle])
    return false;

  if (pgrsbarlist[handle]->minrange>pos || pgrsbarlist[handle]->maxrange<pos)
    return false;

  SendMessage(pgrsbarlist[handle]->barhwnd, PBM_SETPOS, pos, 0);

  return true;
}

//
// I_GUIUpdateBarRange
//
boolean_g I_GUIUpdateBarRange(int handle, int minr, int maxr)
{
  if (handle<0 || handle>=numofpgrsbars)
    return false;

  if (!pgrsbarlist[handle])
    return false;

  pgrsbarlist[handle]->minrange = minr;
  pgrsbarlist[handle]->maxrange = maxr;

  // Set the range
  SendMessage(pgrsbarlist[handle]->barhwnd,
              PBM_SETRANGE32,
              pgrsbarlist[handle]->minrange,
              pgrsbarlist[handle]->maxrange);

  return true;
}

//
// I_GUIDestroyBar
//
void I_GUIDestroyBar(int handle)
{
  if (handle<0 || handle>=numofpgrsbars)
    return;

  if (!pgrsbarlist[handle])
    return;

  DestroyWindow(pgrsbarlist[handle]->barhwnd);

  free(pgrsbarlist[handle]);
  pgrsbarlist[handle] = NULL;
}


// ======================== TEXT ========================

//
// I_GUICreateText
//
int I_GUICreateText(guitext_t* gtext)
{
  _guitext_t **oldtxtlist;
  HWND txthandle;
  COLORREF crf;
  int i;

  if (numoftxts && txtlist)
  {
    i=0;
    while (i<numoftxts && txtlist[i])
      i++;

    if (i==numoftxts)
    {
      oldtxtlist = txtlist;

      txtlist = malloc(sizeof(_guitext_t*)*(numoftxts+1));
      if (!txtlist)
        return 0;

      memset(txtlist, 0, sizeof(_guitext_t*)*(numoftxts+1));
      memcpy(txtlist, oldtxtlist, sizeof(_guitext_t*)*(numoftxts));

      free(oldtxtlist);

      i = numoftxts; 
      numoftxts++;
    }
  }
  else
  {
    txtlist = malloc(sizeof(_guitext_t*));

    if (!txtlist)
      return 0;

    numoftxts = 1;
    i = 0;
  }

  txtlist[i] = malloc(sizeof(_guitext_t));
  if (!txtlist[i])
    return 0;

  txthandle = CreateWindowEx(0, "STATIC",
                             (LPSTR)gtext->text,
                             SS_LEFT|WS_CHILD|WS_VISIBLE,
                             gtext->x,
                             gtext->y,
                             gtext->width,
                             gtext->height,
                             gtext->parentwindow,
                             (HMENU) NULL,
                             gtext->parentinst,
                             NULL);

  if (!txthandle)
  {
    //
    // No need to make the pointer table any smaller - just
    // free the text struct
    //
    free(txtlist[i]);
    return 0;
  }

  // Setup structure
  txtlist[i]->txthwnd = txthandle;

  return i;
}

//
// I_GUISetText
//
boolean_g I_GUISetText(int handle, const char *str)
{
  if (handle<0 || handle>=numoftxts)
    return false;

  if (!txtlist[handle])
    return false;

  SendMessage(txtlist[handle]->txthwnd, WM_SETTEXT, 0, (LPARAM)str); 
  return true;
}

//
// I_GUIDestroyText
//
void I_GUIDestroyText(int handle)
{
  if (handle<0 || handle>=numoftxts)
    return;

  if (!txtlist[handle])
    return;

  DestroyWindow(txtlist[handle]->txthwnd);

  free(txtlist[handle]);
  txtlist[handle] = NULL;
}
