/*
 * Decode ROQ by Chilly Willy
 *
 * based on Dreamroq by Mike Melanson & Josh Pearson
 * uses KOS license terms
 *
 */

#ifndef DECODE_ROQ_H
#define DECODE_ROQ_H


#define RoQ_INFO           0x1001
#define RoQ_QUAD_CODEBOOK  0x1002
#define RoQ_QUAD_VQ        0x1011
#define RoQ_SOUND_MONO     0x1020
#define RoQ_SOUND_STEREO   0x1021
#define RoQ_SIGNATURE      0x1084

#define CHUNK_HEADER_SIZE 8

#define LE_16(buf) (*buf | (*(buf+1) << 8))
#define LE_32(buf) (*buf | (*(buf+1) << 8) | (*(buf+2) << 16) | (*(buf+3) << 24))

#define MAX_BUF_SIZE 0x10000
#define MAX_BUF_MASK 0x0FFFF

#define ROQ_CODEBOOK_SIZE 256

#define ROQ_SUCCESS             0
#define ROQ_FILE_OPEN_FAILURE -11
#define ROQ_FILE_READ_FAILURE -12
#define ROQ_CHUNK_TOO_LARGE   -13
#define ROQ_BAD_CODEBOOK      -14
#define ROQ_INVALID_PIC_SIZE  -15
#define ROQ_NO_MEMORY         -16
#define ROQ_BAD_VQ_STREAM     -17
#define ROQ_INVALID_DIMENSION -18
#define ROQ_RENDER_PROBLEM    -19
#define ROQ_CLIENT_PROBLEM    -20
#define ROQ_USER_INTERRUPT    -21


#define RGB565_RED_MASK    0xF800
#define RGB565_GREEN_MASK  0x7E0
#define RGB565_BLUE_MASK   0x1F

#define RGB565_RED_SHIFT   8
#define RGB565_GREEN_SHIFT 3
#define RGB565_BLUE_SHIFT  3 // right shift


int decode_RoQ(epi::file_c *in, int size);

#endif
