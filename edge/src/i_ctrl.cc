//----------------------------------------------------------------------------
//  EDGE SDL Controller Stuff
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2008  The EDGE Team.
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

#include "g_state.h"
#include "e_event.h"
#include "e_input.h"
#include "e_main.h"
#include "m_argv.h"
#include "r_modes.h"


cvar_c mouse_xaxis, mouse_xsens;
cvar_c mouse_yaxis, mouse_ysens;

cvar_c mouse_invert;


bool nojoy;  // what a wowser, joysticks completely disabled

static int num_joys;
static int cur_joy;
static SDL_Joystick *joy_info;

cvar_c joy_enable;  // 0 = disabled, 1/2/3/etc selects a joystick

cvar_c joy_xaxis, joy_xsens, joy_xdead;
cvar_c joy_yaxis, joy_ysens, joy_ydead;


#undef DEBUG_KB
#undef DEBUG_JOY


cvar_c in_warpmouse;
cvar_c in_keypad;


// Work around for alt-tabbing
static bool alt_is_down;
static bool eat_mouse_motion = true;


//
// Translates a key from SDL -> EDGE
// Returns KEYD_IGNORE if no suitable translation exists.
//
int TranslateSDLKey(int key)
{
	// if keypad is not wanted, convert to normal keys
	if (! in_keypad.d)
	{
		if (SDLK_KP0 <= key && key <= SDLK_KP9)
			return '0' + (key - SDLK_KP0);

		switch (key)
		{
			case SDLK_KP_PLUS:   return '+';
			case SDLK_KP_MINUS:  return '-';
			case SDLK_KP_PERIOD: return '.';
			case SDLK_KP_MULTIPLY: return '*';
			case SDLK_KP_DIVIDE: return '/';
			case SDLK_KP_EQUALS: return '=';
			case SDLK_KP_ENTER:  return KEYD_ENTER;

			default: break;
		}
	}

	if (SDLK_F1 <= key && key <= SDLK_F12)
		return KEYD_F1 + (key - SDLK_F1);

	if (SDLK_KP0 <= key && key <= SDLK_KP9)
		return KEYD_KP0 + (key - SDLK_KP0);

	switch (key) 
	{
		case SDLK_TAB:    return KEYD_TAB;
		case SDLK_RETURN: return KEYD_ENTER;
		case SDLK_ESCAPE: return KEYD_ESCAPE;
		case SDLK_BACKSPACE: return KEYD_BACKSPACE;

		case SDLK_UP:    return KEYD_UPARROW;
		case SDLK_DOWN:  return KEYD_DOWNARROW;
		case SDLK_LEFT:  return KEYD_LEFTARROW;
		case SDLK_RIGHT: return KEYD_RIGHTARROW;

		case SDLK_HOME:     return KEYD_HOME;
		case SDLK_END:      return KEYD_END;
		case SDLK_INSERT:   return KEYD_INSERT;
		case SDLK_DELETE:   return KEYD_DELETE;
		case SDLK_PAGEUP:   return KEYD_PGUP;
		case SDLK_PAGEDOWN: return KEYD_PGDN;

		case SDLK_KP_PERIOD:   return KEYD_KP_DOT;
		case SDLK_KP_PLUS:     return KEYD_KP_PLUS;
		case SDLK_KP_MINUS:    return KEYD_KP_MINUS;
		case SDLK_KP_MULTIPLY: return KEYD_KP_STAR;
		case SDLK_KP_DIVIDE:   return KEYD_KP_SLASH;
		case SDLK_KP_EQUALS:   return KEYD_KP_EQUAL;
		case SDLK_KP_ENTER:    return KEYD_KP_ENTER;

		case SDLK_PRINT:     return KEYD_PRINT;
		case SDLK_CAPSLOCK:  return KEYD_CAPSLOCK;
		case SDLK_NUMLOCK:   return KEYD_NUMLOCK;
		case SDLK_SCROLLOCK: return KEYD_SCRLOCK;
		case SDLK_PAUSE:     return KEYD_PAUSE;

		case SDLK_LSHIFT:
		case SDLK_RSHIFT: return KEYD_SHIFT;
		case SDLK_LCTRL:
		case SDLK_RCTRL:  return KEYD_CTRL;
		case SDLK_LALT:
		case SDLK_RALT:   return KEYD_ALT;
		case SDLK_LMETA:
		case SDLK_RMETA:  return KEYD_ALT;

		default: break;
	}

	if (key <= 0x7f)
		return toupper(key);

	return KEYD_IGNORE;
}


void HandleFocusGain(void)
{
	// Hide cursor and grab input
	I_GrabCursor(true);

	// Ignore any pending mouse motion
	eat_mouse_motion = true;

	// Now active again
	app_state |= APP_STATE_ACTIVE;
}


void HandleFocusLost(void)
{
	I_GrabCursor(false);

	E_Idle();

	// No longer active
	app_state &= ~APP_STATE_ACTIVE;							
}


void HandleKeyEvent(SDL_Event * ev)
{
	if (ev->type != SDL_KEYDOWN && ev->type != SDL_KEYUP) 
		return;

#ifdef DEBUG_KB
	if (ev->type == SDL_KEYDOWN)
		L_WriteDebug("  HandleKey: DOWN\n");
	else if (ev->type == SDL_KEYUP)
		L_WriteDebug("  HandleKey: UP\n");
#endif

	int sym = (int)ev->key.keysym.sym;

	event_t event;
	event.value.key.sym = TranslateSDLKey(sym);
	event.value.key.unicode = ev->key.keysym.unicode;

	// handle certain keys which don't behave normally
	if (sym == SDLK_CAPSLOCK || sym == SDLK_NUMLOCK)
	{
#ifdef DEBUG_KB
		L_WriteDebug("   HandleKey: CAPS or NUMLOCK\n");
#endif

		// -AJA- for some reason (perhaps not SDL's fault), the CAPSLOCK
		//       key behaves differently on Win32 and Linux.  Under Win32
		//       we get the "long press" behaviour, but on Linux we get
		//       "faked key-ups" behaviour.  Oi oi oi.
#ifdef LINUX
		if (ev->type != SDL_KEYDOWN)
			return;
#endif
		event.type = ev_keydown;
		E_PostEvent(&event);

		event.type = ev_keyup;
		E_PostEvent(&event);
		return;
	}

	event.type = (ev->type == SDL_KEYDOWN) ? ev_keydown : ev_keyup;

#ifdef DEBUG_KB
	L_WriteDebug("   HandleKey: sym=%d scan=%d unicode=%d --> key=%d\n",
			sym, ev->key.keysym.scancode, ev->key.keysym.unicode, event.value.key.sym);
#endif

	if (event.value.key.sym == KEYD_IGNORE &&
	    event.value.key.unicode == 0)
	{
		// No translation possible for SDL symbol and no unicode value
		return;
	}

    if (event.value.key.sym == KEYD_TAB && alt_is_down)
    {
#ifdef DEBUG_KB
		L_WriteDebug("   HandleKey: ALT-TAB\n");
#endif
        alt_is_down = false;
		return;
    }

	if (event.value.key.sym == KEYD_ALT)
		alt_is_down = (event.type == ev_keydown);

	E_PostEvent(&event);
}


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
			event.value.key.sym = KEYD_MOUSE1; break;

		case 2:
			event.value.key.sym = KEYD_MOUSE2; break;

		case 3:
			event.value.key.sym = KEYD_MOUSE3; break;

		// handle the mouse wheel
		case 4: 
			event.value.key.sym = KEYD_MWHEEL_UP; break; 

		case 5: 
			event.value.key.sym = KEYD_MWHEEL_DN; break; 

		default:
			return;
	}

	E_PostEvent(&event);
}


void HandleJoystickButtonEvent(SDL_Event * ev)
{
	// ignore other joysticks;
	if ((int)ev->jbutton.which != cur_joy-1)
		return;

	event_t event;
	
	if (ev->type == SDL_JOYBUTTONDOWN) 
		event.type = ev_keydown;
	else if (ev->type == SDL_JOYBUTTONUP) 
		event.type = ev_keyup;
	else 
		return;
	
	if (ev->jbutton.button >= 15)
		return;

	event.value.key.sym = KEYD_JOY1 + ev->jbutton.button;

	E_PostEvent(&event);
}


void HandleMouseMotionEvent(SDL_Event * ev)
{
	int dx, dy;

	if (in_warpmouse.d)
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

	event_t event;

	if (dx)
	{
		if (mouse_xaxis.d < 0)
			dx = -dx;

		event.type = ev_analogue;
		event.value.analogue.axis = abs(mouse_xaxis.d);
		event.value.analogue.amount = dx * mouse_xsens.f;

		E_PostEvent(&event);
	}

	if (dy)
	{
		if (mouse_yaxis.d < 0)
			dy = -dy;

		event.type = ev_analogue;
		event.value.analogue.axis = abs(mouse_yaxis.d);
		event.value.analogue.amount = dy * mouse_ysens.f;

		E_PostEvent(&event);
	}
}


int I_JoyGetAxis(int n)  // n begins at 0
{
	if (nojoy || !joy_info)
		return 0;
	
	return SDL_JoystickGetAxis(joy_info, n);
}

void HandleJoystickAxisEvent(SDL_Event * ev)
{
/*
	// ignore other joysticks;
	if ((int)ev->jaxis.which != cur_joy-1)
		return;

	event_t event;

	event.type = ev_analogue;
	event.value.analogue.axis = ev->jaxis.axis;
	event.value.analogue.amount = ev->jaxis.value / 1000.0;

	E_PostEvent(&event);

	I_Printf("JOYSTICK AXIS %d = %d\n", (int)ev->jaxis.axis,
	         (int)ev->jaxis.value);
*/
}


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

		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			HandleJoystickButtonEvent(sdl_ev);
			break;
			
		case SDL_MOUSEMOTION:
			if (eat_mouse_motion) 
			{
				eat_mouse_motion = false; // One motion needs to be discarded
				break;
			}

			HandleMouseMotionEvent(sdl_ev);
			break;

		case SDL_JOYAXISMOTION:
			HandleJoystickAxisEvent(sdl_ev);
			break;

		case SDL_JOYBALLMOTION:  // TODO
			break;
		case SDL_JOYHATMOTION:   // TODO
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


void I_CentreMouse(void)
{
	SDL_WarpMouse(SCREENWIDTH/2, SCREENHEIGHT/2);
}


void I_ShowJoysticks(void)
{
	if (nojoy)
	{
		I_Printf("Joystick system is disabled.\n");
		return;
	}

	if (num_joys == 0)
	{
		I_Printf("No joysticks found.\n");
		return;
	}

	I_Printf("Joysticks:\n");

	for (int i = 0; i < num_joys; i++)
	{
		const char *name = SDL_JoystickName(i);
		if (! name)
			name = "(UNKNOWN)";

		I_Printf("  %2d : %s\n", i+1, name);
	}
}

void I_OpenJoystick(int index)
{
	SYS_ASSERT(1 <= index && index <= num_joys);

	joy_info = SDL_JoystickOpen(index-1);
	if (! joy_info)
	{
		I_Printf("Unable to open joystick %d (SDL error)\n", index);
		return;
	}

	cur_joy = index;

	const char *name = SDL_JoystickName(index-1);
	if (! name)
		name = "(UNKNOWN)";

	I_Printf("Joystick name   : %s\n", name);
	I_Printf("Joystick axes   : %d\n", SDL_JoystickNumAxes(joy_info));
	I_Printf("Joystick buttons: %d\n", SDL_JoystickNumButtons(joy_info));
	I_Printf("Joystick hats   : %d\n", SDL_JoystickNumHats(joy_info));
	I_Printf("Joystick balls  : %d\n", SDL_JoystickNumBalls(joy_info));
}

void I_ChangeJoystick(int index)
{
	if (joy_info)
	{
		SDL_JoystickClose(joy_info);
		joy_info = NULL;
		cur_joy = 0;
	}

	if (index <= 0)
		return;

	if (index > num_joys)
	{
		I_Printf("Invalid joystick number: %d\n", cur_joy);
		return;
	}

	I_OpenJoystick(index);
}


void I_StartupJoystick(void)
{
	cur_joy = 0;

	if (M_CheckParm("-nojoy"))
	{
		I_Printf("I_StartupControl: Joystick system disabled.\n");
		nojoy = true;
		return;
	}

	if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0)
	{
		I_Printf("I_StartupControl: Couldn't init SDL JOYSTICK!\n");
		nojoy = true;
		return;
	}

	SDL_JoystickEventState(SDL_ENABLE);

	num_joys = SDL_NumJoysticks();

	I_Printf("I_StartupControl: %d joysticks found.\n", num_joys);

	if (num_joys == 0)
		return;

	if (joy_enable.d > num_joys)
	{
		joy_enable = 1;
	}

	if (joy_enable.d > 0)
		I_OpenJoystick(joy_enable.d);
}


/****** Input Event Generation ******/

void I_StartupControl(void)
{
	alt_is_down = false;

	// -ACB- 2008/09/20: No longer an option as we need to
	//                   value for console.
	SDL_EnableUNICODE(1);

#ifndef LINUX
	in_warpmouse = 1;
#endif

	if (M_CheckParm("-warpmouse"))
		in_warpmouse = 1;
	
	I_StartupJoystick();
}

void I_ControlGetEvents(void)
{
	if (joy_enable.CheckModified())
		I_ChangeJoystick(joy_enable.d);

	SDL_Event sdl_ev;

	while (SDL_PollEvent(&sdl_ev))
	{
#ifdef DEBUG_KB
		L_WriteDebug("#I_ControlGetEvents: type=%d\n", sdl_ev.type);
#endif
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


int I_EmergencyGetKey(void)
{
	// this is only used in emergency situations, e.g. for
	// the fallback error dialog.  It returns the unicode
	// key value (or as close as possible), zero if none,
	// and -1 for the system Quit signal.

	SDL_Event sdl_ev;

	while (SDL_PollEvent(&sdl_ev))
	{
		if (sdl_ev.type == SDL_QUIT)
			return -1;

		if (sdl_ev.type == SDL_KEYDOWN)
		{
			int sym = sdl_ev.key.keysym.sym;
			int uni = sdl_ev.key.keysym.unicode;

			if (uni > 0)
				return uni;

			if (sym > 0 && sym <= 127)
				return sym;
		}
	}

	return 0;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
