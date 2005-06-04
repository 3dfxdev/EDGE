//----------------------------------------------------------------------------
//  EDGE Main Menu Code
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
// See M_Option.C for text built menus.
//
// -KM- 1998/07/21 Add support for message input.
//

#include "i_defs.h"
#include "m_menu.h"

#include "dm_defs.h"
#include "dm_state.h"

#include "con_main.h"
#include "ddf_main.h"
#include "dstrings.h"
#include "e_main.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "hu_style.h"
#include "m_argv.h"
#include "m_inline.h"
#include "m_misc.h"
#include "m_option.h"
#include "m_random.h"
#include "n_network.h"
#include "p_setup.h"
#include "r_local.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "sv_chunk.h"
#include "sv_main.h"
#include "v_ctx.h"
#include "v_res.h"
#include "v_colour.h"
#include "w_wad.h"
#include "wi_stuff.h"
#include "z_zone.h"

//
// defaulted values
//
int mouseSensitivity;  // has default.  Note: used only in platform code

// Show messages has default, 0 = off, 1 = on
int showMessages;

int screen_hud;  // has default

static epi::strent_c msg_string;
static int msg_lastmenu;
static int msg_mode;

static epi::strent_c input_string;		

bool menuactive;

#define SKULLXOFF   -24
#define LINEHEIGHT   15  //!!!!

// timed message = no input from user
static bool msg_needsinput;

static void (* message_key_routine)(int response) = NULL;
static void (* message_input_routine)(const char *response) = NULL;

static int chosen_epi;

//
//  IMAGES USED
//
static const image_t *therm_l;
static const image_t *therm_m;
static const image_t *therm_r;
static const image_t *therm_o;

static const image_t *menu_loadg;
static const image_t *menu_saveg;
static const image_t *menu_svol;
static const image_t *menu_doom;
static const image_t *menu_newgame;
static const image_t *menu_skill;
static const image_t *menu_episode;
static const image_t *menu_skull[2];
static const image_t *menu_readthis[2];

static style_c *menu_def_style;
static style_c *main_menu_style;
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
static int save_slot = 0;

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


//
// MENU TYPEDEFS
//
typedef struct
{
	// 0 = no cursor here, 1 = ok, 2 = arrows ok
	int status;

  	// image for menu entry
	char patch_name[10];
	const image_t *image;

  	// choice = menu item #.
  	// if status = 2, choice can be SLIDERLEFT or SLIDERRIGHT
	void (* select_func)(int choice);

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
	void (* draw_func)(void);

	// x,y of menu
	int x, y;

	// last item user was on in menu
	int lastOn;
}
menu_t;

// menu item skull is on
static int itemOn;

// skull animation counter
static int skullAnimCounter;

// which skull to draw
static int whichSkull;

// current menudef
static menu_t *currentMenu;

//
// PROTOTYPES
//
static void M_NewGame(int choice);
static void M_Episode(int choice);
static void M_ChooseSkill(int choice);
static void M_LoadGame(int choice);
static void M_SaveGame(int choice);
static void M_NewMultiGame(int choice);

// 25-6-98 KM
static void M_LoadSavePage(int choice);
static void M_Options(int choice);
static void M_ReadThis(int choice);
static void M_ReadThis2(int choice);
void M_EndGame(int choice);

static void M_ChangeMessages(int choice);
static void M_SfxVol(int choice);
static void M_MusicVol(int choice);
static void M_SizeDisplay(int choice);
// static void M_Sound(int choice);

static void M_FinishReadThis(int choice);
static void M_LoadSelect(int choice);
static void M_SaveSelect(int choice);
static void M_ReadSaveStrings(void);
static void M_QuickSave(void);
static void M_QuickLoad(void);

static void M_DrawMainMenu(void);
static void M_DrawReadThis1(void);
static void M_DrawReadThis2(void);
static void M_DrawNewGame(void);
static void M_DrawEpisode(void);
static void M_DrawSound(void);
static void M_DrawMultiPlayer(void);
static void M_DrawLoad(void);
static void M_DrawSave(void);

static void M_DrawSaveLoadBorder(float x, float y, int len);
static void M_SetupNextMenu(menu_t * menudef);
static void M_ClearMenus(void);
void M_StartControlPanel(void);
// static void M_StopMessage(void);

//
// DOOM MENU
//
enum
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

static menuitem_t MainMenu[] =
{
	{1, "M_NGAME",   NULL, M_NewGame, 'n'},
	{1, "M_OPTION",  NULL, M_Options, 'o'},
	{1, "M_LOADG",   NULL, M_LoadGame, 'l'},
	{1, "M_SAVEG",   NULL, M_SaveGame, 's'},
	// Another hickup with Special edition.
	{1, "M_RDTHIS",  NULL, M_ReadThis, 'r'},
	{1, "M_QUITG",   NULL, M_QuitEDGE, 'q'}
};

static menu_t MainDef =
{
	main_end,
	NULL,
	MainMenu,
	&main_menu_style,
	M_DrawMainMenu,
	97, 64,
	0
};

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
	48, 63,  // x,y
	sk_medium  // lastOn
};

//
// OPTIONS MENU
//
enum
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

static menuitem_t ReadMenu2[] =
{
	{1, "", NULL, M_FinishReadThis, 0}
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

//
// SOUND VOLUME MENU
//
enum
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

//
// MULTIPLAYER MENU
//

static menuitem_t MultiPlayerMenu[] =
{
	{2, "M_NGAME", NULL, M_NewMultiGame, 'n'},
};

static menu_t MultiPlayerDef =
{
	1,
	&MainDef,
	MultiPlayerMenu,
	&dialog_style,
	M_DrawMultiPlayer,
	80, 104,
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
			if (save_page >= SAVE_PAGES-1)
				return;

			save_page++;
			break;
	}

	S_StartSound(NULL, sfx_swtchn);
	M_ReadSaveStrings();
}

//
// M_ReadSaveStrings
//
// Read the strings from the savegame files
//
// 98-7-10 KM Savegame slots increased
//
void M_ReadSaveStrings(void)
{
	int i, version;
  
	epi::string_c fn;
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
    
		G_FileNameFromSlot(fn, save_page * SAVE_SLOTS + i);

		if (! SV_OpenReadFile(fn.GetString()))
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
			strncpy(ex_slots[i].desc, language["CorruptSlot"],
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
		HL_WriteText(style,2, LoadDef.x - 4, y, "< PREV");

	HL_WriteText(style,2, LoadDef.x + 94 - style->fonts[2]->StringWidth(mbuffer) / 2, y,
					  mbuffer);

	if (save_page < SAVE_PAGES-1)
		HL_WriteText(style,2, LoadDef.x + 192 - style->fonts[2]->StringWidth("NEXT >"), y,
						  "NEXT >");
 
	info = ex_slots + itemOn;
	DEV_ASSERT2(0 <= itemOn && itemOn < SAVE_SLOTS);

	if (saveStringEnter || info->empty || info->corrupt)
		return;

	// show some info about the savegame

	y = LoadDef.y + LINEHEIGHT * (row2 + 1);

	mbuffer[0] = 0;

	strcat(mbuffer, info->timestr);

	HL_WriteText(style,3, 310 - style->fonts[3]->StringWidth(mbuffer), y, mbuffer);


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

	HL_WriteText(style,3, 310 - style->fonts[3]->StringWidth(mbuffer), y, mbuffer);


	y -= LINEHEIGHT;
  
	mbuffer[0] = 0;

	switch (info->netgame)
	{
		case 0: strcat(mbuffer, "SP MODE"); break;
		case 1: strcat(mbuffer, "COOP MODE"); break;
		default: strcat(mbuffer, "DM MODE"); break;
	}
  
	HL_WriteText(style,3, 310 - style->fonts[3]->StringWidth(mbuffer), y, mbuffer);


	y -= LINEHEIGHT;
  
	mbuffer[0] = 0;

	strcat(mbuffer, info->mapname);

	HL_WriteText(style,3, 310 - style->fonts[3]->StringWidth(mbuffer), y, mbuffer);
}

//
// M_LoadGame
//
// 1998/07/10 KM Savegame slots increased
//
void M_DrawLoad(void)
{
	int i;

	RGL_ImageEasy320(72, 8, menu_loadg);
      
	for (i = 0; i < SAVE_SLOTS; i++)
		M_DrawSaveLoadBorder(LoadDef.x + 8, LoadDef.y + LINEHEIGHT * (i), 24);

	// draw screenshot ?

	for (i = 0; i < SAVE_SLOTS; i++)
		HL_WriteText(load_style,0, LoadDef.x + 8, LoadDef.y + LINEHEIGHT * (i), ex_slots[i].desc);

	M_DrawSaveLoadCommon(i, i+1, load_style);
}


//
// Draw border for the savegame description
//
void M_DrawSaveLoadBorder(float x, float y, int len)
{
	const image_t *L = W_ImageLookup("M_LSLEFT");
	const image_t *C = W_ImageLookup("M_LSCNTR");
	const image_t *R = W_ImageLookup("M_LSRGHT");

	RGL_ImageEasy320(x - IM_WIDTH(L), y + 7, L);

	for (int i = 0; i < len; i++, x += IM_WIDTH(C))
		RGL_ImageEasy320(x, y + 7, C);

	RGL_ImageEasy320(x, y + 7, R);
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
//  M_SaveGame
//
// 98-7-10 KM Savegame slots increased
//
void M_DrawSave(void)
{
	int i, len;

	RGL_ImageEasy320(72, 8, menu_saveg);

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
	if (!usergame)
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

	if (gamestate != GS_LEVEL)
		return;

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
		S_StartSound(NULL, sfx_swtchx);
	}
}

void M_QuickSave(void)
{
	if (!usergame)
	{
		S_StartSound(NULL, sfx_oof);
		return;
	}

	if (gamestate != GS_LEVEL)
		return;

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
	
	epi::string_c s;
	s.Format(language["QuickSaveOver"],	ex_slots[quickSaveSlot].desc);
	M_StartMessage(s.GetString(), QuickSaveResponse, true);
}

//
// M_QuickLoad
//
static void QuickLoadResponse(int ch)
{
	if (ch == 'y')
	{
		int tempsavepage = save_page;

		save_page = quickSavePage;
		M_LoadSelect(quickSaveSlot);

		save_page = tempsavepage;
		S_StartSound(NULL, sfx_swtchx);
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

	epi::string_c s;
	s.Format(language["QuickLoad"], ex_slots[quickSaveSlot].desc);
	M_StartMessage(s.GetString(), QuickLoadResponse, true);
}

//
// Read This Menus
// Had a "quick hack to fix romero bug"
//
void M_DrawReadThis1(void)
{
	RGL_Image(0, 0, SCREENWIDTH, SCREENHEIGHT, menu_readthis[0]);
}

//
// Read This Menus - optional second page.
//
void M_DrawReadThis2(void)
{
	RGL_Image(0, 0, SCREENWIDTH, SCREENHEIGHT, menu_readthis[1]);
}

//
// M_DrawSound
//
// Change Sfx & Music volumes
//
// -ACB- 1999/10/10 Sound API Volume re-added
// -ACB- 1999/11/13 Music API Volume re-added
//
void M_DrawSound(void)
{
	int musicvol;
	int soundvol;

	musicvol = S_GetMusicVolume();
	soundvol = S_GetSfxVolume();

	RGL_ImageEasy320(60, 38, menu_svol);

	M_DrawThermo(SoundDef.x, SoundDef.y + LINEHEIGHT * (sfx_vol + 1),   SND_SLIDER_NUM, soundvol, 1);
	M_DrawThermo(SoundDef.x, SoundDef.y + LINEHEIGHT * (music_vol + 1), SND_SLIDER_NUM, musicvol, 1);
}

#if 0
//
// M_Sound
//
void M_Sound(int choice)
{
	M_SetupNextMenu(&SoundDef);
}
#endif

//
// M_SfxVol
//
// -ACB- 1999/10/10 Sound API Volume re-added
//
void M_SfxVol(int choice)
{
	int soundvol;

	soundvol = S_GetSfxVolume();

	switch (choice)
	{
		case SLIDERLEFT:
			if (soundvol > 0)
				soundvol--;

			break;

		case SLIDERRIGHT:
			if (soundvol < SND_SLIDER_NUM-1)
				soundvol++;

			break;
	}

	S_SetSfxVolume(soundvol);
}

//
// M_MusicVol
//
// -ACB- 1999/10/07 Removed sound references: New Sound API
//
void M_MusicVol(int choice)
{
	int musicvol;

	musicvol = S_GetMusicVolume();

	switch (choice)
	{
		case SLIDERLEFT:
			if (musicvol > 0)
				musicvol--;

			break;

		case SLIDERRIGHT:
			if (musicvol < SND_SLIDER_NUM-1)
				musicvol++;

			break;
	}

	S_SetMusicVolume(musicvol);
}

//
// M_DrawMainMenu
//
void M_DrawMainMenu(void)
{
	RGL_ImageEasy320(94, 2, menu_doom);
}

//
// M_NewGame
//
void M_DrawNewGame(void)
{
	RGL_ImageEasy320(96, 14, menu_newgame);
	RGL_ImageEasy320(54, 38, menu_skill);
}

void M_NewGame(int choice)
{
	if (netgame && !demoplayback)
	{
		M_StartMessage(language["NewNetGame"], NULL, false);
		return;
	}

	M_SetupNextMenu(&EpiDef);
}

//
//      M_Episode
//

// -KM- 1998/12/16 Generates EpiDef menu dynamically.
static void CreateEpisodeMenu(void)
{
	if (gamedefs.GetSize() == 0)
		I_Error("No defined episodes !\n");

	EpisodeMenu = Z_ClearNew(menuitem_t, gamedefs.GetSize());

	int e = 0;
	epi::array_iterator_c it;

	for (it = gamedefs.GetBaseIterator(); it.IsValid(); it++)
	{
		gamedef_c *g = ITERATOR_TO_TYPE(it, gamedef_c*);
		if (! g) continue;

		if (W_CheckNumForName(g->firstmap.GetString()) == -1)
			continue;

		EpisodeMenu[e].status = 1;
		EpisodeMenu[e].select_func = M_Episode;
		EpisodeMenu[e].image = NULL;
		EpisodeMenu[e].alpha_key = '1' + e;

		Z_StrNCpy(EpisodeMenu[e].patch_name, g->namegraphic.GetString(), 8);
		EpisodeMenu[e].patch_name[8] = 0;

		e++;
	}

	if (e == 0)
		I_Error("No available episodes !\n");

	EpiDef.numitems  = e;
	EpiDef.menuitems = EpisodeMenu;
}


void M_DrawEpisode(void)
{
	if (!EpisodeMenu)
		CreateEpisodeMenu();
    
	RGL_ImageEasy320(54, 38, menu_episode);
}

static void ReallyDoStartLevel(skill_t skill, gamedef_c *g)
{
	newgame_params_c params;

	params.skill = skill;
	params.deathmatch = 0;

	params.random_seed = I_PureRandom();

	params.SinglePlayer(startbots);

	params.game = g;
	params.map = G_LookupMap(g->firstmap.GetString());

	if (! params.map || ! G_DeferredInitNew(params))
	{
		// 23-6-98 KM Fixed this.
		M_SetupNextMenu(&EpiDef);
		M_StartMessage(language["EpisodeNonExist"], NULL, false);
		return;
	}

	M_ClearMenus();
}

static skill_t vcc_skill;
static gamedef_c *vcc_game;

static void VerifyCompatChange(int ch)
{
	if (ch != 'y' && ch != 'n')
		return;

	if (ch == 'y')
	{
		global_flags.compat_mode =
			(global_flags.compat_mode != CM_EDGE) ? CM_EDGE : CM_BOOM;
	}

	ReallyDoStartLevel(vcc_skill, vcc_game);
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

		if (!strcmp(g->namegraphic.GetString(), EpisodeMenu[chosen_epi].patch_name))
		{
			break;
		}
	}

	// Sanity checking...
	if (! g)
	{
		I_Warning("Internal Error: no episode for '%s'.\n",
			EpisodeMenu[chosen_epi].patch_name);
		M_ClearMenus();
		return;
	}

	const mapdef_c * map = G_LookupMap(g->firstmap.GetString());
	if (! map)
	{
		I_Warning("Cannot find map for '%s' (episode %s)\n",
			g->firstmap.GetString(),
			EpisodeMenu[chosen_epi].patch_name);
		M_ClearMenus();
		return;
	}

	// Compatibility check (EDGE vs BOOM)
	int compat = P_DetectWadGameCompat(map);
	int cur_compat = (global_flags.compat_mode == CM_EDGE) ?
			MAP_CM_Edge : MAP_CM_Boom;

	if (compat != 0 && compat != cur_compat)
	{
		char msg_buf[2048];

		if (compat == (MAP_CM_Edge|MAP_CM_Boom))
			sprintf(msg_buf, language["CompatBoth"],
				(cur_compat == MAP_CM_Edge) ? "EDGE" : "BOOM");
		else
			sprintf(msg_buf, language["CompatChange"],
				(cur_compat != MAP_CM_Edge) ? "EDGE" : "BOOM",
				(cur_compat == MAP_CM_Edge) ? "EDGE" : "BOOM");

		// remember settings (Ugh!)
		vcc_skill = skill;
		vcc_game  = g;

		M_StartMessage(msg_buf, VerifyCompatChange, true);
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
	M_SetupNextMenu(&SkillDef);
}

//
// M_Options
//
void M_Options(int choice)
{
	optionsmenuon = 1;
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

//
// M_EndGame
//
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
	if (!usergame)
	{
		S_StartSound(NULL, sfx_oof);
		return;
	}

	optionsmenuon = false;

	if (netgame)
	{
		M_StartMessage(language["EndNetGame"], NULL, false);
		return;
	}

	M_StartMessage(language["EndGameCheck"], EndGameResponse, true);
}

//
// M_ReadThis
//
void M_ReadThis(int choice)
{
	M_SetupNextMenu(&ReadDef1);
}

void M_ReadThis2(int choice)
{
	M_SetupNextMenu(&ReadDef2);
}

void M_FinishReadThis(int choice)
{
	M_SetupNextMenu(&MainDef);
}

//
// M_QuitDOOM
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
					S_StartSound(NULL, sfxdefs.GetEffect(language[refname]));
					break;
				}
				i = (i + 1) % numsounds;
			}
			while (i != start);
		}

		I_WaitVBL(105);
	}

	// -ACB- 1999/09/20 New exit code order
	// Write the default config file first
	I_Printf("Saving system defaults...\n");
	M_SaveDefaults();

	I_Printf("Exiting...\n");
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
	epi::string_c ref;
	epi::string_c msg;
	
	int num_quitmessages = 1;

	// Count the quit messages
	do
	{
		ref.Format("QUITMSG%d", num_quitmessages);
		
		if (!language.IsValidRef(ref.GetString()))
			break;
			
		num_quitmessages++;	// Starts at one not zero
	}
	while (true);
	
	// We started from one, hence we take go back one
	num_quitmessages--;
	
	// -ACB- 2004/08/14 Allow fallback to just the "PressToQuit" message
	if (num_quitmessages > 0)
	{
		// Pick one at random
		ref.Format("QUITMSG%d", (M_Random() % num_quitmessages) + 1);
		
		// Construct the quit message in full
		msg.Format("%s\n\n%s", language[ref], language["PressToQuit"]);
	}
	else
	{
		msg = language["PressToQuit"];
	}
	
	// Trigger the message
	M_StartMessage(msg.GetString(), QuitResponse, true);
}

// 98-7-10 KM Use new defines
void M_SizeDisplay(int choice)
{
	switch (choice)
	{
		case SLIDERLEFT:
			if (screen_hud <= 0)
				return;

			screen_hud--;
			break;

		case SLIDERRIGHT:
			if (screen_hud+1 >= NUMHUD)
				return;

			screen_hud++;
			break;
	}

	R_SetViewSize(screen_hud);
}

void M_NewMultiGame(int choice)
{
	M_NewGame(choice); //!!!!
}

//
// M_DrawMultiPlayer
//
void M_DrawMultiPlayer(void)
{
	HL_WriteText(dialog_style,0, 80, 10, "MULTIPLAYER SETUP");

	HL_WriteText(dialog_style,1, 40, 40, "(Networking not implemented yet)");

	// @@@
}

void M_MultiplayerGame(int choice)
{
	if (usergame)  // shouldn't happen
	{
		S_StartSound(NULL, sfx_oof);
		return;
	}

	optionsmenuon = false;

	M_SetupNextMenu(&MultiPlayerDef);
}

//----------------------------------------------------------------------------
//   MENU FUNCTIONS
//----------------------------------------------------------------------------

//
// M_DrawThermo
//
void M_DrawThermo(int x, int y, int thermWidth, int thermDot, int div)
{
	int i, basex = x;
	int step = (8 / div);

	// Note: the (step+1) here is for compatibility with the original
	// code.  It seems required to make the thermo bar tile properly.

	RGL_Image320(x, y, step+1, IM_HEIGHT(therm_l)/div, therm_l);

	for (i=0, x += step; i < thermWidth; i++, x += step)
	{
		RGL_Image320(x, y, step+1, IM_HEIGHT(therm_m)/div, therm_m);
	}

	RGL_Image320(x, y, step+1, IM_HEIGHT(therm_r)/div, therm_r);

	x = basex + step + thermDot * step;

	RGL_Image320(x, y, step+1, IM_HEIGHT(therm_o)/div, therm_o);
}

//
// M_StartMessage
//
void M_StartMessage(const char *string, void (* routine)(int response), 
					bool input)
{
	msg_lastmenu = menuactive;
	msg_mode = 1;
	msg_string.Set(string);
	message_key_routine = routine;
	message_input_routine = NULL;
	msg_needsinput = input;
	menuactive = true;
	CON_SetVisible(vs_notvisible);
	return;
}

//
// M_StartMessageInput
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
						 void (* routine)(const char *response))
{
	msg_lastmenu = menuactive;
	msg_mode = 2;
	msg_string.Set(string);
	message_input_routine = routine;
	message_key_routine = NULL;
	msg_needsinput = true;
	menuactive = true;
	CON_SetVisible(vs_notvisible);
	return;
}

#if 0
//
// M_StopMessage
//
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
// M_Responder
//
// -KM- 1998/09/01 Analogue binding, and hat support
//
bool M_Responder(event_t * ev)
{
	int ch;
	int i;

	if (ev->type != ev_keydown)
		return false;

	ch = ev->value.key;

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

		S_StartSound(NULL, sfx_swtchx);
		return true;
	}
	else if (msg_mode == 2)
	{		
		if (ch == KEYD_ENTER)
		{
			menuactive = msg_lastmenu?true:false;
			msg_mode = 0;

			if (message_input_routine)
				(* message_input_routine)(input_string.GetString());

			input_string.Clear();
			
			menuactive = false;
			S_StartSound(NULL, sfx_swtchx);
			return true;
		}

		if (ch == KEYD_ESCAPE)
		{
			menuactive = msg_lastmenu?true:false;
			msg_mode = 0;
      
			if (message_input_routine)
				(* message_input_routine)(NULL);

			menuactive = false;
			save_screenshot_valid = false;

			input_string.Clear();
			
			S_StartSound(NULL, sfx_swtchx);
			return true;
		}
		
		if (ch == KEYD_BACKSPACE && !input_string.IsEmpty())
		{
			epi::string_c s = input_string.GetString();
			if (s.GetLength() > 0)
			{
				s.RemoveRight(1);
				input_string.Set(s.GetString());
			}
				
			return true;
		}
		
		ch = toupper(ch);
		if (ch == '-')
			ch = '_';
			
		if (ch >= 32 && ch <= 127)  // FIXME: international characters ??
		{
			epi::string_c s;

			if (!input_string.IsEmpty())
				s = input_string;

			s += ch;
			
			// Set the input_string only if fits
			DEV_ASSERT2(dialog_style);
			if (dialog_style->fonts[1]->StringWidth(s.GetString()) < 300)
				input_string.Set(s.GetString());
		}
		
		return true;
	}

	// new options menu on - use that responder
	if (optionsmenuon)
		return M_OptResponder(ev, ch);

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
				DEV_ASSERT2(save_style);
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
				// 98-7-10 KM Use new defines
				M_SizeDisplay(SLIDERLEFT);
				S_StartSound(NULL, sfx_stnmov);
				return true;

			case KEYD_EQUALS:  // Screen size up

				if (automapactive || chat_on)
					return false;
				// 98-7-10 KM Use new defines
				M_SizeDisplay(SLIDERRIGHT);
				S_StartSound(NULL, sfx_stnmov);
				return true;

			case KEYD_F2:  // Save

				M_StartControlPanel();
				S_StartSound(NULL, sfx_swtchn);
				M_SaveGame(0);
				return true;

			case KEYD_F3:  // Load

				M_StartControlPanel();
				S_StartSound(NULL, sfx_swtchn);
				M_LoadGame(0);
				return true;

			case KEYD_F4:  // Sound Volume

				M_StartControlPanel();
				currentMenu = &SoundDef;
				itemOn = sfx_vol;
				S_StartSound(NULL, sfx_swtchn);
				return true;

			case KEYD_F5:  // Detail toggle, now loads options menu
				// -KM- 1998/07/31 F5 now loads options menu, detail is obsolete.

				S_StartSound(NULL, sfx_swtchn);
				M_StartControlPanel();
				M_Options(0);
				return true;

			case KEYD_F6:  // Quicksave

				S_StartSound(NULL, sfx_swtchn);
				M_QuickSave();
				return true;

			case KEYD_F7:  // End game

				S_StartSound(NULL, sfx_swtchn);
				M_EndGame(0);
				return true;

			case KEYD_F8:  // Toggle messages

				M_ChangeMessages(0);
				S_StartSound(NULL, sfx_swtchn);
				return true;

			case KEYD_F9:  // Quickload

				S_StartSound(NULL, sfx_swtchn);
				M_QuickLoad();
				return true;

			case KEYD_F10:  // Quit DOOM

				S_StartSound(NULL, sfx_swtchn);
				M_QuitEDGE(0);
				return true;

			case KEYD_F11:  // gamma toggle

				current_gamma++;
				if (current_gamma > 4)
					current_gamma = 0;
					
				const char *msg = NULL;
				
				switch(current_gamma)
				{
					case 0: { msg = language["GammaOff"];  break; }
					case 1: { msg = language["GammaLevelOne"];  break; }
					case 2: { msg = language["GammaLevelTwo"];  break; }
					case 3: { msg = language["GammaLevelThree"];  break; }
					case 4: { msg = language["GammaLevelFour"];  break; }
					default: { msg = NULL; break; }
				}
				
				if (msg)	
					CON_Printf("%s\n", msg);

				// -AJA- 1999/07/03: removed PLAYPAL reference.
				return true;

		}

		// Pop-up menu?
		if (ch == KEYD_ESCAPE)
		{
			M_StartControlPanel();
			S_StartSound(NULL, sfx_swtchn);
			return true;
		}
		return false;
	}

	// Keys usable within menu
	switch (ch)
	{
		case KEYD_DOWNARROW:
			do
			{
				if (itemOn + 1 > currentMenu->numitems - 1)
					itemOn = 0;
				else
					itemOn++;
				S_StartSound(NULL, sfx_pstop);
			}
			while (currentMenu->menuitems[itemOn].status == -1);
			return true;

		case KEYD_UPARROW:
			do
			{
				if (!itemOn)
					itemOn = currentMenu->numitems - 1;
				else
					itemOn--;
				S_StartSound(NULL, sfx_pstop);
			}
			while (currentMenu->menuitems[itemOn].status == -1);
			return true;

		case KEYD_PGUP:
		case KEYD_LEFTARROW:
			if (currentMenu->menuitems[itemOn].select_func &&
				currentMenu->menuitems[itemOn].status == 2)
			{
				S_StartSound(NULL, sfx_stnmov);
				// 98-7-10 KM Use new defines
				(* currentMenu->menuitems[itemOn].select_func)(SLIDERLEFT);
			}
			return true;

		case KEYD_PGDN:
		case KEYD_RIGHTARROW:
			if (currentMenu->menuitems[itemOn].select_func &&
				currentMenu->menuitems[itemOn].status == 2)
			{
				S_StartSound(NULL, sfx_stnmov);
				// 98-7-10 KM Use new defines
				(* currentMenu->menuitems[itemOn].select_func)(SLIDERRIGHT);
			}
			return true;

		case KEYD_ENTER:
			if (currentMenu->menuitems[itemOn].select_func &&
				currentMenu->menuitems[itemOn].status)
			{
				currentMenu->lastOn = itemOn;
				(* currentMenu->menuitems[itemOn].select_func)(itemOn);
				S_StartSound(NULL, sfx_pistol);
			}
			return true;

		case KEYD_ESCAPE:
			currentMenu->lastOn = itemOn;
			M_ClearMenus();
			S_StartSound(NULL, sfx_swtchx);
			return true;

		case KEYD_BACKSPACE:
			currentMenu->lastOn = itemOn;
			if (currentMenu->prevMenu)
			{
				currentMenu = currentMenu->prevMenu;
				itemOn = currentMenu->lastOn;
				S_StartSound(NULL, sfx_swtchn);
			}
			return true;

		default:
			for (i = itemOn + 1; i < currentMenu->numitems; i++)
				if (currentMenu->menuitems[i].alpha_key == ch)
				{
					itemOn = i;
					S_StartSound(NULL, sfx_pstop);
					return true;
				}
			for (i = 0; i <= itemOn; i++)
				if (currentMenu->menuitems[i].alpha_key == ch)
				{
					itemOn = i;
					S_StartSound(NULL, sfx_pstop);
					return true;
				}
			break;

	}

	return false;
}

//
// M_StartControlPanel
//
void M_StartControlPanel(void)
{
	// intro might call this repeatedly
	if (menuactive)
		return;

	menuactive = 1;
	CON_SetVisible(vs_notvisible);

	currentMenu = &MainDef;  // JDC
	itemOn = currentMenu->lastOn;  // JDC

	M_OptCheckNetgame();
}

//
// M_Drawer
//
// Called after the view has been rendered,
// but before it has been blitted.
//
void M_Drawer(void)
{
	short x;
	short y;
	unsigned int i;
	unsigned int max;

	if (!menuactive)
		return;

	// Horiz. & Vertically center string and print it.
	if (msg_mode)
	{
		dialog_style->DrawBackground();

		// FIXME: HU code should support center justification: this
		// would remove the code duplication below...
		epi::string_c msg;
		epi::string_c input;
		epi::string_c s;
		int oldpos, pos;

		if (!msg_string.IsEmpty())
			msg = msg_string.GetString();
		
		if (!input_string.IsEmpty())
			input = input_string.GetString();
		
		if (msg_mode == 2)
			input += "_";
		
		// Calc required height
		DEV_ASSERT2(dialog_style);
		s = msg + input;
		y = 100 - (dialog_style->fonts[0]->StringLines(s.GetString()) *
			dialog_style->fonts[0]->NominalHeight()/ 2);
		
		if (!msg.IsEmpty())
		{
			oldpos = 0;
			do
			{
				pos = msg.Find('\n', oldpos);
				
				s.Empty();
				if (pos >= 0)
					msg.GetMiddle(oldpos, pos-oldpos, s);
				else
					msg.GetMiddle(oldpos, msg.GetLength(), s);
			
				if (s.GetLength())
				{
					x = 160 - (dialog_style->fonts[0]->StringWidth(s.GetString()) / 2);
					HL_WriteText(dialog_style,0, x, y, s.GetString());
				}
				
				y += dialog_style->fonts[0]->NominalHeight();
				oldpos = pos + 1;
			}
			while (pos >= 0 && oldpos < (int)msg.GetLength());
		}

		if (!input.IsEmpty())
		{
			oldpos = 0;
			do
			{
				pos = input.Find('\n', oldpos);
				
				s.Empty();
				if (pos >= 0)
					input.GetMiddle(oldpos, pos-oldpos, s);
				else
					input.GetMiddle(oldpos, input.GetLength(), s);
			
				if (s.GetLength())
				{
					x = 160 - (dialog_style->fonts[1]->StringWidth(s.GetString()) / 2);
					HL_WriteText(dialog_style,1, x, y, s.GetString());
				}
				
				y += dialog_style->fonts[1]->NominalHeight();
				oldpos = pos + 1;
			}
			while (pos >= 0 && oldpos < (int)input.GetLength());
		}
		
		return;
	}

	// new options menu enable, use that drawer instead
	if (optionsmenuon)
	{
		M_OptDrawer();
		return;
	}

	style_c *style = currentMenu->style_var[0];
	DEV_ASSERT2(style);

	style->DrawBackground();
	
	// call Draw routine
	if (currentMenu->draw_func)
		(* currentMenu->draw_func)();

	// DRAW MENU
	x = currentMenu->x;
	y = currentMenu->y;
	max = currentMenu->numitems;

	for (i = 0; i < max; i++, y += LINEHEIGHT)
	{
		const image_t *image;
    
		// ignore blank lines
		if (! currentMenu->menuitems[i].patch_name[0])
			continue;

		if (! currentMenu->menuitems[i].image)
			currentMenu->menuitems[i].image = W_ImageLookup(
				currentMenu->menuitems[i].patch_name);
    
		image = currentMenu->menuitems[i].image;

		RGL_ImageEasy320(x, y, image);
	}

	// DRAW SKULL
	{
		int sx = x + SKULLXOFF;
		int sy = currentMenu->y - 5 + itemOn * LINEHEIGHT;

		RGL_ImageEasy320(sx, sy, menu_skull[whichSkull]);
	}
}

//
// M_ClearMenus
//
void M_ClearMenus(void)
{
	menuactive = 0;
	save_screenshot_valid = false;
}

//
// M_SetupNextMenu
//
void M_SetupNextMenu(menu_t * menudef)
{
	currentMenu = menudef;
	itemOn = currentMenu->lastOn;
}

//
// M_Ticker
//
void M_Ticker(void)
{
	if (optionsmenuon)
	{
		M_OptTicker();
		return;
	}

	if (--skullAnimCounter <= 0)
	{
		whichSkull ^= 1;
		skullAnimCounter = 8;
	}
}

//
// M_Init
//
void M_Init(void)
{
	E_ProgressMessage(language["MiscInfo"]);

	currentMenu = &MainDef;
	menuactive = 0;
	itemOn = currentMenu->lastOn;
	whichSkull = 0;
	skullAnimCounter = 10;
	msg_mode = 0;
	msg_string.Clear();
	msg_lastmenu = menuactive;
	quickSaveSlot = -1;

	// lookup styles
	styledef_c *def;

	def = styledefs.Lookup("MENU");
	if (! def) def = default_style;
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
	if (! def) def = styledefs.Lookup("OPTIONS");
	if (! def) def = default_style;
	sound_vol_style = hu_styles.Lookup(def);

	// lookup required images
	therm_l = W_ImageLookup("M_THERML");
	therm_m = W_ImageLookup("M_THERMM");
	therm_r = W_ImageLookup("M_THERMR");
	therm_o = W_ImageLookup("M_THERMO");

	menu_loadg    = W_ImageLookup("M_LOADG");
	menu_saveg    = W_ImageLookup("M_SAVEG");
	menu_svol     = W_ImageLookup("M_SVOL");
	menu_doom     = W_ImageLookup("M_DOOM");
	menu_newgame  = W_ImageLookup("M_NEWG");
	menu_skill    = W_ImageLookup("M_SKILL");
	menu_episode  = W_ImageLookup("M_EPISOD");
	menu_skull[0] = W_ImageLookup("M_SKULL1");
	menu_skull[1] = W_ImageLookup("M_SKULL2");

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

	M_InitOptmenu();
}

