//----------------------------------------------------------------------------
//  EDGE Linux Controller Stuff
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
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

#ifdef MACOSX
#include <SDL.h>
#else
#include <SDL/SDL.h>
#endif

#include "i_defs.h"

#include "dm_defs.h"
#include "m_argv.h"
#include "e_main.h"
#include "e_event.h"


extern bool use_warp_mouse;


/****** Input Event Generation ******/

void I_StartupControl(void)
{
  I_Printf("I_StartupJoystick\n");
//  I_StartupJoystick();

  I_Printf("I_StartupMouse\n");
//  I_StartupMouse();

  I_Printf("I_StartupKeyboard\n");
//  I_StartupKeyboard();

  return;
}

#if 0 //!!! actually in l_vid_x.c
void I_ControlGetEvents(void)
{
//  I_ReadMouse(usemouse);
//  I_ReadJoystick(usejoystick);
//  I_ReadKeyboard();
}
#endif

//
// I_ShutdownControl
//
// Return input devices to system control
//
void I_ShutdownControl(void)
{
  // I_ShutdownJoystick();
  // I_ShutdownMouse();
  // I_ShutdownKeyboard();
}

void I_CalibrateJoystick (int ch)
{
}


//
// Translates a key from SDL -> EDGE
// Returns -1 if no suitable translation exists.

static int TranslateSDLKey(SDL_KeyboardEvent ev)
{
  int label = ev.keysym.sym;

  switch (label) 
  {
    case SDLK_TAB: return KEYD_TAB;
    case SDLK_RETURN: return KEYD_ENTER;
    case SDLK_ESCAPE: return KEYD_ESCAPE;
    case SDLK_DELETE:
    case SDLK_BACKSPACE: return KEYD_BACKSPACE;

    case SDLK_UP:    return KEYD_UPARROW;
    case SDLK_DOWN:  return KEYD_DOWNARROW;
    case SDLK_LEFT:  return KEYD_LEFTARROW;
    case SDLK_RIGHT: return KEYD_RIGHTARROW;

    case SDLK_HOME:   return KEYD_HOME;
    case SDLK_END: return KEYD_END;
    case SDLK_PAGEUP:   return KEYD_PGUP;
    case SDLK_PAGEDOWN:   return KEYD_PGDN;
    case SDLK_INSERT: return KEYD_INSERT;
    case SDLK_PAUSE: return KEYD_PAUSE;

    case SDLK_F1:  return KEYD_F1;
    case SDLK_F2:  return KEYD_F2;
    case SDLK_F3:  return KEYD_F3;
    case SDLK_F4:  return KEYD_F4;
    case SDLK_F5:  return KEYD_F5;
    case SDLK_F6:  return KEYD_F6;
    case SDLK_F7:  return KEYD_F7;
    case SDLK_F8:  return KEYD_F8;
    case SDLK_F9:  return KEYD_F9;
    case SDLK_F10: return KEYD_F10;
    case SDLK_F11: return KEYD_F11;
    case SDLK_F12: return KEYD_F12;

    case SDLK_KP_PLUS:  return '+';
    case SDLK_KP_MINUS: return '-';
    case SDLK_KP_PERIOD: return '.';
    case SDLK_KP_ENTER: return KEYD_ENTER;
    case SDLK_KP_DIVIDE: return '/';
    case SDLK_KP_MULTIPLY:  return '*';
    case SDLK_KP_EQUALS: return '=';

    case SDLK_NUMLOCK:    return KEYD_NUMLOCK;
    case SDLK_SCROLLOCK: return KEYD_SCRLOCK;
    case SDLK_CAPSLOCK:   return KEYD_CAPSLOCK;

    case SDLK_LSHIFT:
    case SDLK_RSHIFT: return KEYD_RSHIFT;
    case SDLK_LCTRL:
    case SDLK_RCTRL:  return KEYD_RCTRL;
    case SDLK_LMETA:
    case SDLK_LALT:   return KEYD_LALT;
    case SDLK_RMETA:
    case SDLK_RALT:   return KEYD_RALT;
  }
  if (label <= 0x7f)
    return tolower(label);

  return -1;
}

static void HandleKeyEvent(SDL_Event* ev)
{
  event_t event;

  if (ev->type != SDL_KEYDOWN && ev->type != SDL_KEYUP) 
    return;

  event.type  = (ev->type == SDL_KEYDOWN) ? ev_keydown : ev_keyup;
  event.value.key = TranslateSDLKey(ev->key);

  if (event.value.key < 0)
    return;
  
  E_PostEvent(&event);
}

static void HandleMouseEvent(SDL_Event * ev)
{
  int dx, dy;
  event_t event;

  if (ev->type == SDL_MOUSEMOTION)
  {
    event.type = ev_analogue;

    if (use_warp_mouse)
    {
      // -DEL- 2001/01/29 SDL_WarpMouse doesn't work properly on beos so
      // calculate relative movement manually.

      dx = ev->motion.x - (SCREENWIDTH/2);
      dy = ev->motion.y - (SCREENHEIGHT/2);

      // don't warp if we don't need to
      if (dx || dy)
        SDL_WarpMouse(SCREENWIDTH/2, SCREENHEIGHT/2);
    }
    else
    {
      dx = ev->motion.xrel;
      dy = ev->motion.yrel;
    }

    if (dx)
    {
      event.value.analogue.axis = mouse_xaxis;
      event.value.analogue.amount = dx * mouseSensitivity;
      E_PostEvent(&event);
    }

    if (dy)
    {
      if (!invertmouse)
        dy = -dy;
       
      event.value.analogue.axis = mouse_yaxis;
      event.value.analogue.amount = dy * mouseSensitivity;
      E_PostEvent(&event);
    }

    return;
  }

  if (ev->type == SDL_MOUSEBUTTONDOWN) 
    event.type = ev_keydown;
  else if (ev->type == SDL_MOUSEBUTTONUP) 
    event.type = ev_keyup;
  else 
    return;

  switch (ev->button.button)
  {
    case 1:
      event.value.key = KEYD_MOUSE1; break;
      
    case 2:
      event.value.key = KEYD_MOUSE2; break;
      
    case 3:
      event.value.key = KEYD_MOUSE3; break;

    // handle the mouse wheel
    case 4: 
      event.value.key = KEYD_MWHEEL_UP; break; 
    
    case 5: 
      event.value.key = KEYD_MWHEEL_DN; break; 
    
    default:
      return;
  }

  E_PostEvent(&event);
}

//
// I_ControlGetEvents
//
void I_ControlGetEvents (void)
{
  SDL_Event ev;

  for (;;) 
  {
    if (SDL_PollEvent (&ev) == 0)
      break;

    HandleKeyEvent(&ev);
    HandleMouseEvent(&ev);
  }
}
