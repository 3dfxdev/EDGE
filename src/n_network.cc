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

#ifdef USE_HAWKNL
#include "nl.h"
#endif

#include "epi/types.h"
#include "epi/endianess.h"

#include "protocol.h"
#include "n_packet.h"
#include "n_network.h"

#include "e_input.h"
#include "e_main.h"
#include "e_player.h"
#include "g_game.h"
#include "p_bot.h"
#include "p_local.h"
#include "version.h"
#include "z_zone.h"


bool var_hogcpu = true;


extern gameflags_t default_gameflags;

#ifdef USE_HAWKNL
static int client_id;
static int game_id;
static int bots_each;

static NLaddress server;
static NLsocket  socket;
static NLint     sk_group;

static packet_c pk;
#endif


#ifdef USE_HAWKNL

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

	srand(I_PureRandom());
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
		if (pk.Read(sock))
		{
			if (pk.CheckType("Bd"))
				break;
			else
				continue;
		}

		count++;

		if (count == 5)
			return false;

		I_Sleep(1000);
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

	const char *addr_name = N_GetAddrName(&loc_addr);

	// ignore loopback (127.0.0.1)
	// FIXME: check other addresses using nlGetAllLocalAddr()
	if (strncmp(addr_name, "127.", 4) != 0)
	{
		fprintf(stderr, "Local addr %s\n", N_GetAddrName(&loc_addr));

		nlSetAddrPort(&loc_addr, 26710);

		if (! nlSetRemoteAddr(loc_sock, &loc_addr))
			I_Error("Set remote address on local socket failed.\n%s", N_GetErrorStr());

		if (N_TestServer(loc_sock))
		{
			nlClose(loc_sock);
			return;
		}
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
	int port = 0;

	// choose a random socket port
	for (int tries = 0; tries < 100; tries++)
	{
		port = 11000 + (rand() & 0x1FFF);

		socket = nlOpen(port, NL_UNRELIABLE);

		if (socket != NL_INVALID)
			break;

		if ((tries % 10) == 9)
			I_Sleep(10 /* millisecs */);
	}

    if (socket == NL_INVALID)
		I_Error("Unable to create main socket:\n%s", N_GetErrorStr());

	fprintf(stderr, "Port %d\n", port);

	nlSetRemoteAddr(socket, &server);

	sk_group = nlGroupCreate();

	if (sk_group == NL_INVALID || ! nlGroupAddSocket(sk_group, socket))
		I_Error("Unable to create socket group (no memory)\n");

	pk.SetType("cs");
	pk.hd().flags = 0;
	pk.hd().data_len = sizeof(connect_proto_t);

	connect_proto_t& con = pk.cs_p();

	sprintf(con.info.name, "Flynn%02d", rand()&63);

	con.ByteSwap();

	if (! pk.Write(socket))
		I_Error("Unable to write CS packet:\n%s", N_GetErrorStr());

	I_Sleep(1000);

	if (! pk.Read(socket))
		I_Error("Failed to connect to server (no reply)\n");
	
	if (pk.CheckType("Er"))
		I_Error("Failed to connect to server:\n%s", pk.er_p().message);

	if (! pk.CheckType("Cs"))
		I_Error("Failed to connect to server: (invalid reply)\n");

	con.ByteSwap();

	client_id = pk.hd().client;
	
	fprintf(stderr, "Connected to server: client_id %d\n", client_id);
	fprintf(stderr, "Server version %x.%02x  Protocol version %x.%02x\n",
		con.server_ver >> 8, con.server_ver & 0xFF,
		con.protocol_ver >> 8, con.protocol_ver & 0xFF);
}

static void N_NewGame(void)
{
	nlSetRemoteAddr(socket, &server);

	pk.SetType("ng");
	pk.hd().flags = 0;
	pk.hd().data_len = sizeof(new_game_proto_t);

	new_game_proto_t& ng = pk.ng_p();

	sprintf(ng.info.engine_name, "EDGE%3x", EDGEVER);
	sprintf(ng.info.game_name,   "DOOM2");
	sprintf(ng.info.level_name,  "MAP12");

	ng.info.mode  = 'D';
	ng.info.skill = 4;  // H.M.P

	ng.info.min_players = 1;
	ng.info.num_bots = 3;
	ng.info.features = 0x00FF;

	ng.info.wad_checksum = 0;
	ng.info.def_checksum = 0;

	ng.ByteSwap();

	if (! pk.Write(socket))
		I_Error("Unable to write NG packet:\n%s", N_GetErrorStr());

	I_Sleep(1000);

	if (! pk.Read(socket))
		I_Error("Failed to create new game (no reply)\n");
	
	if (pk.CheckType("Er"))
		I_Error("Failed to create new game:\n%s", pk.er_p().message);

	if (! pk.CheckType("Ng"))
		I_Error("Failed to create new game (invalid reply)\n");

	ng.ByteSwap();

	game_id = ng.game;

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

	I_Sleep(1000);

	if (! pk.Read(socket))
		I_Error("Failed to vote (no reply)\n");
	
	if (pk.CheckType("Er"))
		I_Error("Failed to vote:\n%s", pk.er_p().message);

	// FIXME: will normally get "Vp" (acknowledge)

	if (! pk.CheckType("Pg"))
		I_Error("Failed to vote (invalid reply)\n");

	play_game_proto_t& pg = pk.pg_p();

	pg.ByteSwap();

	bots_each = pg.bots_each;

	// FIXME: use pg.random_seed

	// read client list !!!

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

#endif // USE_HAWKNL

//----------------------------------------------------------------------------
//  TIC HANDLING
//----------------------------------------------------------------------------

static int last_update_tic;
static int last_tryrun_tic;

void N_InitNetwork(void)
{
	DEV_ASSERT2(sizeof(ticcmd_t) == sizeof(raw_ticcmd_t));

	N_ResetTics();
}

static void GetPackets(bool do_delay)
{
	if (! netgame)
	{
		// -AJA- This can make everything a bit "jerky" :-(
		if (do_delay && ! var_hogcpu)
			I_Sleep(10 /* millis */);

		return;
	}

#ifdef USE_HAWKNL

	NLsocket socks[4];  // only one in the group

	int delay = (do_delay && ! var_hogcpu) ? 10 /* millis */ : 0;
	int num = nlPollGroup(sk_group, NL_READ_STATUS, socks, 4, delay);

	if (num < 1)
		return;

	if (! pk.Read(socks[0]))
		return;

	L_WriteDebug("GOT PACKET [%c%c] len = %d\n", pk.hd().type[0], pk.hd().type[1],
		pk.hd().data_len);

	if (! pk.CheckType("Tg"))
		return;

	// FIXME: validate packet, check size

	tic_group_proto_t& tg = pk.tg_p();

	int count = tg.last_player - tg.first_player + 1;

	tg.ByteSwap();
	tg.ByteSwapCmds((1 + bots_each) * count);

	// !!! FIXME: handle tg.count

	player_t *p = players[consoleplayer]; //!!!!

	int got_tic = tg.gametic + tg.offset;

	L_WriteDebug("-- Got TG: counter = %d  in_tic = %d\n", got_tic, p->in_tic);

	if (got_tic != p->in_tic)
	{
		L_WriteDebug("GOT TIC %d != EXPECTED %d\n", got_tic, p->in_tic);

		// !!! FIXME: handle "future" tics (save them, set bit in mask)
		//            (send retransmission request for gametic NOW).
		return;
	}

	raw_ticcmd_t *raw_cmd = tg.tic_cmds;

	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];

		if (! p || ! p->builder) continue;

		memcpy(p->in_cmds + (got_tic % (MP_SAVETICS*2)), raw_cmd, sizeof(ticcmd_t));

		raw_cmd++;

		p->in_tic++;
	}

	DEV_ASSERT2((raw_cmd - tg.tic_cmds) == (1 + bots_each));

#endif  // USE_HAWKNL
}

static void DoSendTiccmds(int tic)
{
#ifdef USE_HAWKNL
	pk.Clear();  // FIXME: TESTING ONLY

	pk.SetType("tc");
	pk.hd().flags = 0;
	pk.hd().client = client_id;
	pk.hd().data_len = sizeof(ticcmd_proto_t) - sizeof(raw_ticcmd_t);

    ticcmd_proto_t& tc = pk.tc_p();

	DEV_ASSERT2(tic >= gametic);

	tc.gametic = gametic;
	tc.offset  = tic - gametic;
	tc.count   = 1;

	raw_ticcmd_t *raw_cmd = tc.tic_cmds;

	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];

		if (! p || ! p->builder) continue;

		memcpy(raw_cmd, p->out_cmds + (tic % (MP_SAVETICS*2)), sizeof(raw_ticcmd_t));

		raw_cmd++;

		pk.hd().data_len += sizeof(raw_ticcmd_t);
	}

	DEV_ASSERT2((raw_cmd - tc.tic_cmds) == (1 + bots_each));

    tc.ByteSwap();
    tc.ByteSwapCmds((1 + bots_each) * tc.count);

	L_WriteDebug("Writing ticcmd for tic %d\n", tic);

	if (! pk.Write(socket))
		L_WriteDebug("Failed to write packet (tic %d)\n", tic);
	
	// FIXME: need an 'out_tic' for resends....

#endif  // USE_HAWKNL
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

	// build ticcmds
	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];
		if (! p) continue;

		if (p->builder)
		{
			ticcmd_t *cmd;

			if (netgame)
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

		lowtic = MIN(lowtic, p->in_tic);
	}

	return lowtic;
}

int N_TryRunTics(bool *is_fresh)
{
	*is_fresh = false;

	if (singletics)
	{
		N_BuildTiccmds();

		if (numplayers > 0)
			*is_fresh = true;

		return 1;
	}

	int nowtime = N_NetUpdate();
	int realtics = nowtime - last_tryrun_tic;
	last_tryrun_tic = nowtime;

#if 0
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
	DEV_ASSERT2(availabletics >= 0);

	// decide how many tics to run
	int counts;

	if (realtics + 1 < availabletics)
		counts = realtics + 1;
	else
		counts = MIN(realtics, availabletics);

#if 0
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

		DEV_ASSERT2(lowtic >= gametic);

		// don't stay in here forever -- give the menu a chance to work
		if (wait_tics > TICRATE/2)
		{
			L_WriteDebug("Waited %d tics IN VAIN !\n", wait_tics);
			return 3;
		}
	}

	*is_fresh = true;

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

