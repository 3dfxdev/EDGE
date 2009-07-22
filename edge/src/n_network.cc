//----------------------------------------------------------------------------
//  EDGE Networking (New)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2004-2008  The EDGE Team.
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

#include "i_defs.h"
#include "i_net.h"

#include <limits.h>
#include <stdlib.h>

#include "epi/types.h"
#include "epi/endianess.h"

#include "ddf/language.h"

#include "n_reliable.h"
#include "n_network.h"

#include "g_state.h"
#include "e_input.h"
#include "e_main.h"
#include "g_game.h"
#include "e_player.h"
#include "m_argv.h"
#include "m_random.h"

// #define DEBUG_TICS 1

// only true if packets are exchanged with a server
bool netgame = false;

int base_port;


cvar_c m_busywait;


int gametic;
int maketic;

static int last_update_tic;
static int last_tryrun_tic;


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
}


static void GetPackets(bool do_delay)
{
	if (! netgame)
	{
		// -AJA- This can make everything a bit "jerky" :-(
		if (do_delay && ! m_busywait.d)
			I_Sleep(10 /* millis */);

		return;
	}
}

static void DoSendTiccmds(int tic)
{
}

bool N_BuildTiccmds(void)
{
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

	// build ticcmds
	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];
		if (! p) continue;

		if (p->builder)
		{
			ticcmd_t *cmd;

///     L_WriteDebug("N_BuildTiccmds: pnum %d netgame %c\n", pnum, netgame ? 'Y' : 'n');

			if (false) // FIXME: temp hack!!!  if (netgame)
				cmd = &p->out_cmds[maketic % (MP_SAVETICS*2)];
			else
				cmd = &p->in_cmds[maketic % (MP_SAVETICS*2)];

			p->builder(p, p->build_data, cmd);
			
			cmd->consistency = p->consistency[maketic % (MP_SAVETICS*2)];
		}
	}

	E_UpdateKeyState();

	if (netgame)
		DoSendTiccmds(maketic);

	maketic++;
	return true;
}

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
			if (! N_BuildTiccmds())
				break;
		}

		if (t != newtics && numplayers > 0)
			L_WriteDebug("N_NetUpdate: lost tics: %d\n", newtics - t);
	}

	GetPackets(do_delay);

	return nowtime;
}

int DetermineLowTic(void)
{
	if (! netgame)
		return maketic;

	int lowtic = INT_MAX;

	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];
		if (! p) continue;

		// ignore bots
		if (p->playerflags & PFL_Bot)
			continue;

		if (p->playerflags & PFL_Console)
			lowtic = MIN(lowtic, maketic); // correct?????
		else
			lowtic = MIN(lowtic, p->in_tic);
	}

	return lowtic;
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

#ifdef DEBUG_TICS
L_WriteDebug("N_TryRunTics: now %d last_tryrun %d --> real %d\n",
nowtime, nowtime - realtics, realtics);
#endif

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

#ifdef DEBUG_TICS
	L_WriteDebug("=== lowtic %d gametic %d | real %d avail %d raw-counts %d\n",
		lowtic, gametic, realtics, availabletics, counts);
#endif

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
	// !!!! FIXME: N_QuitNetGame
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
// !!!! DEBUG //	I_Warning("Consistency failure on player %d (%i should be %i)",
//					p->pnum + 1, p->cmd.consistency, p->consistency[buf]);
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
