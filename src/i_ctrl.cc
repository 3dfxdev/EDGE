//----------------------------------------------------------------------------
//  EDGE2 SDL Controller Stuff
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2009  The EDGE2 Team.
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

#include "dm_defs.h"
#include "dm_state.h"
#include "e_event.h"
#include "e_input.h"
#include "e_main.h"
#include "i_video.h"
#include "m_argv.h"
#include "r_modes.h"


#undef DEBUG_KB

SDL_Window      *window;

// FIXME: Combine all these SDL bool vars into an int/enum'd flags structure

// Work around for alt-tabbing
bool alt_is_down;
bool eat_mouse_motion = true;

cvar_c in_keypad;
cvar_c in_warpmouse;

float mouse_accelfactor;

int         UseJoystick;
bool    DigiJoy;
int         DualMouse;

bool    MouseMode;//false=microsoft, true=mouse systems

bool window_mouse;

int mouse_x = 0;
int mouse_y = 0;

extern cvar_c mouse_accel, mouse_filter;


/* bool nojoy;  // what a wowser, joysticks completely disabled

int joystick_device;  // choice in menu, 0 for none

static int num_joys;
static int cur_joy;  // 0 for none
static SDL_Joystick *joy_info;

static int joy_num_axes;
static int joy_num_buttons;
static int joy_num_hats;
static int joy_num_balls; */

//
// Translates a key from SDL -> EDGE2
// Returns -1 if no suitable translation exists.
//
static int I_TranslateKey(const int key)
{
    int rc = 0;

    switch(key) {
    case SDLK_LEFT:
        rc = KEYD_LEFTARROW;
        break;
    case SDLK_RIGHT:
        rc = KEYD_RIGHTARROW;
        break;
    case SDLK_DOWN:
        rc = KEYD_DOWNARROW;
        break;
    case SDLK_UP:
        rc = KEYD_UPARROW;
        break;
    case SDLK_ESCAPE:
        rc = KEYD_ESCAPE;
        break;
    case SDLK_RETURN:
        rc = KEYD_ENTER;
        break;
    case SDLK_TAB:
        rc = KEYD_TAB;
        break;
    case SDLK_F1:
        rc = KEYD_F1;
        break;
    case SDLK_F2:
        rc = KEYD_F2;
        break;
    case SDLK_F3:
        rc = KEYD_F3;
        break;
    case SDLK_F4:
        rc = KEYD_F4;
        break;
    case SDLK_F5:
        rc = KEYD_F5;
        break;
    case SDLK_F6:
        rc = KEYD_F6;
        break;
    case SDLK_F7:
        rc = KEYD_F7;
        break;
    case SDLK_F8:
        rc = KEYD_F8;
        break;
    case SDLK_F9:
        rc = KEYD_F9;
        break;
    case SDLK_F10:
        rc = KEYD_F10;
        break;
    case SDLK_F11:
        rc = KEYD_F11;
        break;
    case SDLK_F12:
        rc = KEYD_F12;
        break;
    case SDLK_BACKSPACE:
        rc = KEYD_BACKSPACE;
        break;
    case SDLK_DELETE:
        rc = KEYD_DELETE;
        break;
    case SDLK_INSERT:
        rc = KEYD_INSERT;
        break;
    case SDLK_PAGEUP:
        rc = KEYD_PGUP;
        break;
    case SDLK_PAGEDOWN:
        rc = KEYD_PGDN;
        break;
    case SDLK_HOME:
        rc = KEYD_HOME;
        break;
    case SDLK_END:
        rc = KEYD_END;
        break;
    case SDLK_PAUSE:
        rc = KEYD_PAUSE;
        break;
    case SDLK_EQUALS:
        rc = KEYD_EQUALS;
        break;
    case SDLK_MINUS:
        rc = KEYD_MINUS;
        break;
	case SDLK_KP_0:
        rc = KEYD_KP0;
        break;
	case SDLK_KP_1:
        rc = KEYD_KP1;
        break;
	case SDLK_KP_2:
        rc = KEYD_KP2;
        break;
	case SDLK_KP_3:
        rc = KEYD_KP3;
        break;
	case SDLK_KP_4:
        rc = KEYD_KP4;
        break;
	case SDLK_KP_5:
        rc = KEYD_KP5;
        break;
	case SDLK_KP_6:
        rc = KEYD_KP6;
        break;
	case SDLK_KP_7:
        rc = KEYD_KP7;
        break;
	case SDLK_KP_8:
        rc = KEYD_KP8;
        break;
	case SDLK_KP_9:
        rc = KEYD_KP9;
        break;
    case SDLK_KP_PLUS:
        rc = KEYD_KP_PLUS;
        break;
    case SDLK_KP_MINUS:
        rc = KEYD_KP_MINUS;
        break;
    case SDLK_KP_DIVIDE:
        rc = KEYD_KP_SLASH;
        break;
    case SDLK_KP_MULTIPLY:
        rc = KEYD_KP_STAR;
        break;
    case SDLK_KP_ENTER:
        rc = KEYD_KP_ENTER;
        break;
    case SDLK_KP_PERIOD:
        rc = KEYD_KP_DOT;
		break;
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
        rc = KEYD_RSHIFT;
        break;
    case SDLK_LCTRL:
    case SDLK_RCTRL:
        rc = KEYD_RCTRL;
        break;
    case SDLK_LALT:
    //case SDLK_LMETA:
    case SDLK_RALT:
    //case SDLK_RMETA:
        rc = KEYD_RALT;
        break;
    case SDLK_CAPSLOCK:
        rc = KEYD_CAPSLOCK;
        break;
    default:
        rc = key;
        break;
    }

    return rc;

}


//
// I_SDLtoDoomMouseState
//

static int I_SDLtoDoomMouseState(Uint8 buttonstate) {
    return 0
           | (buttonstate & SDL_BUTTON(SDL_BUTTON_LEFT)      ? 1 : 0)
           | (buttonstate & SDL_BUTTON(SDL_BUTTON_MIDDLE)    ? 2 : 0)
           | (buttonstate & SDL_BUTTON(SDL_BUTTON_RIGHT)     ? 4 : 0);
}

//
// I_ReadMouse
//

static void I_ReadMouse(void) {
    int x, y;
    Uint8 btn;
    event_t ev;
    static Uint8 lastmbtn = 0;

    SDL_GetRelativeMouseState(&x, &y);
    btn = SDL_GetMouseState(&mouse_x, &mouse_y);

    if(x != 0 || y != 0 || btn || (lastmbtn != btn)) {
        ev.type = ev_mouse;
        ev.data1 = I_SDLtoDoomMouseState(btn);
        ev.data2 = x << 5;
        ev.data3 = (-y) << 5;
        ev.data4 = 0;
        E_PostEvent(&ev);
    }

	lastmbtn = btn;
}

//
// I_MouseAccelChange
//

void I_MouseAccelChange(void) {
    mouse_accelfactor = mouse_accel.d / 200.0f + 1.0f;
}

//
// I_MouseAccel
//
/// v_macceleration.value = mouse_accel.d in 3DGE (!)
int I_MouseAccel(int val) 
{
    if(!mouse_accel.d) 
	{
        return val;
    }

    if(val < 0) 
	{
        return -I_MouseAccel(-val);
    }

    return (int)(pow((double)val, (double)mouse_accelfactor));
}

extern cvar_c m_menumouse;
//
// I_ActivateMouse
//

static void I_ActivateMouse(void) {
    SDL_ShowCursor(0);
    SDL_SetRelativeMouseMode(SDL_TRUE);
}

//
// I_DeactivateMouse
//

static void I_DeactivateMouse(void) 
{
	
    SDL_ShowCursor(m_menumouse.d < 1);
    SDL_SetRelativeMouseMode(SDL_FALSE);
}

//
// I_UpdateGrab
//

void I_UpdateGrab(void) 
{
    static bool currently_grabbed = false;
	bool grab;

	grab = /*window_mouse &&*/ !menuactive
	&& (gamestate == GS_LEVEL)
	&& !demoplayback;

	if (grab && !currently_grabbed) 
	{
		SDL_ShowCursor(0);
		SDL_SetRelativeMouseMode(SDL_TRUE); ///src/i_ctrl.cc:335:29: error: invalid conversion from 'int' to 'SDL_bool' [-fpermissive] SDL_SetRelativeMouseMode(1);
		SDL_SetWindowGrab(window, SDL_TRUE);
	}

	if (!grab && currently_grabbed) 
	{
		SDL_SetRelativeMouseMode(SDL_FALSE);
		SDL_SetWindowGrab(window, SDL_FALSE);
		SDL_ShowCursor(m_menumouse.d < 1);
	}

	currently_grabbed = grab;
}

//
// I_GetEvent
//

static void I_GetEvent(SDL_Event *Event) {
    event_t event;
    uint32_t mwheeluptic = 0, mwheeldowntic = 0;
    uint32_t tic = gametic;

    switch(Event->type) {
	case SDL_KEYDOWN:
		if(Event->key.repeat)
			break;
        event.type = ev_keydown;
        event.data1 = I_TranslateKey(Event->key.keysym.sym);
        E_PostEvent(&event);
        break;

    case SDL_KEYUP:
        event.type = ev_keyup;
        event.data1 = I_TranslateKey(Event->key.keysym.sym);
        E_PostEvent(&event);
        break;

	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		if (!window_focused)
			break;

		event.type = (Event->type == SDL_MOUSEBUTTONUP) ? ev_mouseup : ev_mousedown;
		event.data1 =
			I_SDLtoDoomMouseState(SDL_GetMouseState(NULL, NULL));
		event.data2 = event.data3 = 0;

		E_PostEvent(&event);
		break;

	case SDL_MOUSEWHEEL:
		if (Event->wheel.y > 0) {
			event.type = ev_keydown;
			event.data1 = KEYD_MWHEELUP;
			mwheeluptic = tic;
		} else if (Event->wheel.y < 0) {
			event.type = ev_keydown;
			event.data1 = KEYD_MWHEELDOWN;
			mwheeldowntic = tic;
		} else
			break;

		event.data2 = event.data3 = 0;
		E_PostEvent(&event);
		break;

	case SDL_WINDOWEVENT:
		switch (Event->window.event) {
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
        break;

    default:
        break;
    }

    if(mwheeluptic && mwheeluptic + 1 < tic) {
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
}

//
// I_InitInputs
//

static void I_InitInputs(void) {
	SDL_PumpEvents();

    SDL_ShowCursor(m_menumouse.d < 1);

    I_MouseAccelChange();

#ifdef _USE_XINPUT
    I_XInputInit();
#endif
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
