//----------------------------------------------------------------------------
//  EDGE2 Dreamcast Motion Video Code
//----------------------------------------------------------------------------
//
//  Copyright (c) 2017 Chilly Willy.
//  Copyright (c) 2015 Josh Pearson.
//  Copyright (c) 1999-2009  The EDGE2 Team.
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

//#include <kos.h>
//#include <dc/sound/stream.h>

#include "i_defs.h"
#include "i_defs_gl.h"
#include "i_sdlinc.h"

#include <signal.h>

#include "e_input.h"
#include "m_argv.h"
#include "m_misc.h"
#include "r_modes.h"

#include "dm_state.h"
#include "w_wad.h"
#include "e_main.h"
#include "r_gldefs.h"
#include "r_image.h"
#include "s_sound.h"

//#include "../fmv/Simple-SDL2-Audio/src/audio.h"

#include "../epi/file.h"
#include "../epi/filesystem.h"
#include "../epi/file_memory.h"

static int movie_init = 0;

static byte *mbuf = NULL; // movie data read buffer
static int msiz; // current movie data buffer size

static volatile byte VID_FLAGS;
static volatile byte AUD_FLAGS;

static byte *sbuf = NULL;

static s16_t width;
static s16_t height;
static s16_t mb_width;
static s16_t mb_height;
static s16_t mb_count;
static s16_t vix;
static s16_t vfc;
static u16_t *frames[2];
static u16_t *frame0;
static u16_t *frame1;

static int arix, awix; // circular buffer indexes
static int absz; // audio buffer size tracking for timing

static bool playing_movie = false;
static bool movie_pause = false;
static bool movie_done = true;

static int strm_buf_size; // stream buffer size, must be <= SND_STREAM_BUFFER_MAX
static int strm_play_rate; // sample rate for audio
static int strm_frame_rate; // frame rate for video

extern bool E_IsKeyPressed(int keyvar);

#define ROQ_QUAD            0x1000
#define RoQ_QUAD_INFO       0x1001
#define RoQ_QUAD_CODEBOOK   0x1002
#define RoQ_QUAD_VQ         0x1011
#define RoQ_QUAD_JPEG       0x1012
#define RoQ_QUAD_HANG       0x1013
#define RoQ_SOUND_MONO      0x1020
#define RoQ_SOUND_STEREO    0x1021
#define RoQ_MONO_VQ         0x1022
#define RoQ_STEREO_VQ       0x1023
#define RoQ_PACKET          0x1030
#define RoQ_SIGNATURE       0x1084

#define RVQ_FRAME_SIZE      0x1000
#define RVQ_FRAME_CODES     (RVQ_FRAME_SIZE>>1)
#define RVQ_FRAME_SAMPLES   (RVQ_FRAME_CODES<<2)

#define ROQ_CODEBOOK_SIZE   256

#define ROQ_SUCCESS             0
#define ROQ_STOPPED            -1
#define ROQ_BAD_CHUNK         -12
#define ROQ_CHUNK_TOO_LARGE   -13
#define ROQ_BAD_CODEBOOK      -14
#define ROQ_INVALID_PIC_SIZE  -15
#define ROQ_NO_MEMORY         -16
#define ROQ_BAD_VQ_STREAM     -17
#define ROQ_INVALID_DIMENSION -18
#define ROQ_RENDER_PROBLEM    -19
#define ROQ_CLIENT_PROBLEM    -20
#define ROQ_USER_INTERRUPT    -21

#define CHUNK_HEADER_SIZE   8

#define LE_16(buf) (*buf | (*(buf+1) << 8))
#define LE_32(buf) (*buf | (*(buf+1) << 8) | (*(buf+2) << 16) | (*(buf+3) << 24))

#define AICA_VOL_LOW    128.0f
#define AICA_VOL_HIGH   255.0f

extern int au_mus_volume;
SDL_AudioSpec fmt;
SDL_AudioDeviceID dev;
extern u8_t pcm_buffer[]; // circular buffer of stereo pcm samples (wav buffer)

// pcm_buffer size in bytes
#define PCM_BUF_SIZE 65536

#define RGB565_RED_MASK    0xF800
#define RGB565_GREEN_MASK  0x07E0
#define RGB565_BLUE_MASK   0x001F

#define RGB565_RED_SHIFT   8
#define RGB565_GREEN_SHIFT 3
#define RGB565_BLUE_SHIFT  3 // right shift

#ifdef LINUX
static unsigned short __attribute__((aligned(16))) cb2x2[ROQ_CODEBOOK_SIZE][4];
static unsigned short __attribute__((aligned(16))) cb4x4[ROQ_CODEBOOK_SIZE][16];
#else
static unsigned short __declspec(align(16)) cb2x2[ROQ_CODEBOOK_SIZE][4];
static unsigned short __declspec(align(16)) cb4x4[ROQ_CODEBOOK_SIZE][16];
#endif

GLuint tex[1];
int texture_width, texture_height;
float scale_width, scale_height;

static u32_t movie_status = 0;
static SDL_Thread* movie_thread = NULL;

// define to accumulate and print timing info on movie player
//#define PROFILE_MOVIE 0
#ifdef PROFILE_MOVIE
static int num_frames;
static int total_time;
static int chunk_time;
static int cbook_time;
static int vqfrm_time;
static int vqpcm_time;
static int stream_time;
static int opengl_time;
#endif

// movie sound poll thread
#if 1
void do_movie(void *argp)
{
#ifdef PROFILE_MOVIE
	u64_t usec;
#endif

	while (movie_status != 0xDEADBEEF)
	{
		//dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
		SDL_Delay(10);
		if (dev != NULL)
		{
#ifdef PROFILE_MOVIE
			usec = timer_us_gettime64();
#endif
			SDL_AudioCallback(fmt);
#ifdef PROFILE_MOVIE
			usec = timer_us_gettime64() - usec;
			stream_time += (int)usec;
#endif
	}
	}
	return;
}
#endif // 0


// stream wants more PCM samples
static void *stream_callback(SDL_AudioSpec hnd, int size, int *size_out)
{
    (void)hnd;
    int num_samples;
    int rrix = arix;

    if (!playing_movie || movie_pause || movie_done || (AUD_FLAGS == 0x80))
    {
        if (sbuf == NULL)
        {
            sbuf = (byte *)malloc(PCM_BUF_SIZE);
            if (sbuf == NULL)
                return NULL;
            memset(sbuf, 0, size);
        }
        *size_out = size;
        return sbuf;
    }

//    if (size > strm_buf_size)
//        size = strm_buf_size;

    if ((arix + size) >= PCM_BUF_SIZE)
    {
        num_samples = (PCM_BUF_SIZE - arix) / 4;
        size = num_samples * 4;
        arix = 0;
    }
    else
    {
        num_samples = size / 4;
        size = num_samples * 4;
        arix += size;
    }

    absz += size;
    if (absz >= strm_buf_size)
    {
        VID_FLAGS |= 1; // time for another frame
        //absz -= strm_buf_size;
        absz = 0;
    }

    *size_out = size;
    return pcm_buffer + rrix;
}

// decode RoQ codebook
static int roq_unpack_quad_codebook(byte *buf, int size, int arg)
{
    int y[4];
    int u, v, u1, u2, v1, v2;
#ifndef CCIR_601
    int yp;
#endif
    int r, g, b;
    int count2x2;
    int count4x4;
    int i, j;
    u16_t *v2x2;
    u16_t *v4x4;

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
#ifndef CCIR_601
        u1 = (100 * u) >> 8;
        u2 = (517 * u) >> 8;
        v1 = (409 * v) >> 8;
        v2 = (208 * v) >> 8;
#else
        /* CCIR 601 conversion */
        u1 = (88 * u) >> 8;
        u2 = (453 * u) >> 8;
        v1 = (359 * v) >> 8;
        v2 = (183 * v) >> 8;
#endif
        /* convert to RGB565 */
        for (j = 0; j < 4; j++)
        {
#ifndef CCIR_601
			yp = ((y[j] - 16) * 517) >> 8;
			r = yp + v1;
			g = yp - v2 - u1;
			b = yp + u2;
#else
            /* CCIR 601 conversion */
            r = y[j] + v1;
            g = y[j] - v2 - u1;
            b = y[j] + u2;
#endif
            if (r < 0) r = 0;
            else if (r > 255) r = 255;
            if (g < 0) g = 0;
            else if (g > 255) g = 255;
            if (b < 0) b = 0;
            else if (b > 255) b = 255;

            cb2x2[i][j] = (u16_t)(((r << RGB565_RED_SHIFT) & RGB565_RED_MASK) |
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

// decode RoQ frame
static int roq_unpack_vq(byte *buf, int size, u16_t arg)
{
    int status = ROQ_SUCCESS;
    int mb_x, mb_y;
    int block;     /* 8x8 blocks */
    int subblock;  /* 4x4 blocks */
    int i, which;

    /* frame and pixel management */
    u16_t *this_frame;
    u16_t *last_frame;

    int line_offset;
    int mb_offset;
    int block_offset;
    int subblock_offset;

    u16_t *this_ptr;
    u16_t *last_ptr;
    u16_t *vector;

    s16_t stride = texture_width;

    /* bytestream management */
    int index = 0;
    int mode_set = 0;
    int mode, mode_lo, mode_hi;
    int mode_count = 0;

    /* vectors */
    int mx, my;
    int motion_x, motion_y;
    byte data_byte;

    if (vix == 0)
    {
        this_frame = frame0;
        last_frame = frame1;
    }
    else
    {
        this_frame = frame1;
        last_frame = frame0;
    }
    which = vix;
    vix ^= 1; // flip frames

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
        I_Printf("  ROQ_BAD_VQ_STREAM\n");
        status = ROQ_BAD_VQ_STREAM;
    }

    return status;
}

// decode RVQ audio frame
static int decode_RVQ(byte *in, int chunk_size, u16_t chunk_arg, int chans)
{
    int i;

    // counts on # samples in RVQ frame being an even multiple of the pcm buffer size
    byte *cp = &in[RVQ_FRAME_CODES];
    s16_t *cb = (s16_t *)in;
    s16_t *out = (s16_t *)&pcm_buffer[awix];

    // decode left/mono channel
    for (i=0; i< RVQ_FRAME_CODES; i++)
    {
        int j = *cp++ * 4;
        out[(i*4+0)*2] = cb[j + 0];
        out[(i*4+1)*2] = cb[j + 1];
        out[(i*4+2)*2] = cb[j + 2];
        out[(i*4+3)*2] = cb[j + 3];
    }
    if (chans == 1)
    {
        // mono - duplicate left channel to right
        for (i=0; i< RVQ_FRAME_SAMPLES; i++)
            out[i*2 + 1] = out[i*2];
    }
    else
    {
        cp = &in[RVQ_FRAME_SIZE + RVQ_FRAME_CODES];
        cb = (s16_t *)(in + RVQ_FRAME_SIZE);
        // decode right channel
        for (i=0; i< RVQ_FRAME_CODES; i++)
        {
            int j = *cp++ * 4;
            out[(i*4+0)*2+1] = cb[j + 0];
            out[(i*4+1)*2+1] = cb[j + 1];
            out[(i*4+2)*2+1] = cb[j + 2];
            out[(i*4+3)*2+1] = cb[j + 3];
        }
    }

    awix = (awix + RVQ_FRAME_SAMPLES * 4) & (PCM_BUF_SIZE - 1);
    return ROQ_SUCCESS;
}

// decode one or more RoQ chunks
static int32_t decode_RoQ(u8_t *in, u32_t size)
{
	//SDL_AudioSpec fmt;
    u16_t chunk_id, chunk_arg;
    u32_t chunk_size;
    int status = ROQ_SUCCESS;
#ifdef PROFILE_MOVIE
    u64_t usec;
#endif

    // safety check!
    if (size < CHUNK_HEADER_SIZE)
        return ROQ_BAD_CHUNK;

    while (size >= CHUNK_HEADER_SIZE && status >= ROQ_SUCCESS)
    {
        chunk_id   = LE_16(&in[0]);
        chunk_size = LE_32(&in[2]);
        chunk_arg  = LE_16(&in[6]);
        in += CHUNK_HEADER_SIZE;
        size -= CHUNK_HEADER_SIZE;

        switch(chunk_id)
        {
        case RoQ_QUAD_INFO:
            //I_Printf("  ROQ_QUAD_INFO\n");
            width = LE_16(&in[0]);
            height = LE_16(&in[2]);
            //I_Printf("    %dx%d\n", width, height);

            /* width and height each need to be divisible by 16 */
            if ((width & 0xF) || (height & 0xF))
                return ROQ_INVALID_PIC_SIZE;

            if (width < 16 || width > 1024 || height < 16 || height > 1024)
                return ROQ_INVALID_DIMENSION;

            mb_width = width / 16;
            mb_height = height / 16;
            mb_count = mb_width * mb_height;

            // setup for video decode
            if (width <= 256)
                texture_width = 256;
            else if (width <= 512)
                texture_width = 512;
            else if (width <= 1024)
                texture_width = 1024;

            if (height <= 256)
                texture_height = 256;
            else if (height <= 512)
                texture_height = 512;
            else if (height <= 1024)
                texture_height = 1024;

            // setup OpenGL texture buffer
            glGenTextures(1, tex);

            scale_width = (float)width / (float)texture_width;
            scale_height = (float)height / (float)texture_height;

            frame0 = (u16_t *)malloc(texture_width * height * 2);
            frame1 = (u16_t *)malloc(texture_width * height * 2);
            if (!frame0 || !frame1)
                return ROQ_NO_MEMORY;
            memset(frame0, 0, texture_width * height * 2);
            memset(frame1, 0, texture_width * height * 2);
            break;

        case RoQ_QUAD_CODEBOOK:
            //I_Printf("  ROQ_QUAD_CODEBOOK\n");
#ifdef PROFILE_MOVIE
            usec = timer_us_gettime64();
#endif
            status = roq_unpack_quad_codebook(in, chunk_size, chunk_arg);
#ifdef PROFILE_MOVIE
            usec = timer_us_gettime64() - usec;
            cbook_time += (int)usec;
#endif
            break;

        case RoQ_QUAD_VQ:
            //I_Printf("  ROQ_QUAD_VQ\n");
#ifdef PROFILE_MOVIE
            usec = timer_us_gettime64();
#endif
            status = roq_unpack_vq(in, chunk_size, chunk_arg);
#ifdef PROFILE_MOVIE
            usec = timer_us_gettime64() - usec;
            vqfrm_time += (int)usec;
#endif
            if (status == ROQ_SUCCESS)
            {
                frames[vfc] = vix ? frame0 : frame1;
                vfc++;
#ifdef PROFILE_MOVIE
                num_frames++;
#endif
            }
            break;

#ifdef DECODE_ROQ_JPEG
        case RoQ_QUAD_JPEG:
            status = decode_JPG(in, chunk_size, chunk_arg);
            break;
#endif
        case RoQ_QUAD_HANG:
            /* no data - just stop handling RoQ chunks when you encounter this */
            return ROQ_STOPPED;

#ifdef DECODE_ROQ_DPCM
        case RoQ_SOUND_MONO:
            rate = 22050;
            chans = 1;
            status = decode_DPCM(in, chunk_size, chunk_arg, 1);
            break;

        case RoQ_SOUND_STEREO:
            rate = 22050;
            chans = 2;
            status = decode_DPCM(in, chunk_size, chunk_arg, 2);
            break;
#endif
        case RoQ_MONO_VQ:
            //I_Printf("  ROQ_MONO_VQ\n");
            strm_play_rate = chunk_arg;
            strm_buf_size = strm_play_rate / strm_frame_rate * 4;

            if (AUD_FLAGS == 0x80)
            {
#if 0
				// setup stream
				// audiospec is stream_hnd
				// memset needs to &stream_hnd, 0, sizeof(stream_hnd)
				SDL_memset(&stream_hnd, NULL, sizeof(PCM_BUF_SIZE));
				SDL_OpenAudioDevice(NULL, 0, stream_hnd, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
				if (stream_hnd == NULL)
				{
					I_Printf("%s\n", " snd_stream_alloc() failed");
					return ROQ_NO_MEMORY;
				}
				int vol = (int)((float)au_mus_volume * (AICA_VOL_HIGH - AICA_VOL_LOW) / (float)(SND_SLIDER_NUM - 1) + AICA_VOL_LOW);
				snd_stream_reinit(stream_hnd, &stream_callback);
				snd_stream_start(stream_hnd, strm_play_rate, 1);
				snd_stream_volume(stream_hnd, vol);
#endif // 0
				fmt.freq = 22050;
				fmt.format = AUDIO_S16;
				fmt.channels = chunk_arg;
				fmt.samples = 512;
				fmt.callback = NULL;
				fmt.userdata = (void *)do_movie;
				int vol = (int)((float)au_mus_volume * (AICA_VOL_HIGH - AICA_VOL_LOW) / (float)(SND_SLIDER_NUM - 1) + AICA_VOL_LOW);

				if (SDL_OpenAudio(&fmt, NULL) < 0)
				{
					I_Printf("Unable to open movie audio: %s\n", SDL_GetError());
					return -1;
				}

				SDL_PauseAudio(0);
            }
            // decode mono RVQ audio chunk
#ifdef PROFILE_MOVIE
            usec = timer_us_gettime64();
#endif
            status = decode_RVQ(in, chunk_size, chunk_arg, 1);
#ifdef PROFILE_MOVIE
            usec = timer_us_gettime64() - usec;
            vqpcm_time += (int)usec;
#endif
            AUD_FLAGS = 0x40; // start processing pcm
            break;

        case RoQ_STEREO_VQ:
            //I_Printf("  ROQ_STEREO_VQ\n");
            strm_play_rate = chunk_arg;
            strm_buf_size = strm_play_rate / strm_frame_rate * 4;

            if (AUD_FLAGS == 0x80)
            {
#if 0
				// setup stream
				// setup stream
				// audiospec is stream_hnd
				// memset needs to &stream_hnd, 0, sizeof(stream_hnd)
				SDL_memset(&stream_hnd, NULL, sizeof(PCM_BUF_SIZE));
				if (stream_hnd == NULL)
				{
					I_Printf("%s\n", " snd_stream_alloc() failed");
					return ROQ_NO_MEMORY;
				}
				int vol = (int)((float)au_mus_volume * (AICA_VOL_HIGH - AICA_VOL_LOW) / (float)(SND_SLIDER_NUM - 1) + AICA_VOL_LOW);
				SDL_realloc(stream_hnd, &stream_callback);
				snd_stream_start(stream_hnd, strm_play_rate, 1);
				snd_stream_volume(stream_hnd, vol);
#endif // 0
				fmt.freq = 22050;
				fmt.format = AUDIO_S16;
				fmt.channels = chunk_arg;
				fmt.samples = 512;
				fmt.callback = NULL;
				fmt.userdata = (void *)do_movie;
				int vol = (int)((float)au_mus_volume * (AICA_VOL_HIGH - AICA_VOL_LOW) / (float)(SND_SLIDER_NUM - 1) + AICA_VOL_LOW);

				if (SDL_OpenAudio(&fmt, NULL) < 0)
				{
					I_Printf("Unable to open movie audio: %s\n", SDL_GetError());
					return -1;
				}

				SDL_PauseAudio(0);
            }
            // decode stereo RVQ audio chunk
#ifdef PROFILE_MOVIE
            usec = timer_us_gettime64();
#endif
            status = decode_RVQ(in, chunk_size, chunk_arg, 2);
#ifdef PROFILE_MOVIE
            usec = timer_us_gettime64() - usec;
            vqpcm_time += (int)usec;
#endif
            AUD_FLAGS = 0x40; // start processing pcm
            break;

        case RoQ_PACKET:
            /* the chunk has chunk_arg # of RoQ chunks in it - recurse */
            status = decode_RoQ(in, chunk_size);
            break;

        case RoQ_SIGNATURE:
            //I_Printf("  ROQ_SIGNATURE\n");
            strm_frame_rate = chunk_arg;
            //I_Printf("  frame rate: %d\n", strm_frame_rate);
            chunk_size = 0;
            arix = awix = absz = 0;
            vfc = vix = 0;
            break;

        default:
            /* unknown chunk */
            I_Printf("  ROQ_BAD_CHUNK\n");
            return ROQ_BAD_CHUNK;
        }
        in += chunk_size;
        size -= chunk_size;
    }

    return status;
}

// fill the movie data buffer with the next RoQ chunk and return the chunk size
static int get_chunk(epi::file_c *F, int *length)
{
    u16_t chunk_id;
    u32_t chunk_size;
#ifdef PROFILE_MOVIE
    u64_t usec = timer_us_gettime64();
#endif

    if (*length < CHUNK_HEADER_SIZE)
    {
        *length = 0;
        return 0;
    }

    F->Read(mbuf, CHUNK_HEADER_SIZE);
    chunk_id   = LE_16(&mbuf[0]);
    chunk_size = LE_32(&mbuf[2]);

    if (chunk_id == RoQ_SIGNATURE)
        chunk_size = CHUNK_HEADER_SIZE;
    else
    {
        chunk_size += CHUNK_HEADER_SIZE;
        if (*length < (int)chunk_size)
            chunk_size = *length; // just gimme what's left

        if (msiz < (int)chunk_size)
        {
            mbuf = (byte *)realloc(mbuf, chunk_size);
            msiz = chunk_size;
            // TODO: check realloc okay
        }
        F->Read(&mbuf[CHUNK_HEADER_SIZE], chunk_size - CHUNK_HEADER_SIZE);
    }

#ifdef PROFILE_MOVIE
    usec = timer_us_gettime64() - usec;
    chunk_time += (int)usec;
#endif

    *length -= chunk_size;
    return chunk_size;
}


void I_StartupMovie(void)
{
    memset(pcm_buffer, 0, PCM_BUF_SIZE);

    movie_init = 1;
    I_Printf("I_StartupMovie: initialisation OK\n");
}

void I_ShutdownMovie(void)
{
    if (!movie_init)
        return;

    movie_init = 0;
    I_Printf("I_ShutdownMovie: shut down OK\n");
}

int I_PlayMovie(char *name)
{
    int res = ROQ_SUCCESS;
#ifdef PROFILE_MOVIE
    uint64 usec;
#endif

    if (!movie_init)
        return ROQ_CLIENT_PROBLEM;

    I_Printf("\nI_PlayMovie: [%s]\n", name);

    // open the file or lump
    epi::file_c *F;

    if (name)
    {
        std::string fn = M_ComposeFileName(game_dir.c_str(), name);

        F = epi::FS_Open(fn.c_str(), epi::file_c::ACCESS_READ | epi::file_c::ACCESS_BINARY);

        if (F)
        {
            I_Printf("Movie player: Opened movie file '%s'\n", fn.c_str());
        }
        else
        {
            // not a file, try a lump
            int lump = W_CheckNumForName(name);
            if (lump < 0)
            {
                M_WarnError("Movie player: Missing file or lump: %s\n", name);
                return ROQ_CLIENT_PROBLEM;
            }

            F = W_OpenLump(lump);
            if (!F)
            {
                M_WarnError("Movie player: Can't open lump: %s\n", name);
                return ROQ_CLIENT_PROBLEM;
            }
            I_Printf("Movie player: Opened movie lump '%s'\n", name);
        }

        int length = F->GetLength();

        // start movie polling thread
        movie_status = 0;
        
		SDL_threadID(movie_thread);

        if(!movie_thread)
        {
            I_Printf ("  Cannot create movie sound daemon!\n");
            delete F;
            return ROQ_NO_MEMORY;
        }

        mbuf = (byte *)malloc(65536);
        if (!mbuf)
        {
            I_Printf("  Cannot allocate read buffer!\n");
            delete F;
            return ROQ_NO_MEMORY;
        }
        msiz = 65536;
		 
#ifdef PROFILE_MOVIE
        num_frames = total_time = chunk_time = cbook_time = 0;
        vqfrm_time = vqpcm_time = stream_time = opengl_time = 0;
#endif

        VID_FLAGS = 0x00;
        AUD_FLAGS = 0x80;
        movie_done = false;
        movie_pause = false;
        playing_movie = true;

        /* process chunks until get 1st audio chunk */
        while (res == ROQ_SUCCESS && AUD_FLAGS == 0x80)
        {
#ifdef PROFILE_MOVIE
            usec = timer_us_gettime64();
#endif
            res = decode_RoQ(mbuf, get_chunk(F, &length));
#ifdef PROFILE_MOVIE
            usec = timer_us_gettime64() - usec;
            total_time += (int)usec;
#endif
        }

        RGL_SetupMatrices2D();

        while (res == ROQ_SUCCESS)
        {
            if ((VID_FLAGS & 1) && (vfc == 2))
            {
#ifdef PROFILE_MOVIE
                usec = timer_us_gettime64();
#endif
                I_StartFrame();

                // upload decoded video into opengl buffer
                glBindTexture(GL_TEXTURE_2D, tex[0]);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                     texture_width, texture_height, 0, GL_RGB,
                     GL_UNSIGNED_SHORT_5_6_5, frames[0]);

                // draw to screen
                glEnable(GL_TEXTURE_2D);
//                glBindTexture(GL_TEXTURE_2D, tex[0]);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glDisable(GL_ALPHA_TEST);

                glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

                glBegin(GL_QUADS);

                glTexCoord2f(0.0f, scale_height);
                glVertex2i(0, 0);

                glTexCoord2f(scale_width, scale_height);
                glVertex2i(SCREENWIDTH, 0);

                glTexCoord2f(scale_width, 0.0f);
                glVertex2i(SCREENWIDTH, SCREENHEIGHT);

                glTexCoord2f(0.0f, 0.0f);
                glVertex2i(0, SCREENHEIGHT);

                glEnd();

                glDisable(GL_TEXTURE_2D);

                I_FinishFrame();
                frames[0] = frames[1];
                vfc--;
                VID_FLAGS = 0;
#ifdef PROFILE_MOVIE
                usec = timer_us_gettime64() - usec;
                opengl_time += (int)usec;
#endif
            }

            if (vfc < 2)
            {
#ifdef PROFILE_MOVIE
                usec = timer_us_gettime64();
#endif
                res = decode_RoQ(mbuf, get_chunk(F, &length));
#ifdef PROFILE_MOVIE
                usec = timer_us_gettime64() - usec;
                total_time += (int)usec;
#endif
            }

            /* check if press pad button */
            //if (HwMdReadPad(0) & (SEGA_CTRL_A | SEGA_CTRL_B | SEGA_CTRL_C))
            //{
            //    res = ROQ_USER_INTERRUPT;
            //    break;
            //}
			//event.value.key.sym = KEYD_ESCAPE;
			if (E_IsKeyPressed(key_fire))
			{
                res = ROQ_USER_INTERRUPT;
                break;
            }

            // allow daemon time
			E_ProcessEvents();

        }
        playing_movie = false;

        // wait for all buttons clear
        //while (HwMdReadPad(0) & SEGA_CTRL_BUTTONS) ;

        delete F;

        // cleanup thread
        if (movie_thread)
        {
            movie_status = 0xDEADBEEF; // DIE, DAEMON! DIE!!
            SDL_Delay(200);
            movie_thread = NULL;
        }

        // cleanup stream
        if(dev = NULL)
        {
            //snd_stream_stop(stream_hnd);
			SDL_CloseAudioDevice(dev);
            dev = NULL;
        }

        // delete OpenGL texture buffer
        if (tex[0])
            glDeleteTextures(1, tex);
        tex[0] = 0;

        // cleanup buffers
        if (frame0)
            free(frame0);
        if (frame1)
            free(frame1);
        if (mbuf)
            free(mbuf);
        if (sbuf)
            free(sbuf);
        frame0 = frame1 = NULL;
        mbuf = sbuf = NULL;
    }
    else
    {
        res = ROQ_CLIENT_PROBLEM;
    }

#ifdef PROFILE_MOVIE
    I_Printf("\n Movie Stats\n");
    I_Printf("   Total Frames:   %d\n", num_frames);
    I_Printf("   Total Time:     %d usec\n", total_time);
    I_Printf("   usec per frame: %d\n\n", total_time / num_frames);
    I_Printf("     get_chunk time: %d\n", chunk_time / num_frames);
    I_Printf("     codebook time:  %d\n", cbook_time / num_frames);
    I_Printf("     vq frame time:  %d\n", vqfrm_time / num_frames);
    I_Printf("     vq audio time:  %d\n", vqpcm_time / num_frames);
    I_Printf("     opengl time:    %d\n", opengl_time / num_frames);
    I_Printf("     stream time:    %d\n\n", stream_time / num_frames);
#endif

    return res;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
