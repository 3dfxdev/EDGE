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

#ifndef __N_NETWORK_H__
#define __N_NETWORK_H__

void N_InitiateGame(void);

void E_CheckNetGame(void);

// Create any new ticcmds and broadcast to other players.
// returns value of I_GetTime().
int E_NetUpdate(bool do_delay = false);

// Broadcasts special packets to other players
//  to notify of game exit
void E_QuitNetGame(void);

// returns number of ticks to run.
int E_TryRunTics(void);

#endif /* __N_NETWORK_H__ */
