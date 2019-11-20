//----------------------------------------------------------------------------
//  EDGE Networking (New)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2004-2009  The EDGE Team.
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

#include "system/i_defs.h"
#include "system/i_net.h"
#include <system\i_video.h>

#include <limits.h>
#include <stdlib.h>

#include "../epi/types.h"
#include "../epi/endianess.h"

#include "n_bcast.h"
#include "n_reliable.h"
#include "n_network.h"

#include <system\i_system.h>
#include "dm_state.h"
#include "e_input.h"
#include "e_main.h"
#include "g_game.h"
#include "e_player.h"
#include "m_argv.h"
#include "m_menu.h"
#include "m_random.h"

//
// NETWORKING
//
// gametic is the tic about to (or currently being) run
// maketic is the tick that hasn't had control made for it yet
// nettics[] has the maketics for all players
//
// a gametic cannot be run until nettics[] > gametic for all players

bool    ShowGun = true;
bool    drone = false;
bool    net_cl_new_sync = true;    // Use new client syncronisation code
float     offsetms;

ticcmd_t    netcmds[MAXPLAYERS][BACKUPTICS];
int         nettics[MAXNETNODES];
bool    nodeingame[MAXNETNODES];        // set false as nodes leave game
bool    remoteresend[MAXNETNODES];      // set when local needs tics
int         resendto[MAXNETNODES];          // set when remote needs tics
int         resendcount[MAXNETNODES];

int         nodeforplayer[MAXPLAYERS];

int         maketic = 0;
int         lastnettic = 0;
int         skiptics = 0;
int         ticdup = 0;
int         maxsend = 0;    // BACKUPTICS/(2*ticdup)-1
int         extratics;


void D_ProcessEvents(void);
void G_BuildTiccmd(ticcmd_t *cmd);
void D_Display(void);

bool renderinframe = false;

//
// GetAdjustedTime
// 30 fps clock adjusted by offsetms milliseconds
//

static int GetAdjustedTime(void) {
	int time_ms;

	time_ms = I_GetTimeMS();

	if (net_cl_new_sync) {
		// Use the adjustments from net_client.c only if we are
		// using the new sync mode.

		time_ms += (offsetms / FRACUNIT);
	}

	return (time_ms * TICRATE) / 1000;
}

#define DEBUG_TICS 0

// only true if packets are exchanged with a server
bool netgame = false;

int base_port;

DEF_CVAR(m_busywait, int, "c", 1);


int gametic;
int maketic;

static int last_update_tic;
static int last_tryrun_tic;

extern gameflags_t default_gameflags;

//
// PrintMD5Digest
//

static bool had_warning = false;
static void PrintMD5Digest(const char *s, byte *digest) 
{
	unsigned int i;

	I_Printf("%s: ", s);

	for (i = 0; i < sizeof(md5_digest_t); ++i) 
	{
		I_Printf("%02x", digest[i]);
	}

	I_Printf("\n");
}

//
// CheckMD5Sums
//

static void CheckMD5Sums(void) {
	bool correct_wad;

	if (!net_client_received_wait_data || had_warning) {
		return;
	}

	correct_wad = memcmp(net_local_wad_md5sum,
		net_server_wad_md5sum, sizeof(md5_digest_t)) == 0;

	if (correct_wad) {
		return;
	}
	else {
		I_Printf("Warning: WAD MD5 does not match server:\n");
		PrintMD5Digest("Local", net_local_wad_md5sum);
		PrintMD5Digest("Server", net_server_wad_md5sum);
		I_Printf("If you continue, this may cause your game to desync\n");
	}

	had_warning = true;
}


//----------------------------------------------------------------------------
//  TIC HANDLING
//----------------------------------------------------------------------------

void N_InitNetwork(void)
{
	srand(I_PureRandom());

	N_ResetTics();

	if (nonet)
		return;

	base_port = MP_EDGE_PORT;

	const char *str = M_GetParm("-port");
	if (str)
		base_port = atoi(str);

	I_Printf("Network: base port is %d\n", base_port);

	N_StartupReliableLink (base_port+0);
	N_StartupBroadcastLink(base_port+1);
}


// TEMP CRAP
bool N_OpenBroadcastSocket(bool is_host)
{ return true; }
void N_CloseBroadcastSocket(void)
{ }
void N_SendBroadcastDiscovery(void)
{ }

//
// D_NetWait
//

static void N_NetWait(void) 
{
	SDL_Event Event;
	unsigned int id = 0;

	if (M_CheckParm("-server") > 0) {
#ifdef USESYSCONSOLE
		I_Printf("D_NetWait: Waiting for players..\n\nPress ready key to begin game..\n\n");
#else
		I_Printf("D_NetWait: Waiting for players..\n\nWhen ready press any key to begin game..\n\n");
#endif
	}

	I_Printf("---------------------------------------------\n\n");

	while (net_waiting_for_start) 
	{
		CheckMD5Sums();

		if (id != net_clients_in_game) 
		{
			I_Printf("%s - %s\n", net_player_names[net_clients_in_game - 1],
				net_player_addresses[net_clients_in_game - 1]);
			id = net_clients_in_game;
		}

		NET_CL_Run();
		NET_SV_Run();

		if (!net_client_connected) 
		{
			I_Error("D_NetWait: Disconnected from server");
		}

		I_Sleep(100);

		while (SDL_PollEvent(&Event))
			if (Event.type == SDL_KEYDOWN) 
			{
				NET_CL_StartGame();
			}
	}
}



static void GetPackets(bool do_delay)
{
	if (! netgame)
	{
		// -AJA- This can make everything a bit "jerky" :-(
		//if (do_delay && ! m_busywait.d)
		//	I_Sleep(10 /* millis */);
		return;
	}

}

static void DoSendTiccmds(int tic)
{
}

bool N_BuildTiccmds(void)
{
	I_ControlGetEvents();
	E_ProcessEvents();

	if (numplayers == 0)
	{
		E_UpdateKeyState();
		return false;
	}

	if (maketic >= gametic + MP_SAVETICS)
	{
		E_UpdateKeyState();
		return false;  // can't hold any more
	}

	// build ticcmds (reworked here for possibly better performance).
	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];
		if (! p) continue;

		if (p->builder)
		{

			ticcmd_t *cmd = &p->in_cmds[maketic % BACKUPTICS];
			//ticcmd_t *cmd;

 //    L_WriteDebug("N_BuildTiccmds: pnum %d netgame %c\n", pnum, netgame ? 'Y' : 'n');

			//if (false) // FIXME: temp hack!!!  if (netgame)
			//	cmd = &p->out_cmds[maketic % (MP_SAVETICS*2)];
			//else
			//	cmd = &p->in_cmds[maketic % (MP_SAVETICS*2)];

			p->builder(p, p->build_data, cmd);
			
			//cmd->consistency = p->consistency[maketic % (MP_SAVETICS*2)];
		}
	}

	E_UpdateKeyState();

	if (netgame)
		DoSendTiccmds(maketic);

	maketic++;
	return true;
}

#if 0
int N_NetUpdate(bool do_delay)
{
	int nowtime = I_GetTime();

	if (singletics)  // singletic update is syncronous
		return nowtime;

	int newtics = nowtime - last_update_tic;
	last_update_tic = nowtime;

	if (newtics > 0)
	{
		// build and send new ticcmds for local players
		int t;
		for (t = 0; t < newtics; t++)
		{
			if (!N_BuildTiccmds())
				break;
		}

		//		if (t != newtics && numplayers > 0)
		//			L_WriteDebug("N_NetUpdate: lost tics: %d\n", newtics - t);
	}

	// If no true Network Mode, why do we still need to GetPackets?
	GetPackets(do_delay);

	return nowtime;
}
#endif // 0


//
// NetUpdate
// Builds ticcmds for console player,
// sends out a packet
//
int gametime = 0;

void N_NetUpdate(void) 
{
	int nowtime;
	int newtics;
	int i;
	int gameticdiv;

	if (renderinframe) 
	{
		return;
	}

#ifdef FEATURE_MULTIPLAYER

	// Run network subsystems

	NET_CL_Run();
	NET_SV_Run();

#endif

	// check time
	nowtime = GetAdjustedTime() / ticdup;
	newtics = nowtime - gametime;
	gametime = nowtime;

	if (skiptics <= newtics) {
		newtics -= skiptics;
		skiptics = 0;
	}
	else {
		skiptics -= newtics;
		newtics = 0;
	}

	// build new ticcmds for console player(s)
	gameticdiv = gametic / ticdup;
	for (i = 0; i < newtics; i++) {
		ticcmd_t cmd;

		I_StartTic();
		D_ProcessEvents();
		//if (maketic - gameticdiv >= BACKUPTICS/2-1)
		//    break;          // can't hold any more

		M_Ticker();

		if (drone) {  // In drone mode, do not generate any ticcmds.
			continue;
		}

		if (net_cl_new_sync) {
			// If playing single player, do not allow tics to buffer
			// up very far

			if ((!netgame || demoplayback) && maketic - gameticdiv > 2) {
				break;
			}

			// Never go more than ~200ms ahead
			if (maketic - gameticdiv > 8) {
				break;
			}
		}
		else {
			if (maketic - gameticdiv >= 5) {
				break;
			}
		}

		G_BuildTiccmd(&cmd);

#ifdef FEATURE_MULTIPLAYER

		if (netgame && !demoplayback) {
			NET_CL_SendTiccmd(&cmd, maketic);
		}

#endif
		netcmds[consoleplayer][maketic % BACKUPTICS] = cmd;

		++maketic;
		nettics[consoleplayer] = maketic;
	}
}


//
// PlayersInGame
// Returns true if there are currently any players in the game.
//

bool PlayersInGame(void) 
{
	int i;

	for (i = 0; i < MAXPLAYERS; ++i) {
		if (playeringame[i]) {
			return true;
		}
	}

	return false;
}

int DetermineLowTic(void)
{
	int i;
	int lowtic;

	if (net_client_connected) 
	{
		lowtic = INT_MAX;

		for (i = 0; i < MAXPLAYERS; ++i) {
			if (playeringame[i]) {
				if (nettics[i] < lowtic) {
					lowtic = nettics[i];
				}
			}
		}
	}
	else {
		lowtic = maketic;
	}

	return lowtic;
}

DEF_CVAR(r_lerp, int, "c", 1);
DEF_CVAR(r_maxfps, int, "c", 0);
DEF_CVAR(r_vsync, int, "c", 0);
static bool forwardinterpolate = false;
static float interpolate_point = 1;

/*
	=0.0 last frame
	=0.5 halfway between last and current frame
	=1.0 current frame
	>1.0 forward predict
*/
float N_CalculateCurrentSubTickPosition(void)
{
	//I wonder if there could be issues here if the game has been running for a long time?
	return ((float)I_GetMillies() / (1000.0f/35.0f)) - (float)last_update_tic;
}

float N_Interpolate(float previous, float next)
{
	return	next*interpolate_point + (previous - previous*interpolate_point);
}

float N_GetInterpolater(void)
{
	return interpolate_point;
}

void N_ResetInterpolater(void)
{
	interpolate_point = 1;
}

void N_SetInterpolater(void)
{
	if (paused || menuactive)
		return;
	
	if (r_lerp.d)
	{
		interpolate_point = N_CalculateCurrentSubTickPosition();
		if (!forwardinterpolate && interpolate_point > 1)
			interpolate_point = 1;
		if (interpolate_point < 0)
			interpolate_point = 0;
	}
	else
	{
		N_ResetInterpolater();
	}
}

int N_TryRunTics(bool *is_fresh)
{
	*is_fresh = true;

	if (singletics)
	{
		N_BuildTiccmds();

		return 1;
	}

	int nowtime = N_NetUpdate();
	int realtics = nowtime - last_tryrun_tic;
	last_tryrun_tic = nowtime;

//#ifdef DEBUG_TICS
//L_WriteDebug("N_TryRunTics: now %d last_tryrun %d --> real %d\n",
//nowtime, nowtime - realtics, realtics);
//#endif

	// simpler handling when no game in progress
	if (numplayers == 0)
	{
		while (realtics <= 0)
		{
			nowtime = N_NetUpdate(true);
			realtics = nowtime - last_tryrun_tic;
			last_tryrun_tic = nowtime;
		}

		if (realtics > MP_SAVETICS)
			realtics = MP_SAVETICS;

		return realtics;
	}

	int lowtic = DetermineLowTic();
	int availabletics = lowtic - gametic;

	// this shouldn't happen, since we can only run a gametic when
	// the ticcmds for _all_ players have arrived (hence lowtic >= gametic);
	SYS_ASSERT(availabletics >= 0);

	// decide how many tics to run
	int counts;

	if (realtics + 1 < availabletics)
		counts = realtics + 1;
	else
		counts = MIN(realtics, availabletics);

/* #ifdef DEBUG_TICS
	L_WriteDebug("=== lowtic %d gametic %d | real %d avail %d raw-counts %d\n",
		lowtic, gametic, realtics, availabletics, counts);
#endif */

	if (counts < 1)
		counts = 1;

	// wait for new tics if needed
	while (lowtic < gametic + counts)
	{
		int wait_tics = N_NetUpdate(true) - last_tryrun_tic;

		lowtic = DetermineLowTic();

		SYS_ASSERT(lowtic >= gametic);

		// don't stay in here forever -- give the menu a chance to work
		if (wait_tics > TICRATE/2)
		{
			L_WriteDebug("Waited %d tics IN VAIN !\n", wait_tics);
			*is_fresh = false;
			return 3;
		}
	}

	return counts;
}

void N_ResetTics(void)
{
	maketic = gametic = 0;

	last_update_tic = last_tryrun_tic = I_GetTime();
}

void N_QuitNetGame(void)
{
	if (debugfile) {
		fclose(debugfile);
	}

#ifdef FEATURE_MULTIPLAYER
	NET_SV_Shutdown();
	NET_CL_Disconnect();
#endif
}

#define TURBOTHRESHOLD  0x32

void N_TiccmdTicker(void)
{
	// gametic <= maketic is a system-wide invariant.  However,
	// new levels are loaded during G_Ticker(), resetting them
	// both to zero, hence if we increment gametic here, we
	// break the invariant.  The following is a workaround.
	// TODO: this smells like a hack -- fix it properly!
	if (gametic >= maketic)
	{ 
		if (! (gametic == 0 && maketic == 0) && !netgame)
			I_Printf("WARNING: G_TiccmdTicker: gametic >= maketic (%d >= %d)\n", gametic, maketic);
		return;
	}

	int buf = gametic % BACKUPTICS;

	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];
		if (! p) continue;

		memcpy(&p->cmd, p->in_cmds + buf, sizeof(ticcmd_t));

		// check for turbo cheats
		if (p->cmd.forwardmove > TURBOTHRESHOLD
			&& !(gametic & 31) && (((gametic >> 5) + p->pnum) & 0x1f) == 0)
		{
			// FIXME: something better for turbo cheat
			I_Printf(language["IsTurbo"], p->playername);
		}

		if (netgame)
		{
			if (gametic > BACKUPTICS && p->consistency[buf] != p->cmd.consistency)
			{
                    I_Warning("Consistency failure on player %d (%i should be %i)",
					p->pnum + 1, p->cmd.consistency, p->consistency[buf]);
			}
			if (p->mo)
				p->consistency[buf] = (int)p->mo->x;
			else
				p->consistency[buf] = P_ReadRandomState() & 0xff;
		}
	}

	gametic++;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
