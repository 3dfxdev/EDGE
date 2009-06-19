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


extern cvar_c edge_compat;

extern cvar_c g_mlook, g_autoaim;
extern cvar_c g_jumping, g_crouching;
extern cvar_c g_true3d;
extern cvar_c g_moreblood, g_noextra;
extern cvar_c g_fastmon, g_passmissile;
extern cvar_c g_weaponkick, g_weaponswitch;

extern cvar_c am_rotate, am_smoothing;
extern cvar_c m_diskicon, m_busywait, m_screenhud;
extern cvar_c m_messages, m_obituaries;

extern cvar_c debug_nomonsters;
extern cvar_c debug_fullbright, debug_subsector;

extern cvar_c in_autorun, in_stageturn, in_shiftlook;
extern cvar_c mouse_xaxis, mouse_yaxis;
extern cvar_c mouse_xsens, mouse_ysens;
extern cvar_c mouse_invert;

extern cvar_c r_width, r_height, r_depth, r_fullscreen;
extern cvar_c r_colormaterial, r_colorlighting;
extern cvar_c r_dumbsky, r_dumbmulti, r_dumbcombine, r_dumbclamp;
extern cvar_c r_nearclip, r_farclip, r_fadepower;
extern cvar_c r_fov, r_zoomedfov;
extern cvar_c r_mipmapping, r_smoothing;
extern cvar_c r_dithering, r_hq2x;
extern cvar_c r_dynamiclight, r_invultex;
extern cvar_c r_gamma, r_detaillevel;
extern cvar_c r_wipemethod, r_wipereverse;
extern cvar_c r_teleportflash;
extern cvar_c r_crosshair, r_crosscolor;
extern cvar_c r_crosssize, r_crossbright;

extern cvar_c s_volume, s_mixchan, s_quietfactor;
extern cvar_c s_rate, s_bits, s_stereo;
extern cvar_c s_musicvol, s_musicdevice;
extern cvar_c tim_quietfactor;


#ifdef WIN32
#define S_MUSICDEV_CFG  "0"  // hardware
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
    { "edge_compat",    &edge_compat,    "",    "0"  },

    { "g_mlook",        &g_mlook,        "c",   "1"  },
    { "g_autoaim",      &g_autoaim,      "c",   "1"  },
    { "g_jumping",      &g_jumping,      "c",   "0"  },
    { "g_crouching",    &g_crouching,    "c",   "0"  },
    { "g_true3d",       &g_true3d,       "c",   "1"  },
    { "g_noextra",      &g_noextra,      "c",   "0"  },
    { "g_moreblood",    &g_moreblood,    "c",   "0"  },
    { "g_fastmon",      &g_fastmon,      "c",   "0"  },
    { "g_passmissile",  &g_passmissile,  "c",   "1"  },
    { "g_weaponkick",   &g_weaponkick,   "c",   "0"  },
    { "g_weaponswitch", &g_weaponswitch, "c",   "1"  },

	{ "am_rotate",      &am_rotate,      "c",   "0"  },
	{ "am_smoothing",   &am_smoothing,   "c",   "1"  },
                                        
	{ "in_autorun",     &in_autorun,     "c",   "0"  },
	{ "in_stageturn",   &in_stageturn,   "c",   "1"  },
	{ "in_shiftlook",   &in_shiftlook,   "c",   "1"  },

	{ "mouse_xaxis",    &mouse_xaxis,    "c",   "0" /* AXIS_TURN */  },
	{ "mouse_yaxis",    &mouse_yaxis,    "c",   "3" /* AXIS_MLOOK */ },
	{ "mouse_xsens",    &mouse_xsens,    "c",   "10"  },
	{ "mouse_ysens",    &mouse_ysens,    "c",   "10"  },
	{ "mouse_invert",   &mouse_invert,   "c",   "0"  },
                                        
	{ "m_diskicon",     &m_diskicon,     "c",   "1"  },
	{ "m_busywait",     &m_busywait,     "c",   "1"  },
	{ "m_messages",     &m_messages,     "c",   "1"  },
	{ "m_obituaries",   &m_obituaries,   "c",   "1"  },
	{ "m_screenhud",    &m_screenhud,    "c",   "0"  },
                                        
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

	{ "r_dynamiclight", &r_dynamiclight, "c",   "1"  },
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

	{ "s_volume",       &s_volume,       "c",   "0.5"  },
	{ "s_mixchan",      &s_mixchan,      "c",   "32"   },
	{ "s_rate",         &s_rate,         "c",   "22050" },
	{ "s_bits",         &s_bits,         "c",   "16" },
	{ "s_stereo",       &s_stereo,       "c",   "1"  },
	{ "s_musicvol",     &s_musicvol,     "c",   "0.5"  },
	{ "s_musicdevice",  &s_musicdevice,  "c",   S_MUSICDEV_CFG  },

	{ "s_quietfactor",  &s_quietfactor,  "c",   "1"  },
	{ "tim_quietfactor",&tim_quietfactor,"c",   "1"  },

	{ "debug_nomonsters", &debug_nomonsters, "h", "0" },
	{ "debug_fullbright", &debug_fullbright, "h", "0" },
	{ "debug_subsector",  &debug_subsector,  "h", "0" },

//---- END OF LIST -----------------------------------------------------------

	{ NULL, NULL, NULL, NULL }
};

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
