//----------------------------------------------------------------------------
//  EDGE2 Main Menu and Options Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2016 Isotope SoftWorks
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
//
//
//
//

#include "i_defs.h"
#include "i_defs_gl.h"

#include "../epi/str_format.h"

#include "../ddf/main.h"

#include "con_main.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "dstrings.h"
#include "e_input.h"
#include "e_main.h"
#include "g_game.h"
#include "hu_draw.h"
#include "hu_stuff.h"
#include "hu_style.h"
#include "f_interm.h"
#include "hu_draw.h"
#include "hu_stuff.h"
#include "hu_style.h"
#include "m_argv.h"
#include "m_menu.h"
#include "m_misc.h"
#include "m_netgame.h"
/* #include "m_option.h" */
#include "m_random.h"
#include "n_network.h"
#include "p_local.h"
#include "p_setup.h"
#include "am_map.h"
#include "r_local.h"
#include "r_misc.h"
#include "r_gldefs.h"
#include "s_sound.h"
#include "s_music.h"
#include "s_timid.h"
#include "sv_chunk.h"
#include "sv_main.h"
#include "w_wad.h"
#include "z_zone.h"

#include "r_draw.h"
#include "r_modes.h"
#include "r_colormap.h"
#include "r_image.h"
#include "w_wad.h"
#include "r_wipe.h"

#include "defaults.h"


int option_menuon = 0;

static style_c *save_style;

//
// definitions
//

#define MENUSTRINGSIZE      32

#define SKULLXTEXTOFF       -24

#define TEXTLINEHEIGHT      18
#define MENUCOLORRED        D_RGBA(255, 0, 0, menualphacolor)
#define MENUCOLORWHITE        D_RGBA(255, 255, 255, menualphacolor)
#define MAXBRIGHTNESS        100

//
// fade-in/out stuff
//

void M_MenuFadeIn(void);
void M_MenuFadeOut(void);
void(*menufadefunc)(void) = NULL;

static char     MenuBindBuff[256];
static char     MenuBindMessage[256];
static bool 	MenuBindActive = false;
static bool 	showfullitemvalue[3] = { false, false, false};
static int      levelwarp = 0;
static bool 	wireframeon = false;
static bool 	lockmonstersmon = false;
static int      thermowait = 0;
static int      m_aspectRatio = 0;
static int      m_ScreenSize = 1;


static bool     alphaprevmenu = false;
static int          menualphacolor = 0xff;

// Show messages has default, 0 = off, 1 = on
int showMessages;

/// ========================== CVARS ==================

extern cvar_c m_menufadetime;
extern cvar_c m_menumouse;
extern cvar_c m_cursorscale;
extern cvar_c r_texturecombiner;
extern cvar_c m_language;
extern cvar_c r_crosshair;
extern cvar_c r_crosssize;
extern cvar_c r_lerp;
extern cvar_c r_gl2_path;
extern cvar_c r_md5scale;
extern cvar_c debug_fps;
extern cvar_c debug_pos;
extern cvar_c r_vsync;
extern cvar_c r_colorscale; /// find out what this means...
extern cvar_c r_texnonpowresize; 
extern cvar_c g_autoaim;
extern cvar_c r_wipemethod;
extern cvar_c g_jumping;

static int menu_crosshair;  // temp hack
static int menu_crosshair2;  /// love haxxx
static void M_ChangeLanguage(int keypressed);

int screen_hud;  // has default

static std::string msg_string;
static int msg_lastmenu;
static int msg_mode;

static std::string input_string;		

bool menuactive;

#define SKULLXOFF   -24
#define LINEHEIGHT   15  //!!!!

// timed message = no input from user
static bool msg_needsinput;

static void (* message_key_routine)(int response) = NULL;
static void (* message_input_routine)(const char *response) = NULL;

static int chosen_epi;

//
//  IMAGES USED (we are completely removing menu images.)
//
/* static const image_c *therm_l;
static const image_c *therm_m;
static const image_c *therm_r;
static const image_c *therm_o;

static const image_c *menu_loadg;
static const image_c *menu_saveg;
static const image_c *menu_svol;
static const image_c *menu_doom;
static const image_c *menu_newgame;
static const image_c *menu_skill;
static const image_c *menu_episode;
static const image_c *menu_skull[2];
static const image_c *menu_readthis[2];

static style_c *menu_def_style;
static style_c *main_menu_style;
static style_c *episode_style;
static style_c *skill_style;
static style_c *load_style;
static style_c *save_style;
static style_c *dialog_style;
static style_c *sound_vol_style; */

//
//  SAVE STUFF
//
#define SAVESTRINGSIZE 	24

#define SAVE_SLOTS  8
#define SAVE_PAGES  100  // more would be rather unwieldy

// -1 = no quicksave slot picked!
int quickSaveSlot;
int quickSavePage;

// 25-6-98 KM Lots of save games... :-)
int save_page = 0;
int save_slot = 0;

// we are going to be entering a savegame string
static int saveStringEnter;

// which char we're editing
static int saveCharIndex;

// old save description before edit
static char saveOldString[SAVESTRINGSIZE];

typedef struct slot_extra_info_s
{
	bool empty;
	bool corrupt;

	char desc[SAVESTRINGSIZE];
	char timestr[32];
  
	char mapname[10];
	char gamename[32];
  
	int skill;
	int netgame;
	bool has_view;
}
slot_extra_info_t;

static slot_extra_info_t ex_slots[SAVE_SLOTS];

// 98-7-10 KM New defines for slider left.
// Part of savegame changes.
#define SLIDERLEFT  -1
#define SLIDERRIGHT -2

static void DrawKeyword(int index, style_c *style, int y,
		const char *keyword, const char *value)
{
	int x = 120;

	// bool is_selected =
	    // (netgame_menuon == 1 && index == host_pos) ||
	    // (netgame_menuon == 2 && index == join_pos);

	//HL_WriteText(style,(index<0)?3:is_selected?2:0, x - 10 - style->fonts[0]->StringWidth(keyword), y, keyword);
	HL_WriteText(style,1, x + 10, y, value);

	// if (is_selected)
	// {
		// HL_WriteText(style,2, x - style->fonts[2]->StringWidth("*")/2, y, "*");
	// }
}

static bool newmenu = false;    // 20120323 villsa


//
// MENU TYPEDEFS
//

typedef struct
{
	// 0 = no cursor here, 1 = ok, 2 = arrows ok
	short status;

	char name[64];
	

  	// choice = menu item #.
  	// if status = 2, choice can be SLIDERLEFT or SLIDERRIGHT
	void (* select_func)(int choice); /// void (*routine)(int choice);
	

	// hotkey in menu
	char alpha_key;
}
menuitem_t;

typedef struct 
{
    cvar_c* mitem;
    float    mdefault;
} menudefault_t;

typedef struct 
{
    int item;
    float width;
    cvar_c *mitem;
} menuthermobar_t;

typedef struct menu_s
{
	short numitems; // # of menu items	 
	bool textonly;
	struct menu_s *prevMenu; // previous menu
	menuitem_t *menuitems; // menu items	

	void (* draw_func)(void); // draw routine, void (*routine)(void); in DOOM64
	char title[64];
    short x;
    short y;                  // x,y of menu
	short lastOn;
	bool           smallfont;          // draw text using small fonts
    menudefault_t       *defaultitems;      // pointer to default values for cvars
    short               numpageitems;       // number of items to display per page
    short               menupageoffset;
    float               scale;
    char                **hints;
    menuthermobar_t     *thermobars;
}
menu_t;


typedef struct {
    char    *name;
    char    *action;
} menuaction_t;


// menu item skull is on
short itemOn;
short           itemSelected;
short skullAnimCounter;
short whichSkull;

// current menudef
static menu_t* currentMenu;
static menu_t* nextmenu;

extern menu_t ControlMenuDef;

//------------------------------------------------------------------------
//
// PROTOTYPES
//
//------------------------------------------------------------------------

static void M_SetupNextMenu(menu_t * menudef);
void M_ClearMenus(void);

static void M_QuickSave(void);
static void M_QuickLoad(void);

static int M_StringWidth(const char *string);
static int M_StringHeight(const char *string);
static int M_BigStringWidth(const char *string);

static void M_DrawThermo(int x, int y, int thermWidth, float thermDot);
static void M_Return(int choice);
static void M_ReturnToOptions(int choice);

static void M_DrawSmbString(const char* text, menu_t* menu, int item);
static void M_DrawSaveGameFrontend(menu_t* def);
static void M_SetInputString(char* string, int len);
static void M_Scroll(menu_t* menu, bool up);
static bool M_SetThumbnail(int which);

void M_StartControlPanel(void);

//------------------------------------------------------------------------
//
// EPI PROTOTYPERS
//
//------------------------------------------------------------------------
/* static void M_DrawMainMenu(void);
static void M_DrawReadThis1(void);
static void M_DrawReadThis2(void);
static void M_DrawNewGame(void);
static void M_DrawEpisode(void);
static void M_DrawSound(void);
static void M_DrawLoad(void);
static void M_DrawSave(void);
static void M_DrawSaveLoadBorder(float x, float y, int len); */

//------------------------------------------------------------------------
//
// MAIN MENU
//
//------------------------------------------------------------------------
void M_NewGame(int choice);
void M_Options(int choice);
void M_LoadGame(int choice);
void M_QuitDOOM(int choice);
void M_QuitEDGE(int choice);

typedef enum
{
	newgame = 0,
	options,
	loadgame,
	savegame,
	readthis,
	quitdoom,
	main_end
}
main_e;

menuitem_t MainMenu[] =
{
    {1,"New Game",M_NewGame,'n'},
    {1,"Options",M_Options,'o'},
    {1,"Load Game",M_LoadGame,'l'},
    {1,"Quit Game",M_QuitEDGE,'q'},
};

menu_t MainDef = {
    main_end,
    false,
    NULL,
    MainMenu,
    NULL,
    ".",
    112,150,
    0,
    false,
    NULL,
    -1,
    0,
    1.0f,
    NULL,
    NULL
};
/* static menu_t MainDef =

{
	main_end,
	NULL,
	MainMenu,
	&main_menu_style,
	M_DrawMainMenu,
	97, 64,
//Hypertension	37, 100, //97, 64
	0
}; */

//------------------------------------------------------------------------
//
// IN GAME MENU
//
//------------------------------------------------------------------------
static void M_SaveGame(int choice);
void M_Features(int choice);
void M_QuitMainMenu(int choice);
void M_ConfirmRestart(int choice);

typedef enum {
    pause_options = 0,
    pause_mainmenu,
    pause_restartlevel,
    pause_features,
    pause_loadgame,
    pause_savegame,
    pause_quitdoom,
    pause_end
} pause_e;

menuitem_t PauseMenu[]= {
    {1,"Options",M_Options,'o'},
    {1,"Main Menu",M_QuitMainMenu,'m'},
    {1,"Restart Level",M_ConfirmRestart,'r'},
    {-3,"Features",M_Features,'f'},
    {1,"Load Game",M_LoadGame,'l'},
    {1,"Save Game",M_SaveGame,'s'},
    {1,"Quit Game",M_QuitDOOM,'q'},
};

menu_t PauseDef = {
    pause_end,
    false,
    NULL,
    PauseMenu,
    NULL,
    "Pause",
    112,80,
    0,
    false,
    NULL,
    -1,
    0,
    1.0f,
    NULL,
    NULL
};

//------------------------------------------------------------------------
//
// QUIT GAME PROMPT
//
//------------------------------------------------------------------------

void M_QuitGame(int choice);
void M_QuitGameBack(int choice);

typedef enum {
    quityes = 0,
    quitno,
    quitend
} quitprompt_e;


menuitem_t QuitGameMenu[]= {
    {1,"Yes",M_QuitGame,'y'},
    {1,"No",M_QuitGameBack,'n'},
};

menu_t QuitDef = {
    quitend,
    false,
    &PauseDef,
    QuitGameMenu,
    NULL,
    "Quit DOOM?",
    144,112,
    quitno,
    false,
    NULL,
    -1,
    0,
    1.0f,
    NULL,
    NULL
};

void M_QuitDOOM(int choice) 
{
    M_SetupNextMenu(&QuitDef);
}

void M_QuitGame(int choice) 
{
    M_QuitEDGE;
}

void M_QuitGameBack(int choice) 
{
    M_SetupNextMenu(currentMenu->prevMenu);
}


//------------------------------------------------------------------------
//
// QUIT GAME PROMPT (From Main Menu)
//
//------------------------------------------------------------------------

void M_QuitGame2(int choice);
void M_QuitGameBack2(int choice);

typedef enum {
    quit2yes = 0,
    quit2no,
    quit2end
} quit2prompt_e;

menuitem_t QuitGameMenu2[]= {
    {1,"Yes",M_QuitGame2,'y'},
    {1,"No",M_QuitGameBack2,'n'},
};

menu_t QuitDef2 = {
    quit2end,
    false,
    &MainDef,
    QuitGameMenu2,
    NULL,
    "Quit DOOM?",
    144,112,
    quit2no,
    false,
    NULL,
    -1,
    0,
    1.0f,
    NULL,
    NULL
};

void M_QuitDOOM2(int choice) {
    M_SetupNextMenu(&QuitDef2);
}

void M_QuitGame2(int choice) {
    M_QuitEDGE(0);
}

void M_QuitGameBack2(int choice) {
    M_SetupNextMenu(currentMenu->prevMenu);
}

//------------------------------------------------------------------------
//
// EXIT TO MAIN MENU PROMPT
//
//------------------------------------------------------------------------

void M_EndGame(int choice);

typedef enum {
    PMainYes = 0,
    PMainNo,
    PMain_end
} prompt_e;

menuitem_t PromptMain[]= {
    {1,"Yes",M_EndGame,'y'},
    {1,"No",M_ReturnToOptions,'n'},
};

menu_t PromptMainDef = {
    PMain_end,
    false,
    &PauseDef,
    PromptMain,
    NULL,
    "Quit To Main Menu?",
    144,112,
    PMainNo,
    false,
    NULL,
    -1,
    0,
    1.0f,
    NULL,
    NULL
};

void M_QuitMainMenu(int choice) {
    M_SetupNextMenu(&PromptMainDef);
}

//------------------------------------------------------------------------
//
// RESTART LEVEL PROMPT
//
//------------------------------------------------------------------------

void M_RestartLevel(int choice);

typedef enum {
    RMainYes = 0,
    RMainNo,
    RMain_end
} rlprompt_e;

menuitem_t RestartConfirmMain[]= {
    {1,"Yes",M_RestartLevel,'y'},
    {1,"No",M_ReturnToOptions,'n'},
};

menu_t RestartDef = {
    RMain_end,
    false,
    &PauseDef,
    RestartConfirmMain,
    NULL,
    "Quit Current Game?",
    144,112,
    RMainNo,
    false,
    NULL,
    -1,
    0,
    1.0f,
    NULL,
    NULL
};

void M_ConfirmRestart(int choice) {
    M_SetupNextMenu(&RestartDef);
}

void M_RestartLevel(int choice) 
{
/*     if(!netgame) 
	{
        gameaction = ga_loadlevel;
        nextmap = gamemap;
        p->playerstate = PST_REBORN;
    }

    currentMenu->lastOn = itemOn;
    M_ClearMenus(); */
}

//------------------------------------------------------------------------
//
// START NEW IN NETGAME NOTIFY
//
//------------------------------------------------------------------------

void M_DrawStartNewNotify(void);
void M_NewGameNotifyResponse(int choice);

typedef enum 
{
    SNN_Ok = 0,
    SNN_End
} startnewnotify_e;

menuitem_t StartNewNotify[]= 
{
    {1,"Ok",M_NewGameNotifyResponse,'o'}
};

menu_t StartNewNotifyDef = {
    SNN_End,
    false,
    &PauseDef,
    StartNewNotify,
    M_DrawStartNewNotify,
    " ",
    144,112,
    SNN_Ok,
    false,
    NULL,
    -1,
    0,
    1.0f,
    NULL,
    NULL
};

void M_NewGameNotifyResponse(int choice) 
{
    M_SetupNextMenu(&MainDef);
}

void M_DrawStartNewNotify(void) 
{
    Draw_BigText(-1, 16, MENUCOLORRED , "You Cannot Start");
    Draw_BigText(-1, 32, MENUCOLORRED , "A New Game On A Network");
}

//------------------------------------------------------------------------
//
// NEW GAME MENU
//
//------------------------------------------------------------------------

void M_ChooseSkill(int choice);

typedef enum {
    killthings,
    toorough,
    hurtme,
    violence,
    nightmare,
    newg_end
} newgame_e;

menuitem_t NewGameMenu[]= {
    {1,"Be Gentle!",M_ChooseSkill, 'b'},
    {1,"Bring It On!",M_ChooseSkill, 'r'},
    {1,"Hurt Me Plenty!",M_ChooseSkill, 'i'},
    {1,"Ultra-Violence!",M_ChooseSkill, 'w'},
    {-3,"NIGHTMARE!!",M_ChooseSkill, 'h'},
};

menu_t NewDef = {
    newg_end,
    false,
    &MainDef,
    NewGameMenu,
    NULL,
    "Choose Your Skill...",
    112,80,
    toorough,
    false,
    NULL,
    -1,
    0,
    1.0f,
    NULL,
    NULL
};

//------------------------------------------------------------------------
//
// OPTIONS MENU
//
//------------------------------------------------------------------------

void M_DrawOptions(void);
void M_Controls(int choice);
void M_Sound(int choice);
void M_Display(int choice);
void M_Video(int choice);
void M_Misc(int choice);
void M_Password(int choice);
void M_Network(int choice);
/* void M_Region(int choice); */

typedef enum 
{
    options_controls,
    options_misc,
    options_soundvol,
    options_display,
    options_video,
    options_password,
    options_network,
    options_region,
    options_return,
    opt_end
} options_e;

menuitem_t OptionsMenu[]= {
    {1,"Controls",M_Controls, 'c'},
    {1,"Gameplay Options",M_Misc, 'e'},
    {1,"Sound",M_Sound,'s'},
    {1,"Display",M_Display, 'd'},
    {1,"Video",M_Video, 'v'},
    ///{1,"Language",M_ChangeLanguage, 'l'}, //M_Password, 'p'
    {1,"Network",M_Network, 'n'},
    {1,"Unused",NULL, 'f'},
    {1,"/r Return",M_Return, 0x20}
};

char* OptionHints[opt_end]= 
{
    "control configuration",
    "miscellaneous options for gameplay and other features",
    "setup sound and music options",
    "settings for screen display",
    "configure video-specific options",
    ///"set the default language",
    "setup options for a network game",
    "Unused",
    NULL
};

menu_t OptionsDef = {
    opt_end,
    false,
    &PauseDef,
    OptionsMenu,
    M_DrawOptions,
    "Options",
    170,80,
    0,
    false,
    NULL,
    -1,
    0,
    0.75f,
    OptionHints,
    NULL
};

void M_Options(int choice) {
    M_SetupNextMenu(&OptionsDef);
}

void M_DrawOptions(void) {
    if(OptionsDef.hints[itemOn] != NULL) {
        GL_SetOrthoScale(0.5f);
        Draw_BigText(-1, 410, MENUCOLORWHITE, OptionsDef.hints[itemOn]);
        GL_SetOrthoScale(OptionsDef.scale);
    }
}

//------------------------------------------------------------------------
//
// MENU OPTIONS:: OPTION STRUCTURES
//
//------------------------------------------------------------------------

void M_MiscChoice(int choice);
void M_DrawMisc(void);


/* CVAR_EXTERNAL(am_showkeymarkers);
CVAR_EXTERNAL(am_showkeycolors);
CVAR_EXTERNAL(am_drawobjects);
CVAR_EXTERNAL(am_overlay);
CVAR_EXTERNAL(r_skybox);
CVAR_EXTERNAL(r_texnonpowresize);
CVAR_EXTERNAL(i_interpolateframes);
CVAR_EXTERNAL(p_usecontext);
CVAR_EXTERNAL(compat_collision);
CVAR_EXTERNAL(compat_limitpain);
CVAR_EXTERNAL(compat_mobjpass);
CVAR_EXTERNAL(compat_grabitems);
CVAR_EXTERNAL(r_wipe);
CVAR_EXTERNAL(r_rendersprites);
CVAR_EXTERNAL(r_texturecombiner);
CVAR_EXTERNAL(r_colorscale); */

typedef enum {
    misc_header1, /// "MENU OPTIONS"
    misc_menufade,
    misc_empty1, /// NULL
    misc_menumouse,
    misc_cursorsize,
    misc_empty2, /// NULL
    misc_header2, ///GAMEPLAY OPTIONS
    misc_aim,
	misc_mouse,
    misc_jump,
	misc_crouch,
	misc_weaponkick,
	misc_weaponautoswitch,
	misc_obituary,
	misc_moreblood,
	misc_extras,
	misc_true3d,
	misc_pass_missile,
	misc_gravity, ///needs to be a scaled slider
	misc_enemyrespawn,
	misc_itemrespawn,
	misc_fastmonsters,
	misc_respawn,
    misc_context, ///use context...?
    misc_header3, /// Rendering Stuff (?)
    misc_wipe_method,
    misc_texresize, /// interesting?
    misc_frame,
    misc_combine,
    misc_sprites,
    misc_skybox,
    misc_rgbscale,
/*     misc_header4, /// AUTOMAP OPTIONS
    misc_showkey,
    misc_showlocks,
    misc_amobjects,
    misc_amoverlay,
    misc_header5, /// VANILLA COMPATIBILITY FLAGS
    misc_comp_collision,
    misc_comp_pain,
    misc_comp_pass,
    misc_comp_grab, */
    misc_default,
    misc_return,
    misc_end
} misc_e;

static menuitem_t MiscMenu[]= {
    {-1,"Menu Options",0 },
    {3,"Menu Fade Speed",M_MiscChoice, 'm' },
    {-1,"",0 },
    {2,"Show Cursor:",M_MiscChoice, 'h'},
    {3,"Cursor Scale:",M_MiscChoice,'u'},
    {-1,"",0 },
    {-1,"Gameplay Options",0 }, /// HEADER 2
    {2,"Auto Aim:",M_MiscChoice, 'a'},
    {2,"Jumping:",M_MiscChoice, 'j'},
	{2,"Crouching:",M_MiscChoice },
	{2,"Weapon Kick:",M_MiscChoice },
	{2,"Weapon AutoSwitch:",M_MiscChoice },
	{2,"Obituary Messages:",M_MiscChoice },
	{2,"More Blood:",M_MiscChoice },
	{2,"Extras:",M_MiscChoice },
	{2,"True 3D Gameplay:",M_MiscChoice },
	{2,"Shoot-thru Scenery:",M_MiscChoice },
	{2,"Gravity:",M_MiscChoice },
	{2,"Enemy Respawn Mode:",M_MiscChoice },
	{2,"Item Respawn:",M_MiscChoice },
	{2,"Fast Monsters:",M_MiscChoice },
	{2,"Respawn:",M_MiscChoice },
    {2,"Use Context:",M_MiscChoice, 'u'},
    {-1,"Rendering",0 },
    {11,"Screen Melt:",M_MiscChoice, 's' }, ///11 possibile values, with fade being default
    {2,"Texture Fit:",M_MiscChoice,'t' },
    {2,"Interpolation:",M_MiscChoice, 'f' },
    {2,"Use Combiners:",M_MiscChoice, 'c' },
    {2,"Sprite Pitch:",M_MiscChoice,'p'},
    {2,"Skybox:",M_MiscChoice,'k'},
    {2,"Color Scale:",M_MiscChoice,'o'},
/*     {-1,"Automap",0 },
    {2,"Key Pickups:",M_MiscChoice },
    {2,"Locked Doors:",M_MiscChoice },
    {2,"Draw Objects:",M_MiscChoice },
    {2,"Overlay:",M_MiscChoice },
    {-1,"N64 Compatibility",0 },
    {2,"Collision:",M_MiscChoice,'c' },
    {2,"Limit Lost Souls:",M_MiscChoice,'l'},
    {2,"Tall Actors:",M_MiscChoice,'i'},
    {2,"Grab High Items:",M_MiscChoice,'g'}, */
    {-2,"Default",M_ResetDefaults,'d'},
    {1,"/r Return",M_Return, 0x20}
};

char* MiscHints[misc_end]= {
    NULL,
    "change transition speeds between switching menus",
    NULL,
    "enable menu cursor",
    "set the size of the mouse cursor",
    NULL,
    NULL,
    "toggle classic style auto-aiming",
    "toggle the ability to jump",
	"toggle the ability to crouch",
	"if enabled, screen will give weapon kick feedback",
	"if enabled, weapons will auto-switch",
	"toggle obituary messages upon death",
	"if enabled, extra blood will be spawned",
	"toggle Extras, such as rain (anything with EXTRA flag)",
	"enable true-3D gameplay (3DGE feature)",
	"if enabled, this will let shots go through scenery",
	"set global gravity scale",
	"set enemy respawn mode",
	"set item respawn",
	"toggle Fast Monsters, like in Nightmare Mode",
	"respawn toggle",
    "if enabled interactive objects will highlight when near",
    NULL,
    "enable the melt effect when completing a level",
    "set how texture dimentions are stretched",
    "interpolate between frames to achieve smooth framerate",
    "use texture combining - not supported by low-end cards",
    "toggles billboard sprite rendering",
    "toggle skies to render either normally or as skyboxes",
    "scales the overall color RGB",
    /* NULL,
    "display key pickups in automap",
    "colorize locked doors accordingly to the key in automap",
    "set how objects are rendered in automap",
    "render the automap into the player hud",
    NULL,
    "surrounding blockmaps are not checked for an object",
    "limit max amount of lost souls spawned by pain elemental to 17",
    "emulate infinite height bug for all solid actors",
    "be able to grab high items by bumping into the sector it sits on", */
    NULL,
    NULL
};

//set 3DGE CVARS HERE. . . . . . . . . . .
menudefault_t MiscDefault[] = 
{
    { &m_menufadetime, 0 },
    { &m_menumouse, 1 },
    { &m_cursorscale, 8 },
    ///{ &g_autoaim, 1 }, ///p_autoaim
    { &g_jumping, 0 }, ///p_allowjump
    { NULL, 0 }, ///use_context?
    { &r_wipemethod, 1 }, ///r_wipe
    { &r_texnonpowresize, 0 }, ///define r_texnonpowresize. . .
    { &r_lerp, 0 }, ///i_interpolateframes
    { &r_texturecombiner, 1 }, ///define r_texturecombiner(!)
    { NULL, 1 }, ///I seem to remember some sort of weird sprite thingy...can't think of it anymore... D64: &r_rendersprites
    { NULL, 0 }, ///um...don't need this really. . .
    { &r_colorscale, 0 }, ///colorscale means...what exactly?
    /* { &am_showkeymarkers, 0 },
    { &am_showkeycolors, 0 },
    { &am_drawobjects, 0 },
    { &am_overlay, 0 },
    { &compat_collision, 1 },
    { &compat_limitpain, 1 },
    { &compat_mobjpass, 1 },
    { &compat_grabitems, 1 }, */
    { NULL, -1 }
};

menuthermobar_t SetupBars[] = {
    { misc_empty1, 80, &m_menufadetime },
    { misc_empty2, 50, &m_cursorscale },
    { -1, 0 }
};

menu_t MiscDef = {
    misc_end,
    false,
    &OptionsDef,
    MiscMenu,
    M_DrawMisc,
    "Setup",
    216,108,
    0,
    false,
    MiscDefault,
    14,
    0,
    0.5f,
    MiscHints,
    SetupBars
};

void M_Misc(int choice) 
{
    M_SetupNextMenu(&MiscDef);
}

void M_MiscChoice(int choice) 
{
    switch(itemOn) 
	
	{
		
    case misc_menufade:
	
        if(choice) 
			
		{
			
            if(m_menufadetime.d < 80.0f) 
				
			{
				
                (m_menufadetime.d + 0.8f);
				
            }
			
            else 
				
			{
                ///(m_menufadetime.f, 80);
            }
			
        }
        else 
		{
            if(m_menufadetime.d > 0.0f) 
			{
                (m_menufadetime.d - 0.8f);
            }
            else 
			{
                ///CON_CvarSetValue(m_menufadetime.name, 0);
            }
        }
        break;

    case misc_cursorsize:
        if(choice) 
		{
            if(m_cursorscale.d < 50) 
			{
                (m_cursorscale.d + 0.5f);
            }
            else {
                ///CON_CvarSetValue(m_cursorscale.name, 50);
            }
        }
        else 
		{
            if(m_cursorscale.d > 0.0f) 
			{
                (m_cursorscale.d - 0.5f);
            }
            else {
                ///CON_CvarSetValue(m_cursorscale.name, 0);
            }
        }
        break;

    case misc_menumouse:
		/// M_SetOptionValue(int choice, cvar_link_t *name, const char *flags, const char *value);
		/// extern bool CON_SetVar(const char *name, const char *flags, const char *value);
        M_SetOptionValue(choice, 0, 1, 1, &m_menumouse);
        break;

    case misc_aim:
        M_SetOptionValue(choice, 0, 1, 1, &p_autoaim);
        break;

    case misc_jump:
        M_SetOptionValue(choice, 0, 1, 1, &p_allowjump);
        break;

    case misc_context:
        M_SetOptionValue(choice, 0, 1, 1, &p_usecontext);
        break;

    case misc_wipe:
        M_SetOptionValue(choice, 0, 1, 1, &r_wipe);
        break;

    case misc_texresize:
        M_SetOptionValue(choice, 0, 2, 1, &r_texnonpowresize);
        break;

    case misc_frame:
        M_SetOptionValue(choice, 0, 1, 1, &i_interpolateframes);
        break;

    case misc_combine:
        M_SetOptionValue(choice, 0, 1, 1, &r_texturecombiner);
        break;

    case misc_sprites:
        M_SetOptionValue(choice, 1, 2, 1, &r_rendersprites);
        break;

    case misc_skybox:
        M_SetOptionValue(choice, 0, 1, 1, &r_skybox);
        break;

    case misc_rgbscale:
        M_SetOptionValue(choice, 0, 2, 1, &r_colorscale);
        break;

    case misc_showkey:
        M_SetOptionValue(choice, 0, 1, 1, &am_showkeymarkers);
        break;

    case misc_showlocks:
        M_SetOptionValue(choice, 0, 1, 1, &am_showkeycolors);
        break;

    case misc_amobjects:
        M_SetOptionValue(choice, 0, 2, 1, &am_drawobjects);
        break;

    case misc_amoverlay:
        M_SetOptionValue(choice, 0, 1, 1, &am_overlay);
        break;

    case misc_comp_collision:
        M_SetOptionValue(choice, 0, 1, 1, &compat_collision);
        break;

    case misc_comp_pain:
        M_SetOptionValue(choice, 0, 1, 1, &compat_limitpain);
        break;

    case misc_comp_pass:
        M_SetOptionValue(choice, 0, 1, 1, &compat_mobjpass);
        break;

    case misc_comp_grab:
        M_SetOptionValue(choice, 0, 1, 1, &compat_grabitems);
        break;
    }
}

void M_DrawMisc(void) 
{
    static const char* frametype[2] = { "Capped", "Smooth" };
    static const char* mapdisplaytype[2] = { "Hide", "Show" };
    static const char* objectdrawtype[3] = { "Arrows", "Sprites", "Both" };
    static const char* texresizetype[3] = { "Auto", "Padded", "Scaled" };
    static const char* rgbscaletype[3] = { "1x", "2x", "4x" };
    int y;

    if(currentMenu->menupageoffset <= misc_menufade+1 &&
            (misc_menufade+1) - currentMenu->menupageoffset < currentMenu->numpageitems) {
        y = misc_menufade - currentMenu->menupageoffset;
        M_DrawThermo(MiscDef.x,MiscDef.y+LINEHEIGHT*(y+1), 80, m_menufadetime.d);
    }

    if(currentMenu->menupageoffset <= misc_cursorsize+1 &&
            (misc_cursorsize+1) - currentMenu->menupageoffset < currentMenu->numpageitems) {
        y = misc_cursorsize - currentMenu->menupageoffset;
        M_DrawThermo(MiscDef.x,MiscDef.y+LINEHEIGHT*(y+1), 50, m_cursorscale.d);
    }

#define DRAWMISCITEM(a, b, c) \
    if(currentMenu->menupageoffset <= a && \
        a - currentMenu->menupageoffset < currentMenu->numpageitems) \
    { \
        y = a - currentMenu->menupageoffset; \
        Draw_BigText(MiscDef.x + 176, MiscDef.y+LINEHEIGHT*y, MENUCOLORRED, \
            c[(int)b]); \
    }

    DRAWMISCITEM(misc_menumouse, m_menumouse.value, msgNames);
    DRAWMISCITEM(misc_aim, p_autoaim.value, msgNames);
    DRAWMISCITEM(misc_jump, p_allowjump.value, msgNames);
    DRAWMISCITEM(misc_context, p_usecontext.value, mapdisplaytype);
    DRAWMISCITEM(misc_wipe, r_wipe.value, msgNames);
    DRAWMISCITEM(misc_texresize, r_texnonpowresize.value, texresizetype);
    DRAWMISCITEM(misc_frame, i_interpolateframes.value, frametype);
    DRAWMISCITEM(misc_combine, r_texturecombiner.value, msgNames);
    DRAWMISCITEM(misc_sprites, r_rendersprites.value - 1, msgNames);
    DRAWMISCITEM(misc_skybox, r_skybox.value, msgNames);
    DRAWMISCITEM(misc_rgbscale, r_colorscale.value, rgbscaletype);
    DRAWMISCITEM(misc_showkey, am_showkeymarkers.value, mapdisplaytype);
    DRAWMISCITEM(misc_showlocks, am_showkeycolors.value, mapdisplaytype);
    DRAWMISCITEM(misc_amobjects, am_drawobjects.value, objectdrawtype);
    DRAWMISCITEM(misc_amoverlay, am_overlay.value, msgNames);
    DRAWMISCITEM(misc_comp_collision, compat_collision.value, msgNames);
    DRAWMISCITEM(misc_comp_pain, compat_limitpain.value, msgNames);
    DRAWMISCITEM(misc_comp_pass, !compat_mobjpass.value, msgNames);
    DRAWMISCITEM(misc_comp_grab, compat_grabitems.value, msgNames);

#undef DRAWMISCITEM

    if(MiscDef.hints[itemOn] != NULL) 
	{
        GL_SetOrthoScale(0.5f);
        Draw_BigText(-1, 410, MENUCOLORWHITE, MiscDef.hints[itemOn]);
        GL_SetOrthoScale(MiscDef.scale);
    }
}

static int M_GetCurrentSwitchValue(menuitem_t *item)
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

/// M_ReadSaveStrings();

//
// Read the strings from the savegame files
//
// 98-7-10 KM Savegame slots increased
//
void M_ReadSaveStrings(void)
{
	int i, version;
  
	saveglobals_t *globs;

	for (i=0; i < SAVE_SLOTS; i++)
	{
		ex_slots[i].empty = false;
		ex_slots[i].corrupt = true;

		ex_slots[i].skill = -1;
		ex_slots[i].netgame = -1;
		ex_slots[i].has_view = false;

		ex_slots[i].desc[0] = 0;
		ex_slots[i].timestr[0] = 0;
		ex_slots[i].mapname[0] = 0;
		ex_slots[i].gamename[0] = 0;
    
		int slot = save_page * SAVE_SLOTS + i;
		std::string fn(SV_FileName(SV_SlotName(slot), "head"));

		if (! SV_OpenReadFile(fn.c_str()))
		{
			ex_slots[i].empty = true;
			ex_slots[i].corrupt = false;
			continue;
		}

		if (! SV_VerifyHeader(&version))
		{
			SV_CloseReadFile();
			continue;
		}

		globs = SV_LoadGLOB();

		// close file now -- we only need the globals
		SV_CloseReadFile();

		if (! globs)
			continue;

		// --- pull info from global structure ---

		if (!globs->game || !globs->level || !globs->description)
		{
			SV_FreeGLOB(globs);
			continue;
		}

		ex_slots[i].corrupt = false;

		Z_StrNCpy(ex_slots[i].gamename, globs->game,  32-1);
		Z_StrNCpy(ex_slots[i].mapname,  globs->level, 10-1);

		Z_StrNCpy(ex_slots[i].desc, globs->description, SAVESTRINGSIZE-1);

		if (globs->desc_date)
			Z_StrNCpy(ex_slots[i].timestr, globs->desc_date, 32-1);

		ex_slots[i].skill   = globs->skill;
		ex_slots[i].netgame = globs->netgame;

		SV_FreeGLOB(globs);
    
#if 0
		// handle screenshot
		if (globs->view_pixels)
		{
			int x, y;
      
			for (y=0; y < 100; y++)
				for (x=0; x < 160; x++)
				{
					save_screenshot[x][y] = SV_GetShort();
				}
		}
#endif
	}

	// fix up descriptions
	for (i=0; i < SAVE_SLOTS; i++)
	{
		if (ex_slots[i].corrupt)
		{
			strncpy(ex_slots[i].desc, language["Corrupt_Slot"],
					SAVESTRINGSIZE - 1);
			continue;
		}
		else if (ex_slots[i].empty)
		{
			strncpy(ex_slots[i].desc, language["EmptySlot"],
					SAVESTRINGSIZE - 1);
			continue;
		}
	}
}


//
// User wants to load this game
//
// 98-7-10 KM Savegame slots increased
//
/* void M_LoadSelect(int choice)
{
	if (choice < 0)
	{
		M_LoadSavePage(choice);
		return;
	}

	
} */
///G_DeferredLoadGame(save_page * SAVE_SLOTS + choice);
	///M_ClearMenus();
///

//
// Selected from DOOM menu
//
/* void M_LoadGame(int choice)
{
	if (netgame)
	{
		M_StartMessage(language["NoLoadInNetGame"], NULL, false);
		return;
	}

	M_SetupNextMenu(&LoadDef);
	M_ReadSaveStrings();
} */

//
// 98-7-10 KM Savegame slots increased
//
/* void M_DrawSave(void)
{
	int i, len;

	HUD_DrawImage(72, 8, menu_saveg);

	for (i = 0; i < SAVE_SLOTS; i++)
	{
		int y = LoadDef.y + LINEHEIGHT * i;

		M_DrawSaveLoadBorder(LoadDef.x + 8, y, 24);

		if (saveStringEnter && i == save_slot)
		{
			len = save_style->fonts[1]->StringWidth(ex_slots[save_slot].desc);

			HL_WriteText(save_style,1, LoadDef.x + 8, y, ex_slots[i].desc);
			HL_WriteText(save_style,1, LoadDef.x + len + 8, y, "_");
		}
		else
			HL_WriteText(save_style,0, LoadDef.x + 8, y, ex_slots[i].desc);
	}

	M_DrawSaveLoadCommon(i, i+1, save_style);
} */

//
// M_Responder calls this when user is finished
//
// 98-7-10 KM Savegame slots increased
//
/* static void M_DoSave(int page, int slot)
{
	G_DeferredSaveGame(page * SAVE_SLOTS + slot, ex_slots[slot].desc);
	M_ClearMenus();

	// PICK QUICKSAVE SLOT YET?
	if (quickSaveSlot == -2)
	{
		quickSavePage = page;
		quickSaveSlot = slot;
	}

	LoadDef.lastOn = SaveDef.lastOn;
} */

//
// User wants to save. Start string input for M_Responder
//
/* void M_SaveSelect(int choice)
{
	if (choice < 0)
	{
		M_LoadSavePage(choice);
		return;
	}

	// we are going to be intercepting all chars
	saveStringEnter = 1;

	save_slot = choice;
	strcpy(saveOldString, ex_slots[choice].desc);

	if (ex_slots[choice].empty)
		ex_slots[choice].desc[0] = 0;

	saveCharIndex = strlen(ex_slots[choice].desc);
} */

//
// Selected from DOOM menu
//
/* void M_SaveGame(int choice)
{
	if (gamestate != GS_LEVEL)
	{
		M_StartMessage(language["SaveWhenNotPlaying"], NULL, false);
		return;
	}

	// -AJA- big cop-out here (add RTS menu stuff to savegame ?)
	if (rts_menuactive)
	{
		M_StartMessage("You can't save during an RTS menu.\n\npress a key.", NULL, false);
		return;
	}

	M_ReadSaveStrings();
	M_SetupNextMenu(&SaveDef);

	need_save_screenshot = true;
	save_screenshot_valid = false;
} */

//
//   M_QuickSave
//

/* static void QuickSaveResponse(int ch)
{
	if (ch == 'y')
	{
		M_DoSave(quickSavePage, quickSaveSlot);
		S_StartFX(sfx_swtchx);
	}
} */

/* void M_QuickSave(void)
{
	if (gamestate != GS_LEVEL)
	{
		S_StartFX(sfx_oof);
		return;
	}

	if (quickSaveSlot < 0)
	{
		M_StartControlPanel();
		M_ReadSaveStrings();
		M_SetupNextMenu(&SaveDef);

		need_save_screenshot = true;
		save_screenshot_valid = false;

		quickSaveSlot = -2;  // means to pick a slot now
		return;
	}
	
	std::string s(epi::STR_Format(language["QuickSaveOver"],
				  ex_slots[quickSaveSlot].desc));

	M_StartMessage(s.c_str(), QuickSaveResponse, true);
} */

/* static void QuickLoadResponse(int ch)
{
	if (ch == 'y')
	{
		int tempsavepage = save_page;

		save_page = quickSavePage;
		M_LoadSelect(quickSaveSlot);

		save_page = tempsavepage;
		S_StartFX(sfx_swtchx);
	}
} */

/* void M_QuickLoad(void)
{
	if (netgame)
	{
		M_StartMessage(language["NoQLoadInNet"], NULL, false);
		return;
	}

	if (quickSaveSlot < 0)
	{
		M_StartMessage(language["NoQuickSaveSlot"], NULL, false);
		return;
	}

	std::string s(epi::STR_Format(language["QuickLoad"],
					ex_slots[quickSaveSlot].desc));

	M_StartMessage(s.c_str(), QuickLoadResponse, true);
} */


#if 0
void M_Sound(int choice)
{
	M_SetupNextMenu(&SoundDef);
}
#endif


/* void M_NewGame(int choice)
{
	if (netgame)
	{
		M_StartMessage(language["NewNetGame"], NULL, false);
		return;
	}

	M_SetupNextMenu(&EpiDef);
} */

//
//      M_Episode
//

// -KM- 1998/12/16 Generates EpiDef menu dynamically.
/* static void CreateEpisodeMenu(void)
{
	if (gamedefs.GetSize() == 0)
		I_Error("No defined episodes !\n");

	EpisodeMenu = Z_New(menuitem_t, gamedefs.GetSize());

	Z_Clear(EpisodeMenu, menuitem_t, gamedefs.GetSize());

	int e = 0;
	epi::array_iterator_c it;

	for (it = gamedefs.GetBaseIterator(); it.IsValid(); it++)
	{
		gamedef_c *g = ITERATOR_TO_TYPE(it, gamedef_c*);
		if (! g) continue;

		if (W_CheckNumForName(g->firstmap.c_str()) == -1)
			continue;

		EpisodeMenu[e].status = 1;
		EpisodeMenu[e].select_func = M_Episode;
		EpisodeMenu[e].image = NULL;
		EpisodeMenu[e].alpha_key = '1' + e;

		Z_StrNCpy(EpisodeMenu[e].patch_name, g->namegraphic.c_str(), 8);
		EpisodeMenu[e].patch_name[8] = 0;

		e++;
	}

	if (e == 0)
		I_Error("No available episodes !\n");

	EpiDef.numitems  = e;
	EpiDef.menuitems = EpisodeMenu;
} */


static void ReallyDoStartLevel(skill_t skill, gamedef_c *g)
{
	newgame_params_c params;

	params.skill = skill;
	params.deathmatch = 0;

	params.random_seed = I_PureRandom();

	if (splitscreen_mode)
		params.Splitscreen();
	else
		params.SinglePlayer(0);

	params.map = G_LookupMap(g->firstmap.c_str());

	if (! params.map)
	{
		// 23-6-98 KM Fixed this.
		/* M_SetupNextMenu(&EpiDef); */
		M_StartMessage(language["EpisodeNonExist"], NULL, false);
		return;
	}

	SYS_ASSERT(G_MapExists(params.map));
	SYS_ASSERT(params.map->episode);

	G_DeferredNewGame(params);

	M_ClearMenus();
}

static void DoStartLevel(skill_t skill)
{
	// -KM- 1998/12/17 Clear the intermission.
	WI_Clear();
  
	// find episode
	gamedef_c *g = NULL;
	epi::array_iterator_c it;

	for (it = gamedefs.GetBaseIterator(); it.IsValid(); it++) 
	{ 
		g = ITERATOR_TO_TYPE(it, gamedef_c*);

		/* if (!strcmp(g->namegraphic.c_str(), EpisodeMenu[chosen_epi].patch_name))
		{
			break;
		} */
	}

	// Sanity checking...
	if (! g)
	{
		/* I_Warning("Internal Error: no episode for '%s'.\n",
			EpisodeMenu[chosen_epi].patch_name); */
		M_ClearMenus();
		return;
	}

	const mapdef_c * map = G_LookupMap(g->firstmap.c_str());
	if (! map)
	{
		/* I_Warning("Cannot find map for '%s' (episode %s)\n",
			g->firstmap.c_str(),
			EpisodeMenu[chosen_epi].patch_name); */
		M_ClearMenus();
		return;
	}

	ReallyDoStartLevel(skill, g);
}

static void VerifyNightmare(int ch)
{
	if (ch != 'y')
		return;

	DoStartLevel(sk_nightmare);
}

void M_ChooseSkill(int choice)
{
	if (choice == sk_nightmare)
	{
		M_StartMessage(language["NightMareCheck"], VerifyNightmare, true);
		return;
	}
	
	DoStartLevel((skill_t)choice);
}

void M_Episode(int choice)
{
	chosen_epi = choice;
	///M_SetupNextMenu(&SkillDef);
}

//
// Toggle messages on/off
//
void M_ChangeMessages(int choice)
{
	// warning: unused parameter `int choice'
	(void) choice;

	showMessages = 1 - showMessages;

	if (showMessages)
		CON_Printf("%s\n", language["MessagesOn"]);
	else
		CON_Printf("%s\n", language["MessagesOff"]);
}

static void EndGameResponse(int ch)
{
	if (ch != 'y')
		return;

	G_DeferredEndGame();

	currentMenu->lastOn = itemOn;
	M_ClearMenus();
}

void M_EndGame(int choice)
{
	if(choice) 
	{
        currentMenu->lastOn = itemOn;
		
        if(currentMenu->prevMenu) 
		{
            menufadefunc = M_MenuFadeOut;
            alphaprevmenu = true;
        }
    }
    else 
	{
        currentMenu->lastOn = itemOn;
        M_ClearMenus();
    }
	
	if (gamestate != GS_LEVEL)
	{
		S_StartFX(sfx_oof);
		return;
	}

	option_menuon  = 0;
	netgame_menuon = 0;

	if (netgame)
	{
		M_StartMessage(language["EndNetGame"], NULL, false);
		return;
	}

	M_StartMessage(language["EndGameCheck"], EndGameResponse, true);
}

void M_ReadThis(int choice)
{
	///M_SetupNextMenu(&ReadDef1);
}

void M_ReadThis2(int choice)
{
	///M_SetupNextMenu(&ReadDef2);
}

void M_FinishReadThis(int choice)
{
	///M_SetupNextMenu(&MainDef);
}

//
// -KM- 1998/12/16 Handle sfx that don't exist in this version.
// -KM- 1999/01/31 Generate quitsounds from default.ldf
//
static void QuitResponse(int ch)
{
	if (ch != 'y')
		return;
		
	if (!netgame)
	{
		int numsounds = 0;
		char refname[16];
		char sound[16];
		int i, start;

		// Count the quit messages
		do
		{
			sprintf(refname, "QuitSnd%d", numsounds + 1);
			if (language.IsValidRef(refname))
				numsounds++;
			else
				break;
		}
		while (true);

		if (numsounds)
		{
			// cycle through all the quit sounds, until one of them exists
			// (some of the default quit sounds do not exist in DOOM 1)
			start = i = M_Random() % numsounds;
			do
			{
				sprintf(refname, "QuitSnd%d", i + 1);
				sprintf(sound, "DS%s", language[refname]);
				if (W_CheckNumForName(sound) != -1)
				{
					S_StartFX(sfxdefs.GetEffect(language[refname]));
					break;
				}
				i = (i + 1) % numsounds;
			}
			while (i != start);
		}
	}

	// -ACB- 1999/09/20 New exit code order
	// Write the default config file first
	I_Printf("Saving system defaults...\n");
	M_SaveDefaults();

	I_Printf("Exiting...\n");

	E_EngineShutdown();
	I_SystemShutdown();

	I_DisplayExitScreen();
	I_CloseProgram(0);
}

//
// -ACB- 1998/07/19 Removed offensive messages selection (to some people);
//     Better Random Selection.
//
// -KM- 1998/07/21 Reinstated counting quit messages, so adding them to dstrings.c
//                   is all you have to do.  Using P_Random for the random number
//                   automatically kills the demo sync...
//                   (hence M_Random()... -AJA-).
//
// -KM- 1998/07/31 Removed Limit. So there.
// -KM- 1999/01/31 Load quit messages from default.ldf
//
void M_QuitEDGE(int choice)
{
	char ref[64];

	std::string msg;

	int num_quitmessages = 0;

	// Count the quit messages
	do
	{
		num_quitmessages++;

		sprintf(ref, "QUITMSG%d", num_quitmessages);
	}
	while (language.IsValidRef(ref));

	// we stopped at one higher than the last
	num_quitmessages--;

	// -ACB- 2004/08/14 Allow fallback to just the "PressToQuit" message
	if (num_quitmessages > 0)
	{
		// Pick one at random
		sprintf(ref, "QUITMSG%d", 1 + (M_Random() % num_quitmessages));

		// Construct the quit message in full
		msg = epi::STR_Format("%s\n\n%s", language[ref], language["PressToQuit"]);
	}
	else
	{
		msg = std::string(language["PressToQuit"]);
	}

	// Trigger the message
	M_StartMessage(msg.c_str(), QuitResponse, true);
}


//----------------------------------------------------------------------------
//   MENU FUNCTIONS
//----------------------------------------------------------------------------


void M_StartMessage(const char *string, void (* draw_func)(int response), 
					bool input)
{
	msg_lastmenu = menuactive;
	msg_mode = 1;
	msg_string = std::string(string);
	message_key_routine = draw_func;
	message_input_routine = NULL;
	msg_needsinput = input;
	menuactive = true;
	CON_SetVisible(vs_notvisible);
	return;
}

//
// -KM- 1998/07/21 Call M_StartMesageInput to start a message that needs a
//                 string input. (You can convert it to a number if you want to.)
//                 
// string:  The prompt.
//
// routine: Format is void routine(char *s)  Routine will be called
//          with a pointer to the input in s.  s will be NULL if the user
//          pressed ESCAPE to cancel the input.
//
void M_StartMessageInput(const char *string,
						 void (* draw_func)(const char *response))
{
	msg_lastmenu = menuactive;
	msg_mode = 2;
	msg_string = std::string(string);
	message_input_routine = draw_func;
	message_key_routine = NULL;
	msg_needsinput = true;
	menuactive = true;
	CON_SetVisible(vs_notvisible);
	return;
}

#if 0
void M_StopMessage(void)
{
	menuactive = msg_lastmenu?true:false;
	msg_string.Clear();
	msg_mode = 0;
  
	if (!menuactive)
		save_screenshot_valid = false;
}
#endif

//
// CONTROL PANEL
//


//
// -KM- 1998/09/01 Analogue binding, and hat support
//
bool M_Responder(event_t * ev)
{
	int i;

	if (ev->type != ev_keydown)
		return false;

	int ch = ev->data1;

	// -ACB- 1999/10/11 F1 is responsible for print screen at any time
	if (ch == KEYD_F1 || ch == KEYD_PRTSCR)
	{
		G_DeferredScreenShot();
		return true;
	}

	// Take care of any messages that need input
	// -KM- 1998/07/21 Message Input
	if (msg_mode == 1)
	{
		if (msg_needsinput == true &&
			!(ch == ' ' || ch == 'n' || ch == 'y' || ch == KEYD_ESCAPE))
			return false;

		msg_mode = 0;
		// -KM- 1998/07/31 Moved this up here to fix bugs.
		menuactive = msg_lastmenu?true:false;

		if (message_key_routine)
			(* message_key_routine)(ch);

		S_StartFX(sfx_swtchx);
		return true;
	}
	else if (msg_mode == 2)
	{		
		if (ch == KEYD_ENTER)
		{
			menuactive = msg_lastmenu?true:false;
			msg_mode = 0;

			if (message_input_routine)
				(* message_input_routine)(input_string.c_str());

			input_string.clear();
			
			M_ClearMenus();
			S_StartFX(sfx_swtchx);
			return true;
		}

		if (ch == KEYD_ESCAPE)
		{
			menuactive = msg_lastmenu?true:false;
			msg_mode = 0;
      
			if (message_input_routine)
				(* message_input_routine)(NULL);

			input_string.clear();
			
			M_ClearMenus();
			S_StartFX(sfx_swtchx);
			return true;
		}

		if ((ch == KEYD_BACKSPACE || ch == KEYD_DELETE) && !input_string.empty())
		{
			std::string s = input_string.c_str();

			if (input_string.size() > 0)
			{
				input_string.resize(input_string.size() - 1);
			}

			return true;
		}
		
		ch = toupper(ch);
		if (ch == '-')
			ch = '_';
			
		if (ch >= 32 && ch <= 126)  // FIXME: international characters ??
		{
			// Set the input_string only if fits
			if (input_string.size() < 64)
			{
				input_string += ch;
			}
		}

		return true;
	}

	// new options menu on - use that responder
/* 	if (option_menuon)
		return M_OptResponder(ev, ch);

	if (netgame_menuon)
		return M_NetGameResponder(ev, ch); */

	// Save Game string input
	if (saveStringEnter)
	{
		switch (ch)
		{
			case KEYD_BACKSPACE:
				if (saveCharIndex > 0)
				{
					saveCharIndex--;
					ex_slots[save_slot].desc[saveCharIndex] = 0;
				}
				break;

			case KEYD_ESCAPE:
				saveStringEnter = 0;
				strcpy(ex_slots[save_slot].desc, saveOldString);
				break;

			case KEYD_ENTER:
				saveStringEnter = 0;
				if (ex_slots[save_slot].desc[0])
					//M_DoSave(save_page, save_slot);
				break;

			default:
				ch = toupper(ch);
				SYS_ASSERT(save_style);
				if (ch >= 32 && ch <= 127 &&
					saveCharIndex < SAVESTRINGSIZE - 1 &&
					save_style->fonts[1]->StringWidth(ex_slots[save_slot].desc) <
					(SAVESTRINGSIZE - 2) * 8)
				{
					ex_slots[save_slot].desc[saveCharIndex++] = ch;
					ex_slots[save_slot].desc[saveCharIndex] = 0;
				}
				break;
		}
		return true;
	}

	// F-Keys
	if (!menuactive)
	{
		switch (ch)
		{
			case KEYD_MINUS:  // Screen size down

				if (automapactive || chat_on)
					return false;

				screen_hud = (screen_hud - 1 + NUMHUD) % NUMHUD;

				S_StartFX(sfx_stnmov);
				return true;

			case KEYD_EQUALS:  // Screen size up

				if (automapactive || chat_on)
					return false;

				screen_hud = (screen_hud + 1) % NUMHUD;

				S_StartFX(sfx_stnmov);
				return true;

			case KEYD_F2:  // Save

				M_StartControlPanel();
				S_StartFX(sfx_swtchn);
				M_SaveGame(0);
				return true;

			case KEYD_F3:  // Load

				M_StartControlPanel();
				S_StartFX(sfx_swtchn);
				M_LoadGame(0);
				return true;

			case KEYD_F4:  // Sound Volume

				M_StartControlPanel();
				//currentMenu = &SoundDef;
				itemOn = sfx_volume;
				S_StartFX(sfx_swtchn);
				return true;

			case KEYD_F5:  // Detail toggle, now loads options menu
				// -KM- 1998/07/31 F5 now loads options menu, detail is obsolete.

				S_StartFX(sfx_swtchn);
				M_StartControlPanel();
				M_Options(0);
				return true;

			case KEYD_F6:  // Quicksave

				S_StartFX(sfx_swtchn);
				M_QuickSave();
				return true;

			case KEYD_F7:  // End game

				S_StartFX(sfx_swtchn);
				M_EndGame(0);
				return true;

			case KEYD_F8:  // Toggle messages

				M_ChangeMessages(0);
				S_StartFX(sfx_swtchn);
				return true;

			case KEYD_F9:  // Quickload

				S_StartFX(sfx_swtchn);
				M_QuickLoad();
				return true;

			case KEYD_F10:  // Quit DOOM

				S_StartFX(sfx_swtchn);
				M_QuitEDGE(0);
				return true;

			case KEYD_F11:  // gamma toggle

				var_gamma++;
				if (var_gamma > 5)
					var_gamma = 0;

				const char *msg = NULL;
				
				switch(var_gamma)
				{
					case 0: { msg = language["GammaOff"];  break; }
					case 1: { msg = language["GammaLevelOne"];  break; }
					case 2: { msg = language["GammaLevelTwo"];  break; }
					case 3: { msg = language["GammaLevelThree"];  break; }
					case 4: { msg = language["GammaLevelFour"];  break; }
					case 5: { msg = language["GammaLevelFive"];  break; }

					default: { msg = NULL; break; }
				}
				
				if (msg)
					CON_PlayerMessage(consoleplayer1, "%s", msg);

				// -AJA- 1999/07/03: removed PLAYPAL reference.
				return true;

		}

		// Pop-up menu?
		if (ch == KEYD_ESCAPE)
		{
			M_StartControlPanel();
			S_StartFX(sfx_swtchn);
			return true;
		}
		return false;
	}

	// Keys usable within menu
	switch (ch)
	{
		case KEYD_DOWNARROW:
		case KEYD_WHEEL_DN:
			do
			{
				if (itemOn + 1 > currentMenu->numitems - 1)
					itemOn = 0;
				else
					itemOn++;
				S_StartFX(sfx_pstop);
			}
			while (currentMenu->menuitems[itemOn].status == -1);
			return true;

		case KEYD_UPARROW:
		case KEYD_WHEEL_UP:
			do
			{
				if (itemOn == 0)
					itemOn = currentMenu->numitems - 1;
				else
					itemOn--;
				S_StartFX(sfx_pstop);
			}
			while (currentMenu->menuitems[itemOn].status == -1);
			return true;

		case KEYD_PGUP:
		case KEYD_LEFTARROW:
			if (currentMenu->menuitems[itemOn].select_func &&
				currentMenu->menuitems[itemOn].status == 2)
			{
				S_StartFX(sfx_stnmov);
				// 98-7-10 KM Use new defines
				(* currentMenu->menuitems[itemOn].select_func)(SLIDERLEFT);
			}
			return true;

		case KEYD_PGDN:
		case KEYD_RIGHTARROW:
			if (currentMenu->menuitems[itemOn].select_func &&
				currentMenu->menuitems[itemOn].status == 2)
			{
				S_StartFX(sfx_stnmov);
				// 98-7-10 KM Use new defines
				(* currentMenu->menuitems[itemOn].select_func)(SLIDERRIGHT);
			}
			return true;

		case KEYD_ENTER:
		case KEYD_MOUSE1:
			if (currentMenu->menuitems[itemOn].select_func &&
				currentMenu->menuitems[itemOn].status)
			{
				currentMenu->lastOn = itemOn;
				(* currentMenu->menuitems[itemOn].select_func)(itemOn);
				S_StartFX(sfx_pistol);
			}
			return true;

		case KEYD_ESCAPE:
		case KEYD_MOUSE2:
		case KEYD_MOUSE3:
			currentMenu->lastOn = itemOn;
			M_ClearMenus();
			S_StartFX(sfx_swtchx);
			return true;

		case KEYD_BACKSPACE:
			currentMenu->lastOn = itemOn;
			if (currentMenu->prevMenu)
			{
				currentMenu = currentMenu->prevMenu;
				itemOn = currentMenu->lastOn;
				S_StartFX(sfx_swtchn);
			}
			return true;

		default:
			for (i = itemOn + 1; i < currentMenu->numitems; i++)
				if (currentMenu->menuitems[i].alpha_key == ch)
				{
					itemOn = i;
					S_StartFX(sfx_pstop);
					return true;
				}
			for (i = 0; i <= itemOn; i++)
				if (currentMenu->menuitems[i].alpha_key == ch)
				{
					itemOn = i;
					S_StartFX(sfx_pstop);
					return true;
				}
			break;

	}

	return false;
}


//
// M_StartControlPanel
//

void M_StartControlPanel(bool forcenext) {
    if(!allowmenu) {
        return;
    }

    if(demoplayback) {
        return;
    }

    // intro might call this repeatedly
    if(menuactive) {
        return;
    }

    menuactive = true;
	CON_SetVisible(vs_notvisible);
	
    menufadefunc = NULL;
    nextmenu = NULL;
    newmenu = forcenext;
	
	///currentMenu = &MainDef;  // JDC
	///itemOn = currentMenu->lastOn;  // JDC
    currentMenu = &MainDef;
    itemOn = currentMenu->lastOn;

    S_PauseSound();
}

//
// M_StartMainMenu
//

void M_StartMainMenu(void) {
    currentMenu = &MainDef;
    itemOn = 0;
    allowmenu = true;
    menuactive = true;
    mainmenuactive = true;
    M_StartControlPanel(false);
}

//
// M_DrawMenuSkull
//
// Draws skull icon from the symbols lump
// Pretty straightforward stuff..
//
/* 
static void M_DrawMenuSkull(int x, int y) {
    int index = 0;
    float vx1 = 0.0f;
    float vy1 = 0.0f;
    float tx1 = 0.0f;
    float tx2 = 0.0f;
    float ty1 = 0.0f;
    float ty2 = 0.0f;
    float smbwidth;
    float smbheight;
    int pic;
    vtx_t vtx[4];
    const rgbcol_t color = MENUCOLORWHITE;

    pic = GL_BindTexture("SYMBOLS", true);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, DGL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, DGL_CLAMP);

    glEnable(GL_BLEND);
    glSetVertex(vtx);

    GL_SetOrtho(0);

    index = (whichSkull & 7) + SM_SKULLS;

    vx1 = (float)x;
    vy1 = (float)y;

    smbwidth = (float)gfxwidth[pic];
    smbheight = (float)gfxheight[pic];

    tx1 = ((float)symboldata[index].x / smbwidth) + 0.001f;
    tx2 = (tx1 + (float)symboldata[index].w / smbwidth) - 0.001f;
    ty1 = ((float)symboldata[index].y / smbheight) + 0.005f;
    ty2 = (ty1 + (((float)symboldata[index].h / smbheight))) - 0.008f;

    GL_Set2DQuad
	(
        vtx,
        vx1,
        vy1,
        symboldata[index].w,
        symboldata[index].h,
        tx1,
        tx2,
        ty1,
        ty2,
        color
    );

    glTriangle(0, 1, 2);
    glTriangle(3, 2, 1);
    glDrawGeometry(4, vtx);

    GL_ResetViewport();
    glDisable(GL_BLEND);
} */

//
// M_DrawCursor
//

/* static void M_DrawCursor(int x, int y) 
{
    if(m_menumouse.d) 
	{
        HUD_DrawImage(72, 8, cursor);
    }
} */

//
// M_Drawer
//
// Called after the view has been rendered,
// but before it has been blitted.
//

void M_Drawer(void) {
    short x;
    short y;
    short i;
    short max;
    int start;
    int height;

    if(currentMenu != &MainDef) 
	{
        ST_FlashingScreen(0, 0, 0, 96);
    }

    if(MenuBindActive) 
	{
        Draw_BigText(-1, 64, MENUCOLORWHITE, "Press New Key For");
        Draw_BigText(-1, 80, MENUCOLORRED, MenuBindMessage);
        return;
    }

    if(!menuactive) 
	{
        return;
    }

    Draw_BigText(-1, 16, MENUCOLORRED, currentMenu->title);
    if(currentMenu->scale != 1) {
        GL_SetOrthoScale(currentMenu->scale);
    }

    if(currentMenu->draw_func) {
        currentMenu->draw_func();    // call Draw routine
    }

    // DRAW MENU
    x = currentMenu->x;
    y = currentMenu->y;

    start = currentMenu->menupageoffset;
    max = (currentMenu->numpageitems == -1) ? currentMenu->numitems : currentMenu->numpageitems;

    if(currentMenu->textonly) {
        height = TEXTLINEHEIGHT;
    }
    else {
        height = LINEHEIGHT;
    }

    if(currentMenu->smallfont) {
        height /= 2;
    }

    //
    // begin drawing all menu items
    //
    for(i = start; i < max+start; i++) {
        //
        // skip hidden items
        //
        if(currentMenu->menuitems[i].status == -3) {
            continue;
        }

/*         if(currentMenu == &PasswordDef) 
		{
            if(i > 0) {
                    x += TEXTLINEHEIGHT;
                }
            }
        } */

        if(currentMenu->menuitems[i].status != -1) {
            //
            // blinking letter for password menu
            //
/*             if(currentMenu == &PasswordDef && gametic & 4 && i == itemOn) {
                continue;
            } */

            if(!currentMenu->smallfont) 
			{
                rgbcol_t fontcolor = MENUCOLORRED;

                if(itemSelected == i) {
                    fontcolor += D_RGBA(0, 128, 8, 0);
                }

                Draw_BigText(x, y, fontcolor, currentMenu->menuitems[i].name);
            }
            else {
                rgbcol_t color = MENUCOLORWHITE;

                if(itemSelected == i) {
                    color = D_RGBA(255, 255, 0, menualphacolor);
                }

                //
                // tint the non-bindable key items to a shade of red
                //
                if(currentMenu == &ControlsDef) 
				{
                    if(i >= (NUM_CONTROL_ITEMS - NUM_NONBINDABLE_ITEMS)) {
                        color = D_RGBA(255, 192, 192, menualphacolor);
                    }
                }

                Draw_Text(
                    x,
                    y,
                    color,
                    currentMenu->scale,
                    false,
                    currentMenu->menuitems[i].name
                );

                //
                // nasty hack to re-set the scale after a drawtext call
                //
                if(currentMenu->scale != 1) {
                    GL_SetOrthoScale(currentMenu->scale);
                }
            }
        }
        //
        // if menu item is static but has text, then display it as gray text
        // used for subcategories
        //
        else if(currentMenu->menuitems[i].name != ".") {
            if(!currentMenu->smallfont) {
                Draw_BigText(
                    -1,
                    y,
                    MENUCOLORWHITE,
                    currentMenu->menuitems[i].name
                );
            }
            else {
                int strwidth = M_StringWidth(currentMenu->menuitems[i].name);

                Draw_Text(
                    ((int)(160.0f / currentMenu->scale) - (strwidth / 2)),
                    y,
                    D_RGBA(255, 0, 0, menualphacolor),
                    currentMenu->scale,
                    false,
                    currentMenu->menuitems[i].name
                );

                //
                // nasty hack to re-set the scale after a drawtext call
                //
                if(currentMenu->scale != 1) {
                    glOrtho(currentMenu->scale);
                }
            }
        }

        if(currentMenu != &PasswordDef) {
            y += height;
        }
    }

    //
    // display indicators that the user can scroll farther up or down
    //
    if(currentMenu->numpageitems != -1) {
        if(currentMenu->menupageoffset) {
            //up arrow
            Draw_BigText(currentMenu->x, currentMenu->y - 24, MENUCOLORWHITE, "/u More...");
        }

        if(currentMenu->menupageoffset + currentMenu->numpageitems < currentMenu->numitems) {
            //down arrow
            Draw_BigText(currentMenu->x, (currentMenu->y - 2 + (currentMenu->numpageitems-1) * height) + 24,
                         MENUCOLORWHITE, "/d More...");
        }
    }

    //
    // draw password cursor
    //
    if(currentMenu) 
	{
        // DRAW SKULL
        if(!currentMenu->smallfont) {
            int offset = 0;

            if(currentMenu->textonly) {
                x += SKULLXTEXTOFF;
            }
            else {
                x += SKULLXOFF;
            }

            if(itemOn) 
			{
                for(i = itemOn; i > 0; i--) {
                    if(currentMenu->menuitems[i].status == -3) {
                        offset++;
                    }
                }
            }
			{
            int sx = x + SKULLXOFF;
			int sy = currentMenu->y - 5 + itemOn * LINEHEIGHT;

			HUD_DrawImage(sx, sy, menu_skull[whichSkull]);
			}
        }
        //
        // draw arrow cursor
        //
        else {
            Draw_BigText(x - 12,
                         currentMenu->y - 4 + (itemOn - currentMenu->menupageoffset) * height,
                         MENUCOLORWHITE, "/l");
        }
    }

    if(currentMenu->scale != 1) 
	{
        GL_SetOrthoScale(1.0f);
    }

#ifdef _USE_XINPUT  // XINPUT
    if(xgamepad.connected && currentMenu != &MainDef) {
        GL_SetOrthoScale(0.75f);
        if(currentMenu == &PasswordDef) {
            M_DrawXInputButton(4, 271, XINPUT_GAMEPAD_B);
            Draw_Text(22, 276, MENUCOLORWHITE, 0.75f, false, "Change");
        }

        GL_SetOrthoScale(0.75f);
        M_DrawXInputButton(4, 287, XINPUT_GAMEPAD_A);
        Draw_Text(22, 292, MENUCOLORWHITE, 0.75f, false, "Select");

        if(currentMenu != &PauseDef) {
            GL_SetOrthoScale(0.75f);
            M_DrawXInputButton(5, 303, XINPUT_GAMEPAD_START);
            Draw_Text(22, 308, MENUCOLORWHITE, 0.75f, false, "Return");
        }

        GL_SetOrthoScale(1);
    }
#endif

    M_DrawCursor(mouse_x, mouse_y);
}


//
// M_ClearMenus
//

void M_ClearMenus(void) {
    if(!allowclearmenu) {
        return;
	}

    menufadefunc = NULL;
    nextmenu = NULL;
    menualphacolor = 0xff;
    menuactive = 0;

    S_ResumeSound();
}

//
// M_NextMenu
//

static void M_NextMenu(void) {
    currentMenu = nextmenu;
    itemOn = currentMenu->lastOn;
    menualphacolor = 0xff;
    alphaprevmenu = false;
    menufadefunc = NULL;
    nextmenu = NULL;
}


//
// M_SetupNextMenu
//

void M_SetupNextMenu(menu_t *menudef) {
    if(newmenu) {
        menufadefunc = M_NextMenu;
    }
    else {
        menufadefunc = M_MenuFadeOut;
    }

    alphaprevmenu = false;
    nextmenu = menudef;
    newmenu = false;
}


//
// M_MenuFadeIn
//

void M_MenuFadeIn(void) {
    int fadetime = (int)(m_menufadetime.value + 20);

    if((menualphacolor + fadetime) < 0xff) {
        menualphacolor += fadetime;
    }
    else {
        menualphacolor = 0xff;
        alphaprevmenu = false;
        menufadefunc = NULL;
        nextmenu = NULL;
    }
}


//
// M_MenuFadeOut
//

void M_MenuFadeOut(void) {
    int fadetime = (int)(m_menufadetime.value + 20);

    if(menualphacolor > fadetime) {
        menualphacolor -= fadetime;
    }
    else {
        menualphacolor = 0;

        if(alphaprevmenu == false) {
            currentMenu = nextmenu;
            itemOn = currentMenu->lastOn;
        }
        else {
            currentMenu = currentMenu->prevMenu;
            itemOn = currentMenu->lastOn;
        }

        menufadefunc = M_MenuFadeIn;
    }
}


//
// M_Ticker
//

/* CVAR_EXTERNAL(p_features); */

void M_Ticker(void) {
    mainmenuactive = (currentMenu == &MainDef) ? true : false;

    if((currentMenu == &MainDef ||
            currentMenu == &PauseDef) && usergame && demoplayback) {
        menuactive = 0;
        return;
    }
    if(!usergame) {
        OptionsDef.prevMenu = &MainDef;
        LoadDef.prevMenu = &MainDef;
        SaveDef.prevMenu = &MainDef;
    }
    else {
        OptionsDef.prevMenu = &PauseDef;
        LoadDef.prevMenu = &PauseDef;
        SaveDef.prevMenu = &PauseDef;
    }

    //
    // hidden features menu
    //
    if(currentMenu == &PauseDef) {
        currentMenu->menuitems[pause_features].status = p_features.value ? 1 : -3;
    }

    //
    // hidden hardcore difficulty option
    //
    if(currentMenu == &NewDef) {
        currentMenu->menuitems[nightmare].status = p_features.value ? 1 : -3;
    }

#ifdef _USE_XINPUT  // XINPUT
    //
    // hide mouse menu if xbox 360 controller is plugged in
    //
    if(currentMenu == &ControlMenuDef) {
        currentMenu->menuitems[controls_gamepad].status = xgamepad.connected ? 1 : -3;
    }
#endif

    //
    // hide anisotropic option if not supported on video card
    //
    if(!has_GL_EXT_texture_filter_anisotropic) {
        VideoMenu[anisotropic].status = -3;
    }

    // auto-adjust itemOn and page offset if the first menu item is being used as a header
    if(currentMenu->menuitems[0].status == -1 &&
            currentMenu->menuitems[0].name != ".") {
        // bump page offset up
        if(itemOn == 1) {
            currentMenu->menupageoffset = 0;
        }

        // bump the cursor down
        if(itemOn <= 0) {
            itemOn = 1;
        }
    }

    if(menufadefunc) {
        menufadefunc();
    }

    // auto adjust page offset for long menu items
    if(currentMenu->numpageitems != -1) {
        if(itemOn >= (currentMenu->numpageitems + currentMenu->menupageoffset)) {
            currentMenu->menupageoffset = (itemOn + 1) - currentMenu->numpageitems;

            if(currentMenu->menupageoffset >= currentMenu->numitems) {
                currentMenu->menupageoffset = currentMenu->numitems;
            }
        }
        else if(itemOn < currentMenu->menupageoffset) {
            currentMenu->menupageoffset = itemOn;

            if(currentMenu->menupageoffset < 0) {
                currentMenu->menupageoffset = 0;
            }
        }
    }

    if(--skullAnimCounter <= 0) {
        whichSkull++;
        skullAnimCounter = 4;
    }

    if(thermowait != 0 && currentMenu->menuitems[itemOn].status == 3 &&
            currentMenu->menuitems[itemOn].routine) {
        currentMenu->menuitems[itemOn].routine(thermowait == -1 ? 1 : 0);
    }
}


//
// M_Init
//

void M_Init(void) {
    int i = 0;

    currentMenu = &MainDef;
    menuactive = 0;
    itemOn = currentMenu->lastOn;
    itemSelected = -1;
    whichSkull = 0;
    skullAnimCounter = 4;
    quickSaveSlot = -1;
    menufadefunc = NULL;
    nextmenu = NULL;
    newmenu = false;

    for(i = 0; i < NUM_CONTROL_ITEMS; i++) 
	{
        ControlsItem[i].alphaKey = 0;
        memset(ControlsItem[i].name, 0, 64);
        ControlsItem[i].routine = NULL;
        ControlsItem[i].status = 1;
    }

/*     // setup password menu

    for(i = 0; i < 32; i++) {
        PasswordMenu[i].status = 1;
        PasswordMenu[i].name[0] = passwordChar[i];
        PasswordMenu[i].routine = NULL;
        PasswordMenu[i].alphaKey = (char)passwordChar[i];
    }

    dmemset(passwordData, 0xff, 16); */

    MainDef.y += 8;
    NewDef.prevMenu = &MainDef;

/*     // setup region menu

    if(W_CheckNumForName("BLUDA0") != -1) {
        CON_CvarSetValue(m_regionblood.name, 0);
        RegionMenu[region_blood].status = 1;
    }

    if(W_CheckNumForName("JPMSG01") == -1) {
        CON_CvarSetValue(st_regionmsg.name, 0);
        RegionMenu[region_lang].status = 1;
    }

    if(W_CheckNumForName("PLLEGAL") == -1 &&
            W_CheckNumForName("JPLEGAL") == -1) {
        CON_CvarSetValue(p_regionmode.name, 0);
        RegionMenu[region_mode].status = 1;
    } */

    M_InitShiftXForm();
}



//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
