//----------------------------------------------------------------------------
//  EDGE2 SDL Controller Stuff
//----------------------------------------------------------------------------
//  Copyright (c) 2015 Fraggle (Chocolate Doom)
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
#include "SDL_keycode.h"

#include "dm_defs.h"
#include "dm_state.h"
#include "e_event.h"
#include "e_input.h"
#include "e_main.h"
#include "m_argv.h"
#include "r_modes.h"

#undef DEBUG_KB

static const int scancode_translate_table[] = SCANCODE_TO_KEYS_ARRAY;

// FIXME: Combine all these SDL bool vars into an int/enum'd flags structure

// Work around for alt-tabbing
bool alt_is_down;
bool eat_mouse_motion = true;

cvar_c in_keypad;
cvar_c in_warpmouse;
/* 

bool nojoy;  // what a wowser, joysticks completely disabled

int joystick_device;  // choice in menu, 0 for none

static int num_joys;
static int cur_joy;  // 0 for none
static SDL_Joystick *joy_info;

static int joy_num_axes;
static int joy_num_buttons;
static int joy_num_hats;
static int joy_num_balls; */

// Bit mask of mouse button state.
static unsigned int mouse_button_state = 0;

// If true, I_StartTextInput() has been called, and we are populating
// the data3 field of ev_keydown events.
static bool text_input_enabled = true;

static const char shiftxform[] =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
    31, ' ', '!', '"', '#', '$', '%', '&',
    '"', // shift-'
    '(', ')', '*', '+',
    '<', // shift-,
    '_', // shift--
    '>', // shift-.
    '?', // shift-/
    ')', // shift-0
    '!', // shift-1
    '@', // shift-2
    '#', // shift-3
    '$', // shift-4
    '%', // shift-5
    '^', // shift-6
    '&', // shift-7
    '*', // shift-8
    '(', // shift-9
    ':',
    ':', // shift-;
    '<',
    '+', // shift-=
    '>', '?', '@',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    '[', // shift-[
    '!', // shift-backslash - OH MY GOD DOES WATCOM SUCK
    ']', // shift-]
    '"', '_',
    '\'', // shift-`
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    '{', '|', '}', '~', 127
};

//
// Translates a key from SDL -> EDGE
// Returns -1 if no suitable translation exists.
//
int TranslateSDLKey(SDL_Keysym *sym)///(int key)
{
	
	int scancode = sym->scancode;
	// if keypad is not wanted, convert to normal keys
/* 	if (! in_keypad.d)
	{
		if (SDLK_KP0 <= key && key <= SDLK_KP9)
			return '0' + (key - SDLK_KP0);

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
	} */

	switch (scancode) 
	{
		case SDL_SCANCODE_LCTRL:
        case SDL_SCANCODE_RCTRL:
            return KEYD_RCTRL;

        case SDL_SCANCODE_LSHIFT:
        case SDL_SCANCODE_RSHIFT:
            return KEYD_RSHIFT;

        case SDL_SCANCODE_LALT:
            return KEYD_LALT;

        case SDL_SCANCODE_RALT:
            return KEYD_RALT;

         default:
            if (scancode >= 0 && scancode < arrlen(scancode_translate_table))
            {
                return scancode_translate_table[scancode];
            }
            else
            {
                return 0;
            } 

 	if (scancode <= 0x7f)
		return tolower(scancode);
	}

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

static int GetLocalizedKey(SDL_Keysym *sym)
{
    // When using Vanilla mapping, we just base everything off the scancode
    // and always pretend the user is using a US layout keyboard.

        int result = sym->sym;

        if (result < 0 || result >= 128)
        {
            result = 0;
        }

        return result;
}


// Get the equivalent ASCII (Unicode?) character for a keypress.
static int GetTypedChar(SDL_Keysym *sym)
{
    // We only return typed characters when entering text, after
    // I_StartTextInput() has been called. Otherwise we return nothing.
    if (!text_input_enabled)
    {
        return 0;
    }

    // If we're strictly emulating Vanilla, we should always act like
    // we're using a US layout keyboard (in ev_keydown, data1=data2).
    // Otherwise we should use the native key mapping.

        SDL_Event next_event;

        // Special cases, where we always return a fixed value.
        switch (sym->sym)
        {
            case SDLK_BACKSPACE: return KEYD_BACKSPACE;
            case SDLK_RETURN:    return KEYD_ENTER;
            default:
                break;
        }

		/// The following is from Fraggle, who was kind enough to lay it out in Chocolate Doom:
        // The following is a gross hack, but I don't see an easier way
        // of doing this within the SDL2 API (in SDL1 it was easier).
        // We want to get the fully transformed input character associated
        // with this keypress - correct keyboard layout, appropriately
        // transformed by any modifier keys, etc. So peek ahead in the SDL
        // event queue and see if the key press is immediately followed by
        // an SDL_TEXTINPUT event. If it is, it's reasonable to assume the
        // key press and the text input are connected. Technically the SDL
        // API does not guarantee anything of the sort, but in practice this
        // is what happens and I've verified it through manual inspect of
        // the SDL source code.
        //
        // In an ideal world we'd split out ev_keydown into a separate
        // ev_textinput event, as SDL2 has done. But this doesn't work
        // (I experimented with the idea), because lots of Doom's code is
        // based around different responders "eating" events to stop them
        // being passed on to another responder. If code is listening for
        // a text input, it cannot block the corresponding keydown events
        // which can affect other responders.
        //
        // So we're stuck with this as a rather fragile alternative.

        if (SDL_PeepEvents(&next_event, 1, SDL_PEEKEVENT,
                           SDL_FIRSTEVENT, SDL_LASTEVENT) == 1
         && next_event.type == SDL_TEXTINPUT)
        {
            // If an SDL_TEXTINPUT event is found, we always assume it
            // matches the key press. The input text must be a single
            // ASCII character - if it isn't, it's possible the input
            // char is a Unicode value instead; better to send a null
            // character than the unshifted key.
            if (strlen(next_event.text.text) == 1
             && (next_event.text.text[0] & 0x80) == 0)
            {
                return next_event.text.text[0];
            }
        }

        // Failed to find anything :/
        return 0;
}


/// I_HandleKeyboardEvent(SDL_Event *sdlevent) in i_input.c (Chocolate Doom/Fraggle)
void I_HandleKeyEvent(SDL_Event* ev)
{	
    event_t event;

    switch (ev->type)
    {
        case SDL_KEYDOWN:
            event.type = ev_keydown;
            event.data1 = TranslateSDLKey(&ev->key.keysym);
            event.data2 = GetLocalizedKey(&ev->key.keysym);
            event.data3 = GetTypedChar(&ev->key.keysym);

            if (event.data1 != 0)
            {
                E_PostEvent(&event);
            }
            break;

        case SDL_KEYUP:
            event.type = ev_keyup;
            event.data1 = TranslateSDLKey(&ev->key.keysym);

            // data2/data3 are initialized to zero for ev_keyup.
            // For ev_keydown it's the shifted Unicode character
            // that was typed, but if something wants to detect
            // key releases it should do so based on data1
            // (key ID), not the printable char.

            event.data2 = 0;
            event.data3 = 0;

            if (event.data1 != 0)
            {
                E_PostEvent(&event);
            }
            break;

        default:
            break;
    }

/* 	if (ev->type != SDL_KEYDOWN && ev->type != SDL_KEYUP) 
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
			sym, ev->key.keysym.scancode, ev->key.keysym.unicode, event.value.key);
#endif

	if (event.value.key.sym < 0 &&
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

	if (event.value.key.sym == KEYD_LALT)
		alt_is_down = (event.type == ev_keydown); */

	E_PostEvent(&event);
}

void I_StartTextInput(int x1, int y1, int x2, int y2)
{
    text_input_enabled = true;

        // SDL2-TODO: SDL_SetTextInputRect(...);
     SDL_StartTextInput();

}

void I_StopTextInput(void)
{
    text_input_enabled = false;

    SDL_StopTextInput();

}

static void UpdateMouseButtonState(unsigned int button, bool on)
{
    event_t event;

    if (button < SDL_BUTTON_LEFT || button > MAX_MOUSE_BUTTONS)
    {
        return;
    }

    // Note: button "0" is left, button "1" is right,
    // button "2" is middle for Doom.  This is different
    // to how SDL sees things.

    switch (button)
    {
        case SDL_BUTTON_LEFT:
            button = 0;
            break;

        case SDL_BUTTON_RIGHT:
            button = 1;
            break;

        case SDL_BUTTON_MIDDLE:
            button = 2;
            break;

        default:
            // SDL buttons are indexed from 1.
            --button;
            break;
    }

    // Turn bit representing this button on or off.

    if (on)
    {
        mouse_button_state |= (1 << button);
    }
    else
    {
        mouse_button_state &= ~(1 << button);
    }

    // Post an event with the new button state.

    event.type = ev_mouse;
    event.data1 = mouse_button_state;
    event.data2 = event.data3 = 0;
    E_PostEvent(&event);
}

void I_HandleMouseEvent(SDL_Event *sdlevent)
{
    switch (sdlevent->type)
    {
        case SDL_MOUSEBUTTONDOWN:
            UpdateMouseButtonState(sdlevent->button.button, true);
            break;

        case SDL_MOUSEBUTTONUP:
            UpdateMouseButtonState(sdlevent->button.button, false);
            break;

        default:
            break;
    }
}


void HandleMouseButtonEvent(SDL_Event* ev)
{
	  switch (ev->type)
    {
        case SDL_MOUSEBUTTONDOWN:
            UpdateMouseButtonState(ev->button.button, true);
            break;

        case SDL_MOUSEBUTTONUP:
            UpdateMouseButtonState(ev->button.button, false);
            break;

        default:
            break;
    }
	
/* 	event_t event;
	
	if (ev->type == SDL_MOUSEBUTTONDOWN) 
		event.type = ev_keydown;
	else if (ev->type == SDL_MOUSEBUTTONUP) 
		event.type = ev_keyup;
	else 
		return;

	switch (ev->button.button)
	{
		case 1: event.value.key.sym = KEYD_MOUSE1; break;
		case 2: event.value.key.sym = KEYD_MOUSE2; break;
		case 3: event.value.key.sym = KEYD_MOUSE3; break;

		// handle the mouse wheel
		case 4: event.value.key.sym = KEYD_WHEEL_UP; break; 
		case 5: event.value.key.sym = KEYD_WHEEL_DN; break; 

		case 6: event.value.key.sym = KEYD_MOUSE4; break;
		case 7: event.value.key.sym = KEYD_MOUSE5; break;
		case 8: event.value.key.sym = KEYD_MOUSE6; break;

		default:
			return;
	}

	E_PostEvent(&event); */
}


/* void HandleJoystickButtonEvent(SDL_Event * ev)
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

	if (ev->jbutton.button > 14)
		return;

	event.value.key.sym = KEYD_JOY1 + ev->jbutton.button;

	E_PostEvent(&event);
} */


/// Choclate Doom says we should combine this into one function - void I_ReadMouse(void)
//void HandleMouseMotionEvent(SDL_Event * ev) /// Refs I_ReadMouse in Chocolate Doom
void I_ReadMouse(void)
{
	int dx, dy;
	event_t ev;

	SDL_GetRelativeMouseState(&dx, &dy);

	if (dx || dy)
	{
		//event_t event;
		ev.type = ev_mouse;
		ev.data1 = mouse_button_state;

		//event.type = ev_mouse;
/* 		ev.mouse.dx =  dx;
		ev.mouse.dy = -dy;  // -AJA- positive should be "up" */

		E_PostEvent(&ev);
	}
}


/* int I_JoyGetAxis(int n)  // n begins at 0
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
} */


//
// Event handling while the application is active
//
void ActiveEventProcess(SDL_Event *sdl_ev)
{
	switch(sdl_ev->type)
	{
		///SDL2-TODO, just like Chocolate DOOM/Fraggle in their event handler (d_event)
/* 		case SDL_ACTIVEEVENT:
		{
			if ((sdl_ev->active.state & SDL_APPINPUTFOCUS) &&
				(sdl_ev->active.gain == 0))
			{
				HandleFocusLost();
			}
			
			break;
		} */
		
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			I_HandleKeyEvent(sdl_ev);
			break;
				
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			HandleMouseButtonEvent(sdl_ev);
			break;
		
/* 		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			HandleJoystickButtonEvent(sdl_ev);
			break; */

		case SDL_MOUSEMOTION:
			if (eat_mouse_motion) 
			{
				eat_mouse_motion = false; // One motion needs to be discarded
				break;
			}

			I_ReadMouse();
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
///void InactiveEventProcess(SDL_Event *sdl_ev)
// InactiveEventProcess has now become I_GetEvent
void I_GetEvent(void)
{
    /* ~nothing to do here */
}

void I_CentreMouse(void)
{
	SDL_WarpMouseGlobal(SCREENWIDTH/2, SCREENHEIGHT/2);
	//SDL_WarpMouse(SCREENWIDTH/2, SCREENHEIGHT/2);
}


/****** Input Event Generation ******/

void I_StartupControl(void)
{
	alt_is_down = false;

	///-SDL1- -> SDL_EnableUNICODE(1);
	///SDL2-TODO: no more enableunicode?

	//I_StartupJoystick(); ///new joystick code will be called here, in I_StartupControl
}

void I_ControlGetEvents(void)
{
	///CheckJoystickChanged();
 	extern void I_HandleKeyEvent(SDL_Event* sdl_ev);
    extern void I_HandleMouseEvent(SDL_Event* sdl_ev);
    SDL_Event sdl_ev;

    SDL_PumpEvents();

    while (SDL_PollEvent(&sdl_ev))
    {
		#ifdef DEBUG_KB
		L_WriteDebug("#I_ControlGetEvents: type=%d\n", sdl_ev.type);
		#endif
        switch (sdl_ev.type)
        {
            case SDL_KEYDOWN:
            case SDL_KEYUP:
				I_HandleKeyEvent(&sdl_ev);
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                I_HandleMouseEvent(&sdl_ev);
                break;
/*             case SDL_QUIT:
                {
                     I don't want to link console here...
                }
                else
                {
                    event_t event;
                    event.type = ev_quit;
                    E_PostEvent(&event);
                }
                break; */

/*             case SDL_WINDOWEVENT:
                if (sdl_ev.window.windowID == SDL_GetWindowID())
                {
                    HandleWindowEvent(&sdl_ev.window);
                }
                break; */

            default:
                break;
        }
    }
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
