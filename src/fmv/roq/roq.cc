/*
 * Decode ROQ by Chilly Willy
 *
 * based on Dreamroq by Mike Melanson & Josh Pearson
 * uses KOS license terms
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../epi/epi.h"
#include "../../../epi/endianess.h"
#include "../../../epi/file.h"
#include "../../../epi/file_sub.h"

#include "../../system/i_system.h"
#include "../../system/i_defs.h"
#include "../../system/i_defs_gl.h"


#include "roq.h"
#define FMV_DEBUG

static int width;
static int height;
static int mb_width;
static int mb_height;
static int mb_count;

static int current_frame;
static int stride;

static unsigned short __declspec(align(16)) cb2x2[ROQ_CODEBOOK_SIZE][4];
static unsigned short __declspec(align(16)) cb4x4[ROQ_CODEBOOK_SIZE][16];

// allocated buffers from avi.cc
extern unsigned short *frame[2];
extern unsigned char *read_buffer;

extern int texture_width, texture_height;
extern float scale_width, scale_height;


static int roq_unpack_quad_codebook(unsigned char *buf, int size, int arg)
{
    int y[4];
    int u, v, u1, u2, v1, v2;
    int r, g, b;
    int count2x2;
    int count4x4;
    int i, j;
    unsigned short *v2x2;
    unsigned short *v4x4;

    count2x2 = (arg >> 8) & 0xFF;
    count4x4 =  arg       & 0xFF;

    if (!count2x2)
        count2x2 = ROQ_CODEBOOK_SIZE;
    /* 0x00 means 256 4x4 vectors if there is enough space in the chunk
     * after accounting for the 2x2 vectors */
    if (!count4x4 && count2x2 * 6 < size)
        count4x4 = ROQ_CODEBOOK_SIZE;

    /* size sanity check */
    if ((count2x2 * 6 + count4x4 * 4) != size)
    {
        return ROQ_BAD_CODEBOOK;
    }

    /* unpack the 2x2 vectors */
    for (i = 0; i < count2x2; i++)
    {
        /* unpack the YUV components from the bytestream */
        for (j = 0; j < 4; j++)
            y[j] = *buf++;
        u  = *buf++;
        v  = *buf++;
        u -= 128;
        v -= 128;
		/* CCIR 601 conversion */
        u1 = (88 * u) >> 8;
        u2 = (453 * u) >> 8;
        v1 = (359 * v) >> 8;
        v2 = (183 * v) >> 8;

        /* convert to RGB565 */
        for (j = 0; j < 4; j++)
        {
			/* CCIR 601 conversion */
			r = y[j] + v1;
			g = y[j] - v2 - u1;
			b = y[j] + u2;

            if (r < 0) r = 0;
            else if (r > 255) r = 255;
            if (g < 0) g = 0;
            else if (g > 255) g = 255;
            if (b < 0) b = 0;
            else if (b > 255) b = 255;

            cb2x2[i][j] = (unsigned short)(((r << RGB565_RED_SHIFT) & RGB565_RED_MASK) |
						  ((g << RGB565_GREEN_SHIFT) & RGB565_GREEN_MASK) |
						  ((b >> RGB565_BLUE_SHIFT) & RGB565_BLUE_MASK));
        }
    }

    /* unpack the 4x4 vectors */
    for (i = 0; i < count4x4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            v2x2 = cb2x2[*buf++];
            v4x4 = cb4x4[i] + (j / 2) * 8 + (j % 2) * 2;
            v4x4[0] = v2x2[0];
            v4x4[1] = v2x2[1];
            v4x4[4] = v2x2[2];
            v4x4[5] = v2x2[3];
        }
    }

    return ROQ_SUCCESS;
}

#define GET_BYTE(x) \
    if (index >= size) { \
        status = ROQ_BAD_VQ_STREAM; \
        x = 0; \
    } else { \
        x = buf[index++]; \
    }

#define GET_MODE() \
    if (!mode_count) { \
        GET_BYTE(mode_lo); \
        GET_BYTE(mode_hi); \
        mode_set = (mode_hi << 8) | mode_lo; \
        mode_count = 16; \
    } \
    mode_count -= 2; \
    mode = (mode_set >> mode_count) & 0x03;

static int roq_unpack_vq(unsigned char *buf, int size, unsigned int arg)
{
    int status = ROQ_SUCCESS;
    int mb_x, mb_y;
    int block;     /* 8x8 blocks */
    int subblock;  /* 4x4 blocks */
    int i, which;

    /* frame and pixel management */
    unsigned short *this_frame;
    unsigned short *last_frame;

    int line_offset;
    int mb_offset;
    int block_offset;
    int subblock_offset;

    unsigned short *this_ptr;
    unsigned short *last_ptr;
    unsigned short *vector;

    /* bytestream management */
    int index = 0;
    int mode_set = 0;
    int mode, mode_lo, mode_hi;
    int mode_count = 0;

    /* vectors */
    int mx, my;
    int motion_x, motion_y;
    unsigned char data_byte;

    if (current_frame == 0)
    {
        this_frame = frame[0];
        last_frame = frame[1];
    }
    else
    {
        this_frame = frame[1];
        last_frame = frame[0];
    }
    which = current_frame;
    current_frame ^= 1; // flip frames

    mx = (arg >> 8) & 0xFF;
    my =  arg       & 0xFF;

    for (mb_y = 0; mb_y < mb_height && status == ROQ_SUCCESS; mb_y++)
    {
        line_offset = mb_y * 16 * stride;
        for (mb_x = 0; mb_x < mb_width && status == ROQ_SUCCESS; mb_x++)
        {
            mb_offset = line_offset + mb_x * 16;
			/* macro blocks are 16x16 and are subdivided into four 8x8 blocks */
            for (block = 0; block < 4 && status == ROQ_SUCCESS; block++)
            {
                block_offset = mb_offset + (block / 2 * 8 * stride) + (block % 2 * 8);
                /* each 8x8 block gets a mode */
                GET_MODE();
                switch (mode)
                {
                case 0:  /* MOT: skip */
                    break;

                case 1:  /* FCC: motion compensation */
                    GET_BYTE(data_byte);
                    motion_x = 8 - (data_byte >>  4) - mx;
                    motion_y = 8 - (data_byte & 0xF) - my;
                    last_ptr = last_frame + block_offset +
                        (motion_y * stride) + motion_x;
                    this_ptr = this_frame + block_offset;

                    for (i = 0; i < 8; i++)
                    {
                        *this_ptr++ = *last_ptr++;
                        *this_ptr++ = *last_ptr++;
                        *this_ptr++ = *last_ptr++;
                        *this_ptr++ = *last_ptr++;
                        *this_ptr++ = *last_ptr++;
                        *this_ptr++ = *last_ptr++;
                        *this_ptr++ = *last_ptr++;
                        *this_ptr++ = *last_ptr++;

                        last_ptr += stride - 8;
                        this_ptr += stride - 8;
                    }
                    break;

                case 2:  /* SLD: upsample 4x4 vector */
                    GET_BYTE(data_byte);
                    vector = cb4x4[data_byte];

                    for (i = 0; i < 4*4; i++)
                    {
                        this_ptr = this_frame + block_offset +
                            (i / 4 * 2 * stride) + (i % 4 * 2);
                        this_ptr[0] = *vector;
                        this_ptr[1] = *vector;
                        this_ptr[stride+0] = *vector;
                        this_ptr[stride+1] = *vector;

                        vector++;
                    }
                    break;

                case 3:  /* CCC: subdivide into four 4x4 subblocks */
                    for (subblock = 0; subblock < 4; subblock++)
                    {
                        subblock_offset = block_offset + (subblock / 2 * 4 * stride) + (subblock % 2 * 4);

                        GET_MODE();
                        switch (mode)
                        {
                        case 0:  /* MOT: skip */
                             break;

                        case 1:  /* FCC: motion compensation */
                            GET_BYTE(data_byte);
                            motion_x = 8 - (data_byte >>  4) - mx;
                            motion_y = 8 - (data_byte & 0xF) - my;
                            last_ptr = last_frame + subblock_offset +
                                (motion_y * stride) + motion_x;
                            this_ptr = this_frame + subblock_offset;

                            for (i = 0; i < 4; i++)
                            {
                                *this_ptr++ = *last_ptr++;
                                *this_ptr++ = *last_ptr++;
                                *this_ptr++ = *last_ptr++;
                                *this_ptr++ = *last_ptr++;

                                last_ptr += stride - 4;
                                this_ptr += stride - 4;
                            }
                            break;

                        case 2:  /* SLD: use 4x4 vector from codebook */
                            GET_BYTE(data_byte);
                            vector = cb4x4[data_byte];
                            this_ptr = this_frame + subblock_offset;

                            for (i = 0; i < 4; i++)
                            {
                                *this_ptr++ = *vector++;
                                *this_ptr++ = *vector++;
                                *this_ptr++ = *vector++;
                                *this_ptr++ = *vector++;

                                this_ptr += stride - 4;
                            }
                            break;

                        case 3:  /* CCC: subdivide into four 2x2 subblocks */
                            GET_BYTE(data_byte);
                            vector = cb2x2[data_byte];
                            this_ptr = this_frame + subblock_offset;

                            this_ptr[0] = vector[0];
                            this_ptr[1] = vector[1];
                            this_ptr[stride+0] = vector[2];
                            this_ptr[stride+1] = vector[3];

                            GET_BYTE(data_byte);
                            vector = cb2x2[data_byte];

                            this_ptr[2] = vector[0];
                            this_ptr[3] = vector[1];
                            this_ptr[stride+2] = vector[2];
                            this_ptr[stride+3] = vector[3];

                            this_ptr += stride * 2;

                            GET_BYTE(data_byte);
                            vector = cb2x2[data_byte];

                            this_ptr[0] = vector[0];
                            this_ptr[1] = vector[1];
                            this_ptr[stride+0] = vector[2];
                            this_ptr[stride+1] = vector[3];

                            GET_BYTE(data_byte);
                            vector = cb2x2[data_byte];

                            this_ptr[2] = vector[0];
                            this_ptr[3] = vector[1];
                            this_ptr[stride+2] = vector[2];
                            this_ptr[stride+3] = vector[3];
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }

    /* sanity check to see if the stream was fully consumed */
    if ((status == ROQ_SUCCESS) && (index < size-2))
    {
        status = ROQ_BAD_VQ_STREAM;
    }

    return status < 0 ? status : which;
}

int decode_RoQ(epi::file_c *in, int size)
{
    int chunk_id;
    unsigned int chunk_size;
    unsigned int chunk_arg;
    int status = ROQ_SUCCESS;

	while (size > 0 && status >= ROQ_SUCCESS)
	{
		in->Read(read_buffer, CHUNK_HEADER_SIZE);
        size -= CHUNK_HEADER_SIZE;
        chunk_id   = EPI_LE_U16(read_buffer[0]);
        chunk_size = EPI_LE_U32(read_buffer[2]);
        chunk_arg  = EPI_LE_U16(read_buffer[6]);

        if (chunk_size > MAX_BUF_SIZE)
        {
            return ROQ_CHUNK_TOO_LARGE;
        }
		in->Read(read_buffer, chunk_size);
        size -= chunk_size;

        switch(chunk_id)
        {
        case RoQ_INFO:
            width = EPI_LE_U16(read_buffer[0]);
            height = EPI_LE_U16(read_buffer[2]);

            /* width and height each need to be divisible by 16 */
            if ((width & 0xF) || (height & 0xF))
            {
                status = ROQ_INVALID_PIC_SIZE;
                break;
            }
            if (width < 16 || width > 1024 || height < 16 || height > 1024)
            {
                status = ROQ_INVALID_DIMENSION;
                break;
			}
			if (width <= 256)
			{
				stride = 256;
			}
			else if (width <= 512)
			{
				stride = 512;
			}
			else if (width <= 1024)
			{
				stride = 1024;
			}
			else
            {
                status = ROQ_INVALID_DIMENSION;
                break;
			}

            mb_width = width / 16;
            mb_height = height / 16;
            mb_count = mb_width * mb_height;

#ifdef FMV_DEBUG
            I_Printf("RoQ INFO: %dx%d (%dx%d)", width, height, mb_width, mb_height);
#endif

            current_frame = 0;
            break;

        case RoQ_QUAD_CODEBOOK:
            status = roq_unpack_quad_codebook(read_buffer, chunk_size, chunk_arg);
            break;

        case RoQ_QUAD_VQ:
            status = roq_unpack_vq(read_buffer, chunk_size, chunk_arg);
            break;

        default:
			/* unknown chunk */
			status = ROQ_FILE_READ_FAILURE;
            break;
        }
    }

    return status;
}

