//----------------------------------------------------------------------------
//  LIST OF ALL CVARS
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2007-2009  The EDGE Team.
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

#include "con_var.h"

#include "e_input.h"  // jaxis_group_c


extern cvar_c edge_compat;

extern cvar_c g_skill, g_gametype;
extern cvar_c g_mlook, g_autoaim;
extern cvar_c g_jumping, g_crouching;
extern cvar_c g_true3d, g_aggression;
extern cvar_c g_moreblood, g_noextra;
extern cvar_c g_fastmon, g_passmissile;
extern cvar_c g_weaponkick, g_weaponswitch;

extern cvar_c am_rotate, am_smoothing;

extern cvar_c m_language;
extern cvar_c m_diskicon, m_busywait, m_screenhud;
extern cvar_c m_messages, m_obituaries;

extern cvar_c sys_directx, sys_waveout;
extern cvar_c sys_grabfocus;

extern cvar_c in_autorun, in_stageturn, in_shiftlook;
extern cvar_c in_warpmouse;
extern cvar_c in_keypad;

extern cvar_c mouse_x_axis, mouse_y_axis;
extern cvar_c mouse_x_sens, mouse_y_sens;
extern cvar_c mouse_accel,  mouse_filter;

extern cvar_c joy_enable;

extern jaxis_group_c joyaxis1, joyaxis2, joyaxis3;
extern jaxis_group_c joyaxis4, joyaxis5, joyaxis6;
extern jaxis_group_c joyaxis7, joyaxis8, joyaxis9;

extern cvar_c r_width, r_height, r_depth, r_fullscreen;
extern cvar_c r_colormaterial, r_colorlighting;
extern cvar_c r_dumbsky, r_dumbmulti, r_dumbcombine, r_dumbclamp;
extern cvar_c r_nearclip, r_farclip, r_fadepower;
extern cvar_c r_fov, r_zoomedfov;
extern cvar_c r_mipmapping, r_smoothing;
extern cvar_c r_dithering, r_hq2x;
extern cvar_c r_dynlight, r_invultex;
extern cvar_c r_gamma, r_detaillevel;
extern cvar_c r_wipemethod, r_wipereverse;
extern cvar_c r_teleportflash;
extern cvar_c r_crosshair, r_crosscolor;
extern cvar_c r_crosssize, r_crossbright;

extern cvar_c s_volume, s_mixchan, s_quietfactor;
extern cvar_c s_rate, s_bits, s_stereo;
extern cvar_c s_musicvol, s_musicdevice;
extern cvar_c tim_quietfactor;

extern cvar_c debug_nomonsters, debug_hom;
extern cvar_c debug_fullbright, debug_subsector;
extern cvar_c debug_joyaxis,    debug_mouse;
extern cvar_c debug_normals;


#ifndef LINUX
#define S_MUSICDEV_CFG  "0"  // native
#else
#define S_MUSICDEV_CFG  "1"  // timidity
#endif


// Flag letters:
// =============
//
//   r : read only, user cannot change it
//   c : config file (saved and loaded)
//   h : cheat
//


cvar_link_t  all_cvars[] =
{
	/* General Stuff */

    { "edge_compat",    &edge_compat,    "",    "0"  },

    { "language",       &m_language,     "c",   "ENGLISH" },

    { "sys_directx",    &sys_directx,    "c",   "0"  },
    { "sys_waveout",    &sys_waveout,    "c",   "0"  },
    { "sys_grabfocus",  &sys_grabfocus,  "c",   "1"  },

    { "g_skill",        &g_skill,        "c",   "3"  },
    { "g_gametype",     &g_gametype,     "",    "0"  },
    { "g_mlook",        &g_mlook,        "c",   "1"  },
    { "g_autoaim",      &g_autoaim,      "c",   "1"  },
    { "g_jumping",      &g_jumping,      "c",   "0"  },
    { "g_crouching",    &g_crouching,    "c",   "0"  },
    { "g_true3d",       &g_true3d,       "c",   "1"  },
    { "g_noextra",      &g_noextra,      "c",   "0"  },
    { "g_moreblood",    &g_moreblood,    "c",   "0"  },
    { "g_fastmon",      &g_fastmon,      "c",   "0"  },
    { "g_aggression",   &g_aggression,   "c",   "0"  },
    { "g_passmissile",  &g_passmissile,  "c",   "1"  },
    { "g_weaponkick",   &g_weaponkick,   "c",   "0"  },
    { "g_weaponswitch", &g_weaponswitch, "c",   "1"  },

	{ "am_rotate",      &am_rotate,      "c",   "0"  },
	{ "am_smoothing",   &am_smoothing,   "c",   "1"  },
                                        
	{ "m_diskicon",     &m_diskicon,     "c",   "1"  },
	{ "m_busywait",     &m_busywait,     "c",   "1"  },
	{ "m_messages",     &m_messages,     "c",   "1"  },
	{ "m_obituaries",   &m_obituaries,   "c",   "1"  },
	{ "m_screenhud",    &m_screenhud,    "c",   "0"  },
                                        
	/* Rendering Stuff */

	{ "r_width",        &r_width,        "c",   "640"   },
	{ "r_height",       &r_height,       "c",   "480"   },
    { "r_depth",        &r_depth,        "c",   "32"    },
    { "r_fullscreen",   &r_fullscreen,   "c",   "1"     },
												
	{ "r_gamma",        &r_gamma,        "c",   "1"  },
	{ "r_nearclip",     &r_nearclip,     "c",   "4"  },
	{ "r_farclip",      &r_farclip,      "c",   "64000" },
	{ "r_fadepower",    &r_fadepower,    "c",   "1"  },
	{ "r_fov",          &r_fov,          "c",   "90" },
	{ "r_zoomedfov",    &r_zoomedfov,    "c",   "10" },

	{ "r_crosshair",    &r_crosshair,    "c",   "0"  },
	{ "r_crosscolor",   &r_crosscolor,   "c",   "7"  },
	{ "r_crosssize",    &r_crosssize,    "c",   "16" },
	{ "r_crossbright",  &r_crossbright,  "c",   "1.0" },

	{ "r_mipmapping",   &r_mipmapping,   "c",   "0"  },
	{ "r_smoothing",    &r_smoothing,    "c",   "0"  },
	{ "r_dithering",    &r_dithering,    "c",   "0"  },
	{ "r_hq2x",         &r_hq2x,         "c",   "1"  },

	{ "r_dynlight",     &r_dynlight,     "c",   "1"  },
	{ "r_detaillevel",  &r_detaillevel,  "c",   "1"  },
	{ "r_invultex",     &r_invultex,     "c",   "1"  },
	{ "r_wipemethod",   &r_wipemethod,   "c",   "1" /* Melt */ },
	{ "r_wipereverse",  &r_wipereverse,  "c",   "0"  },
	{ "r_teleportflash",&r_teleportflash,"c",   "1"  },

	{ "r_colormaterial",&r_colormaterial,"",    "1"  },
	{ "r_colorlighting",&r_colorlighting,"",    "1"  },
	{ "r_dumbsky",      &r_dumbsky,      "",    "0"  },
	{ "r_dumbmulti",    &r_dumbmulti,    "",    "0"  },
	{ "r_dumbcombine",  &r_dumbcombine,  "",    "0"  },
	{ "r_dumbclamp",    &r_dumbclamp,    "",    "0"  },

	/* Sound Stuff */

	{ "s_volume",       &s_volume,       "c",   "0.5"  },
	{ "s_mixchan",      &s_mixchan,      "c",   "32"   },
	{ "s_rate",         &s_rate,         "c",   "22050" },
	{ "s_bits",         &s_bits,         "c",   "16" },
	{ "s_stereo",       &s_stereo,       "c",   "1"  },
	{ "s_musicvol",     &s_musicvol,     "c",   "0.5"  },
	{ "s_musicdevice",  &s_musicdevice,  "c",   S_MUSICDEV_CFG },

	{ "s_quietfactor",  &s_quietfactor,  "c",   "1"  },
	{ "tim_quietfactor",&tim_quietfactor,"c",   "1"  },

	/* Input Stuff */

	{ "in_autorun",     &in_autorun,     "c",   "0"  },
	{ "in_keypad",      &in_keypad,      "c",   "0"  },
	{ "in_stageturn",   &in_stageturn,   "c",   "1"  },
	{ "in_shiftlook",   &in_shiftlook,   "c",   "1"  },
	{ "in_warpmouse",   &in_warpmouse,   "",    "0"  },

	{ "mouse_x.axis",   &mouse_x_axis,   "c",   "1" /* AXIS_TURN */  },
	{ "mouse_x.sens",   &mouse_x_sens,   "c",   "10"  },
	{ "mouse_y.axis",   &mouse_y_axis,   "c",   "4" /* AXIS_MLOOK */ },
	{ "mouse_y.sens",   &mouse_y_sens,   "c",   "10" },
	{ "mouse_accel",    &mouse_accel,    "c",   "0"  },
	{ "mouse_filter",   &mouse_filter,   "c",   "0"  },

	{ "joy_enable",     &joy_enable,     "c",   "0"  },

	{ "jaxis1.axis",    &joyaxis1.axis,  "c",   "3" /* AXIS_STRAFE */  },
	{ "jaxis1.sens",    &joyaxis1.sens,  "c",   "10"  },
	{ "jaxis1.dead",    &joyaxis1.dead,  "c",   "0.10" },
	{ "jaxis1.peak",    &joyaxis1.peak,  "c",   "0.95" },
	{ "jaxis1.tune",    &joyaxis1.tune,  "c",   "1.0" },
	{ "jaxis1.filter",  &joyaxis1.filter,"c",   "1"   },

	{ "jaxis2.axis",    &joyaxis2.axis,  "c",   "2" /* AXIS_FORWARD */ },
	{ "jaxis2.sens",    &joyaxis2.sens,  "c",   "10"  },
	{ "jaxis2.dead",    &joyaxis2.dead,  "c",   "0.10" },
	{ "jaxis2.peak",    &joyaxis2.peak,  "c",   "0.95" },
	{ "jaxis2.tune",    &joyaxis2.tune,  "c",   "1.0" },
	{ "jaxis2.filter",  &joyaxis2.filter,"c",   "1"   },

	{ "jaxis3.axis",    &joyaxis3.axis,  "c",   "1" /* AXIS_TURN */  },
	{ "jaxis3.sens",    &joyaxis3.sens,  "c",   "10"  },
	{ "jaxis3.dead",    &joyaxis3.dead,  "c",   "0.10" },
	{ "jaxis3.peak",    &joyaxis3.peak,  "c",   "0.95" },
	{ "jaxis3.tune",    &joyaxis3.tune,  "c",   "1.0" },
	{ "jaxis3.filter",  &joyaxis3.filter,"c",   "1"   },

	{ "jaxis4.axis",    &joyaxis4.axis,  "c",   "4" /* AXIS_MLOOK */ },
	{ "jaxis4.sens",    &joyaxis4.sens,  "c",   "10"  },
	{ "jaxis4.dead",    &joyaxis4.dead,  "c",   "0.10" },
	{ "jaxis4.peak",    &joyaxis4.peak,  "c",   "0.95" },
	{ "jaxis4.tune",    &joyaxis4.tune,  "c",   "1.0" },
	{ "jaxis4.filter",  &joyaxis4.filter,"c",   "1"   },

	{ "jaxis5.axis",    &joyaxis5.axis,  "c",   "0"   },
	{ "jaxis5.sens",    &joyaxis5.sens,  "c",   "10"  },
	{ "jaxis5.dead",    &joyaxis5.dead,  "c",   "0.10" },
	{ "jaxis5.peak",    &joyaxis5.peak,  "c",   "0.95" },
	{ "jaxis5.tune",    &joyaxis5.tune,  "c",   "1.0" },
	{ "jaxis5.filter",  &joyaxis5.filter,"c",   "1"   },

	{ "jaxis6.axis",    &joyaxis6.axis,  "c",   "0"   },
	{ "jaxis6.sens",    &joyaxis6.sens,  "c",   "10"  },
	{ "jaxis6.dead",    &joyaxis6.dead,  "c",   "0.10" },
	{ "jaxis6.peak",    &joyaxis6.peak,  "c",   "0.95" },
	{ "jaxis6.tune",    &joyaxis6.tune,  "c",   "1.0" },
	{ "jaxis6.filter",  &joyaxis6.filter,"c",   "1"   },

	{ "jaxis7.axis",    &joyaxis7.axis,  "c",   "0"   },
	{ "jaxis7.sens",    &joyaxis7.sens,  "c",   "10"  },
	{ "jaxis7.dead",    &joyaxis7.dead,  "c",   "0.10" },
	{ "jaxis7.peak",    &joyaxis7.peak,  "c",   "0.95" },
	{ "jaxis7.tune",    &joyaxis7.tune,  "c",   "1.0" },
	{ "jaxis7.filter",  &joyaxis7.filter,"c",   "1"   },

	{ "jaxis8.axis",    &joyaxis8.axis,  "c",   "0"   },
	{ "jaxis8.sens",    &joyaxis8.sens,  "c",   "10"  },
	{ "jaxis8.dead",    &joyaxis8.dead,  "c",   "0.10" },
	{ "jaxis8.peak",    &joyaxis8.peak,  "c",   "0.95" },
	{ "jaxis8.tune",    &joyaxis8.tune,  "c",   "1.0" },
	{ "jaxis8.filter",  &joyaxis8.filter,"c",   "1"   },

	{ "jaxis9.axis",    &joyaxis9.axis,  "c",   "0"   },
	{ "jaxis9.sens",    &joyaxis9.sens,  "c",   "10"  },
	{ "jaxis9.dead",    &joyaxis9.dead,  "c",   "0.10" },
	{ "jaxis9.peak",    &joyaxis9.peak,  "c",   "0.95" },
	{ "jaxis9.tune",    &joyaxis9.tune,  "c",   "1.0" },
	{ "jaxis9.filter",  &joyaxis9.filter,"c",   "1"   },

	/* Debugging Stuff */

	{ "debug_nomonsters", &debug_nomonsters, "h", "0" },
	{ "debug_fullbright", &debug_fullbright, "h", "0" },
	{ "debug_hom",        &debug_hom,        "h", "0" },
	{ "debug_subsector",  &debug_subsector,  "h", "0" },
	{ "debug_normals",    &debug_normals,    "h", "0" },
	{ "debug_joyaxis",    &debug_joyaxis,    "h", "0" },
	{ "debug_mouse",      &debug_mouse,      "h", "0" },

//---- END OF LIST -----------------------------------------------------------

	{ NULL, NULL, NULL, NULL }
};

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
