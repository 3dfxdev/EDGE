//----------------------------------------------------------------------------
//  EDGE New SaveGame Handling (Chunks)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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
//
// See the file "docs/save_sys.txt" for a complete description of the
// new savegame system.
//
// -AJA- 2000/07/13: Wrote this file.
//

#include "i_defs.h"
#include "sv_chunk.h"

#include "z_zone.h"

#include <zlib.h>

#ifdef HAVE_LZO1X_H
#include <lzo1x.h>
#else
#include "lzo/minilzo.h"
#endif

#include "epi/math_crc.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define DEBUG_GETBYTE  0
#define DEBUG_PUTBYTE  0


#define XOR_STRING  "EDGE!"
#define XOR_LEN     5


#define STRING_MARKER   0xAA
#define NULLSTR_MARKER  0xDE

#define EDGESAVE_MAGIC   "EdgeSave"
#define FIRST_CHUNK_OFS  16L


int savegame_version = 0;

static int last_error = 0;


// maximum size that compressing could give (worst-case scenario)
#define MAX_COMP_SIZE(orig)  (compressBound(orig) + 4)


// The chunk stack will never get any deeper than this
#define MAX_CHUNK_DEPTH  16

typedef struct chunk_s
{
	char s_mark[6];
	char e_mark[6];

	// read/write data.  When reading, this is only allocated/freed for
	// top level chunks (depth 0), lower chunks just point inside their
	// parent's data.  When writing, all chunks are allocated (and grow
	// bigger as needed).  Note: `end' is the byte _after_ the last one.

	unsigned char *start; 
	unsigned char *end; 
	unsigned char *pos;
}
chunk_t;

static chunk_t chunk_stack[MAX_CHUNK_DEPTH];
static int chunk_stack_size = 0;

static FILE *current_fp = NULL;
static epi::crc32_c current_crc;


static bool CheckMagic(void)
{
	int i;
	int len = strlen(EDGESAVE_MAGIC);

	for (i=0; i < len; i++)
		if (SV_GetByte() != EDGESAVE_MAGIC[i])
			return false;

	return true;
}

static void PutMagic(void)
{
	int i;
	int len = strlen(EDGESAVE_MAGIC);

	for (i=0; i < len; i++)
		SV_PutByte(EDGESAVE_MAGIC[i]);
}

static void PutPadding(void)
{
	SV_PutByte(0x1A);
	SV_PutByte(0x0D);
	SV_PutByte(0x0A);
	SV_PutByte(0x00);
}

static INLINE bool VerifyMarker(const char *id)
{
	return isalnum(id[0]) && isalnum(id[1]) &&
		isalnum(id[2]) && isalnum(id[3]);
}

//
// SV_ChunkInit
//
void SV_ChunkInit(void)
{
	if (lzo_init() != LZO_E_OK)
		I_Error("SV_ChunkInit: LZO initialisation error !\n");

	/* ZLib doesn't need to be initialised */
}

//
// SV_ChunkShutdown
//
void SV_ChunkShutdown(void)
{
	// nothing to do
}

//
// SV_GetError
//
int SV_GetError(void)
{
	int result = last_error;
	last_error = 0;

	return result;
}


//----------------------------------------------------------------------------
//  READING PRIMITIVES
//----------------------------------------------------------------------------

//
// SV_OpenReadFile
//
bool SV_OpenReadFile(const char *filename)
{
	chunk_stack_size = 0;
	last_error = 0;

	current_crc.Reset();

	current_fp = fopen(filename, "rb");

	if (! current_fp)
	{
		/// I_Warning("LOADGAME: Couldn't open file: %s\n", filename);
		return false;
	}

	return true;
}

//
// SV_CloseReadFile
//
bool SV_CloseReadFile(void)
{
	DEV_ASSERT2(current_fp);

	DEV_ASSERT(chunk_stack_size == 0,
		("SV_CloseReadFile: Too many Pushes (missing Pop somewhere).\n"));

	fclose(current_fp);

	if (last_error)
		I_Warning("LOADGAME: Error(s) occurred during reading.\n");

	return true;
}

//
// SV_VerifyHeader
//
// Sets the version field, which is BCD, with the patch level in the
// two least significant digits.
//
bool SV_VerifyHeader(int *version)
{
	// check header

	if (! CheckMagic())
	{
		I_Warning("LOADGAME: Bad magic in savegame file\n");
		return false;
	}

	// skip padding
	SV_GetByte();
	SV_GetByte();
	SV_GetByte();
	SV_GetByte();

	(*version) = SV_GetInt();

	savegame_version = (*version);

	if (last_error)
	{
		I_Warning("LOADGAME: Bad header in savegame file\n");
		return false;
	}

	return true;
}

//
// SV_VerifyContents
//
bool SV_VerifyContents(void)
{
	DEV_ASSERT2(current_fp);
	DEV_ASSERT2(chunk_stack_size == 0);

	// skip top-level chunks until end...
	for (;;)
	{
		unsigned int orig_len;
		unsigned int file_len;

		char start_marker[6];

		SV_GetMarker(start_marker);

		if (! VerifyMarker(start_marker))
		{
			I_Warning("LOADGAME: Verify failed: Invalid start marker: "
				"%02X %02X %02X %02X\n", start_marker[0], start_marker[1],
				start_marker[2], start_marker[3]);
			return false;
		}

		if (strcmp(start_marker, DATA_END_MARKER) == 0)
			break;

		// read chunk length
		file_len = SV_GetInt();

		// read original, uncompressed size
		orig_len = SV_GetInt();

		if ((orig_len & 3) != 0 || file_len > MAX_COMP_SIZE(orig_len))
		{
			I_Warning("LOADGAME: Verify failed: Chunk has bad size: "
				"(file=%d orig=%d)\n", file_len, orig_len);
			return false;
		}

		// skip data bytes (merely compute the CRC)
		for (; (file_len > 0) && !last_error; file_len--)
			SV_GetByte();

		// run out of data ?
		if (last_error)
		{
			I_Warning("LOADGAME: Verify failed: Chunk corrupt or "
				"File truncated.\n");
			return false;
		}

		if (savegame_version < 0x12901)
		{
			// check for matching markers
			char end_marker[6];

			SV_GetMarker(end_marker);

			if (! VerifyMarker(end_marker))
			{
				I_Warning("LOADGAME: Verify failed: Invalid end marker: "
					"%02X %02X %02X %02X\n", end_marker[0], end_marker[1],
					end_marker[2], end_marker[3]);
				return false;
			}
			else if (stricmp(start_marker, end_marker) != 0)
			{
				I_Warning("LOADGAME: Verify failed: Mismatched markers: "
					"%s != %s\n", start_marker, end_marker);
				return false;
			}
		}
	}

	// check trailer
	if (! CheckMagic())
	{
		I_Warning("LOADGAME: Verify failed: Bad trailer.\n");
		return false;
	}

	// CRC is now computed

	epi::crc32_c final_crc(current_crc);

	u32_t read_crc = SV_GetInt();

	if (read_crc != final_crc.crc)
	{
		I_Warning("LOADGAME: Verify failed: Bad CRC: %08X != %08X\n", 
			current_crc.crc, read_crc);
		return false;
	}

	// Move file pointer back to beginning
	fseek(current_fp, FIRST_CHUNK_OFS, SEEK_SET);
	clearerr(current_fp);

	return true;
}

//
// SV_GetByte
//
unsigned char SV_GetByte(void) 
{ 
	chunk_t *cur;
	unsigned char result;

	if (last_error)
		return 0;

	// read directly from file when no chunks are on the stack
	if (chunk_stack_size == 0)
	{
		int c = fgetc(current_fp);

		if (c == EOF)
		{
			I_Error("LOADGAME: Corrupt Savegame (reached EOF).\n");
			last_error = 1;
			return 0;
		}

		current_crc += (byte) c;

#if (DEBUG_GETBYTE)
		{ 
			static int pos=0; pos++;
			L_WriteDebug("%08X: %02X \n", ftell(current_fp), c);
//			L_WriteDebug("0.%02X%s", result, ((pos % 10)==0) ? "\n" : " ");
		}
#endif

		return (unsigned char) c;
	}

	cur = &chunk_stack[chunk_stack_size - 1];

	DEV_ASSERT2(cur->start);
	DEV_ASSERT2(cur->pos >= cur->start);
	DEV_ASSERT2(cur->pos <= cur->end);

	if (cur->pos == cur->end)
	{
		//!!!
		I_Error("LOADGAME: Corrupt Savegame (reached end of [%s] chunk).\n", cur->s_mark);
		last_error = 2;
		return 0;
	}

	result = cur->pos[0];
	cur->pos++;

#if (DEBUG_GETBYTE)
	{ 
		static int pos=0; pos++;
		L_WriteDebug("%d.%02X%s", chunk_stack_size, result, ((pos % 10)==0) ? "\n" : " ");
	}
#endif

	return result;
}

//
// SV_PushReadChunk
//
bool SV_PushReadChunk(const char *id)
{
	chunk_t *cur;
	unsigned int file_len;
	char marker[6];

	DEV_ASSERT(chunk_stack_size < MAX_CHUNK_DEPTH,
		("SV_PushReadChunk: Too many Pushes (missing Pop somewhere)."));

	// read chunk length
	file_len = SV_GetInt();

	// create new chunk_t
	cur = &chunk_stack[chunk_stack_size];

	strcpy(cur->s_mark, id);
	strcpy(cur->e_mark, id);
	strupr(cur->e_mark);

	// top level chunk ?
	if (chunk_stack_size == 0)
	{
		unsigned int i;

		unsigned int orig_len;
		unsigned int decomp_len;
		byte *file_data;

		// read uncompressed size
		orig_len = SV_GetInt();

		DEV_ASSERT2(file_len <= MAX_COMP_SIZE(orig_len));

		file_data = Z_New(byte, file_len);

		for (i=0; (i < file_len) && !last_error; i++)
			file_data[i] = SV_GetByte();

		DEV_ASSERT2(!last_error);

		cur->start = Z_New(byte, orig_len);
		cur->end = cur->start + orig_len;

		// decompress data
		decomp_len = orig_len;

		if (savegame_version < 0x12902)
		{
			int res = lzo1x_decompress_safe(file_data, file_len,
				cur->start, &decomp_len, NULL);

			if (res != LZO_E_OK)
				I_Error("LOADGAME: ReadChunk [%s] failed: LZO Decompress error.\n", id);
		}
		else if (orig_len == file_len)
		{
			// no compression
			memcpy(cur->start, file_data, file_len);
			decomp_len = file_len;
		}
		else // use ZLIB
		{
			DEV_ASSERT2(file_len > 0);
			DEV_ASSERT2(file_len < orig_len);

			uLongf out_len = orig_len;

			int res = uncompress(cur->start, &out_len,
					file_data, file_len);

			if (res != Z_OK)
				I_Error("LOADGAME: ReadChunk [%s] failed: ZLIB uncompress error.\n", id);

			decomp_len = (unsigned int)out_len;
		}

		DEV_ASSERT2(decomp_len == orig_len);

		if (savegame_version < 0x12901)
		{
			for (i=0; i < decomp_len; i++)
				cur->start[i] ^= (byte)(XOR_STRING[i % XOR_LEN]);
		}
	}
	else
	{
		chunk_t *parent = &chunk_stack[chunk_stack_size - 1];

		cur->start = parent->pos;
		cur->end = cur->start + file_len;

		// skip data in parent
		parent->pos += file_len;

		DEV_ASSERT2(parent->pos >= parent->start);
		DEV_ASSERT2(parent->pos <= parent->end);
	}

	cur->pos = cur->start;

	// check for matching markers

	if (savegame_version < 0x12901)
	{
		SV_GetMarker(marker);

		if (strcmp(cur->e_mark, marker) != 0)
		{
			I_Error("LOADGAME: ReadChunk [%s] failed: Bad markers.\n", id);
		}
	}

	// let the SV_GetByte routine (etc) see the new chunk
	chunk_stack_size++;
	return true;
}

// SV_PopReadChunk
//
bool SV_PopReadChunk(void)
{
	chunk_t *cur;

	DEV_ASSERT(chunk_stack_size > 0, 
		("SV_PopReadChunk: Too many Pops (missing Push somewhere)."));

	cur = &chunk_stack[chunk_stack_size - 1];

	if (chunk_stack_size == 1)
	{
		// free the data
		Z_Free(cur->start);
	}

	cur->start = cur->pos = cur->end = NULL;
	chunk_stack_size--;

	return true;
}

//
// SV_RemainingChunkSize
//
int SV_RemainingChunkSize(void)
{
	chunk_t *cur;

	DEV_ASSERT2(chunk_stack_size > 0);

	cur = &chunk_stack[chunk_stack_size - 1];

	DEV_ASSERT2(cur->pos >= cur->start);
	DEV_ASSERT2(cur->pos <= cur->end);

	return (cur->end - cur->pos);
}

//
// SV_SkipReadChunk
//
bool SV_SkipReadChunk(const char *id)
{
	if (! SV_PushReadChunk(id))
		return false;

	return SV_PopReadChunk();
}


//----------------------------------------------------------------------------
//  WRITING PRIMITIVES
//----------------------------------------------------------------------------

//
// SV_OpenWriteFile
//
bool SV_OpenWriteFile(const char *filename, int version)
{
	chunk_stack_size = 0;
	last_error = 0;

	savegame_version = version;

	current_crc.Reset();

	current_fp = fopen(filename, "wb");

	if (! current_fp)
	{
		I_Warning("SAVEGAME: Couldn't open file: %s\n", filename);
		return false;
	}

	// write header

	PutMagic();
	PutPadding();
	SV_PutInt(version);

	return true;
}

//
// SV_CloseWriteFile
//
bool SV_CloseWriteFile(void)
{
	DEV_ASSERT2(current_fp);

	DEV_ASSERT(chunk_stack_size == 0,
		("SV_CloseWriteFile: Too many Pushes (missing Pop somewhere).\n"));

	// write trailer

	SV_PutMarker(DATA_END_MARKER);
	PutMagic();

	epi::crc32_c final_crc(current_crc);

	SV_PutInt(final_crc.crc);

	if (last_error)
		I_Warning("SAVEGAME: Error(s) occurred during writing.\n");

	fclose(current_fp);

	return true;
}

//
// SV_PushWriteChunk
//
bool SV_PushWriteChunk(const char *id)
{
	chunk_t *cur;

	DEV_ASSERT(chunk_stack_size < MAX_CHUNK_DEPTH,
		("SV_PushWriteChunk: Too many Pushes (missing Pop somewhere)."));

	// create new chunk_t
	cur = &chunk_stack[chunk_stack_size];
	chunk_stack_size++;

	strcpy(cur->s_mark, id);
	strcpy(cur->e_mark, id);
	strupr(cur->e_mark);

	// create initial buffer
	cur->start = Z_New(byte, 1024);
	cur->pos   = cur->start;
	cur->end   = cur->start + 1024;

	return true;
}

//
// SV_PopWriteChunk
//
bool SV_PopWriteChunk(void)
{
	int i;
	chunk_t *cur;
	int len;

	DEV_ASSERT(chunk_stack_size > 0, 
		("SV_PopWriteChunk: Too many Pops (missing Push somewhere)."));

	cur = &chunk_stack[chunk_stack_size - 1];

	DEV_ASSERT2(cur->start);
	DEV_ASSERT2(cur->pos >= cur->start);
	DEV_ASSERT2(cur->pos <= cur->end);

	len = cur->pos - cur->start;

	// pad chunk to multiple of 4 characters
	for (; len & 3; len++)
		SV_PutByte(0);

	// decrement stack size, so future PutBytes go where they should
	chunk_stack_size--;

	// firstly, write out marker
	SV_PutMarker(cur->s_mark);

	// write out data.  For top-level chunks, compress it.

	if (chunk_stack_size == 0)
	{
		unsigned char *out_buf;
		uLongf out_len = MAX_COMP_SIZE(len);

		out_buf = Z_New(byte, out_len);

		int res = compress2(out_buf, &out_len, cur->start, len, Z_BEST_SPEED);

		if (res != Z_OK || (int)out_len >= len)
		{
L_WriteDebug("WriteChunk UNCOMPRESSED (res %d != %d, out_len %d > %d)\n",
res, Z_OK, (int)out_len, len);
			// compression failed, so write uncompressed
			memcpy(out_buf, cur->start, len);
			out_len = len;
		}
else {
L_WriteDebug("WriteChunk compress (res %d == %d, out_len %d < %d)\n",
res, Z_OK, (int)out_len, len);
}

		DEV_ASSERT2((int)out_len <= MAX_COMP_SIZE(len));

		// write compressed length
		SV_PutInt((int)out_len);

		// write original length to parent
		SV_PutInt(len);

		for (i=0; i < (int)out_len && !last_error; i++)
			SV_PutByte(out_buf[i]);

		DEV_ASSERT2(!last_error);

		Z_Free(out_buf);
	}
	else
	{
		// write chunk length to parent
		SV_PutInt(len);

		// FIXME: optimise this (transfer data directly into parent)
		for (i=0; i < len; i++)
			SV_PutByte(cur->start[i]);
	}

	// all done, free stuff
	Z_Free(cur->start);

	cur->start = cur->pos = cur->end = NULL;
	return true;
}

//
// SV_PutByte
//
void SV_PutByte(unsigned char value) 
{
	chunk_t *cur;

#if (DEBUG_PUTBYTE)
	{ 
		static int pos=0; pos++;
		L_WriteDebug("%d.%02x%s", chunk_stack_size, value, 
			((pos % 10)==0) ? "\n" : " ");
	}
#endif

	if (last_error)
		return;

	// write directly to the file when chunk stack is empty
	if (chunk_stack_size == 0)
	{
		fputc(value, current_fp);

		if (ferror(current_fp))
		{
			I_Warning("SAVEGAME: Write error occurred !\n");
			last_error = 3;
			return;
		}

		current_crc += (byte) value;
		return;
	}

	cur = &chunk_stack[chunk_stack_size - 1];

	DEV_ASSERT2(cur->start);
	DEV_ASSERT2(cur->pos >= cur->start);
	DEV_ASSERT2(cur->pos <= cur->end);

	// space left in chunk ?  If not, resize it.
	if (cur->pos == cur->end)
	{
		int new_len = (cur->end - cur->start) + 1024;
		int pos_idx = (cur->pos - cur->start);

		Z_Resize(cur->start, byte, new_len);

		cur->end = cur->start + new_len;
		cur->pos = cur->start + pos_idx;
	}

	*(cur->pos++) = value;
}


//----------------------------------------------------------------------------

//
//  BASIC DATATYPES
//

void SV_PutShort(unsigned short value) 
{
	SV_PutByte(value & 0xff);
	SV_PutByte(value >> 8);
}

void SV_PutInt(unsigned int value) 
{
	SV_PutShort(value & 0xffff);
	SV_PutShort(value >> 16);
}

unsigned short SV_GetShort(void) 
{ 
	// -ACB- 2004/02/08 Force the order of execution; otherwise 
	// compilr optimisations may reverse the order of execution
	byte b1 = SV_GetByte();
	byte b2 = SV_GetByte();
	return b1 | (b2 << 8);
}

unsigned int SV_GetInt(void) 
{ 
	// -ACB- 2004/02/08 Force the order of execution; otherwise 
	// compiler optimisations may reverse the order of execution
	unsigned short s1 = SV_GetShort();
	unsigned short s2 = SV_GetShort();
	return s1 | (s2 << 16);
}


//----------------------------------------------------------------------------

//
//  ANGLES
//


void SV_PutAngle(angle_t value) 
{ 
	SV_PutInt((unsigned int) value);
}

angle_t SV_GetAngle(void) 
{
	return (angle_t) SV_GetInt();
}


//----------------------------------------------------------------------------

//
//  FLOATING POINT
//

void SV_PutFloat(float value) 
{ 
	int exp;
	int mant;
	bool neg;

	neg = (value < 0.0f);
	if (neg) value = -value;

	mant = (int) ldexp(frexp(value, &exp), 30);

	SV_PutShort(256 + exp);
	SV_PutInt((unsigned int) (neg ? -mant : mant));
}

float SV_GetFloat(void) 
{ 
	int exp;
	int mant;

	exp = SV_GetShort() - 256;
	mant = (int) SV_GetInt();

	return (float)ldexp((float) mant, -30 + exp);
}


//----------------------------------------------------------------------------

//
//  STRINGS & MARKERS
//

void SV_PutString(const char *str) 
{ 
	if (str == NULL)
	{
		SV_PutByte(NULLSTR_MARKER);
		return;
	}

	SV_PutByte(STRING_MARKER);
	SV_PutShort(strlen(str));

	for (; *str; str++)
		SV_PutByte(*str);
}

void SV_PutMarker(const char *id)
{
	int i;

	DEV_ASSERT2(id);
	DEV_ASSERT2(strlen(id) == 4);

	for (i=0; i < 4; i++)
		SV_PutByte((unsigned char) id[i]);
}

const char *SV_GetString(void) 
{
	int len, i;
	char *result;

	int type = SV_GetByte();

	if (type == NULLSTR_MARKER)
		return NULL;

	if (type != STRING_MARKER)
		I_Error("Corrupt savegame (invalid string).\n");

	len = SV_GetShort();

	result = Z_New(char, len + 1);
	result[len] = 0;

	for (i = 0; i < len; i++)
		result[i] = (char) SV_GetByte();

	// Intentional Const Override
	return (const char *) result;
}

bool SV_GetMarker(char id[5])
{ 
	int i;

	for (i=0; i < 4; i++)
		id[i] = (char) SV_GetByte();

	id[4] = 0;

	return VerifyMarker(id);
}

