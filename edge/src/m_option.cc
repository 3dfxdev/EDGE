//----------------------------------------------------------------------------
//  EDGE Option Menu Modification
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
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
#include "g_game.h"
#include "hu_stuff.h"
#include "m_menu.h"
#include "m_misc.h"
#include "p_local.h"
#include "r_main.h"
#include "r_vbinit.h"
#include "r2_defs.h"
#include "s_sound.h"
#include "v_ctx.h"
#include "v_res.h"
#include "v_colour.h"
#include "w_image.h"
#include "w_wad.h"
#include "wp_main.h"

#include "epi/epistring.h"

#define OPTSHADE  text_white_map

int optionsmenuon = 0;

//submenus
static void M_StandardControlOptions(int keypressed);
static void M_ExtendedControlOptions(int keypressed);

static void M_VideoOptions(int keypressed);
static void M_GameplayOptions(int keypressed);
static void M_AnalogueOptions(int keypressed);

static void M_CalibrateJoystick(int keypressed);

void M_ResetToDefaults(int keypressed);

static void M_Key2String(int key, char *deststring);

// -ACB- 1998/08/09 "Does Map allow these changes?" procedures.
static void M_ChangeMonsterRespawn(int keypressed);
static void M_ChangeItemRespawn(int keypressed);
static void M_ChangeTransluc(int keypressed);
static void M_ChangeTrue3d(int keypressed);
static void M_ChangeAutoAim(int keypressed);
static void M_ChangeFastparm(int keypressed);
static void M_ChangeRespawn(int keypressed);

//Special function declarations
int menunormalfov, menuzoomedfov;

static void M_ChangeBlood(int keypressed);
static void M_ChangeJumping(int keypressed);
static void M_ChangeCrouching(int keypressed);
static void M_ChangeExtra(int keypressed);
static void M_ChangeGamma(int keypressed);
static void M_ChangeShadows(int keypressed);
static void M_ChangeHalos(int keypressed);
static void M_ChangeCompatMode(int keypressed);
static void M_ChangeKicking(int keypressed);
static void M_ChangeMipMap(int keypressed);
static void M_ChangeDLights(int keypressed);

// -ES- 1998/08/20 Added resolution options
// -ACB- 1998/08/29 Moved to top and tried different system

static void M_ResOptDrawer(int topy, int bottomy, int dy, int centrex);
static void M_ResolutionOptions(int keypressed);
static void M_OptionSetResolution(int keypressed);
static void M_OptionTestResolution(int keypressed);
static void M_RestoreResSettings(int keypressed);
static void M_ChangeStoredRes(int keypressed);
static void M_ChangeStoredBpp(int keypressed);

static void M_LanguageDrawer(int x, int y, int deltay);
static void M_ChangeLanguage(int keypressed);

static char YesNo[] = "Off/On";  // basic on/off
static char CompatSet[] = "EDGE/Boom";
static char CrosO[] = "None/Cross/Dot/Angle";  // crosshair options
static char Respw[] = "Teleport/Resurrect";  // monster respawning
static char Axis[] = "Turn/Forward/Strafe/MLook/Fly/Disable";
static char SkySq[] = "Small/Medium/Large/Mirror";

// Screen resolution changes
static int prevscrmode;
static int selectedscrmode;
static int testticker = -1;

// Volume Changes
static int menumusicvol;
static int menusoundvol;

// -ES- 1998/11/28 Wipe and Faded teleportation options
//static char FadeT[] = "Off/On, flash/On, no flash";
static char AAim[] = "Off/On/Mlook";
static char Huds[] = "Full/None/Overlay";
static char MipMaps[] = "None/Good/Best";
static char Details[] = "Low/Medium/High";


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

//
// M_ChangeMusVol
//
// -ACB- 1999/11/13 Music API Change implemented
//
static void M_ChangeMusVol(int keypressed)
{
	S_SetMusicVolume(menumusicvol);
	return;
}

//
// M_ChangeSfxVol
//
// -ACB- 1999/10/07 Sound API Change implemented
//
static void M_ChangeSfxVol(int keypressed)
{
	S_SetSfxVolume(menusoundvol);
	return;
}

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
static optmenuitem_t mainmenu[] =
{
	{OPT_Function, "Keyboard Controls", NULL, 0, 0, NULL, M_StandardControlOptions, "Controls"},
	{OPT_Function, "Mouse Options", NULL, 0, 0, NULL, M_AnalogueOptions, "AnalogueOptions"},
	{OPT_Function, "Gameplay Options", NULL, 0, 0, NULL, M_GameplayOptions, "GameplayOptions"},
	{OPT_Function, "Video Options", NULL, 0, 0, NULL, M_VideoOptions, "VideoOptions"},
	{OPT_Function, "Set Resolution", NULL, 0, 0, NULL, M_ResolutionOptions, "ChangeRes"},
	{OPT_Plain, "", NULL, 0, 0, NULL, NULL, NULL},
	{OPT_Function, "Language", NULL, 0, 0, NULL, M_ChangeLanguage, NULL},
	{OPT_Switch, "Messages", YesNo, 2, 1, &showMessages, NULL, "Messages"},
	{OPT_Boolean, "Swap Stereo", YesNo, 2, 0, &swapstereo, NULL, "SwapStereo"},
	{OPT_Slider, "Sound Volume", NULL, 16, 12,  &menusoundvol, M_ChangeSfxVol, NULL},
	{OPT_Slider, "Music Volume", NULL, 16, 12,  &menumusicvol, M_ChangeMusVol, NULL},
	{OPT_Plain, "", NULL, 0, 0, NULL, NULL, NULL},
	{OPT_Function, "Reset to Defaults", NULL, 0, 0, NULL, M_ResetToDefaults, "ResetToDefaults"}
};

static menuinfo_t mainmenuinfo = 
{
	mainmenu, sizeof(mainmenu) / sizeof(optmenuitem_t), 
	164, 108, "M_OPTTTL", NULL, 0, "", NULL, NULL
};

//
//  VIDEO OPTIONS
//
// -ACB- 1998/07/15 Altered menu structure

// -ES- 1999/03/29 New fov stuff
static optmenuitem_t vidoptions[] =
{
	{OPT_Slider, "Brightness", NULL, 5, 0, &current_gamma, M_ChangeGamma, NULL},
	{OPT_Slider, "Field Of View", NULL, 35, 17, &menunormalfov, M_ChangeNormalFOV, NULL},
	{OPT_Slider, "Zoomed FOV", NULL, 35, 1, &menuzoomedfov, M_ChangeZoomedFOV, NULL},
	{OPT_Plain, "", NULL, 0, 0, NULL, NULL, NULL},
	{OPT_Switch, "HUD", Huds, 3, HUD_Full, &screen_hud, M_ChangeScreenHud, NULL},
	{OPT_Boolean, "Translucency", YesNo, 2, 1, &global_flags.trans, M_ChangeTransluc, NULL},
	{OPT_Switch, "Mipmapping", MipMaps, 3, 0, &use_mipmapping, M_ChangeMipMap, NULL},
	{OPT_Boolean, "Smoothing", YesNo, 2, 1, &use_smoothing, M_ChangeMipMap, NULL},
	{OPT_Boolean, "Shadows", YesNo, 2, 0, &global_flags.shadows, M_ChangeShadows, NULL},
	{OPT_Boolean, "Dynamic Lighting", YesNo, 2, 0, &use_dlights, M_ChangeDLights, NULL},
	{OPT_Switch, "Detail Level", Details, 3, 1, &detail_level, NULL, NULL},
	{OPT_Switch, "Crosshair", CrosO, 4, 0, &crosshair, NULL, NULL},
	{OPT_Boolean, "Map Overlay", YesNo, 2, 0, &map_overlay, NULL, NULL},
	{OPT_Boolean, "Map Rotation", YesNo, 2, 0, &rotatemap, NULL, NULL},
	{OPT_Switch, "Sky stretch", SkySq, 4, 1, &sky_stretch, M_ChangeMipMap, NULL},
	{OPT_Plain, "", NULL, 0, 0, NULL, NULL, NULL},
	{OPT_Switch, "Teleport Flash", YesNo, 2, 1, &telept_flash, NULL, NULL},
	{OPT_Switch, "Wipe method", WIPE_EnumStr, WIPE_NUMWIPES, 1, &wipe_method, NULL, NULL} 
  
#if 0  // TEMPORARILY DISABLED (we need an `Advanced Options' menu)
	{OPT_Switch, "Teleportation effect", WIPE_EnumStr, WIPE_NUMWIPES, 0, &telept_effect, NULL, NULL},
    {OPT_Switch, "Reverse effect", YesNo, 2, 0, &telept_reverse, NULL, NULL},
    {OPT_Switch, "Reversed wipe", YesNo, 2, 0, &wipe_reverse, NULL, NULL},
#endif
};

static menuinfo_t vidoptionsinfo = 
{
	vidoptions, sizeof(vidoptions) / sizeof(optmenuitem_t),
	150, 77, "M_VIDEO", NULL, 0, "", NULL, NULL
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
	150, 77, "M_VIDEO", NULL, 3, "", NULL, NULL
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
	{OPT_Boolean, "Invert Mouse", YesNo, 2, false, &invertmouse, NULL, NULL},
	{OPT_Switch, "Mouse X Axis", Axis, 6, AXIS_TURN, &mouse_xaxis, NULL, NULL},
	{OPT_Switch, "Mouse Y Axis", Axis, 6, AXIS_FORWARD, &mouse_yaxis, NULL, NULL},
	{OPT_Slider, "MouseSpeed", NULL, 20, 8, &mouseSensitivity, NULL, NULL},
	{OPT_Plain, "", NULL, 0, 0, NULL, NULL, NULL},
	{OPT_Slider, "MLook Speed", NULL, 20, 8, &mlookspeed, NULL, NULL},
	{OPT_Plain, "", NULL, 0, 0, NULL, NULL, NULL},
	{OPT_Boolean, "Two-Stage Turning", YesNo, 2, 0, &stageturn, NULL, NULL},
	{OPT_Slider, "Turning Speed", NULL, 9, 0, &angleturnspeed, NULL, NULL},
	{OPT_Slider, "Side Move Speed", NULL, 9, 0, &sidemovespeed, NULL, NULL},
	{OPT_Slider, "Forward Move Speed", NULL, 9, 0, &forwardmovespeed, NULL, NULL}

#if 0  // DISABLED, Because no joystick support yet
	{OPT_Plain, "", NULL, 0, 0, NULL, NULL, NULL},
	{OPT_Switch, "Joystick X Axis", Axis, 6, AXIS_TURN, &joy_xaxis, NULL, NULL},
	{OPT_Switch, "Joystick Y Axis", Axis, 6, AXIS_FORWARD, &joy_yaxis, NULL, NULL},
	{OPT_Function, "Calibrate Joystick", NULL, 0, 0, NULL, M_CalibrateJoystick, NULL}
#endif
};

static menuinfo_t analogueoptionsinfo = 
{
	analogueoptions, sizeof(analogueoptions) / sizeof(optmenuitem_t),
	150, 75, "M_MSETTL", NULL, 0, "", NULL, NULL
};

//
//  GAMEPLAY OPTIONS
//
// -ACB- 1998/07/15 Altered menu structure
// -KM- 1998/07/21 Change blood to switch
//
optmenuitem_t playoptions[] =
{
	{OPT_Switch, "Compatibility", CompatSet, 2, 0, &global_flags.compat_mode, M_ChangeCompatMode, NULL},
	{OPT_Switch, "AutoAiming", AAim, 3, 1, &global_flags.autoaim, M_ChangeAutoAim, NULL},
	{OPT_Boolean, "Jumping", YesNo, 2, 0, &global_flags.jump, M_ChangeJumping, NULL},
	{OPT_Boolean, "Crouching", YesNo, 2, 0, &global_flags.crouch, M_ChangeCrouching, NULL},
	{OPT_Boolean, "Weapon Kick", YesNo, 2, 1, &global_flags.kicking, M_ChangeKicking, NULL},
	{OPT_Boolean, "More Blood", YesNo, 2, 0, &global_flags.more_blood, M_ChangeBlood, "Blood"},
	{OPT_Boolean, "Extras", YesNo, 2, 1, &global_flags.have_extra, M_ChangeExtra, NULL},
	{OPT_Boolean, "True 3D Gameplay", YesNo, 2, 1, &global_flags.true3dgameplay, M_ChangeTrue3d, "True3d"},
	{OPT_Plain, "", NULL, 0, 0, NULL, NULL, NULL},
	{OPT_Slider, "Gravity", NULL, 20, 8, &global_flags.menu_grav, NULL, "Gravity"},
	{OPT_Plain, "", NULL, 0, 0, NULL, NULL, NULL},
	{OPT_Boolean, "Enemy Respawn Mode", Respw, 2, 0, &global_flags.res_respawn, M_ChangeMonsterRespawn, NULL},
	{OPT_Boolean, "Item Respawn", YesNo, 2, 0, &global_flags.itemrespawn, M_ChangeItemRespawn, NULL},
	{OPT_Boolean, "Fast Monsters", YesNo, 2, 0, &global_flags.fastparm, M_ChangeFastparm, NULL},
	{OPT_Boolean, "Respawn", YesNo, 2, 0, &global_flags.respawn, M_ChangeRespawn, NULL}
};

static menuinfo_t playoptionsinfo = 
{
	playoptions, sizeof(playoptions) / sizeof(optmenuitem_t),
	160, 46, "M_GAMEPL", NULL, 0, "", NULL, NULL
};

//
//  KEY CONFIG : STANDARD
//
// -ACB- 1998/07/15 Altered menuinfo struct
// -KM- 1998/07/10 Used better names :-)
//
static optmenuitem_t stdkeyconfig[] =
{
	{OPT_KeyConfig, "Primary Attack", NULL, 0, KEYD_RCTRL + (KEYD_MOUSE1 << 16), &key_fire, NULL, NULL},
    {OPT_KeyConfig, "Secondary Atk", NULL, 0, 'E', &key_secondatk, NULL, NULL},
	{OPT_KeyConfig, "Use Item", NULL, 0, ' ', &key_use, NULL, NULL},
	{OPT_KeyConfig, "Walk Forward", NULL, 0, KEYD_UPARROW, &key_up, NULL, NULL},
	{OPT_KeyConfig, "Walk Backwards", NULL, 0, KEYD_DOWNARROW, &key_down, NULL, NULL},
	{OPT_KeyConfig, "Turn Left", NULL, 0, KEYD_LEFTARROW, &key_left, NULL, NULL},
	{OPT_KeyConfig, "Turn Right", NULL, 0, KEYD_RIGHTARROW, &key_right, NULL, NULL},
	{OPT_KeyConfig, "Move Up", NULL, 0, KEYD_INSERT, &key_flyup, NULL, NULL},
	{OPT_KeyConfig, "Move Down", NULL, 0, KEYD_DELETE, &key_flydown, NULL, NULL},
	{OPT_KeyConfig, "Toggle Autorun", NULL, 0, KEYD_CAPSLOCK, &key_autorun, NULL, NULL},
	{OPT_KeyConfig, "Run", NULL, 0, KEYD_RSHIFT, &key_speed, NULL, NULL},
	{OPT_Plain, "", NULL, 0, 0, NULL, NULL, NULL},
	{OPT_KeyConfig, "Strafe Left", NULL, 0, ',', &key_strafeleft, NULL, NULL},
	{OPT_KeyConfig, "Strafe Right", NULL, 0, '.', &key_straferight, NULL, NULL},
	{OPT_KeyConfig, "Strafe", NULL, 0, KEYD_RALT + (KEYD_MOUSE2 << 16), &key_strafe, NULL, NULL}
};

static menuinfo_t stdkeyconfiginfo = 
{
	stdkeyconfig, sizeof(stdkeyconfig) / sizeof(optmenuitem_t),
	110, 98, "M_CONTRL", NULL, 0, 
	"STD", NULL, NULL
};

//
//  KEY CONFIG : EXTENDED
//
// -ACB- 1998/07/15 Altered menuinfo struct
// -ES- 1999/03/28 Added Zoom Key
//
static optmenuitem_t extkeyconfig[] =
{
	{OPT_KeyConfig, "Look Up", NULL, 0, KEYD_PGUP, &key_lookup, NULL, NULL},
	{OPT_KeyConfig, "Look Down", NULL, 0, KEYD_PGDN, &key_lookdown, NULL, NULL},
	{OPT_KeyConfig, "Center View", NULL, 0, KEYD_HOME, &key_lookcenter, NULL, NULL},
	{OPT_KeyConfig, "Zoom in/out", NULL, 0, '\\', &key_zoom, NULL, NULL},
	{OPT_KeyConfig, "Jump", NULL, 0, '/', &key_jump, NULL, NULL},
	{OPT_Plain, "", NULL, 0, 0, NULL, NULL, NULL},
	{OPT_KeyConfig, "180 degree turn", NULL, 0, 0, &key_180, NULL, NULL},
    {OPT_KeyConfig, "Manual Reload", NULL, 0, 0, &key_reload, NULL, NULL},
	{OPT_KeyConfig, "Mouse Look", NULL, 0, 0, &key_mlook, NULL, NULL},
	{OPT_KeyConfig, "Map Toggle", NULL, 0, KEYD_TAB, &key_map, NULL, NULL},
	{OPT_KeyConfig, "Multiplay Talk", NULL, 0, 't', &key_talk, NULL, NULL}
};

static menuinfo_t extkeyconfiginfo = 
{
	extkeyconfig, sizeof(extkeyconfig) / sizeof(optmenuitem_t),
	110, 98, "M_CONTRL", NULL, 0, 
	"EXT", NULL, NULL
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
    {KEYD_MOUSE4, "Mouse3"},
    {KEYD_MWHEEL_UP, "Wheel Up"},
    {KEYD_MWHEEL_DN, "Wheel Down"},
    {-1, ""}
};

//
// M_InitOptmenu
//
// Menu Initialisation
//
void M_InitOptmenu()
{
	optionsmenuon = 0;
	curr_menu = &mainmenuinfo;
	curr_item = curr_menu->items + curr_menu->pos;
	keyscan = 0;

	// Needed to handle the circular reference that C++ init doesn't allow
	stdkeyconfiginfo.sister_next = &extkeyconfiginfo;
	extkeyconfiginfo.sister_prev = &stdkeyconfiginfo;

	menumusicvol = S_GetMusicVolume(); // -ACB- 1999/11/13 Music API Volume
	menusoundvol = S_GetSfxVolume();   // -ACB- 1999/10/10 Sound API Volume

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
	int displaybpp = 0;

	if (setresfailed)
	{
		epi::string_c s;
		
		displaybpp = scrmode[selectedscrmode].depth;

		s.Format(language["ModeSelErr"],
				scrmode[selectedscrmode].width,
				scrmode[selectedscrmode].height,
				displaybpp);

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

	// make sure the local volume values are kept up-to-date
	menumusicvol = S_GetMusicVolume();
	menusoundvol = S_GetSfxVolume();
 
	// -ACB- 1998/06/15 Calculate height for menu and then center it.
	menutop = 68 - ((curr_menu->item_num * hu_font.height) / 2);

	{
		const image_t *image;

		if (! curr_menu->title_image)
			curr_menu->title_image = W_ImageFromPatch(curr_menu->title_name);

		image = curr_menu->title_image;

		RGL_ImageEasy320(curr_menu->title_x, menutop, image);
	}

	//now, draw all the menuitems
	deltay = 1 + hu_font.height;

	curry = menutop + 25;

	if (curr_menu->key_page[0])
	{
		if (curr_menu->sister_prev)
			HL_WriteTextTrans(60, 200-deltay*4, text_yellow_map, "< PREV");

		if (curr_menu->sister_next)
			HL_WriteTextTrans(260 - HL_StringWidth("NEXT >"), 200-deltay*4, 
							  text_yellow_map, "NEXT >");

		HL_WriteTextTrans(160 - HL_StringWidth(curr_menu->key_page)/2, 
						  200-deltay*4, text_yellow_map, curr_menu->key_page);
    
		if (keyscan)
			HL_WriteTextTrans(160 - (HL_StringWidth(keystring2) / 2), 
							  200-deltay*2, text_green_map, keystring2);
		else
			HL_WriteTextTrans(160 - (HL_StringWidth(keystring1) / 2), 
							  200-deltay*2, text_green_map, keystring1);
	}
	else if (curr_menu == &resoptionsinfo)
	{
		M_ResOptDrawer(curry, curry + (deltay * (resoptionsinfo.item_num - 2)), 
					   deltay, curr_menu->menu_center);
	}
	else if (curr_menu == &mainmenuinfo)
	{
		M_LanguageDrawer(curr_menu->menu_center, curry, deltay);
	}

	for (i = 0; i < curr_menu->item_num; i++)
	{
		HL_WriteText((curr_menu->menu_center) - HL_StringWidth(curr_menu->items[i].name),
					 curry, curr_menu->items[i].name);

		// -ACB- 1998/07/15 Menu Cursor is colour indexed.
		if (i == curr_menu->pos)
		{
			HL_WriteTextTrans((curr_menu->menu_center + 4), curry, 
							  text_yellow_map, "*");

			if (curr_menu->items[i].help)
			{
				const char *help = language[curr_menu->items[i].help];

				HL_WriteTextTrans(160 - (HL_StringWidth(help) / 2), 200 - deltay*2, 
								  text_green_map, help);
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

				HL_WriteTextTrans((curr_menu->menu_center) + 15, curry, OPTSHADE, tempstring);
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
				HL_WriteTextTrans((curr_menu->menu_center + 15), curry, OPTSHADE, tempstring);
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
static void M_ResOptDrawer(int topy, int bottomy, int dy, int centrex)
{
	char tempstring[80];
	int y;
	int displaybpp = 8;

	// Draw current resolution
	y = topy;
	sprintf(tempstring, "Current Resolution:");
	HL_WriteText(160 - (HL_StringWidth(tempstring) / 2), y, tempstring);

	y += dy;
	sprintf(tempstring, "%d x %d in %d-bit mode", SCREENWIDTH, SCREENHEIGHT,
			SCREENBITS);
	HL_WriteTextTrans(160 - (HL_StringWidth(tempstring) / 2), y, OPTSHADE, tempstring);

	// Draw resolution selection option
	y += (dy*2);
	sprintf(tempstring, "%dx%d", scrmode[selectedscrmode].width, scrmode[selectedscrmode].height);
	HL_WriteTextTrans(centrex+15, y, OPTSHADE, tempstring);

	// Draw depth selection option
	displaybpp = scrmode[selectedscrmode].depth;

	y += dy;
	sprintf(tempstring, "%d bit", displaybpp);
	HL_WriteTextTrans(centrex+15, y, OPTSHADE, tempstring);

	// Draw selected resolution and mode:
	y = bottomy;
	sprintf(tempstring, "Selected Resolution:");
	HL_WriteText(160 - (HL_StringWidth(tempstring) / 2), y, tempstring);

	y += dy;

	sprintf(tempstring, "%d x %d in %d-bit mode",
			scrmode[selectedscrmode].width,
			scrmode[selectedscrmode].height,
			displaybpp);

	HL_WriteTextTrans(160 - (HL_StringWidth(tempstring) / 2), y, OPTSHADE, tempstring);
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
	epi::string_c s;

	s.Format("%s", language.GetName());

	HL_WriteTextTrans(x+15, y + deltay * 6, OPTSHADE, s);
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

			S_StartSound(NULL, sfx_pstop);
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

			S_StartSound(NULL, sfx_pstop);
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

					S_StartSound(NULL, sfx_pstop);
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

					S_StartSound(NULL, sfx_pistol);

					if (curr_item->routine != NULL)
						curr_item->routine(ch);

					return true;
				}

				case OPT_Switch:
				{
					(*(int*)(curr_item->switchvar))--;

					if ((*(int*)(curr_item->switchvar)) < 0)
						(*(int*)(curr_item->switchvar)) = curr_item->numtypes - 1;

					S_StartSound(NULL, sfx_pistol);

					if (curr_item->routine != NULL)
						curr_item->routine(ch);

					return true;
				}

				case OPT_Function:
				{
					if (curr_item->routine != NULL)
						curr_item->routine(ch);

					S_StartSound(NULL, sfx_pistol);
					return true;
				}

				case OPT_Slider:
				{
					if ((*(int*)(curr_item->switchvar)) > 0)
					{
						(*(int*)(curr_item->switchvar))--;
						S_StartSound(NULL, sfx_stnmov);
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

					S_StartSound(NULL, sfx_pstop);
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

					S_StartSound(NULL, sfx_pistol);

					if (curr_item->routine != NULL)
						curr_item->routine(ch);

					return true;
				}

				case OPT_Switch:
				{
					(*(int*)(curr_item->switchvar))++;

					if ((*(int*)(curr_item->switchvar)) >= curr_item->numtypes)
						(*(int*)(curr_item->switchvar)) = 0;

					S_StartSound(NULL, sfx_pistol);

					if (curr_item->routine != NULL)
						curr_item->routine(ch);

					return true;
				}

				case OPT_Function:
				{
					if (curr_item->routine != NULL)
						curr_item->routine(ch);

					S_StartSound(NULL, sfx_pistol);
					return true;
				}

				case OPT_Slider:
				{
					if ((*(int*)(curr_item->switchvar)) < (curr_item->numtypes - 1))
					{
						(*(int*)(curr_item->switchvar))++;
						S_StartSound(NULL, sfx_stnmov);
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
				optionsmenuon = 0;
			}
			else
			{
				curr_menu = &mainmenuinfo;
				curr_item = curr_menu->items + curr_menu->pos;
			}
			S_StartSound(NULL, sfx_swtchx);
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
	int i;
	byte depth = SCREENBITS;
	screenmode_t curMode;

	// Get a depth mask for resolution selection
	DEV_ASSERT2(depth == 8 || depth == 16 || depth == 24);

	// Find the current mode in the scrmode[] table
	curMode.width = SCREENWIDTH;
	curMode.height = SCREENHEIGHT;
	curMode.depth = SCREENBITS;
	curMode.windowed = SCREENWINDOW;

	i = V_FindClosestResolution(&curMode, true, true);

	if (i == -1)
		I_Error("M_ResolutionOptions: Graphics mode not listed in scrmode[]");

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
// M_GameplayOptions
//
static void M_GameplayOptions(int keypressed)
{
	if (netgame)
		return;

	curr_menu = &playoptionsinfo;
	curr_item = curr_menu->items + curr_menu->pos;
}

//
// M_StandardControlOptions
//
static void M_StandardControlOptions(int keypressed)
{
	curr_menu = &stdkeyconfiginfo;
	curr_item = curr_menu->items + curr_menu->pos;
}

//
// M_ExtendedControlOptions
//
static void M_ExtendedControlOptions(int keypressed)
{
	curr_menu = &extkeyconfiginfo;
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

//
// M_CalibrateJoystick
//
// 1998/07/10 -KM- Recalibration of Joystick
// 2000/11/03 -ACB- Unable to use as non-portable
//
static void M_CalibrateJoystick(int keypressed)
{
//  I_CalibrateJoystick(0);
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

static void M_ChangeTransluc(int keypressed)
{
	if (currmap && ((currmap->force_on | currmap->force_off) & MPF_Translucency))
		return;

	level_flags.trans = global_flags.trans;
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

// this used by both MIPMIP and SMOOTHING options
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

static void M_ChangeHalos(int keypressed)
{
	if (currmap && ((currmap->force_on | currmap->force_off) & MPF_Halos))
		return;

	level_flags.halos = global_flags.halos;
}

static void M_ChangeCompatMode(int keypressed)
{
	if (currmap && ((currmap->force_on | currmap->force_off) & MPF_BoomCompat))
		return;

	level_flags.compat_mode = global_flags.compat_mode;

	// clear line/sector lookup caches
	DDF_BoomClearGenTypes();
}

static void M_ChangeKicking(int keypressed)
{
	if (currmap && ((currmap->force_on | currmap->force_off) & MPF_Kicking))
		return;

	level_flags.kicking = global_flags.kicking;
}

static void M_ChangeDLights(int keypressed)
{
	/* nothing to do -- change occurs at next level load */
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
	int i;

	if (keypressed == KEYD_LEFTARROW)
	{
		i=selectedscrmode-1;
		while (i != selectedscrmode)
		{
			// allow for rotation
			if (i < 0)
				i = (numscrmodes-1);

			// ignore different windowed-ness
			if (scrmode[i].windowed == scrmode[selectedscrmode].windowed &&
				scrmode[i].depth == scrmode[selectedscrmode].depth)
				break;

			i--;
		}
		selectedscrmode = i;
	}
	else if (keypressed == KEYD_RIGHTARROW)
	{
		i=selectedscrmode+1;
		while (i != selectedscrmode)
		{
			// allow for rotation
			if (i == numscrmodes)
				i = 0;

			// ignore different windowed-ness
			if (scrmode[i].windowed == scrmode[selectedscrmode].windowed &&
				scrmode[i].depth == scrmode[selectedscrmode].depth)
				break;

			i++;
		}
		selectedscrmode = i;
	}
}

//
// M_ChangeStoredBpp
//
// -ACB- 1998/08/29 Depth Changes...
//
static void M_ChangeStoredBpp(int keypressed)
{
	int newdepthbit;
	bool gotnewdepth;  // Got new resolution setting
	screenmode_t newMode;
	int idx;

	// Ignore anything by LEFT and RIGHT arrow keys
	if (keypressed != KEYD_LEFTARROW && keypressed != KEYD_RIGHTARROW)
		return;

	newdepthbit = scrmode[selectedscrmode].depth;

	gotnewdepth = false;
	while (!gotnewdepth)
	{
		if (keypressed == KEYD_LEFTARROW)
		{
			if (newdepthbit == 8)
				newdepthbit = 24;
			else if (newdepthbit == 24)
				newdepthbit = 16;
			else if (newdepthbit == 16)
				newdepthbit = 8;
		}
		else if (keypressed == KEYD_RIGHTARROW)
		{
			if (newdepthbit == 8)
				newdepthbit = 16;
			else if (newdepthbit == 16)
				newdepthbit = 24;
			else if (newdepthbit == 24)
				newdepthbit = 8;
		}
    
		newMode.width = scrmode[selectedscrmode].width;
		newMode.height = scrmode[selectedscrmode].height;
		newMode.depth = newdepthbit;
		newMode.windowed = SCREENWINDOW;

		idx = V_FindClosestResolution(&newMode, false, true);
     
		// Select res
		if (idx != -1)
		{ 
			selectedscrmode = idx;
			gotnewdepth = true;
		}
	}

	return;
}

//
// M_OptionSetResolution
//
static void M_OptionSetResolution(int keypressed)
{
	R_ChangeResolution(
		scrmode[selectedscrmode].width, 
		scrmode[selectedscrmode].height, 
		scrmode[selectedscrmode].depth, SCREENWINDOW);
}

//
// M_OptionTestResolution
//
static void M_OptionTestResolution(int keypressed)
{
	R_ChangeResolution(
		scrmode[selectedscrmode].width, 
		scrmode[selectedscrmode].height, 
		scrmode[selectedscrmode].depth, SCREENWINDOW);

	testticker = TICRATE * 3;
}

//
// M_RestoreResSettings
//
static void M_RestoreResSettings(int keypressed)
{
	R_ChangeResolution(
		scrmode[prevscrmode].width, 
		scrmode[prevscrmode].height,
		scrmode[prevscrmode].depth, SCREENWINDOW);
}

