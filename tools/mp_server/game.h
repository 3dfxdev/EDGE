//------------------------------------------------------------------------
//  Games
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

#ifndef __GAME_H__
#define __GAME_H__

#include "protocol.h"
#include "client.h"

class packet_c;

class game_c
{
public:
	game_c(const game_info_t *info);
	~game_c();

private:
	enum
	{
		ST_Queueing,
		ST_Playing,
		ST_Zombie  // no players or queuers
	};

	int state;

public: //FIXME
	std::string engine_name;  // EDGE129
	std::string game_name;    // DOOM2 (etc)
	std::string level_name;   // MAP01 (etc)

	char mode;
	byte skill;

	int min_players;
	int num_bots;

	std::vector<int> queuers;
	std::vector<int> players;  // in the game

	int num_players;
	int bots_each;
	int num_votes;

	// ---- tic handlig ----

	int tic_counter;

	std::bitset<MP_PLAYER_MAX> got_cmds;
	raw_ticcmd_t *tic_cmds;

	int zombie_millies;  // countdown before deletion

public:
	bool InQueue(int client_id) const;
	bool InGame (int client_id) const;

	int AddToQueue(int client_id);  // returns player_id
	int AddToGame (int client_id);  //

	void RemoveFromQueue(int client_id);
	void RemoveFromGame (int client_id);

	void CalcBots();
	void FillGameInfo(game_info_t *info) const;

	void InitGame();
	void BumpTic();
};

extern std::vector<game_c *> games;

void PK_new_game(packet_c *pk);
void PK_join_queue(packet_c *pk);
void PK_leave_game(packet_c *pk);
void PK_vote_to_play(packet_c *pk);

void PK_query_game(packet_c *pk);
void PK_ticcmd(packet_c *pk);

#endif /* __GAME_H__ */
