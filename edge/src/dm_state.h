//----------------------------------------------------------------------------
//  EDGE Global State Variables
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
// -MH- 1998/07/02 "lookupdown" --> "true3dgameplay"
//
// -ACB- 1999/10/07 Removed Sound Parameters - New Sound API
//

#ifndef __D_STATE__
#define __D_STATE__

// We need globally shared data structures,
//  for defining the global state variables.
#include "dm_data.h"

// We need the playr data structure as well.
#include "e_player.h"

#include "epi/utility.h"

extern bool devparm;  // DEBUG: launched with -devparm

extern gameflags_t level_flags;
extern gameflags_t global_flags;

// -------------------------------------------
// Selected skill type, map etc.
//

// Defaults for menu, methinks.
extern skill_t startskill;
extern char *startmap;

extern bool autostart;

// Selected by user. 
extern skill_t gameskill;

// Netgame? Only true if >1 player.
extern bool netgame;

// Flag: true only if started as net deathmatch.
// An enum might handle altdeath/cooperative better.
extern int deathmatch;

// -------------------------
// Status flags for refresh.
//

// Depending on view size - no status bar?
// Note that there is no way to disable the
//  status bar explicitely.
extern bool statusbaractive;
extern int automapactive;  // In AutoMap mode?
extern bool menuactive;  // Menu overlayed?
extern bool rts_menuactive;
extern bool paused;  // Game Pause?
extern bool viewactive;
extern bool nodrawers;
extern bool noblit;

// This one is related to the 3-screen display mode.
// ANG90 = left side, ANG270 = right
extern angle_t viewanglebaseoffset;

// -------------------------------------
// Scores, rating.
// Statistics on a given map, for intermission.
//
extern int totalkills;
extern int totalitems;
extern int totalsecret;

// Timer, for scores.
extern int leveltime;  // tics in game play for par

// --------------------------------------
// DEMO playback/recording related stuff.
// No demo, there is a human player in charge?
// Disable save/end game?
extern bool usergame;

//?
extern bool demoplayback;
extern bool demorecording;

// -AJA- 2000/12/07: auto quick-load feature
extern bool autoquickload;

// Quit after playing a demo from cmdline.
extern bool singledemo;

//?
extern gamestate_e gamestate;

//-----------------------------
// Internal parameters, fixed.
// These are set by the engine, and not changed
//  according to user inputs. Partly load from
//  WAD, partly set at startup time.

extern int gametic;

// The maximum number of players, multiplayer/networking.
#define MAXPLAYERS  30

// Pointer to each player in the game.
extern player_t *players[MAXPLAYERS];
extern int numplayers;

// Player taking events, and displaying.
extern int consoleplayer;
extern int displayplayer;

#define MAXHEALTH 200
#define MAXARMOUR 200

#define CHEATARMOUR      MAXARMOUR
#define CHEATARMOURTYPE  ARMOUR_Blue

// Player spawn spots for deathmatch, coop.
extern spawnpointarray_c dm_starts;
extern spawnpointarray_c coop_starts;

// Intermission stats.
// Parameters for world map / intermission.
extern wbstartstruct_t wminfo;

//-----------------------------------------
// Internal parameters, used for engine.
//

// File handling stuff.
extern char *iwaddir;
extern char *homedir;
extern char *gamedir;
extern char *savedir;
extern char *ddfdir;
extern epi::strent_c cfgfile;
extern FILE *logfile;
extern FILE *debugfile;

// if true, load DDF/RTS as external files (instead of from EDGE.WAD)
extern bool external_ddf;

// if true, load all graphics at level load
extern bool precache;

// if true, prefer to crash out on various errors
extern bool strict_errors;

// if true, prefer to ignore or fudge various (serious) errors
extern bool lax_errors;

// if true, disable warning messages
extern bool no_warnings;

// if true, disable obsolete warning messages
extern bool no_obsoletes;

// if true, enable HOM detection (hall of mirrors effect)
extern bool hom_detect;

// wipegamestate can be set to GS_NOTHING to force a wipe on the next draw
extern gamestate_e wipegamestate;

extern int mouseSensitivity;
extern int save_page;

extern int quickSaveSlot;

// debug flag to cancel adaptiveness
extern bool singletics;

extern int bodyqueslot;

// Needed to store the number of the dummy sky flat.
// Used for rendering, as well as tracking projectiles etc.

extern const struct image_s *skyflatimage;

#define IS_SKY(plane)  ((plane).image == skyflatimage)


//---------------------------------------------------
// Netgame stuff (buffers and pointers, i.e. indices).

extern int maketic;

//misc stuff
extern bool map_overlay;
extern bool rotatemap;
extern bool showstats;
extern bool swapstereo;
extern bool infight;

typedef enum
{
	HUD_Full = 0,
	HUD_None,
	HUD_Overlay,

	NUMHUD
}
hud_type_e;

extern int crosshair;
extern int screen_hud;
extern int sky_stretch;
extern int menunormalfov, menuzoomedfov;
extern int usemouse;
extern int usejoystick;

extern int missileteleport;
extern int teleportdelay;

//cd-audio stuff
typedef enum
{
  CD_OFF = 0,
  CD_ON,
  CD_ATMOS
}
cdType_t;

extern cdType_t cdaudio;
extern int cdtrack;
extern int cdnumtracks;
extern int cdcounter;

//okay, heres the resolution/hicolour:
extern int SCREENWIDTH;
extern int SCREENHEIGHT;
extern int SCREENBITS;
extern bool SCREENWINDOW;

// I_Video.c / V_Video*.c Precalc. Stuff
extern float DX, DY, DXI, DYI, DY2, DYI2;
extern int SCALEDWIDTH, SCALEDHEIGHT, X_OFFSET, Y_OFFSET;

extern bool graphicsmode;

// -ES- 1999/08/15 Added teleport effects
extern int telept_effect;
extern int telept_flash;
extern int telept_reverse;

//mlook stuff
extern int mlookspeed;
extern bool invertmouse; // -ACB- 1999/09/03 Must be true or false - becomes boolean

// -KM- 1998/09/01 Analogue binding stuff, These hold what axis they bind to.
extern int joy_xaxis;
extern int joy_yaxis;
extern int mouse_xaxis;
extern int mouse_yaxis;

//
// -ACB- 1998/09/06 Analogue binding:
//                   Two stage turning, angleturn control
//                   horzmovement control, vertmovement control
//                   strafemovediv;
//
extern bool stageturn;
extern int forwardmovespeed;
extern int angleturnspeed;
extern int sidemovespeed;

#endif
