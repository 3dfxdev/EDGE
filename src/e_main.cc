//---------------------------------------------------------------------------
//  EDGE Main Init + Program Loop Code
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
// DESCRIPTION:
//      EDGE main program (engine::Main),
//      game loop (engine::Loop) and startup functions.
//
// -MH- 1998/07/02 "shootupdown" --> "true3dgameplay"
// -MH- 1998/08/19 added up/down movement variables
//

#include "i_defs.h"
#include "e_main.h"

#include "am_map.h"
#include "con_defs.h" // Ansi C++ wants to know what funclist_s is.
#include "con_main.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "dstrings.h"
#include "f_finale.h"
#include "g_game.h"
#include "gui_main.h"
#include "gui_gui.h"
#include "hu_stuff.h"
#include "l_glbsp.h"
#include "m_argv.h"
#include "m_bbox.h"
#include "m_misc.h"
#include "m_menu.h"
#include "p_setup.h"
#include "p_spec.h"
#include "r_local.h"
#include "rad_trig.h"
#include "r_draw1.h"
#include "r_draw2.h"
#include "r_layers.h"
#include "r_vbinit.h"
#include "r2_defs.h"
#include "wp_main.h"   // need wipetype_e in rgl_defs.h
#include "rgl_defs.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "sv_chunk.h"
#include "sv_main.h"
#include "version.h"
#include "v_colour.h"
#include "v_ctx.h"
#include "v_res.h"
#include "v_toplev.h"
#include "w_image.h"
#include "w_textur.h"
#include "w_wad.h"
#include "wi_stuff.h"
#include "z_zone.h"

#include "./epi/epierror.h"
#include "./epi/epistring.h"

#define DEFAULT_LANGUAGE  "ENGLISH"

// Internals
static bool SetGlobalVars(void);
static bool SetLanguage(void); 
static bool SpecialWadVerify(void);
static bool ShowNotice(void);

typedef struct
{
	bool (*function)(void);
	char *LDFmessage;
}
startuporder_t;

startuporder_t startcode[] =
{
	{ M_LoadDefaults,      NULL            },
//	{ M_LoadDefaults,      "DefaultLoad"   },
	{ SetGlobalVars,       NULL            },
	{ RAD_Init,            NULL            },
	{ W_InitMultipleFiles, NULL            },
//	{ W_InitMultipleFiles, "WadFileInit"   },
	{ V_InitPalette,       NULL            },
	{ W_InitImages,        NULL            },
	{ R_InitFlats,         NULL            },
	{ W_InitTextures,      NULL            },
	{ DDF_MainCleanUp,     NULL            },
	{ SetLanguage,         NULL            },
	{ SpecialWadVerify,    NULL            },
	{ ShowNotice,          NULL            },
	{ V_MultiResInit,      "AllocScreens"  },
	{ I_SystemStartup,     "InitMachine"   },
	{ RAD_LoadParam,       NULL            },
	{ GUI_MainInit,        NULL            },
	{ SV_ChunkInit,        NULL            },
	{ SV_MainInit,         NULL            },
	{ M_Init,              "MiscInfo"      },
	{ R_Init,              "RefreshDaemon" },
	{ P_Init,              "PlayState"     },
	{ P_MapInit,           NULL            },
	{ P_InitSwitchList,    NULL            },
	{ R_InitPicAnims,      NULL            },
	{ R_InitSprites,       NULL            },
	{ S_Init,              "SoundInit"     },
	{ HU_Init,             "HeadsUpInit"   },
	{ E_CheckNetGame,      "CheckNetGame"  },
	{ ST_Init,             "STBarInit"     },
	{ NULL,                NULL            }
};

bool devparm;  // started game with -devparm
bool singletics = false;  // debug flag to cancel adaptiveness

// -ES- 2000/02/13 Takes screenshot every screenshot_rate tics.
// Must be used in conjunction with singletics.
static int screenshot_rate;

// For screenies...
bool m_screenshot_required = false;
bool need_save_screenshot  = false;

FILE *logfile = NULL;
FILE *debugfile = NULL;

gameflags_t default_gameflags =
{
	false,  // nomonsters
	false,  // fastparm

	false,  // res_respawn setting
	false,  // respawn
	false,  // item respawn

	false,  // true 3d gameplay
	MENU_GRAV_NORMAL, // gravity
	false,  // more blood

	true,   // jump
	true,   // crouch
	true,   // mlook
	AA_ON,  // autoaim
     
	true,   // trans
	true,   // cheats
     
	true,   // have_extra
	false,  // limit_zoom
	false,  // shadows
	false,  // halos

	CM_EDGE,  // compat_mode
	true      // kicking
};

// -KM- 1998/12/16 These flags hold everything needed about a level
// -KM- 1999/01/29 Added autoaim flag.
// -AJA- 2000/02/02: Removed initialisation (it *should* be setup at
//       level start).

gameflags_t level_flags;

// -KM- 1998/12/16 These flags are the users prefs and are copied to
//   gameflags when a new level is started.
// -AJA- 2000/02/02: Removed initialisation (done in code using
//       `default_gameflags').

gameflags_t global_flags;

bool drone = false;

skill_t startskill;
char *startmap;

bool autostart;
bool advancedemo;

int newnmrespawn = 0;

bool rotatemap = false;
bool showstats = false;
bool swapstereo = false;
bool mus_pause_stop = false;
bool infight = false;

bool external_ddf = false;
bool strict_errors = false;
bool lax_errors = false;
bool hom_detect = false;
bool no_warnings = false;
bool no_obsoletes = false;
bool autoquickload = false;

char *iwaddir;
char *homedir;
char *gamedir;
char *savedir;
char *ddfdir;

int crosshair = 0;
int missileteleport = 0;
int teleportdelay = 0;

layer_t *backbg_layer = NULL;
layer_t *conplayer_layer = NULL;
layer_t *pause_layer = NULL;

bool e_display_OK = false;

//
// EVENT HANDLING
//
// Events are asynchronous inputs generally generated by the game user.
// Events can be discarded if no responder claims them
//
event_t events[MAXEVENTS];
int eventhead;
int eventtail;

// ===================Internal====================

//
// SetGlobalVars
//
// -ACB- 1999/09/20 Created. Sets Global Stuff.
//
static bool SetGlobalVars(void)
{
	int p;
	const char *s;

	// Screen Resolution Check...
	s = M_GetParm("-width");
	if (s)
		SCREENWIDTH = atoi(s);

	s = M_GetParm("-height");
	if (s)
		SCREENHEIGHT = atoi(s);

	p = M_CheckParm("-res");
	if (p && p + 2 < M_GetArgCount())
	{
		SCREENWIDTH  = atoi(M_GetArgument(p + 1));
		SCREENHEIGHT = atoi(M_GetArgument(p + 2));
	}

	// Bits per pixel check....
	s = M_GetParm("-bpp");
	if (s)
		SCREENBITS = atoi(s) * 8;

	M_CheckBooleanParm("windowed",   &SCREENWINDOW, false);
	M_CheckBooleanParm("fullscreen", &SCREENWINDOW, true);

	// deathmatch check...
	if (M_CheckParm("-altdeath"))
	{
		deathmatch = 2;
	}
	else
	{
		p = M_CheckParm("-deathmatch");

		if (p)
		{
			deathmatch = 1;

			if (p + 1 < M_GetArgCount())
				deathmatch = atoi(M_GetArgument(p + 1));

			if (!deathmatch)
				deathmatch = 1;
		}
	}

	// sprite kludge (TrueBSP)
	p = M_CheckParm("-spritekludge");
	if (p)
	{
		if (p + 1 < M_GetArgCount())
			sprite_kludge = atoi(M_GetArgument(p + 1));

		if (!sprite_kludge)
			sprite_kludge = 1;
	}

	// speed for mouse look
	s = M_GetParm("-vspeed");
	if (s)
		mlookspeed = atoi(s) / 64;

	// -AJA- 1999/10/18: Reworked these with M_CheckBooleanParm
	M_CheckBooleanParm("rotatemap", &rotatemap, false);
	M_CheckBooleanParm("invertmouse", &invertmouse, false);
	M_CheckBooleanParm("showstats", &showstats, false);
	M_CheckBooleanParm("hom", &hom_detect, false);
	M_CheckBooleanParm("sound", &nosound, true);
	M_CheckBooleanParm("music", &nomusic, true);
	M_CheckBooleanParm("devparm", &devparm, false);
	M_CheckBooleanParm("itemrespawn", &global_flags.itemrespawn, false);
	M_CheckBooleanParm("mlook", &global_flags.mlook, false);
	M_CheckBooleanParm("monsters", &global_flags.nomonsters, true);
	M_CheckBooleanParm("fast", &global_flags.fastparm, false);
	M_CheckBooleanParm("extras", &global_flags.have_extra, false);
	M_CheckBooleanParm("shadows", &global_flags.shadows, false);
	M_CheckBooleanParm("halos", &global_flags.halos, false);
	M_CheckBooleanParm("kick", &global_flags.kicking, false);
	M_CheckBooleanParm("singletics", &singletics, false);
	M_CheckBooleanParm("infight", &infight, false);
	M_CheckBooleanParm("true3d", &global_flags.true3dgameplay, false);
	M_CheckBooleanParm("blood", &global_flags.more_blood, false);
	M_CheckBooleanParm("cheats", &global_flags.cheats, false);
	M_CheckBooleanParm("trans", &global_flags.trans, false);
	M_CheckBooleanParm("jumping", &global_flags.jump, false);
	M_CheckBooleanParm("crouching", &global_flags.crouch, false);
	M_CheckBooleanParm("dlights", &use_dlights, false);
	M_CheckBooleanParm("autoload", &autoquickload, false);

	if (M_CheckParm("-boom"))
		global_flags.compat_mode = CM_BOOM;
	else if (M_CheckParm("-edge") || M_CheckParm("-noboom"))
		global_flags.compat_mode = CM_EDGE;

	if (!global_flags.respawn)
	{
		if (M_CheckParm("-newnmrespawn"))
		{
			global_flags.res_respawn = true;
			global_flags.respawn = true;
		}
		else if (M_CheckParm("-respawn"))
		{
			global_flags.respawn = true;
		}
	}

	return true;
}

//
// bool SetLanguage
//
bool SetLanguage(void)
{
	if (M_CheckParm("-lang") > 0)
	{
		epi::string_c s;
		
		s = M_GetParm("-lang");
		
		if (!language.Select(s))
		{
			epi::string_c s2;
			s2.Format("Invalid language: '%s'", s.GetString());
			I_Error(s2.GetString());
		}
	}

	if (!language.IsValid())
	{
		if (!language.Select(DEFAULT_LANGUAGE))
		{
			if (!language.Select(0))
				I_Error("Unable to select any language!");
		}
	}
	
	return true;
}

//
// SpecialWadVerify
//
static bool SpecialWadVerify(void)
{
	int lump;

	// the WAD version
	int wad_ver;
	int wad_ver_frac;

	// the backward compatibility of the WAD
	int wad_bc;
	int wad_bc_frac;

	// The sub-version of the WAD (100% compatible with other WADs within the same wad_version)
	int wad_sub_ver;
	const void *data;
	const char *s;

	lump = W_CheckNumForName("EDGEVER");
	if (lump < 0)
		I_Error("EDGEVER lump not found. Get EDGE.WAD at http://edge.sourceforge.net/");

	data = W_CacheLumpNum(lump);
	s = (const char*)data;

	wad_ver = atoi(s);
	while (isdigit(*s))
		s++;

	s++;
	wad_ver_frac = atoi(s);
	while (isdigit(*s))
		s++;

	s++;
	wad_sub_ver = atoi(s);
	while (*s != '\n')
		s++;

	while (!isdigit(*s))
		s++;

	wad_bc = atoi(s);
	while (isdigit(*s))
		s++;

	s++;
	wad_bc_frac = atoi(s);

	W_DoneWithLump(data);

	if (wad_ver * 1024 + wad_ver_frac < EDGE_WAD_VERSION * 1024 + EDGE_WAD_VERSION_FRAC)
	{
		I_Error("EDGE.WAD version %d.%d found, version %d.%d is required.\n"
				"Get it at http://edge.sourceforge.net/", wad_ver, wad_ver_frac,
				EDGE_WAD_VERSION, EDGE_WAD_VERSION_FRAC);
		return false;
	}

	if (wad_bc * 1024 + wad_bc_frac > EDGE_WAD_VERSION * 1024 + EDGE_WAD_VERSION_FRAC)
	{
		I_Error("EDGE.WAD version %d.%d required, found too new version %d.%d\n"
				"which is not backward compatible enough. Get an older EDGE.WAD or\n"
				"a newer EDGE version at http://edge.sourceforge.net/",
				EDGE_WAD_VERSION, EDGE_WAD_VERSION_FRAC, wad_ver, wad_ver_frac);
		return false;
	}

	if (wad_ver > EDGE_WAD_VERSION)
	{
		I_Warning("EDGE.WAD version %d.%d required, found newer version %d.%d.\n"
				  "This version of EDGE is probably out-of-date, newer versions are\n"
				  "available at http://edge.sourceforge.net/",
				  EDGE_WAD_VERSION, EDGE_WAD_VERSION_FRAC, wad_ver, wad_ver_frac);
	}
	else if (EDGE_WAD_SUB_VERSION > wad_sub_ver)
	{
		I_Warning("Slightly out-of-date EDGE.WAD (v%d.%d.%d) found,\n"
				  "%d.%d.%d is recommended. Get it at http://edge.sourceforge.net/",
				  wad_ver, wad_ver_frac, wad_sub_ver,
				  EDGE_WAD_VERSION, EDGE_WAD_VERSION_FRAC, EDGE_WAD_SUB_VERSION);
	}

	I_Printf("EDGE.WAD version %d.%d.%d found.\n", wad_ver, wad_ver_frac, wad_sub_ver);

	return true;
}

//
// ShowNotice
//
static bool ShowNotice(void)
{
	I_Printf("%s", language["Notice"]);
 
	return true;
}

// ===============End of Internals================


//
// E_PostEvent
//
// Called by the I/O functions when input is detected
//
void E_PostEvent(event_t * ev)
{
	events[eventhead] = *ev;
	eventhead = (++eventhead) & (MAXEVENTS - 1);
}

//
// E_ProcessEvents
//
// Send all the events of the given timestamp down the responder chain
//
void E_ProcessEvents(void)
{
	event_t *ev;

	for (; eventtail != eventhead; eventtail = (++eventtail) & (MAXEVENTS - 1))
	{
		ev = &events[eventtail];
		if (chat_on && HU_Responder(ev))
			continue;  // let chat eat the event first of all

		if (GUI_MainResponder(ev))
			continue;  // GUI ate the event

		if (M_Responder(ev))
			continue;  // menu ate the event

		G_Responder(ev);  // let game eat it, nobody else wanted it
	}
}

static void M_DisplayPause(void)
{
	static const image_t *pause_image = NULL;

	if (! pause_image)
		pause_image = W_ImageFromPatch("M_PAUSE");

	// make sure image is centered horizontally

	int w = FROM_320(IM_WIDTH(pause_image));
	int h = FROM_200(IM_HEIGHT(pause_image));

	int x = FROM_320(160) - w / 2;
	int y = FROM_200(10);

    vctx.DrawImage(x, y, w, h, pause_image, 0, 0,
                   IM_RIGHT(pause_image), IM_BOTTOM(pause_image), NULL, 1.0f);
}

//
// E_Display
//
// Draw current display, possibly wiping it from the previous
//
// -ACB- 1998/07/27 Removed doublebufferflag check (unneeded).  

// wipegamestate can be set to GS_NOTHING to force a wipe on the next draw

gamestate_e wipegamestate = GS_DEMOSCREEN;
wipetype_e wipe_method = WIPE_Melt;
int wipe_reverse = 0;
bool redrawsbar;

static bool wipe_gl_active = false;

void E_Display(void)
{
	static bool viewactivestate = false;
	static bool menuactivestate = false;
	static bool inhelpscreensstate = false;
	static bool fullscreen = false;
	static int borderdrawcount;
	static gamestate_e oldgamestate = GS_NOTHING;

	// for wiping
#ifndef USE_GL
	static wipeinfo_t *wipeinfo = NULL;
	static screen_t *wipeend = NULL;
#endif
	static screen_t *wipestart = NULL;
	bool wipe;

	if (nodrawers)
		return;  // for comparative timing / profiling

	// -ES- 1998/08/20 Resolution Change Check
	if (changeresneeded)
		R_ExecuteChangeResolution();

	// Start the frame - should we need to.
	I_StartFrame();

	// change the view size if needed
	if (setsizeneeded)
	{
		R_ExecuteSetViewSize();
		oldgamestate = GS_NOTHING;  // force background redraw

		borderdrawcount = 3;
	}

	// -AJA- 1999/07/03: removed PLAYPAL reference.
	if (gamestate != oldgamestate && gamestate != GS_LEVEL)
	{
		V_SetPalette(PALETTE_NORMAL, 0);
	}

	// -AJA- 1999/08/02: Make sure palette/gamma is OK. This also should
	//       fix (finally !) the "gamma too late on walls" bug.
	V_ColourNewFrame();

	// save the current screen if about to wipe
	if (gamestate != wipegamestate)
	{
		wipe = true;
		wipestart = V_ResizeScreen(wipestart, SCREENWIDTH, SCREENHEIGHT, BPP);

#ifdef USE_GL
		wipe_gl_active = true;
		RGL_InitWipe(wipe_reverse, wipe_method);
#else
		V_CopyScreen(wipestart, main_scr);
#endif
	}
	else
		wipe = false;

	if (gamestate == GS_LEVEL && gametic)
		HU_Erase();

	// do buffered drawing
	switch (gamestate)
	{
		case GS_LEVEL:
			if (!gametic)
				break;
			if (automapactive == 2)
				AM_Drawer();
			if (wipe || (viewwindowheight != SCREENHEIGHT && fullscreen))
				redrawsbar = true;
			if (inhelpscreensstate && !inhelpscreens)
				redrawsbar = true;  // just put away the help screen

			ST_Drawer(viewwindowheight == SCREENHEIGHT, redrawsbar);
			redrawsbar = false;
			fullscreen = viewwindowheight == SCREENHEIGHT;
			break;

		case GS_INTERMISSION:
			WI_Drawer();
			break;

		case GS_FINALE:
			F_Drawer();
			break;

		case GS_DEMOSCREEN:
			E_PageDrawer();
			break;

		case GS_NOTHING:
			break;
	}

	if (need_save_screenshot)
	{
		R_Render();
		M_MakeSaveScreenShot();

		need_save_screenshot = false;
	}

	// draw the view directly
	if (gamestate == GS_LEVEL && gametic && automapactive != 2)
	{
		R_Render();

		if (automapactive != 2)
			AM_Drawer();
	}

	// clean up border stuff

#if 1  // #ifdef USE_GL
	// -AJA- temp hack for GL
	oldgamestate = GS_NOTHING;
#endif

	// see if the border needs to be initially drawn
	if (gamestate == GS_LEVEL && oldgamestate != GS_LEVEL)
	{
		viewactivestate = false;  // view was not active

		R_FillBackScreen();  // draw the pattern into the back screen
		// fixme hack

		ST_Drawer(viewwindowheight == SCREENHEIGHT, true);
	}

	// see if the border needs to be updated to the screen
	if (gamestate == GS_LEVEL && automapactive != 2)
	{
		if (menuactive || menuactivestate || !viewactivestate)
			borderdrawcount = 3;
		if (borderdrawcount)
		{
			R_DrawViewBorder();  // erase old menu stuff

			borderdrawcount--;
		}
	}

	if (gamestate == GS_LEVEL && gametic)
	{
		HU_Drawer();
		M_DisplayAir();
	}

	menuactivestate = menuactive;
	viewactivestate = viewactive;
	inhelpscreensstate = inhelpscreens;
	oldgamestate = wipegamestate = gamestate;

	if (paused)
		M_DisplayPause();

	// -AJA- hack to draw glbsp progress bars
	if (gb_draw_progress)
		GB_DrawProgress();

	// menus go directly to the screen
	M_Drawer();  // menu is drawn even on top of everything...
	GUI_MainDrawer();  // 2004/04/12 -AJA- ...EXCEPT the console
	M_DisplayDisk();

	E_NetUpdate();  // send out any new accumulation

	if (m_screenshot_required)
	{
		m_screenshot_required = false;
		M_ScreenShot();
	}
	else if (screenshot_rate && gamestate == GS_LEVEL)
	{
		if (!singletics)
			I_Error("E_Display: -screenshot must be used in conjunction with timedemo or singletics!");

		if (leveltime % screenshot_rate == 0)
			M_ScreenShot();
	}

	// normal update
	if (!wipe && !wipe_gl_active)
	{
		I_FinishFrame();  // page flip or blit buffer
		return;
	}

#ifdef USE_GL
	// -AJA- Wipe code for GL.  Sorry for all this ugliness, but it just
	//       didn't fit into the existing wipe framework.
	//
	if (RGL_DoWipe())
	{
		RGL_StopWipe();
		wipe_gl_active = false;
	}

	M_Drawer();  // menu is drawn even on top of wipes
	I_FinishFrame();  // page flip or blit buffer
      
#else // USE_GL

	// -ES- 1999/08/10 New wiping system
	// wipe update
	wipeend = V_ResizeScreen(wipeend, SCREENWIDTH, SCREENHEIGHT, BPP);
	V_CopyScreen(wipeend, main_scr);

	wipeinfo = WIPE_InitWipe(main_scr, 0, 0,
			wipestart, 0, 0, 1,
			wipeend, 0, 0, 1,
			SCREENWIDTH, SCREENHEIGHT, wipeinfo,
			-1, wipe_reverse?true:false, wipe_method);

	int wipestarttime = I_GetTime();
	int tics = 0;
	bool done;

	do
	{
		int nowtime;

		do
		{
			nowtime = I_GetTime();
		}
		while (tics == nowtime - wipestarttime);
		tics = nowtime - wipestarttime;
		done = WIPE_DoWipe(main_scr, wipestart, wipeend, tics, wipeinfo);
		M_Drawer();  // menu is drawn even on top of wipes

		I_FinishFrame();  // page flip or blit buffer

	}
	while (!done);
	redrawsbar = true;

#endif // USE_GL
}

//
//  DEMO LOOP
//
int demosequence;
static int demo_num;
static int page_map;
static int page_pic;
static int pagetic;
static const image_t *page_image = NULL;

//
// E_PageTicker
//
// Handles timing for warped projection
//
void E_PageTicker(void)
{
	if (--pagetic < 0)
		E_AdvanceDemo();
}

#define NOPAGE_COLOUR  (GRAY + GRAY_LEN*4/5)

//
// E_PageDrawer
//
void E_PageDrawer(void)
{
	if (page_image)
		VCTX_Image(0, 0, SCREENWIDTH, SCREENHEIGHT, page_image);
	else
		vctx.SolidBox(0, 0, SCREENWIDTH, SCREENHEIGHT, NOPAGE_COLOUR, 1);
}

//
// E_AdvanceDemo
//
// Called after each demo or intro demosequence finishes
//
void E_AdvanceDemo(void)
{
	advancedemo = true;
}

static void DemoNextPicture(void)
{
	int count;
	gamedef_c *g;
  
	// prevent an infinite loop
	for (count=0; count < 200; count++)
	{
		g = gamedefs[page_map];

		if (page_pic >= g->titlepics.GetSize())
		{
			page_map = (page_map + 1) % gamedefs.GetSize();
			page_pic = 0;
			continue;
		}

		// ignore non-existing episodes.  Doesn't include title-only ones
		// like [EDGE].
		if (page_pic == 0 && g->firstmap && g->firstmap[0] &&
			W_CheckNumForName(g->firstmap) == -1)
		{
			page_map = (page_map + 1) % gamedefs.GetSize();
			continue;
		}

		// ignore non-existing images
		if (W_CheckNumForName(g->titlepics[page_pic]) < 0)
		{
			page_pic += 1;
			continue;
		}

		// found one !!

		if (page_pic == 0 && g->titlemusic > 0)
			S_ChangeMusic(g->titlemusic, false);

		page_image = W_ImageFromPatch(g->titlepics[page_pic]);
		page_pic += 1;
    
		return;
	}
}

//
// This cycles through the demo sequences.
// -KM- 1998/12/16 Fixed for DDF.
//
void E_DoAdvanceDemo(void)
{
	consoleplayer->playerstate = PST_LIVE;  // not reborn

	advancedemo = false;
	usergame = false;     // no save or end game here
	paused = false;
	gameaction = ga_nothing;

	demosequence = (demosequence + 1) % 2;  // - Kester

	switch (demosequence)  // - Kester
	{
		case 0:  // Title Picture
		{
			gamestate = GS_DEMOSCREEN;

			DemoNextPicture();

			pagetic = gamedefs[page_map]->titletics;
			break;
		}

		default:  // Play Demo
		{
			char buffer[9];

			sprintf(buffer, "DEMO%x", demo_num++);

			if (W_CheckNumForName(buffer) < 0)
			{
				demo_num = 1;
				sprintf(buffer, "DEMO1");
			}

			G_DeferredPlayDemo(buffer);
			break;
		}
	}
}

//
// E_StartTitle
//
void E_StartTitle(void)
{
	gameaction = ga_nothing;
	demosequence = 1;
	demo_num = 1;

	// force pic overflow -> first available titlepic
	page_map = gamedefs.GetSize() - 1;
	page_pic = 999;
 
	E_AdvanceDemo();
}

//
// InitDirectories
//
// Detects which directories to search for DDFs, WADs and other files in.
// Does not set iwaddir though (E_IdentifyVersion does that).
//
// -ES- 2000/01/01 Written.
//
static void InitDirectories(void)
{
	const char *location;
	const char *p;
	char *parmfile;

	location = M_GetParm("-home");

	// Get the Home Directory from environment if set
	if (!location)
		location = getenv(EDGEWADDIR);

	if (location)
	{
		homedir = I_PreparePath(location);
	}
	else
	{
		char *home = getenv("HOME");
		if (home)
		{
			bool valid = true;
			char *tempdir;

			tempdir = I_PreparePath(home);

			homedir = Z_New(char, strlen(tempdir) + strlen(EDGEHOMESUBDIR) + 2);
			sprintf(homedir, "%s%c%s", tempdir, DIRSEPARATOR, EDGEHOMESUBDIR);

			Z_Free(tempdir);

			if (!I_PathIsDirectory(homedir))
			{
				// Hack with define before we stick in the epi
#ifdef WIN32
				mkdir(homedir);
#else
				mkdir(homedir, SAVEGAMEMODE);
#endif
				// Check whether the directory was created
				if (!I_PathIsDirectory(homedir))
				{
					valid = false;
				}
			}

			if (!valid)
			{
				Z_Free(homedir);
				homedir = NULL;
			}
		}
		
		if (!homedir)
		{
			homedir = Z_StrDup(".");
		}
	}

	// Get the Game Directory from parameter.
	location = M_GetParm("-game");
	if (location)
    {
		gamedir = I_PreparePath(location);
	}
	else
	{
		gamedir = getenv("EDGEDATA");
		if (gamedir)
		{
			gamedir = I_PreparePath(gamedir);
		}
		else
		{
			//gamedir = Z_StrDup(homedir);
			gamedir = Z_StrDup(".");
		}
	}

	// add parameter file "gamedir/parms" if it exists.
	parmfile = (char*)I_TmpMalloc(strlen(gamedir) + strlen("parms") + 2);
	sprintf(parmfile, "%s%cparms", gamedir, DIRSEPARATOR);

#ifdef DEVELOPERS
	L_WriteDebug("Response file '%s' ", parmfile);
#endif

	if (I_Access(parmfile))
	{
#ifdef DEVELOPERS
		L_WriteDebug("found.\n");
#endif
		// Insert it right after the game parameter
		M_ApplyResponseFile(parmfile, M_CheckParm("-game") + 2);
	}
	else
	{
#ifdef DEVELOPERS
		L_WriteDebug("not found.\n");
#endif
	}

	I_TmpFree(parmfile);

	location = M_GetParm("-ddf");
	if (location)
	{
		external_ddf = true;
		ddfdir = I_PreparePath(location);
	} 
	else
	{
		ddfdir = Z_StrDup(gamedir);
	}

	// config file
	p = M_GetParm("-config");
	if (p)
	{
		epi::string_c fn;
		
		M_ComposeFileName(fn, homedir, p);
		cfgfile.Set(fn.GetString());
	}
	else
	{
		epi::string_c fn;

	    fn.Format("%s%c%s", homedir, DIRSEPARATOR, EDGECONFIGFILE);
		cfgfile.Set(fn.GetString());
	}
	
	// savegame directory
	{
		epi::string_c dir;
		
		dir = homedir;
		dir += DIRSEPARATOR;
		dir += SAVEGAMEDIR;
		
		// FIXME!! Replace with epi::strent_c
		savedir = new char[dir.GetLength()+1];
		strcpy(savedir, dir.GetString());
		
#ifdef WIN32
	    mkdir(savedir);
#else
	    mkdir(savedir, SAVEGAMEMODE);
#endif
	}
	

    return;
}

//
// CheckExternal
//
// Checks if DDF files exist in the DDF directory, and if so then
// enables "external-ddf mode", which prevents the DDF lumps within
// EDGE.WAD from being parsed.
//
// -AJA- 2000/05/23: written.
//
#define EXTERN_FILE  "things.ddf"

static void CheckExternal(void)
{
	char *testfile;
  
	// too simplistic ?

	testfile = (char*)I_TmpMalloc(strlen(gamedir) + strlen(EXTERN_FILE) + 2);
	sprintf(testfile, "%s%c%s", gamedir, DIRSEPARATOR, EXTERN_FILE);

	if (I_Access(testfile))
		external_ddf = true;
  
	I_TmpFree(testfile);
}

//
// E_IdentifyVersion
//
// Adds an IWAD and EDGE.WAD. -ES-  2000/01/01 Rewritten.
//
const char *wadname[] = { "doom2", "doom", "plutonia", "tnt", "freedoom", NULL };

static void IdentifyVersion(void)
{
	bool done;
	const char *location;
	char *iwad;
	int i;
	int wadnum;
	epi::string_c fn;

	// Check -iwad parameter, find out if we are talking directory or file
	location = M_GetParm("-iwad");

	if (!location)
		location = getenv("DOOMWADDIR");

	if (location)
	{
		iwad = I_PreparePath(location);

		if (I_PathIsDirectory(iwad))
		{
			// it was a directory
			iwaddir = iwad;
			iwad = NULL;
		}
		else
		{
			// it was a file
			iwaddir = Z_StrDup(gamedir);
		}
	}
	else
	{
		iwaddir = Z_StrDup(gamedir);
		iwad = NULL;
	}

	// Has an iwad name been specified?
	if (iwad)
	{
		epi::string_c fn;
		
		if (M_CheckExtension(EDGEWADEXT, iwad) != EXT_MATCHING)
		{
			fn.Format("%s.%s", iwad, EDGEWADEXT);
		}
		else
		{
			fn = iwad;
		}

		if (I_Access(fn.GetString()))
		{
			W_AddRawFilename(fn.GetString(), false);
		}
		else
		{
			I_Error("IdentifyVersion: Unable to add specified '%s'", 
					fn.GetString());
		}
		
		Z_Free(iwad);
	}
	else // cycle through default wad names and add them if they exist
	{
		done = false;
		for (i = 0; i < 3 && !done; i++)
		{
			location = (i == 0 ? iwaddir : gamedir);

			//
			// go through the available wad names constructing an access
			// name for each, adding the file if they exist.
			//
			// -ACB- 2000/06/08 Quit after we found a file - don't load
			//                  more than one IWAD
			//
			wadnum = 0;
			while (wadname[wadnum] && !done)
			{
				fn.Format("%s%c%s.%s", location, DIRSEPARATOR, 
						wadname[wadnum], EDGEWADEXT);

				if (I_Access(fn.GetString()))
				{
					W_AddRawFilename(fn.GetString(), false);
					done = true;
				}

				wadnum++;
			}
		}
	}

	if (!addwadnum)
		I_Error("IdentifyVersion: No IWADS found!\n");

	done = false;
	for (i = 0; i < 2 && !done; i++)
	{
		location = (i == 0 ? iwaddir : gamedir);

		if (location)
		{
			fn.Format("%s%c%s.%s", location, DIRSEPARATOR, REQUIREDWAD, EDGEWADEXT);

			if (I_Access(fn.GetString()))
			{
				// Only read the DDF/RTS lumps in EDGE.WAD if we are not in
				// external-ddf mode.

				W_AddRawFilename(fn.GetString(), external_ddf ? false : true);
				done = true;
			}
		}
	}

	if (!done)
		I_Error("IdentifyVersion: Could not find required %s.%s!\n", REQUIREDWAD, EDGEWADEXT);
}

static void ShowDate(void)
{
	time_t cur_time;
	char timebuf[100];

	time(&cur_time);
	strftime(timebuf, 99, "%I:%M %p on %d/%b/%Y", localtime(&cur_time));

	L_WriteLog("[Log file created at %s]\n\n", timebuf);
	L_WriteDebug("[Debug file created at %s]\n\n", timebuf);
}

static void ShowVersion(void)
{
	// 23-6-98 KM Changed to hex to allow versions such as 0.65a etc
	I_Printf("EDGE v" EDGEVERSTR " compiled on " __DATE__ " at " __TIME__ "\n");
	I_Printf("EDGE homepage is at http://edge.sourceforge.net/\n");
	I_Printf("EDGE is based on DOOM by id Software http://www.idsoftware.com/\n");
}


//
// E_EngineShutdown
//
void E_EngineShutdown(void)
{
	if (demorecording)
	{
		G_CheckDemoStatus();
	}

	S_StopMusic();
	E_QuitNetGame();
}

// The engine namespace
namespace engine
{
	// Local Prototypes
	bool Startup(void);
	void Loop(void);
	void Shutdown(void);

	//
	// Startup
	//
	bool Startup()
	{
		int p;
		const char *ps;
		char title[] = "EDGE v" EDGEVERSTR;
		int turbo_scale = 100;
		bool success;

		// Version check ?
		if (M_CheckParm("-version"))
		{
			// -AJA- using I_Error here, since I_Printf crashes this early on
			I_Error("\nEDGE version is " EDGEVERSTR "\n");
		}

		// -AJA- 2000/02/02: initialise global gameflags to defaults
		global_flags = default_gameflags;

		// -AJA- 2003/11/08 The log file gets all CON_Printfs, I_Printfs,
		//                  I_Warnings and I_Errors.
		if (! M_CheckParm("-nolog"))
		{
			logfile = fopen(EDGELOGFILE, "w");

			if (!logfile)
				I_Error("[engine::Startup] Unable to create log file");
		}
		else
		{
			logfile = NULL;
		}
		
		//
		// -ACB- 1998/09/06 Only used for debugging.
		//                  Moved here to setup debug file for DDF Parsing...
		//
		// -ES- 1999/08/01 Debugfiles can now be used without -DDEVELOPERS, and
		//                 then logs all the CON_Printfs, I_Printfs and I_Errors.
		//
		// -ACB- 1999/10/02 Don't print to console, since we don't have a console yet.
		//
		p = M_CheckParm("-debugfile");
		if (p)
		{
			epi::string_c fn;
			int i = 1;

			// -ES- 1999/03/29 allow -debugfile <file>
			if (p + 1 < M_GetArgCount() && (ps = M_GetArgument(p + 1))[0] != '-')
			{
				fn = ps;
			}
			else
			{
				// -KM- 1999/01/29 Consoleplayer is always 0 at this stage.
				fn = "debug0.txt";
				while (I_Access(fn.GetString()))
				{
					fn.Format("debug%d.txt", i++);

					// give up: File system is probably corrupt. If not, there are 1000
					// debug files already, and it's about time to delete some of them...
					if (i >= 1000)
						I_Error("[engine::Startup] Couldn't create debug file!");
				}
			}
			debugfile = fopen(fn.GetString(), "w");

			if (!debugfile)
				I_Error("[engine::Startup] Unable to create debugfile");
	    
			L_WriteDebug("%s\n",title);
		}
		else
		{
			debugfile = NULL;
		}

		// Assume that we are using a standard game setup...
		modifiedgame = false;

		// -ACB- 1999/09/20 defines to be used?
		CON_InitConsole(79, 25, false);

		// -ES- 1999/10/29 Declare all function lists.
		R_InitFunctions_Draw1();
#ifndef NOHICOLOUR
		R_InitFunctions_Draw2();
#endif
		R_InitVBFunctions();
		I_RegisterAssembler();
		I_PutTitle(title);

		ShowDate();
		ShowVersion();

		InitDirectories();

		// check for strict and no-warning options
		M_CheckBooleanParm("strict", &strict_errors, false);
		M_CheckBooleanParm("warn", &no_warnings, true);
		M_CheckBooleanParm("obsolete", &no_obsoletes, true);
		M_CheckBooleanParm("lax", &lax_errors, false);

		CheckExternal();

		DDF_MainInit();
	
		IdentifyVersion();

		if (devparm)
			I_Printf("%s", language["DevelopmentMode"]);

		p = M_CheckParm("-turbo");
		if (p)
		{
			if (p + 1 < M_GetArgCount())
				turbo_scale = atoi(M_GetArgument(p + 1));
			else
				turbo_scale = 200;

			if (turbo_scale < 10)
				turbo_scale = 10;

			if (turbo_scale > 400)
				turbo_scale = 400;

			CON_MessageLDF("TurboScale", turbo_scale);
		}

		G_SetTurboScale(turbo_scale);

		I_CheckCPU();

		ps = M_GetParm("-col8");
		if (ps)
			CON_ChooseFunctionFromList(&drawcol8_funcs, ps);

		ps = M_GetParm("-span8");
		if (ps)
			CON_ChooseFunctionFromList(&drawspan8_funcs, ps);

		ps = M_GetParm("-col16");
		if (ps)
			CON_ChooseFunctionFromList(&drawcol16_funcs, ps);

		ps = M_GetParm("-span16");
		if (ps)
			CON_ChooseFunctionFromList(&drawspan16_funcs, ps);

		{
			epi::string_c fn;
			p = M_CheckNextParm("-file", 0);
			while (p)
			{
				// the parms after p are wadfile/lump names,
				// until end of parms or another - preceded parm
				modifiedgame = true;
	
				p++;
				while (p < M_GetArgCount() && '-' != (ps = M_GetArgument(p))[0])
				{
					
					M_ComposeFileName(fn, gamedir, ps);
					W_AddRawFilename(fn.GetString(), true);
					p++;
				}
	
				p = M_CheckNextParm("-file", p-1);
			}
		}
		
		ps = M_GetParm("-playdemo");

		if (!ps)
			ps = M_GetParm("-timedemo");

		if (ps)
		{
			epi::string_c fn;
			
			M_ComposeFileName(fn, gamedir, ps);
			fn += ".lmp";	// FIXME!! Check we need to use extension here
			W_AddRawFilename(fn.GetString(), false);
			I_Printf("Playing demo %s.\n", fn.GetString());
		}

		// get skill / episode / map from parms
		startskill = sk_medium;
		autostart = false;

		// -KM- 1999/01/29 Use correct skill: 1 is easiest, not 0
		ps = M_GetParm("-skill");
		if (ps)
		{
			startskill = (skill_t)(atoi(ps) - 1);
			autostart = true;
		}

		ps = M_GetParm("-timer");
		if (ps && deathmatch)
		{
			int time;

			time = atoi(ps);
			I_Printf("Levels will end after %d minute", time);

			if (time > 1)
				I_Printf("s");

			I_Printf(".\n");
		}

		ps = M_GetParm("-warp");
		if (ps)
		{
			startmap = Z_StrDup(ps);
			autostart = true;
		}
		else
		{
			startmap = Z_StrDup("MAP01"); // MUNDO HACK!!!!
		}

		// Cycle through all the startup functions, quit on failure.
		for (p=0; startcode[p].function != NULL; p++)
		{
			// Print Message On Screen
			if (startcode[p].LDFmessage)
				I_Printf(language[startcode[p].LDFmessage]);

			// if the startup function fails - quit startup
			success = startcode[p].function();
			if (!success)
				return false;
		}

		ps = M_GetParm("-screenshot");
		if (ps)
		{
			screenshot_rate = atoi(ps);
		}
	  
		// start the appropriate game based on parms
		ps = M_GetParm("-record");
		if (ps)
		{
			G_RecordDemo(ps);
			autostart = true;
		}

		ps = M_GetParm("-playdemo");
		if (ps)
		{
			// quit after one demo
			singledemo = true;
			G_DeferredPlayDemo(ps);
			return true;
		}

		ps = M_GetParm("-timedemo");
		if (ps)
		{
			G_TimeDemo(ps);
			return true;
		}

		ps = M_GetParm("-loadgame");
		if (ps)
		{
			G_LoadGame(atoi(ps));
		}

		// -ACB- 1998/09/06 use new mapping system
		if (gameaction != ga_loadgame)
		{
			if (autostart || netgame)
			{
				// if startmap is failed, do normal start.
				if (! G_DeferredInitNew(startskill, startmap, true))
					E_StartTitle();

				Z_Free(startmap);
			}
			else
			{
				E_StartTitle();  // start up intro loop
			}
		}

		return true;
	}

	//
	// Main
	//
	// -ACB- 1998/08/10 Removed all reference to a gamemap, episode and mission
	//                  Used LanguageLookup() for lang specifics.
	//
	// -ACB- 1998/09/06 Removed all the unused code that no longer has
	//                  relevance.    
	//
	// -ACB- 1999/09/04 Removed statcopy parm check - UNUSED
	//
	// -ACB- 2004/05/31 Moved into a namespace, the c++ revolution begins....
	//
	void Main(int argc, const char **argv)
	{
		// Start the EPI Interface 
		epi::Init();

		// Start memory allocation system at the very start (SCHEDULED FOR REMOVAL)
		Z_Init();

		// Implemented here - since we need to bring the memory manager up first
		// -ACB- 2004/05/31
		M_InitArguments(argc, argv);

		try
		{
			// Todo: All Startup functions should throw errors
			// Startup()
			if (!Startup())
			{
				epi::error_c err(ERR_GENERIC, "Failed Startup!");
				throw err;
			}
				
			Loop();
		}
		catch(epi::error_c err)
		{
			//I_Error(err.GetInfo());
		};
		Shutdown();								// Shutdown whatever at this point

		// Kill the epi interface
		epi::Shutdown();
	}

	//
	// Loop
	//
	// This calls I_Loop which performs the main loop. I_Loop is
	// required because the loop is not always infinite on platforms.
	//
	void Loop(void)
	{
		// SV_MainTestPrimitives();
		// RGL_TestPolyQuads();

		// -ES- 1998/09/11 Use R_ChangeResolution to enter gfx mode
		R_ChangeResolution(SCREENWIDTH, SCREENHEIGHT, SCREENBITS, SCREENWINDOW);

		// -KM- 1998/09/27 Change res now, so music doesn't start before
		// screen.  Reset clock too.
		R_ExecuteChangeResolution();

		//
		// -ACB- 1999/09/24 Call System Specific Looping function. Some systems
		//                  don't loop forever.
		//
		I_Loop();
		return;
	}

	//
	// Tick
	//
	// This Function is called by I_Loop for a single loop in the
	// system.
	//
	// -ACB- 1999/09/24 Written
	// -ACB- 2004/05/31 Namespace'd
	//
	void Tick(void)
	{
		// -ES- 1998/09/11 It's a good idea to frequently check the heap
#ifdef DEVELOPERS
		//Z_CheckHeap();
//		L_WriteDebug("[0] Mem size: %ld\n", epi::the_mem_manager->GetAllocatedSize());
#endif

		// process one or more tics
		if (singletics)
		{
			I_ControlGetEvents();
			E_ProcessEvents();
			G_BuildTiccmd(&consoleplayer->netcmds[maketic % BACKUPTICS]);

			if (advancedemo)
				E_DoAdvanceDemo();

			M_Ticker();
			GUI_MainTicker();
			G_Ticker();
			S_SoundTicker();
			S_MusicTicker();
			gametic++;
			maketic++;
		}
		else
		{
			E_TryRunTics();  // will run at least one tic
		}

		// Update display, next frame, with current state.
		E_Display();

		// -AJA- hack to allow other code to know when they can call
		//       E_Display().
		e_display_OK = true;
	}

	//
	// Shutdown
	//
	void Shutdown()
	{
		/* ... */
	}
};
