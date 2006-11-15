//----------------------------------------------------------------------------
//  EDGE SDL Controller Stuff
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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

#include "i_defs.h"
#include "i_sdlinc.h"

#include "e_main.h"
#include "e_event.h"
#include "v_res.h"

#include <ctype.h>

// FIXME: Combine all these SDL bool vars into an int/enum'd flags structure

// Work around for alt-tabbing
bool alt_is_down;
bool eat_mouse_motion = true;
bool use_grab = true;

#if defined(MACOSX) || defined(BeOS) || defined(WIN32)
bool use_warp_mouse = true;
#else
bool use_warp_mouse = false;
#endif

void HandleFocusGain();
void HandleFocusLost();
void HandleKeyEvent(SDL_Event* ev);
void HandleMouseButtonEvent(SDL_Event* ev);
void HandleMouseMotionEvent(SDL_Event* ev);
int TranslateSDLKey(int key);

//
// ActiveEventProcess
//
// Event handling while the application is active
//
void ActiveEventProcess(SDL_Event *sdl_ev)
{
	switch(sdl_ev->type)
	{
		case SDL_ACTIVEEVENT:
		{
			if ((sdl_ev->active.state & SDL_APPINPUTFOCUS) &&
				(sdl_ev->active.gain == 0))
			{
				HandleFocusLost();
			}
			
			break;
		}
		
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			HandleKeyEvent(sdl_ev);
			break;
				
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			HandleMouseButtonEvent(sdl_ev);
			break;
			
		case SDL_MOUSEMOTION:
			if (eat_mouse_motion) 
			{
				eat_mouse_motion = false; // One motion needs to be discarded
				break;
			}

			HandleMouseMotionEvent(sdl_ev);
			break;
		
		case SDL_QUIT:
			// Note we deliberate clear all other flags here. Its our method of 
			// ensuring nothing more is done with events.
			app_state = APP_STATE_PENDING_QUIT;
			break;
				
		default:
			break; // Don't care
	}
}

//
// InactiveEventProcess
//
// Event handling while the application is not active
//
void InactiveEventProcess(SDL_Event *sdl_ev)
{
	switch(sdl_ev->type)
	{
		case SDL_ACTIVEEVENT:
			if (app_state & APP_STATE_PENDING_QUIT)
				break; // Don't care: we're going to exit
			
			if (!sdl_ev->active.gain)
				break;
				
			if (sdl_ev->active.state & SDL_APPACTIVE ||
                sdl_ev->active.state & SDL_APPINPUTFOCUS)
				HandleFocusGain();
			break;

		case SDL_QUIT:
			// Note we deliberate clear all other flags here. Its our method of 
			// ensuring nothing more is done with events.
			app_state = APP_STATE_PENDING_QUIT;
			break;
					
		default:
			break; // Don't care
	}
}

//
// HandleFocusGain
//
void HandleFocusGain(void)
{
	if (use_grab)
	{
        // Hide cursor and grab input
	    SDL_ShowCursor(0);
        SDL_WM_GrabInput(SDL_GRAB_ON);
	}

	// Ignore any pending mouse motion
	eat_mouse_motion = true;

	// Now active again
	app_state |= APP_STATE_ACTIVE;
}

//
// HandleFocusLost
//
void HandleFocusLost(void)
{
	if (use_grab)
	{
        SDL_ShowCursor(1);
        SDL_WM_GrabInput(SDL_GRAB_OFF);
    }

	E_Idle();

	// No longer active
	app_state &= ~APP_STATE_ACTIVE;							
}

//
// HandleKeyEvent
//
void HandleKeyEvent(SDL_Event* ev)
{
	if (ev->type != SDL_KEYDOWN && ev->type != SDL_KEYUP) 
		return;

	event_t event;

	int sym = (int)ev->key.keysym.sym;

	// handle certain keys which don't behave normally
	if (sym == SDLK_CAPSLOCK || sym == SDLK_NUMLOCK)
	{
		event.type = ev_keydown;
		event.value.key = TranslateSDLKey(sym);
		E_PostEvent(&event);

		event.type = ev_keyup;
		E_PostEvent(&event);

		return;
	}

	event.type = (ev->type == SDL_KEYDOWN) ? ev_keydown : ev_keyup;
	event.value.key = TranslateSDLKey(sym);

	if (event.value.key < 0)
		return;

    if (event.value.key == KEYD_TAB && alt_is_down)
    {
        alt_is_down = false;
		return;
    }

	if (event.value.key == KEYD_LALT)
		alt_is_down = (event.type == ev_keydown);

	E_PostEvent(&event);
}

//
// HandleMouseButtonEvent
//
void HandleMouseButtonEvent(SDL_Event * ev)
{
	event_t event;
	
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
// HandleMouseMotionEvent
//
void HandleMouseMotionEvent(SDL_Event * ev)
{
	int dx, dy;

	if (use_warp_mouse)
	{
		// -DEL- 2001/01/29 SDL_WarpMouse doesn't work properly on beos so
		// calculate relative movement manually.

		dx = ev->motion.x - (SCREENWIDTH/2);
		dy = ev->motion.y - (SCREENHEIGHT/2);

		// don't warp if we don't need to
		if (dx || dy)
			I_CentreMouse();
	}
	else
	{
		dx = ev->motion.xrel;
		dy = ev->motion.yrel;
	}

	if (dx)
	{
		event_t event;

		event.type = ev_analogue;
		event.value.analogue.axis = mouse_xaxis;
		event.value.analogue.amount = dx * mouseSensitivity;
		
		E_PostEvent(&event);
	}

	if (dy)
	{
		if (!invertmouse)
			dy = -dy;

		event_t event;

		event.type = ev_analogue;
		event.value.analogue.axis = mouse_yaxis;
		event.value.analogue.amount = dy * mouseSensitivity;

		E_PostEvent(&event);
	}
}

//
// Translates a key from SDL -> EDGE
// Returns -1 if no suitable translation exists.
//
int TranslateSDLKey(int key)
{
	switch (key) 
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
		case SDLK_PRINT: return KEYD_PRTSCR;

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

		case SDLK_KP0: return '0';
		case SDLK_KP1: return '1';
		case SDLK_KP2: return '2';
		case SDLK_KP3: return '3';
		case SDLK_KP4: return '4';
		case SDLK_KP5: return '5';
		case SDLK_KP6: return '6';
		case SDLK_KP7: return '7';
		case SDLK_KP8: return '8';
		case SDLK_KP9: return '9';

		case SDLK_KP_PLUS:  return '+';
		case SDLK_KP_MINUS: return '-';
		case SDLK_KP_PERIOD: return '.';
		case SDLK_KP_ENTER: return KEYD_ENTER;
		case SDLK_KP_DIVIDE: return '/';
		case SDLK_KP_MULTIPLY:  return '*';
		case SDLK_KP_EQUALS: return '=';

		case SDLK_NUMLOCK:   return KEYD_NUMLOCK;
		case SDLK_SCROLLOCK: return KEYD_SCRLOCK;
		case SDLK_CAPSLOCK:  return KEYD_CAPSLOCK;
		case SDLK_PAUSE:     return KEYD_PAUSE;

		case SDLK_LSHIFT:
		case SDLK_RSHIFT: return KEYD_RSHIFT;
		case SDLK_LCTRL:
		case SDLK_RCTRL:  return KEYD_RCTRL;
		case SDLK_LMETA:
		case SDLK_LALT:   return KEYD_LALT;
		case SDLK_RMETA:
		case SDLK_RALT:   return KEYD_RALT;

		default: break;
	}

	if (key <= 0x7f)
		return tolower(key);

	return -1;
}

void I_CentreMouse(void)
{
	SDL_WarpMouse(SCREENWIDTH/2, SCREENHEIGHT/2);
}

/****** Input Event Generation ******/

void I_StartupControl(void)
{
	alt_is_down = false;
}

void I_ControlGetEvents(void)
{
	SDL_Event sdl_ev;

	while(SDL_PollEvent(&sdl_ev))
	{
		if (app_state & APP_STATE_ACTIVE)
			ActiveEventProcess(&sdl_ev);
		else
			InactiveEventProcess(&sdl_ev);		
	}
}

void I_ShutdownControl(void)
{
}


int I_GetTime(void)
{
    Uint32 t = SDL_GetTicks();

    // more complex than "t*35/1000" to give more accuracy
    return (t / 1000) * 35 + (t % 1000) * 35 / 1000;
}


//
// Same as I_GetTime, but returns time in milliseconds
//
int I_GetMillies(void)
{
    return SDL_GetTicks();
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
