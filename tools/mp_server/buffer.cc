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

#include "includes.h"

#include "buffer.h"
#include "lib_list.h"
#include "network.h"
#include "packet.h"
#include "protocol.h"
#include "ui_log.h"

volatile int buffered_packets = 0;
volatile int buffered_bytes = 0;

class buffer_packet_c : public node_c
{
friend class buffer_queue_c;

private:
	static const int OLD_TIME = 30;  /* seconds */

	NLsocket sock;
	NLaddress addr;

	int ntime;

	char *data;
	int len;

public:
	buffer_packet_c(NLsocket _sock, NLaddress *_addr, int _ntime,
		const char *_data, int _len) : node_c(),
			sock(_sock), ntime(_ntime), len(_len)
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

	buffer_packet_c *Next() const { return (buffer_packet_c *) NodeNext(); }
	buffer_packet_c *Prev() const { return (buffer_packet_c *) NodePrev(); }

	bool TooOld(int cur_time) const
	{
		return cur_time >= (ntime + OLD_TIME * 100);
	}

	bool Write() const
	{
		int wrote = nlWrite(sock, data, len); 

		if (wrote == 0)
		{
			return false;
		}
		else if (wrote < 0)
		{
			LogPrintf(2, "BufferWrite failed: %s\n", GetNLErrorStr());
			return true;
		}

		total_out_packets++;
		total_out_bytes += len;

		if (wrote != len)
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

	list_c packets;

	int size;

public:
	buffer_queue_c() : packets(), size(0) { }
	~buffer_queue_c() { }

	buffer_packet_c *begin() const { return (buffer_packet_c *) packets.begin(); }
	buffer_packet_c *end()   const { return (buffer_packet_c *) packets.end(); }

	void Remove(buffer_packet_c *bp)
	{
		buffered_packets--;
		buffered_bytes -= bp->len;

		packets.remove(bp);
		delete bp;

		size--;
		SYS_ASSERT(size > 0);
	}

	void Add(NLsocket sock, NLaddress *addr, const char *data, int len)
	{
		if (size == QUEUE_MAX)
			Remove(end());

		buffered_packets++;
		buffered_bytes += len;

		buffer_packet_c *bp = new buffer_packet_c(sock, addr, GetNetTime(), data, len);

		packets.push_front(bp);

		size++;
		SYS_ASSERT(size <= QUEUE_MAX);
	}

	void RetryWrites(int cur_time)
	{
		buffer_packet_c *next;

		for (buffer_packet_c *bp = begin(); bp; bp = next)
		{
			next = bp->Next();

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
void BufferRetryWrites()
{
	buffer_Q.RetryWrites(cur_net_time);
}
