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

#ifndef __NETWORK_H__
#define __NETWORK_H__

#define MPS_DEF_PORT  26710

extern NLsocket main_socket;
extern NLmutex global_lock;

extern volatile bool net_quit;
extern volatile bool net_failure;

extern int cur_net_time;

void NetInit(void);
void *NetRun(void *data);

const char *GetNLErrorStr(void);
const char *GetAddrName(const NLaddress *addr);
const char *GetNetFailureMsg(void);

int GetNetTime(void);

#endif /* __NETWORK_H__ */
