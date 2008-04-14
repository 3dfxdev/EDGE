
/* WOLF UTIL */

#include "i_defs.h"

#include "epi/endianess.h"

#include "wf_local.h"

#define NEARTAG	 0xA7
#define FARTAG	 0xA8

// Note: length is of the expanded data in BYTES.
void WF_Carmack_Expand(const byte *source, byte *dest, int length)
{
	SYS_ASSERT((length & 1) == 0);

	length /= 2;

	const byte *inptr = source;
	byte *outptr = dest;

	while (length > 0)
	{
		// !!!! if (inptr > src_end) I_Error("CARMACK_EXPAND: OUT OF DATA\n");

		byte count  = *inptr++;
		byte c_high = *inptr++;

		if (c_high == NEARTAG)
		{
			if (! count)
			{
				*outptr++ = *inptr++;
				*outptr++ = NEARTAG;

				length--; continue;
			}

			unsigned int offset = *inptr++;

			byte *copyptr = outptr - offset * 2;
			SYS_ASSERT(copyptr >= dest);

			length -= (int)count;
			if (length < 0)
				I_Error("CARMACK_EXPAND: OVERFLOW\n");

			for (; count > 0; count--)
			{
				*outptr++ = *copyptr++;
				*outptr++ = *copyptr++;
			}
		}
		else if (c_high == FARTAG)
		{
			if (!count)
			{
				*outptr++ = *inptr++;
				*outptr++ = FARTAG;

				length--; continue;
			}

			unsigned int offset = (inptr[1] << 8) | inptr[0];
			inptr += 2;

			byte *copyptr = dest + offset * 2;
			SYS_ASSERT(copyptr < outptr);

			length -= (int)count;
			if (length < 0)
				I_Error("CARMACK_EXPAND: OVERFLOW\n");

			for (; count > 0; count--)
			{
				*outptr++ = *copyptr++;
				*outptr++ = *copyptr++;
			}
		}
		else  // normal data word
		{
			*outptr++ = count;
			*outptr++ = c_high;

			length--;
		}
	}
}

// AJA: also swaps endianness
void WF_RLEW_Expand(const u16_t *source, u16_t *dest, int length, u16_t rlew_tag)
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

void WF_Huff_Expand(const byte *source, int src_len, byte *dest, int dest_len,
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
}

