//----------------------------------------------------------------------------
//  Win32 GUI Class Functions
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

typedef struct
{
  const char *name;
  ATOM id;
}
_class_type_entry_t;

_class_type_entry_t classlist[] =
{
  { "MAINAPP",      0},
  { "POPUPWINDOW",  0}
};

typedef struct _wndproc_s
{
  HWND winhandle;
  long (*handler)(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
  boolean_g inuse;
  struct _wndproc_s *next;
}
_wndproc_t;

static _wndproc_t *prochead = NULL;

//
// ClassMessageHandler
//
long FAR PASCAL ClassMessageHandler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  _wndproc_t *currwndproc;

  switch (message)
  {
    case WM_BOXCLOSED: // Specific to this app
    case WM_CLOSE:
    case WM_COMMAND:
    case WM_DESTROY:
    {
      currwndproc = prochead;
      while (currwndproc)
      {
        if (currwndproc->inuse && currwndproc->winhandle == hwnd)
          return currwndproc->handler(hwnd, message, wParam, lParam);

        currwndproc = currwndproc->next;
      }

      break;
    }
  }

  return DefWindowProc(hwnd, message, wParam, lParam);
}

//
// I_RegisterClasses
//
boolean_g I_GUIRegisterClass(HINSTANCE instance, classtype_e classtype)
{
  WNDCLASS wc;

  if (classtype == CLTYPE_MAINAPP)
  {
    if (classlist[CLTYPE_MAINAPP].id)
      return true;

    // Set up and register window class
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = ClassMessageHandler;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = instance;
    wc.hIcon         = LoadIcon(instance, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = classlist[CLTYPE_MAINAPP].name;

    classlist[CLTYPE_MAINAPP].id = RegisterClass(&wc);
    if(!classlist[CLTYPE_MAINAPP].id)
      return false;
  }

  if (classtype == CLTYPE_POPUP)
  {
    if (classlist[CLTYPE_POPUP].id)
      return true;

    // Set up and register window class
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = ClassMessageHandler;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = instance;
    wc.hIcon         = LoadIcon(instance, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = classlist[CLTYPE_POPUP].name;

    classlist[CLTYPE_POPUP].id = RegisterClass(&wc);
    if(!classlist[CLTYPE_POPUP].id)
      return false;
  }

  return true;
}

//
// I_GUIReturnClassName
//
const char *I_GUIReturnClassName(classtype_e type)
{
  if (type<0 || type>=NUMOFCLTYPES)
    return NULL;

  return classlist[type].name;
}

//
// I_GUIAddMessageHandler
//
boolean_g I_GUIAddMessageHandler(msgfunc_t *msgf)
{
  _wndproc_t *newwndproc;
  _wndproc_t *searwndproc;

  if (prochead != NULL)
  {
    searwndproc = prochead;
    while (searwndproc != NULL)
    {
      // Check not already added
      if (searwndproc->inuse && searwndproc->winhandle == msgf->winhandle)
        return false;

      // Check for spare entry we can use
      if (!searwndproc->inuse && searwndproc->handler == msgf->handler)
      {
        searwndproc->winhandle = msgf->winhandle;
        searwndproc->inuse     = true;
        return true;
      }

      searwndproc = searwndproc->next;
    }
  }

  newwndproc = malloc(sizeof(_wndproc_t));
  if (!newwndproc)
    return false;

  newwndproc->winhandle = msgf->winhandle;
  newwndproc->handler   = msgf->handler;

  if (prochead != NULL)
    newwndproc->next = prochead;

  prochead = newwndproc;
  prochead->inuse = true;
  return true;
}

//
// I_GUIRemoveMessageHandler
//
boolean_g I_GUIRemoveMessageHandler(HWND hwnd)
{
  _wndproc_t *searwndproc;

  searwndproc = prochead;
  while (searwndproc != NULL)
  {
    // Check not already added
    if (searwndproc->inuse && searwndproc->winhandle == hwnd)
    {
      searwndproc->inuse = false;
      return true;
    }

    searwndproc = searwndproc->next;
  }

  return false;
}
