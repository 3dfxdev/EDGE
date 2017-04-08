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


int decode_PCM(FILE *in, int chunk_size);
int decode_ADPCM_IMA(FILE *in, int chunk_size);
int decode_ADPCM_Yamaha(FILE *in, int chunk_size);

#endif
