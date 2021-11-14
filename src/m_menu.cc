//----------------------------------------------------------------------------
//  EDGE Main Menu Code
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2018  The EDGE Team.
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
// See M_Option.C for text built menus.
//
// -KM- 1998/07/21 Add support for message input.
//

#include "system/i_defs.h"

#include "../epi/str_format.h"

#include "../ddf/main.h"

#include "con_main.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "dstrings.h"
#include "e_main.h"
#include "g_game.h"
#include "f_interm.h"
#include "hu_draw.h"
#include "hu_stuff.h"
#include "hu_style.h"
#include "m_argv.h"
#include "m_menu.h"
#include "m_misc.h"
#include "m_netgame.h"
#include "m_option.h"
#include "m_random.h"
#include "n_network.h"
#include "p_setup.h"
#include "am_map.h"
#include "r_local.h"
#include "m_shift.h"
#include "r_draw.h"
#include "r_modes.h"
#include "r_colormap.h"
#include "r_wipe.h"
#include "s_sound.h"
#include "s_music.h"
#include "sv_chunk.h"
#include "sv_main.h"
#include "w_wad.h"
#include "z_zone.h"

extern void R_StartFading(int start, int range); //For Menu Fading

extern bool heretic_mode; //that's why this never worked right. . .
extern bool rott_mode;
//
// defaulted values
//

static bool need_wipe = false;

static bool alphaprevmenu = false;
static int  menualphacolor = 0xff; // 255!

//
// E_Display
//
// Draw current display, possibly wiping it from the previous
//
// -ACB- 1998/07/27 Removed doublebufferflag check (unneeded).

static bool wipe_gl_active = true;

// Show messages has default, 0 = off, 1 = on
int showMessages;
// This is the original cfg variable, now reintroduced.
int traditional_menu;

DEF_CVAR(m_language, std::string, "c", "ENGLISH");

int screen_hud;  // has default

static std::string msg_string;
static int msg_lastmenu;
static int msg_mode;
static int MenuTime;
char       *mn_wadname;            // wad to load

static std::string input_string;

bool singleplayer;
bool menuactive;

#define MENUSTRINGSIZE      32

#define SKULLXOFF   -24
#define LINEHEIGHT   15  //!!!!

#define HLEFT_DIR 0 //From Heretic
#define HRIGHT_DIR 1 //From Heretic
#define HSKULLXOFF   -28
#define HSKULLYOFF   -1
#define HLINEHEIGHT   20  //equiv to ITEM_HEIGHT in Heretic's source
#define HASCII_CURSOR '[' //not sure what this is for...from Heretic.

//
// fade-in/out stuff
//
void M_MenuFadeIn(void);
void M_MenuFadeOut(void);
void(*menufadefunc)(void) = NULL;

// timed message = no input from user
static bool msg_needsinput;

static void(*message_key_routine)(int response) = NULL;
static void(*message_input_routine)(const char *response) = NULL;

static int chosen_epi;

// SOUNDS
sfx_t * sfx_swtchn;
sfx_t * sfx_tink;
sfx_t * sfx_radio;
sfx_t * sfx_oof;
sfx_t * sfx_pstop;
sfx_t * sfx_stnmov;
sfx_t * sfx_pistol;
sfx_t * sfx_swtchx;
sfx_t * sfx_network;
//
//  IMAGES USED
//
static const image_c *therm_l;
static const image_c *therm_m;
static const image_c *therm_r;
static const image_c *therm_o;

static const image_c *menu_loadg;
static const image_c *menu_saveg;
static const image_c *menu_svol;
static const image_c *menu_doom;
static const image_c *menu_newgame;
static const image_c *menu_multiplayer;
static const image_c *menu_gamefiles;
static const image_c *menu_skill;
static const image_c *menu_episode;
static const image_c *menu_skull[2];
static const image_c *rott_skull;
static const image_c *menu_readthis[2];
static const image_c *SkullBaseLump[18]; //replaced = NULL -> 17, 17 frames of animation

// STYLES
static style_c *menu_def_style;
static style_c *main_menu_style;
static style_c *multi_menu_style;
static style_c *files_menu_style;
static style_c *episode_style;
static style_c *skill_style;
static style_c *load_style;
static style_c *save_style;
static style_c *dialog_style;
static style_c *sound_vol_style;

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

// Heretic defines.
#define HSLIDERLEFT  0
#define HSLIDERRIGHT 1

//
// MENU TYPEDEFS
//
typedef struct
{
	// 0 = no cursor here, 1 = ok, 2 = arrows ok
	int status;

	// image for menu entry
	char patch_name[10]; // inside of Quotes
	const image_c *image; // NULL for image (TODO, need to add char *data from Eternity)
	//char *data; // TEXT instead of image

	//const char *text; //this is to sub out from *image?

	// choice = menu item #.
	// if status = 2, choice can be SLIDERLEFT or SLIDERRIGHT
	void(*select_func)(int choice); // What we actually end up selecting

	// hotkey in menu
	char alpha_key;
}
menuitem_t;

typedef struct menu_s
{
	// # of menu items
	int numitems;

	// previous menu
	struct menu_s *prevMenu;

	// menu items
	menuitem_t *menuitems;

	// style variable
	style_c **style_var;

	// draw routine
	void(*draw_func)(void);

	// x,y of menu
	int x, y;

	// last item user was on in menu
	int lastOn;
}
menu_t;

// menu item skull is on
static int itemOn;

// doom64
short  itemSelected;

// Heretic Gold Skull animation counter (NOT blinking skull)
static int goldskullAnimCounter;

// Heretic: which skull to draw:
static int whichgoldSkull;

// skull animation counter
static int skullAnimCounter;

// which skull to draw
static int whichSkull;

char  msgNames[2][4] = { "Off","On" };

// current menudef
static menu_t *currentMenu;

//------------------------------------------------------------------------
// 3DGE
// PROTOTYPES
//
//------------------------------------------------------------------------
static void M_NewGame(int choice);
static void M_Episode(int choice);
static void M_ChooseSkill(int choice);
static void M_LoadGame(int choice);
static void M_SaveGame(int choice);

// 25-6-98 KM
extern void M_Options(int choice);
extern void M_StartServerMultiplayer(int choice);
extern void M_Multiplayer(int choice);
extern void M_GameFiles(int choice);
static void M_LoadSavePage(int choice);
static void M_ReadThis(int choice);
static void M_ReadThis2(int choice);
static void H_ReadThis(int choice);
static void H_ReadThis2(int choice);
void M_EndGame(int choice);

static void M_ChangeMessages(int choice);
static void M_SfxVol(int choice);
static void M_MusicVol(int choice);
// static void M_Sound(int choice);

static void M_HostNetGame(int choice);
static void M_JoinNetGame(int choice);
static void M_SplitScreenGame(int choice);

static void M_FinishReadThis(int choice);
static void H_FinishReadThis(int choice);
static void M_LoadSelect(int choice);
static void M_SaveSelect(int choice);
static void M_ReadSaveStrings(void);
static void M_QuickSave(void);
static void M_QuickLoad(void);

static void M_DrawMainMenu(void);
static void M_DrawReadThis1(void);
static void M_DrawReadThis2(void);
static void M_DrawNewGame(void);
static void M_DrawMultiGame(void);
static void M_DrawEpisode(void);
static void HereticMainMenuDrawer(void); // Heretic
static void DrawFilesMenu(void); //Heretic only.
static void M_DrawSlider(menu_t *currentMenu, int item, int width, int slot);
static void M_DrawSound(void);
static void M_DrawLoad(void);
static void M_DrawSave(void);

static void M_DrawSaveLoadBorder(float x, float y, int len);
static void M_SetupNextMenu(menu_t * menudef);
void M_ClearMenus(void);
void M_StartControlPanel(void);
// static void M_StopMessage(void);

//
// DOOM MENU
//
typedef enum
{
	newgame = 0,
	multiplayer,
	options,
	loadgame,
	savegame,
	readthis,
	quitdoom,
	main_end
}
main_e;

//
// HERETIC MENU
//
typedef enum
{
	hnewgame = 0,
	hmultiplayer,
	hoptions,
	gamefiles,
	hreadthis,
	quitheretic,
	hmain_end
}
hmain_e;

//
// ROTT MENU
//
typedef enum
{
	rnewgame = 0,
	rmultiplayer,
	roptions,
	rreadthis,
	quitrott,
	rmain_end
}
rmain_e;

#if 0
d64menuitem_t MainMenu[] = {
{ 1,"New Game",M_NewGame,'n' },
{ 1,"Options",M_Options,'o' },
{ 1,"Load Game",M_LoadGame,'l' },
{ 1,"Quit Game",M_QuitEDGE,'q' },
};
#endif // 0

// Heretic "old" main menu, planned to be phased out!
static menuitem_t HereticMainMenu[] =
{
	{ 1, "H_NGAME",   NULL, M_NewGame, '1' },
	{ 1, "H_MULTI",   NULL, M_Multiplayer, 'p' },
	{ 1, "H_OPTION",  NULL, M_Options, 'o' },
	{ 1, "H_GAMFIL", NULL, M_GameFiles, 'f' }, //already written, needs a graphic from plums
	{ 1, "H_RDTHIS",  NULL, H_ReadThis, 'r' },
	{ 1, "H_QUITG",   NULL, M_QuitEDGE, 'q' }
};

//traditional DOOM main menu support
static menuitem_t MainMenu[] =
{
	{1, "M_NGAME",   NULL,  M_NewGame, 'n'},
	{1, "M_MULTI",   NULL,  M_Multiplayer, 'p'},
	{1, "M_OPTION",  NULL,  M_Options, 'o'},
	{1, "M_LOADG",   NULL, M_LoadGame, 'l'},
	{1, "M_SAVEG",   NULL,  M_SaveGame, 's'},
	// Another hickup with Special edition.
	{1, "M_RDTHIS",  NULL,  M_ReadThis, 'r'},
	{1, "M_QUITG",   NULL, M_QuitEDGE, 'q'}
};

static menuitem_t ROTTMainMenu[] =
{
{ 1, "TNGAME",   NULL,  M_NewGame, 'n' },
{ 1, "TCBGAME",   NULL,  M_Multiplayer, 'p' },
{ 1, "TOPTIONS",  NULL,  M_Options, 'o' },
{ 1, "TRESTORE",   NULL, M_LoadGame, 'l' },
{ 1, "TSAVE",   NULL,  M_SaveGame, 's' },
// Another hickup with Special edition.
{ 1, "THELP",  NULL,  M_ReadThis, 'r' },
{ 1, "TQUITG",   NULL, M_QuitEDGE, 'q' }
};

static menu_t MainDef =
{
	main_end,
	NULL,
	MainMenu,
	&main_menu_style,
	M_DrawMainMenu,
#ifdef HYPERTENSION
	37, 100,
#else
	97, 64,
#endif
	//Hypertension	37, 100, //97, 64
	0
};

static menu_t HereticMainDef =
{
	hmain_end,
	NULL,
	HereticMainMenu,
	&main_menu_style,
	HereticMainMenuDrawer,
	110, 56,
	0
};

static menu_t ROTTMainDef =
{
	main_end,
	NULL,
	ROTTMainMenu,
	&main_menu_style,
	M_DrawMainMenu,
	97, 64,
	0
};

//Removed Multiplayer Host/Join NetGame for now. Left 2 Player option open!
static menuitem_t MultiMenu[] = ///Dupe Doom Legacy's Multiplayer menu. . .
{
	//{ 1, "M_STSERV", NULL, M_HostNetGame, 'o' }, // Pulls up the Advanced Start Menu (Host)
	//{ 1, "M_CONNEC",  NULL, M_JoinNetGame, 'h' }, //Pulls up the Join Game Menu (client)
	{ 1, "M_2PLAYR",  NULL, M_SplitScreenGame, 'z' }, //Goes to splitscreen game (II)
	{ 1, "M_OPTION", NULL,  M_Options, 'o' }, // Goes to Options...
	{ 1, "M_QUITG",   NULL, M_EndGame, 'e'}
};

static menu_t MultiDef =
{
	3,	//multi_numtypes, // # of menu items...try main_end
	&MainDef, // previous menu
	MultiMenu,  // ::from::SkillMenu ->> menuitem_t ->
	&multi_menu_style, /// Same style, I guess?
	M_DrawMultiGame,
	48, 63,
	0  // lastOn
};

static menuitem_t HMultiMenu[] = ///Dupe Doom Legacy's Multiplayer menu. . .
{
	//{ 1, "M_SETUPA", NULL, NULL, 'a' }, //Setup Player One (controls and stuff, only shows in multiplayer menu)
	//{ 1, "M_SETUPB", NULL, NULL, 'b' }, //Setup Player Two (controls and stuff, only shows in multiplayer menu)
	{ 1, "H_HOSTNG", NULL, M_HostNetGame, 'o' }, // Pulls up the Advanced Start Menu (Host)
	{ 1, "H_JOINNG",  NULL, M_JoinNetGame, 'h' }, //Pulls up the Join Game Menu (client)
	{ 1, "M_2PLAYR",  NULL, M_SplitScreenGame, 'z' }, //Goes to splitscreen game (II)
	{ 1, "H_OPTION", NULL, M_Options, 'o' } // Goes to Options...
};

static menu_t HMultiDef =
{
	4,	//multi_numtypes, // # of menu items...try main_end
	&HereticMainDef, // previous menu
	HMultiMenu,  // ::from::SkillMenu ->> menuitem_t ->
	&multi_menu_style, /// Same style, I guess?
	M_DrawMultiGame,
	48, 63,
	0  // lastOn
};

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------

// GAMEFILES MENU (Heretic Specific)
// Since LoadGame and Savegame are in a submenu now.
static menuitem_t FilesMenu[] =
{
	{ 1, "H_LOADG",   NULL, M_LoadGame, 'l' },
	{ 1, "H_SAVEG",   NULL, M_SaveGame, 's' }
};

// FilesDef = GAMEFILES MENU, from Heretic.
static menu_t FilesDef =
{
	2,	//multi_numtypes, // # of menu items...try main_end
	&MainDef, // previous menu
	FilesMenu,  // DrawFilesMenu
	&files_menu_style, /// create this...somewhere
	NULL, //graphic that appears at the top of this menu
	110, 60,
	0  // lastOn
};

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------

//
// EPISODE SELECT
//
// -KM- 1998/12/16 This is generated dynamically.
//
static menuitem_t *EpisodeMenu = NULL;

static menuitem_t DefaultEpiMenu =
{
	1,  // status
	"Working",  // name
	NULL,  // image
	NULL,  // select_func
	'w'  // alphakey
};

static menu_t EpiDef =
{
	0,  //ep_end,  // # of menu items
	&MainDef,  // previous menu
	&DefaultEpiMenu,  // menuitem_t ->
	&episode_style,
	M_DrawEpisode,  // drawing routine ->
	48, 63,  // x,y
	0  // lastOn
};

static menu_t HereticEpiDef =
{
	0,  //ep_end,  // # of menu items
	&HereticMainDef,  // previous menu
	&DefaultEpiMenu,  // menuitem_t ->
	&episode_style,
	M_DrawEpisode,  // drawing routine ->
	80, 50,  // x,y as in original Heretic
	0  // lastOn
};

static menuitem_t SkillMenu[] =
{
	{1, "M_JKILL", NULL, M_ChooseSkill, 'p'},
	{1, "M_ROUGH", NULL, M_ChooseSkill, 'r'},
	{1, "M_HURT",  NULL, M_ChooseSkill, 'h'},
	{1, "M_ULTRA", NULL, M_ChooseSkill, 'u'},
	{1, "M_NMARE", NULL, M_ChooseSkill, 'n'}
};

static menu_t SkillDef =
{
	sk_numtypes,  // # of menu items
	&EpiDef,  // previous menu
	SkillMenu,  // menuitem_t ->
	&skill_style,
	M_DrawNewGame,  // drawing routine ->
	48, 63,  // x,y 63
	sk_medium  // lastOn
};

static menuitem_t HereticSkillMenu[] =
{
	{ 1, "H_JKILL", NULL, M_ChooseSkill, 'p' },
	{ 1, "H_ROUGH", NULL, M_ChooseSkill, 'r' },
	{ 1, "H_HURT",  NULL, M_ChooseSkill, 'h' },
	{ 1, "H_ULTRA", NULL, M_ChooseSkill, 'u' },
	{ 1, "H_NMARE", NULL, M_ChooseSkill, 'n' }
};

static menu_t HereticSkillDef =
{
	sk_numtypes,  // # of menu items
	&EpiDef,  // previous menu
	HereticSkillMenu,  // menuitem_t ->
	&skill_style,
	M_DrawNewGame,  // drawing routine ->
	38, 30,  // x,y 63
	sk_medium  // lastOn
};

//
// OPTIONS MENU
//
typedef enum
{
	endgame,
	messages,
	scrnsize,
	option_empty1,
	mousesens,
	option_empty2,
	soundvol,
	opt_end
}
options_e;

//
// Read This! MENU 1 & 2
//

static menuitem_t ReadMenu1[] =
{
	{1, "", NULL, M_ReadThis2, 0}
};

static menuitem_t HReadMenu1[] =
{
	{ 1, "", NULL, H_ReadThis2, 0 }
};

static menu_t ReadDef1 =
{
	1,
	&MainDef,
	ReadMenu1,
	&menu_def_style,  // FIXME: maybe have READ_1 and READ_2 styles ??
	M_DrawReadThis1,
	280, 185,
	0
};

static menu_t HReadDef1 =
{
	1,
	&HereticMainDef,
	HReadMenu1,
	&menu_def_style,  // FIXME: maybe have READ_1 and READ_2 styles ??
	M_DrawReadThis1,
	280, 185,
	0
};

static menuitem_t ReadMenu2[] =
{
	{1, "", NULL, M_FinishReadThis, 0}
};

static menuitem_t HReadMenu2[] =
{
	{ 1, "", NULL, H_FinishReadThis, 0 }
};

static menu_t ReadDef2 =
{
	1,
	&ReadDef1,
	ReadMenu2,
	&menu_def_style,  // FIXME: maybe have READ_1 and READ_2 styles ??
	M_DrawReadThis2,
	330, 175,
	0
};

static menu_t HReadDef2 =
{
	1,
	&HReadDef1,
	HReadMenu2,
	&menu_def_style,  // FIXME: maybe have READ_1 and READ_2 styles ??
	M_DrawReadThis2,
	330, 175,
	0
};

//
// SOUND VOLUME MENU
//
typedef enum
{
	sfx_vol,
	sfx_empty1,
	music_vol,
	sfx_empty2,
	sound_end
}
sound_e;

static menuitem_t SoundMenu[] =
{
	{2, "M_SFXVOL", NULL, M_SfxVol, 's'},
	{-1, "", NULL, 0},
	{2, "M_MUSVOL", NULL, M_MusicVol, 'm'},
	{-1, "", NULL, 0}
};

static menu_t SoundDef =
{
	sound_end,
	&MainDef,  ///  &OptionsDef,
	SoundMenu,
	&sound_vol_style,
	M_DrawSound,
	80, 64,
	0
};

static menu_t HereticSoundDef =
{
	sound_end,
	&HereticMainDef,  ///  &OptionsDef,
	SoundMenu,
	&sound_vol_style,
	M_DrawSound,
	90, 20,
	0
};

//
// LOAD GAME MENU
//
// Note: upto 10 slots per page
//
static menuitem_t LoadingMenu[] =
{
	{2, "", NULL, M_LoadSelect, '1'},
	{2, "", NULL, M_LoadSelect, '2'},
	{2, "", NULL, M_LoadSelect, '3'},
	{2, "", NULL, M_LoadSelect, '4'},
	{2, "", NULL, M_LoadSelect, '5'},
	{2, "", NULL, M_LoadSelect, '6'},
	{2, "", NULL, M_LoadSelect, '7'},
	{2, "", NULL, M_LoadSelect, '8'},
	{2, "", NULL, M_LoadSelect, '9'},
	{2, "", NULL, M_LoadSelect, '0'}
};

static menu_t LoadDef =
{
	SAVE_SLOTS,
	&MainDef,
	LoadingMenu,
	&load_style,
	M_DrawLoad,
	30, 34,
	0
};

//
// SAVE GAME MENU
//
static menuitem_t SavingMenu[] =
{
	{2, "", NULL, M_SaveSelect, '1'},
	{2, "", NULL, M_SaveSelect, '2'},
	{2, "", NULL, M_SaveSelect, '3'},
	{2, "", NULL, M_SaveSelect, '4'},
	{2, "", NULL, M_SaveSelect, '5'},
	{2, "", NULL, M_SaveSelect, '6'},
	{2, "", NULL, M_SaveSelect, '7'},
	{2, "", NULL, M_SaveSelect, '8'},
	{2, "", NULL, M_SaveSelect, '9'},
	{2, "", NULL, M_SaveSelect, '0'}
};

static menu_t SaveDef =
{
	SAVE_SLOTS,
	&MainDef,
	SavingMenu,
	&save_style,
	M_DrawSave,
	30, 34,
	0
};

// 98-7-10 KM Chooses the page of savegames to view
void M_LoadSavePage(int choice)
{
	switch (choice)
	{
	case SLIDERLEFT:
		// -AJA- could use `OOF' sound...
		if (save_page == 0)
			return;

		save_page--;
		break;

	case SLIDERRIGHT:
		if (save_page >= SAVE_PAGES - 1)
			return;

		save_page++;
		break;
	}

	S_StartFX(sfx_swtchn);
	M_ReadSaveStrings();
}

//
// Read the strings from the savegame files
//
// 98-7-10 KM Savegame slots increased
//
void M_ReadSaveStrings(void)
{
	int i, version;

	saveglobals_t *globs;

	for (i = 0; i < SAVE_SLOTS; i++)
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

		if (!SV_OpenReadFile(fn.c_str()))
		{
			ex_slots[i].empty = true;
			ex_slots[i].corrupt = false;
			continue;
		}

		if (!SV_VerifyHeader(&version))
		{
			SV_CloseReadFile();
			continue;
		}

		globs = SV_LoadGLOB();

		// close file now -- we only need the globals
		SV_CloseReadFile();

		if (!globs)
			continue;

		// --- pull info from global structure ---

		if (!globs->game || !globs->level || !globs->description)
		{
			SV_FreeGLOB(globs);
			continue;
		}

		ex_slots[i].corrupt = false;

		Z_StrNCpy(ex_slots[i].gamename, globs->game, 32 - 1);
		Z_StrNCpy(ex_slots[i].mapname, globs->level, 10 - 1);

		Z_StrNCpy(ex_slots[i].desc, globs->description, SAVESTRINGSIZE - 1);

		if (globs->desc_date)
			Z_StrNCpy(ex_slots[i].timestr, globs->desc_date, 32 - 1);

		ex_slots[i].skill = globs->skill;
		ex_slots[i].netgame = globs->netgame;

		SV_FreeGLOB(globs);

#if 0
		// handle screenshot
		if (globs->view_pixels)
		{
			int x, y;

			for (y = 0; y < 100; y++)
				for (x = 0; x < 160; x++)
				{
					save_screenshot[x][y] = SV_GetShort();
				}
		}
#endif
	}

	// fix up descriptions
	for (i = 0; i < SAVE_SLOTS; i++)
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

static void M_DrawSaveLoadCommon(int row, int row2, style_c *style)
{
	int y = LoadDef.y + LINEHEIGHT * row;

	slot_extra_info_t *info;

	char mbuffer[200];

	sprintf(mbuffer, "PAGE %d", save_page + 1);

	// -KM-  1998/06/25 This could quite possibly be replaced by some graphics...
	if (save_page > 0)
		HL_WriteText(style, 2, LoadDef.x - 4, y, "< PREV");

	HL_WriteText(style, 2, LoadDef.x + 94 - style->fonts[2]->StringWidth(mbuffer) / 2, y,
		mbuffer);

	if (save_page < SAVE_PAGES - 1)
		HL_WriteText(style, 2, LoadDef.x + 192 - style->fonts[2]->StringWidth("NEXT >"), y,
			"NEXT >");

	info = ex_slots + itemOn;
	SYS_ASSERT(0 <= itemOn && itemOn < SAVE_SLOTS);

	if (saveStringEnter || info->empty || info->corrupt)
		return;

	// show some info about the savegame

	y = LoadDef.y + LINEHEIGHT * (row2 + 1);

	mbuffer[0] = 0;

	strcat(mbuffer, info->timestr);

	HL_WriteText(style, 3, 310 - style->fonts[3]->StringWidth(mbuffer), y, mbuffer);

	y -= LINEHEIGHT;

	mbuffer[0] = 0;

	// FIXME: use the patches (but shrink them)
	switch (info->skill)
	{
	case 0: strcat(mbuffer, "Too Young To Die"); break;
	case 1: strcat(mbuffer, "Not Too Rough"); break;
	case 2: strcat(mbuffer, "Hurt Me Plenty"); break;
	case 3: strcat(mbuffer, "Ultra Violence"); break;
	default: strcat(mbuffer, "NIGHTMARE"); break;
	}

	HL_WriteText(style, 3, 310 - style->fonts[3]->StringWidth(mbuffer), y, mbuffer);

	y -= LINEHEIGHT;

	mbuffer[0] = 0;

	switch (info->netgame)
	{
	case 0: strcat(mbuffer, "SP MODE"); break;
	case 1: strcat(mbuffer, "COOP MODE"); break;
	default: strcat(mbuffer, "DM MODE"); break;
	}

	HL_WriteText(style, 3, 310 - style->fonts[3]->StringWidth(mbuffer), y, mbuffer);

	y -= LINEHEIGHT;

	mbuffer[0] = 0;

	strcat(mbuffer, info->mapname);

	HL_WriteText(style, 3, 310 - style->fonts[3]->StringWidth(mbuffer), y, mbuffer);
}

//
// 1998/07/10 KM Savegame slots increased
//
void M_DrawLoad(void)
{
	int i;

	HUD_DrawImage(72, 8, menu_loadg);

	for (i = 0; i < SAVE_SLOTS; i++)
		M_DrawSaveLoadBorder(LoadDef.x + 8, LoadDef.y + LINEHEIGHT * (i), 24);

	// draw screenshot ?

	for (i = 0; i < SAVE_SLOTS; i++)
		HL_WriteText(load_style, ex_slots[i].corrupt ? 3 : 0,
			LoadDef.x + 8, LoadDef.y + LINEHEIGHT * (i),
			ex_slots[i].desc);

	M_DrawSaveLoadCommon(i, i + 1, load_style);
}

//
// Draw border for the savegame description
//
void M_DrawSaveLoadBorder(float x, float y, int len)
{
	if (heretic_mode)
	{
		const image_c *L = W_ImageLookup("NULL");
		const image_c *C = W_ImageLookup("H_FSLOT");
		const image_c *R = W_ImageLookup("NULL");

		HUD_DrawImage(x - IM_WIDTH(L), y + 7, L);

		for (int i = 0; i < len; i++, x += IM_WIDTH(C))
			HUD_DrawImage(x, y + 7, C);

		HUD_DrawImage(x, y + 7, R);
	}

		const image_c *L = W_ImageLookup("M_LSLEFT");
		const image_c *C = W_ImageLookup("M_LSCNTR");
		const image_c *R = W_ImageLookup("M_LSRGHT");

		HUD_DrawImage(x - IM_WIDTH(L), y + 7, L);

		for (int i = 0; i < len; i++, x += IM_WIDTH(C))
			HUD_DrawImage(x, y + 7, C);

		HUD_DrawImage(x, y + 7, R);
	
}

//
// User wants to load this game
//
// 98-7-10 KM Savegame slots increased
//
void M_LoadSelect(int choice)
{
	if (choice < 0)
	{
		M_LoadSavePage(choice);
		return;
	}

	G_DeferredLoadGame(save_page * SAVE_SLOTS + choice);
	M_ClearMenus();
}

//
// Selected from DOOM menu
//
void M_LoadGame(int choice)
{
	if (netgame)
	{
		M_StartMessage(language["NoLoadInNetGame"], NULL, false);
		return;
	}

	M_SetupNextMenu(&LoadDef);
	M_ReadSaveStrings();
}

//
// 98-7-10 KM Savegame slots increased
//
void M_DrawSave(void)
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

			HL_WriteText(save_style, 1, LoadDef.x + 8, y, ex_slots[i].desc);
			HL_WriteText(save_style, 1, LoadDef.x + len + 8, y, "_");
		}
		else
			HL_WriteText(save_style, 0, LoadDef.x + 8, y, ex_slots[i].desc);
	}

	M_DrawSaveLoadCommon(i, i + 1, save_style);
}

//
// M_Responder calls this when user is finished
//
// 98-7-10 KM Savegame slots increased
//
static void M_DoSave(int page, int slot)
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
}

//
// User wants to save. Start string input for M_Responder
//
void M_SaveSelect(int choice)
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
}

//
// Selected from DOOM menu
//
void M_SaveGame(int choice)
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
}

//
//   M_QuickSave
//

static void QuickSaveResponse(int ch)
{
	if (ch == 'y')
	{
		M_DoSave(quickSavePage, quickSaveSlot);
		S_StartFX(sfx_swtchx);
	}
}

void M_QuickSave(void)
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
}

static void QuickLoadResponse(int ch)
{
	if (ch == 'y')
	{
		int tempsavepage = save_page;

		save_page = quickSavePage;
		M_LoadSelect(quickSaveSlot);

		save_page = tempsavepage;
		S_StartFX(sfx_swtchx);
	}
}

void M_QuickLoad(void)
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
}

//---------------------------------------------------------------------------
//
// PROC InitFonts
//
//---------------------------------------------------------------------------

static void InitFonts(void)
{
	//FontABaseLump = W_GetNumForName("FONTA_S") + 1;
	//FontBBaseLump = W_GetNumForName("FONTB_S") + 1;
}

//
// Read This Menus
// Had a "quick hack to fix romero bug"
//
void M_DrawReadThis1(void)
{
	HUD_StretchImage(0, 0, 320, 200, menu_readthis[0]);
}

//
// Read This Menus - optional second page.
//
void M_DrawReadThis2(void)
{
	HUD_StretchImage(0, 0, 320, 200, menu_readthis[1]);
}

void M_DrawSound(void)
{
	HUD_DrawImage(60, 38, menu_svol);

	M_DrawThermo(SoundDef.x, SoundDef.y + LINEHEIGHT * (sfx_vol + 1), SND_SLIDER_NUM, au_sfx_volume, 1);
	M_DrawThermo(SoundDef.x, SoundDef.y + LINEHEIGHT * (music_vol + 1), SND_SLIDER_NUM, au_mus_volume, 1);
}

void M_DrawHereticSound(void)
{
	HUD_DrawImage(60, 38, menu_svol);

	M_DrawThermo(SoundDef.x, SoundDef.y + HLINEHEIGHT * (sfx_vol + 1), SND_SLIDER_NUM, au_sfx_volume, 1);
	M_DrawThermo(SoundDef.x, SoundDef.y + HLINEHEIGHT * (music_vol + 1), SND_SLIDER_NUM, au_mus_volume, 1);
}

#if 0
void M_Sound(int choice)
{
	M_SetupNextMenu(&SoundDef);
}
#endif

// -ACB- 1999/10/10 Sound API Volume re-added
void M_SfxVol(int choice)
{
	switch (choice)
	{
	case SLIDERLEFT:
		if (au_sfx_volume > 0)
			au_sfx_volume--;

		break;

	case SLIDERRIGHT:
		if (au_sfx_volume < SND_SLIDER_NUM - 1)
			au_sfx_volume++;

		break;
	}

	S_ChangeSoundVolume();
}

// -ACB- 1999/10/07 Removed sound references: New Sound API
void M_MusicVol(int choice)
{
	switch (choice)
	{
	case SLIDERLEFT:
		if (au_mus_volume > 0)
			au_mus_volume--;

		break;

	case SLIDERRIGHT:
		if (au_mus_volume < SND_SLIDER_NUM - 1)
			au_mus_volume++;

		break;
	}

	S_ChangeMusicVolume();
}

void HereticMainMenuDrawer(void)
{
	//int frame = (I_GetTime() / 3) % 18;
	///int frame = (I_GetTime() / 3) % 18; //TODO: M_SKL00 has animations that need defining in ANIMS.DDF!!
	int frame = (I_GetTime() / 3) % 18;

	HUD_DrawImage(40, 10, SkullBaseLump[goldskullAnimCounter]);// + (17 - frame)); LEFT

	HUD_DrawImage(240, 10, SkullBaseLump[goldskullAnimCounter]);// + (17 - frame)); RIGHT

	//Now, finally, draw the "main" menu.
	HUD_DrawImage(94, 2, menu_doom);
}

void M_DrawMainMenu(void)
{
	HUD_DrawImage(94, 2, menu_doom);
}

void M_DrawNewGame(void)
{
	HUD_DrawImage(96, 14, menu_newgame);
	HUD_DrawImage(54, 38, menu_skill);
}

void M_DrawMultiGame(void)
{
	HUD_DrawImage(96, 14, menu_multiplayer);
}

// Game Files menu from Heretic!
void M_DrawFileGFX(void)
{
	HUD_DrawImage(96, 14, menu_gamefiles); //hack
}

void M_NewGame(int choice)
{
	if (splitscreen_mode)
	{
		M_StartMessage(language["NewSplitGame"], NULL, false);
		return;
	}

#if 0
	if (netgame)
	{
		M_StartMessage(language["NewNetGame"], NULL, false);
		return;
	}
#endif // 0

	netgame = false; //temp hack
	splitscreen_mode = false; //temp hack
	M_SetupNextMenu(&EpiDef);
}

void M_Multiplayer(int choice)
{
	if (netgame)
	{
		M_StartMessage(language["NewNetGame"], NULL, true);
		return;
	}

	if (heretic_mode)
	{
		M_SetupNextMenu(&HMultiDef);
	}
	else
	{
		M_SetupNextMenu(&MultiDef);
	}

	// hack
}

void M_GameFiles(int choice)
{
	///bool netgame = true;
	if (netgame)
	{
		M_StartMessage(language["NewNetGame"], NULL, true);
		return;
	}

	M_SetupNextMenu(&FilesDef);
}

extern void M_NetHostBegun(void);
extern void M_NetJoinBegun(void);
extern void M_SplitScreenBegun(void);

void M_HostNetGame(int choice)
{
	option_menuon = 0;
	netgame_menuon = 1;
	splitgame_menuon = 0;

	M_NetHostBegun();
}

void M_JoinNetGame(int choice)
{
	//#if 0
	option_menuon = 0;
	netgame_menuon = 2;
	splitgame_menuon = 0;

	M_NetJoinBegun();
	//#endif
}

void M_SplitScreenGame(int choice)
{
	if (gamestate != GS_TITLESCREEN)
	{
		M_StartMessage(language["NewSplitGame"], NULL, false);
		return;
	}

	//bool netgame = true;
	option_menuon = 0;
	netgame_menuon = 0;
	splitgame_menuon = 1;

	M_SplitScreenBegun();
}
//
//      M_Episode
//

// -KM- 1998/12/16 Generates EpiDef menu dynamically.
static void CreateEpisodeMenu(void)
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
		if (!g) continue;

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

	EpiDef.numitems = e;
	EpiDef.menuitems = EpisodeMenu;
}

void M_DrawEpisode(void)
{
	if (!EpisodeMenu)
		CreateEpisodeMenu();

	if (EpiDef.numitems == 1)
		M_Episode(0);

	HUD_DrawImage(54, 38, menu_episode);
}

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

	if (!params.map)
	{
		// 23-6-98 KM Fixed this.
		M_SetupNextMenu(&EpiDef);
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

		if (!strcmp(g->namegraphic.c_str(), EpisodeMenu[chosen_epi].patch_name))
		{
			break;
		}
	}

	// Sanity checking...
	if (!g)
	{
		I_Warning("Internal Error: no episode for '%s'.\n",
			EpisodeMenu[chosen_epi].patch_name);
		M_ClearMenus();
		return;
	}

	const mapdef_c * map = G_LookupMap(g->firstmap.c_str());
	if (!map)
	{
		I_Warning("Cannot find map for '%s' (episode %s)\n",
			g->firstmap.c_str(),
			EpisodeMenu[chosen_epi].patch_name);
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

	if (heretic_mode)
	{
		M_SetupNextMenu(&HereticSkillDef);
		return;
	}
	else
		M_SetupNextMenu(&SkillDef);
}

//
// Toggle messages on/off
//
void M_ChangeMessages(int choice)
{
	// warning: unused parameter `int choice'
	(void)choice;

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

	R_StartFading(255, 0);
	currentMenu->lastOn = itemOn;
	M_ClearMenus();
}

void M_EndGame(int choice)
{
	if (gamestate != GS_LEVEL)
	{
		S_StartFX(sfx_oof);
		return;
	}

	option_menuon = 0;
	netgame_menuon = 0;
	splitgame_menuon = 0;

	if (netgame)
	{
		M_StartMessage(language["EndNetGame"], NULL, false);
		return;
	}

	M_StartMessage(language["EndGameCheck"], EndGameResponse, true);
	splitscreen_mode = false;
}

void M_ReadThis(int choice)
{
	M_SetupNextMenu(&ReadDef1);
}

void H_ReadThis(int choice)
{
	M_SetupNextMenu(&HReadDef1);
}

void M_ReadThis2(int choice)
{
	M_SetupNextMenu(&ReadDef2);
}

void H_ReadThis2(int choice)
{
	M_SetupNextMenu(&HReadDef2);
}

void M_FinishReadThis(int choice)
{
	M_SetupNextMenu(&MainDef);
}

void H_FinishReadThis(int choice)
{
	M_SetupNextMenu(&HereticMainDef);
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
		} while (true);

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
			} while (i != start);
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
	} while (language.IsValidRef(ref));

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

void M_DrawThermo(int x, int y, int thermWidth, int thermDot, int div)
{
	int i, basex = x;
	int step = (8 / div);

	// Note: the (step+1) here is for compatibility with the original
	// code.  It seems required to make the thermo bar tile properly.

	HUD_StretchImage(x, y, step + 1, IM_HEIGHT(therm_l) / div, therm_l);

	for (i = 0, x += step; i < thermWidth; i++, x += step)
	{
		HUD_StretchImage(x, y, step + 1, IM_HEIGHT(therm_m) / div, therm_m);
	}

	HUD_StretchImage(x, y, step + 1, IM_HEIGHT(therm_r) / div, therm_r);

	x = basex + step + thermDot * step;

	HUD_StretchImage(x, y, step + 1, IM_HEIGHT(therm_o) / div, therm_o);
}

//----------------------------------------------------------------------------
//   HERETIC MENU FUNCTIONS
//----------------------------------------------------------------------------
#if 0

void M_DrawSlider(menu_t *currentMenu, int item, int width, int slot)
{
	//
	//int i, basex = x;
	int x;
	int y;
	int x2;
	int count; //compat with Heretic, we can hack around step int and use count like Vanilla Heretic..

			 // Note: the (step+1) here is for compatibility with the original
			 // code.  It seems required to make the thermo bar tile properly.
			 // int step = (8 / div);

			 //NOTE: Assume weird void above

	x = currentMenu->x + 24;
	y = currentMenu->y + 2 + (item * LINEHEIGHT);

	//HUD_StretchImage(x - 32, y, IM_HEIGHT(therm_l));
	HUD_DrawImage(x - 32, y, therm_l);

	for (x2 = x, count = width; count--; x2 += 8)
	{
		HUD_DrawImage(x2, y, W_ImageLookup(count & 1 ? "M_SLDMD1" : "M_SLDMD2"));
	}

	HUD_DrawImage(x2, y, therm_r); //M_SLDRT

	HUD_DrawImage(x + 4 + slot * 8, y + 7, therm_o); //M_SLDKB
}
#endif // 0

void M_StartMessage(const char *string, void(*routine)(int response),
	bool input)
{
	msg_lastmenu = menuactive;
	msg_mode = 1;
	msg_string = std::string(string);
	message_key_routine = routine;
	message_input_routine = NULL;
	msg_needsinput = input;
	menuactive = true;
	CON_SetVisible(vs_notvisible);
	paused = false;
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
	void(*routine)(const char *response))
{
	msg_lastmenu = menuactive;
	msg_mode = 2;
	msg_string = std::string(string);
	message_input_routine = routine;
	message_key_routine = NULL;
	msg_needsinput = true;
	menuactive = true;
	CON_SetVisible(vs_notvisible);
	paused = false;
	return;
}

#if 0
void M_StopMessage(void)
{
	menuactive = msg_lastmenu ? true : false;
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
			!(ch == ' ' || ch == 'n' || ch == 'y' || ch == KEYD_ESCAPE || ch == KEYD_JOY2))
			return false;

		msg_mode = 0;
		// -KM- 1998/07/31 Moved this up here to fix bugs.
		menuactive = msg_lastmenu ? true : false;

		if (message_key_routine)
			(*message_key_routine)(ch);

		S_StartFX(sfx_swtchx);
		return true;
	}
	else if (msg_mode == 2)
	{
		if (ch == KEYD_ENTER || ch == KEYD_JOY1)
		{
			menuactive = msg_lastmenu ? true : false;
			msg_mode = 0;

			if (message_input_routine)
				(*message_input_routine)(input_string.c_str());

			input_string.clear();

			M_ClearMenus();
			S_StartFX(sfx_swtchx);
			return true;
		}

		if (ch == KEYD_ESCAPE || ch == KEYD_JOY2)
		{
			menuactive = msg_lastmenu ? true : false;
			msg_mode = 0;

			if (message_input_routine)
				(*message_input_routine)(NULL);

			input_string.clear();

			M_ClearMenus();
			S_StartFX(sfx_swtchx);
			return true;
		}

		if ((ch == KEYD_BACKSPACE || ch == KEYD_DELETE) && !input_string.empty())
		{
			std::string s = input_string.c_str(); //TODO: V811 https://www.viva64.com/en/w/v811/ Decreased performance. Excessive type casting: string -> char * -> string. Consider inspecting the 'input_string.c_str()' expression. //TODO: V808 https://www.viva64.com/en/w/v808/ 's' object of 'basic_string' type was created but was not utilized.

			if (input_string.size() > 0) //TODO: V547 https://www.viva64.com/en/w/v547/ Expression 'input_string.size() > 0' is always true.
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
	if (option_menuon)
		return M_OptResponder(ev, ch);

	if (netgame_menuon)
		return M_NetGameResponder(ev, ch);

	if (splitgame_menuon)
		return M_SplitGameResponder(ev, ch);

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
				M_DoSave(save_page, save_slot);
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
			currentMenu = &SoundDef;
			itemOn = sfx_vol;
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

			r_gamma++;
			if (r_gamma > 5)
				r_gamma = 0;

			const char *msg = NULL;

			switch (r_gamma)
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
		if (ch == KEYD_ESCAPE || ch == KEYD_JOY7)
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
	case KEYD_JOY13:
		do
		{
			if (itemOn + 1 > currentMenu->numitems - 1)
				itemOn = 0;
			else
				itemOn++;
			S_StartFX(sfx_pstop);
		} while (currentMenu->menuitems[itemOn].status == -1);
		return true;

	case KEYD_UPARROW:
	case KEYD_WHEEL_UP:
	case KEYD_JOY12:
		do
		{
			if (itemOn == 0)
				itemOn = currentMenu->numitems - 1;
			else
				itemOn--;
			S_StartFX(sfx_pstop);
		} while (currentMenu->menuitems[itemOn].status == -1);
		return true;

	case KEYD_PGUP:
	case KEYD_LEFTARROW:
	case KEYD_JOY14:
	//case KEYD_AXIS3:
		if (currentMenu->menuitems[itemOn].select_func &&
			currentMenu->menuitems[itemOn].status == 2)
		{
			S_StartFX(sfx_stnmov);
			// 98-7-10 KM Use new defines
			(*currentMenu->menuitems[itemOn].select_func)(SLIDERLEFT);
		}
		return true;

	case KEYD_PGDN:
	case KEYD_RIGHTARROW:
	case KEYD_JOY15:
	//case KEYD_AXIS4:
		if (currentMenu->menuitems[itemOn].select_func &&
			currentMenu->menuitems[itemOn].status == 2)
		{
			S_StartFX(sfx_stnmov);
			// 98-7-10 KM Use new defines
			(*currentMenu->menuitems[itemOn].select_func)(SLIDERRIGHT);
		}
		return true;

	case KEYD_ENTER:
	case KEYD_MOUSE1:
	case KEYD_JOY1:
		if (currentMenu->menuitems[itemOn].select_func &&
			currentMenu->menuitems[itemOn].status)
		{
			currentMenu->lastOn = itemOn;
			(*currentMenu->menuitems[itemOn].select_func)(itemOn);
			S_StartFX(sfx_pistol);
		}
		return true;

	case KEYD_ESCAPE:
	case KEYD_MOUSE2:
	case KEYD_MOUSE3:
	case KEYD_JOY2:
		currentMenu->lastOn = itemOn;
		M_ClearMenus();
		S_StartFX(sfx_swtchx);
		return true;

	case KEYD_BACKSPACE:
	//case KEYD_JOY2:
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

void M_StartControlPanel(void)
{
	// intro might call this repeatedly
	if (menuactive)
		return;

	menuactive = true;
	CON_SetVisible(vs_notvisible);
	paused = false; // extra check to disable graphic from con_setvisible

	if (heretic_mode)
	{
		currentMenu = &HereticMainDef;
	}
	else if (rott_mode)
	{
		currentMenu = &ROTTMainDef;
	}
	else
	{
		currentMenu = &MainDef;  // JDC
	}

	itemOn = currentMenu->lastOn;  // JDC

	//Disable this for now.
	//M_OptCheckNetgame();
}

static int FindChar(std::string& str, char ch, int pos)
{
	SYS_ASSERT(pos <= (int)str.size());

	const char *scan = strchr(str.c_str() + pos, ch);

	if (!scan)
		return -1;

	return (int)(scan - str.c_str());
}

static std::string GetMiddle(std::string& str, int pos, int len)
{
	SYS_ASSERT(pos >= 0 && len >= 0);
	SYS_ASSERT(pos + len <= (int)str.size());

	if (len == 0)
		return std::string();

	return std::string(str.c_str() + pos, len);
}

static void DrawMessage(void)
{
	short x, y;

	HUD_SetAlpha(0.64f);
	HUD_SolidBox(0, 0, 320, 200, T_BLACK);
	dialog_style->DrawBackground();
	// disable for test : dialog_style->DrawBackground(); //to replace above call
	HUD_SetAlpha();

	// FIXME: HU code should support center justification: this
	// would remove the code duplication below...

	std::string msg(msg_string);

	std::string input(input_string);

	if (msg_mode == 2)
		input += "_";

	// Calc required height
	SYS_ASSERT(dialog_style);

	std::string s = msg + input;

	y = 100 - (dialog_style->fonts[0]->StringLines(s.c_str()) *
		dialog_style->fonts[0]->NominalHeight() / 2);

	if (!msg.empty())
	{
		int oldpos = 0;
		int pos;

		do
		{
			pos = FindChar(msg, '\n', oldpos);

			if (pos < 0)
				s = std::string(msg, oldpos);
			else
				s = GetMiddle(msg, oldpos, pos - oldpos);

			if (s.size() > 0)
			{
				x = 160 - (dialog_style->fonts[0]->StringWidth(s.c_str()) / 2);
				HL_WriteText(dialog_style, 0, x, y, s.c_str());
			}

			y += dialog_style->fonts[0]->NominalHeight();

			oldpos = pos + 1;
		} while (pos >= 0 && oldpos < (int)msg.size());
	}

	if (!input.empty())
	{
		int oldpos = 0;
		int pos;

		do
		{
			pos = FindChar(input, '\n', oldpos);

			if (pos < 0)
				s = std::string(input, oldpos);
			else
				s = GetMiddle(input, oldpos, pos - oldpos);

			if (s.size() > 0)
			{
				x = 160 - (dialog_style->fonts[1]->StringWidth(s.c_str()) / 2);
				HL_WriteText(dialog_style, 1, x, y, s.c_str());
			}

			y += dialog_style->fonts[1]->NominalHeight();

			oldpos = pos + 1;
		} while (pos >= 0 && oldpos < (int)input.size());
	}
}

static void ROTTMessage(void)
{
	short x, y;

	const image_c *MESSAGEBOX = W_ImageLookup("M_BOX");
	float w = IM_TOTAL_WIDTH(MESSAGEBOX);
	float h = IM_TOTAL_HEIGHT(MESSAGEBOX);

	HUD_SetAlpha(0.64f);
	HUD_StretchImage(0, 0, 320, 200, MESSAGEBOX);
	dialog_style->DrawBackground();
	// disable for test : dialog_style->DrawBackground(); //to replace above call
	HUD_SetAlpha();

	// FIXME: HU code should support center justification: this
	// would remove the code duplication below...

	std::string msg(msg_string);

	std::string input(input_string);

	if (msg_mode == 2)
		input += "_";

	// Calc required height
	SYS_ASSERT(dialog_style);

	std::string s = msg + input;

	y = 100 - (dialog_style->fonts[0]->StringLines(s.c_str()) *
		dialog_style->fonts[0]->NominalHeight() / 2);

	if (!msg.empty())
	{
		int oldpos = 0;
		int pos;

		do
		{
			pos = FindChar(msg, '\n', oldpos);

			if (pos < 0)
				s = std::string(msg, oldpos);
			else
				s = GetMiddle(msg, oldpos, pos - oldpos);

			if (s.size() > 0)
			{
				x = 160 - (dialog_style->fonts[0]->StringWidth(s.c_str()) / 2);
				HL_WriteText(dialog_style, 0, x, y, s.c_str());
			}

			y += dialog_style->fonts[0]->NominalHeight();

			oldpos = pos + 1;
		} while (pos >= 0 && oldpos < (int)msg.size());
	}

	if (!input.empty())
	{
		int oldpos = 0;
		int pos;

		do
		{
			pos = FindChar(input, '\n', oldpos);

			if (pos < 0)
				s = std::string(input, oldpos);
			else
				s = GetMiddle(input, oldpos, pos - oldpos);

			if (s.size() > 0)
			{
				x = 160 - (dialog_style->fonts[1]->StringWidth(s.c_str()) / 2);
				HL_WriteText(dialog_style, 1, x, y, s.c_str());
			}

			y += dialog_style->fonts[1]->NominalHeight();

			oldpos = pos + 1;
		} while (pos >= 0 && oldpos < (int)input.size());
	}
}

//
// Called after the view has been rendered,
// but before it has been blitted.
//
void M_Drawer(void)
{
	short x, y;

	unsigned int i;
	unsigned int max;

	if (!menuactive)
		return;

	if (heretic_mode)
	{
		H_Drawer();
		return;
	}

	if (rott_mode)
	{
		ROTT_Drawer();
		return;
	}


	// Horiz. & Vertically center string and print it.
	if (msg_mode)
	{
		DrawMessage();
		return;
	}

	// new options menu enable, use that drawer instead
	if (option_menuon)
	{
		M_OptDrawer();
		return;
	}

	if (netgame_menuon)
	{
		M_NetGameDrawer();
		return;
	}

	if (splitgame_menuon)
	{
		M_SplitGameDrawer();
		return;
	}

	style_c *style = currentMenu->style_var[0];
	//SYS_ASSERT(style);
	HUD_SetAlpha(0.64f);
	HUD_SolidBox(0, 0, 320, 200, T_BLACK);
	HUD_SetAlpha();

	// call Draw routine
	if (currentMenu->draw_func)
		(*currentMenu->draw_func)();

	// DRAW MENU
	x = currentMenu->x;
	y = currentMenu->y;
	max = currentMenu->numitems;

	for (i = 0; i < max; i++, y += LINEHEIGHT)
	{
		// ignore blank lines
		if (!currentMenu->menuitems[i].patch_name[0])
			continue;

		if (!currentMenu->menuitems[i].image)
			currentMenu->menuitems[i].image = W_ImageLookup(
				currentMenu->menuitems[i].patch_name);

		const image_c *image = currentMenu->menuitems[i].image;

		HUD_DrawImage(x, y, image);
	}

	// DRAW SKULL

#if 0
	if (heretic_mode)
	{
#define SKULLXOFF   -24
#define LINEHEIGHT   15  //!!!!
		int sx = x + SKULLXOFF;
		int sy = currentMenu->y - 5 + itemOn * LINEHEIGHT;

		HUD_DrawImage(sx, sy, menu_skull[whichSkull]);
	}

	else if (!heretic_mode)
	{
#endif // 0

		int sx = x + SKULLXOFF;
		int sy = currentMenu->y - 5 + itemOn * LINEHEIGHT;

		HUD_DrawImage(sx, sy, menu_skull[0]);
}

//
// Called after the view has been rendered,
// but before it has been blitted.
//
void ROTT_Drawer(void)
{
	short x, y;

	unsigned int i;
	unsigned int max;

	if (!menuactive)
		return;

	// Horiz. & Vertically center string and print it.
	if (msg_mode)
	{
		ROTTMessage();
		return;
	}

	// new options menu enable, use that drawer instead
	if (option_menuon)
	{
		M_OptDrawer();
		return;
	}

	if (netgame_menuon)
	{
		M_NetGameDrawer();
		return;
	}

	if (splitgame_menuon)
	{
		M_SplitGameDrawer();
		return;
	}

	const image_c *plane_flip = W_ImageLookup("PLANE2");
	float w = IM_TOTAL_WIDTH(plane_flip);
	float h = IM_TOTAL_HEIGHT(plane_flip);

	style_c *style = currentMenu->style_var[0];
	SYS_ASSERT(style);
	HUD_SetAlpha(0.64f);
	HUD_DrawImage(288, 158, plane_flip); //w,h should probably be...0, 0? second w/h should probably also be 288x158
	HUD_SetAlpha();

	// call Draw routine
	if (currentMenu->draw_func)
		(*currentMenu->draw_func)();

	// DRAW MENU
	x = currentMenu->x;
	y = currentMenu->y;
	max = currentMenu->numitems;

	for (i = 0; i < max; i++, y += LINEHEIGHT)
	{
		// ignore blank lines
		if (!currentMenu->menuitems[i].patch_name[0])
			continue;

		if (!currentMenu->menuitems[i].image)
			currentMenu->menuitems[i].image = W_ImageLookup(
				currentMenu->menuitems[i].patch_name);

		const image_c *image = currentMenu->menuitems[i].image;

		HUD_DrawImage(x, y, image);
	}

	// DRAW SKULL

	int sx = x + SKULLXOFF;
	int sy = currentMenu->y - 5 + itemOn * LINEHEIGHT;



	HUD_DrawImage(sx, sy, rott_skull);

	//}
}

void H_Drawer(void)
{
	short x, y;

	unsigned int i;
	unsigned int max;

	if (!menuactive)
		return;

	// Horiz. & Vertically center string and print it.
	if (msg_mode)
	{
		DrawMessage();
		return;
	}

	// new options menu enable, use that drawer instead
	if (option_menuon)
	{
		M_OptDrawer();
		return;
	}

	if (netgame_menuon)
	{
		M_NetGameDrawer();
		return;
	}

	if (splitgame_menuon)
	{
		M_SplitGameDrawer();
		return;
	}

	style_c *style = currentMenu->style_var[0];
	SYS_ASSERT(style);
	HUD_SetAlpha(0.64f);
	HUD_SolidBox(0, 0, 320, 200, T_BLACK);
	HUD_SetAlpha();

	// call Draw routine
	if (currentMenu->draw_func)
		(*currentMenu->draw_func)();

	// DRAW MENU
	x = currentMenu->x;
	y = currentMenu->y;
	max = currentMenu->numitems;

	for (i = 0; i < max; i++, y += HLINEHEIGHT)
	{
		// ignore blank lines
		if (!currentMenu->menuitems[i].patch_name[0])
			continue;

		if (!currentMenu->menuitems[i].image)
			currentMenu->menuitems[i].image = W_ImageLookup(
				currentMenu->menuitems[i].patch_name);

		const image_c *image = currentMenu->menuitems[i].image;

		HUD_DrawImage(x, y, image);
	}

	// DRAW SKULL

	int sx = x + HSKULLXOFF;
	int sy = currentMenu->y - 5 + itemOn * HLINEHEIGHT;

	HUD_DrawImage(sx, sy, menu_skull[0]);
	//}
}

void M_ClearMenus(void)
{
	// -AJA- 2007/12/24: save user changes ASAP (in case of crash)
	if (menuactive)
	{
		M_SaveDefaults();
	}

	menuactive = false;
	save_screenshot_valid = false;
}

void M_SetupNextMenu(menu_t * menudef)
{
	currentMenu = menudef;
	itemOn = currentMenu->lastOn;
}

//
// M_MenuFadeIn / M_MenuFadeOut
//

#if 0
	void M_MenuFadeIn(void)
	{
		int fadetime = (int)(m_menufadetime.value + 20);

		if ((menualphacolor + fadetime) < 0xff) //0xff = 25
		{
			menualphacolor += fadetime;
		}
		else
		{
			menualphacolor = 0xff;
			alphaprevmenu = false;
			menufadefunc = NULL;
			nextmenu = NULL;
		}
	}

	//
	// M_MenuFadeOut
	//

	void M_MenuFadeOut(void)
	{
		int fadetime = (int)(m_menufadetime.value + 20);

		if (menualphacolor > fadetime) {
			menualphacolor -= fadetime;
		}
		else {
			menualphacolor = 0;

			if (alphaprevmenu == false) {
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
#endif // 0

void M_Ticker(void)
{
	// update language if it changed
	if (m_language_cv_.CheckModified())
		if (!language.Select(m_language.c_str()))
			I_Printf("Unknown language: %s\n", m_language.c_str());

	if (option_menuon)
	{
		M_OptTicker();
		return;
	}

	if (netgame_menuon)
	{
		M_NetGameTicker();
		return;
	}

	if (splitgame_menuon)
	{
		M_NetGameTicker();
		return;
	}

	if (--skullAnimCounter <= 0)
	{
		whichSkull ^= 1;
		skullAnimCounter = 8;
	}

	if (--goldskullAnimCounter <= 0)
	{
		whichgoldSkull ^= 1;
		goldskullAnimCounter = 17;
	}
}

void H_Init(void)
{
	if (!heretic_mode)
	{
		I_Printf("Heretic mode not detected, breaking into M_Init!\n");
		return;
	}

	I_Printf("- Heretic Main Menu Init\n");

	E_ProgressMessage(language["MiscInfo"]);

	currentMenu = &HereticMainDef;
	menuactive = false;
	itemOn = currentMenu->lastOn;
	whichSkull = 0;
	skullAnimCounter = 10;
	whichgoldSkull = 0;
	goldskullAnimCounter = 17;
	msg_mode = 0;
	msg_string.clear();
	msg_lastmenu = menuactive;
	quickSaveSlot = -1;

	// lookup styles
	styledef_c *def;

	def = styledefs.Lookup("MENU");
	if (!def) def = default_style;
	menu_def_style = hu_styles.Lookup(def);

	def = styledefs.Lookup("MULTIPLAYER");
	if (!def) def = default_style;
	menu_def_style = hu_styles.Lookup(def);

	def = styledefs.Lookup("OPTIONS");
	if (!def) def = default_style;
	menu_def_style = hu_styles.Lookup(def);

	def = styledefs.Lookup("MAIN MENU");
	main_menu_style = def ? hu_styles.Lookup(def) : menu_def_style;

	def = styledefs.Lookup("CHOOSE EPISODE");
	episode_style = def ? hu_styles.Lookup(def) : menu_def_style;

	def = styledefs.Lookup("CHOOSE SKILL");
	skill_style = def ? hu_styles.Lookup(def) : menu_def_style;

	def = styledefs.Lookup("GAME FILES");
	files_menu_style = def ? hu_styles.Lookup(def) : menu_def_style;

	def = styledefs.Lookup("LOAD MENU");
	load_style = def ? hu_styles.Lookup(def) : menu_def_style;

	def = styledefs.Lookup("SAVE MENU");
	save_style = def ? hu_styles.Lookup(def) : menu_def_style;

	def = styledefs.Lookup("DIALOG");
	dialog_style = def ? hu_styles.Lookup(def) : menu_def_style;

	def = styledefs.Lookup("SOUND VOLUME");
	if (!def) def = styledefs.Lookup("OPTIONS");
	if (!def) def = default_style;
	sound_vol_style = hu_styles.Lookup(def);

	// lookup required images

	therm_l = W_ImageLookup("H_THERML");

	therm_m = W_ImageLookup("H_THERMM");

	therm_r = W_ImageLookup("H_THERMR");

	therm_o = W_ImageLookup("H_THERMO"); //Heretic: M_SLDKB */

	menu_loadg = W_ImageLookup("H_LOADG");
	menu_saveg = W_ImageLookup("H_SAVEG");
	menu_svol = W_ImageLookup("H_SNDOPT");
	menu_newgame = W_ImageLookup("H_NGAME");
	menu_multiplayer = W_ImageLookup("H_MULTI");
	///menu_skill = W_ImageLookup("H_SKILL");
	menu_skill = W_ImageLookup("NULL");
	menu_episode = W_ImageLookup("H_EPISOD");

	menu_doom = W_ImageLookup("M_HTIC");

	// Menu Skull code. Notice the int is [2], but this holds [0] and [1], which as in integer = 2!
	// Heretic logic: int is [17], but should go from [0] to [16], which as an intereger = 17!
	menu_skull[0] = W_ImageLookup("M_SLCTR1");
	menu_skull[1] = W_ImageLookup("M_SLCTR2");

	SkullBaseLump[0] = W_ImageLookup("M_SKL00"); // iterate to 18
	SkullBaseLump[1] = W_ImageLookup("M_SKL01");
	SkullBaseLump[2] = W_ImageLookup("M_SKL02");
	SkullBaseLump[3] = W_ImageLookup("M_SKL03");
	SkullBaseLump[4] = W_ImageLookup("M_SKL04");
	SkullBaseLump[5] = W_ImageLookup("M_SKL05");
	SkullBaseLump[6] = W_ImageLookup("M_SKL06");
	SkullBaseLump[7] = W_ImageLookup("M_SKL07");
	SkullBaseLump[8] = W_ImageLookup("M_SKL08");
	SkullBaseLump[9] = W_ImageLookup("M_SKL09");
	SkullBaseLump[10] = W_ImageLookup("M_SKL10");
	SkullBaseLump[11] = W_ImageLookup("M_SKL11");
	SkullBaseLump[12] = W_ImageLookup("M_SKL12");
	SkullBaseLump[13] = W_ImageLookup("M_SKL13");
	SkullBaseLump[14] = W_ImageLookup("M_SKL14");
	SkullBaseLump[15] = W_ImageLookup("M_SKL15");
	SkullBaseLump[16] = W_ImageLookup("M_SKL16");
	SkullBaseLump[17] = W_ImageLookup("M_SKL17");

	// Further code switches out DOOM -> Heretic graphics

	// Here we could catch other version dependencies,
	//  like HELP1/2, and four episodes.
	//    if (W_CheckNumForName("M_EPI4") < 0)
	//      EpiDef.numitems -= 2;
	//    else if (W_CheckNumForName("M_EPI5") < 0)
	//      EpiDef.numitems--;

	if (W_CheckNumForName("HELP") >= 0)
		menu_readthis[0] = W_ImageLookup("HELP");
	else
		menu_readthis[0] = W_ImageLookup("HELP1");

	if (W_CheckNumForName("HELP2") >= 0)
		menu_readthis[1] = W_ImageLookup("HELP2");
	else
	{
		menu_readthis[1] = W_ImageLookup("CREDIT");

		// This is used because DOOM 2 had only one HELP
		//  page. I use CREDIT as second page now, but
		//  kept this hack for educational purposes.

		HereticMainMenu[hreadthis] = HereticMainMenu[quitheretic];
		HereticMainDef.numitems--;
		HereticMainDef.y += 8; // FIXME
		SkillDef.prevMenu = &HereticMainDef;
		HReadDef1.draw_func = M_DrawReadThis1;
		HReadDef1.x = 330;
		HReadDef1.y = 165;
		HReadMenu1[0].select_func = H_FinishReadThis;
	}

	sfx_swtchn = sfxdefs.GetEffect("SWTCHN");
	sfx_tink = sfxdefs.GetEffect("TINK");
	sfx_radio = sfxdefs.GetEffect("RADIO");
	sfx_oof = sfxdefs.GetEffect("OOF");
	sfx_pstop = sfxdefs.GetEffect("PSTOP");
	sfx_stnmov = sfxdefs.GetEffect("STNMOV");
	sfx_pistol = sfxdefs.GetEffect("PISTOL");
	sfx_swtchx = sfxdefs.GetEffect("SWTCHX");
	sfx_network = sfxdefs.GetEffect("DSSECRET");

	M_OptMenuInit();
	M_NetGameInit();

	M_InitShiftXForm();
}

void M_Init(void)
{
	if (heretic_mode)
	{
		H_Init();
		return;
	}

	E_ProgressMessage(language["MiscInfo"]);

	currentMenu = &MainDef;
	menuactive = false;
	itemOn = currentMenu->lastOn;
	whichSkull = 0;
	skullAnimCounter = 15;
	msg_mode = 0;
	msg_string.clear();
	msg_lastmenu = menuactive;
	quickSaveSlot = -1;

	// lookup styles
	styledef_c *def;

	def = styledefs.Lookup("MENU");
	if (!def) def = default_style;
	menu_def_style = hu_styles.Lookup(def);

	def = styledefs.Lookup("MULTIPLAYER");
	if (!def) def = default_style;
	menu_def_style = hu_styles.Lookup(def);

	def = styledefs.Lookup("OPTIONS");
	if (!def) def = default_style;
	menu_def_style = hu_styles.Lookup(def);

	def = styledefs.Lookup("MAIN MENU");
	main_menu_style = def ? hu_styles.Lookup(def) : menu_def_style;

	def = styledefs.Lookup("CHOOSE EPISODE");
	episode_style = def ? hu_styles.Lookup(def) : menu_def_style;

	def = styledefs.Lookup("CHOOSE SKILL");
	skill_style = def ? hu_styles.Lookup(def) : menu_def_style;

	def = styledefs.Lookup("LOAD MENU");
	load_style = def ? hu_styles.Lookup(def) : menu_def_style;

	def = styledefs.Lookup("SAVE MENU");
	save_style = def ? hu_styles.Lookup(def) : menu_def_style;

	def = styledefs.Lookup("DIALOG");
	dialog_style = def ? hu_styles.Lookup(def) : menu_def_style;

	def = styledefs.Lookup("SOUND VOLUME");
	if (!def) def = styledefs.Lookup("OPTIONS");
	if (!def) def = default_style;
	sound_vol_style = hu_styles.Lookup(def);

	// lookup required images
	therm_l = W_ImageLookup("M_THERML");
	therm_m = W_ImageLookup("M_THERMM");
	therm_r = W_ImageLookup("M_THERMR");
	therm_o = W_ImageLookup("M_THERMO");

	//if (W_CheckNumForName("TRESTORE") >= 0)
	//	menu_loadg = W_ImageLookup("TRESTORE");
	//else
	menu_loadg = W_ImageLookup("M_LOADG");

	//if (W_CheckNumForName("TSAVE") >= 0)
	//	menu_saveg = W_ImageLookup("TSAVE");
	//else
	menu_saveg = W_ImageLookup("M_SAVEG");

	menu_svol = W_ImageLookup("M_SVOL");

	if (rott_mode)//(W_CheckNumForName("TNGAME") >= 0)
		menu_newgame = W_ImageLookup("TNGAME");
	else
		menu_newgame = W_ImageLookup("M_NEWG");

	menu_multiplayer = W_ImageLookup("M_MULTI");

	menu_skill = W_ImageLookup("M_SKILL");

	menu_episode = W_ImageLookup("M_EPISOD");

	menu_skull[0] = W_ImageLookup("M_SKULL1");
	menu_skull[1] = W_ImageLookup("M_SKULL2");

	if (rott_mode)
		rott_skull = W_ImageLookup("CURSOR01");

	//	if (W_CheckNumForName("M_NEWG") >= 0)
	//	    DrawKeyword("NEW GAME");//HL_WriteText(style,2, 80, 30, "NEW GAME");//HUD_DrawText(0, 0, "NEW GAME");//HL_WriteText(style,2, LoadDef.x - 4, y, "NEW GAME");
	//or
	// menu_doom = HL_WriteText(style,2,LoadDef.x,y, "NEW GAME");
	if (W_CheckNumForName("M_HTIC") >= 0)
		menu_doom = W_ImageLookup("M_HTIC");
	else if (W_CheckNumForName("TMAIN") >= 0)
		menu_doom = W_ImageLookup("TMAIN");
	else
		if (W_CheckNumForName("M_DOOM") >= 0)
			menu_doom = W_ImageLookup("M_DOOM");

	//code below switches out skull
	if (W_CheckNumForName("M_SLCTR1") >= 0)
		menu_skull[0] = W_ImageLookup("M_SLCTR1");
	else if (W_CheckNumForName("CURSOR01") >= 0)
		menu_skull[0] = W_ImageLookup("CURSOR01");
	else
		menu_skull[0] = W_ImageLookup("M_SKULL1");

	if (W_CheckNumForName("M_SLCTR2") >= 0)
		menu_skull[1] = W_ImageLookup("M_SLCTR2");
	else
		menu_skull[1] = W_ImageLookup("M_SKULL2");

	// Further code switches out DOOM -> Heretic graphics

	// Here we could catch other version dependencies,
	//  like HELP1/2, and four episodes.
	//    if (W_CheckNumForName("M_EPI4") < 0)
	//      EpiDef.numitems -= 2;
	//    else if (W_CheckNumForName("M_EPI5") < 0)
	//      EpiDef.numitems--;

	if (W_CheckNumForName("HELP") >= 0)
		menu_readthis[0] = W_ImageLookup("HELP");
	else
		menu_readthis[0] = W_ImageLookup("HELP1");

	if (W_CheckNumForName("HELP2") >= 0)
		menu_readthis[1] = W_ImageLookup("HELP2");
	else
	{
		menu_readthis[1] = W_ImageLookup("CREDIT");

		// This is used because DOOM 2 had only one HELP
		//  page. I use CREDIT as second page now, but
		//  kept this hack for educational purposes.

		MainMenu[readthis] = MainMenu[quitdoom];
		MainDef.numitems--;
		MainDef.y += 8; // FIXME
		SkillDef.prevMenu = &MainDef;
		ReadDef1.draw_func = M_DrawReadThis1;
		ReadDef1.x = 330;
		ReadDef1.y = 165;
		ReadMenu1[0].select_func = M_FinishReadThis;
	}

	sfx_swtchn = sfxdefs.GetEffect("SWTCHN");
	sfx_tink = sfxdefs.GetEffect("TINK");
	sfx_radio = sfxdefs.GetEffect("RADIO");
	sfx_oof = sfxdefs.GetEffect("OOF");
	sfx_pstop = sfxdefs.GetEffect("PSTOP");
	sfx_stnmov = sfxdefs.GetEffect("STNMOV");
	sfx_pistol = sfxdefs.GetEffect("PISTOL");
	sfx_swtchx = sfxdefs.GetEffect("SWTCHX");

	M_OptMenuInit();
	M_NetGameInit();

	M_InitShiftXForm();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab