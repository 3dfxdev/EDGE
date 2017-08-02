/*
 *
 * Decode PCM/ADPCM - Copyright 2016 by Chilly Willy <ChillyWillyGuru@gmail.com>
 *
 * This code falls under the BSD license.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../fmv.h"
#include "../avi.h"

#include "../src/i_system.h"


extern stream_format_auds_t astream_format;

extern unsigned char *read_buffer;
extern short *pcm_buffer;

extern int pcm_samples, pcm_rix, pcm_wix, pcm_cnt;
extern int lstep, lpred;
extern int rstep, rpred;

static const int ima_index_table[16] = {
  -1, -1, -1, -1, 2, 4, 6, 8,
  -1, -1, -1, -1, 2, 4, 6, 8
};

static const int ima_step_table[89] = {
	7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
	19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
	50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
	130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
	337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
	876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
	2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
	5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
	15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

static const int yamaha_indexscale[16] = {
    230, 230, 230, 230, 307, 409, 512, 614,
    230, 230, 230, 230, 307, 409, 512, 614
};

static const int yamaha_difflookup[16] = {
    1, 3, 5, 7, 9, 11, 13, 15,
    -1, -3, -5, -7, -9, -11, -13, -15
};


int decode_PCM(FILE *in, int chunk_size)
{
	in->Read(read_buffer, chunk_size);

	if (astream_format.block_size_of_data == 16)
	{
		short *buf = (short *)read_buffer;
		// 16-bit samples - just copy
		for (int i=0; i<(chunk_size/2); i++)
		{
			pcm_buffer[pcm_wix++] = buf[i];
			pcm_cnt++;
			if (pcm_wix >= pcm_samples)
				pcm_wix = 0; // circular buffer
		}
	}
	else
	{
		// 8-bit samples - convert to 16-bit
		for (int i=0; i<chunk_size; i++)
		{
			pcm_buffer[pcm_wix++] = (short)((read_buffer[i] ^ 0x80) << 8);
			pcm_cnt++;
			if (pcm_wix >= pcm_samples)
				pcm_wix = 0; // circular buffer
		}
	}

	return PCM_SUCCESS;
}

int decode_ADPCM_IMA(FILE *in, int chunk_size)
{
	lpred = read_word(in);
	lstep = read_word(in);
	chunk_size -= 4;
	if (astream_format.channels == 2)
	{
		rpred = read_word(in);
		rstep = read_word(in);
		chunk_size -= 4;
	}
	in->Read(read_buffer, chunk_size);

	for (int i=0; i<chunk_size; )
	{
		int l[8];
		int r[8];

		// four bytes for left/mono
		for (int j=0; j<4; j++)
		{
			int v = buf[i++];
			// decode two adpcm values
			int step = ima_step_table[lstep];
			lstep += ima_index_table[v & 0x0F];
			lstep = lstep < 0 ? 0 : lstep > 88 ? 88 : lstep;
			int diff = ((2 * (v & 7) + 1) * step) >> 3;
			lpred += v & 8 ? -diff : diff;
			l[j*2] = lpred = lpred < -32768 ? -32768 : lpred > 32767 ? 32767 : lpred;

			step = ima_step_table[lstep];
			lstep += ima_index_table[(v >> 4) & 0x0F];
			lstep = lstep < 0 ? 0 : lstep > 88 ? 88 : lstep;
			diff = ((2 * ((v >> 4) & 7) + 1) * step) >> 3;
			lpred += v & 0x80 ? -diff : diff;
			l[j*2+1] = lpred = lpred < -32768 ? -32768 : lpred > 32767 ? 32767 : lpred;
		}
		if (astream_format.channels == 2)
		{
			// four bytes for right
			for (int j=0; j<4; j++)
			{
				int v = buf[i++];
				// decode two adpcm values
				int step = ima_step_table[rstep];
				rstep += ima_index_table[v & 0x0F];
				rstep = rstep < 0 ? 0 : rstep > 88 ? 88 : rstep;
				int diff = ((2 * (v & 7) + 1) * step) >> 3;
				rpred += v & 8 ? -diff : diff;
				r[j*2] = rpred = rpred < -32768 ? -32768 : rpred > 32767 ? 32767 : rpred;

				step = ima_step_table[rstep];
				rstep += ima_index_table[(v >> 4) & 0x0F];
				rstep = rstep < 0 ? 0 : rstep > 88 ? 88 : rstep;
				diff = ((2 * ((v >> 4) & 7) + 1) * step) >> 3;
				rpred += v & 0x80 ? -diff : diff;
				r[j*2+1] = rpred = rpred < -32768 ? -32768 : rpred > 32767 ? 32767 : rpred;
			}
		}

		for (int j=0; j<8; j++)
		{
			pcm_buffer[pcm_wix++] = l[j];
			pcm_cnt++;
			if (pcm_wix >= pcm_samples)
				pcm_wix = 0; // circular buffer
			if (astream_format.channels == 2)
			{
				pcm_buffer[pcm_wix++] = r[j];
				pcm_cnt++;
				if (pcm_wix >= pcm_samples)
					pcm_wix = 0; // circular buffer
			}
		}
	}

	return PCM_SUCCESS;
}

int decode_ADPCM_Yamaha(FILE *in, int chunk_size)
{
    lpred = rpred = 0;
    lstep = rstep = 127;

	in->Read(read_buffer, chunk_size);

	for (int i=0; i<chunk_size; i++)
	{
		int v = read_buffer[i];
		lpred += (lstep * yamaha_difflookup[v & 0x0F]) / 8;
		lpred = lpred < -32768 ? -32768 : lpred > 32767 ? 32767 : lpred;
		lstep = (lstep * yamaha_indexscale[v & 0x0F]) >> 8;
		lstep = lstep < 127 ? 127 : lstep > 24567 ? 24567 : lstep;

		pcm_buffer[pcm_wix++] = lpred;
		pcm_cnt++;
		if (pcm_wix >= pcm_samples)
			pcm_wix = 0; // circular buffer

		if (astream_format.channels == 2)
		{
			rpred += (rstep * yamaha_difflookup[(v >> 4) & 0x0F]) / 8;
			rpred = rpred < -32768 ? -32768 : rpred > 32767 ? 32767 : rpred;
			rstep = (rstep * yamaha_indexscale[(v >> 4) & 0x0F]) >> 8;
			rstep = rstep < 127 ? 127 : rstep > 24567 ? 24567 : rstep;

			pcm_buffer[pcm_wix++] = rpred;
			pcm_cnt++;
			if (pcm_wix >= pcm_samples)
				pcm_wix = 0; // circular buffer
		}
		else
		{
			lpred += (lstep * yamaha_difflookup[(v >> 4) & 0x0F]) / 8;
			lpred = lpred < -32768 ? -32768 : lpred > 32767 ? 32767 : lpred;
			lstep = (lstep * yamaha_indexscale[(v >> 4) & 0x0F]) >> 8;
			lstep = lstep < 127 ? 127 : lstep > 24567 ? 24567 : lstep;

			pcm_buffer[pcm_wix++] = lpred;
			pcm_cnt++;
			if (pcm_wix >= pcm_samples)
				pcm_wix = 0; // circular buffer
		}
	}

	return PCM_SUCCESS;
}
