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


class buffer_packet_c
{
private:
	static const int OLD_TIME = 30;  /* seconds */

public:  // a hack

	NLsocket sock;
	NLaddress addr;

	int ntime;

	char *data;
	int len;

public:
	buffer_packet_c(NLsocket _sock, NLaddress *_addr, int _ntime,
		const char *_data, int _len) :
			sock(_sock), ntime(_ntime), len(_len)
	{
		memcpy(&addr, _addr, sizeof(NLaddress));

		data = new char[len];

		memcpy(data, _data, len);
	}

	buffer_packet_c(const buffer_packet_c& other) :
		sock(other.sock), ntime(other.ntime), len(other.len)
	{
		memcpy(&addr, &other.addr, sizeof(NLaddress));

		data = new char[len];

		memcpy(data, other.data, len);
	}

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

	/* ---- predicates ---- */

	class retry_write
	{
		// returns true if should remove packet from buffer
		// (either write was successful, or packet too old).

	private:
		int cur_time;

	public:
		retry_write(int _time) : cur_time(_time) { }

		inline bool operator() (const buffer_packet_c& bp)
		{
			if (bp.TooOld(cur_time))
			{
				LogPrintf(1, "Removing old packet: type [%c%c] addr %s\n",
					bp.data[0], bp.data[1], GetAddrName(&bp.addr));
				return true;
			}

			return bp.Write();
		}
	};
};

class buffer_queue_c
{
private:
	static const unsigned int QUEUE_MAX = 256;

	typedef std::list<buffer_packet_c> pkt_list;

	pkt_list queue;

public:
	buffer_queue_c() : queue() { }
	~buffer_queue_c() { }

	void Add(NLsocket sock, NLaddress *addr, const char *data, int len)
	{
		if (queue.size() == QUEUE_MAX)
			queue.erase(queue.begin());

		queue.push_back(buffer_packet_c(sock, addr, GetNetTime(), data, len));

		SYS_ASSERT(queue.size() <= QUEUE_MAX);
	}

	void RetryWrites(int cur_time)
	{
		std::remove_if(queue.begin(), queue.end(),
			buffer_packet_c::retry_write(cur_time));
	}
};

buffer_queue_c buffer_Q;

//------------------------------------------------------------------------

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
