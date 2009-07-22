/** 
 @file  packet.c
 @brief ENet packet management functions
*/
#include <string.h>
#define ENET_BUILDING_LIB 1
#include "enet.h"

/** @defgroup Packet ENet packet functions 
    @{ 
*/

/** Creates a packet that may be sent to a peer.
    @param dataContents initial contents of the packet's data; the packet's data will remain uninitialized if dataContents is NULL.
    @param dataLength   size of the data allocated for this packet
    @param flags        flags for this packet as described for the ENetPacket structure.
    @returns the packet on success, NULL on failure
*/
ENetPacket *
enet_packet_create (const void * data, size_t dataLength, enet_uint32 flags)
{
    ENetPacket * packet = (ENetPacket *) enet_malloc (sizeof (ENetPacket));

    if (flags & ENET_PACKET_FLAG_NO_ALLOCATE)
      packet -> data = (enet_uint8 *) data;
    else
    {
       packet -> data = (enet_uint8 *) enet_malloc (dataLength);
       if (packet -> data == NULL)
       {
          enet_free (packet);
          return NULL;
       }

       if (data != NULL)
         memcpy (packet -> data, data, dataLength);
    }

    packet -> referenceCount = 0;
    packet -> flags = flags;
    packet -> dataLength = dataLength;
    packet -> freeCallback = NULL;

    return packet;
}

/** Destroys the packet and deallocates its data.
    @param packet packet to be destroyed
*/
void
enet_packet_destroy (ENetPacket * packet)
{
    if (packet -> freeCallback != NULL)
      (* packet -> freeCallback) (packet);
    if (! (packet -> flags & ENET_PACKET_FLAG_NO_ALLOCATE))
      enet_free (packet -> data);
    enet_free (packet);
}

/** Attempts to resize the data in the packet to length specified in the 
    dataLength parameter 
    @param packet packet to resize
    @param dataLength new size for the packet data
    @returns 0 on success, < 0 on failure
*/
int
enet_packet_resize (ENetPacket * packet, size_t dataLength)
{
    enet_uint8 * newData;
   
    if (dataLength <= packet -> dataLength || (packet -> flags & ENET_PACKET_FLAG_NO_ALLOCATE))
    {
       packet -> dataLength = dataLength;

       return 0;
    }

    newData = (enet_uint8 *) enet_malloc (dataLength);
    if (newData == NULL)
      return -1;

    memcpy (newData, packet -> data, packet -> dataLength);
    enet_free (packet -> data);
    
    packet -> data = newData;
    packet -> dataLength = dataLength;

    return 0;
}

static int initializedCRC32 = 0;
static enet_uint32 crcTable [256];

static void initialize_crc32 ()
{
    int byte;

    for (byte = 0; byte < 256; ++ byte)
    {
        enet_uint32 crc = byte << 24;
        int offset;

        for(offset = 0; offset < 8; ++ offset)
        {
            if (crc & 0x80000000)
                crc = (crc << 1) ^ 0x04c11db7;
            else
                crc <<= 1;
        }

        crcTable [byte] = crc;
    }

    initializedCRC32 = 1;
}
    
enet_uint32
enet_crc32 (const ENetBuffer * buffers, size_t bufferCount)
{
    enet_uint32 crc = 0xFFFFFFFF;
    
    if (! initializedCRC32) initialize_crc32 ();

    while (bufferCount -- > 0)
    {
        const enet_uint8 * data = (const enet_uint8 *) buffers -> data,
                         * dataEnd = & data [buffers -> dataLength];

        while (data < dataEnd)
        {
            crc = ((crc << 8) | * data ++) ^ crcTable [crc >> 24];        
        }

        ++ buffers;
    }

    return ENET_HOST_TO_NET_32 (~ crc);
}

/** @} */
