//----------------------------------------------------------------------------
//  EDGE Default Settings
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
#ifndef __DEFAULT_SETTINGS__
#define __DEFAULT_SETTINGS__

// Screen resolution
#define CFGDEF_SCREENWIDTH      (1024)
#define CFGDEF_SCREENHEIGHT     (768)
#define CFGDEF_SCREENBITS       (32)
#define CFGDEF_SCREENWINDOW     (0)

// Controls (Key/Mouse Buttons)
#define CFGDEF_KEY_FIRE         (KEYD_RCTRL + (KEYD_MOUSE1 << 16))
#define CFGDEF_KEY_SECONDATK    ('e')
#define CFGDEF_KEY_USE          (' ')
#define CFGDEF_KEY_UP           (KEYD_UPARROW)
#define CFGDEF_KEY_DOWN         (KEYD_DOWNARROW)
#define CFGDEF_KEY_LEFT         (KEYD_LEFTARROW)
#define CFGDEF_KEY_RIGHT        (KEYD_RIGHTARROW)
#define CFGDEF_KEY_FLYUP        (KEYD_INSERT)
#define CFGDEF_KEY_FLYDOWN      (KEYD_DELETE)
#define CFGDEF_KEY_SPEED        (KEYD_RSHIFT)
#define CFGDEF_KEY_STRAFE       (KEYD_RALT + (KEYD_MOUSE3 << 16))
#define CFGDEF_KEY_STRAFELEFT   (',')
#define CFGDEF_KEY_STRAFERIGHT  ('.')
#define CFGDEF_KEY_AUTORUN      (KEYD_CAPSLOCK)

#define CFGDEF_KEY_LOOKUP       (KEYD_PGUP)
#define CFGDEF_KEY_LOOKDOWN     (KEYD_PGDN)
#define CFGDEF_KEY_LOOKCENTER   (KEYD_HOME)
#define CFGDEF_KEY_MLOOK        ('m')
#define CFGDEF_KEY_ZOOM         ('\\')
#define CFGDEF_KEY_JUMP         ('/')
#define CFGDEF_KEY_MAP          (KEYD_TAB)
#define CFGDEF_KEY_180          (0)
#define CFGDEF_KEY_RELOAD       ('r')
#define CFGDEF_KEY_NEXTWEAPON   (KEYD_MWHEEL_DN)
#define CFGDEF_KEY_PREVWEAPON   (KEYD_MWHEEL_UP)
#define CFGDEF_KEY_TALK         ('t')

// Controls (Analogue)
#define CFGDEF_INVERTMOUSE      (0)
#define CFGDEF_MOUSE_XAXIS      (AXIS_TURN)
#define CFGDEF_MOUSE_YAXIS      (AXIS_MLOOK)
#define CFGDEF_MOUSESENSITIVITY (10)
#define CFGDEF_MLOOKSPEED       (10)
#define CFGDEF_STAGETURN        (0)
#define CFGDEF_ANGLETURNSPEED   (4)
#define CFGDEF_SIDEMOVESPEED    (4)
#define CFGDEF_FORWARDMOVESPEED (4)

#define CFGDEF_JOY_XAXIS        (AXIS_TURN)
#define CFGDEF_JOY_YAXIS        (AXIS_FORWARD)

// Misc
#define CFGDEF_MENULANGUAGE     (0)
#define CFGDEF_SHOWMESSAGES     (1)

// Sound and Music
#define CFGDEF_SOUND_VOLUME     (8)
#define CFGDEF_MUSIC_VOLUME     (8)
#define CFGDEF_SAMPLE_RATE      (1)  // 22050Hz
#define CFGDEF_SOUND_BITS       (0)  // 8-bit
#define CFGDEF_SOUND_STEREO     (1)  // Stereo
#define CFGDEF_MIX_CHANNELS     (2)  // 32 channels
#define CFGDEF_QUIET_FACTOR     (1)

// Video Options
#define CFGDEF_CURRENT_GAMMA    (2)
#define CFGDEF_NORMALFOV        (90)  // 8
#define CFGDEF_ZOOMEDFOV        (10)  // 8
#define CFGDEF_USE_SMOOTHING    (1)
#define CFGDEF_USE_DLIGHTS      (0)
#define CFGDEF_DOOM_FADING      (1)
#define CFGDEF_DETAIL_LEVEL     (1)
#define CFGDEF_USE_MIPMAPPING   (0)
#define CFGDEF_SCREEN_HUD       (HUD_Full)
#define CFGDEF_SKY_STRETCH      (1)
#define CFGDEF_SHADOWS          (0)
#define CFGDEF_CROSSHAIR        (0)
#define CFGDEF_MAP_OVERLAY      (0)
#define CFGDEF_ROTATEMAP        (0)
#define CFGDEF_TELEPT_FLASH     (1)
#define CFGDEF_WIPE_METHOD      (1)
#define CFGDEF_PNG_SCRSHOTS     (1)
#define CFGDEF_TELEPT_EFFECT    (0)
#define CFGDEF_TELEPT_REVERSE   (0)
#define CFGDEF_WIPE_REVERSE     (0)

// Gameplay Options
#define CFGDEF_AUTOAIM          (1)
#define CFGDEF_MLOOK            (1)
#define CFGDEF_JUMP             (0)
#define CFGDEF_CROUCH           (0)
#define CFGDEF_KICKING          (0)
#define CFGDEF_WEAPON_SWITCH    (1)
#define CFGDEF_MORE_BLOOD       (0)
#define CFGDEF_HAVE_EXTRA       (0)
#define CFGDEF_TRUE3DGAMEPLAY   (1)
#define CFGDEF_PASS_MISSILE     (1)
#define CFGDEF_MENU_GRAV        (MENU_GRAV_NORMAL)
#define CFGDEF_RES_RESPAWN      (1)       // Resurrect Mode 
#define CFGDEF_ITEMRESPAWN      (0)
#define CFGDEF_FASTPARM         (0)
#define CFGDEF_RESPAWN          (0)

#endif /* __CFGDEF_SETTINGS__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
