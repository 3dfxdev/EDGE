//------------------------------------------------------------------------
//  Buffering for packets
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
#include "network.h"
#include "packet.h"
#include "protocol.h"
#include "ui_log.h"

volatile int buffered_packets = 0;
volatile int buffered_bytes = 0;

class buffer_packet_c
{
friend class buffer_queue_c;

private:
	static const int OLD_TIME = 30;  /* seconds */

	buffer_packet_c *next;  // link in list
	buffer_packet_c *prev;

	NLsocket sock;
	NLaddress addr;

	int ntime;

	char *data;
	int len;

public:
	buffer_packet_c(NLsocket _sock, NLaddress *_addr, int _ntime,
		const char *_data, int _len) :
			next(NULL), prev(NULL), sock(_sock), ntime(_ntime), len(_len)
	{
		memcpy(&addr, _addr, sizeof(NLaddress));

		data = new char[len];

		memcpy(data, _data, len);
	}

#if 0
	buffer_packet_c(const buffer_packet_c& other) :
		sock(other.sock), ntime(other.ntime), len(other.len)
	{
		memcpy(&addr, &other.addr, sizeof(NLaddress));

		data = new char[len];

		memcpy(data, other.data, len);
	}
#endif

	~buffer_packet_c()
	{
		delete[] data;
		data = NULL;
	}

	bool TooOld(int cur_time) const
	{
		return cur_time >= (ntime + OLD_TIME * 100);
	}

	bool Write() const
	{
		int wrote = nlWrite(sock, data, len); 

		if (wrote == 0)
			return false;

		else if (wrote < 0)
			LogPrintf(2, "BufferWrite failed: %s\n", GetNLErrorStr());

		else if (wrote != len)
			LogPrintf(1, "BufferWrite: wrote less data (%d < %d)\n", wrote, len);

		return true;
	}

	// returns true if should remove packet from buffer
	// (either write was successful, or packet too old).
	bool RetryWrite(int cur_time)
	{
		if (TooOld(cur_time))
		{
			LogPrintf(1, "Dropping old packet: type [%c%c] addr %s\n",
				data[0], data[1], GetAddrName(&addr));
			return true;
		}

		return Write();
	}
};

//------------------------------------------------------------------------

class buffer_queue_c
{
private:
	static const int QUEUE_MAX = 256;

	buffer_packet_c *head;
	buffer_packet_c *tail;

	int size;

public:
	buffer_queue_c() : head(NULL), tail(NULL), size(0) { }
	~buffer_queue_c() { }

	void Remove(buffer_packet_c *bp)
	{
		buffered_bytes -= bp->len;
		buffered_packets--;

		if (bp->next)
			bp->next->prev = bp->prev;
		else
			tail = bp->prev;

		if (bp->prev)
			bp->prev->next = bp->next;
		else
			head = bp->next;

		delete bp;

		size--;
		SYS_ASSERT(size > 0);
	}

	void Add(NLsocket sock, NLaddress *addr, const char *data, int len)
	{
		if (size == QUEUE_MAX)
			Remove(tail);

		buffered_bytes += len;
		buffered_packets++;

		buffer_packet_c *bp = new buffer_packet_c(sock, addr, GetNetTime(), data, len);

		bp->next = head;
		bp->prev = NULL;

		if (head)
			head->prev = bp;
		else
			tail = bp;

		head = bp;

		size++;
		SYS_ASSERT(size <= QUEUE_MAX);
	}

	void RetryWrites(int cur_time)
	{
		buffer_packet_c *next;

		for (buffer_packet_c *bp = head; bp; bp = next)
		{
			next = bp->next;

			if (bp->RetryWrite(cur_time))
				Remove(bp);
		}
	}
};

//------------------------------------------------------------------------

static buffer_queue_c buffer_Q;

//
// BufferPacket
//
void BufferPacket(NLsocket sock, const char *data, int len)
{
	SYS_ASSERT((unsigned)len >= sizeof(header_proto_t));

	NLaddress remote_addr;
	nlGetRemoteAddr(sock, &remote_addr);

	buffer_Q.Add(sock, &remote_addr, data, len);
}

//
// BufferRetryWrites
//
void BufferRetryWrites(int cur_time)
{
	buffer_Q.RetryWrites(cur_time);
}
