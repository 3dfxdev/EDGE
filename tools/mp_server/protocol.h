//------------------------------------------------------------------------
//  Protocol structures
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

#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

//
// common packet header
//
typedef struct proto_header_s
{
	char type[2];

	s16_t flags;
	s16_t request_id;  // use to match requests with replies, also = TicCounter
	s16_t data_len;

	s16_t source;  // client IDs
	s16_t dest;    //
	s16_t game;
	s16_t reserved;  // (future expansion)

	enum  // special destinations
	{
		D_ALL_BROWSING = -1,
		D_ALL_NOT_PLAYING = -2,
		D_OTHER_QUEUERS = -3,
		D_OTHER_PLAYERS = -4,
		D_ABSOLUTELY_EVERYONE = -9,  // try to avoid this!
	};

	void ByteSwap();
}
proto_header_t;


//
// ERROR ("Er")
//
typedef struct error_proto_s
{
	s16_t type;
	s16_t param[2];
	s16_t reserved[2];

	static const int ERR_STR_MAX = 200;

	char message[1];  // as big as needed (upto ERR_STR_MAX)

	enum  // error types
	{
		ERR_Unknown = 1,  // general purpose value

		// connecting/disconnecting
		ERR_AlreadyConnected = 10,

		ERR_NotInQueue = 20,

		ERR_NotPlaying = 30,
	};

	void ByteSwap();
}
error_proto_t;


/* BROADCAST-DISCOVER ("bd") has no data */


//
// Client information
//
typedef struct client_info_s
{
	static const int NAME_LEN = 16;

	char name[NAME_LEN];

	s16_t reserved1[3];

	/* --- query output --- */

	byte state;
	byte reserved2;

	enum
	{
		CS_NotExist = 'N',
		CS_Browsing = 'B',
		CS_Queueing = 'Q',
		CS_Voted    = 'V',  // implies Queueing
		CS_Playing  = 'P'
	};

	s16_t game_id;

	void ByteSwap();
}
client_info_t;

//
// CONNECT ("cs")
//
typedef struct connect_proto_s
{
	client_info_t info;

	void ByteSwap();
}
connect_proto_t;


/* LEAVE-SERVER ("ls") has no data */

/* KEEP-ALIVE ("ka") has no data */


//
// QUERY-CLIENT ("qc")
//
typedef struct query_client_proto_s 
{
	s16_t first_id;
	byte  count;

	byte  reserved1;
	s16_t reserved2[2];

	static const int CLIENT_FIT = 16;

	client_info_t info[1];  // upto CLIENT_FIT structs

	void ByteSwap(bool do_info);
}
query_client_proto_t;


//
// Game information
//
typedef struct game_info_s
{
	static const int GAME_STR_MAX  = 16;
	static const int LEVEL_STR_MAX = 10;

	char game_name[GAME_STR_MAX];    // e.g. DOOM2
	char level_name[LEVEL_STR_MAX];  // e.g. MAP01

	char mode;   // 'C' for coop, 'D' for deathmatch
	char skill;  // '1' to '5'

	s16_t min_players;
	s16_t engine_ver;  // hexadecimal, e.g. 0x129
	s16_t reserved1[2]; 

	/* --- query output --- */

	byte state;
	byte reserved2;

	enum
	{
		GS_NotExist = 'N',
		GS_Queued   = 'Q',
		GS_Playing  = 'P'
	};

	s16_t num_players;
	s16_t num_votes;

	void ByteSwap();
}
game_info_t;

//
// NEW-GAME ("ng")
//
typedef struct new_game_proto_s
{
	game_info_t info;

	void ByteSwap();
}
new_game_proto_t;


//
// QUERY-GAME ("qg")
//
typedef struct query_game_proto_s
{
	s16_t first_id;
	byte  count;

	byte  reserved1;
	s16_t reserved2[2];

	static const int GAME_FIT = 10;

	game_info_t info[1];  // upto GAME_FIT structures

	void ByteSwap(bool do_info);
}
query_game_proto_t;


/* JOIN-QUEUE ("jq") has no data */

/* LEAVE-GAME ("lg") has no data */

/* VOTE-to-PLAY ("vp") has no data */


//
// PLAY-GAME ("Pg")
//
typedef struct play_game_proto_s
{
	s16_t total_players;
	s16_t first_player;
	byte  count;

	byte  reserved1;
	s16_t reserved2[2];

	static const int PLAYER_MAX = 24;

	s16_t client_id[1];  // client IDs of each player (upto PLAYER_MAX)

	void ByteSwap();
}
play_game_proto_t;


//
// raw TICCMD (contents are engine-specific)
//
typedef struct raw_ticcmd_s
{
	static const int SIZE = 16;  // actual size can be smaller

	char data[SIZE];
}
raw_ticcmd_t;


//
// TICCMD ("tc")
//
typedef struct ticcmd_proto_s
{
	/// s16_t tic_counter;
	s16_t reserved[3];

	raw_ticcmd_t tic_cmd;

	void ByteSwap();
}
ticcmd_proto_t;


//
// TIC-GROUP ("Tg")
//
typedef struct tic_group_proto_s
{
	/// s16_t tic_counter;
	s16_t first_player;
	byte  count;

	byte  reserved1;
	s16_t reserved2[3];

	static const int TICCMD_FIT = 24;

	raw_ticcmd_t tic_cmds[1];  // as big as needed (upto TICCMD_FIT)

	void ByteSwap();
}
tic_group_proto_t;


/* MESSAGE ("ms") : data is engine-specific (we don't care!) */


#endif /* __PROTOCOL_H__ */
