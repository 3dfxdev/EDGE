//----------------------------------------------------------------------------
//  EDGE Global State Variables
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2001  The EDGE Team.
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
#include "e_net.h"

// We need the playr data structure as well.
#include "e_player.h"

extern boolean_t devparm;  // DEBUG: launched with -devparm

extern boolean_t redrawsbar;

extern gameflags_t level_flags;
extern gameflags_t global_flags;

// -----------------------------------------------------
// Game Mode - identify IWAD as shareware, retail etc.
//

// Set if homebrew PWAD stuff has been added.
extern boolean_t modifiedgame;

// -------------------------------------------
// Selected skill type, map etc.
//

// Defaults for menu, methinks.
extern skill_t startskill;
extern char *startmap;
extern boolean_t drone;

extern boolean_t autostart;

// Selected by user. 
extern skill_t gameskill;

// Netgame? Only true if >1 player.
extern boolean_t netgame;

// Flag: true only if started as net deathmatch.
// An enum might handle altdeath/cooperative better.
extern int deathmatch;

// -------------------------
// Status flags for refresh.
//

// Depending on view size - no status bar?
// Note that there is no way to disable the
//  status bar explicitely.
extern boolean_t statusbaractive;
extern int automapactive;  // In AutoMap mode?
extern boolean_t menuactive;  // Menu overlayed?
extern boolean_t paused;  // Game Pause?
extern boolean_t viewactive;
extern boolean_t nodrawers;
extern boolean_t noblit;

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
extern boolean_t usergame;

//?
extern boolean_t demoplayback;
extern boolean_t demorecording;

// -AJA- 2000/12/07: auto quick-load feature
extern boolean_t autoquickload;

// Quit after playing a demo from cmdline.
extern boolean_t singledemo;

//?
extern gamestate_t gamestate;

//-----------------------------
// Internal parameters, fixed.
// These are set by the engine, and not changed
//  according to user inputs. Partly load from
//  WAD, partly set at startup time.

extern int gametic;

// The maximum number of players, multiplayer/networking.
#define MAXPLAYERS  32

// Pointer to each player.
extern player_t **playerlookup;

// Linked list of all players in the game.
extern player_t *players;

// Player taking events, and displaying.
extern player_t *consoleplayer;
extern player_t *displayplayer;

#define MAXHEALTH 200
#define MAXARMOUR 200

#define CHEATARMOUR      MAXARMOUR
#define CHEATARMOURTYPE  ARMOUR_Blue

// Player spawn spots for deathmatch.
extern int max_deathmatch_starts;
extern spawnpoint_t *deathmatchstarts;
extern spawnpoint_t *deathmatch_p;

// Player spawn spots.
extern spawnpoint_t *playerstarts;

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
extern const char *cfgfile;
extern FILE *debugfile;

// if true, load DDF/RTS as external files (instead of from EDGE.WAD)
extern boolean_t external_ddf;

// if true, load all graphics at level load
extern boolean_t precache;

// if true, prefer to crash out on various errors
extern boolean_t strict_errors;

// if true, prefer to ignore or fudge various (serious) errors
extern boolean_t lax_errors;

// if true, disable warning messages
extern boolean_t no_warnings;

// if true, disable obsolete warning messages
extern boolean_t no_obsoletes;

// if true, enable HOM detection (hall of mirrors effect)
extern boolean_t hom_detect;

// wipegamestate can be set to -1
//  to force a wipe on the next draw
extern gamestate_t wipegamestate;

extern int mouseSensitivity;

extern boolean_t inhelpscreens;
extern int setblocks;
extern int quickSaveSlot;
extern int darken_screen;

// debug flag to cancel adaptiveness
extern boolean_t singletics;

extern int bodyqueslot;

// Needed to store the number of the dummy sky flat.
// Used for rendering, as well as tracking projectiles etc.

extern const struct image_s *skyflatimage;

#define IS_SKY(plane)  ((plane).image == skyflatimage)


//---------------------------------------------------
// Netgame stuff (buffers and pointers, i.e. indices).

// This is for use in the network communication.
extern doomcom_t *doomcom;

extern int maketic;

extern int ticdup;

//misc stuff
extern boolean_t newhud;
extern boolean_t rotatemap;
extern boolean_t showstats;
extern boolean_t swapstereo;
extern boolean_t infight;

extern int crosshair;
extern int screen_size;
extern int screenblocks;
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
extern int SCREENPITCH;
extern int SCREENBITS;
extern boolean_t SCREENWINDOW;

// transitional macros
#define BPP  (SCREENBITS / 8)
#define SCREENDEPTH  SCREENPITCH
#define in_a_window  SCREENWINDOW

// I_Video.c / V_Video*.c Precalc. Stuff
extern float_t DX, DY, DXI, DYI, DY2, DYI2;
extern int SCALEDWIDTH, SCALEDHEIGHT, X_OFFSET, Y_OFFSET;
extern float_t BASEYCENTER, BASEXCENTER;

extern boolean_t graphicsmode;

// -ES- 1999/08/15 Added teleport effects
extern int telept_effect;
extern int telept_flash;
extern int telept_reverse;
extern int wipe_reverse;
extern int wipe_method;

//mlook stuff
extern int mlookspeed;
extern int mlookon;
extern boolean_t invertmouse; // -ACB- 1999/09/03 Must be true or false - becomes boolean

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
extern boolean_t stageturn;
extern int forwardmovespeed;
extern int angleturnspeed;
extern int sidemovespeed;

#endif
