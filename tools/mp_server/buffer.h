//------------------------------------------------------------------------
//  Buffering for packets
//------------------------------------------------------------------------
//
//  Edge MultiPlayer Server (C) 2005  Andrew Apted
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

#ifndef __BUFFER_H__
#define __BUFFER_H__

extern volatile int buffered_packets;
extern volatile int buffered_bytes;

void BufferPacket(NLsocket sock, const char *data, int len);

void BufferRetryWrites(void);
void BufferClearAll(void);

#endif /* __BUFFER_H__ */
