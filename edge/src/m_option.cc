//----------------------------------------------------------------------------
//  EDGE Option Menu Modification
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
//

#include "i_defs.h"
#include "m_option.h"

#include "dm_state.h"

#include "ddf_main.h"
#include "e_input.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "hu_style.h"
#include "m_menu.h"
#include "m_misc.h"
#include "m_netgame.h"
#include "n_network.h"
#include "p_local.h"
#include "r_main.h"
#include "r_vbinit.h"
#include "r2_defs.h"
#include "rgl_defs.h"
#include "s_sound.h"
#include "s_music.h"
#include "v_ctx.h"
#include "v_res.h"
#include "v_colour.h"
#include "r_image.h"
#include "w_wad.h"
#include "wp_main.h"

#include "defaults.h"

#include "epi/strings.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

int option_menuon = 0;

//submenus
static void M_KeyboardOptions(int keypressed);
static void M_VideoOptions(int keypressed);
static void M_GameplayOptions(int keypressed);
static void M_AnalogueOptions(int keypressed);
static void M_SoundOptions(int keypressed);
// static void M_CalibrateJoystick(int keypressed);

void M_ResetToDefaults(int keypressed);

static void M_Key2String(int key, char *deststring);

// -ACB- 1998/08/09 "Does Map allow these changes?" procedures.
static void M_ChangeMonsterRespawn(int keypressed);
static void M_ChangeItemRespawn(int keypressed);
static void M_ChangeTrue3d(int keypressed);
static void M_ChangeAutoAim(int keypressed);
static void M_ChangeFastparm(int keypressed);
static void M_ChangeRespawn(int keypressed);
static void M_ChangePassMissile(int keypressed);

//Special function declarations
int menunormalfov, menuzoomedfov;

static void M_ChangeBlood(int keypressed);
static void M_ChangeMLook(int keypressed);
static void M_ChangeJumping(int keypressed);
static void M_ChangeCrouching(int keypressed);
static void M_ChangeExtra(int keypressed);
static void M_ChangeGamma(int keypressed);
static void M_ChangeShadows(int keypressed);
static void M_ChangeKicking(int keypressed);
static void M_ChangeWeaponSwitch(int keypressed);
static void M_ChangeMipMap(int keypressed);
static void M_ChangeDLights(int keypressed);
// static void M_ChangeHalos(int keypressed);

// -ES- 1998/08/20 Added resolution options
// -ACB- 1998/08/29 Moved to top and tried different system

static void M_ResOptDrawer(style_c *style, int topy, int bottomy, int dy, int centrex);
static void M_ResolutionOptions(int keypressed);
static void M_OptionSetResolution(int keypressed);
static void M_OptionTestResolution(int keypressed);
static void M_RestoreResSettings(int keypressed);
static void M_ChangeStoredRes(int keypressed);
static void M_ChangeStoredBpp(int keypressed);
static void M_ChangeStoredMode(int keypressed);
static void M_HostNetGame(int keypressed);
static void M_JoinNetGame(int keypressed);

static void M_LanguageDrawer(int x, int y, int deltay);
static void M_ChangeLanguage(int keypressed);

static char YesNo[]     = "Off/On";  // basic on/off
static char CrosO[]     = "None/Cross/Dot/Angle";  // crosshair options
static char Respw[]     = "Teleport/Resurrect";  // monster respawning
static char Axis[]      = "Turn/Forward/Strafe/MLook/Fly/Disable";
static char SkySq[]     = "Small/Medium/Large/Mirror/Original";
static char DLMode[]    = "Off/On";
static char JpgPng[]    = "JPEG/PNG";  // basic on/off
static char AAim[]      = "Off/On/Mlook";
static char Huds[]      = "Full/None/Overlay";
static char MipMaps[]   = "None/Good/Best";
static char Details[]   = "Low/Medium/High";

// for CVar enums
const char WIPE_EnumStr[] = "none/melt/crossfade/pixelfade/top/bottom/left/right/spooky/doors";

static char SampleRates[] = "11025 Hz/16000 Hz/22050 Hz/32000 Hz/44100 Hz";
static char SoundBits[]   = "8 bit/16 bit";
static char StereoNess[]  = "Off/On/Swapped";
static char MixChans[]    = "8/16/32/64/96";
static char QuietNess[]   = "Loud (distorted)/Normal/Soft";

// Screen resolution changes
static int prevscrmode;
static int selectedscrmode;
static int testticker = -1;

// -ES- 1998/11/28 Wipe and Faded teleportation options
//static char FadeT[] = "Off/On, flash/On, no flash";


//
//  OPTION STRUCTURES
//

typedef enum
{
	OPT_Plain     = 0,  // 0 means plain text,
	OPT_Switch    = 1,  // 1 is change a switch,
	OPT_Function  = 2,  // 2 is call a function,
	OPT_Slider    = 3,  // 3 is a slider,
	OPT_KeyConfig = 4,  // 4 is a key config,
	OPT_Boolean   = 5,  // 5 is change a boolean switch
	OPT_NumTypes
}
opt_type_e;

typedef struct optmenuitem_s
{
	opt_type_e type;

	char name[48];
	const char *typenames;
  
	int numtypes;
	int default_val;
	void *switchvar;
  
	void (*routine)(int keypressed);

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
	const image_t *title_image;

	// current position
	int pos;

	// key config, with left and right sister menus ?
	char key_page[20];

	struct menuinfo_s *sister_prev;
	struct menuinfo_s *sister_next;
}
menuinfo_t;

// current menu and position
static menuinfo_t *curr_menu;
static optmenuitem_t *curr_item;
static int keyscan;

static style_c *opt_def_style;
static style_c *keyboard_style;
static style_c *mouse_style;
static style_c *gameplay_style;
static style_c *video_style;
static style_c *setres_style;


typedef struct specialkey_s
{
	int keycode;
	char keystring[20];
}
specialkey_t;

static void M_ChangeScreenHud(int keypressed)
{
	R_SetViewSize(screen_hud);
}


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

#if 0
//
// M_ChangeNormalFOV
//
static void M_ChangeNormalFOV(int keypressed)
{
	R_SetNormalFOV((ANG45 / 9) * (menunormalfov + 1));
}

static void M_ChangeZoomedFOV(int keypressed)
{
	R_SetZoomedFOV((ANG45 / 9) * (menuzoomedfov + 1));
}
#endif

static int M_GetCurrentSwitchValue(optmenuitem_t *item)
{
	int retval = 0;

	switch(item->type)
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

static void M_DefaultMenuItem(optmenuitem_t *item)
{
	switch (item->type)
	{
		case OPT_Boolean:
		{
			*(bool*)item->switchvar = (item->default_val)?true:false;
			break;
		}

		case OPT_KeyConfig:
		case OPT_Slider:
		case OPT_Switch:
		{
			*(int*)(item->switchvar) = item->default_val;
			break;
		}

		default:
		{
			break;
		}
	}
}

//
//  MAIN MENU
//
#define LANGUAGE_POS  8
#define HOSTNET_POS   11

static optmenuitem_t mainmenu[] =
{
	{OPT_Function, "Keyboard Controls", NULL,  0, 0, NULL, M_KeyboardOptions, "Controls"},
	{OPT_Function, "Mouse Options",     NULL,  0, 0, NULL, M_AnalogueOptions, "AnalogueOptions"},
	{OPT_Function, "Gameplay Options",  NULL,  0, 0, NULL, M_GameplayOptions, "GameplayOptions"},
	{OPT_Plain,    "",                  NULL,  0, 0, NULL, NULL, NULL},
	{OPT_Function, "Sound Options",     NULL,  0, 0, NULL, M_SoundOptions, NULL},
	{OPT_Function, "Video Options",     NULL,  0, 0, NULL, M_VideoOptions, "VideoOptions"},
	{OPT_Function, "Set Resolution",    NULL,  0, 0, NULL, M_ResolutionOptions, "ChangeRes"},

	{OPT_Plain,    "",                  NULL,  0, 0, NULL, NULL, NULL},
	{OPT_Function, "Language",          NULL,  0, CFGDEF_MENULANGUAGE, NULL, M_ChangeLanguage, NULL},
	{OPT_Switch,   "Messages",          YesNo, 2, CFGDEF_SHOWMESSAGES, &showMessages, NULL, "Messages"},
	{OPT_Plain,    "",                  NULL,  0, 0, NULL, NULL, NULL},
	{OPT_Function, "Host NetGame",      NULL,  0, 0, NULL, M_HostNetGame, NULL},
	{OPT_Function, "Join NetGame",      NULL,  0, 0, NULL, M_JoinNetGame, NULL},
	{OPT_Plain,    "",                  NULL,  0,              0, NULL, NULL, NULL},
	{OPT_Function, "Reset to Defaults", NULL,  0, 0, NULL, M_ResetToDefaults, "ResetToDefaults"}
};

static menuinfo_t mainmenuinfo = 
{
	mainmenu, sizeof(mainmenu) / sizeof(optmenuitem_t), 
	&opt_def_style, 164, 108, "M_OPTTTL", NULL, 0, "", NULL, NULL
};

//
//  VIDEO OPTIONS
//
// -ACB- 1998/07/15 Altered menu structure

// -ES- 1999/03/29 New fov stuff
static optmenuitem_t vidoptions[] =
{
	{OPT_Slider,  "Brightness",    NULL,  5,  CFGDEF_CURRENT_GAMMA,  &current_gamma, M_ChangeGamma, NULL},

#if 0 // Really don't get the reason for these - a map developers 
	{OPT_Slider,  "Field Of View", NULL,  35, 17,      &menunormalfov, M_ChangeNormalFOV, NULL},
	{OPT_Slider,  "Zoomed FOV",    NULL,  35, 1,      &menuzoomedfov, M_ChangeZoomedFOV, NULL},
#endif 

	{OPT_Plain,   "",              NULL,  0,  0, NULL, NULL, NULL},

	{OPT_Boolean, "Smoothing",     YesNo, 2,  CFGDEF_USE_SMOOTHING,  &use_smoothing, M_ChangeMipMap, NULL},

	{OPT_Switch,  "Dynamic Lighting", DLMode, 2, CFGDEF_USE_DLIGHTS,    &use_dlights, M_ChangeDLights, NULL},

	{OPT_Boolean, "Doom-like fading", YesNo, 2, CFGDEF_DOOM_FADING,    &doom_fading, NULL, NULL},

	{OPT_Switch,  "Detail Level",  Details, 3, CFGDEF_DETAIL_LEVEL,   &detail_level, M_ChangeMipMap, NULL},

	{OPT_Switch,  "Mipmapping", MipMaps, 3,   CFGDEF_USE_MIPMAPPING, &use_mipmapping, M_ChangeMipMap, NULL},

	{OPT_Plain,   "",              NULL, 0,   0,                      NULL, NULL, NULL},

	{OPT_Switch,  "HUD",           Huds,  3,  CFGDEF_SCREEN_HUD,     &screen_hud, M_ChangeScreenHud, NULL},
	{OPT_Switch,  "Sky stretch",   SkySq, 5,  CFGDEF_SKY_STRETCH,    &sky_stretch, M_ChangeMipMap, NULL},
	{OPT_Boolean, "Shadows",       YesNo, 2,  CFGDEF_SHADOWS,  &global_flags.shadows, M_ChangeShadows, NULL},
	{OPT_Switch,  "Crosshair",     CrosO, 4,  CFGDEF_CROSSHAIR,      &crosshair, NULL, NULL},
	{OPT_Boolean, "Map Overlay",   YesNo, 2,  CFGDEF_MAP_OVERLAY,    &map_overlay, NULL, NULL},
	{OPT_Boolean, "Map Rotation",  YesNo, 2,  CFGDEF_ROTATEMAP,      &rotatemap, NULL, NULL},
	{OPT_Switch,  "Teleport Flash",YesNo, 2,  CFGDEF_TELEPT_FLASH,   &telept_flash, NULL, NULL},

	{OPT_Switch,  "Wipe method",   WIPE_EnumStr, WIPE_NUMWIPES, 
                                              CFGDEF_WIPE_METHOD,    &wipe_method, NULL, NULL},
  
	{OPT_Boolean, "Screenshot Format",  JpgPng, 2, 
                                              CFGDEF_PNG_SCRSHOTS,   &png_scrshots, NULL, NULL}

#if 0  // TEMPORARILY DISABLED (we need an `Advanced Options' menu)
	{OPT_Switch,  "Teleportation effect", WIPE_EnumStr, WIPE_NUMWIPES, 
                                              CFGDEF_TELEPT_EFFECT,  &telept_effect, NULL, NULL},

    {OPT_Switch,  "Reverse effect", YesNo, 2, CFGDEF_TELEPT_REVERSE, &telept_reverse, NULL, NULL},
    {OPT_Switch,  "Reversed wipe",  YesNo, 2, CFGDEF_WIPE_REVERSE, &wipe_reverse, NULL, NULL},
#endif
};

static menuinfo_t vidoptionsinfo = 
{
	vidoptions, sizeof(vidoptions) / sizeof(optmenuitem_t),
	&video_style, 150, 77, "M_VIDEO", NULL, 0, "", NULL, NULL
};

//
//  SET RESOLUTION MENU
//
static optmenuitem_t resoptions[] =
{
	{OPT_Plain, "", NULL, 0, 0, NULL, NULL, NULL},
	{OPT_Plain, "", NULL, 0, 0, NULL, NULL, NULL},
	{OPT_Plain, "", NULL, 0, 0, NULL, NULL, NULL},
	{OPT_Function, "Change Size", NULL, 0, 0, NULL, M_ChangeStoredRes, NULL},
	{OPT_Function, "Change Depth", NULL, 0, 0, NULL, M_ChangeStoredBpp, NULL},
	{OPT_Function, "Change Mode", NULL, 0, 0, NULL, M_ChangeStoredMode, NULL},
	{OPT_Plain, "", NULL, 0, 0, NULL, NULL, NULL},
	{OPT_Function, "Set Resolution", NULL, 0, 0, NULL, M_OptionSetResolution, NULL},
	{OPT_Function, "Test Resolution", NULL, 0, 0, NULL, M_OptionTestResolution, NULL},
	{OPT_Plain, "", NULL, 0, 0, NULL, NULL, NULL},
	{OPT_Plain, "", NULL, 0, 0, NULL, NULL, NULL},
	{OPT_Plain, "", NULL, 0, 0, NULL, NULL, NULL}
};

static menuinfo_t resoptionsinfo = 
{
	resoptions, sizeof(resoptions) / sizeof(optmenuitem_t),
	&setres_style, 150, 77, "M_VIDEO", NULL, 3, "", NULL, NULL
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
	{OPT_Boolean,  "Invert Mouse",       YesNo, 2, CFGDEF_INVERTMOUSE,      &invertmouse, NULL, NULL},
	{OPT_Switch,   "Mouse X Axis",       Axis, 6,  CFGDEF_MOUSE_XAXIS,      &mouse_xaxis, NULL, NULL},
	{OPT_Switch,   "Mouse Y Axis",       Axis, 6,  CFGDEF_MOUSE_YAXIS,      &mouse_yaxis, NULL, NULL},
	{OPT_Slider,   "MouseSpeed",         NULL, 20, CFGDEF_MOUSESENSITIVITY, &mouseSensitivity, NULL, NULL},
	{OPT_Plain,    "",                   NULL, 0,  0,                        NULL, NULL, NULL},
	{OPT_Slider,   "MLook Speed",        NULL, 20, CFGDEF_MLOOKSPEED,       &mlookspeed, NULL, NULL},
	{OPT_Plain,    "",                   NULL, 0,  0,                        NULL, NULL, NULL},
	{OPT_Boolean,  "Two-Stage Turning",  YesNo, 2, CFGDEF_STAGETURN,        &stageturn, NULL, NULL},
	{OPT_Slider,   "Turning Speed",      NULL, 9,  CFGDEF_ANGLETURNSPEED,   &angleturnspeed, NULL, NULL},
	{OPT_Slider,   "Side Move Speed",    NULL, 9,  CFGDEF_SIDEMOVESPEED,    &sidemovespeed, NULL, NULL},
	{OPT_Slider,   "Forward Move Speed", NULL, 9,  CFGDEF_FORWARDMOVESPEED, &forwardmovespeed, NULL, NULL}

#if 0  // DISABLED, Because no joystick support yet
	{OPT_Plain,    "",                   NULL, 0,  0,                        NULL, NULL, NULL},
	{OPT_Switch,   "Joystick X Axis",    Axis, 6,  CFGDEF_JOY_XAXIS,        &joy_xaxis, NULL, NULL},
	{OPT_Switch,   "Joystick Y Axis",    Axis, 6,  CFGDEF_JOY_YAXIS,        &joy_yaxis, NULL, NULL},
	{OPT_Function, "Calibrate Joystick", NULL, 0,  0,                        NULL, M_CalibrateJoystick, NULL}
#endif
};

static menuinfo_t analogueoptionsinfo = 
{
	analogueoptions, sizeof(analogueoptions) / sizeof(optmenuitem_t),
	&mouse_style, 150, 75, "M_MSETTL", NULL, 0, "", NULL, NULL
};

//
//  SOUND OPTIONS
//
// -AJA- 2007/03/14 Added new sound menu
//
static optmenuitem_t soundoptions[] =
{
	{OPT_Slider,   "Sound Volume",  NULL, SND_SLIDER_NUM, CFGDEF_SOUND_VOLUME, &sfx_volume, M_ChangeSfxVol, NULL},
	{OPT_Slider,   "Music Volume",  NULL, SND_SLIDER_NUM, CFGDEF_MUSIC_VOLUME, &mus_volume, M_ChangeMusVol, NULL},

	{OPT_Plain,    "", NULL, 0,  0, NULL, NULL, NULL},
	{OPT_Switch,   "Sample Rate",   SampleRates, 5, CFGDEF_SAMPLE_RATE,  &var_sample_rate,  NULL, NULL},
	{OPT_Switch,   "Sample Size",   SoundBits, 2,   CFGDEF_SOUND_BITS,   &var_sound_bits,   NULL, NULL},
	{OPT_Switch,   "Stereo",        StereoNess, 3,  CFGDEF_SOUND_STEREO, &var_sound_stereo, NULL, NULL},

	{OPT_Plain,    "", NULL, 0,  0, NULL, NULL, NULL},
	{OPT_Switch,  "Mix Channels",   MixChans,  4, CFGDEF_MIX_CHANNELS, &var_mix_channels, M_ChangeMixChan, NULL},
	{OPT_Switch,  "Quiet Factor",   QuietNess, 3, CFGDEF_QUIET_FACTOR, &var_quiet_factor, NULL, NULL},
};

static menuinfo_t soundoptionsinfo = 
{
	soundoptions, sizeof(soundoptions) / sizeof(optmenuitem_t),
	&mouse_style, 150, 75, "M_SVOL", NULL, 0, "", NULL, NULL
};

//
//  GAMEPLAY OPTIONS
//
// -ACB- 1998/07/15 Altered menu structure
// -KM- 1998/07/21 Change blood to switch
//
static optmenuitem_t playoptions[] =
{
	{OPT_Switch,  "AutoAiming",         AAim, 3, 
     CFGDEF_AUTOAIM,     
     &global_flags.autoaim, M_ChangeAutoAim, NULL},

	{OPT_Boolean, "Mouse Look",         YesNo, 2, 
     CFGDEF_MLOOK,       
     &global_flags.mlook, M_ChangeMLook, NULL},

	{OPT_Boolean, "Jumping",            YesNo, 2, 
     CFGDEF_JUMP,        
     &global_flags.jump, M_ChangeJumping, NULL},

	{OPT_Boolean, "Crouching",          YesNo, 2, 
     CFGDEF_CROUCH,      
     &global_flags.crouch, M_ChangeCrouching, NULL},

	{OPT_Boolean, "Weapon Kick",        YesNo, 2, 
     CFGDEF_KICKING,        
     &global_flags.kicking, M_ChangeKicking, NULL},

	{OPT_Boolean, "Weapon Auto-Switch", YesNo, 2, 
     CFGDEF_WEAPON_SWITCH, 
     &global_flags.weapon_switch, M_ChangeWeaponSwitch, NULL},

	{OPT_Boolean, "More Blood",         YesNo, 2, 
     CFGDEF_MORE_BLOOD, 
     &global_flags.more_blood, M_ChangeBlood, "Blood"},

	{OPT_Boolean, "Extras",             YesNo, 2, 
     CFGDEF_HAVE_EXTRA, 
     &global_flags.have_extra, M_ChangeExtra, NULL},

	{OPT_Boolean, "True 3D Gameplay",   YesNo, 2, 
     CFGDEF_TRUE3DGAMEPLAY, 
     &global_flags.true3dgameplay, M_ChangeTrue3d, "True3d"},

	{OPT_Boolean, "Shoot-thru Scenery",   YesNo, 2, 
     CFGDEF_PASS_MISSILE, 
     &global_flags.pass_missile, M_ChangePassMissile, NULL},

	{OPT_Plain,   "", NULL, 0, 0, NULL, NULL, NULL},

	{OPT_Slider,  "Gravity",            NULL, 20, 
     CFGDEF_MENU_GRAV, 
     &global_flags.menu_grav, NULL, "Gravity"},

	{OPT_Plain,   "", NULL, 0, 0, NULL, NULL, NULL},

	{OPT_Boolean, "Enemy Respawn Mode", Respw, 2, 
     CFGDEF_RES_RESPAWN, 
     &global_flags.res_respawn, M_ChangeMonsterRespawn, NULL},

	{OPT_Boolean, "Item Respawn",       YesNo, 2, 
     CFGDEF_ITEMRESPAWN, 
     &global_flags.itemrespawn, M_ChangeItemRespawn, NULL},
	
    {OPT_Boolean, "Fast Monsters",      YesNo, 2, 
     CFGDEF_FASTPARM, 
     &global_flags.fastparm, M_ChangeFastparm, NULL},
	
    {OPT_Boolean, "Respawn",            YesNo, 2, 
     CFGDEF_RESPAWN, 
     &global_flags.respawn, M_ChangeRespawn, NULL}
};

static menuinfo_t playoptionsinfo = 
{
	playoptions, sizeof(playoptions) / sizeof(optmenuitem_t),
	&gameplay_style, 160, 46, "M_GAMEPL", NULL, 0, "", NULL, NULL
};

//
//  KEY CONFIG : STANDARD
//
// -ACB- 1998/07/15 Altered menuinfo struct
// -KM- 1998/07/10 Used better names :-)
//
static optmenuitem_t stdkeyconfig[] =
{
	{OPT_KeyConfig, "Primary Attack", NULL, 0, CFGDEF_KEY_FIRE,        &key_fire, NULL, NULL},
    {OPT_KeyConfig, "Secondary Atk",  NULL, 0, CFGDEF_KEY_SECONDATK,   &key_secondatk, NULL, NULL},
	{OPT_KeyConfig, "Use Item",       NULL, 0, CFGDEF_KEY_USE,         &key_use, NULL, NULL},
	{OPT_KeyConfig, "Walk Forward",   NULL, 0, CFGDEF_KEY_UP,          &key_up, NULL, NULL},
	{OPT_KeyConfig, "Walk Backwards", NULL, 0, CFGDEF_KEY_DOWN,        &key_down, NULL, NULL},
	{OPT_KeyConfig, "Turn Left",      NULL, 0, CFGDEF_KEY_LEFT,        &key_left, NULL, NULL},
	{OPT_KeyConfig, "Turn Right",     NULL, 0, CFGDEF_KEY_RIGHT,       &key_right, NULL, NULL},
	{OPT_KeyConfig, "Move Up",        NULL, 0, CFGDEF_KEY_FLYUP,       &key_flyup, NULL, NULL},
	{OPT_KeyConfig, "Move Down",      NULL, 0, CFGDEF_KEY_FLYDOWN,     &key_flydown, NULL, NULL},
	{OPT_Plain,      "",              NULL, 0, 0,                       NULL, NULL, NULL},
	{OPT_KeyConfig, "Run",            NULL, 0, CFGDEF_KEY_SPEED,       &key_speed, NULL, NULL},
	{OPT_KeyConfig, "Strafe",         NULL, 0, CFGDEF_KEY_STRAFE,      &key_strafe, NULL, NULL},
	{OPT_KeyConfig, "Strafe Left",    NULL, 0, CFGDEF_KEY_STRAFELEFT,  &key_strafeleft, NULL, NULL},
	{OPT_KeyConfig, "Strafe Right",   NULL, 0, CFGDEF_KEY_STRAFERIGHT, &key_straferight, NULL, NULL},
	{OPT_KeyConfig, "Toggle Autorun", NULL, 0, CFGDEF_KEY_AUTORUN,     &key_autorun, NULL, NULL}
};

static menuinfo_t stdkeyconfiginfo = 
{
	stdkeyconfig, sizeof(stdkeyconfig) / sizeof(optmenuitem_t),
	&keyboard_style, 110, 98, "M_CONTRL", NULL, 0, "Page 1", NULL, NULL
};

//
//  KEY CONFIG : EXTENDED
//
// -ACB- 1998/07/15 Altered menuinfo struct
// -ES- 1999/03/28 Added Zoom Key
//
static optmenuitem_t extkeyconfig[] =
{
	{OPT_KeyConfig, "Look Up",         NULL, 0, CFGDEF_KEY_LOOKUP,     &key_lookup, NULL, NULL},
	{OPT_KeyConfig, "Look Down",       NULL, 0, CFGDEF_KEY_LOOKDOWN,   &key_lookdown, NULL, NULL},
	{OPT_KeyConfig, "Center View",     NULL, 0, CFGDEF_KEY_LOOKCENTER, &key_lookcenter, NULL, NULL},
	{OPT_KeyConfig, "Mouse Look",      NULL, 0, CFGDEF_KEY_MLOOK,      &key_mlook, NULL, NULL},
	{OPT_KeyConfig, "Zoom in/out",     NULL, 0, CFGDEF_KEY_ZOOM,       &key_zoom, NULL, NULL},
	{OPT_Plain,     "",                NULL, 0, 0,                      NULL, NULL, NULL},
	{OPT_KeyConfig, "Jump",            NULL, 0, CFGDEF_KEY_JUMP,       &key_jump, NULL, NULL},
	{OPT_KeyConfig, "Map Toggle",      NULL, 0, CFGDEF_KEY_MAP,        &key_map, NULL, NULL},
	{OPT_KeyConfig, "180 degree turn", NULL, 0, CFGDEF_KEY_180,        &key_180, NULL, NULL},
    {OPT_KeyConfig, "Weapon Reload",   NULL, 0, CFGDEF_KEY_RELOAD,     &key_reload, NULL, NULL},
    {OPT_KeyConfig, "Next Weapon",     NULL, 0, CFGDEF_KEY_NEXTWEAPON, &key_nextweapon, NULL, NULL},
    {OPT_KeyConfig, "Previous Weapon", NULL, 0, CFGDEF_KEY_PREVWEAPON, &key_prevweapon, NULL, NULL},
	{OPT_KeyConfig, "Multiplay Talk",  NULL, 0, CFGDEF_KEY_TALK,       &key_talk, NULL, NULL}
};

static menuinfo_t extkeyconfiginfo = 
{
	extkeyconfig, sizeof(extkeyconfig) / sizeof(optmenuitem_t),
	&keyboard_style, 110, 98, "M_CONTRL", NULL, 0, "Page 2", NULL, NULL
};

static char keystring1[] = "Enter to change, Backspace to Clear";
static char keystring2[] = "Press a key for this action";

static specialkey_t specialkeylist[] =  // terminate on -1
{
    {KEYD_RIGHTARROW, "Right Arrow"},
    {KEYD_LEFTARROW, "Left Arrow"},
    {KEYD_UPARROW, "Up Arrow"},
    {KEYD_DOWNARROW, "Down Arrow"},
    {KEYD_ESCAPE, "Escape"},
    {KEYD_ENTER, "Enter"},
    {KEYD_TAB, "Tab"},
    {KEYD_F1, "F1"},
    {KEYD_F2, "F2"},
    {KEYD_F3, "F3"},
    {KEYD_F4, "F4"},
    {KEYD_F5, "F5"},
    {KEYD_F6, "F6"},
    {KEYD_F7, "F7"},
    {KEYD_F8, "F8"},
    {KEYD_F9, "F9"},
    {KEYD_F10, "F10"},
    {KEYD_F11, "F11"},
    {KEYD_F12, "F12"},
    {KEYD_BACKSPACE, "Backspace"},
    {KEYD_EQUALS, "Equals"},
    {KEYD_MINUS, "Minus"},
    {KEYD_RSHIFT, "Shift"},
    {KEYD_RCTRL, "Ctrl"},
    {KEYD_RALT, "Alt"},
    {KEYD_INSERT, "Insert"},
    {KEYD_DELETE, "Delete"},
    {KEYD_PGDN, "PageDown"},
    {KEYD_PGUP, "PageUp"},
    {KEYD_HOME, "Home"},
    {KEYD_END, "End"},
    {KEYD_SCRLOCK,  "ScrollLock"},
    {KEYD_NUMLOCK,  "NumLock"},
    {KEYD_CAPSLOCK, "CapsLock"},
    {KEYD_END, "End"},
    {32, "Space"},
    {'\'', "\'"},
    {KEYD_TILDE, "Tilde"},
    {KEYD_MOUSE1, "Mouse1"},
    {KEYD_MOUSE2, "Mouse2"},
    {KEYD_MOUSE3, "Mouse3"},
    {KEYD_MOUSE4, "Mouse4"},
    {KEYD_MWHEEL_UP, "Wheel Up"},
    {KEYD_MWHEEL_DN, "Wheel Down"},
    {-1, ""}
};

//
// M_OptCheckNetgame
//
// Sets the first option to be "Leave Game" or "Multiplayer Game"
// depending on whether we are playing a game or not.
//
void M_OptCheckNetgame(void)
{
	if (usergame)
	{
		strcpy(mainmenu[HOSTNET_POS+0].name, "Leave Game");
		mainmenu[HOSTNET_POS+0].routine = &M_EndGame;
		mainmenu[HOSTNET_POS+0].help = NULL;

		strcpy(mainmenu[HOSTNET_POS+1].name, "");
		mainmenu[HOSTNET_POS+1].type = OPT_Plain;
		mainmenu[HOSTNET_POS+1].routine = NULL;
		mainmenu[HOSTNET_POS+1].help = NULL;
	}
	else
	{
		strcpy(mainmenu[HOSTNET_POS+0].name, "Host Net Game");
		mainmenu[HOSTNET_POS+0].routine = &M_HostNetGame;
		mainmenu[HOSTNET_POS+0].help = NULL;

		strcpy(mainmenu[HOSTNET_POS+1].name, "Join Net Game");
		mainmenu[HOSTNET_POS+1].type = OPT_Function;
		mainmenu[HOSTNET_POS+1].routine = &M_JoinNetGame;
		mainmenu[HOSTNET_POS+1].help = NULL;
	}
}

//
// Menu Initialisation
//
void M_OptMenuInit()
{
	option_menuon = 0;
	curr_menu = &mainmenuinfo;
	curr_item = curr_menu->items + curr_menu->pos;
	keyscan = 0;

	// load styles
	styledef_c *def;

	def = styledefs.Lookup("OPTIONS");
	if (! def) def = default_style;
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

	// Needed to handle the circular reference that C++ init doesn't allow
	stdkeyconfiginfo.sister_next = &extkeyconfiginfo;
	extkeyconfiginfo.sister_prev = &stdkeyconfiginfo;

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
	if (setresfailed)
	{
		epi::string_c s;
		
		scrmode_t* sm = scrmodelist[selectedscrmode];
		
		s.Format(language["ModeSelErr"],
				sm->width, sm->height, sm->depth);

		M_StartMessage(s.GetString(), NULL, false);
		
		testticker = -1;
		
		selectedscrmode = prevscrmode;
		setresfailed = false;
	}

	if (testticker > 0)
	{
		testticker--;
	}
	else if (!testticker)
	{
		testticker--;
		M_RestoreResSettings(-1);
	}
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

	style_c *style = curr_menu->style_var[0];
	SYS_ASSERT(style);

	style->DrawBackground();

	int font_h = style->fonts[0]->NominalHeight(); // FIXME: fonts[0] maybe null

	// -ACB- 1998/06/15 Calculate height for menu and then center it.
	menutop = 68 - ((curr_menu->item_num * font_h) / 2);

	{
		const image_t *image;

		if (! curr_menu->title_image)
			curr_menu->title_image = W_ImageLookup(curr_menu->title_name);

		image = curr_menu->title_image;

		RGL_ImageEasy320(curr_menu->title_x, menutop, image);
	}

	//now, draw all the menuitems
	deltay = 1 + font_h;

	curry = menutop + 25;

	if (curr_menu->key_page[0])
	{
		if (curr_menu->sister_prev)
			HL_WriteText(style,2, 60, 200-deltay*4, "< PREV");

		if (curr_menu->sister_next)
			HL_WriteText(style,2, 260 - style->fonts[2]->StringWidth("NEXT >"), 200-deltay*4, 
							  "NEXT >");

		HL_WriteText(style,2, 160 - style->fonts[2]->StringWidth(curr_menu->key_page)/2, 
						  200-deltay*4, curr_menu->key_page);
    
		if (keyscan)
			HL_WriteText(style,3, 160 - (style->fonts[3]->StringWidth(keystring2) / 2), 
							  200-deltay*2, keystring2);
		else
			HL_WriteText(style,3, 160 - (style->fonts[3]->StringWidth(keystring1) / 2), 
							  200-deltay*2, keystring1);
	}
	else if (curr_menu == &resoptionsinfo)
	{
		M_ResOptDrawer(style, curry, curry + (deltay * (resoptionsinfo.item_num - 2)), 
					   deltay, curr_menu->menu_center);
	}
	else if (curr_menu == &mainmenuinfo)
	{
		M_LanguageDrawer(curr_menu->menu_center, curry, deltay);
	}

	for (i = 0; i < curr_menu->item_num; i++)
	{
		HL_WriteText(style,0, (curr_menu->menu_center) - style->fonts[0]->StringWidth(curr_menu->items[i].name),
					 curry, curr_menu->items[i].name);

		// -ACB- 1998/07/15 Menu Cursor is colour indexed.
		if (i == curr_menu->pos)
		{
			HL_WriteText(style,2, (curr_menu->menu_center + 4), curry, "*");

			if (curr_menu->items[i].help)
			{
				const char *help = language[curr_menu->items[i].help];

				HL_WriteText(style,3, 160 - (style->fonts[3]->StringWidth(help) / 2), 200 - deltay*2, 
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

				HL_WriteText(style,1, (curr_menu->menu_center) + 15, curry, tempstring);
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

				k = *(int*)(curr_menu->items[i].switchvar);
				M_Key2String(k, tempstring);
				HL_WriteText(style,1, (curr_menu->menu_center + 15), curry, tempstring);
				break;

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
	sprintf(tempstring, "Current Resolution:");
	HL_WriteText(style,0, 160 - (style->fonts[0]->StringWidth(tempstring) / 2), y, tempstring);

	y += dy;
	sprintf(tempstring, "%d x %d at %d-bit %s", SCREENWIDTH, SCREENHEIGHT,
			SCREENBITS, SCREENWINDOW ? "Windowed" : "Fullscreen");
	HL_WriteText(style,1, 160 - (style->fonts[1]->StringWidth(tempstring) / 2), y, tempstring);

	scrmode_t* cur_mode = scrmodelist[selectedscrmode];

	// Draw resolution selection option
	y += (dy*2);
	sprintf(tempstring, "%dx%d", cur_mode->width, cur_mode->height);
	HL_WriteText(style,1, centrex+15, y, tempstring);

	// Draw depth selection option
	y += dy;
	sprintf(tempstring, "%d bit", cur_mode->depth);
	HL_WriteText(style,1, centrex+15, y, tempstring);

	y += dy;
	sprintf(tempstring, "%s", cur_mode->windowed ?
		 "Windowed" : "Fullscreen");
	HL_WriteText(style,1, centrex+15, y, tempstring);

	// Draw selected resolution and mode:
	y = bottomy;
	sprintf(tempstring, "Selected Resolution:");
	HL_WriteText(style,0, 160 - (style->fonts[0]->StringWidth(tempstring) / 2), y, tempstring);

	y += dy;

	sprintf(tempstring, "%d x %d at %d-bit %s",
			cur_mode->width, cur_mode->height, cur_mode->depth,
			cur_mode->windowed ? "Windowed" : "Fullscreen");

	HL_WriteText(style,1, 160 - (style->fonts[1]->StringWidth(tempstring) / 2), y, tempstring);
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
	epi::string_c buf;

	buf.Format("%s", language.GetName());

	HL_WriteText(opt_def_style,1, x+15, y + deltay * LANGUAGE_POS, buf);
}

//
// M_OptResponder
//
bool M_OptResponder(event_t * ev, int ch)
{
	if (testticker != -1)
		return true;

	// Scan for keycodes
	if (keyscan)
	{
		int *blah;
		int key;

		if (ev->type != ev_keydown)
			return false;
		key = ev->value.key;

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
		{
			if (curr_item->type == OPT_KeyConfig)
				*(int*)(curr_item->switchvar) = 0;
			return true;
		}

		case KEYD_DOWNARROW:
		{
			do
			{
				curr_menu->pos++;
				if (curr_menu->pos >= curr_menu->item_num)
					curr_menu->pos = 0;
				curr_item = curr_menu->items + curr_menu->pos;
			}
			while (curr_item->type == 0);

			S_StartFX(sfx_pstop);
			return true;
		}

		case KEYD_UPARROW:
		{
			do
			{
				curr_menu->pos--;
				if (curr_menu->pos < 0)
					curr_menu->pos = curr_menu->item_num - 1;
				curr_item = curr_menu->items + curr_menu->pos;
			}
			while (curr_item->type == 0);

			S_StartFX(sfx_pstop);
			return true;
		}

		case KEYD_LEFTARROW:
		{
			if (curr_menu->key_page[0])
			{
				if (curr_menu->sister_prev)
				{
					curr_menu = curr_menu->sister_prev;
					curr_item = curr_menu->items + curr_menu->pos;

					S_StartFX(sfx_pstop);
				}
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
				if (curr_menu->sister_next)
				{
					curr_menu = curr_menu->sister_next;
					curr_item = curr_menu->items + curr_menu->pos;

					S_StartFX(sfx_pstop);
				}
				return true;
			}

      /* FALL THROUGH... */
     
		case KEYD_ENTER:
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
		{
			if (curr_menu == &mainmenuinfo)
			{
				option_menuon = 0;
			}
			else
			{
				curr_menu = &mainmenuinfo;
				curr_item = curr_menu->items + curr_menu->pos;
			}
			S_StartFX(sfx_swtchx);
			return true;
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
	curr_menu = &vidoptionsinfo;
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
	// Get a depth mask for resolution selection
	SYS_ASSERT(SCREENBITS == 16 || SCREENBITS == 32);

	int i = scrmodelist.Find(SCREENWIDTH, 
							 SCREENHEIGHT,
							 SCREENBITS,
							 SCREENWINDOW);
							 
	if (i < 0)
		I_Error("M_ResolutionOptions: Graphics mode not listed!");
	
	selectedscrmode = i;
	prevscrmode = i;
	
	curr_menu = &resoptionsinfo;
	curr_item = curr_menu->items + curr_menu->pos;
}

//
// M_AnalogueOptions
//
static void M_AnalogueOptions(int keypressed)
{
	curr_menu = &analogueoptionsinfo;
	curr_item = curr_menu->items + curr_menu->pos;
}

//
// M_SoundOptions
//
static void M_SoundOptions(int keypressed)
{
	curr_menu = &soundoptionsinfo;
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

	curr_menu = &playoptionsinfo;
	curr_item = curr_menu->items + curr_menu->pos;
}

//
// M_KeyboardOptions
//
static void M_KeyboardOptions(int keypressed)
{
	curr_menu = &stdkeyconfiginfo;
	curr_item = curr_menu->items + curr_menu->pos;
}

// ===== END OF SUB-MENUS =====

//
// M_ResetToDefaults
//
void M_ResetToDefaults(int keypressed)
{
	int i;

	for (i = 0; i < mainmenuinfo.item_num; i++)
		M_DefaultMenuItem(&mainmenu[i]);

	for (i = 0; i < vidoptionsinfo.item_num; i++)
		M_DefaultMenuItem(&vidoptions[i]);

	for (i = 0; i < playoptionsinfo.item_num; i++)
		M_DefaultMenuItem(&playoptions[i]);

	for (i = 0; i < analogueoptionsinfo.item_num; i++)
		M_DefaultMenuItem(&analogueoptions[i]);

	for (i = 0; i < soundoptionsinfo.item_num; i++)
		M_DefaultMenuItem(&soundoptions[i]);

	for (i = 0; i < stdkeyconfiginfo.item_num; i++)
		M_DefaultMenuItem(&stdkeyconfig[i]);

	for (i = 0; i < extkeyconfiginfo.item_num; i++)
		M_DefaultMenuItem(&extkeyconfig[i]);
}

//
// M_Key2String
//
static void M_Key2String(int key, char *deststring)
{
	int key1, key2;
	char key2string[100];
	int j;

	if (((key & 0xffff) == 0) && ((key >> 16) != 0))
		I_Error("key problem!");

	if (key == 0)
	{
		strcpy(deststring, "---");
		return;
	}
	key1 = key & 0xffff;
	key2 = key >> 16;

	//first do key 1
	if ((toupper(key1) >= ',') && (toupper(key1) <= ']'))
	{
		deststring[0] = toupper(key1);
		deststring[1] = 0;
	}
	else
	{
		if (key1 >= KEYD_JOYBASE)
			sprintf(deststring, "Joystick %d", key1 - KEYD_JOYBASE + 1);
		else
			sprintf(deststring, "Keycode %d", key1);
		j = 0;
		while (specialkeylist[j].keycode != -1)
		{
			if (specialkeylist[j].keycode == key1)
			{
				strcpy(deststring, specialkeylist[j].keystring);
				break;
			}
			j++;
		}
	}

	if (key2 == 0)
		return;

	//now, do key 2
	if ((toupper(key2) >= ',') && (toupper(key2) <= ']'))
	{
		key2string[0] = toupper(key2);
		key2string[1] = 0;
	}
	else
	{
		if (key2 >= KEYD_JOYBASE)
			sprintf(key2string, "Joystick %d", key2 - KEYD_JOYBASE + 1);
		else
			sprintf(key2string, "Keycode %d", key2);
		j = 0;
		while (specialkeylist[j].keycode != -1)
		{
			if (specialkeylist[j].keycode == key2)
			{
				strcpy(key2string, specialkeylist[j].keystring);
				break;
			}
			j++;
		}
	}
	strcat(deststring, " or ");
	strcat(deststring, key2string);
	return;
}

#if 0
//
// M_CalibrateJoystick
//
// 1998/07/10 -KM- Recalibration of Joystick
// 2000/11/03 -ACB- Unable to use as non-portable
//
static void M_CalibrateJoystick(int keypressed)
{
	I_CalibrateJoystick(0);
}
#endif

//
// M_ChangeGamma
//
// -AJA- 1999/07/03: stuck this here & removed PLAYPAL reference.
//
static void M_ChangeGamma(int keypressed)
{
  /* nothing to do */
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
	W_ResetImages();
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
}


//
// M_ChangeStoredRes
//
// -ACB- 1998/08/29 Resolution Changes...
//
static void M_ChangeStoredRes(int keypressed)
{
	// Ignore anything but LEFT and RIGHT arrow keys
	if (keypressed != KEYD_LEFTARROW && keypressed != KEYD_RIGHTARROW)
		return;

	if (keypressed == KEYD_LEFTARROW)
	{
		int mode = selectedscrmode; 
		selectedscrmode = scrmodelist.Prev(mode, scrmodelist_c::RES);
	}
	else /* if (keypressed == KEYD_RIGHTARROW) */
	{
		int mode = selectedscrmode; 
		selectedscrmode = scrmodelist.Next(mode, scrmodelist_c::RES);
	}
}

//
// M_ChangeStoredBpp
//
// -ACB- 1998/08/29 Depth Changes...
//
static void M_ChangeStoredBpp(int keypressed)
{
	// Ignore anything but LEFT and RIGHT arrow keys
	if (keypressed != KEYD_LEFTARROW && keypressed != KEYD_RIGHTARROW)
		return;

	if (keypressed == KEYD_LEFTARROW)
	{
		int mode = selectedscrmode; 
		selectedscrmode = scrmodelist.Prev(mode, scrmodelist_c::DEPTH);
	}
	else /* if (keypressed == KEYD_RIGHTARROW) */
	{
		int mode = selectedscrmode; 
		selectedscrmode = scrmodelist.Next(mode, scrmodelist_c::DEPTH);
	}
}

//
// M_ChangeStoredMode
//
// -AJA- 2005/01/02: Windowed vs Fullscreen
//
static void M_ChangeStoredMode(int keypressed)
{
	// Ignore anything but LEFT and RIGHT arrow keys
	if (keypressed != KEYD_LEFTARROW && keypressed != KEYD_RIGHTARROW)
		return;

	if (keypressed == KEYD_LEFTARROW)
	{
		int mode = selectedscrmode; 
		selectedscrmode = scrmodelist.Prev(mode, scrmodelist_c::WINDOWMODE);
	}
	else /* if (keypressed == KEYD_RIGHTARROW) */
	{
		int mode = selectedscrmode; 
		selectedscrmode = scrmodelist.Next(mode, scrmodelist_c::WINDOWMODE);
	}
}

//
// M_OptionSetResolution
//
static void M_OptionSetResolution(int keypressed)
{
    R_ChangeResolution(selectedscrmode);
}

//
// M_OptionTestResolution
//
static void M_OptionTestResolution(int keypressed)
{
    R_ChangeResolution(selectedscrmode);
	testticker = TICRATE * 3;
}

//
// M_RestoreResSettings
//
static void M_RestoreResSettings(int keypressed)
{
    R_ChangeResolution(prevscrmode);
}

extern void M_NetHostBegun(void);
extern void M_NetJoinBegun(void);

static void M_HostNetGame(int keypressed)
{
	option_menuon  = 0;
	netgame_menuon = 1;

	M_NetHostBegun();
}

static void M_JoinNetGame(int keypressed)
{
	option_menuon  = 0;
	netgame_menuon = 2;

	M_NetJoinBegun();
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
