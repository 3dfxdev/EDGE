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

#include "i_defs.h"
#include "e_demo.h"

#include "am_map.h"
#include "con_cvar.h"
#include "con_main.h"
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

#include "epi/epiendian.h"


// if true, exit with report on completion 
static bool timingdemo;

bool demorecording;
bool demoplayback;
bool netdemo;
bool newdemo;

// 98-7-10 KM Remove maxdemo limit
static const byte *playdemobuffer = NULL;
static FILE *demofile = NULL;
static int demo_p;
// -ES- 2000/01/28 Added.
static int demo_length;
int maxdemo;

// quit after playing a demo from cmdline 
bool singledemo;


//
// DEMO RECORDING 
// 
#define DEMOMARKER              0x80

//
// Writes data to the demo
//
// -ES- 1999/10/17 Added.
//
static void WriteToDemo(const void *src, int length)
{
	fwrite(src,1,length,demofile);
	// This will make sure that also the last bytes are written at a crash
	fflush(demofile);
}

//
// Stores a single byte to the demo.
//
static void WriteByteToDemo(byte c)
{
	WriteToDemo(&c, 1);
}

//
// G_ReadDemoTiccmd
//
// A demo file is essentially a stream of ticcmds: every tic,
// the ticcmd holds all the info for movement for a player on
// that tic. This means that a demo merely replays the movements
// and actions of the player.
//
// This function gets the actions from the recdemobuffer and gives
// them to ticcmd to be played out. Its worth a note that this
// is the reason demos desync when played on two different
// versions, since any alteration to the gameplay could give
// a different reaction to a player action and therefore the
// game is different to the original.
//  
void G_ReadDemoTiccmd(ticcmd_t * cmd)
{
	// 98-7-10 KM Demolimit removed
	if (demo_p >= demo_length)
	{
		// end of demo data stream
		G_FinishDemo();
		return;
	}

	// -ACB- 1998/07/11 Added additional ticcmd stuff to demo
	// -MH-  1998/08/18 Added same for fly up/down
	//                  Keep all upward stuff before all forward stuff, to
	//                  keep consistent. Will break existing demos. Damn.
	*cmd = *(ticcmd_t *)&playdemobuffer[demo_p];
	demo_p += sizeof(ticcmd_t);
}

//
// G_WriteDemoTiccmd
//
// A demo file is essentially a stream of ticcmds: every tic,
// the ticcmd holds all the info for movement for a player on
// that tic. This means that a demo merely replays the movements
// and actions of the player.
//
// This function writes the ticcmd to the recdemobuffer and
// then get G_ReadDemoTiccmd to read it, so that whatever is
// recorded is played out. 
//
void G_WriteDemoTiccmd(ticcmd_t * cmd)
{
	// press q to end demo recording
	if (E_InputCheckKey((int)('q')))
		G_FinishDemo();

	// -ACB- 1998/07/11 Added additional ticcmd stuff to demo
	// -MH-  1998/08/18 Added same for fly up/down
	//                  Keep all upward stuff before all forward stuff, to
	//                  keep consistent. Will break existing demos. Damn.
	WriteToDemo(cmd, sizeof(ticcmd_t));
}

//
// G_RecordDemo 
// 
// 98-7-10 KM Demolimit removed
//
void G_RecordDemo(const char *name)
{
	// assume demo name is less than 256 chars
	epi::string_c demoname;
		
	M_ComposeFileName(demoname, gamedir, name);
	demoname += ".lmp";	// FIXME!! Check extension has not been given
	
	usergame = false;
	maxdemo = 0x20000;

	// Write directly to file. Possibly a bit slower without disk cache, but
	// uses less memory, and the demo can record EDGE crashes.
	demofile = fopen(demoname.GetString(), "wb");	
	demorecording = true;
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
	int i;
	///  player_t *p;

	demo_p = 0;

	WriteByteToDemo(DEMOVERSION);

	if ((int)gameskill == -1)
		gameskill = startskill;

	//---------------------------------------------------------
	// -ACB- 1998/09/03 Record Level Name In Demo
	i = (int)strlen(currmap->ddf.name);
	WriteByteToDemo(i);
	WriteToDemo(currmap->ddf.name, i);
	L_WriteDebug("G_BeginRecording: %s\n", currmap->ddf.name.GetString());
	//---------------------------------------------------------

	WriteByteToDemo(gameskill);
	WriteByteToDemo(deathmatch);
	WriteByteToDemo(consoleplayer);
	WriteToDemo(&level_flags, sizeof(level_flags));

	///  for (p = players; p; p = p->next)
	///    WriteByteToDemo(p->in_game);

	i = EPI_LE_S32(random_seed);
	WriteToDemo(&i, 4);
}

//
// G_DeferredPlayDemo
//
epi::strent_c defdemoname;

void G_DeferredPlayDemo(const char *name)
{
	defdemoname.Set(name);
	gameaction = ga_playdemo;
}

//
// G_DoPlayDemo
// Sets up the system to play a demo.
//
// -ACB- 1998/07/02 Change the code only to play version 0.65 demos.
// -KM-  1998/07/10 Displayed error message on screen and make demos limitless
// -ACB- 1998/07/12 Removed Lost Soul/Spectre Ability Check
// -ACB- 1998/07/12 Removed error message (became bloody annoying...)
//
void G_DoPlayDemo(void)
{
	skill_t skill;
	int i,j;
	int demversion;
	char mapname[30];
	const gamedef_c *newgamedef;
	const mapdef_c *newmap;
	long random_seed;
	///  player_t *p;

	gameaction = ga_nothing;
	playdemobuffer = (const byte*)W_CacheLumpName(defdemoname);
	demo_p = 0;
	demversion = playdemobuffer[demo_p++];

	// -ES- 1999/10/17 Allow cut off demos: Add a demo marker if it doesn't exist.
	demo_length = W_LumpLength(W_GetNumForName(defdemoname));
	if (demo_length < 16)
		// no real demo could be smaller than 16 bytes
		I_Error("Demo '%s' is too small!", defdemoname.GetString());
	if (playdemobuffer[demo_length-1] != DEMOMARKER)
		I_Warning("Warning: Demo has no end marker! It might be corrupt.\n");
	else
		demo_length--;

	if (demversion != DEMOVERSION)
	{
		gameaction = ga_nothing;
		return;
	}
	else
	{
		//------------------------------------------------------
		// -ACB- 1998/09/03 Read the Level Name from the demo.
		i = playdemobuffer[demo_p++];

		for (j = 0; j < i; j++)
			mapname[j] = playdemobuffer[demo_p + j];
		mapname[i] = 0;

		demo_p += i;
		//------------------------------------------------------

		skill = (skill_t) playdemobuffer[demo_p++];
		deathmatch = playdemobuffer[demo_p++];
		G_SetConsolePlayer(playdemobuffer[demo_p++]);

		level_flags = *(gameflags_t *)&playdemobuffer[demo_p];
		demo_p += sizeof(level_flags);

		/// FIXME: !!!
		///    for (p = players; p; p = p->next)
		///      p->in_game = playdemobuffer[demo_p++];

		// -ES- 2000/02/04 Random seed
		random_seed = EPI_LE_S32(*(long*)&playdemobuffer[demo_p]);
		demo_p += 4;
	}

	//----------------------------------------------------------------
	// -ACB- 1998/09/03 Setup the given mapname; fail if map does not
	// exist.
	newmap = game::LookupMap(mapname);
	if (newmap == NULL)
	{
		gameaction = ga_nothing;
		return;
	}

	newgamedef = gamedefs.Lookup(newmap->episode_name);
	if (newgamedef == NULL)
	{
		gameaction = ga_nothing;
		return;
	}


	//----------------------------------------------------------------

#if 0  // WTF???
	if (players->next && players->next->in_game)
	{
		netgame = true;
		netdemo = true;
	}
#endif

	// don't spend a lot of time in loadlevel
	precache = false;

	G_InitNew(skill, newmap, newgamedef, random_seed);
	G_DoLoadLevel();

	precache = true;
	usergame = false;
	demoplayback = true;
}

//
// G_DeferredTimeDemo
//
void G_DeferredTimeDemo(const char *name)
{
	nodrawers = M_CheckParm("-nodraw")?true:false;
	noblit = M_CheckParm("-noblit")?true:false;
	timingdemo = true;
	singletics = true;

	defdemoname.Set(name);
	gameaction = ga_playdemo;
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
	int endtime;

	if (timingdemo)
	{
		float fps;

		endtime = I_GetTime();
		fps = ((float)(gametic * TICRATE)) / (endtime - starttime);
		I_Error("timed %i gametics in %i realtics, which equals %f fps", gametic,
			endtime - starttime, fps);
	}

	if (demoplayback)
	{
		if (singledemo)
		{
			// -ACB- 1999/09/20 New code order, shutdown system then close program.
			I_SystemShutdown();
			I_CloseProgram(0);
		}

		W_DoneWithLump(playdemobuffer);
		demoplayback = false;
		netdemo = false;
		netgame = false;
		deathmatch = false;

		//!!! FIXME: this is wrong
#if 0
		for (p = players; p; p = p->next)
			p->in_game = false;
#endif

		level_flags.fastparm = false;
		level_flags.nomonsters = false;
		consoleplayer = 0; //???
		E_AdvanceDemo();
		return true;
	}

	if (demorecording)
	{
		// Finish it
		WriteByteToDemo(DEMOMARKER);
		// Finish the demo file:
		fclose(demofile);
		demofile = NULL;
		I_Error("Demo recorded");

		demorecording = false;
	}

	return false;
}

