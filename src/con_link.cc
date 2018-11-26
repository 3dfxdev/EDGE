//----------------------------------------------------------------------------
//  LIST OF ALL CVARS
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2007-2009  The EDGE2 Team.
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

#include "system/i_defs.h"

#include "con_var.h"

#include "e_input.h"  // jaxis_group_c


extern cvar_c edge_compat;

extern cvar_c ddf_strict, ddf_lax, ddf_quiet;

extern cvar_c g_skill, g_gametype;
extern cvar_c g_mlook, g_autoaim;
extern cvar_c g_jumping, g_crouching;
extern cvar_c g_true3d, g_aggression;
extern cvar_c g_moreblood, g_noextra;
extern cvar_c g_fastmon, g_passmissile;
extern cvar_c g_weaponkick, g_weaponswitch;

extern cvar_c debug_testlerp; //for testing ticrate lerp code.

extern cvar_c am_rotate, am_smoothing;
extern cvar_c am_gridsize;

extern cvar_c m_language;
extern cvar_c m_diskicon, m_busywait, m_screenhud;
extern cvar_c m_tactile, melee_tactile; //screen shake
extern cvar_c m_messages, m_obituaries;
extern cvar_c m_centerem;
extern cvar_c m_goobers;

extern cvar_c sys_directx, sys_waveout;

extern cvar_c in_running, in_stageturn, in_shiftlook;
extern cvar_c in_keypad,  in_warpmouse;
extern cvar_c in_grab;

extern cvar_c mouse_x_axis, mouse_y_axis;
extern cvar_c mouse_x_sens, mouse_y_sens;
extern cvar_c mouse_accel,  mouse_filter;

extern cvar_c joy_dead, joy_peak, joy_tuning;

extern cvar_c r_spriteflip;

extern cvar_c r_textscale, r_text_x, r_text_y, r_text_alpha;

extern cvar_c r_width, r_height, r_depth, r_fullscreen;
extern cvar_c r_colormaterial, r_colorlighting;
extern cvar_c r_dumbsky, r_dumbmulti, r_dumbcombine, r_dumbclamp;
extern cvar_c r_nearclip, r_farclip, r_fadepower;
extern cvar_c r_fov, r_zoomfov, r_aspect;
extern cvar_c r_mipmapping, r_smoothing, r_anisotropy;// , r_anisotropyval;
extern cvar_c r_dithering, r_hq2x;
extern cvar_c r_dynlight, r_invultex;
extern cvar_c r_gamma, r_detaillevel;
extern cvar_c r_wipemethod, r_wipereverse;
extern cvar_c r_teleportflash;
extern cvar_c r_crosshair, r_crosscolor;
extern cvar_c r_crosssize, r_crossbright;
extern cvar_c r_precache_tex, r_precache_sprite, r_precache_model;
extern cvar_c r_gl3_path;
extern cvar_c r_renderprecise;
extern cvar_c r_oldblend;

extern cvar_c r_bloom, r_bloom_amount;//, r_exposure_scale, r_exposure_min, r_exposure_base, r_exposure_speed, r_bloom_kernal_size;
extern cvar_c r_lens;
extern cvar_c r_fxaa;
extern cvar_c r_fxaa_quality;

extern cvar_c r_stretchworld;
extern cvar_c r_fixspritescale;

extern cvar_c r_gpuswitch;

extern cvar_c r_transfix;

extern cvar_c s_volume, s_mixchan, s_quietfactor;
extern cvar_c s_rate, s_bits, s_stereo;
extern cvar_c s_musicvol, s_musicdevice;
extern cvar_c tim_quietfactor;

extern cvar_c debug_fullbright, debug_hom;
extern cvar_c debug_mouse,      debug_joyaxis;
extern cvar_c debug_fps,        debug_pos;

extern cvar_c r_lerp, r_maxfps, r_vsync, r_swapinterval;
extern cvar_c r_shadows;
extern cvar_c r_md5scale;

extern cvar_c debug_nomonsters, debug_subsector;
extern cvar_c camera_subdir;

extern cvar_c sound_pitch;

extern cvar_c i_skipsplash;



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

	{ "language",       &m_language,     "c",   "ENGLISH" },

	{ "ddf_strict",     &ddf_strict,     "c",   "0"  },
	{ "ddf_lax",        &ddf_lax,        "c",   "0"  },
	{ "ddf_quiet",      &ddf_quiet,      "c",   "0"  },

	{ "aggression",     &g_aggression,   "c",   "0"  },

	{ "spriteflip",      &r_spriteflip,  "c",   "0"  },

	{ "screen_shake",    &m_tactile,     "c",   "1"  },
	{ "melee_tactile",   &melee_tactile, "c",   "0"  },

	{ "shadows",         &r_shadows,     "c",   "1"  },

	{ "soundpitch",      &sound_pitch,   "c",    "1" },

	{ "r_oldblend",      &r_oldblend,    "c",    "1" },

	// If palette incorrectly remaps palette, this can be set.
	{ "r_transfix",      &r_transfix,    "c",    "0" },

	/* Input Stuff */

	{ "in_grab",        &in_grab,        "c",   "1"  },
	{ "in_keypad",      &in_keypad,      "c",   "1"  },
	{ "in_running",     &in_running,     "c",   "0"  },
	{ "in_stageturn",   &in_stageturn,   "c",   "1"  },
	{ "in_warpmouse",   &in_warpmouse,   "c",   "1"  },

	{ "joy_dead",       &joy_dead,       "c",   "0.15" },
	{ "joy_peak",       &joy_peak,       "c",   "0.95" },
	{ "joy_tuning",     &joy_peak,       "c",   "1.0"  },

	{ "mouse_filter",   &mouse_filter,   "c",   "0"  },

	{ "goobers",        &m_goobers,      "",    "0" },
	{ "m_diskicon",     &m_diskicon,     "c",   "0"  },
	{ "m_busywait",     &m_busywait,     "c",   "1"  },
	{ "camera_subdir",  &camera_subdir,  "c",   "doom_ddf/cameras" },

	/* Experimental Text Scaling Stuff*/
	{ "r_textscale",    &r_textscale,   "c",   "0.7" }, //0.7f is the default for HUD_SetScale(). Sets HUD Text Scale. Dupliate for RTS tips.
	{ "r_text_xpos",    &r_text_x,      "c",   "160" }, // Center text on the X Axis
	{ "r_text_ypos",    &r_text_y,      "c",   "3" }, // Align text on the Y Axis

	/* Rendering Stuff */

	{ "r_aspect",       &r_aspect,       "c",   "1.777" }, //Default to 16:9 mode!!
	{ "r_fov",          &r_fov,          "c",   "90" },
	{ "r_zoomfov",      &r_zoomfov,      "c",   "10" },

	{ "r_crosshair",    &r_crosshair,    "c",   "0"  },
	{ "r_crosscolor",   &r_crosscolor,   "c",   "0"  },
	{ "r_crosssize",    &r_crosssize,    "c",   "16" },
	{ "r_crossbright",  &r_crossbright,  "c",   "1.0" },

	{ "r_nearclip",     &r_nearclip,     "c",   "4"  },
	{ "r_farclip",      &r_farclip,      "c",   "64000" },
	{ "r_fadepower",    &r_fadepower,    "c",   "1"  },

	{ "r_precache_tex",    &r_precache_tex,    "c", "1" },
	{ "r_precache_sprite", &r_precache_sprite, "c", "1" },
	{ "r_precache_model",  &r_precache_model,  "c", "1" },

	{ "r_anisotropy",	&r_anisotropy,	  "c",  "0"  },
	{ "r_colormaterial",&r_colormaterial, "",   "1"  },
	{ "r_colorlighting",&r_colorlighting, "",   "1"  },
	{ "r_dumbsky",      &r_dumbsky,       "",   "0"  },
	{ "r_dumbmulti",    &r_dumbmulti,     "",   "0"  },
	{ "r_dumbcombine",  &r_dumbcombine,   "",   "0"  },
	{ "r_dumbclamp",    &r_dumbclamp,     "",   "0"  },
	{ "r_gl3_path",     &r_gl3_path,      "c",  "0"  },
	{ "r_swapinterval", &r_swapinterval,  "",   "1"  },

	{ "r_gpuswitch",    &r_gpuswitch,     "c",   "0" }, // notebook optimus gpu selector

	{ "r_stretchworld", &r_stretchworld, "c",   "1"  },
	{ "r_fixspritescale", &r_fixspritescale, "c", "1"},
	{ "r_renderprecise", &r_renderprecise, "c", "0"  },

	{ "am_smoothing",   &am_smoothing,   "c",   "1"  },
	{ "am_gridsize",    &am_gridsize,    "c",   "128"},

	/* Sound Stuff */

	/* Debugging Stuff */

	{ "debug_fullbright", &debug_fullbright, "h", "0" },
	{ "debug_hom",        &debug_hom,        "h", "0" },
	{ "debug_joyaxis",    &debug_joyaxis,    "",  "0" },
	{ "debug_mouse",      &debug_mouse,      "",  "0" },
	{ "debug_pos",        &debug_pos,        "h", "0" },
	{ "debug_fps",        &debug_fps,        "c", "0" },
	{ "debug_lerp",       &debug_testlerp,   "c", "0"},

	{ "r_md5scale",        &r_md5scale,        "c", "0"},
	{ "r_lerp",			   &r_lerp,        "c", "1" },
	{ "r_maxfps",          &r_maxfps,        "c", "0" }, //experimental. . .
	{ "r_vsync",           &r_vsync,        "c", "1" },


	{ "r_bloom",           &r_bloom,        "c", "1" }, 
	{ "r_bloom_amount",    &r_bloom_amount, "c", "1.5" },
#if 0
	{ "r_exposure_scale",  &r_exposure_scale, "c", "2.0" },
	{ "r_exposure_min",    &r_exposure_min, "c", "0.1" },
	{ "r_exposure_base",   &r_exposure_base, "c", "0.1" },
	{ "r_exposure_speed",  &r_exposure_speed, "c", "0.05" },
#endif // 0

	{ "r_lens",		       &r_lens,			"c", "1" },
	{ "r_fxaa",            &r_fxaa,         "c", "0" },
	{ "r_fxaa_quality",    &r_fxaa_quality, "c", "0" },

	{ "i_skipsplash",	&i_skipsplash,		"c", "0" },

#if 0 // FIXME
    { "edge_compat",    &edge_compat,    "",    "0"  },

    { "sys_directx",    &sys_directx,    "c",   "0"  },
    { "sys_waveout",    &sys_waveout,    "c",   "0"  },

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
    { "g_passmissile",  &g_passmissile,  "c",   "1"  },
    { "g_weaponkick",   &g_weaponkick,   "c",   "0"  },
    { "g_weaponswitch", &g_weaponswitch, "c",   "1"  },

	{ "am_rotate",      &am_rotate,      "c",   "0"  },
                                        
	{ "m_messages",     &m_messages,     "c",   "1"  },
	{ "m_obituaries",   &m_obituaries,   "c",   "1"  },
	{ "m_screenhud",    &m_screenhud,    "c",   "0"  },

	{ "r_width",        &r_width,        "c",   "640"   },
	{ "r_height",       &r_height,       "c",   "480"   },
    { "r_depth",        &r_depth,        "c",   "32"    },
    { "r_fullscreen",   &r_fullscreen,   "c",   "0"     },
												
	{ "r_gamma",        &r_gamma,        "c",   "1"  },

	{ "r_mipmapping",   &r_mipmapping,   "c",   "0"  },
	{ "r_smoothing",    &r_smoothing,    "c",   "0"  },
	{ "r_dithering",    &r_dithering,    "c",   "0"  },
	{ "r_hq2x",         &r_hq2x,         "c",   "0"  },

	{ "r_dynlight",     &r_dynlight,     "c",   "1"  },
	{ "r_detaillevel",  &r_detaillevel,  "c",   "1"  },
	{ "r_invultex",     &r_invultex,     "c",   "1"  },
	{ "r_wipemethod",   &r_wipemethod,   "c",   "1" /* Melt */ },
	{ "r_wipereverse",  &r_wipereverse,  "c",   "0"  },
	{ "r_teleportflash",&r_teleportflash,"c",   "1"  },

	{ "s_volume",       &s_volume,       "c",   "0.5"  },
	{ "s_mixchan",      &s_mixchan,      "c",   "32"   },
	{ "s_rate",         &s_rate,         "c",   "22050" },
	{ "s_bits",         &s_bits,         "c",   "16" },
	{ "s_stereo",       &s_stereo,       "c",   "1"  },
	{ "s_musicvol",     &s_musicvol,     "c",   "0.5"  },
	{ "s_musicdevice",  &s_musicdevice,  "c",   S_MUSICDEV_CFG },

	{ "s_quietfactor",  &s_quietfactor,  "c",   "1"  },
	{ "tim_quietfactor",&tim_quietfactor,"c",   "1"  },

	{ "in_shiftlook",   &in_shiftlook,   "c",   "1"  },

	{ "mouse_x.axis",   &mouse_x_axis,   "c",   "1" /* AXIS_TURN */  },
	{ "mouse_x.sens",   &mouse_x_sens,   "c",   "10"  },
	{ "mouse_y.axis",   &mouse_y_axis,   "c",   "4" /* AXIS_MLOOK */ },
	{ "mouse_y.sens",   &mouse_y_sens,   "c",   "10" },
//	{ "mouse_accel",    &mouse_accel,    "c",   "0"  },

	{ "debug_nomonsters", &debug_nomonsters, "h", "0" },
	{ "debug_subsector",  &debug_subsector,  "h", "0" },
#endif

//---- END OF LIST -----------------------------------------------------------

	{ NULL, NULL, NULL, NULL }
};

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
