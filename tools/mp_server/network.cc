//------------------------------------------------------------------------
//  Networking bits
//------------------------------------------------------------------------
//
//  Edge MultiPlayer Server (C) 2004  Andrew Apted
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
//------------------------------------------------------------------------

#include "includes.h"

#include "buffer.h"
#include "client.h"
#include "game.h"
#include "network.h"
#include "packet.h"
#include "protocol.h"
#include "ui_log.h"


NLsocket main_socket;
static NLint main_group;

NLmutex  global_lock;  // lock the client/game structures

volatile bool net_quit = false;
volatile bool net_failure = false;

static char net_fail_message[512];

static NLtime base_time;

static int last_update_time = -1;


const char *GetAddrName(const NLaddress *addr)
{
	static char name_buf[NL_MAX_STRING_LENGTH];

	return nlAddrToString(addr, name_buf);
}

const char *GetNetFailureMsg(void)
{
	return net_fail_message;
}

const char *GetNLErrorStr(void)
{
	int err = nlGetError();

	if (err == NL_NO_ERROR)
		return "(Unknown Problem)";

	static char err_buf[512];

	if (err == NL_SYSTEM_ERROR)
	{
		err = nlGetSystemError();
		sprintf(err_buf, "%s", nlGetSystemErrorStr(err));
	}
	else
	{
		sprintf(err_buf, "NL: %s", nlGetErrorStr(err));
	}

	// remove newlines and other rubbish
	for (int i = 0; err_buf[i] && (unsigned)i < sizeof(err_buf); i++)
	{
		if (! isprint(err_buf[i]))
			err_buf[i] = ' ';
	}

	return err_buf;
}

static void InitNetTime(void)
{
	nlTime(&base_time);
}

//
// GetNetTime
//
// Returns a time value which increases by 1 for every 10 milliseconds
// (i.e. 100HZ timebase).  Values start at zero.  For 32-bit integers,
// this will overflow in about 248 days.
//
int GetNetTime(void)
{
	NLtime current;

	nlTime(&current);

	NLlong sec_diff  = current.seconds  - base_time.seconds;
	NLlong usec_diff = current.useconds - base_time.useconds;

	return int(sec_diff * 100 + usec_diff / 10000);
}

//------------------------------------------------------------------------

static void HandleTimeouts()
{
	// LOCK STRUCTURES
	nlMutexLock(&global_lock);

	int cur_time = GetNetTime();

	if (cur_time > last_update_time)
	{
		BufferRetryWrites(cur_time);

		// FIXME: remove gone clients

		// FIXME: remove zombie games

		last_update_time = cur_time;
	}

	// UNLOCK STRUCTURES
	nlMutexUnlock(&global_lock);
}

//
// NetInit
//
// Calls fl_alert if something goes wrong.
//
void NetInit(void)
{
	nlMutexInit(&global_lock);

	InitNetTime();

    DebugPrintf("NL_VERSION: %s\n", nlGetString(NL_VERSION));
  	DebugPrintf("NL_NETWORK_TYPES: %s\n", nlGetString(NL_NETWORK_TYPES));
 
    if (! nlSelectNetwork(NL_IP))
	{
		fl_alert("Hawk network library: unable to select IP networking:\n%s",
				 GetNLErrorStr());
        exit(5); //!!!!
	}

	DebugPrintf("NL_SOCKET_TYPES: %s\n\n", nlGetString(NL_CONNECTION_TYPES));

#ifdef LINUX
	if (1) /// !!!!! FIXME: BIG HACK
	{
		NLaddress addr;
		nlStringToAddr("192.168.0.197", &addr);
		nlSetLocalAddr(&addr);
	}
#endif

	main_socket = nlOpen(MPS_DEF_PORT, NL_UNRELIABLE);

	if (main_socket == NL_INVALID)
	{
		fl_alert("Unable to create UDP socket on port %d:\n%s", MPS_DEF_PORT,
				 GetNLErrorStr());
		exit(5); //!!!!
	}

	main_group = nlGroupCreate();

	if (main_group == NL_INVALID || ! nlGroupAddSocket(main_group, main_socket))
	{
		fl_alert("Out of memory (cannot create socket group)");
		exit(5);
	}

	NLaddress addr;

    nlGetLocalAddr(main_socket, &addr);
    LogPrintf(0, "Server address: %s\n\n", GetAddrName(&addr));
}

//
// NetRun
//
// Listens to net_quit variable if main program wants to quit.
// Sets net_failure variable if an exception is caught.
//
void *NetRun(void *data)
{
	net_fail_message[0] = 0;

	try
	{
		packet_c pk;

		while (! net_quit)
		{
			HandleTimeouts();

			pk.Clear();  // paranoia

			NLsocket socks[4];  // only one in the group

			int num = nlPollGroup(main_group, NL_READ_STATUS, socks, 4, 10 /* millis */);

			if (num < 1)
				continue;

			if (! pk.Read(socks[0]))
				continue;

			NLaddress remote_addr;
			nlGetRemoteAddr(socks[0], &remote_addr);

			DebugPrintf("GOT PACKET: type [%c%c] data_len %d addr %s\n",
				pk.hd().type[0], pk.hd().type[1], pk.hd().data_len,
				GetAddrName(&remote_addr));

			/* send replies back to originator */
			nlSetRemoteAddr(socks[0], &remote_addr);

			if (pk.CheckType("cs"))   // connect to server
			{
				PK_connect_to_server(&pk, &remote_addr);
				continue;
			}
			else if (pk.CheckType("bd"))   // broadcast discovery
			{
				PK_broadcast_discovery(&pk, &remote_addr);
				continue;
			}

			// must validate the client field (the only exceptions are the
			// connect-to-server and the broadcast-discovery packets).

			if (! VerifyClient(pk.hd().source, &remote_addr))
			{
				LogPrintf(2, "Client %d verify failed: packet [%c%c]\n",
					pk.hd().source, pk.hd().type[0], pk.hd().type[1]);
				continue;
			}

			if (pk.CheckType("ls"))   // leave server
			{
				PK_leave_server(&pk);
			}
			else if (pk.CheckType("ka"))  // keep-alive
			{
				PK_keep_alive(&pk);
			}
			else if (pk.CheckType("qc"))  // query client
			{
				PK_query_client(&pk);
			}
			else if (pk.CheckType("qg"))  // query game
			{
				PK_query_game(&pk);
			}
			else if (pk.CheckType("ng"))  // new game
			{
				PK_new_game(&pk);
			}
			else if (pk.CheckType("jq"))  // join queue
			{
				PK_join_queue(&pk);
			}
			else if (pk.CheckType("lg"))  // leave game
			{
				PK_leave_game(&pk);
			}
			else if (pk.CheckType("vp"))  // play game, dammit!
			{
				PK_vote_to_play(&pk);
			}
			else if (pk.CheckType("st"))  // send ticcmd
			{
				PK_ticcmd(&pk);
			}
			else if (pk.CheckType("sm"))  // send message
			{
				PK_message(&pk);
			}
			else
			{
				LogPrintf(1, "Unknown packet: [%c%c] addr %s\n",
					pk.hd().type[0], pk.hd().type[1],
					GetAddrName(&remote_addr));
			}
		}
	}
	catch (assert_fail_c err)
	{
		net_failure = true;
		strcpy(net_fail_message, err.GetMessage());
		return net_fail_message;
	}
	catch (...)
	{
		net_failure = true;
		strcpy(net_fail_message, "Unknown problem in networking code.");
		return net_fail_message;
	}

	return NULL;
}

