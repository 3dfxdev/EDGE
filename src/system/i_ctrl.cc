//----------------------------------------------------------------------------
//  EDGE2 SDL Controller Stuff
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2018  The EDGE2 Team.
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

#include "../dm_defs.h"
#include "../dm_state.h"
#include "../e_event.h"
#include "../e_input.h"
#include "../e_main.h"
#include "../e_player.h"
#include "../m_argv.h"
#include "../r_modes.h"

#ifdef WIN32
// put win32 specific ff stuff here

#elif defined (MACOSX)
// put osx specific ff stuff here

#elif defined (BSD)
// put bsd specific ff stuff here
#include <unistd.h>

#else
// put linux specific ff stuff here
#include <unistd.h>
#include <linux/input.h>
extern struct ff_effect effect[MAXPLAYERS];
#endif

extern int ff_frequency[MAXPLAYERS];
extern int ff_intensity[MAXPLAYERS];
extern int ff_timeout[MAXPLAYERS];
//extern int ff_rumble[MAXPLAYERS];

#undef DEBUG_KB

int window_focused;
int mouse_accel;
bool window_mouse;
// FIXME: Combine all these SDL bool vars into an int/enum'd flags structure

// Work around for alt-tabbing
bool alt_is_down;
bool eat_mouse_motion = true;

cvar_c in_keypad;
cvar_c in_warpmouse;


bool nojoy;  // what a wowser, joysticks completely disabled

int joystick_device;  // choice in menu, 0 for none

static int num_joys;
static int cur_joy;  // 0 for none
static SDL_Joystick *joy_info;

static int joy_num_axes;
static int joy_num_buttons;
static int joy_num_hats;
static int joy_num_balls;

//
// Translates a key from SDL -> EDGE
// Returns -1 if no suitable translation exists.
//
// For SDL2, SDKL_KPx has become SDLK_KP_x
//
int TranslateSDLKey(int key)
{
	// if keypad is not wanted, convert to normal keys
	if (! in_keypad.d)
	{
		if (SDLK_KP_0 <= key && key <= SDLK_KP_9)
			return '0' + (key - SDLK_KP_0);

		switch (key)
		{
			case SDLK_KP_PLUS:     return '+';
			case SDLK_KP_MINUS:    return '-';
			case SDLK_KP_PERIOD:   return '.';
			case SDLK_KP_MULTIPLY: return '*';
			case SDLK_KP_DIVIDE:   return '/';
			case SDLK_KP_EQUALS:   return '=';
			case SDLK_KP_ENTER:    return KEYD_ENTER;

			default: break;
		}
	}

	switch (key)
	{
		case SDLK_TAB: return KEYD_TAB;
		case SDLK_RETURN: return KEYD_ENTER;
		case SDLK_ESCAPE: return KEYD_ESCAPE;
		case SDLK_BACKSPACE: return KEYD_BACKSPACE;

		case SDLK_UP:    return KEYD_UPARROW;
		case SDLK_DOWN:  return KEYD_DOWNARROW;
		case SDLK_LEFT:  return KEYD_LEFTARROW;
		case SDLK_RIGHT: return KEYD_RIGHTARROW;

		case SDLK_HOME:   return KEYD_HOME;
		case SDLK_END:    return KEYD_END;
		case SDLK_INSERT: return KEYD_INSERT;
		case SDLK_DELETE: return KEYD_DELETE;
		case SDLK_PAGEUP: return KEYD_PGUP;
		case SDLK_PAGEDOWN: return KEYD_PGDN;

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

		case SDLK_KP_0: return KEYD_KP0;
		case SDLK_KP_1: return KEYD_KP1;
		case SDLK_KP_2: return KEYD_KP2;
		case SDLK_KP_3: return KEYD_KP3;
		case SDLK_KP_4: return KEYD_KP4;
		case SDLK_KP_5: return KEYD_KP5;
		case SDLK_KP_6: return KEYD_KP6;
		case SDLK_KP_7: return KEYD_KP7;
		case SDLK_KP_8: return KEYD_KP8;
		case SDLK_KP_9: return KEYD_KP9;

		case SDLK_KP_PERIOD:   return KEYD_KP_DOT;
		case SDLK_KP_PLUS:     return KEYD_KP_PLUS;
		case SDLK_KP_MINUS:    return KEYD_KP_MINUS;
		case SDLK_KP_MULTIPLY: return KEYD_KP_STAR;
		case SDLK_KP_DIVIDE:   return KEYD_KP_SLASH;
		case SDLK_KP_EQUALS:   return KEYD_KP_EQUAL;
		case SDLK_KP_ENTER:    return KEYD_KP_ENTER;

		case SDLK_PRINTSCREEN:     return KEYD_PRTSCR;
		case SDLK_CAPSLOCK:  return KEYD_CAPSLOCK;
		case SDLK_NUMLOCKCLEAR:   return KEYD_NUMLOCK;
		case SDLK_SCROLLLOCK: return KEYD_SCRLOCK;
		case SDLK_PAUSE:     return KEYD_PAUSE;

		case SDLK_LSHIFT:
		case SDLK_RSHIFT: return KEYD_RSHIFT;
		case SDLK_LCTRL:
		case SDLK_RCTRL:  return KEYD_RCTRL;
		case SDLK_LGUI:
		case SDLK_LALT:   return KEYD_LALT;
		case SDLK_RGUI:
		case SDLK_RALT:   return KEYD_RALT;

		default: break;
	}

	if (key <= 0x7f)
		return tolower(key);

	return -1;
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

static int I_SDLtoDoomMouseState(Uint8 buttonstate) 
{
    return 0
           | (buttonstate & SDL_BUTTON(SDL_BUTTON_LEFT)      ? 1 : 0)
           | (buttonstate & SDL_BUTTON(SDL_BUTTON_MIDDLE)    ? 2 : 0)
           | (buttonstate & SDL_BUTTON(SDL_BUTTON_RIGHT)     ? 4 : 0);
}


void HandleKeyEvent(SDL_Event* ev)
{
	SDL_PumpEvents();
//	if (ev->type != SDL_KEYDOWN && ev->type != SDL_KEYUP)
//		return;

	// For SDL2, we no longer require the SYM codes.
	///int sym = (int)ev->key.keysym.sym;
	event_t event;

    switch(ev->type)
	{
	case SDL_KEYDOWN:
		if(ev->key.repeat)
			break;
        event.type = ev_keydown;
        event.data1 = TranslateSDLKey(ev->key.keysym.sym); //will set event.data1 to -1 if no translation found (shouldn't happen on a normal keyboard)
        E_PostEvent(&event);
        break;

    case SDL_KEYUP:
        event.type = ev_keyup;
        event.data1 = TranslateSDLKey(ev->key.keysym.sym);
        E_PostEvent(&event);
        break;

	/*case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		 if (!window_focused)
			break;

		event.type = (ev->type == SDL_MOUSEBUTTONUP) ? ev_mouseup : ev_mousedown;
		event.data1 =
			I_SDLtoDoomMouseState(SDL_GetMouseState(NULL, NULL));
		event.data2 = event.data3 = 0;

		E_PostEvent(&event);
		break;*/

	/*case SDL_WINDOWEVENT:
		switch (ev->window.event)
		{
		case SDL_WINDOWEVENT_FOCUS_GAINED:
			window_focused = true;
			break;

		case SDL_WINDOWEVENT_FOCUS_LOST:
			window_focused = false;
			break;

		case SDL_WINDOWEVENT_ENTER:
			window_mouse = true;
			break;

		case SDL_WINDOWEVENT_LEAVE:
			window_mouse = false;
			break;

		default:
			break;
		}
		break;

    case SDL_QUIT:
         I_CloseProgram(-1);
        break; */

    default:
        break;
    }

   /* if(mwheeluptic && mwheeluptic + 1 < tic) {
        event.type = ev_keyup;
        event.data1 = KEYD_MWHEELUP;
        E_PostEvent(&event);
        mwheeluptic = 0;
    }

    if(mwheeldowntic && mwheeldowntic + 1 < tic) {
        event.type = ev_keyup;
        event.data1 = KEYD_MWHEELDOWN;
        E_PostEvent(&event);
        mwheeldowntic = 0;
    }
	*/


//	E_PostEvent(&event);
return;
}


void HandleMouseButtonEvent(SDL_Event * ev)
{
	event_t event;
	SDL_PumpEvents();

	if (ev->type == SDL_MOUSEBUTTONDOWN)
//		event.type = ev_mousedown;
		event.type = ev_keydown;
	else if (ev->type == SDL_MOUSEBUTTONUP)
//		event.type = ev_mouseup;
		event.type = ev_keyup;
	else
		return;

	switch (ev->button.button)
	{
		case 1: event.data1 = KEYD_MOUSE1; break;
		case 2: event.data1 = KEYD_MOUSE2; break;
		case 3: event.data1 = KEYD_MOUSE3; break;

		// handle the mouse wheel
		case 4: event.data1 = KEYD_WHEEL_UP; break;
		case 5: event.data1 = KEYD_WHEEL_DN; break;

		case 6: event.data1 = KEYD_MOUSE4; break;
		case 7: event.data1 = KEYD_MOUSE5; break;
		case 8: event.data1 = KEYD_MOUSE6; break;

		default:
			return;
	}

	E_PostEvent(&event);
}


void HandleMouseMotionEvent(SDL_Event * ev)
{
	int dx, dy;
	SDL_PumpEvents();

	dx = ev->motion.xrel;
	dy = ev->motion.yrel;
	SDL_SetRelativeMouseMode(SDL_TRUE);

	if (dx || dy)
	{
		event_t event;

		event.type = ev_mouse;
		event.data2 =  dx; ///mouseX
		event.data3 = -dy;  /// mouseY

		E_PostEvent(&event);

	}
}

void HandleMouseWheelEvent(SDL_Event * ev)
{
	event_t event;
	SDL_PumpEvents();

	if (ev->wheel.y > 0) {
		event.type = ev_keydown;
		event.data1 = KEYD_WHEEL_UP;
	}
	else if (ev->wheel.y < 0) {
		event.type = ev_keydown;
		event.data1 = KEYD_WHEEL_DN;
	}
	else
		return; //TODO: determine if we need to handle this case. This event shouldn't ever fire with a value of 0 anyway.
    event.data2 = event.data3 = 0;
	E_PostEvent(&event);
	event.type = ev_keyup;
	E_PostEvent(&event);
	return;
}

void HandleJoystickButtonEvent(SDL_Event * ev)
{
	SDL_PumpEvents();
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

	if (ev->jbutton.button > 14)
		return;

	event.data1 = KEYD_JOY1 + ev->jbutton.button;

	E_PostEvent(&event);
}




int I_JoyGetAxis(int n)  // n begins at 0
{
	if (nojoy || !joy_info)
		return 0;

	if (n < joy_num_axes)
		return SDL_JoystickGetAxis(joy_info, n);

	n -= joy_num_axes;

	// -AJA- handle joystick HATS by mapping it to axes
	if (n/2 < joy_num_hats)
	{
		Uint8 hat = SDL_JoystickGetHat(joy_info, n/2);

		if (n & 1)
		{
			return (hat & SDL_HAT_DOWN) ? -32767 :
				   (hat & SDL_HAT_UP)   ? +32767 : 0;
		}
		else
		{
			return (hat & SDL_HAT_LEFT)  ? -32767 :
				   (hat & SDL_HAT_RIGHT) ? +32767 : 0;
		}
	}

	return 0;
}


//
// Event handling while the application is active
//
void ActiveEventProcess(SDL_Event *sdl_ev)
{
	switch(sdl_ev->type)
	{
		case SDL_WINDOWEVENT:
		switch (sdl_ev->window.event)
		{
		case SDL_WINDOWEVENT_FOCUS_GAINED:
			window_focused = true;
			break;

		case SDL_WINDOWEVENT_FOCUS_LOST:
			window_focused = false;
			break;

		case SDL_WINDOWEVENT_ENTER:
			window_mouse = true;
			break;

		case SDL_WINDOWEVENT_LEAVE:
			window_mouse = false;
			break;

		default:
			break;
		}
		break;

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

		case SDL_MOUSEWHEEL:
			HandleMouseWheelEvent(sdl_ev);
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
	/* switch(sdl_ev->type)
	{
		case SDL_WINDOWEVENT:
			if (app_state & APP_STATE_PENDING_QUIT)
				break; // Don't care: we're going to exit

			 if (!sdl_ev->window.event)
				break;

			if (sdl_ev->window.event & SDL_APPACTIVE ||
                sdl_ev->window.event & SDL_APPINPUTFOCUS)
				window_focused = true;
			break;

		case SDL_QUIT:
			// Note we deliberate clear all other flags here. Its our method of
			// ensuring nothing more is done with events.
			app_state = APP_STATE_PENDING_QUIT;
			break;

		default:
			break; // Don't care
	} */
}


void I_CentreMouse(void)
{
	SDL_SetRelativeMouseMode(SDL_TRUE);//(SCREENWIDTH/2, SCREENHEIGHT/2);

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
		const char *name = SDL_JoystickNameForIndex(i);
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

	const char *name = SDL_JoystickNameForIndex(cur_joy-1);
	if (! name)
		name = "(UNKNOWN)";

	joy_num_axes    = SDL_JoystickNumAxes(joy_info);
	joy_num_buttons = SDL_JoystickNumButtons(joy_info);
	joy_num_hats    = SDL_JoystickNumHats(joy_info);
	joy_num_balls   = SDL_JoystickNumBalls(joy_info);

	I_Printf("Opened joystick %d : %s\n", cur_joy, name);
	I_Printf("Axes:%d buttons:%d hats:%d balls:%d\n",
			 joy_num_axes, joy_num_buttons, joy_num_hats, joy_num_balls);
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

	joystick_device = CLAMP(0, joystick_device, num_joys);

	if (num_joys == 0)
		return;

	if (joystick_device > 0)
		I_OpenJoystick(joystick_device);
}


void CheckJoystickChanged(void)
{
	int new_joy = joystick_device;

	if (joystick_device < 0 || joystick_device > num_joys)
		new_joy = 0;

	if (new_joy == cur_joy)
		return;

	if (joy_info)
	{
		SDL_JoystickClose(joy_info);
		joy_info = NULL;

		I_Printf("Closed joystick %d\n", cur_joy);
		cur_joy = 0;
	}

	if (new_joy > 0)
	{
		I_OpenJoystick(new_joy);
	}
}


/****** Input Event Generation ******/

void I_StartupControl(void)
{
	alt_is_down = false;

	SDL_PumpEvents();

	I_StartupJoystick();
}

void I_ControlGetEvents(void)
{
	CheckJoystickChanged();

	SDL_PumpEvents();

	SDL_Event sdl_ev;

	while (SDL_PollEvent(&sdl_ev))
	{
#ifdef DEBUG_KB
		L_WriteDebug("#I_ControlGetEvents: type=%d\n", sdl_ev.type);
#endif
		if (app_state & APP_STATE_ACTIVE)
		{
			ActiveEventProcess(&sdl_ev);
		}
		else
		{
			InactiveEventProcess(&sdl_ev);
		}
	}
#if 0

	// -CW- poll force feedback rumble
	if (ff_rumble[0])
	{
#ifdef WIN32

#elif defined (MACOSX)

#else
		struct input_event play;
		static bool ff_on = false;

		if (I_GetMillies() >= ff_timeout[0])
			ff_timeout[0] = 0;

		if (ff_timeout[0])
		{
			if (!ff_on)
			{
				ff_on = true;
				play.type = EV_FF;
				play.code = effect[0].id;	/* the id we got when uploading the effect */
				play.value = 1;				/* play: 1, stop: 0 */
				write(ff_rumble[0], (const void*)&play, sizeof(play));
			}
		}
		else
		{
			if (ff_on)
			{
				ff_on = false;
				play.type = EV_FF;
				play.code = effect[0].id;	/* the id we got when uploading the effect */
				play.value = 0;				/* play: 1, stop: 0 */
				write(ff_rumble[0], (const void*)&play, sizeof(play));
			}
	}
#endif
}
#endif // 0

}

void I_ShutdownControl(void)
{
}


int I_GetTime(void)
{
    Uint32 t = SDL_GetTicks();

    // more complex than "t*35/1000" to give more accuracy
    //return t*(35.0f/1000.0f);
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
