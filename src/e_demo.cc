//----------------------------------------------------------------------------
//  EDGE Demo Code
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
// -AJA- 2005/01/27: implemented new demo format
//
// See "docs/tech/demo_fmt.txt" for the new demo specs.
// 

#include "i_defs.h"
#include "e_demo.h"

#include "am_map.h"
#include "con_cvar.h"
#include "con_main.h"
#include "dem_chunk.h"
#include "dem_glob.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "dstrings.h"
#include "e_main.h"
#include "e_input.h"
#include "f_finale.h"
#include "g_game.h"
#include "m_argv.h"
#include "m_cheat.h"
#include "m_inline.h"
#include "m_misc.h"
#include "m_menu.h"
#include "m_random.h"
#include "hu_stuff.h"
#include "p_bot.h"
#include "p_local.h"
#include "p_setup.h"
#include "p_tick.h"
#include "r_data.h"
#include "r_layers.h"
#include "r_sky.h"
#include "r_view.h"
#include "rad_trig.h"
#include "s_sound.h"
#include "sv_chunk.h"
#include "sv_main.h"
#include "st_stuff.h"
#include "version.h"
#include "v_res.h"
#include "w_image.h"
#include "w_textur.h"
#include "w_wad.h"
#include "wi_stuff.h"
#include "z_zone.h"

#include "epi/epi.h"
#include "epi/endianess.h"
#include "epi/files.h"
#include "epi/filesystem.h"


// if true, exit with report on completion 
static bool timingdemo;

bool demorecording;
bool demo_notbegun;

bool demoplayback;
bool netdemo;

static epi::strent_c defdemoname;

static epi::file_c *demo_in = NULL;

// quit after playing a demo from cmdline 
bool singledemo;


static void DemoReadPCMD(void)
{
	DEM_PushReadChunk("Pcmd");

	int pnum = 0;

	for (;;)
	{
		if (DEM_GetError() != 0)
			break;  /// set error !!

		if (DEM_RemainingChunkSize() < 16)
			break;

		// find player
		while (! players[pnum] && pnum < MAXPLAYERS)
			pnum++;

		if (pnum >= MAXPLAYERS)
		{
			L_WriteDebug("LOAD_DEMO: more players in PCMD than in game\n");
			break;
		}

		DEM_GetTiccmd(&players[pnum]->cmd);
	}

	int remain = DEM_RemainingChunkSize();

	if (remain != 0)
		I_Warning("LOAD_DEMO: %d unused bytes in PCMD chunk\n", remain);

	DEM_PopReadChunk();
}

//
// E_DemoReadTick
//
void E_DemoReadTick(void)
{
	char marker[6];

	DEM_GetMarker(marker);

	if (strcmp(marker, DEMO_END_MARKER) == 0)
	{
		G_FinishDemo();
		return;
	}

	if (strcmp(marker, "Pack") == 0)
		I_Error("Demo is from future version of EDGE (has compression).\n");

	if (strcmp(marker, "Tick") != 0)
		I_Error("Corrupt demo, missing TICK chunk (got [%s])\n", marker);

	DEM_PushReadChunk(marker);

	for (;;)
	{
		if (DEM_GetError() != 0)
			break;  /// set error !!

		if (DEM_RemainingChunkSize() == 0)
			break;

		DEM_GetMarker(marker);

		if (strcmp(marker, "Pcmd") == 0)
		{
			DemoReadPCMD();
			continue;
		}

		// skip chunk
		I_Warning("LOAD_DEMO: Unknown TICK sub-chunk [%s]\n", marker);

		if (! DEM_SkipReadChunk(marker))
			break;
	}

	DEM_PopReadChunk();
}

//
// E_DemoWriteTick
//
void E_DemoWriteTick(void)
{
	DEM_PushWriteChunk("Tick");
	DEM_PushWriteChunk("Pcmd");

	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];
		if (! p) continue;

		DEM_PutTiccmd(&p->cmd);
	}

	DEM_PopWriteChunk();  // Pcmd

	// TODO: SYNC chunks

	// TODO: store game events

	DEM_PopWriteChunk();  // Tick
}

//
// G_RecordDemo 
// 
// 98-7-10 KM Demolimit removed
//
void G_RecordDemo(const char *filename)
{
	epi::string_c demoname;

	M_ComposeFileName(demoname, gamedir, filename);

	if (M_CheckExtension("edm", demoname.GetString()) == EXT_NONE)
		demoname += ".edm";

	usergame = false;

	// Write directly to file. Possibly a bit slower without disk cache, but
	// uses less memory, and the demo can record EDGE crashes.
	if (! DEM_OpenWriteFile(demoname.GetString(), (EDGEVER << 8) | EDGEPATCH))
		I_Error("Unable to create demo file: %s\n", demoname.GetString());

	demorecording = true;
	demo_notbegun = true;
}

//
// G_BeginRecording
//
// -ACB- 1998/07/02 Changed the code to record as version 0.65 (065),
//                  All of the additional EDGE features are stored in
//                  the demo.
//
// -KM-  1998/07/10 Removed the demo limit.
//
// -ACB- 1998/07/12 Removed Lost Soul/Spectre Ability Check
//
void G_BeginRecording(void)
{
	demo_notbegun = false;

	epi::string_c fn;

	saveglobals_t *globs = DEM_NewGLOB();

	L_WriteDebug("G_BeginRecording: %s\n", currmap->ddf.name.GetString());

	// --- fill in global structure ---

	globs->game = Z_StrDup(currmap->episode_name);
	globs->level = Z_StrDup(currmap->ddf.name);
	globs->flags = level_flags;
	globs->gravity = level_flags.menu_grav;

	globs->skill = gameskill;
	globs->netgame = netgame ? (1+deathmatch) : 0;
	globs->p_random = random_seed;
	globs->console_player = consoleplayer;

	time_t cur_time;
	char timebuf[100];

	time(&cur_time);
	strftime(timebuf, 99, "%I:%M %p  %d/%b/%Y", localtime(&cur_time));

	if (timebuf[0] == '0' && isdigit(timebuf[1]))
		timebuf[0] = ' ';

	globs->desc_date = Z_StrDup(timebuf);

	globs->mapsector.count = numsectors;
	globs->mapsector.crc = mapsector_CRC.crc;
	globs->mapline.count = numlines;
	globs->mapline.crc = mapline_CRC.crc;
	globs->mapthing.count = mapthing_NUM;
	globs->mapthing.crc = mapthing_CRC.crc;

	// FIXME: store DDF CRC values too...

	DEM_SaveGLOB(globs);
	DEM_FreeGLOB(globs);
}

//
// G_DeferredPlayDemo
//
void G_DeferredPlayDemo(const char *filename)
{
	epi::string_c demoname;

	M_ComposeFileName(demoname, gamedir, filename);

	if (M_CheckExtension("edm", demoname.GetString()) == EXT_NONE)
		demoname += ".edm";

	defdemoname.Set(demoname.GetString());

	gameaction = ga_playdemo;
}

//
// G_DeferredTimeDemo
//
void G_DeferredTimeDemo(const char *filename)
{
	nodrawers = M_CheckParm("-nodraw")?true:false;
	noblit = M_CheckParm("-noblit")?true:false;

	timingdemo = true;
	singletics = true;

	G_DeferredPlayDemo(filename);
}


//
// G_DoPlayDemo
//
// Sets up the system to play a demo.
//
// -KM-  1998/07/10 Displayed error message on screen and make demos limitless
// -ACB- 1998/07/12 Removed Lost Soul/Spectre Ability Check
// -ACB- 1998/07/12 Removed error message (became bloody annoying...)
//
void G_DoPlayDemo(void)
{
	gameaction = ga_nothing;

	epi::file_c *fp = epi::the_filesystem->Open(defdemoname.GetString(),
        epi::file_c::ACCESS_READ | epi::file_c::ACCESS_BINARY);

	if (! fp || ! DEM_OpenReadFile(fp))
	{
		I_Printf("LOAD-DEMO: cannot find file: %s\n", defdemoname.GetString());
		return;
	}

	int version;

	if (! DEM_VerifyHeader(&version))
	{
		I_Printf("LOAD-DEMO: Unknown demo format !\n");
		DEM_CloseReadFile();
		return;
	}

	if (version >= (0x130 << 8))
		I_Warning("Demo is for future version: EDGE %x.%02x !\n",
			(version >> 16), (version >> 8) & 0xFF);

	saveglobals_t *globs = DEM_LoadGLOB();

	if (!globs)
		I_Error("LOAD-DEMO: Bad demo file (missing GLOB)\n");

	// --- pull info from global structure ---

	newgame_params_c params;

	params.map = G_LookupMap(globs->level);
	if (! params.map)
		I_Error("LOAD-DEMO: No such map %s !  Check WADS\n", globs->level);

	params.game = gamedefs.Lookup(globs->game);
	if (!params.game)
		I_Error("LOAD-DEMO: No such episode/mod %s !  Check WADS\n", globs->game);

	params.skill       = (skill_t) globs->skill;
	params.deathmatch  = (globs->netgame >= 2) ? (globs->netgame - 1) : 0;

	params.random_seed = globs->p_random;
	params.warping     = false;

	// this player is a dummy one : FIXME !!!! use info in PLYR chunks
	params.total_players = 1;
	params.players[0] = PFL_Demo;

	//!!!! FIXME: compatability FLAG !!!

	// don't spend a lot of time in loadlevel
	precache = false;

	G_InitNew(params);

	level_flags = globs->flags;
	level_flags.menu_grav = globs->gravity;
	
	G_DoLoadLevel();
	G_SpawnInitialPlayers();

	// -- Check LEVEL consistency (crc) --
	//
	// FIXME: ideally we shouldn't bomb out, just display an error box

	if (globs->mapsector.count != numsectors ||
		globs->mapsector.crc != mapsector_CRC.crc ||
		globs->mapline.count != numlines ||
		globs->mapline.crc != mapline_CRC.crc ||
		globs->mapthing.count != mapthing_NUM ||
		globs->mapthing.crc != mapthing_CRC.crc)
	{
		I_Error("LOAD-DEMO: Level data does not match !  Check WADs\n");
	}

	DEM_FreeGLOB(globs);

	//!!! FIXME: Check DDF/RTS consistency (crc), warning only

	precache = true;
	usergame = false;
	demoplayback = true;
}

// 
// G_FinishDemo 
//
// Called after a death or level completion to allow demos to be cleaned up, 
// Returns true if a new demo loop action will take place 
// 
// -KM- 1998/07/10 Reformed code for limitless demo
//
bool G_FinishDemo(void)
{
	if (timingdemo)
	{
		int endtime = I_GetTime();

		float fps = float(gametic * TICRATE) / (endtime - starttime);

		I_Error("timed %i gametics in %i realtics, which equals %f fps", gametic,
			endtime - starttime, fps);
		/* NOT REACHED */
	}

	if (demoplayback)
	{
		epi::the_filesystem->Close(demo_in);
		demo_in = NULL;

		if (singledemo)
		{
			// -ACB- 1999/09/20 New code order, shutdown system then close program.
			I_SystemShutdown();
			I_CloseProgram(0);
		}

		netdemo = false;
		netgame = false;
		deathmatch = false;

		level_flags.fastparm = false;
		level_flags.nomonsters = false;

		// P_ShutdownLevel();
		P_DestroyAllPlayers();

		E_AdvanceDemo();
		return true;
	}

	if (demorecording)
	{
		demorecording = false;

		// Finish the demo file
		DEM_CloseWriteFile();

		I_Error("Demo recorded");
		/* NOT REACHED */
	}

	return false;
}

