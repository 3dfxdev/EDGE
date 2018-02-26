//----------------------------------------------------------------------------
//  Win32 GUI Header
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
#ifndef __WIN32_GUI_HEADER__
#define __WIN32_GUI_HEADER__

// ========================== STRUCTURES ========================== 

// App specific messages
#define WM_APPMSG     0x8000
#define WM_BOXCLOSED  (WM_APPMSG+1)

// Class Types
typedef enum
{
  CLTYPE_MAINAPP,
  CLTYPE_POPUP,
  NUMOFCLTYPES
}
classtype_e;

// RGB Type
typedef struct
{
  unsigned char red;
  unsigned char green;
  unsigned char blue;
}
rgb_t;

// Button Structure
typedef struct guibutton_s
{
  HWND parentwindow;                         // Parent Window
  HINSTANCE parentinst;                      // Parent Instance
  const char *text;                          // Text
  int x;                                     // X Position
  int y;                                     // Y Position
  int width;                                 // Width
  int height;                                // Height
  void (*action)(HWND wnd, DWORD msg);       // Action Procedure
}
guibutton_t;

// Edit Box Structure
typedef struct editbox_s
{
  HWND parentwindow;                        // Parent Window
  HINSTANCE parentinst;                     // Parent Instance
  int x;                                    // X Position
  int y;                                    // Y Position
  int width;                                // Width
  int height;                               // Height
  void (*action)(HWND wnd, DWORD msg);      // Action Procedure
}
editbox_t;

// File Dialog Structure
typedef struct filedialog_s
{
  HWND parentwindow;
  const char *title;
  const char *filters;
  const char *filternames;
  const char *startdir;
}
filedialog_t;

// Progress Bar Structure
typedef struct progressbar_s
{
  HWND parentwindow;     // Parent Window
  HINSTANCE parentinst;  // Parent Instance
  int x;                 // X Position
  int y;                 // Y Position
  int width;             // Width
  int height;            // Height
  int minrange;          // Minimum Range
  int maxrange;          // Maximum Range
  int step;              // Bar steps (gaps)
  int initpos;           // Initial Position
  rgb_t backcolour;      // Background colour
  rgb_t barcolour;       // Bar Colour
}
progressbar_t;

typedef struct guitext_s
{
  HWND parentwindow;     // Parent Window
  HINSTANCE parentinst;  // Parent Instance
  int x;                 // X Position
  int y;                 // Y Position
  int width;             // Width
  int height;            // Height
  const char *text;      // Initial text
}
guitext_t;

typedef struct
{
  HWND winhandle;
  long (*handler)(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
}
msgfunc_t;

// ========================== FUNCTIONS ========================== 

// Buttons
int I_GUICreateButton(guibutton_t* button);
boolean_g I_GUIButtonMsghandler(HWND window, DWORD message);
void I_GUIDestroyButton(int handle);

// Edit boxes
int I_GUICreateEditbox(editbox_t* editbox);
boolean_g I_GUISetReadOnlyEditbox(int handle, boolean_g enable);
boolean_g I_GUIEnableEditbox(int handle, boolean_g readonly);
boolean_g I_GUIGetEditboxText(int handle, char* str, int strsize);
boolean_g I_GUISetEditboxText(int handle, const char *str);
boolean_g I_GUIEditboxMsghandler(HWND window, DWORD message);
void I_GUIDestroyEditbox(int handle);

// Open Dialogs
const char *I_GUIOpenFileDialog(filedialog_t *filedlg);

// Progress Bars
int I_GUICreateBar(progressbar_t* prgrbar);
boolean_g I_GUIUpdateBar(int handle, int pos);
boolean_g I_GUIUpdateBarRange(int handle, int minr, int maxr);
boolean_g I_GUIStepBar(int handle);
void I_GUIDestroyBar(int handle);

// Text
int I_GUICreateText(guitext_t* guitext);
boolean_g I_GUISetText(int handle, const char *str);
void I_GUIDestroyText(int handle);

// Window Classes
boolean_g I_GUIRegisterClass(HINSTANCE instance, classtype_e classtype);
const char *I_GUIReturnClassName(classtype_e type);
boolean_g I_GUIAddMessageHandler(msgfunc_t *msgf);
boolean_g I_GUIRemoveMessageHandler(HWND hwnd);

#endif /* __WIN32_GUI_HEADER__ */

