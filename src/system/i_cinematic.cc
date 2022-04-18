//----------------------------------------------------------------------------
//  EDGE Cinematic Playback Engine (ROQ)
//----------------------------------------------------------------------------
//
//  Copyright (c) 2018 The EDGE Team
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
//----------------------------------
//  Based on the Quake 3 Arena source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1996-2005 by id Software, Inc.
//
//  Inspired by cl_cinematic.cpp from OverDose (C) Team Blur Games
//--------------------------------
#ifndef USE_FFMPEG //define this first, in case people want to compile with i_ffmpeg instead of our ROQ cinematic decoder.

#include <signal.h>
#include "i_defs.h"
#include "i_defs_gl.h"
#include "i_sdlinc.h"
#include "i_system.h"
#include "i_sound.h"
#include "i_cinematic.h"

#include "dm_state.h"
#include "con_main.h"
#include "e_input.h"
#include "e_main.h"
#include "m_argv.h"
#include "m_menu.h"
#include "m_misc.h"
#include "r_gldefs.h"
#include "r_image.h"
#include "r_modes.h"
#include "s_blit.h"
#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"


static short cin_cr2rTable[256];
static short cin_cb2gTable[256];
static short cin_cr2gTable[256];
static short cin_cb2bTable[256];

static short cin_sqrTable[256];

// TODO: Check if the GCC-based attribute/aligned is correctly written for Linux users!
#if !defined(_MSC_VER)
static u32_t __attribute__((aligned(16))) cin_quadVQ2[256 << 2];
static u32_t __attribute__((aligned(16))) cin_quadVQ4[256 << 4];
static u32_t __attribute__((aligned(16))) cin_quadVQ8[256 << 6];
#else
// CA: New template (ALIGN_x, resides in i_defs.h [for MSVC users]
ALIGN_16(static u32_t)  cin_quadVQ2[256 << 2];
ALIGN_16(static u32_t)  cin_quadVQ4[256 << 4];
ALIGN_16(static u32_t)  cin_quadVQ8[256 << 6];
#endif

static short cin_soundSamples[ROQ_CHUNK_MAX_DATA_SIZE << 2];

static u8_t cin_chunkData[ROQ_CHUNK_HEADER_SIZE + ROQ_CHUNK_MAX_DATA_SIZE];

static cinematic_t cin_cinematics[MAX_CINEMATICS];

static cinHandle_t current_movie;

bool playing_movie = false;

static SDL_AudioSpec mydev;

extern int au_sfx_volume;
extern float slider_to_gain[];

#define DEBUG_ROQ_READER
#undef DEBUG_ROQ_READER

static void MovieSnd_Callback(void *udata, Uint8 *stream, int len)
{
	CIN_UpdateAudio(stream, len);
}

/*
 ==================
 CIN_TryOpenSound
 ==================
*/
static bool CIN_TryOpenSound(int rate)
{
	SDL_AudioSpec firstdev;
	SDL_zero(firstdev);

	SDL_CloseAudio();

	int samples = 512;
	if (rate < 18000)
		samples = 256;
	else if (rate >= 40000)
		samples = 1024;

	I_Printf("CIN_TryOpenSound: trying %d Hz %s\n",
		rate, "Stereo SP Floating Point");

	firstdev.freq = rate;
	firstdev.format = AUDIO_F32SYS;
	firstdev.channels = 2;
	firstdev.samples = samples;
	firstdev.callback = MovieSnd_Callback;

	mydev_id = SDL_OpenAudioDevice(NULL, 0, &firstdev, &mydev, 0);


	if (mydev_id >= 0)
	if (SDL_OpenAudio(&firstdev, &mydev) >= 0)
	{
		SDL_PauseAudio(0);
		return true;
	}

	I_Printf("CIN_TryOpenSound: failed!\n");
	return false;
}

/*
 ==================
 CIN_BlitBlock2x2
 ==================
*/
static void CIN_BlitBlock2x2(cinematic_t *cin, int x, int y, int index)
{

	u32_t   *src, *dst;

	src = (u32_t *)&cin_quadVQ2[index << 2];
	dst = (u32_t *)cin->frameBuffer[0] + (y * cin->frameWidth + x);

#if defined SIMD_X64

	__m128i xmm[2];

	xmm[0] = _mm_loadl_epi64((const __m128i *)(src + 0));
	xmm[1] = _mm_loadl_epi64((const __m128i *)(src + 2));

	_mm_storel_epi64((__m128i *)(dst + cin->frameWidth * 0), xmm[0]);
	_mm_storel_epi64((__m128i *)(dst + cin->frameWidth * 1), xmm[1]);

#else

	dst[cin->frameWidth * 0 + 0] = src[0];
	dst[cin->frameWidth * 0 + 1] = src[1];
	dst[cin->frameWidth * 1 + 0] = src[2];
	dst[cin->frameWidth * 1 + 1] = src[3];

#endif
}

/*
 ==================
 CIN_BlitBlock4x4
 ==================
*/
static void CIN_BlitBlock4x4(cinematic_t *cin, int x, int y, int index)
{

	u32_t   *src, *dst;

	src = (u32_t *)&cin_quadVQ4[index << 4];
	dst = (u32_t *)cin->frameBuffer[0] + (y * cin->frameWidth + x);

#if defined SIMD_X64

	__m128i xmm[4];

	xmm[0] = _mm_loadu_si128((const __m128i *)(src + 0));
	xmm[1] = _mm_loadu_si128((const __m128i *)(src + 4));
	xmm[2] = _mm_loadu_si128((const __m128i *)(src + 8));
	xmm[3] = _mm_loadu_si128((const __m128i *)(src + 12));

	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 0), xmm[0]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 1), xmm[1]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 2), xmm[2]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 3), xmm[3]);

#else

	dst[cin->frameWidth * 0 + 0] = src[0];
	dst[cin->frameWidth * 0 + 1] = src[1];
	dst[cin->frameWidth * 0 + 2] = src[2];
	dst[cin->frameWidth * 0 + 3] = src[3];
	dst[cin->frameWidth * 1 + 0] = src[4];
	dst[cin->frameWidth * 1 + 1] = src[5];
	dst[cin->frameWidth * 1 + 2] = src[6];
	dst[cin->frameWidth * 1 + 3] = src[7];
	dst[cin->frameWidth * 2 + 0] = src[8];
	dst[cin->frameWidth * 2 + 1] = src[9];
	dst[cin->frameWidth * 2 + 2] = src[10];
	dst[cin->frameWidth * 2 + 3] = src[11];
	dst[cin->frameWidth * 3 + 0] = src[12];
	dst[cin->frameWidth * 3 + 1] = src[13];
	dst[cin->frameWidth * 3 + 2] = src[14];
	dst[cin->frameWidth * 3 + 3] = src[15];

#endif
}

/*
 ==================
 CIN_BlitBlock8x8
 ==================
*/
static void CIN_BlitBlock8x8(cinematic_t *cin, int x, int y, int index) {

	u32_t   *src, *dst;

	src = (u32_t *)&cin_quadVQ8[index << 6];
	dst = (u32_t *)cin->frameBuffer[0] + (y * cin->frameWidth + x);

#if defined SIMD_X64

	__m128i xmm[16];

	xmm[0] = _mm_loadu_si128((const __m128i *)(src + 0));
	xmm[1] = _mm_loadu_si128((const __m128i *)(src + 4));
	xmm[2] = _mm_loadu_si128((const __m128i *)(src + 8));
	xmm[3] = _mm_loadu_si128((const __m128i *)(src + 12));
	xmm[4] = _mm_loadu_si128((const __m128i *)(src + 16));
	xmm[5] = _mm_loadu_si128((const __m128i *)(src + 20));
	xmm[6] = _mm_loadu_si128((const __m128i *)(src + 24));
	xmm[7] = _mm_loadu_si128((const __m128i *)(src + 28));
	xmm[8] = _mm_loadu_si128((const __m128i *)(src + 32));
	xmm[9] = _mm_loadu_si128((const __m128i *)(src + 36));
	xmm[10] = _mm_loadu_si128((const __m128i *)(src + 40));
	xmm[11] = _mm_loadu_si128((const __m128i *)(src + 44));
	xmm[12] = _mm_loadu_si128((const __m128i *)(src + 48));
	xmm[13] = _mm_loadu_si128((const __m128i *)(src + 52));
	xmm[14] = _mm_loadu_si128((const __m128i *)(src + 56));
	xmm[15] = _mm_loadu_si128((const __m128i *)(src + 60));

	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 0 + 0), xmm[0]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 0 + 4), xmm[1]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 1 + 0), xmm[2]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 1 + 4), xmm[3]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 2 + 0), xmm[4]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 2 + 4), xmm[5]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 3 + 0), xmm[6]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 3 + 4), xmm[7]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 4 + 0), xmm[8]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 4 + 4), xmm[9]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 5 + 0), xmm[10]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 5 + 4), xmm[11]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 6 + 0), xmm[12]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 6 + 4), xmm[13]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 7 + 0), xmm[14]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 7 + 4), xmm[15]);

#else

	dst[cin->frameWidth * 0 + 0] = src[0];
	dst[cin->frameWidth * 0 + 1] = src[1];
	dst[cin->frameWidth * 0 + 2] = src[2];
	dst[cin->frameWidth * 0 + 3] = src[3];
	dst[cin->frameWidth * 0 + 4] = src[4];
	dst[cin->frameWidth * 0 + 5] = src[5];
	dst[cin->frameWidth * 0 + 6] = src[6];
	dst[cin->frameWidth * 0 + 7] = src[7];
	dst[cin->frameWidth * 1 + 0] = src[8];
	dst[cin->frameWidth * 1 + 1] = src[9];
	dst[cin->frameWidth * 1 + 2] = src[10];
	dst[cin->frameWidth * 1 + 3] = src[11];
	dst[cin->frameWidth * 1 + 4] = src[12];
	dst[cin->frameWidth * 1 + 5] = src[13];
	dst[cin->frameWidth * 1 + 6] = src[14];
	dst[cin->frameWidth * 1 + 7] = src[15];
	dst[cin->frameWidth * 2 + 0] = src[16];
	dst[cin->frameWidth * 2 + 1] = src[17];
	dst[cin->frameWidth * 2 + 2] = src[18];
	dst[cin->frameWidth * 2 + 3] = src[19];
	dst[cin->frameWidth * 2 + 4] = src[20];
	dst[cin->frameWidth * 2 + 5] = src[21];
	dst[cin->frameWidth * 2 + 6] = src[22];
	dst[cin->frameWidth * 2 + 7] = src[23];
	dst[cin->frameWidth * 3 + 0] = src[24];
	dst[cin->frameWidth * 3 + 1] = src[25];
	dst[cin->frameWidth * 3 + 2] = src[26];
	dst[cin->frameWidth * 3 + 3] = src[27];
	dst[cin->frameWidth * 3 + 4] = src[28];
	dst[cin->frameWidth * 3 + 5] = src[29];
	dst[cin->frameWidth * 3 + 6] = src[30];
	dst[cin->frameWidth * 3 + 7] = src[31];
	dst[cin->frameWidth * 4 + 0] = src[32];
	dst[cin->frameWidth * 4 + 1] = src[33];
	dst[cin->frameWidth * 4 + 2] = src[34];
	dst[cin->frameWidth * 4 + 3] = src[35];
	dst[cin->frameWidth * 4 + 4] = src[36];
	dst[cin->frameWidth * 4 + 5] = src[37];
	dst[cin->frameWidth * 4 + 6] = src[38];
	dst[cin->frameWidth * 4 + 7] = src[39];
	dst[cin->frameWidth * 5 + 0] = src[40];
	dst[cin->frameWidth * 5 + 1] = src[41];
	dst[cin->frameWidth * 5 + 2] = src[42];
	dst[cin->frameWidth * 5 + 3] = src[43];
	dst[cin->frameWidth * 5 + 4] = src[44];
	dst[cin->frameWidth * 5 + 5] = src[45];
	dst[cin->frameWidth * 5 + 6] = src[46];
	dst[cin->frameWidth * 5 + 7] = src[47];
	dst[cin->frameWidth * 6 + 0] = src[48];
	dst[cin->frameWidth * 6 + 1] = src[49];
	dst[cin->frameWidth * 6 + 2] = src[50];
	dst[cin->frameWidth * 6 + 3] = src[51];
	dst[cin->frameWidth * 6 + 4] = src[52];
	dst[cin->frameWidth * 6 + 5] = src[53];
	dst[cin->frameWidth * 6 + 6] = src[54];
	dst[cin->frameWidth * 6 + 7] = src[55];
	dst[cin->frameWidth * 7 + 0] = src[56];
	dst[cin->frameWidth * 7 + 1] = src[57];
	dst[cin->frameWidth * 7 + 2] = src[58];
	dst[cin->frameWidth * 7 + 3] = src[59];
	dst[cin->frameWidth * 7 + 4] = src[60];
	dst[cin->frameWidth * 7 + 5] = src[61];
	dst[cin->frameWidth * 7 + 6] = src[62];
	dst[cin->frameWidth * 7 + 7] = src[63];

#endif
}

/*
 ==================
 CIN_MoveBlock4x4
 ==================
*/
static void CIN_MoveBlock4x4(cinematic_t *cin, int x, int y, int xMean, int yMean, int xyMove) {

	u32_t   *src, *dst;
	int     xMot, yMot;

	xMot = x + xMean - (xyMove >> 4);
	yMot = y + yMean - (xyMove & 15);

	src = (u32_t *)cin->frameBuffer[1] + (yMot * cin->frameWidth + xMot);
	dst = (u32_t *)cin->frameBuffer[0] + (y * cin->frameWidth + x);

#if defined SIMD_X64

	__m128i xmm[4];

	xmm[0] = _mm_loadu_si128((const __m128i *)(src + cin->frameWidth * 0));
	xmm[1] = _mm_loadu_si128((const __m128i *)(src + cin->frameWidth * 1));
	xmm[2] = _mm_loadu_si128((const __m128i *)(src + cin->frameWidth * 2));
	xmm[3] = _mm_loadu_si128((const __m128i *)(src + cin->frameWidth * 3));

	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 0), xmm[0]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 1), xmm[1]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 2), xmm[2]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 3), xmm[3]);

#else

	dst[cin->frameWidth * 0 + 0] = src[cin->frameWidth * 0 + 0];
	dst[cin->frameWidth * 0 + 1] = src[cin->frameWidth * 0 + 1];
	dst[cin->frameWidth * 0 + 2] = src[cin->frameWidth * 0 + 2];
	dst[cin->frameWidth * 0 + 3] = src[cin->frameWidth * 0 + 3];
	dst[cin->frameWidth * 1 + 0] = src[cin->frameWidth * 1 + 0];
	dst[cin->frameWidth * 1 + 1] = src[cin->frameWidth * 1 + 1];
	dst[cin->frameWidth * 1 + 2] = src[cin->frameWidth * 1 + 2];
	dst[cin->frameWidth * 1 + 3] = src[cin->frameWidth * 1 + 3];
	dst[cin->frameWidth * 2 + 0] = src[cin->frameWidth * 2 + 0];
	dst[cin->frameWidth * 2 + 1] = src[cin->frameWidth * 2 + 1];
	dst[cin->frameWidth * 2 + 2] = src[cin->frameWidth * 2 + 2];
	dst[cin->frameWidth * 2 + 3] = src[cin->frameWidth * 2 + 3];
	dst[cin->frameWidth * 3 + 0] = src[cin->frameWidth * 3 + 0];
	dst[cin->frameWidth * 3 + 1] = src[cin->frameWidth * 3 + 1];
	dst[cin->frameWidth * 3 + 2] = src[cin->frameWidth * 3 + 2];
	dst[cin->frameWidth * 3 + 3] = src[cin->frameWidth * 3 + 3];

#endif
}

/*
 ==================
 CIN_MoveBlock8x8
 ==================
*/
static void CIN_MoveBlock8x8(cinematic_t *cin, int x, int y, int xMean, int yMean, int xyMove) {

	u32_t   *src, *dst;
	int     xMot, yMot;

	xMot = x + xMean - (xyMove >> 4);
	yMot = y + yMean - (xyMove & 15);

	src = (u32_t *)cin->frameBuffer[1] + (yMot * cin->frameWidth + xMot);
	dst = (u32_t *)cin->frameBuffer[0] + (y * cin->frameWidth + x);

#if defined SIMD_X64

	__m128i xmm[16];

	xmm[0] = _mm_loadu_si128((const __m128i *)(src + cin->frameWidth * 0 + 0));
	xmm[1] = _mm_loadu_si128((const __m128i *)(src + cin->frameWidth * 0 + 4));
	xmm[2] = _mm_loadu_si128((const __m128i *)(src + cin->frameWidth * 1 + 0));
	xmm[3] = _mm_loadu_si128((const __m128i *)(src + cin->frameWidth * 1 + 4));
	xmm[4] = _mm_loadu_si128((const __m128i *)(src + cin->frameWidth * 2 + 0));
	xmm[5] = _mm_loadu_si128((const __m128i *)(src + cin->frameWidth * 2 + 4));
	xmm[6] = _mm_loadu_si128((const __m128i *)(src + cin->frameWidth * 3 + 0));
	xmm[7] = _mm_loadu_si128((const __m128i *)(src + cin->frameWidth * 3 + 4));
	xmm[8] = _mm_loadu_si128((const __m128i *)(src + cin->frameWidth * 4 + 0));
	xmm[9] = _mm_loadu_si128((const __m128i *)(src + cin->frameWidth * 4 + 4));
	xmm[10] = _mm_loadu_si128((const __m128i *)(src + cin->frameWidth * 5 + 0));
	xmm[11] = _mm_loadu_si128((const __m128i *)(src + cin->frameWidth * 5 + 4));
	xmm[12] = _mm_loadu_si128((const __m128i *)(src + cin->frameWidth * 6 + 0));
	xmm[13] = _mm_loadu_si128((const __m128i *)(src + cin->frameWidth * 6 + 4));
	xmm[14] = _mm_loadu_si128((const __m128i *)(src + cin->frameWidth * 7 + 0));
	xmm[15] = _mm_loadu_si128((const __m128i *)(src + cin->frameWidth * 7 + 4));

	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 0 + 0), xmm[0]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 0 + 4), xmm[1]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 1 + 0), xmm[2]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 1 + 4), xmm[3]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 2 + 0), xmm[4]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 2 + 4), xmm[5]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 3 + 0), xmm[6]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 3 + 4), xmm[7]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 4 + 0), xmm[8]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 4 + 4), xmm[9]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 5 + 0), xmm[10]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 5 + 4), xmm[11]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 6 + 0), xmm[12]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 6 + 4), xmm[13]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 7 + 0), xmm[14]);
	_mm_storeu_si128((__m128i *)(dst + cin->frameWidth * 7 + 4), xmm[15]);

#else

	dst[cin->frameWidth * 0 + 0] = src[cin->frameWidth * 0 + 0];
	dst[cin->frameWidth * 0 + 1] = src[cin->frameWidth * 0 + 1];
	dst[cin->frameWidth * 0 + 2] = src[cin->frameWidth * 0 + 2];
	dst[cin->frameWidth * 0 + 3] = src[cin->frameWidth * 0 + 3];
	dst[cin->frameWidth * 0 + 4] = src[cin->frameWidth * 0 + 4];
	dst[cin->frameWidth * 0 + 5] = src[cin->frameWidth * 0 + 5];
	dst[cin->frameWidth * 0 + 6] = src[cin->frameWidth * 0 + 6];
	dst[cin->frameWidth * 0 + 7] = src[cin->frameWidth * 0 + 7];
	dst[cin->frameWidth * 1 + 0] = src[cin->frameWidth * 1 + 0];
	dst[cin->frameWidth * 1 + 1] = src[cin->frameWidth * 1 + 1];
	dst[cin->frameWidth * 1 + 2] = src[cin->frameWidth * 1 + 2];
	dst[cin->frameWidth * 1 + 3] = src[cin->frameWidth * 1 + 3];
	dst[cin->frameWidth * 1 + 4] = src[cin->frameWidth * 1 + 4];
	dst[cin->frameWidth * 1 + 5] = src[cin->frameWidth * 1 + 5];
	dst[cin->frameWidth * 1 + 6] = src[cin->frameWidth * 1 + 6];
	dst[cin->frameWidth * 1 + 7] = src[cin->frameWidth * 1 + 7];
	dst[cin->frameWidth * 2 + 0] = src[cin->frameWidth * 2 + 0];
	dst[cin->frameWidth * 2 + 1] = src[cin->frameWidth * 2 + 1];
	dst[cin->frameWidth * 2 + 2] = src[cin->frameWidth * 2 + 2];
	dst[cin->frameWidth * 2 + 3] = src[cin->frameWidth * 2 + 3];
	dst[cin->frameWidth * 2 + 4] = src[cin->frameWidth * 2 + 4];
	dst[cin->frameWidth * 2 + 5] = src[cin->frameWidth * 2 + 5];
	dst[cin->frameWidth * 2 + 6] = src[cin->frameWidth * 2 + 6];
	dst[cin->frameWidth * 2 + 7] = src[cin->frameWidth * 2 + 7];
	dst[cin->frameWidth * 3 + 0] = src[cin->frameWidth * 3 + 0];
	dst[cin->frameWidth * 3 + 1] = src[cin->frameWidth * 3 + 1];
	dst[cin->frameWidth * 3 + 2] = src[cin->frameWidth * 3 + 2];
	dst[cin->frameWidth * 3 + 3] = src[cin->frameWidth * 3 + 3];
	dst[cin->frameWidth * 3 + 4] = src[cin->frameWidth * 3 + 4];
	dst[cin->frameWidth * 3 + 5] = src[cin->frameWidth * 3 + 5];
	dst[cin->frameWidth * 3 + 6] = src[cin->frameWidth * 3 + 6];
	dst[cin->frameWidth * 3 + 7] = src[cin->frameWidth * 3 + 7];
	dst[cin->frameWidth * 4 + 0] = src[cin->frameWidth * 4 + 0];
	dst[cin->frameWidth * 4 + 1] = src[cin->frameWidth * 4 + 1];
	dst[cin->frameWidth * 4 + 2] = src[cin->frameWidth * 4 + 2];
	dst[cin->frameWidth * 4 + 3] = src[cin->frameWidth * 4 + 3];
	dst[cin->frameWidth * 4 + 4] = src[cin->frameWidth * 4 + 4];
	dst[cin->frameWidth * 4 + 5] = src[cin->frameWidth * 4 + 5];
	dst[cin->frameWidth * 4 + 6] = src[cin->frameWidth * 4 + 6];
	dst[cin->frameWidth * 4 + 7] = src[cin->frameWidth * 4 + 7];
	dst[cin->frameWidth * 5 + 0] = src[cin->frameWidth * 5 + 0];
	dst[cin->frameWidth * 5 + 1] = src[cin->frameWidth * 5 + 1];
	dst[cin->frameWidth * 5 + 2] = src[cin->frameWidth * 5 + 2];
	dst[cin->frameWidth * 5 + 3] = src[cin->frameWidth * 5 + 3];
	dst[cin->frameWidth * 5 + 4] = src[cin->frameWidth * 5 + 4];
	dst[cin->frameWidth * 5 + 5] = src[cin->frameWidth * 5 + 5];
	dst[cin->frameWidth * 5 + 6] = src[cin->frameWidth * 5 + 6];
	dst[cin->frameWidth * 5 + 7] = src[cin->frameWidth * 5 + 7];
	dst[cin->frameWidth * 6 + 0] = src[cin->frameWidth * 6 + 0];
	dst[cin->frameWidth * 6 + 1] = src[cin->frameWidth * 6 + 1];
	dst[cin->frameWidth * 6 + 2] = src[cin->frameWidth * 6 + 2];
	dst[cin->frameWidth * 6 + 3] = src[cin->frameWidth * 6 + 3];
	dst[cin->frameWidth * 6 + 4] = src[cin->frameWidth * 6 + 4];
	dst[cin->frameWidth * 6 + 5] = src[cin->frameWidth * 6 + 5];
	dst[cin->frameWidth * 6 + 6] = src[cin->frameWidth * 6 + 6];
	dst[cin->frameWidth * 6 + 7] = src[cin->frameWidth * 6 + 7];
	dst[cin->frameWidth * 7 + 0] = src[cin->frameWidth * 7 + 0];
	dst[cin->frameWidth * 7 + 1] = src[cin->frameWidth * 7 + 1];
	dst[cin->frameWidth * 7 + 2] = src[cin->frameWidth * 7 + 2];
	dst[cin->frameWidth * 7 + 3] = src[cin->frameWidth * 7 + 3];
	dst[cin->frameWidth * 7 + 4] = src[cin->frameWidth * 7 + 4];
	dst[cin->frameWidth * 7 + 5] = src[cin->frameWidth * 7 + 5];
	dst[cin->frameWidth * 7 + 6] = src[cin->frameWidth * 7 + 6];
	dst[cin->frameWidth * 7 + 7] = src[cin->frameWidth * 7 + 7];

#endif
}

/*
 ==================
 CIN_DecodeQuadInfo
 ==================
*/
static void CIN_DecodeQuadInfo(cinematic_t *cin, const byte *data) {

	if (cin->frameBuffer[0] && cin->frameBuffer[1])
		return;

	// Allocate the frame buffers
	cin->frameWidth = data[0] | (data[1] << 8);
	cin->frameHeight = data[2] | (data[3] << 8);
	I_Printf("Cinematic dimensions: %dx%d\n", cin->frameWidth, cin->frameHeight);

	if ((cin->frameWidth & 15) || (cin->frameHeight & 15))
		I_Error("CIN_DecodeQuadInfo: video dimensions not divisible by 16");

	cin->frameBuffer[0] = (byte *)Z_Malloc(cin->frameWidth * cin->frameHeight * 4);// , TAG_COMMON);
	cin->frameBuffer[1] = (byte *)Z_Malloc(cin->frameWidth * cin->frameHeight * 4);// , TAG_COMMON);
}

/*
 ==================
 CIN_DecodeQuadCodebook
 ==================
*/
static void CIN_DecodeQuadCodebook(cinematic_t *cin, const byte *data) {

	u32_t   *quadVQ2, *quadVQ4, *quadVQ8;
	int     index0, index1, index2, index3;
	int     numQuadVecs;
	int     numQuadCels;
	int     i;

	if (!cin->chunkHeader.args) {
		numQuadVecs = 256;
		numQuadCels = 256;
	}
	else {
		numQuadVecs = (cin->chunkHeader.args >> 8) & 255;
		numQuadCels = (cin->chunkHeader.args >> 0) & 255;

		if (!numQuadVecs)
			numQuadVecs = 256;
	}

#if defined SIMD_X64

	__m128i xmmRGBA, xmmPixels[2];
	__m128i xmmVQ2[4], xmmVQ4[4], xmmVQ8[8];

	// Convert 2x2 pixel vectors from YCbCr to RGB
	for (i = 0; i < numQuadVecs; i++) {
		quadVQ2 = &cin_quadVQ2[i << 2];

		xmmRGBA = _mm_set_epi32(255, cin_cb2bTable[data[4]], cin_cb2gTable[data[4]] + cin_cr2gTable[data[5]], cin_cr2rTable[data[5]]);
		xmmRGBA = _mm_packs_epi32(xmmRGBA, xmmRGBA);

		xmmPixels[0] = _mm_add_epi16(xmmRGBA, _mm_packs_epi32(_mm_set1_epi32(data[0]), _mm_set1_epi32(data[1])));
		xmmPixels[1] = _mm_add_epi16(xmmRGBA, _mm_packs_epi32(_mm_set1_epi32(data[2]), _mm_set1_epi32(data[3])));

		_mm_store_si128((__m128i *)quadVQ2, _mm_packus_epi16(xmmPixels[0], xmmPixels[1]));

		data += 6;
	}

	// Set up 4x4 and 8x8 pixel vectors
	for (i = 0; i < numQuadCels; i++) {
		index0 = data[0] << 2;
		index1 = data[1] << 2;
		index2 = data[2] << 2;
		index3 = data[3] << 2;

		quadVQ4 = &cin_quadVQ4[i << 4];
		quadVQ8 = &cin_quadVQ8[i << 6];

		xmmVQ2[0] = _mm_load_si128((const __m128i *)(cin_quadVQ2 + index0));
		xmmVQ2[1] = _mm_load_si128((const __m128i *)(cin_quadVQ2 + index1));
		xmmVQ2[2] = _mm_load_si128((const __m128i *)(cin_quadVQ2 + index2));
		xmmVQ2[3] = _mm_load_si128((const __m128i *)(cin_quadVQ2 + index3));

		// Set up 4x4 pixel vectors
		xmmVQ4[0] = _mm_unpacklo_epi64(xmmVQ2[0], xmmVQ2[1]);
		xmmVQ4[1] = _mm_unpackhi_epi64(xmmVQ2[0], xmmVQ2[1]);
		xmmVQ4[2] = _mm_unpacklo_epi64(xmmVQ2[2], xmmVQ2[3]);
		xmmVQ4[3] = _mm_unpackhi_epi64(xmmVQ2[2], xmmVQ2[3]);

		_mm_store_si128((__m128i *)(quadVQ4 + 0), xmmVQ4[0]);
		_mm_store_si128((__m128i *)(quadVQ4 + 4), xmmVQ4[1]);
		_mm_store_si128((__m128i *)(quadVQ4 + 8), xmmVQ4[2]);
		_mm_store_si128((__m128i *)(quadVQ4 + 12), xmmVQ4[3]);

		// Set up 8x8 pixel vectors
		xmmVQ8[0] = _mm_unpacklo_epi32(xmmVQ2[0], xmmVQ2[0]);
		xmmVQ8[1] = _mm_unpackhi_epi32(xmmVQ2[0], xmmVQ2[0]);
		xmmVQ8[2] = _mm_unpacklo_epi32(xmmVQ2[1], xmmVQ2[1]);
		xmmVQ8[3] = _mm_unpackhi_epi32(xmmVQ2[1], xmmVQ2[1]);
		xmmVQ8[4] = _mm_unpacklo_epi32(xmmVQ2[2], xmmVQ2[2]);
		xmmVQ8[5] = _mm_unpackhi_epi32(xmmVQ2[2], xmmVQ2[2]);
		xmmVQ8[6] = _mm_unpacklo_epi32(xmmVQ2[3], xmmVQ2[3]);
		xmmVQ8[7] = _mm_unpackhi_epi32(xmmVQ2[3], xmmVQ2[3]);

		_mm_store_si128((__m128i *)(quadVQ8 + 0), xmmVQ8[0]);
		_mm_store_si128((__m128i *)(quadVQ8 + 4), xmmVQ8[2]);
		_mm_store_si128((__m128i *)(quadVQ8 + 8), xmmVQ8[0]);
		_mm_store_si128((__m128i *)(quadVQ8 + 12), xmmVQ8[2]);
		_mm_store_si128((__m128i *)(quadVQ8 + 16), xmmVQ8[1]);
		_mm_store_si128((__m128i *)(quadVQ8 + 20), xmmVQ8[3]);
		_mm_store_si128((__m128i *)(quadVQ8 + 24), xmmVQ8[1]);
		_mm_store_si128((__m128i *)(quadVQ8 + 28), xmmVQ8[3]);
		_mm_store_si128((__m128i *)(quadVQ8 + 32), xmmVQ8[4]);
		_mm_store_si128((__m128i *)(quadVQ8 + 36), xmmVQ8[6]);
		_mm_store_si128((__m128i *)(quadVQ8 + 40), xmmVQ8[4]);
		_mm_store_si128((__m128i *)(quadVQ8 + 44), xmmVQ8[6]);
		_mm_store_si128((__m128i *)(quadVQ8 + 48), xmmVQ8[5]);
		_mm_store_si128((__m128i *)(quadVQ8 + 52), xmmVQ8[7]);
		_mm_store_si128((__m128i *)(quadVQ8 + 56), xmmVQ8[5]);
		_mm_store_si128((__m128i *)(quadVQ8 + 60), xmmVQ8[7]);

		data += 4;
	}

#else

	int     r, g, b;
	byte    pixels[4][4];

	// Convert 2x2 pixel vectors from YCbCr to RGB
	for (i = 0; i < numQuadVecs; i++) {
		quadVQ2 = &cin_quadVQ2[i << 2];

		r = cin_cr2rTable[data[5]];
		g = cin_cb2gTable[data[4]] + cin_cr2gTable[data[5]];
		b = cin_cb2bTable[data[4]];

		pixels[0][0] = epi::ClampByte(r + data[0]);
		pixels[0][1] = epi::ClampByte(g + data[0]);
		pixels[0][2] = epi::ClampByte(b + data[0]);
		pixels[0][3] = 255;

		pixels[1][0] = epi::ClampByte(r + data[1]);
		pixels[1][1] = epi::ClampByte(g + data[1]);
		pixels[1][2] = epi::ClampByte(b + data[1]);
		pixels[1][3] = 255;

		pixels[2][0] = epi::ClampByte(r + data[2]);
		pixels[2][1] = epi::ClampByte(g + data[2]);
		pixels[2][2] = epi::ClampByte(b + data[2]);
		pixels[2][3] = 255;

		pixels[3][0] = epi::ClampByte(r + data[3]);
		pixels[3][1] = epi::ClampByte(g + data[3]);
		pixels[3][2] = epi::ClampByte(b + data[3]);
		pixels[3][3] = 255;

		quadVQ2[0] = *(u32_t *)(&pixels[0]);
		quadVQ2[1] = *(u32_t *)(&pixels[1]);
		quadVQ2[2] = *(u32_t *)(&pixels[2]);
		quadVQ2[3] = *(u32_t *)(&pixels[3]);

		data += 6;
	}

	// Set up 4x4 and 8x8 pixel vectors
	for (i = 0; i < numQuadCels; i++) {
		index0 = data[0] << 2;
		index1 = data[1] << 2;
		index2 = data[2] << 2;
		index3 = data[3] << 2;

		quadVQ4 = &cin_quadVQ4[i << 4];
		quadVQ8 = &cin_quadVQ8[i << 6];

		// Set up 4x4 pixel vectors
		quadVQ4[0] = cin_quadVQ2[index0 + 0];
		quadVQ4[1] = cin_quadVQ2[index0 + 1];
		quadVQ4[2] = cin_quadVQ2[index1 + 0];
		quadVQ4[3] = cin_quadVQ2[index1 + 1];
		quadVQ4[4] = cin_quadVQ2[index0 + 2];
		quadVQ4[5] = cin_quadVQ2[index0 + 3];
		quadVQ4[6] = cin_quadVQ2[index1 + 2];
		quadVQ4[7] = cin_quadVQ2[index1 + 3];
		quadVQ4[8] = cin_quadVQ2[index2 + 0];
		quadVQ4[9] = cin_quadVQ2[index2 + 1];
		quadVQ4[10] = cin_quadVQ2[index3 + 0];
		quadVQ4[11] = cin_quadVQ2[index3 + 1];
		quadVQ4[12] = cin_quadVQ2[index2 + 2];
		quadVQ4[13] = cin_quadVQ2[index2 + 3];
		quadVQ4[14] = cin_quadVQ2[index3 + 2];
		quadVQ4[15] = cin_quadVQ2[index3 + 3];

		// Set up 8x8 pixel vectors
		quadVQ8[0] = cin_quadVQ2[index0 + 0];
		quadVQ8[1] = cin_quadVQ2[index0 + 0];
		quadVQ8[2] = cin_quadVQ2[index0 + 1];
		quadVQ8[3] = cin_quadVQ2[index0 + 1];
		quadVQ8[4] = cin_quadVQ2[index1 + 0];
		quadVQ8[5] = cin_quadVQ2[index1 + 0];
		quadVQ8[6] = cin_quadVQ2[index1 + 1];
		quadVQ8[7] = cin_quadVQ2[index1 + 1];
		quadVQ8[8] = cin_quadVQ2[index0 + 0];
		quadVQ8[9] = cin_quadVQ2[index0 + 0];
		quadVQ8[10] = cin_quadVQ2[index0 + 1];
		quadVQ8[11] = cin_quadVQ2[index0 + 1];
		quadVQ8[12] = cin_quadVQ2[index1 + 0];
		quadVQ8[13] = cin_quadVQ2[index1 + 0];
		quadVQ8[14] = cin_quadVQ2[index1 + 1];
		quadVQ8[15] = cin_quadVQ2[index1 + 1];
		quadVQ8[16] = cin_quadVQ2[index0 + 2];
		quadVQ8[17] = cin_quadVQ2[index0 + 2];
		quadVQ8[18] = cin_quadVQ2[index0 + 3];
		quadVQ8[19] = cin_quadVQ2[index0 + 3];
		quadVQ8[20] = cin_quadVQ2[index1 + 2];
		quadVQ8[21] = cin_quadVQ2[index1 + 2];
		quadVQ8[22] = cin_quadVQ2[index1 + 3];
		quadVQ8[23] = cin_quadVQ2[index1 + 3];
		quadVQ8[24] = cin_quadVQ2[index0 + 2];
		quadVQ8[25] = cin_quadVQ2[index0 + 2];
		quadVQ8[26] = cin_quadVQ2[index0 + 3];
		quadVQ8[27] = cin_quadVQ2[index0 + 3];
		quadVQ8[28] = cin_quadVQ2[index1 + 2];
		quadVQ8[29] = cin_quadVQ2[index1 + 2];
		quadVQ8[30] = cin_quadVQ2[index1 + 3];
		quadVQ8[31] = cin_quadVQ2[index1 + 3];
		quadVQ8[32] = cin_quadVQ2[index2 + 0];
		quadVQ8[33] = cin_quadVQ2[index2 + 0];
		quadVQ8[34] = cin_quadVQ2[index2 + 1];
		quadVQ8[35] = cin_quadVQ2[index2 + 1];
		quadVQ8[36] = cin_quadVQ2[index3 + 0];
		quadVQ8[37] = cin_quadVQ2[index3 + 0];
		quadVQ8[38] = cin_quadVQ2[index3 + 1];
		quadVQ8[39] = cin_quadVQ2[index3 + 1];
		quadVQ8[40] = cin_quadVQ2[index2 + 0];
		quadVQ8[41] = cin_quadVQ2[index2 + 0];
		quadVQ8[42] = cin_quadVQ2[index2 + 1];
		quadVQ8[43] = cin_quadVQ2[index2 + 1];
		quadVQ8[44] = cin_quadVQ2[index3 + 0];
		quadVQ8[45] = cin_quadVQ2[index3 + 0];
		quadVQ8[46] = cin_quadVQ2[index3 + 1];
		quadVQ8[47] = cin_quadVQ2[index3 + 1];
		quadVQ8[48] = cin_quadVQ2[index2 + 2];
		quadVQ8[49] = cin_quadVQ2[index2 + 2];
		quadVQ8[50] = cin_quadVQ2[index2 + 3];
		quadVQ8[51] = cin_quadVQ2[index2 + 3];
		quadVQ8[52] = cin_quadVQ2[index3 + 2];
		quadVQ8[53] = cin_quadVQ2[index3 + 2];
		quadVQ8[54] = cin_quadVQ2[index3 + 3];
		quadVQ8[55] = cin_quadVQ2[index3 + 3];
		quadVQ8[56] = cin_quadVQ2[index2 + 2];
		quadVQ8[57] = cin_quadVQ2[index2 + 2];
		quadVQ8[58] = cin_quadVQ2[index2 + 3];
		quadVQ8[59] = cin_quadVQ2[index2 + 3];
		quadVQ8[60] = cin_quadVQ2[index3 + 2];
		quadVQ8[61] = cin_quadVQ2[index3 + 2];
		quadVQ8[62] = cin_quadVQ2[index3 + 3];
		quadVQ8[63] = cin_quadVQ2[index3 + 3];

		data += 4;
	}

#endif
}

/*
 ==================
 CIN_DecodeQuadVQ
 ==================
*/
static void CIN_DecodeQuadVQ(cinematic_t *cin, const byte *data) {

	int     code, codeBits, codeWord;
	int     xPos, yPos, xOfs, yOfs;
	int     xMean, yMean;
	int     x = 0, y = 0;
	int     i;

	if (!cin->frameBuffer[0] || !cin->frameBuffer[1])
		I_Error("CIN_DecodeQuadVQ: video buffers not allocated");

	codeBits = 0;
	codeWord = 0;

	xMean = 8 - (char)((cin->chunkHeader.args >> 8) & 255);
	yMean = 8 - (char)((cin->chunkHeader.args >> 0) & 255);

	while (1) {
		// Subdivide the 16x16 pixel macro block into 8x8 pixel blocks
		for (yPos = y; yPos < y + 16; yPos += 8) {
			for (xPos = x; xPos < x + 16; xPos += 8) {
				// Decode
				if (!codeBits) {
					codeBits = 16;
					codeWord = data[0] | (data[1] << 8);

					data += 2;
				}

				code = codeWord & 0xC000;

				codeBits -= 2;
				codeWord <<= 2;

				switch (code) {
				case ROQ_VQ_MOT:
					// Skip over block

					break;
				case ROQ_VQ_FCC:
					// Motion compensation
					CIN_MoveBlock8x8(cin, xPos, yPos, xMean, yMean, *data);

					data += 1;

					break;
				case ROQ_VQ_SLD:
					// Vector quantization
					CIN_BlitBlock8x8(cin, xPos, yPos, *data);

					data += 1;

					break;
				case ROQ_VQ_CCC:
					// Subdivide the 8x8 pixel block into 4x4 pixel sub blocks
					for (i = 0; i < 4; i++) {
						xOfs = xPos + 4 * (i & 1);
						yOfs = yPos + 4 * (i >> 1);

						// Decode
						if (!codeBits) {
							codeBits = 16;
							codeWord = data[0] | (data[1] << 8);

							data += 2;
						}

						code = codeWord & 0xC000;

						codeBits -= 2;
						codeWord <<= 2;

						switch (code) {
						case ROQ_VQ_MOT:
							// Skip over block

							break;
						case ROQ_VQ_FCC:
							// Motion compensation
							CIN_MoveBlock4x4(cin, xOfs, yOfs, xMean, yMean, *data);

							data += 1;

							break;
						case ROQ_VQ_SLD:
							// Vector quantization
							CIN_BlitBlock4x4(cin, xOfs, yOfs, *data);

							data += 1;

							break;
						case ROQ_VQ_CCC:
							// Vector quantization in 2x2 pixel sub blocks
							CIN_BlitBlock2x2(cin, xOfs + 0, yOfs + 0, data[0]);
							CIN_BlitBlock2x2(cin, xOfs + 2, yOfs + 0, data[1]);
							CIN_BlitBlock2x2(cin, xOfs + 0, yOfs + 2, data[2]);
							CIN_BlitBlock2x2(cin, xOfs + 2, yOfs + 2, data[3]);

							data += 4;

							break;
						default:
							I_Error("CIN_DecodeQuadVQ: bad code (%i)", code);
						}
					}

					break;
				default:
					I_Error("CIN_DecodeQuadVQ: bad code (%i)", code);
				}
			}
		}

		// Go to the next 16x16 pixel macro block
		x += 16;
		if (x == cin->frameWidth) {
			x = 0;

			y += 16;
			if (y == cin->frameHeight)
				break;
		}
	}

	// Bump frame count
	cin->frameCount++;

	// swap the frame buffers
	byte *tmp = cin->frameBuffer[0];
	cin->frameBuffer[0] = cin->frameBuffer[1];
	cin->frameBuffer[1] = tmp;
}

/*
 ==================
 CIN_DecodeSoundMono22
 ==================
*/
static void CIN_DecodeSoundMono22(cinematic_t *cin, const byte *data) {

	short   prev;
	int     i;

	if (cin->flags & CIN_SILENT)
		return;

	// Decode the sound samples
	prev = (short)cin->chunkHeader.args;

	for (i = 0; i < cin->chunkHeader.size; i++) {
		prev += cin_sqrTable[data[i]];

		cin->soundSamples[cin->nextSample] = prev;
		cin->soundSamples[cin->nextSample + 1] = prev;
		cin->nextSample += 2;
	}
	if (cin->sampleRate != 22050)
		if (CIN_TryOpenSound(22050))
			cin->sampleRate = 22050;
}

/*
 ==================
 CIN_DecodeSoundStereo22
 ==================
*/
static void CIN_DecodeSoundStereo22(cinematic_t *cin, const byte *data) {

	short   prevL, prevR;
	int     i;

	if (cin->flags & CIN_SILENT)
		return;

	// Decode the sound samples
	prevL = (short)((cin->chunkHeader.args & 0xFF00) << 0);
	prevR = (short)((cin->chunkHeader.args & 0x00FF) << 8);

	for (i = 0; i < cin->chunkHeader.size; i += 2) {
		prevL += cin_sqrTable[data[i + 0]];
		prevR += cin_sqrTable[data[i + 1]];

		cin->soundSamples[cin->nextSample] = prevL;
		cin->soundSamples[cin->nextSample + 1] = prevR;
		cin->nextSample += 2;
	}
	if (cin->sampleRate != 22050)
		if (CIN_TryOpenSound(22050))
			cin->sampleRate = 22050;
}

/*
 ==================
 CIN_DecodeSoundMono48
 ==================
*/
static void CIN_DecodeSoundMono48(cinematic_t *cin, const byte *data) {

	short   samp;
	int     i;

	if (cin->flags & CIN_SILENT)
		return;

	// Decode the sound samples
	for (i = 0; i < cin->chunkHeader.size; i += 2) {
		samp = (short)(data[i] | (data[i + 1] << 8));

		cin->soundSamples[cin->nextSample] = samp;
		cin->soundSamples[cin->nextSample + 1] = samp;
		cin->nextSample += 2;
	}
	if (cin->sampleRate != 48000)
		if (CIN_TryOpenSound(48000))
			cin->sampleRate = 48000;
}

/*
 ==================
 CIN_DecodeSoundStereo48
 ==================
*/
static void CIN_DecodeSoundStereo48(cinematic_t *cin, const byte *data) {

	short   sampL, sampR;
	int     i;

	if (cin->flags & CIN_SILENT)
		return;

	// Decode the sound samples
	for (i = 0; i < cin->chunkHeader.size; i += 4) {
		sampL = (short)(data[i + 0] | (data[i + 1] << 8));
		sampR = (short)(data[i + 2] | (data[i + 3] << 8));

		cin->soundSamples[cin->nextSample] = sampL;
		cin->soundSamples[cin->nextSample + 1] = sampR;
		cin->nextSample += 2;
	}
	cin->sampleRate = 48000;
}

/*
 ==================
 CIN_DecodeChunk
 ==================
*/
static bool CIN_DecodeChunk(cinematic_t *cin)
{
#if DEBUG_ROQ_READER
	I_Printf("CIN_DecodeChunk!!\n");
#endif
	//epi::file_c *f = cin->file;
	byte    *data;

	if (cin->offset >= cin->size)
	{
#if DEBUG_ROQ_READER
		I_Printf("  Out of data!\n");
#endif
		cin->frameCount = 0;
		return false;   // Finished
	}

	data = cin_chunkData;

	// Read and decode the first chunk header if needed
	if (cin->offset == ROQ_CHUNK_HEADER_SIZE)
	{
#if DEBUG_ROQ_READER
		I_Printf("  offset 0x%08x\n", cin->offset);
#endif
		cin->offset += cin->file->Read(cin_chunkData, ROQ_CHUNK_HEADER_SIZE);
		cin->chunkHeader.id = EPI_LE_U16(data[0] | (data[1] << 8));
		cin->chunkHeader.size = EPI_LE_U32(data[2] | (data[3] << 8) | (data[4] << 16) | (data[5] << 24));
		cin->chunkHeader.args = EPI_LE_U16(data[6] | (data[7] << 8));
#if DEBUG_ROQ_READER
		I_Printf("    CHDR: 0x%04x %d 0x%04x\n", cin->chunkHeader.id, cin->chunkHeader.size, cin->chunkHeader.args);
#endif
	}

	// Read the chunk data and the next chunk header
	if (cin->chunkHeader.size > ROQ_CHUNK_MAX_DATA_SIZE)
		I_Error("CIN_DecodeChunk: bad chunk size (%u)", cin->chunkHeader.size);

	if (cin->offset + cin->chunkHeader.size >= cin->size)
		cin->offset += cin->file->Read(cin_chunkData, cin->chunkHeader.size);
	else
		cin->offset += cin->file->Read(cin_chunkData, cin->chunkHeader.size + ROQ_CHUNK_HEADER_SIZE);

	// Decode the chunk data
	switch (cin->chunkHeader.id)
	{
	case ROQ_QUAD_INFO:
		CIN_DecodeQuadInfo(cin, data);
		break;
	case ROQ_QUAD_CODEBOOK:
		CIN_DecodeQuadCodebook(cin, data);
		break;
	case ROQ_QUAD_VQ:
		CIN_DecodeQuadVQ(cin, data);
		break;
	case ROQ_SOUND_MONO_22:
		CIN_DecodeSoundMono22(cin, data);
		break;
	case ROQ_SOUND_STEREO_22:
		CIN_DecodeSoundStereo22(cin, data);
		break;
	case ROQ_SOUND_MONO_48:
		CIN_DecodeSoundMono48(cin, data);
		break;
	case ROQ_SOUND_STEREO_48:
		CIN_DecodeSoundStereo48(cin, data);
		break;
	default:
		I_Error("CIN_DecodeChunk: bad chunk id (%u)", cin->chunkHeader.id);
	}

	// Decode the next chunk header if needed
	if (cin->offset >= cin->size)
		return true;

	data += cin->chunkHeader.size;

#if DEBUG_ROQ_READER
	I_Printf("  offset 0x%08x\n", cin->offset);
#endif
	cin->chunkHeader.id = data[0] | (data[1] << 8);
	cin->chunkHeader.size = data[2] | (data[3] << 8) | (data[4] << 16) | (data[5] << 24);
	cin->chunkHeader.args = data[6] | (data[7] << 8);
#if DEBUG_ROQ_READER
	I_Printf("    CHDR: 0x%04x %d 0x%04x\n", cin->chunkHeader.id, cin->chunkHeader.size, cin->chunkHeader.args);
#endif

	return true;
}


// ============================================================================


/*
 ==================
 CIN_HandleForCinematic

 Finds a free cinHandle_t
 ==================
*/
static cinematic_t *CIN_HandleForCinematic(cinHandle_t *handle)
{

	cinematic_t *cin;
	int         i;

	for (i = 0, cin = cin_cinematics; i < MAX_CINEMATICS; i++, cin++)
	{
		if (!cin->playing)
			break;
	}

	if (i == MAX_CINEMATICS)
		I_Error("CIN_HandleForCinematic: none free");

	*handle = i + 1;

	return cin;
}

/*
 ==================
 CIN_GetCinematicByHandle

 Returns a cinematic_t for the given cinHandle_t
 ==================
*/
static cinematic_t *CIN_GetCinematicByHandle(cinHandle_t handle)
{

	cinematic_t *cin;

	if (handle <= 0 || handle > MAX_CINEMATICS)
		I_Error("CIN_GetCinematicByHandle: handle out of range");

	cin = &cin_cinematics[handle - 1];

	if (!cin->playing)
		I_Error("CIN_GetCinematicByHandle: invalid handle");

	return cin;
}

/*
 ==================
 CIN_PlayCinematic
 ==================
*/
cinHandle_t CIN_PlayCinematic(const char *name, int flags)
{

	cinematic_t        *cin;
	cinHandle_t        handle;
	epi::file_c        *F;
	u16_t            id, fps;
	int                i;

	if (name)
	{
		// See if already playing
		for (i = 0, cin = cin_cinematics; i < MAX_CINEMATICS; i++, cin++)
		{
			if (!cin->playing)
				continue;

			if (!strcmp(cin->name, name))
			{
				if (cin->flags != flags)
					continue;

				return i + 1;
			}
		}

		std::string fn = M_ComposeFileName(game_dir.c_str(), name);

		F = epi::FS_Open(fn.c_str(), epi::file_c::ACCESS_READ |
			epi::file_c::ACCESS_BINARY);

		if (F)
		{
			I_Printf("Movie player: Opened movie file '%s'\n", fn.c_str());
		}
		else
		{
			// not a file, try a lump
			int lump = W_FindLumpFromPath(name);
			if (lump < 0)
			{
				M_WarnError("Movie player: Missing file or lump: %s\n", name);
				return -1;
			}

			F = W_OpenLump(lump);
			if (!F)
			{
				M_WarnError("Movie player: Can't open lump: %s\n", name);
				return -1;
			}
			I_Printf("Movie player: Opened movie lump '%s'\n", name);
		}

		int size = F->GetLength();

		int offset = F->GetPosition();

		F->Read(&cin_chunkData, ROQ_CHUNK_HEADER_SIZE);

		id = cin_chunkData[0] | (cin_chunkData[1] << 8);
		fps = cin_chunkData[6] | (cin_chunkData[7] << 8);

		if (id != ROQ_ID)
		{
			delete F;

			if (flags & CIN_SYSTEM)
				I_Printf("Cinematic %s is not a RoQ file\n", name);

			return -1;
		}
		I_Printf("RoQ format movie at %d FPS opened.\n", fps);

		// Play the cinematic
		cin = CIN_HandleForCinematic(&handle);

		if (flags & CIN_SYSTEM)
		{
			I_Printf("Playing cinematic %s\n", name);

			// Force console and GUI off
			//Con_Close();
			//CON_SetVisible(vs_notvisible);
			//GUI_Close();
			M_ClearMenus();

			// Make sure sounds aren't playing
			//S_FreeChannels();
		}

		// Fill it in
		cin->playing = true;
		strcpy(cin->name, name);
		cin->flags = flags;
		cin->file = F;
		cin->size = size;
		cin->offset = ROQ_CHUNK_HEADER_SIZE;
		cin->startTime = 0;
		cin->frameRate = (fps) ? fps : 30;
		cin->frameWidth = 0;
		cin->frameHeight = 0;
		cin->frameCount = 0;
		cin->frameBuffer[0] = NULL;
		cin->frameBuffer[1] = NULL;
		cin->sampleRate = 0;
		cin->nextSample = 0;
		cin->soundSamples = (short *)Z_Malloc(ROQ_CHUNK_MAX_DATA_SIZE << 2);

		return handle;
	}

	return -1;
}

//EDGE Function to play ROQ
void E_PlayMovie(const char *name, int flags)
{
	GLuint tex[1];
	cinHandle_t midx;
	cinData_t mdata;

	playing_movie = false;

	CIN_Init();

	midx = CIN_PlayCinematic(name, flags);

	if (midx < 0)
	{
		CIN_Shutdown();
		SDL_CloseAudio();
		I_StartupSound();
		SDL_PauseAudio(0);
		I_Printf("WARNING: Could not open ROQ: %s", name);
		return; // couldn't open movie
	}

	// setup OpenGL texture buffer
	glGenTextures(1, tex);

	RGL_SetupMatrices2D();

	current_movie = midx;
	playing_movie = true;
	while (playing_movie)
	{
		uint32_t curr_ms;

		SDL_Delay(5);

		curr_ms = SDL_GetTicks();
		CIN_UpdateCinematic(midx, curr_ms, &mdata);

		if ((mdata.image == NULL) && (mdata.dirty == false) &&
			(mdata.width == 0) && (mdata.height == 0))
		{
			I_Printf("End of movie %s\n", name);
			break; // end of movie
		}

		if (mdata.dirty)
		{
			// dirty set => need to draw a movie frame
			I_StartFrame();

			// upload decoded video into opengl buffer
			glBindTexture(GL_TEXTURE_2D, tex[0]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
				mdata.width, mdata.height, 0, GL_RGBA,
				GL_UNSIGNED_BYTE, mdata.image);

			// draw to screen
			glEnable(GL_TEXTURE_2D);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glDisable(GL_ALPHA_TEST);

			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

			glBegin(GL_QUADS);

			glTexCoord2f(0.0f, 1.0f);
			glVertex2i(0, 0);

			glTexCoord2f(1.0f, 1.0f);
			glVertex2i(SCREENWIDTH, 0);

			glTexCoord2f(1.0f, 0.0f);
			glVertex2i(SCREENWIDTH, SCREENHEIGHT);

			glTexCoord2f(0.0f, 0.0f);
			glVertex2i(0, SCREENHEIGHT);

			glEnd();

			glDisable(GL_TEXTURE_2D);

			I_FinishFrame();

			/* check if press key/button */
			SDL_PumpEvents();
			SDL_Event sdl_ev;
			while (SDL_PollEvent(&sdl_ev))
			{
				switch (sdl_ev.type)
				{
				case SDL_KEYUP:
				case SDL_MOUSEBUTTONUP:
				case SDL_JOYBUTTONUP:
					playing_movie = false;
					break;

				default:
					break; // Don't care
				}
			}
		}
	}
	playing_movie = false;
	CIN_StopCinematic(midx);
	I_Printf("Cinematic stopped\n");

	CIN_Shutdown();
	SDL_CloseAudio();
	I_StartupSound();
	SDL_PauseAudio(0);

	// delete OpenGL texture buffer
	if (tex[0])
	{
		I_Printf("Deleting OpenGL texture\n");
		glDeleteTextures(1, tex);
	}

	I_Printf("E_PlayMovie exiting\n");
	return;
}


/*
 ==================
 CIN_UpdateCinematic
 ==================
*/
void CIN_UpdateCinematic(cinHandle_t handle, int time, cinData_t *mdata)
{
#if DEBUG_ROQ_READER
	I_Printf("CIN_UpdateCinematic!\n");
#endif
	cinematic_t *cin;
	int         frame;
	//epi::file_c *f = cin->file;

	cin = CIN_GetCinematicByHandle(handle);

	// If we don't have a frame yet, set the start time
	if (!cin->frameCount)
		cin->startTime = time;

	// Check if a new frame is needed
	frame = (time - cin->startTime) * cin->frameRate / 1000;
	if (frame < 1)
		frame = 1;

	if (frame <= cin->frameCount)
	{
		mdata->image = cin->frameBuffer[1];
		mdata->dirty = false;

		mdata->width = cin->frameWidth;
		mdata->height = cin->frameHeight;

		return;
	}

	// If we're dropping frames
	if (frame > cin->frameCount + 1)
	{
		if (cin->flags & CIN_SYSTEM)
			I_Printf("Dropped cinematic frame: %i > %i (%s)\n", frame, cin->frameCount + 1, cin->name);
		else
			I_Debugf("Dropped cinematic frame: %i > %i (%s)\n", frame, cin->frameCount + 1, cin->name);

		frame = cin->frameCount + 1;

		// Reset the start time
		cin->startTime = time - frame * 1000 / cin->frameRate;
	}

	// Get the desired frame
	while (frame > cin->frameCount)
	{
		// Decode a chunk
		if (CIN_DecodeChunk(cin))
			continue;

		// If we get here, the cinematic has finished
		if (!cin->frameCount && !(cin->flags & CIN_LOOPING))
		{
			I_Printf("  Cinematic finished!\n");

			mdata->image = NULL;
			mdata->dirty = false;

			mdata->width = 0;
			mdata->height = 0;

			return;
		}

		// Reset the cinematic
		cin->file->Seek(ROQ_CHUNK_HEADER_SIZE, epi::file_c::SEEKPOINT_START);
		//F->Seek(cin->file, ROQ_CHUNK_HEADER_SIZE, SEEKPOINT_START);

		cin->offset = ROQ_CHUNK_HEADER_SIZE;
		cin->startTime = time;
		cin->frameCount = 0;

		// Get the first frame
		frame = 1;
	}

	mdata->image = cin->frameBuffer[1];
	mdata->dirty = true;

	mdata->width = cin->frameWidth;
	mdata->height = cin->frameHeight;

	return;
}

/*
 =================
 CIN_UpdateAudio
 =================
*/
void CIN_UpdateAudio(Uint8 *stream, int len)
{
	//I_Printf("CIN_UpdateAudio!\n");
	cinematic_t *cin;
	int j;
	float *dst = (float *)stream;

	cin = CIN_GetCinematicByHandle(current_movie);

	if (cin->nextSample)
	{
		int wanted = len >> 2;
		if (wanted <= cin->nextSample)
		{
			for (j = 0; j < wanted; j++)
				dst[j] = (float)cin->soundSamples[j] * slider_to_gain[au_sfx_volume] * 0.000030518f;
			if (wanted < cin->nextSample)
				memcpy(cin->soundSamples, &cin->soundSamples[wanted], (cin->nextSample - wanted) * 2);
			cin->nextSample -= wanted;
		}
		else
		{
			for (j = 0; j < cin->nextSample; j++)
				dst[j] = (float)cin->soundSamples[j] * slider_to_gain[au_sfx_volume] * 0.000030518f;
			cin->nextSample = 0;

			for (; j < wanted; j++)
				dst[j] = 0.0f;
		}
	}
	else
		for (j = 0; j < len >> 2; j++)
			dst[j] = 0.0f;
}

/*
 ==================
 CIN_ResetCinematic
 ==================
*/
void CIN_ResetCinematic(cinHandle_t handle)
{

	cinematic_t *cin;
	//epi::file_c *f = cin->file;

	cin = CIN_GetCinematicByHandle(handle);

	// Reset the cinematic
	cin->file->Seek(ROQ_CHUNK_HEADER_SIZE, epi::file_c::SEEKPOINT_START);
	//FS_Seek(cin->file, ROQ_CHUNK_HEADER_SIZE, FS_SEEK_SET);

	cin->offset = ROQ_CHUNK_HEADER_SIZE;
	cin->startTime = 0;
	cin->frameCount = 0;
}

/*
 ==================
 CIN_StopCinematic
 ==================
*/
void CIN_StopCinematic(cinHandle_t handle)
{

	cinematic_t *cin;
	//epi::file_c *f = cin->file;

	cin = CIN_GetCinematicByHandle(handle);

	// Stop the cinematic
	if (cin->flags & CIN_SYSTEM)
	{
		I_Debugf("Stopped cinematic %s\n", cin->name);

		// Make sure sounds aren't playing
		// S_FreeChannels();
	}

	// Free the frame buffers
	if (cin->frameBuffer[0])
	{
		I_Printf("  Freeing framebuffer[0]\n");
		Z_Free(cin->frameBuffer[0]);
		cin->frameBuffer[0] = NULL;
	}
	if (cin->frameBuffer[1])
	{
		I_Printf("  Freeing framebuffer[1]\n");
		Z_Free(cin->frameBuffer[1]);
		cin->frameBuffer[1] = NULL;
	}
	if (cin->soundSamples)
	{
		I_Printf("  Freeing soundSamples\n");
		Z_Free(cin->soundSamples);
		cin->soundSamples = NULL;
	}

	// Close the file
	if (cin->file)
	{
		I_Printf("  Deleting file\n");
		delete cin->file;
		cin->file = 0;
	}

	cin->playing = false;
}


// ============================================================================

#if 0

/*
 ==================
 CIN_PlayCinematic_f (play through EDGE console?)
 ==================
*/
static void CIN_PlayCinematic_f(void)
{

	char    name[MAX_PATH];

	if (Cmd_Argc() != 2)
	{
		I_Printf("Usage: playCinematic <name>\n");
		return;
	}

	Str_Copy(name, Cmd_Argv(1), sizeof(name));
	Str_DefaultFilePath(name, "videos", sizeof(name));
	Str_DefaultFileExtension(name, ".RoQ", sizeof(name));

	// If running a local server, kill it
	SV_Shutdown("Server quit");

	// If connected to a server, disconnect
	CL_Disconnect(true);

	// Play the cinematic
	cls.cinematicHandle = CIN_PlayCinematic(name, CIN_SYSTEM);
}
#endif // 0


/*
 ==================
 CIN_ListCinematics_f
 ==================
*/
#if 0
static void CIN_ListCinematics_f(void)
{

	cinematic_t *cin;
	int         count = 0, bytes = 0;
	int         i;

	I_Printf("\n");
	I_Printf("    -w-- -h-- -size- fps -name-----------\n");

	for (i = 0, cin = cin_cinematics; i < MAX_CINEMATICS; i++, cin++) {
		if (!cin->playing)
			continue;

		count++;
		bytes += cin->frameWidth * cin->frameHeight * 8;

		I_Printf("%2i: ", i);

		I_Printf("%4i %4i ", cin->frameWidth, cin->frameHeight);

		I_Printf("%5ik ", BYTES_TO_KB(cin->frameWidth * cin->frameHeight * 8));

		I_Printf("%3i ", cin->frameRate);

		I_Printf("%s\n", cin->name);
	}

	I_Printf("-----------------------------------------\n");
	I_Printf("%i total cinematics\n", count);
	I_Printf("%.2f MB of cinematic data\n", BYTES_TO_FLOAT_MB(bytes));
	I_Printf("\n");
}
#endif // 0


/*
 ==================
 CIN_Init
 ==================
*/
void CIN_Init(void)
{
	I_Printf("I_Cinematic: Starting up...\n");
	float   f;
	int     i;

	// Add commands
	//Cmd_AddCommand("playCinematic", CIN_PlayCinematic_f, "Plays a cinematic", Cmd_ArgCompletion_VideoName);
	//Cmd_AddCommand("listCinematics", CIN_ListCinematics_f, "Lists playing cinematics", NULL);

	// Clear global array
	memset(cin_cinematics, 0, sizeof(cin_cinematics));

	// Build YCbCr-to-RGB tables
	for (i = 0; i < 256; i++)
	{
		f = (float)(i - 128);

		cin_cr2rTable[i] = (short)(f *  1.40200f);
		cin_cb2gTable[i] = (short)(f * -0.34414f);
		cin_cr2gTable[i] = (short)(f * -0.71414f);
		cin_cb2bTable[i] = (short)(f *  1.77200f);
	}

	// CA: Removed SQUARE template -- just write it out manually ;)
	//Build square table (cin_sqrTable[])
	for (int i = 0; i < 128; i++)
	{
		const short s = (short)(i * i);
		cin_sqrTable[i] = s;
		cin_sqrTable[i + 128] = -s;
	}
}

/*
 ==================
 CIN_Shutdown
 ==================
*/
void CIN_Shutdown(void)
{

	cinematic_t *cin;
	int         i;

	// Remove commands
	//Cmd_RemoveCommand("playCinematic");
	//Cmd_RemoveCommand("listCinematics");

	// Stop all the cinematics
	cinematicHandle = 0;

	for (i = 0, cin = cin_cinematics; i < MAX_CINEMATICS; i++, cin++)
	{
		if (!cin->playing)
			continue;

		// Free the frame buffers
		if (cin->frameBuffer[0])
			Z_Free(cin->frameBuffer[0]);
		if (cin->frameBuffer[1])
			Z_Free(cin->frameBuffer[1]);

		// Free the sound sample buffer
		if (cin->soundSamples)
			Z_Free(cin->soundSamples);

		// Close the file
		if (cin->file)
			delete cin->file;
	}
}
#endif // !USE_FFMPEG //define this first, in case people want to compile with i_ffmpeg instead of our ROQ cinematic decoder.
