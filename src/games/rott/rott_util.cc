//----------------------------------------------------------------------------
//  Rise of the Triad Utility Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2016 Isotope SoftWorks
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

//#include "../../system/i_defs.h"

#include "../../../epi/endianess.h"

#include "rott_local.h"

//#define NEARTAG	 0xA7
//#define FARTAG	 0xA8

int elevatorstart;

/// CA- 6.2.2016: CA_RLEW_EXPAND UTILITY FUNCTION (from rt_ted.c)
//  AJA - also swaps endianess
/// length is EXPANDED bytes
void CA_RLEW_Expand(const u16_t *source, u16_t *dest, int length, u16_t rlew_tag)
{
	L_WriteDebug("RLEW_EXPAND: length %d\n", length);

	u16_t *end = dest + (length/2);

for (int k = 0; k < 128; k++)
{
L_WriteDebug("%04x%c", source[k], (k%8 == 7) ? '\n' : ' ');
}

	while (dest < end)
	{
		//!!!! if (source >= src_end) I_Error("RLEW_EXPAND: OUT OF DATA\n");

		u16_t value = EPI_LE_U16(*source); source++;

		if (value != rlew_tag)  // uncompressed
		{
			*dest++ = value;
		}
		else // compressed string
		{
			u16_t count = EPI_LE_U16(*source); source++;
			u16_t val2  = EPI_LE_U16(*source); source++;

			if (dest + count > end)
				I_Error("RLEW_EXPAND: OVERFLOW\n");

			for (; count > 0; count--)
				*dest++ = val2;
		}
	}
}


int GetWallIndex (int texture)
{
    int wallstart;
    int exitstart;

    wallstart = W_GetNumForName ("WALLSTRT");
    exitstart = W_GetNumForName ("EXITSTRT");
    elevatorstart = W_GetNumForName ("ELEVSTRT");

    if (texture & 0x1000) {
	texture &= ~0x1000;
	if (texture == 0)
	    return 41;
	else if (texture == 1)
	    return 90;
	else if (texture == 2)
	    return 91;
	else if (texture == 3)
	    return 42;
	else if (texture == 4)
	    return 92;
	else if (texture == 5)
	    return 93;
	else if (texture == 6)
	    return 94;
	else if (texture == 7)
	    return 95;
	else if (texture == 8)
	    return 96;
	else if (texture == 9)
	    return 97;
	else if (texture == 10)
	    return 98;
	else if (texture == 11)
	    return 99;
	else if (texture == 12)
	    return 100;
	else if (texture == 13)
	    return 101;
	else if (texture == 14)
	    return 102;
	else if (texture == 15)
	    return 103;
	else if (texture == 16)
	    return 104;
    } else if (texture > elevatorstart)
	return (texture - elevatorstart + 68);
//      else if (texture > specialstart)
//      return (texture - specialstart + 41);
    else if (texture > exitstart)
	return (texture - exitstart + 43);
    else {
	if (texture > wallstart + 63)
	    return (texture - (wallstart + 63) + 76);
	else if (texture > wallstart + 40)
	    return (texture - (wallstart + 40) + 45);
	else
	    return (texture - wallstart);
    }
    return 0x8000;
}

/* void WF_Huff_Expand(const byte *source, int src_len, byte *dest, int dest_len,
				 const huffnode_t *table)
{
	SYS_ASSERT(dest_len > 0);

	static const int head_node = 254;

	const byte *src_end = source + src_len;
	byte *dest_end = dest + dest_len;

	int node = head_node;

	byte datum = *source++;
	byte mask  = 1;

	for (;;)
	{
		SYS_ASSERT(0 <= node && node < 256);  // FIXME -- uncomment when working

		if (datum & mask)
			node = table[node].bit1;
		else
			node = table[node].bit0;

		if (node >= 256)
		{
			*dest++ = (byte)node;
			node = head_node;
		}

		if (dest >= dest_end)
			break;

		if (mask == 128)
		{
			if (source >= src_end)
				I_Error("Huff_expand: OUT!\n"); // throw "Huff_expand: OUT OF DATA!";

			datum = *source++;
			mask  = 1;
		}
		else
			mask <<= 1;
	}
} */


