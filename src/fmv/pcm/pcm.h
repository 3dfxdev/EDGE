/*
 *
 * Decode PCM/ADPCM - Copyright 2016 by Chilly Willy <ChillyWillyGuru@gmail.com>
 *
 * This code falls under the BSD license.
 *
 */

#ifndef DECODE_PCM_H
#define DECODE_PCM_H

#define PCM_SUCCESS	0


int decode_PCM(epi::file_c *in, int chunk_size);
int decode_ADPCM_IMA(epi::file_c *in, int chunk_size);
int decode_ADPCM_Yamaha(epi::file_c *in, int chunk_size);

#endif
