//----------------------------------------------------------------------------
//  EDGE Networking (New)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2004  The EDGE Team.
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
#include "nl.h"

#include "epi/epitype.h"
#include "epi/epiendian.h"

#include "protocol.h"
#include "n_packet.h"

#include "e_main.h"
#include "e_player.h"
#include "g_game.h"
#include "p_local.h"
#include "z_zone.h"


extern gameflags_t default_gameflags;

static int client_id;
static int game_id;

static NLaddress server;
static NLsocket  socket;
static NLint     sk_group;

static packet_c pk;


static void N_Preliminaries(void)
{
	nlHint(NL_REUSE_ADDRESS, NL_TRUE);

    if (nlSelectNetwork(NL_IP) == NL_FALSE)
		I_Error("nlSelectNetwork failed:\n%s", N_GetErrorStr());

	NLaddress local;

#ifdef LINUX  // !!!! FIXME: TEMP HACK
	nlStringToAddr("192.168.0.197", &local);
	nlSetLocalAddr(&local);
#endif
}

static bool N_TestServer(NLsocket sock)
{
	pk.SetType("bd");
	pk.hd().flags = 0;
	pk.hd().data_len = 0;

	if (! pk.Write(sock))
		I_Error("Unable to write BD packet:\n%s", N_GetErrorStr());

	int count = 0;

	for (;;)
	{
		sleep(1);

		if (pk.Read(sock))
			break;

		count++;

		if (count == 5)
			return false;
	}

	nlGetRemoteAddr(sock, &server);

	fprintf(stderr, "Found server: %s\n", N_GetAddrName(&server));

	return true;
}

static void N_FindServer(void)
{
	// test local computer

	NLsocket loc_sock = nlOpen(0, NL_UNRELIABLE);

    if (loc_sock == NL_INVALID)
		I_Error("Unable to create local socket:\n%s", N_GetErrorStr());

	NLaddress loc_addr;

	nlGetLocalAddr(loc_sock, &loc_addr);

	fprintf(stderr, "Local addr %s\n", N_GetAddrName(&loc_addr));

	nlSetAddrPort(&loc_addr, 26710);

	if (! nlSetRemoteAddr(loc_sock, &loc_addr))
		I_Error("Set remote address on local socket failed.\n%s", N_GetErrorStr());

	if (N_TestServer(loc_sock))
	{
		nlClose(loc_sock);
		return;
	}

	fprintf(stderr, "Server not on same computer, trying LAN...\n");

	//--------------------------------------------------------

	NLsocket bc_sock = nlOpen(26710, NL_BROADCAST);

    if (bc_sock == NL_INVALID) // FIXME: don't error on this
		I_Error("Unable to create broadcast socket:\n%s", N_GetErrorStr());

	NLaddress remote;
	nlStringToAddr("192.168.0.255:26710", &remote);  //!!! HACK

	if (! nlSetRemoteAddr(bc_sock, &remote))
		I_Error("Set remote address on broadcast socket failed.\n%s", N_GetErrorStr());

	if (! N_TestServer(bc_sock))
		I_Error("Server not found (didn't receive reply).\n");

	nlClose(bc_sock);
}

static void N_ConnectServer(void)
{
	socket = nlOpen(0, NL_UNRELIABLE);

    if (socket == NL_INVALID)
		I_Error("Unable to create main socket:\n%s", N_GetErrorStr());

	nlSetRemoteAddr(socket, &server);

	sk_group = nlGroupCreate();

	if (sk_group == NL_INVALID || ! nlGroupAddSocket(sk_group, socket))
		I_Error("Unable to create socket group (no memory)\n");

	pk.SetType("cs");
	pk.hd().flags = 0;
	pk.hd().data_len = sizeof(connect_proto_t);

	connect_proto_t& cs = pk.cs_p();

///!!!	cs.protocol_ver = MP_PROTOCOL_VER;

	strcpy(cs.info.name, "Flynn");

	cs.ByteSwap();

	if (! pk.Write(socket))
		I_Error("Unable to write CS packet:\n%s", N_GetErrorStr());

	sleep(1);
	sleep(1);

	if (! pk.Read(socket))
		I_Error("Failed to connect to server (no reply)\n");
	
	if (pk.CheckType("Er"))
		I_Error("Failed to connect to server:\n%s", pk.er_p().message);

	if (! pk.CheckType("Cs"))
		I_Error("Failed to connect to server: (invalid reply)\n");

	client_id = pk.hd().source;
	
	fprintf(stderr, "Connected to server: client_id %d\n", client_id);
}

static void N_NewGame(void)
{
	nlSetRemoteAddr(socket, &server);

	pk.SetType("ng");
	pk.hd().flags = 0;
	pk.hd().data_len = sizeof(new_game_proto_t);

	new_game_proto_t& ng = pk.ng_p();

	strcpy(ng.info.game_name,  "DOOM2");
	strcpy(ng.info.level_name, "MAP01");

	ng.info.mode  = 'D';
	ng.info.skill = '4';

	ng.info.min_players = 1;
	ng.info.engine_ver  = 0x129;

	ng.ByteSwap();

	if (! pk.Write(socket))
		I_Error("Unable to write NG packet:\n%s", N_GetErrorStr());

	sleep(1);
	sleep(1);

	if (! pk.Read(socket))
		I_Error("Failed to create new game (no reply)\n");
	
	if (pk.CheckType("Er"))
		I_Error("Failed to create new game:\n%s", pk.er_p().message);

	if (! pk.CheckType("Ng"))
		I_Error("Failed to create new game (invalid reply)\n");

	ng.ByteSwap();

	game_id = pk.hd().game;

	fprintf(stderr, "Created new game: %d\n", game_id);
}

static void N_Vote(void)
{
	nlSetRemoteAddr(socket, &server);

	pk.SetType("vp");
	pk.hd().flags = 0;
	pk.hd().data_len = 0;

	if (! pk.Write(socket))
		I_Error("Unable to write VP packet:\n%s", N_GetErrorStr());

	sleep(1);
	sleep(1);

	if (! pk.Read(socket))
		I_Error("Failed to vote (no reply)\n");
	
	if (pk.CheckType("Er"))
		I_Error("Failed to vote:\n%s", pk.er_p().message);

	if (! pk.CheckType("Pg"))
		I_Error("Failed to vote (invalid reply)\n");

	fprintf(stderr, "LET THE GAMES BEGIN !!!\n");
}

//
// N_InitiateGame
//
void N_InitiateGame(void)
{
	pk.Clear();

	N_Preliminaries();
	N_FindServer();
	N_ConnectServer();
	N_NewGame();
	N_Vote();
}

//----------------------------------------------------------------------------

static int update_tic;

void E_CheckNetGame(void)
{
	// FIXME: singletics, WTF ???

#if 0
	N_InitiateGame();

	netgame = true;
#else
	netgame = false;
#endif

	for (int i = 0; i < 1; i++)
		P_CreatePlayer(i);

	consoleplayer = 0;
	DEV_ASSERT2(players[consoleplayer]);

	G_SetConsolePlayer(consoleplayer);
	G_SetDisplayPlayer(consoleplayer);

	// -ES- Fixme: This belongs somewhere else (around G_PlayerReborn).
	// Needs a big cleanup though.
	players[consoleplayer]->builder = P_ConsolePlayerBuilder;
	players[consoleplayer]->build_data = NULL;

#if 0
	global_flags = default_gameflags;
	level_flags  = global_flags;

	startskill = (skill_t)4;
	deathmatch = true;

	startmap = Z_StrDup("MAP01");

	autostart = true;
#endif

	update_tic = I_GetTime();
}

static void GetPackets(bool do_delay)
{
	if (! netgame)
	{
		//!!!! FIXME: sleep for 10 millisec if do_delay true
		return;
	}

	NLsocket socks[4];  // only one in the group

	int delay = do_delay ? 10 /* millis */ : 0;
	int num = nlPollGroup(sk_group, NL_READ_STATUS, socks, 4, delay);

	if (num < 1)
		return;

	if (! pk.Read(socks[0]))
		return;

	I_Printf("GOT PACKET [%c%c]\n", pk.hd().type[0], pk.hd().type[1]);

	if (! pk.CheckType("Tg"))
		return;

	// FIXME: validate packet, check size

	tic_group_proto_t& tg = pk.tg_p();

	tg.ByteSwap();

	player_t *p = players[consoleplayer]; //!!!!

	int got_tic = pk.hd().request_id;

I_Printf("-- Got TG: counter = %d  in_tic = %d\n", got_tic, p->in_tic);

	if (got_tic != p->in_tic)
	{
		I_Printf("GOT TIC %d != EXPECTED %d\n", got_tic, p->in_tic);

		// FIXME: handle "future" tics (save them, set bit in mask)
		return;
	}

	// !!!! FIXME: MUST SWAP BYTES
	memcpy(p->in_cmds + (got_tic % BACKUPTICS), tg.tic_cmds, sizeof(ticcmd_t));

	p->in_tic++;
}

static void DoSendTiccmd(player_t *p, ticcmd_t *cmd, int tic)
{
	pk.Clear();  // FIXME: TESTING ONLY

	pk.SetType("tc");
	pk.hd().flags = 0;
	pk.hd().source = client_id;
	pk.hd().request_id = tic;  // FIXME !!!! 16 bits, will run out quickly
	pk.hd().data_len = sizeof(ticcmd_proto_t);

    ticcmd_proto_t& tc = pk.tc_p();

	memcpy(&tc.tic_cmd, p->out_cmds + (tic % BACKUPTICS), sizeof(ticcmd_t));

    tc.ByteSwap();

I_Printf("Writing ticcmd for tic %d\n", tic);
	if (! pk.Write(socket))
		I_Printf("Failed to write packet (pnum %d tic %d)\n", p->pnum, tic);
	
	// FIXME: need an 'out_tic' for resends....
}

static bool DoBuildTiccmds(void)
{
	I_ControlGetEvents();
	E_ProcessEvents();

	if (maketic - gametic >= BACKUPTICS / 2 - 1)
		return false;  // can't hold any more

	// build ticcmds
	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];
		if (! p) continue;

		if (p->builder)
		{
			ticcmd_t *cmd;

			if (netgame)
				cmd = &p->out_cmds[maketic % BACKUPTICS];
			else
				cmd = &p->in_cmds[maketic % BACKUPTICS];

			p->builder(p, p->build_data, cmd);
			
			cmd->consistency = p->consistency[maketic % BACKUPTICS];

			if (netgame)
				DoSendTiccmd(p, cmd, maketic);
		}
	}

	maketic++;
	return true;
}

int E_NetUpdate(bool do_delay)
{
	if (singletics)  // singletic update is syncronous
		return 0;

	int nowtime = I_GetTime();
	int newtics = nowtime - update_tic;

	update_tic = nowtime;

	if (newtics > 0)
	{
		// build and send new ticcmds for local players
		for (int t = 0; t < newtics; t++)
		{
			if (! DoBuildTiccmds())
				break;
		}
	}

	GetPackets(do_delay);

	return newtics;
}

int DetermineLowTic(void)
{
	if (! netgame)
		return maketic;

	int lowtic = INT_MAX;

	for (int pnum = 0; pnum < num_players; pnum++)
	{
		player_t *p = players[pnum];
		if (! p) continue;

		lowtic = MIN(lowtic, p->in_tic);
	}

	return lowtic;
}

int E_TryRunTics(void)
{
	DEV_ASSERT2(! singletics);

	int realtics = E_NetUpdate();

	int lowtic = DetermineLowTic();
	int availabletics = lowtic - gametic;

	// this shouldn't happen, since we can only run a gametic when
	// the ticcmds for _all_ players have arrived (hence lowtic >= gametic);
	DEV_ASSERT2(availabletics >= 0);

	// decide how many tics to run
	int counts;

	if (realtics < availabletics - 1)
		counts = realtics + 1;
	else if (realtics < availabletics)
		counts = realtics;
	else
		counts = availabletics;

#if 0 // DEBUGGING
	I_Printf("=== lowtic %d gametic %d | real %d avail %d counts %d\n",
	lowtic, gametic, realtics, availabletics, counts);
#endif

	if (counts < 1)
		counts = 1;

	// wait for new tics if needed
	int wait_tics = 0;

	while (lowtic < gametic + counts)
	{
		wait_tics += E_NetUpdate(true);

		lowtic = DetermineLowTic();

		DEV_ASSERT2(lowtic >= gametic);

		// don't stay in here forever -- give the menu a chance to work
		if (wait_tics > TICRATE/2)
			return 0;
	}

	return counts;
}

void E_QuitNetGame(void)
{
	// !!!! FIXME: E_QuitNetGame
}

