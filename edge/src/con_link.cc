//----------------------------------------------------------------------------
//  LIST OF ALL CVARS
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2007-2008  The EDGE Team.
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


// TESTING STUFF
cvar_c r_width, r_height, r_depth, r_fullscreen;
cvar_c s_sfxvol, s_musvol;


cvar_link_t  all_cvars[] =
{

//----------------------------------------------------------------------------
//  CONFIG FILE + COMMAND LINE OPTION
//----------------------------------------------------------------------------

	// "edge_version"

	{ "cfor", &r_width,      "640",    "r_width", "width,screenwidth" },
	{ "cfor", &r_height,     "480",    "r_height", "height,screenheight" },
    { "cfor", &r_depth,      "32",     "r_depth", "bits,screendepth" },
    { "cforb",&r_fullscreen, "1",      "r_fullscreen", "fullscreen" },

	{ "cfo",  &s_sfxvol,     "10",     "s_sfxvol", "sfx_volume" },
	{ "cfo",  &s_musvol,     "10",     "s_musvol", "mus_volume" },

#if 0  // TODO
    {"directx",           &force_directx,  0},
    {"waveout",           &force_waveout,  0},
    {"usegamma",          &var_gamma,             CFGDEF_CURRENT_GAMMA},
 
    {"music_device",      &var_music_dev,             CFGDEF_MUSIC_DEVICE},
    {"sample_rate",       &var_sample_rate,             CFGDEF_SAMPLE_RATE},
    {"sound_bits",        &var_sound_bits,              CFGDEF_SOUND_BITS},
    {"sound_stereo",      &var_sound_stereo,            CFGDEF_SOUND_STEREO},
    {"mix_channels",      &var_mix_channels,            CFGDEF_MIX_CHANNELS},
    {"quiet_factor",      &var_quiet_factor,            CFGDEF_QUIET_FACTOR},
    {"timidity_quiet",    &var_timid_factor,            CFGDEF_QUIET_FACTOR},

    {"show_messages",     &showMessages,              CFGDEF_SHOWMESSAGES},
    {"autorun",           &autorunning,    0},
    {"mouse_sensitivity", &mouseSensitivity,          CFGDEF_MOUSESENSITIVITY},
    {"invertmouse",       &invertmouse,               CFGDEF_INVERTMOUSE},
    {"mlookspeed",        &mlookspeed,                CFGDEF_MLOOKSPEED},

    {"telept_effect",     &telept_effect,             CFGDEF_TELEPT_EFFECT},
    {"telept_reverse",    &telept_reverse,            CFGDEF_TELEPT_REVERSE},
    {"telept_flash",      &telept_flash,              CFGDEF_TELEPT_FLASH},
    {"invuln_fx",         &var_invul_fx,              CFGDEF_INVUL_FX},
    {"wipe_method",       &wipe_method,               CFGDEF_WIPE_METHOD},
    {"wipe_reverse",      &wipe_reverse,              CFGDEF_WIPE_REVERSE},
    {"crosshair",         &crosshair,                 CFGDEF_CROSSHAIR},
    {"rotatemap",         &rotatemap,                 CFGDEF_ROTATEMAP},
    {"respawnsetting",    &global_flags.res_respawn,  CFGDEF_RES_RESPAWN},
    {"itemrespawn",       &global_flags.itemrespawn,  CFGDEF_ITEMRESPAWN},
    {"respawn",           &global_flags.respawn,      CFGDEF_RESPAWN},
    {"fastparm",          &global_flags.fastparm,     CFGDEF_FASTPARM},
    {"grav",              &global_flags.menu_grav,    CFGDEF_MENU_GRAV},
    {"true3dgameplay",    &global_flags.true3dgameplay,CFGDEF_TRUE3DGAMEPLAY},
    {"autoaim",           &global_flags.autoaim,      CFGDEF_AUTOAIM},
    {"doom_fading",       &doom_fading,               CFGDEF_DOOM_FADING},
    {"shootthru_scenery", &global_flags.pass_missile, CFGDEF_PASS_MISSILE},

    {"blood",             &global_flags.more_blood,   CFGDEF_MORE_BLOOD},
    {"extra",             &global_flags.have_extra,   CFGDEF_HAVE_EXTRA},
    {"shadows",           &global_flags.shadows,      CFGDEF_SHADOWS},
    {"halos",             &global_flags.halos, 0},
    {"weaponkick",        &global_flags.kicking,      CFGDEF_KICKING},
    {"weaponswitch",      &global_flags.weapon_switch,CFGDEF_WEAPON_SWITCH},
    {"mlook",             &global_flags.mlook,        CFGDEF_MLOOK},
    {"jumping",           &global_flags.jump,         CFGDEF_JUMP},
    {"crouching",         &global_flags.crouch,       CFGDEF_CROUCH},
    {"mipmapping",        &var_mipmapping,            CFGDEF_USE_MIPMAPPING},
    {"smoothing",         &var_smoothing,             CFGDEF_USE_SMOOTHING},
    {"dither",            &var_dithering, 0},
    {"dlights",           &use_dlights,               CFGDEF_USE_DLIGHTS},
    {"detail_level",      &detail_level,              CFGDEF_DETAIL_LEVEL},
    {"hq2x_scaling",      &hq2x_scaling,              CFGDEF_HQ2X_SCALING},

    {"mouse_xaxis",       &mouse_xaxis,               CFGDEF_MOUSE_XAXIS},
    {"mouse_yaxis",       &mouse_yaxis,               CFGDEF_MOUSE_YAXIS},

    {"twostage_turning",  &stageturn,                 CFGDEF_STAGETURN},
    {"forwardmove_speed", &forwardmovespeed,            CFGDEF_FORWARDMOVESPEED},
    {"angleturn_speed",   &angleturnspeed,            CFGDEF_ANGLETURNSPEED},
    {"sidemove_speed",    &sidemovespeed,             CFGDEF_SIDEMOVESPEED},

    {"joy_xaxis",         &joy_xaxis,                 CFGDEF_JOY_XAXIS},
    {"joy_yaxis",         &joy_yaxis,                 CFGDEF_JOY_YAXIS},

    {"screen_hud",        &screen_hud,                CFGDEF_SCREEN_HUD},

    {"fieldofview",       &cfgnormalfov,              CFGDEF_NORMALFOV},
    {"zoomedfieldofview", &cfgzoomedfov,              CFGDEF_ZOOMEDFOV},

    {"save_page",         &save_page, 0},

    {"png_scrshots",      &png_scrshots,              CFGDEF_PNG_SCRSHOTS},

    /------- VARS --------------------

    {"var_diskicon",      &var_diskicon,   1},
    {"var_hogcpu",        &var_hogcpu,     1},
    {"var_fadepower",     &var_fadepower,  1},
    {"var_smoothmap",     &var_smoothmap,  1},

    {"var_nearclip",      &var_nearclip,   4},
    {"var_farclip",       &var_farclip,    64000},
#endif


//---- END OF LIST -----------------------------------------------------------

	{ NULL, 0, NULL }
};

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
