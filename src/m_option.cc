//----------------------------------------------------------------------------
//  EDGE2 Option Menu Modification
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
//
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------
//
// Original Author: Chi Hoang
//
// -ACB- 1998/06/15 All functions are now m_* to follow doom standard.
//
// -MH-  1998/07/01 Shoot Up/Down becomes "True 3D Gameplay"
//                  Added keys for fly up and fly down
//
// -KM-  1998/07/10 Used better names :-) (Controls Menu)
//
// -ACB- 1998/07/10 Print screen is now possible in this menu
//
// -ACB- 1998/07/12 Removed Lost Soul/Spectre Ability Menu Entries
//
// -ACB- 1998/07/15 Changed menu structure for graphic titles
//
// -ACB- 1998/07/30 Remove M_SetRespawn and the newnmrespawn &
//                  respawnmonsters. Used new respawnsetting variable.
//
// -ACB- 1998/08/10 Edited the menu's to reflect the fact that currmap
//                  flags can prevent changes.
//
// -ES-  1998/08/21 Added resolution options
//
// -ACB- 1998/08/29 Modified Resolution menus for user-friendlyness
//
// -ACB- 1998/09/06 MouseOptions renamed to AnalogueOptions
//
// -ACB- 1998/09/11 Cleaned up and used new white text colour for menu
//                  selections
//
// -KM- 1998/11/25 You can scroll backwards through the resolution list!
//
// -ES- 1998/11/28 Added faded teleportation option
//
// -ACB- 1999/09/19 Removed All CD Audio References.
//
// -ACB- 1999/10/11 Reinstated all sound volume controls.
//
// -ACB- 1999/11/19 Reinstated all music volume controls.
//
// -ACB- 2000/03/11 All menu functions now have the keypress passed
//                  that called them.
//
// -ACB- 2000/03/12 Menu resolution hack now allow user to cycle both
//                  ways through the resolution list.
//
// -AJA- 2001/07/26: Reworked colours, key config, and other code.
//
// -CA-  2016.2.24: Added more console -> menu customizations

#include "system/i_defs.h"

#include "../epi/str_format.h"

#include "../ddf/main.h"

#include "dm_state.h"
#include "e_input.h"
#include "g_game.h"
#include "hu_draw.h"
#include "hu_stuff.h"
#include "hu_style.h"
#include "m_menu.h"
#include "m_misc.h"
#include "m_netgame.h"
#include "m_option.h"
#include "n_network.h"
#include "p_local.h"
#include "r_misc.h"
#include "r_gldefs.h"
#include "s_sound.h"
#include "s_music.h"
#include "s_timid.h"
#include "am_map.h"
#include "r_draw.h"
#include "r_modes.h"
#include "r_colormap.h"
#include "r_image.h"
#include "w_wad.h"
#include "r_wipe.h"

#include "defaults.h"

int option_menuon = 0;

extern cvar_c m_language;
extern cvar_c r_crosshair;
extern cvar_c r_crosssize;
extern cvar_c r_lerp;
extern cvar_c r_gl3_path;
extern cvar_c r_md5scale;
extern cvar_c debug_pos;
extern cvar_c debug_fps;
extern cvar_c debug_testlerp;
extern cvar_c r_vsync;
extern cvar_c m_goobers;
extern cvar_c mouse_filter;
extern cvar_c m_diskicon;
extern cvar_c debug_mouse;
extern cvar_c debug_joyaxis;
extern cvar_c g_aggression;
extern cvar_c m_busywait;
extern cvar_c r_shadows;
extern cvar_c r_bloom;
extern cvar_c r_lens;
extern cvar_c sound_pitch;
extern cvar_c r_stretchworld;
extern cvar_c r_fixspritescale;
extern cvar_c m_tactile;

//extern cvar_c r_textscale; //temp hack for HUD text scaling size

static int menu_crosshair;  // temp hack
static int menu_crosshair2;  /// love haxxx
extern int monitor_size;

extern int joystick_device;

extern bool heretic_mode;

//submenus
static void M_KeyboardOptions(int keypressed);
static void M_VideoOptions(int keypressed);
static void M_AdvancedOptions(int keypressed);
static void M_GameplayOptions(int keypressed); /// Make Gameplay Options page-flip for MORE options...apparently...
static void M_AnalogueOptions(int keypressed);
static void M_SoundOptions(int keypressed);
static void M_DebugMenu(int keypressed); /// New Debugging Sub-menu

static void M_Key2String(int key, char *deststring);

// -ACB- 1998/08/09 "Does Map allow these changes?" procedures.
static void M_ChangeMonsterRespawn(int keypressed);
static void M_ChangeItemRespawn(int keypressed);
static void M_ChangeTrue3d(int keypressed);
static void M_ChangeAutoAim(int keypressed);
static void M_ChangeFastparm(int keypressed);
static void M_ChangeRespawn(int keypressed);
static void M_ChangePassMissile(int keypressed);
static void M_ChangeCrossHair(int keypressed);
static void M_ChangeCrossHairSize(int keypressed);
static void M_ChangeHUDTextScale(int keypressed); // HUD_SetScale() User-Definable scaling stuff.

static void M_ChangeBlood(int keypressed);
static void M_ChangeMLook(int keypressed);
static void M_ChangeJumping(int keypressed);
static void M_ChangeCrouching(int keypressed);
static void M_ChangeExtra(int keypressed);
static void M_ChangeGamma(int keypressed);
static void M_ChangeShadows(int keypressed);
static void M_ChangeMonitorSize(int keypressed);
static void M_ChangeKicking(int keypressed);
static void M_ChangeWeaponSwitch(int keypressed);
static void M_ChangeMipMap(int keypressed);
static void M_ChangeDLights(int keypressed);
/* static void M_ChangeHalos(int keypressed); */

// -ES- 1998/08/20 Added resolution options
// -ACB- 1998/08/29 Moved to top and tried different system

static void M_ResOptDrawer(style_c *style, int topy, int bottomy, int dy, int centrex);
static void M_ResolutionOptions(int keypressed);
static void M_OptionSetResolution(int keypressed);
///--static void M_OptionTestResolution(int keypressed);
///--  static void M_RestoreResSettings(int keypressed);
static void M_ChangeResSize(int keypressed);
static void M_ChangeResDepth(int keypressed);
static void M_ChangeResFull(int keypressed);

static void M_HostNetGame(int keypressed);
static void M_JoinNetGame(int keypressed);

static void M_LanguageDrawer(int x, int y, int deltay);
static void M_ChangeLanguage(int keypressed);

static char YesNo[] = "Off/On";  // basic on/off
static char Lerp[] = "None/1(worst)/2(better)/3(best)";
static char CrossH[] = "None/Dot/Angle/Plus/Spiked/Thin/Cross/Carat/Circle/Double";
static char HudT[] = "Values range from 0.1f - 2.0f";
static char Respw[] = "Teleport/Resurrect";  // monster respawning
static char Axis[] = "Off/+Turn/-Turn/+MLook/-MLook/+Forward/-Forward/+Strafe/-Strafe/+Fly/-Fly";
static char JoyDevs[] = "None/1/2/3/4/5/6";
static char DLMode[] = "Off/On";
static char JpgPng[] = "JPEG/PNG";  // basic on/off
static char AAim[] = "Off/On/Mlook";
static char MipMaps[] = "None/Bilinear/Trilinear"; ///CA - Renamed for better understanding of what they are changing
static char Anisotropy[] = "Off/On";
static char Details[] = "Low/Medium/High";
static char Hq2xMode[] = "Off/UI Only/UI & Sprites/All";
static char Invuls[] = "Simple/Complex/Textured";
static char MonitSiz[] = "4:3/16:9/16:10/3:2/24:10";
static char GLMode[] = "1/2";
static char VsyncValue[] = "1/2/3";
static char Shadows[] = "None/Simple/Sprite"; //TODO: Add in "Complex" Shadows for 3D Models and Dynamic Light Projection!
static char TeleEff[] = "None/Fade In/Warp In";
static char StretchWorld[] = "Off/On";

// for CVar enums
const char WIPE_EnumStr[] = "none/melt/crossfade/pixelfade/top/bottom/left/right/spooky/doors";

static char SampleRates[] = "11025 Hz/16000 Hz/22050 Hz/32000 Hz/44100 Hz";
static char SoundBits[] = "8 bit/16 bit/32 bit";
static char StereoNess[] = "Off/On/Swapped";
static char MixChans[] = "8/16/32/64/96";
static char QuietNess[] = "Loud (distorted)/Normal/Soft/Very Soft";
static char MusicDevs[] = "System/Timidity/OPL";
static char SoundPitching[] = "Off/On";
static char OPL[] = "OPL 1/OPL 3";

// Screen resolution changes
static scrmode_c new_scrmode;

//
//  OPTION STRUCTURES
//

typedef enum
{
	OPT_Plain = 0,  // 0 means plain text,
	OPT_Switch = 1,  // 1 is change a switch,
	OPT_Function = 2,  // 2 is call a function,
	OPT_Slider = 3,  // 3 is a slider,
	OPT_KeyConfig = 4,  // 4 is a key config,
	OPT_Boolean = 5,  // 5 is change a boolean switch
	OPT_NumTypes
}
opt_type_e;

typedef struct optmenuitem_s
{
	opt_type_e type;

	char name[48];
	const char *typenames;

	int numtypes;
	void *switchvar;

	void(*routine)(int keypressed);

	const char *help;
}
optmenuitem_t;

typedef struct menuinfo_s
{
	// array of menu items
	optmenuitem_t *items;
	int item_num;

	style_c **style_var;

	int menu_center;

	// title information
	int title_x;
	char title_name[10];
	const image_c *title_image;

	// current position
	int pos;

	// key config, with left and right sister menus ?
	char key_page[20];
}
menuinfo_t;

// current menu and position
static menuinfo_t *curr_menu;
static optmenuitem_t *curr_item;
static int curr_key_menu;

static int keyscan;

static style_c *opt_def_style;
static style_c *keyboard_style;
static style_c *mouse_style;
static style_c *gameplay_style;
static style_c *video_style;
static style_c *setres_style;
static style_c *advanced_style;
static style_c *debugmenu_style; //debugging menu "style"

static void M_ChangeMusVol(int keypressed)
{
	S_ChangeMusicVolume();
}

static void M_ChangeSfxVol(int keypressed)
{
	S_ChangeSoundVolume();
}

static void M_ChangeMixChan(int keypressed)
{
	S_ChangeChannelNum();
}

static void M_ChangeTimidQuiet(int keypressed)
{
	S_ChangeTimidQuiet();
}

static int M_GetCurrentSwitchValue(optmenuitem_t *item)
{
	int retval = 0;

	switch (item->type)
	{
	case OPT_Boolean:
	{
		retval = *(bool*)item->switchvar ? 1 : 0;
		break;
	}

	case OPT_Switch:
	{
		retval = *(int*)(item->switchvar);
		break;
	}

	default:
	{
		I_Error("M_GetCurrentSwitchValue: Menu item type is not a switch!\n");
		break;
	}
	}

	return retval;
}

//
//  MAIN MENU
//
#define LANGUAGE_POS  11
#define HOSTNET_POS   14

static optmenuitem_t mainoptions[] =
{
	{OPT_Function, "Keyboard Controls", NULL,  0, NULL, M_KeyboardOptions, "Controls"},
	{OPT_Function, "Mouse / Joystick",  NULL,  0, NULL, M_AnalogueOptions, "AnalogueOptions"},
	{OPT_Plain,    "",                  NULL,  0, NULL, NULL, NULL},
	{OPT_Function, "Gameplay Options",  NULL,  0, NULL, M_GameplayOptions, "GameplayOptions"},
	{OPT_Plain,    "",                  NULL,  0, NULL, NULL, NULL},
	{OPT_Function, "Sound Options",     NULL,  0, NULL, M_SoundOptions, "SoundOptions"},
	{OPT_Function, "Screen Options",    NULL,  0, NULL, M_VideoOptions, "VideoOptions"},
	{OPT_Function, "Video Options",     NULL,  0, NULL, M_AdvancedOptions, "AdvancedOptions"}, //call to LDF
	{ OPT_Plain,    "",                  NULL,  0, NULL, NULL, NULL },

	{OPT_Function, "Set Resolution",    NULL,  0, NULL, M_ResolutionOptions, "ChangeRes"},

	{OPT_Plain,    "",                  NULL,  0, NULL, NULL, NULL},
	{OPT_Function, "Language",          NULL,  0, NULL, M_ChangeLanguage, NULL},
	{OPT_Switch,   "Messages",          YesNo, 2, &showMessages, NULL, "Messages"},
	{OPT_Plain,    "",                  NULL,  0, NULL, NULL, NULL},
	{OPT_Function, "Debug Menu",	    NULL,  0, NULL, M_DebugMenu, "Debugging Options" },
	{OPT_Function, "Advanced Start",    NULL,  0, NULL, M_HostNetGame, NULL },

	{OPT_Plain,    "",                  NULL,  0, NULL, NULL, NULL},
	{OPT_Function, "Reset to Defaults", NULL,  0, NULL, M_ResetDefaults, NULL}
};

static menuinfo_t main_optmenu =
{
	mainoptions,
	sizeof(mainoptions) / sizeof(optmenuitem_t),
	&opt_def_style,
	164,
	108,
	"M_OPTTTL",
	NULL,
	0,
	""
};

static menuinfo_t hereticmain_optmenu =
{
	mainoptions,
	sizeof(mainoptions) / sizeof(optmenuitem_t),
	&opt_def_style,
	164,
	108,
	"H_OPTION",
	NULL,
	0,
	""
};

//
//  VIDEO OPTIONS
//
// -ACB- 1998/07/15 Altered menu structure

static optmenuitem_t vidoptions[] =
{
	{OPT_Slider,  "Brightness",    NULL,  6,  &var_gamma, M_ChangeGamma, NULL},

	{OPT_Plain,   "",  NULL,  0,  NULL, NULL, NULL},

	{OPT_Switch,  "Monitor Size",  MonitSiz,  5, &monitor_size, M_ChangeMonitorSize, NULL},
	{OPT_Switch,  "Smoothing",     YesNo, 2, &var_smoothing, M_ChangeMipMap, NULL},
	{OPT_Switch,  "H.Q.2x Scaling", Hq2xMode, 4, &hq2x_scaling, M_ChangeMipMap, NULL},
	{OPT_Switch ,  "Detail Level",   Details,  3, &detail_level, M_ChangeMipMap, NULL},
	{OPT_Switch,  "Texture Filtering",     MipMaps,  3, &var_mipmapping, M_ChangeMipMap, NULL},
	{ OPT_Plain,   "",  NULL,  0,  NULL, NULL, NULL },
	{ OPT_Plain,   "",  NULL,  0,  NULL, NULL, NULL },
	{OPT_Boolean, "Show Disk Icon",  YesNo, 1, &m_diskicon, NULL, NULL},
	//{OPT_Slider,  "HUD Text Scale",  HudT,  20,  &r_textscale, NULL, "Experimental"},
	{OPT_Boolean, "Screen Shake",  YesNo, 1, &m_tactile, NULL, NULL },
	{OPT_Switch,  "Crosshair",       CrossH, 10, &menu_crosshair, M_ChangeCrossHair, NULL},
	{OPT_Slider, "Crosshair Scale",  NULL, 15, &menu_crosshair2, M_ChangeCrossHairSize, NULL }, /// -- New Crosshair Size Slider (like Global MD5 Scale), define this in LDF!
	{OPT_Plain,   "",  NULL,  0,  NULL, NULL, NULL },
	{OPT_Boolean, "Map Rotation",    YesNo,   2, &rotatemap, NULL, NULL},
	{OPT_Switch,  "Teleport Flash",  YesNo,   2, &telept_flash, NULL, NULL},
	{OPT_Switch,  "Teleport Effect",  TeleEff,   3, &telept_effect, NULL, NULL},
	{OPT_Switch,  "Wipe method",     WIPE_EnumStr, WIPE_NUMWIPES, &wipe_method, NULL, NULL},
};

static optmenuitem_t advancedoptions[] =
{
	{OPT_Switch, "OpenGL 3 Mode",     YesNo, 2, &r_gl3_path, NULL, "OpenGL defaults to 1.x (off)"}, /// Change from GL1 to GL3
	{ OPT_Plain,   "",  NULL,  0,  NULL, NULL, NULL },
	{OPT_Switch,   "Bloom Processing",  YesNo,  2,  &r_bloom, NULL, "Toggle Bloom Shader On or Off"},
	{OPT_Plain,   "",  NULL,  0,  NULL, NULL, NULL },
	{OPT_Switch,   "Lens Distortion",  YesNo,  2,  &r_lens, NULL, "Toggle Lens Distortion Effect"},
	{OPT_Plain,   "",  NULL,  0,  NULL, NULL, NULL},
	{OPT_Boolean, "Video Sync",   YesNo,   2, &r_vsync, NULL, "Enable VSYNC"},
	{OPT_Plain,   "",  NULL,  0,  NULL, NULL, NULL },
	{OPT_Switch,  "Interpolation",    YesNo,   2, &r_lerp, NULL, "Frame Prediction"},
	{OPT_Plain,   "",  NULL,  0,  NULL, NULL, NULL},
	{OPT_Switch,  "Dynamic Lighting", DLMode, 2, &use_dlights, M_ChangeDLights, "DynaLight"},
	{OPT_Plain,   "",  NULL,  0,  NULL, NULL, NULL },
	{OPT_Switch,  "Shadows", Shadows, 3,  &r_shadows, NULL, "Simple, Sprite"},
	{OPT_Plain,   "",  NULL,  0,  NULL, NULL, NULL },
	{OPT_Switch,  "Stretch World", YesNo, 2,  &r_stretchworld, NULL, "StretchWorld" }, //LDF Lookup
	{OPT_Plain,   "",  NULL,  0,  NULL, NULL, NULL },
	{OPT_Switch,  "Fix Sprite Scale", YesNo, 2,  &r_fixspritescale, NULL, "SpriteScale" }, //LDF Lookup

	{OPT_Switch,  "Invulnerability", Invuls, NUM_INVULFX,  &var_invul_fx, NULL, "Set GodMode Rendering"},
	{OPT_Plain,   "",  NULL,  0,  NULL, NULL, NULL },
	{OPT_Boolean, "Screenshot Format", JpgPng, 2, &png_scrshots, NULL, NULL }
};

static optmenuitem_t debuggingoptions[] =
{
	{ OPT_Plain,   "<---Debugging--->",  NULL, 0, NULL, NULL, NULL },
	{ OPT_Plain,   "",  NULL,  0,  NULL, NULL, NULL },
	{ OPT_Switch,  "Framerate Info",    YesNo,  2,  &debug_fps, NULL, NULL },
	{ OPT_Plain,   "",  NULL,  0,  NULL, NULL, NULL },
	{ OPT_Plain,   "",  NULL,  0,  NULL, NULL, NULL },
	{ OPT_Switch,  "Show HOM Errors",    YesNo,  2,  &debug_hom, NULL, "showhom" },
	{ OPT_Plain,   "",  NULL,  0,  NULL, NULL, NULL },
	{ OPT_Switch,  "Show Position Coords",    YesNo,  2,  &debug_pos, NULL, NULL },
	{ OPT_Plain,   "",  NULL,  0,  NULL, NULL, NULL },
	{ OPT_Switch,  "Busy/Wait?", YesNo,  2,  &m_busywait, NULL, "BusyWait" }, //Call to LDF.
	{ OPT_Plain,   "",  NULL,  0,  NULL, NULL, NULL },
	{ OPT_Switch,  "Get Psyched!",   YesNo, 2,  &m_goobers, NULL, "Wolf3D Mode: Requires map restart!" },
	{ OPT_Plain,   "",  NULL,  0,  NULL, NULL, NULL },
	{OPT_Slider,   "Global MD5 Scale",    NULL,  4,  &r_md5scale, NULL, "(debugging)"}
};

///Screen Options, custom graphic by Julian
static menuinfo_t video_optmenu =
{
	vidoptions, sizeof(vidoptions) / sizeof(optmenuitem_t),
	&video_style, 150, 77, "M_SCROPT", NULL, 0, ""
};
static menuinfo_t hereticvideo_optmenu =
{
	vidoptions, sizeof(vidoptions) / sizeof(optmenuitem_t),
	&video_style, 150, 77, "H_SCROPT", NULL, 0, ""
};

// for advanced video options
// advancedoptions[], m_ADVANCED
static menuinfo_t advanced_optmenu =
{
	advancedoptions, sizeof(advancedoptions) / sizeof(optmenuitem_t),
	&advanced_style, 150, 77, "M_VIDEO", NULL, 0, "" //maybe I can replace the stupid hard coded graphic. . .
};

static menuinfo_t hereticadvanced_optmenu =
{
	advancedoptions, sizeof(advancedoptions) / sizeof(optmenuitem_t),
	&advanced_style, 150, 77, "H_VIDEO", NULL, 0, "" //maybe I can replace the stupid hard coded graphic. . .
};

// Debug Menu
static menuinfo_t debug_optmenu =
{
	debuggingoptions, sizeof(debuggingoptions) / sizeof(optmenuitem_t),
	&advanced_style, 150, 77, "M_SETUPM", NULL, 0, "" //TODO: supposed to be &debugmenu_style and M_DEBUG
};

//
//  SET RESOLUTION MENU
//
static optmenuitem_t resoptions[] =
{
	{OPT_Plain,    "",          NULL, 0, NULL, NULL, NULL},
	{OPT_Plain,    "",          NULL, 0, NULL, NULL, NULL},
	{OPT_Plain,    "",          NULL, 0, NULL, NULL, NULL},
	{OPT_Function, "New Size",  NULL, 0, NULL, M_ChangeResSize, NULL},
	{OPT_Function, "New Depth", NULL, 0, NULL, M_ChangeResDepth, NULL},
	{OPT_Function, "New Mode",  NULL, 0, NULL, M_ChangeResFull, NULL},
	{OPT_Plain,    "",          NULL, 0, NULL, NULL, NULL},
	{OPT_Function, "Set Resolution", NULL, 0, NULL, M_OptionSetResolution, NULL},
	/*	{OPT_Function, "Test Resolution", NULL, 0, NULL, M_OptionTestResolution, NULL}, */
		{OPT_Plain,    "",          NULL, 0, NULL, NULL, NULL},
		{OPT_Plain,    "",          NULL, 0, NULL, NULL, NULL},
		{OPT_Plain,    "",          NULL, 0, NULL, NULL, NULL}
};

//Custom GFX by Julian
static menuinfo_t res_optmenu =
{
	resoptions, sizeof(resoptions) / sizeof(optmenuitem_t),
	&setres_style, 150, 77, "M_SETRES", NULL, 3, ""
};
static menuinfo_t hereticres_optmenu =
{
	resoptions, sizeof(resoptions) / sizeof(optmenuitem_t),
	&setres_style, 150, 77, "H_SETRES", NULL, 3, ""
};

//
//  MOUSE OPTIONS
//
// -ACB- 1998/06/15 Added new mouse menu
// -KM- 1998/09/01 Changed to an analogue menu.  Must change those names
// -ACB- 1998/07/15 Altered menu structure
//
static optmenuitem_t analogueoptions[] =
{
	{OPT_Switch,   "Mouse X Axis",       Axis, 11, &mouse_xaxis, NULL, NULL},
	{OPT_Switch,   "Mouse Y Axis",       Axis, 11, &mouse_yaxis, NULL, NULL},
	{OPT_Slider,   "X Sensitivity",      NULL, 16, &mouse_xsens, NULL, NULL},
	{OPT_Slider,   "Y Sensitivity",      NULL, 16, &mouse_ysens, NULL, NULL},
	//	{OPT_Slider,   "Mouse Acceleration", NULL, 20,  &mouse_accel, NULL, NULL},
		{OPT_Boolean,  "Mouse Filtering",    YesNo, 0,  &mouse_filter, NULL, NULL},
		{OPT_Plain,    "",                   NULL, 0,  NULL, NULL, NULL},
		{OPT_Switch,   "Joystick Device", JoyDevs, 7,  &joystick_device, NULL, NULL},
		{OPT_Switch,   "First Axis",         Axis, 11, &joy_axis[0], NULL, NULL},
		{OPT_Switch,   "Second Axis",        Axis, 11, &joy_axis[1], NULL, NULL},
		{OPT_Switch,   "Third Axis",         Axis, 11, &joy_axis[2], NULL, NULL},
		{OPT_Switch,   "Fourth Axis",        Axis, 11, &joy_axis[3], NULL, NULL},
		{OPT_Switch,   "Fifth Axis",         Axis, 11, &joy_axis[4], NULL, NULL},
		{OPT_Switch,   "Sixth Axis",         Axis, 11, &joy_axis[5], NULL, NULL},

		{OPT_Plain,    "",                   NULL, 0,  NULL, NULL, NULL},
		{OPT_Slider,   "Turning Speed",      NULL, 12, &var_turnspeed,    NULL, NULL},
		{OPT_Slider,   "MLook Speed",        NULL, 12, &var_mlookspeed,   NULL, NULL},
		{OPT_Slider,   "Forward Move Speed", NULL, 8,  &var_forwardspeed, NULL, NULL},
		{OPT_Slider,   "Side Move Speed",    NULL, 8,  &var_sidespeed,    NULL, NULL},
		{OPT_Plain,    "",                   NULL, 0,  NULL, NULL, NULL},
		{OPT_Boolean,  "Debug Joystick Axis",    YesNo, 0,  &debug_joyaxis, NULL, NULL},
		{OPT_Boolean,  "Debug Mouse ",    YesNo, 0,  &debug_mouse, NULL, NULL}
};

static menuinfo_t analogue_optmenu =
{
	analogueoptions, sizeof(analogueoptions) / sizeof(optmenuitem_t),
	&mouse_style, 150, 75, "M_MSETTL", NULL, 0, ""
};
static menuinfo_t hereticanalogue_optmenu =
{
	analogueoptions, sizeof(analogueoptions) / sizeof(optmenuitem_t),
	&mouse_style, 150, 75, "H_MSETTL", NULL, 0, ""
};

//
//  SOUND OPTIONS
//
// -AJA- 2007/03/14 Added new sound menu
//
static optmenuitem_t soundoptions[] =
{
	{OPT_Slider,  "Sound Volume", NULL, SND_SLIDER_NUM, &sfx_volume, M_ChangeSfxVol, NULL},
	{OPT_Slider,  "Music Volume", NULL, SND_SLIDER_NUM, &mus_volume, M_ChangeMusVol, NULL},
	{OPT_Plain,   "",             NULL, 0,  NULL, NULL, NULL},
	//{OPT_Switch,  "Sample Rate",  SampleRates, 6, &var_sample_rate,  NULL, "NeedRestart"},
	//{OPT_Switch,  "Sample Size",  SoundBits, 3,   &var_sound_bits,   NULL, "NeedRestart"},
	{OPT_Switch,  "Stereo",       StereoNess, 3,  &var_sound_stereo, NULL, "NeedRestart"},
	{ OPT_Switch,  "Music Device",    MusicDevs, 3, &var_music_dev, NULL, "Win32: SYSTEM will not work with Vista or higher!" },

	{ OPT_Plain,   "",                NULL, 0,  NULL, NULL, NULL },
	{OPT_Boolean, "Sound Pitching",  SoundPitching, 2,  &sound_pitch, NULL, "Emulate Doom 1.2 Random SFX Pitching"},
	{OPT_Switch,  "Mix Channels",    MixChans,  4, &var_mix_channels, M_ChangeMixChan, NULL},
	{OPT_Switch,  "SFX: Quiet Factor",    QuietNess, 3, &var_quiet_factor, NULL, "How normalized do you want the sound?"},

	{OPT_Plain,   "",                NULL, 0,  NULL, NULL, NULL},

	{OPT_Boolean, "OPL Emulation Mode",       OPL,     2, &var_opl_opl3mode, NULL, "OPL1 or OPL3 mode emulation"},
	{ OPT_Switch, "Timidity Factor", QuietNess, 3, &var_timid_factor, M_ChangeTimidQuiet, NULL },
};

static menuinfo_t sound_optmenu =
{
	soundoptions, sizeof(soundoptions) / sizeof(optmenuitem_t),
	&mouse_style, 150, 75, "M_SFXOPT", NULL, 0, ""
};
static menuinfo_t hereticsound_optmenu =
{
	soundoptions, sizeof(soundoptions) / sizeof(optmenuitem_t),
	&mouse_style, 150, 75, "H_SFXOPT", NULL, 0, ""
};

//
//  GAMEPLAY OPTIONS
//
// -ACB- 1998/07/15 Altered menu structure
// -KM- 1998/07/21 Change blood to switch
//
static optmenuitem_t playoptions[] =
{
	{OPT_Boolean, "Mouse Look",         YesNo, 2,
	 &global_flags.mlook, M_ChangeMLook, NULL},

	{OPT_Switch,  "AutoAiming",         AAim, 3,
	 &global_flags.autoaim, M_ChangeAutoAim, NULL},

	{OPT_Boolean, "Jumping",            YesNo, 2,
	 &global_flags.jump, M_ChangeJumping, NULL},

	{OPT_Boolean, "Crouching",          YesNo, 2,
	 &global_flags.crouch, M_ChangeCrouching, NULL},

	{OPT_Boolean, "Weapon Kick",        YesNo, 2,
	 &global_flags.kicking, M_ChangeKicking, NULL},

	{OPT_Boolean, "Weapon Auto-Switch", YesNo, 2,
	 &global_flags.weapon_switch, M_ChangeWeaponSwitch, NULL},

	{OPT_Boolean, "Obituary Messages",  YesNo, 2,
	 &var_obituaries, NULL, NULL},

	{OPT_Boolean, "More Blood",         YesNo, 2,
	 &global_flags.more_blood, M_ChangeBlood, "Blood"},

	{OPT_Boolean, "Extras",             YesNo, 2,
	 &global_flags.have_extra, M_ChangeExtra, NULL},

	{OPT_Boolean, "True 3D Gameplay",   YesNo, 2,
	 &global_flags.true3dgameplay, M_ChangeTrue3d, "True3d"},

	{OPT_Boolean, "Shoot-Through Scenery",   YesNo, 2,
	 &global_flags.pass_missile, M_ChangePassMissile, NULL},

	{OPT_Plain,   "Monster Aggression", YesNo, 2,
	&g_aggression, NULL, "Aggressive Monsters (EDGE feature)!"},

	{OPT_Slider,  "Gravity",            NULL, 20,
	 &global_flags.menu_grav, NULL, "Gravity"},

	{OPT_Plain,   "", NULL, 0, NULL, NULL, NULL},

	{OPT_Boolean, "Enemy Respawn Mode", Respw, 2,
	 &global_flags.res_respawn, M_ChangeMonsterRespawn, NULL},

	{OPT_Boolean, "Item Respawn",       YesNo, 2,
	 &global_flags.itemrespawn, M_ChangeItemRespawn, NULL},

	{OPT_Boolean, "Fast Monsters",      YesNo, 2,
	 &global_flags.fastparm, M_ChangeFastparm, NULL},

	{OPT_Boolean, "Respawn",            YesNo, 2,
	 &global_flags.respawn, M_ChangeRespawn, NULL}
};

static menuinfo_t gameplay_optmenu =
{
	playoptions, sizeof(playoptions) / sizeof(optmenuitem_t),
	&gameplay_style, 160, 46, "M_GAMEPL", NULL, 0, ""
};
static menuinfo_t hereticgameplay_optmenu =
{
	playoptions, sizeof(playoptions) / sizeof(optmenuitem_t),
	&gameplay_style, 160, 46, "H_GAMEPL", NULL, 0, ""
};

//
//  KEY CONFIG : MOVEMENT
//
// -ACB- 1998/07/15 Altered menuinfo struct
// -KM- 1998/07/10 Used better names :-)
//
static optmenuitem_t move_keyconfig[] =
{
	{OPT_KeyConfig, "Walk Forward",   NULL, 0, &key_up, NULL, NULL},
	{OPT_KeyConfig, "Walk Backwards", NULL, 0, &key_down, NULL, NULL},
	{OPT_Plain,      "",              NULL, 0, NULL, NULL, NULL},
	{OPT_KeyConfig, "Strafe Left",    NULL, 0, &key_strafeleft, NULL, NULL},
	{OPT_KeyConfig, "Strafe Right",   NULL, 0, &key_straferight, NULL, NULL},
	{OPT_Plain,      "",              NULL, 0, NULL, NULL, NULL},
	{OPT_KeyConfig, "Turn Left",      NULL, 0, &key_left, NULL, NULL},
	{OPT_KeyConfig, "Turn Right",     NULL, 0, &key_right, NULL, NULL},
	{OPT_Plain,      "",              NULL, 0, NULL, NULL, NULL},
	{OPT_KeyConfig, "Up / Jump",      NULL, 0, &key_flyup, NULL, NULL},
	{OPT_KeyConfig, "Down / Crouch",  NULL, 0, &key_flydown, NULL, NULL},
};

static menuinfo_t movement_optmenu =
{
	move_keyconfig, sizeof(move_keyconfig) / sizeof(optmenuitem_t),
	&keyboard_style, 140, 98, "M_CONTRL", NULL, 0,
	"Movement"
};
static menuinfo_t hereticmovement_optmenu =
{
	move_keyconfig, sizeof(move_keyconfig) / sizeof(optmenuitem_t),
	&keyboard_style, 140, 98, "H_CONTRL", NULL, 0,
	"Movement"
};

//
//  KEY CONFIG : ATTACK + LOOK
//
// -ACB- 1998/07/15 Altered menuinfo struct
// -ES- 1999/03/28 Added Zoom Key
//
static optmenuitem_t attack_keyconfig[] =
{
	{OPT_KeyConfig, "Primary Attack",   NULL, 0, &key_fire, NULL, NULL},
	{OPT_KeyConfig, "Secondary Attack", NULL, 0, &key_secondatk, NULL, NULL},
	{OPT_KeyConfig, "Next Weapon",      NULL, 0, &key_nextweapon, NULL, NULL},
	{OPT_KeyConfig, "Previous Weapon",  NULL, 0, &key_prevweapon, NULL, NULL},
	{OPT_KeyConfig, "Weapon Reload",    NULL, 0, &key_reload, NULL, NULL},
	{OPT_Plain,     "",                 NULL, 0, NULL, NULL, NULL },
	{OPT_KeyConfig, "Use Item",         NULL, 0, &key_use, NULL, NULL },
	{OPT_Plain,     "",                 NULL, 0, NULL, NULL, NULL},
	{OPT_KeyConfig, "Look Up",          NULL, 0, &key_lookup, NULL, NULL},
	{OPT_KeyConfig, "Look Down",        NULL, 0, &key_lookdown, NULL, NULL},
	{OPT_KeyConfig, "Center View",      NULL, 0, &key_lookcenter, NULL, NULL},
	{OPT_KeyConfig, "Mouse Look",       NULL, 0, &key_mlook, NULL, NULL},
	{OPT_KeyConfig, "Zoom in/out",      NULL, 0, &key_zoom, NULL, NULL},
};

static menuinfo_t attack_optmenu =
{
	attack_keyconfig, sizeof(attack_keyconfig) / sizeof(optmenuitem_t),
	&keyboard_style, 140, 98, "M_CONTRL", NULL, 0,
	"Attack / Look"
};
static menuinfo_t hereticattack_optmenu =
{
	attack_keyconfig, sizeof(attack_keyconfig) / sizeof(optmenuitem_t),
	&keyboard_style, 140, 98, "H_CONTRL", NULL, 0,
	"Attack / Look"
};

//
//  KEY CONFIG : OTHER STUFF
//
static optmenuitem_t other_keyconfig[] =
{
	{OPT_Plain,     "",                 NULL, 0, NULL, NULL, NULL },
	{OPT_KeyConfig, "Strafe",           NULL, 0, &key_strafe, NULL, NULL},
	{OPT_KeyConfig, "Run",              NULL, 0, &key_speed, NULL, NULL},
	{OPT_KeyConfig, "Toggle Autorun",   NULL, 0, &key_autorun, NULL, NULL},
	{OPT_KeyConfig, "180 degree turn",  NULL, 0, &key_180, NULL, NULL},
	{OPT_Plain,     "",                 NULL, 0, NULL, NULL, NULL},
	{OPT_KeyConfig, "Map Toggle",       NULL, 0, &key_map, NULL, NULL},
	{OPT_KeyConfig, "Console",          NULL, 0, &key_console, NULL, NULL},
	{OPT_KeyConfig, "Pause",            NULL, 0, &key_pause, NULL, NULL},
	{OPT_KeyConfig, "Action 1",         NULL, 0, &key_action1, NULL, NULL},
	{OPT_KeyConfig, "Action 2",         NULL, 0, &key_action2, NULL, NULL},

	///	{OPT_KeyConfig, "Multiplayer Talk", NULL, 0, &key_talk, NULL, NULL},
};

static menuinfo_t otherkey_optmenu =
{
	other_keyconfig, sizeof(other_keyconfig) / sizeof(optmenuitem_t),
	&keyboard_style, 140, 98, "M_CONTRL", NULL, 0,
	"Other Keys"
};
static menuinfo_t hereticotherkey_optmenu =
{
	other_keyconfig, sizeof(other_keyconfig) / sizeof(optmenuitem_t),
	&keyboard_style, 140, 98, "H_CONTRL", NULL, 0,
	"Other Keys"
};

//
//  KEY CONFIG : WEAPONS
//
static optmenuitem_t weapon_keyconfig[] =
{
	{OPT_KeyConfig, "Weapon 1",  NULL, 0, &key_weapons[1], NULL, NULL},
	{OPT_KeyConfig, "Weapon 2",  NULL, 0, &key_weapons[2], NULL, NULL},
	{OPT_KeyConfig, "Weapon 3",  NULL, 0, &key_weapons[3], NULL, NULL},
	{OPT_KeyConfig, "Weapon 4",  NULL, 0, &key_weapons[4], NULL, NULL},
	{OPT_KeyConfig, "Weapon 5",  NULL, 0, &key_weapons[5], NULL, NULL},
	{OPT_Plain,     "",          NULL, 0, NULL, NULL, NULL},
	{OPT_KeyConfig, "Weapon 6",  NULL, 0, &key_weapons[6], NULL, NULL},
	{OPT_KeyConfig, "Weapon 7",  NULL, 0, &key_weapons[7], NULL, NULL},
	{OPT_KeyConfig, "Weapon 8",  NULL, 0, &key_weapons[8], NULL, NULL},
	{OPT_KeyConfig, "Weapon 9",  NULL, 0, &key_weapons[9], NULL, NULL},
	{OPT_KeyConfig, "Weapon 0",  NULL, 0, &key_weapons[0], NULL, NULL},
};

static menuinfo_t weapon_optmenu =
{
	weapon_keyconfig, sizeof(weapon_keyconfig) / sizeof(optmenuitem_t),
	&keyboard_style, 140, 98, "M_CONTRL", NULL, 0,
	"Weapon Keys"
};
static menuinfo_t hereticweapon_optmenu =
{
	weapon_keyconfig, sizeof(weapon_keyconfig) / sizeof(optmenuitem_t),
	&keyboard_style, 140, 98, "H_CONTRL", NULL, 0,
	"Weapon Keys"
};

//
//  KEY CONFIG : AUTOMAP
//
static optmenuitem_t automap_keyconfig[] =
{
	{OPT_KeyConfig, "Pan Up",        NULL, 0, &key_am_up, NULL, NULL},
	{OPT_KeyConfig, "Pan Down",      NULL, 0, &key_am_down, NULL, NULL},
	{OPT_KeyConfig, "Pan Left",      NULL, 0, &key_am_left, NULL, NULL},
	{OPT_KeyConfig, "Pan Right",     NULL, 0, &key_am_right, NULL, NULL},
	{OPT_Plain,     "",              NULL, 0, NULL, NULL, NULL},
	{OPT_KeyConfig, "Follow Mode",   NULL, 0, &key_am_follow, NULL, NULL},
	{OPT_KeyConfig, "Show Grid",     NULL, 0, &key_am_grid, NULL, NULL},
	{OPT_KeyConfig, "Zoom In",       NULL, 0, &key_am_zoomin, NULL, NULL},
	{OPT_KeyConfig, "Zoom Out",      NULL, 0, &key_am_zoomout, NULL, NULL},
	{OPT_KeyConfig, "Add Mark",      NULL, 0, &key_am_mark, NULL, NULL},
	{OPT_KeyConfig, "Clear Marks",   NULL, 0, &key_am_clear, NULL, NULL},
};

static menuinfo_t automap_optmenu =
{
	automap_keyconfig, sizeof(automap_keyconfig) / sizeof(optmenuitem_t),
	&keyboard_style, 140, 98, "M_CONTRL", NULL, 0,
	"Automap Keys"
};
static menuinfo_t hereticautomap_optmenu =
{
	automap_keyconfig, sizeof(automap_keyconfig) / sizeof(optmenuitem_t),
	&keyboard_style, 140, 98, "H_CONTRL", NULL, 0,
	"Automap Keys"
};

/*
 * ALL KEYBOARD MENUS
 */
#define NUM_KEY_MENUS  5

static menuinfo_t * all_key_menus[NUM_KEY_MENUS] =
{
	&movement_optmenu,
	&attack_optmenu,
	&otherkey_optmenu,
	&weapon_optmenu,
	&automap_optmenu
};
static menuinfo_t * heretic_key_menus[NUM_KEY_MENUS] =
{
	&hereticmovement_optmenu,
	&hereticattack_optmenu,
	&hereticotherkey_optmenu,
	&hereticweapon_optmenu,
	&hereticautomap_optmenu
};

static char keystring1[] = "Enter to change, Backspace to Clear";
static char keystring2[] = "Press a key for this action";

//
// M_OptCheckNetgame
//
// Sets the first option to be "Leave Game" or "Multiplayer Game"
// depending on whether we are playing a game or not.
//
void M_OptCheckNetgame(void)
{
	if (gamestate >= GS_LEVEL)
	{
		strcpy(mainoptions[HOSTNET_POS + 0].name, "Leave Game");
		mainoptions[HOSTNET_POS + 0].routine = &M_EndGame;
		mainoptions[HOSTNET_POS + 0].help = NULL;

		//		strcpy(mainoptions[HOSTNET_POS+1].name, "");
		//		mainoptions[HOSTNET_POS+1].type = OPT_Plain;
		//		mainoptions[HOSTNET_POS+1].routine = NULL;
		//		mainoptions[HOSTNET_POS+1].help = NULL;
	}
	else
	{
		strcpy(mainoptions[HOSTNET_POS + 0].name, "Host Net Game");
		mainoptions[HOSTNET_POS + 0].routine = &M_HostNetGame;
		mainoptions[HOSTNET_POS + 0].help = NULL;

		strcpy(mainoptions[HOSTNET_POS + 1].name, "Join Net Game");
		mainoptions[HOSTNET_POS + 1].type = OPT_Function;
		mainoptions[HOSTNET_POS + 1].routine = &M_JoinNetGame;
		mainoptions[HOSTNET_POS + 1].help = NULL;
	}
}

//
// Menu Initialisation
//
void M_OptMenuInit()
{
	option_menuon = 0;

	if (heretic_mode)
	{
		curr_menu = &hereticmain_optmenu;
	}
	else if (!heretic_mode)
	{
		curr_menu = &main_optmenu;
	}

	curr_item = curr_menu->items + curr_menu->pos;
	curr_key_menu = 0;
	keyscan = 0;

	// load styles
	styledef_c *def;

	def = styledefs.Lookup("OPTIONS");
	/* 	if (! def) def = default_style; */
	opt_def_style = hu_styles.Lookup(def);

	def = styledefs.Lookup("KEYBOARD CONTROLS");
	keyboard_style = def ? hu_styles.Lookup(def) : opt_def_style;

	def = styledefs.Lookup("MOUSE CONTROLS");
	mouse_style = def ? hu_styles.Lookup(def) : opt_def_style;

	def = styledefs.Lookup("GAMEPLAY OPTIONS");
	gameplay_style = def ? hu_styles.Lookup(def) : opt_def_style;

	def = styledefs.Lookup("VIDEO OPTIONS");
	video_style = def ? hu_styles.Lookup(def) : opt_def_style;

	def = styledefs.Lookup("SET RESOLUTION");
	setres_style = def ? hu_styles.Lookup(def) : opt_def_style;

	def = styledefs.Lookup("ADVANCED");
	advanced_style = def ? hu_styles.Lookup(def) : opt_def_style;

	// Restore the config setting.
	M_ChangeBlood(-1);
}

//
// M_OptTicker
//
// Text menu ticker
//
void M_OptTicker(void)
{
	///--  	if (testticker > 0)
	///--  	{
	///--  		testticker--;
	///--  	}
	///--  	else if (!testticker)
	///--  	{
	///--  		testticker--;
	///--  		M_RestoreResSettings(-1);
	///--  	}
}

//
// M_OptDrawer
//
// Text menu drawer
//

void M_OptDrawer()
{
	char tempstring[80];
	int curry, deltay, menutop;
	int i, j;
	unsigned int k;

	HUD_SetAlpha(0.64f);
	HUD_SolidBox(0, 0, 320, 200, T_BLACK);
	HUD_SetAlpha();

	style_c *style = curr_menu->style_var[0];
	SYS_ASSERT(style);

	if (!style->fonts[0])
		return;

	int font_h = style->fonts[0]->NominalHeight();

	// -ACB- 1998/06/15 Calculate height for menu and then center it.
	menutop = 68 - ((curr_menu->item_num * font_h) / 2);

	if (curr_menu->key_page[0])
		menutop = 9 * font_h / 2;

	const image_c *image;

	if (!curr_menu->title_image)
		curr_menu->title_image = W_ImageLookup(curr_menu->title_name);

	image = curr_menu->title_image;

	HUD_DrawImage(curr_menu->title_x, menutop, image);

	//now, draw all the menuitems
	deltay = 1 + font_h;

	curry = menutop + 25;

	if (curr_menu->key_page[0])
	{
		if (curr_key_menu > 0)
			HL_WriteText(style, 2, 60, 200 - deltay * 4, "< PREV");

		if (curr_key_menu < NUM_KEY_MENUS - 1)
			HL_WriteText(style, 2, 260 - style->fonts[2]->StringWidth("NEXT >"), 200 - deltay * 4,
				"NEXT >");

		HL_WriteText(style, 3, 160 - style->fonts[2]->StringWidth(curr_menu->key_page) / 2,
			curry, curr_menu->key_page);
		curry += font_h * 2;

		if (keyscan)
			HL_WriteText(style, 3, 160 - (style->fonts[3]->StringWidth(keystring2) / 2),
				200 - deltay * 2, keystring2);
		else
			HL_WriteText(style, 3, 160 - (style->fonts[3]->StringWidth(keystring1) / 2),
				200 - deltay * 2, keystring1);
	}
	else if (curr_menu == &res_optmenu)
	{
		M_ResOptDrawer(style, curry, curry + (deltay * (res_optmenu.item_num - 2)),
			deltay, curr_menu->menu_center);
	}
	else if (curr_menu == &main_optmenu)
	{
		M_LanguageDrawer(curr_menu->menu_center, curry, deltay);
	}
	else if (curr_menu == &hereticmain_optmenu)
	{
		M_LanguageDrawer(curr_menu->menu_center, curry, deltay);
	}

	for (i = 0; i < curr_menu->item_num; i++)
	{
		bool is_selected = (i == curr_menu->pos);

		HL_WriteText(style, is_selected ? 2 : 0,
			(curr_menu->menu_center) - style->fonts[0]->StringWidth(curr_menu->items[i].name),
			curry, curr_menu->items[i].name);

		// -ACB- 1998/07/15 Menu Cursor is colour indexed.
		if (is_selected)
		{
			HL_WriteText(style, 2, (curr_menu->menu_center + 4), curry, ">");

			if (curr_menu->items[i].help)
			{
				const char *help = language[curr_menu->items[i].help];

				HL_WriteText(style, 3, 160 - (style->fonts[3]->StringWidth(help) / 2), 200 - deltay * 2,
					help);
			}
		}

		switch (curr_menu->items[i].type)
		{
		case OPT_Boolean:
		case OPT_Switch:
		{
			k = 0;
			for (j = 0; j < M_GetCurrentSwitchValue(&curr_menu->items[i]); j++)
			{
				while ((curr_menu->items[i].typenames[k] != '/') && (k < strlen(curr_menu->items[i].typenames)))
					k++;

				k++;
			}

			if (k < strlen(curr_menu->items[i].typenames))
			{
				j = 0;
				while ((curr_menu->items[i].typenames[k] != '/') && (k < strlen(curr_menu->items[i].typenames)))
				{
					tempstring[j] = curr_menu->items[i].typenames[k];
					j++;
					k++;
				}
				tempstring[j] = 0;
			}
			else
			{
				sprintf(tempstring, "Invalid");
			}

			HL_WriteText(style, 1, (curr_menu->menu_center) + 15, curry, tempstring);
			break;
		}

		case OPT_Slider:
		{
			M_DrawThermo(curr_menu->menu_center + 15, curry,
				curr_menu->items[i].numtypes,
				*(int*)(curr_menu->items[i].switchvar), 2);

			break;
		}

		case OPT_KeyConfig:
		{
			k = *(int*)(curr_menu->items[i].switchvar);
			M_Key2String(k, tempstring);
			HL_WriteText(style, 1, (curr_menu->menu_center + 15), curry, tempstring);
			break;
		}

		default:
			break;
		}
		curry += deltay;
	}
}

//
// M_OptResDrawer
//
// Something of a hack, but necessary to give a better way of changing
// resolution
//
// -ACB- 1999/10/03 Written
//
static void M_ResOptDrawer(style_c *style, int topy, int bottomy, int dy, int centrex)
{
	char tempstring[80];

	// Draw current resolution
	int y = topy;

	y += dy;

	// Draw resolution selection option
	y += (dy * 2);
	sprintf(tempstring, "%dx%d", new_scrmode.width, new_scrmode.height);
	HL_WriteText(style, 1, centrex + 15, y, tempstring);

	// Draw depth selection option
	y += dy;
	sprintf(tempstring, "%d bit", (new_scrmode.depth < 20) ? 16 : 32);
	HL_WriteText(style, 1, centrex + 15, y, tempstring);

	y += dy;
	sprintf(tempstring, "%s", new_scrmode.full ? "Fullscreen" : "Windowed");
	HL_WriteText(style, 1, centrex + 15, y, tempstring);

	// Draw selected resolution and mode:
	y = bottomy;
	y += (dy / 2);

	sprintf(tempstring, "Current Resolution:");
	HL_WriteText(style, 3, 160 - (style->fonts[0]->StringWidth(tempstring) / 2), y, tempstring);

	y += dy;

	sprintf(tempstring, "%d x %d at %d-bit %s",
		SCREENWIDTH, SCREENHEIGHT, (SCREENBITS < 20) ? 16 : 32,
		FULLSCREEN ? "Fullscreen" : "Windowed");

	HL_WriteText(style, 1, 160 - (style->fonts[1]->StringWidth(tempstring) / 2), y, tempstring);
}

//
// M_LanguageDrawer
//
// Yet another hack (this stuff badly needs rewriting) to draw the
// current language name.
//
// -AJA- 2000/04/16 Written

static void M_LanguageDrawer(int x, int y, int deltay)
{
	HL_WriteText(opt_def_style, 1, x + 15, y + deltay * LANGUAGE_POS, language.GetName());
}

static void KeyMenu_Next()
{
	if (curr_key_menu >= NUM_KEY_MENUS - 1)
		return;

	curr_key_menu++;

	curr_menu = all_key_menus[curr_key_menu];

	S_StartFX(sfx_pstop);
}

static void KeyMenu_Prev()
{
	if (curr_key_menu <= 0)
		return;

	curr_key_menu--;

	curr_menu = all_key_menus[curr_key_menu];

	S_StartFX(sfx_pstop);
}

//
// M_OptResponder
//
bool M_OptResponder(event_t * ev, int ch)
{
	///--  	if (testticker != -1)
	///--  		return true;

		// Scan for keycodes
	if (keyscan)
	{
		int *blah;

		if (ev->type != ev_keydown)
			return false;
		int key = ev->data1;

		keyscan = 0;

		if (ch == KEYD_ESCAPE)
			return true;

		blah = (int*)(curr_item->switchvar);
		if (((*blah) >> 16) == key)
		{
			(*blah) &= 0xffff;
			return true;
		}
		if (((*blah) & 0xffff) == key)
		{
			(*blah) >>= 16;
			return true;
		}

		if (((*blah) & 0xffff) == 0)
			*blah = key;
		else if (((*blah) >> 16) == 0)
			*blah |= key << 16;
		else
		{
			*blah >>= 16;
			*blah |= key << 16;
		}
		return true;
	}

	switch (ch)
	{
	case KEYD_BACKSPACE:
	case KEYD_DELETE:
	{
		if (curr_item->type == OPT_KeyConfig)
			*(int*)(curr_item->switchvar) = 0;
		return true;
	}

	case KEYD_DOWNARROW:
	case KEYD_WHEEL_DN:
	{
		do
		{
			curr_menu->pos++;
			if (curr_menu->pos >= curr_menu->item_num)
				curr_menu->pos = 0;
			curr_item = curr_menu->items + curr_menu->pos;
		} while (curr_item->type == 0);

		S_StartFX(sfx_pstop);
		return true;
	}

	case KEYD_UPARROW:
	case KEYD_WHEEL_UP:
	{
		do
		{
			curr_menu->pos--;
			if (curr_menu->pos < 0)
				curr_menu->pos = curr_menu->item_num - 1;
			curr_item = curr_menu->items + curr_menu->pos;
		} while (curr_item->type == 0);

		S_StartFX(sfx_pstop);
		return true;
	}

	case KEYD_LEFTARROW:
	{
		if (curr_menu->key_page[0])
		{
			KeyMenu_Prev();
			return true;
		}

		switch (curr_item->type)
		{
		case OPT_Plain:
		{
			return false;
		}

		case OPT_Boolean:
		{
			bool *boolptr = (bool*)curr_item->switchvar;

			*boolptr = !(*boolptr);

			S_StartFX(sfx_pistol);

			if (curr_item->routine != NULL)
				curr_item->routine(ch);

			return true;
		}

		case OPT_Switch:
		{
			int *val_ptr = (int*)curr_item->switchvar;

			*val_ptr -= 1;

			if (*val_ptr < 0)
				*val_ptr = curr_item->numtypes - 1;

			S_StartFX(sfx_pistol);

			if (curr_item->routine != NULL)
				curr_item->routine(ch);

			return true;
		}

		case OPT_Function:
		{
			if (curr_item->routine != NULL)
				curr_item->routine(ch);

			S_StartFX(sfx_pistol);
			return true;
		}

		case OPT_Slider:
		{
			int *val_ptr = (int*)curr_item->switchvar;

			if (*val_ptr > 0)
			{
				*val_ptr -= 1;

				S_StartFX(sfx_stnmov);
			}

			if (curr_item->routine != NULL)
				curr_item->routine(ch);

			return true;
		}

		default:
			break;
		}
	}

	case KEYD_RIGHTARROW:
		if (curr_menu->key_page[0])
		{
			KeyMenu_Next();
			return true;
		}

		/* FALL THROUGH... */

	case KEYD_ENTER:
	case KEYD_MOUSE1:
	{
		switch (curr_item->type)
		{
		case OPT_Plain:
			return false;

		case OPT_Boolean:
		{
			bool *boolptr = (bool*)curr_item->switchvar;

			*boolptr = !(*boolptr);

			S_StartFX(sfx_pistol);

			if (curr_item->routine != NULL)
				curr_item->routine(ch);

			return true;
		}

		case OPT_Switch:
		{
			int *val_ptr = (int*)curr_item->switchvar;

			*val_ptr += 1;

			if (*val_ptr >= curr_item->numtypes)
				*val_ptr = 0;

			S_StartFX(sfx_pistol);

			if (curr_item->routine != NULL)
				curr_item->routine(ch);

			return true;
		}

		case OPT_Function:
		{
			if (curr_item->routine != NULL)
				curr_item->routine(ch);

			S_StartFX(sfx_pistol);
			return true;
		}

		case OPT_Slider:
		{
			int *val_ptr = (int*)curr_item->switchvar;

			if (*val_ptr < (curr_item->numtypes - 1))
			{
				*val_ptr += 1;

				S_StartFX(sfx_stnmov);
			}

			if (curr_item->routine != NULL)
				curr_item->routine(ch);

			return true;
		}

		case OPT_KeyConfig:
		{
			keyscan = 1;
			return true;
		}

		default:
			break;
		}
		I_Error("Invalid menu type!");
	}
	case KEYD_ESCAPE:
	case KEYD_MOUSE2:
	case KEYD_MOUSE3:
	{
		if (heretic_mode)
		{
			if (curr_menu == &hereticmain_optmenu)
			{
				option_menuon = 0;
			}
			else
			{
				curr_menu = &hereticmain_optmenu;
				curr_item = curr_menu->items + curr_menu->pos;
			}
			S_StartFX(sfx_swtchx);
			return true;
		}
		else if (!heretic_mode)
		{
			if (curr_menu == &main_optmenu)
			{
				option_menuon = 0;
			}
			else
			{
				curr_menu = &main_optmenu;
				curr_item = curr_menu->items + curr_menu->pos;
			}
			S_StartFX(sfx_swtchx);
			return true;
		}
	}
	}
	return false;
}

// ===== SUB-MENU SETUP ROUTINES =====

//
// M_VideoOptions
//
static void M_VideoOptions(int keypressed)
{
	if (heretic_mode)
	{
		curr_menu = &hereticvideo_optmenu;
	}
	else if (!heretic_mode)
	{
		curr_menu = &video_optmenu;
	}

	curr_item = curr_menu->items + curr_menu->pos;
}

static void M_AdvancedOptions(int keypressed)
{
	if (heretic_mode)
	{
		curr_menu = &hereticadvanced_optmenu;
	}
	else if (!heretic_mode)
	{
		curr_menu = &advanced_optmenu;
	}
	curr_item = curr_menu->items + curr_menu->pos;
}

static void M_DebugMenu(int keypressed)
{
	if (heretic_mode)
	{
		curr_menu = &debug_optmenu;
	}
	else if (!heretic_mode)
	{
		curr_menu = &debug_optmenu;
	}

	curr_item = curr_menu->items + curr_menu->pos;
}

//
// M_ResolutionOptions
//
// This menu is different in the fact that is most be calculated at runtime,
// this is because different resolutions are available on different machines.
//
// -ES- 1998/08/20 Added
// -ACB 1999/10/03 rewrote to Use scrmodes array.
//
static void M_ResolutionOptions(int keypressed)
{
	new_scrmode.width = SCREENWIDTH;
	new_scrmode.height = SCREENHEIGHT;
	new_scrmode.depth = SCREENBITS;
	new_scrmode.full = FULLSCREEN;

	if (heretic_mode)
	{
		curr_menu = &hereticres_optmenu;
	}
	else if (!heretic_mode)
	{
		curr_menu = &res_optmenu;
	}
	curr_item = curr_menu->items + curr_menu->pos;
}

//
// M_AnalogueOptions
//
static void M_AnalogueOptions(int keypressed)
{
	if (heretic_mode)
	{
		curr_menu = &hereticanalogue_optmenu;
	}
	else if (!heretic_mode)
	{
		curr_menu = &analogue_optmenu;
	}
	curr_item = curr_menu->items + curr_menu->pos;
}

//
// M_SoundOptions
//
static void M_SoundOptions(int keypressed)
{
	if (heretic_mode)
	{
		curr_menu = &hereticsound_optmenu;
	}
	else if (!heretic_mode)
	{
		curr_menu = &sound_optmenu;
	}

	curr_item = curr_menu->items + curr_menu->pos;
}

//
// M_GameplayOptions
//
static void M_GameplayOptions(int keypressed)
{
	// not allowed in netgames (changing most of these options would
	// break synchronisation with the other machines).
	if (netgame)
		return;

	if (heretic_mode)
	{
		curr_menu = &hereticgameplay_optmenu;
	}
	else if (!heretic_mode)
	{
		curr_menu = &gameplay_optmenu;
	}

	curr_item = curr_menu->items + curr_menu->pos;
}

//
// M_KeyboardOptions
//
static void M_KeyboardOptions(int keypressed)
{
	if (heretic_mode)
	{
		curr_menu = heretic_key_menus[curr_key_menu];
	}
	else if (!heretic_mode)
	{
		curr_menu = all_key_menus[curr_key_menu];
	}

	curr_item = curr_menu->items + curr_menu->pos;
}

// ===== END OF SUB-MENUS =====

//
// M_Key2String
//
static void M_Key2String(int key, char *deststring)
{
	int key1 = key & 0xffff;
	int key2 = key >> 16;

	if (key1 == 0)
	{
		strcpy(deststring, "---");
		return;
	}

	strcpy(deststring, E_GetKeyName(key1));

	if (key2 != 0)
	{
		strcat(deststring, " or ");
		strcat(deststring, E_GetKeyName(key2));
	}
}

//
// M_ChangeGamma
//
// -AJA- 1999/07/03: stuck this here & removed PLAYPAL reference.
//
static void M_ChangeGamma(int keypressed)
{
	/* nothing to do */
}

static void M_ChangeMonitorSize(int key)
{
	static const float aspect_ratios[5] =
	{
		1.333, 1.777, 1.6, 1.5, 2.4
	};

	monitor_size = CLAMP(0, monitor_size, 4);

	r_aspect = aspect_ratios[monitor_size];
}

//
// M_ChangeBlood
//
// -KM- 1998/07/21 Change blood to a bool
// -ACB- 1998/08/09 Check map setting allows this
//
static void M_ChangeBlood(int keypressed)
{
	if (currmap && ((currmap->force_on | currmap->force_off) & MPF_MoreBlood))
		return;

	level_flags.more_blood = global_flags.more_blood;
}

static void M_ChangeMLook(int keypressed)
{
	if (currmap && ((currmap->force_on | currmap->force_off) & MPF_Mlook))
		return;

	level_flags.mlook = global_flags.mlook;
}

static void M_ChangeJumping(int keypressed)
{
	if (currmap && ((currmap->force_on | currmap->force_off) & MPF_Jumping))
		return;

	level_flags.jump = global_flags.jump;
}

static void M_ChangeCrouching(int keypressed)
{
	if (currmap && ((currmap->force_on | currmap->force_off) & MPF_Crouching))
		return;

	level_flags.crouch = global_flags.crouch;
}

static void M_ChangeExtra(int keypressed)
{
	if (currmap && ((currmap->force_on | currmap->force_off) & MPF_Extras))
		return;

	level_flags.have_extra = global_flags.have_extra;
}

//
// M_ChangeMonsterRespawn
//
// -ACB- 1998/08/09 New DDF settings, check that map allows the settings
//
static void M_ChangeMonsterRespawn(int keypressed)
{
	if (currmap && ((currmap->force_on | currmap->force_off) & MPF_ResRespawn))
		return;

	level_flags.res_respawn = global_flags.res_respawn;
}

static void M_ChangeItemRespawn(int keypressed)
{
	if (currmap && ((currmap->force_on | currmap->force_off) & MPF_ItemRespawn))
		return;

	level_flags.itemrespawn = global_flags.itemrespawn;
}

static void M_ChangeTrue3d(int keypressed)
{
	if (currmap && ((currmap->force_on | currmap->force_off) & MPF_True3D))
		return;

	level_flags.true3dgameplay = global_flags.true3dgameplay;
}

static void M_ChangeAutoAim(int keypressed)
{
	if (currmap && ((currmap->force_on | currmap->force_off) & MPF_AutoAim))
		return;

	level_flags.autoaim = global_flags.autoaim;
}

static void M_ChangeRespawn(int keypressed)
{
	if (gameskill == sk_nightmare)
		return;

	if (currmap && ((currmap->force_on | currmap->force_off) & MPF_Respawn))
		return;

	level_flags.respawn = global_flags.respawn;
}

static void M_ChangeFastparm(int keypressed)
{
	if (gameskill == sk_nightmare)
		return;

	if (currmap && ((currmap->force_on | currmap->force_off) & MPF_FastParm))
		return;

	level_flags.fastparm = global_flags.fastparm;
}

static void M_ChangePassMissile(int keypressed)
{
	level_flags.pass_missile = global_flags.pass_missile;
}

// this used by both MIPMIP, SMOOTHING and DETAIL options
static void M_ChangeMipMap(int keypressed)
{
	W_DeleteAllImages();
}

static void M_ChangeShadows(int keypressed)
{
	if (currmap && ((currmap->force_on | currmap->force_off) & MPF_Shadows))
		return;

	level_flags.shadows = global_flags.shadows;
}

#if 0
static void M_ChangeHalos(int keypressed)
{
	if (currmap && ((currmap->force_on | currmap->force_off) & MPF_Halos))
		return;

	level_flags.halos = global_flags.halos;
}
#endif

static void M_ChangeKicking(int keypressed)
{
	if (currmap && ((currmap->force_on | currmap->force_off) & MPF_Kicking))
		return;

	level_flags.kicking = global_flags.kicking;
}

static void M_ChangeWeaponSwitch(int keypressed)
{
	if (currmap && ((currmap->force_on | currmap->force_off) & MPF_WeaponSwitch))
		return;

	level_flags.weapon_switch = global_flags.weapon_switch;
}

static void M_ChangeDLights(int keypressed)
{
	/* nothing to do */
}

static void M_ChangeCrossHair(int keypressed)
{
	r_crosshair = menu_crosshair;
}

static void M_ChangeCrossHairSize(int keypressed)
{
	r_crosssize = menu_crosshair2;
}

#if 0
// Trying new user-defined scaling code for Heads-Up Display Text.
extern void HUD_SetScale(float scale);
static void M_ChangeHUDTextScale(int keypressed)
{
	float scale;

	{
		scale = CLAMP(0, r_textscale.f, 2); //from 0.1f -> 2.0f
		HUD_SetScale(scale);
	}

	HUD_SetScale();
}
#endif // 0

//
// M_ChangeLanguage
//
// -AJA- 2000/04/16 Run-time language changing...
//
static void M_ChangeLanguage(int keypressed)
{
	if (keypressed == KEYD_LEFTARROW)
	{
		int idx, max;

		idx = language.GetChoice();
		max = language.GetChoiceCount();

		idx--;
		if (idx < 0) { idx += max; }

		language.Select(idx);
	}
	else if (keypressed == KEYD_RIGHTARROW)
	{
		int idx, max;

		idx = language.GetChoice();
		max = language.GetChoiceCount();

		idx++;
		if (idx >= max) { idx = 0; }

		language.Select(idx);
	}

	// update cvar
	m_language = language.GetName();
}

//
// M_ChangeResSize
//
// -ACB- 1998/08/29 Resolution Changes...
//
static void M_ChangeResSize(int keypressed)
{
	if (keypressed == KEYD_LEFTARROW)
	{
		R_IncrementResolution(&new_scrmode, RESINC_Size, -1);
	}
	else if (keypressed == KEYD_RIGHTARROW)
	{
		R_IncrementResolution(&new_scrmode, RESINC_Size, +1);
	}
}

//
// M_ChangeResDepth
//
// -ACB- 1998/08/29 Depth Changes...
//
static void M_ChangeResDepth(int keypressed)
{
	if (keypressed == KEYD_LEFTARROW)
	{
		R_IncrementResolution(&new_scrmode, RESINC_Depth, -1);
	}
	else if (keypressed == KEYD_RIGHTARROW)
	{
		R_IncrementResolution(&new_scrmode, RESINC_Depth, +1);
	}
}

//
// M_ChangeResFull
//
// -AJA- 2005/01/02: Windowed vs Fullscreen
//
static void M_ChangeResFull(int keypressed)
{
	if (keypressed == KEYD_LEFTARROW)
	{
		R_IncrementResolution(&new_scrmode, RESINC_Full, +1);
	}
	else if (keypressed == KEYD_RIGHTARROW)
	{
		R_IncrementResolution(&new_scrmode, RESINC_Full, +1);
	}
}

//
// M_OptionSetResolution
//
static void M_OptionSetResolution(int keypressed)
{
	if (R_ChangeResolution(&new_scrmode))
	{
		R_SoftInitResolution();
	}
	else
	{
		std::string msg(epi::STR_Format(language["ModeSelErr"],
			new_scrmode.width, new_scrmode.height,
			(new_scrmode.depth < 20) ? 16 : 32));

		M_StartMessage(msg.c_str(), NULL, false);

		///--  		testticker = -1;

		///??	selectedscrmode = prevscrmode;
	}
}

///--  //
///--  // M_OptionTestResolution
///--  //
///--  static void M_OptionTestResolution(int keypressed)
///--  {
///--      R_ChangeResolution(selectedscrmode);
///--  	testticker = TICRATE * 3;
///--  }
///--
///--  //
///--  // M_RestoreResSettings
///--  //
///--  static void M_RestoreResSettings(int keypressed)
///--  {
///--      R_ChangeResolution(prevscrmode);
///--  }

extern void M_NetHostBegun(void);
extern void M_NetJoinBegun(void);

void M_HostNetGame(int keypressed)
{
	option_menuon = 0;
	netgame_menuon = 1;

	M_NetHostBegun();
}

void M_JoinNetGame(int keypressed)
{
	//#if 0
	option_menuon = 0;
	netgame_menuon = 2;

	M_NetJoinBegun();
	//#endif
}

void M_Options(int choice)
{
	option_menuon = 1;

	// hack
	menu_crosshair = CLAMP(0, r_crosshair.d, 9);

	menu_crosshair2 = CLAMP(0, r_crosssize.f, 15);

	// continued
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab