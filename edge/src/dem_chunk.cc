//----------------------------------------------------------------------------
//  EDGE New Demo Handling (Chunks)
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
// See the file "docs/demo_fmt.txt" for a description of the new
// demo system.
//
// FIXME: merge common code!! (demo and savegame)
//

#include "i_defs.h"
#include "dem_chunk.h"
#include "z_zone.h"

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


#define STRING_MARKER   0xAA
#define NULLSTR_MARKER  0xDE

#define EDGEDEMO_MAGIC   "EdgeDemo"
#define FIRST_CHUNK_OFS  16L

int demo_version;

static int last_error = 0;


// maximum size that compressing could give (worst-case scenario)
#define MAX_COMP_SIZE(orig)  ((orig) + (orig)/16 + 20)


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

static epi::file_c *read_fp = NULL;
static FILE *write_fp = NULL;
static epi::crc32_c current_crc;


static bool CheckMagic(void)
{
	int len = strlen(EDGEDEMO_MAGIC);

	for (int i=0; i < len; i++)
		if (DEM_GetByte() != EDGEDEMO_MAGIC[i])
			return false;

	return true;
}

static void PutMagic(void)
{
	int len = strlen(EDGEDEMO_MAGIC);

	for (int i=0; i < len; i++)
		DEM_PutByte(EDGEDEMO_MAGIC[i]);
}

static void PutPadding(void)
{
	DEM_PutByte(0x1A);
	DEM_PutByte(0x0D);
	DEM_PutByte(0x0A);
	DEM_PutByte(0x00);
}

static INLINE bool VerifyMarker(const char *id)
{
	return isalnum(id[0]) && isalnum(id[1]) &&
		isalnum(id[2]) && isalnum(id[3]);
}

//
// DEM_GetError
//
int DEM_GetError(void)
{
	int result = last_error;
	last_error = 0;

	return result;
}


//----------------------------------------------------------------------------
//  READING PRIMITIVES
//----------------------------------------------------------------------------

//
// DEM_OpenReadFile
//
bool DEM_OpenReadFile(epi::file_c *fp)
{
	chunk_stack_size = 0;
	last_error = 0;

	current_crc.Reset();

	read_fp = fp;

	return true;
}

//
// DEM_CloseReadFile
//
bool DEM_CloseReadFile(void)
{
	DEV_ASSERT2(read_fp);

	DEV_ASSERT(chunk_stack_size == 0,
		("DEM_CloseReadFile: Too many Pushes (missing Pop somewhere).\n"));

	// fclose(read_fp);

	if (last_error)
		I_Warning("LOAD_DEMO: Error(s) occurred during reading.\n");

	return true;
}

//
// DEM_VerifyHeader
//
// Sets the version field, which is BCD, with the patch level in the
// two least significant digits.
//
bool DEM_VerifyHeader(int *version)
{
	// check header

	if (! CheckMagic())
	{
		I_Warning("LOAD_DEMO: Bad magic in demo file\n");
		return false;
	}

	// skip padding
	DEM_GetByte(); DEM_GetByte();
	DEM_GetByte(); DEM_GetByte();

	(*version) = DEM_GetInt();

	demo_version = (*version);

	if (last_error)
	{
		I_Warning("LOAD_DEMO: Bad header in demo file\n");
		return false;
	}

	return true;
}


//
// DEM_GetByte
//
unsigned char DEM_GetByte(void) 
{ 
	chunk_t *cur;
	unsigned char result;

	if (last_error)
		return 0;

	// read directly from file when no chunks are on the stack
	if (chunk_stack_size == 0)
	{
		byte c;

		if (read_fp->Read(&c, 1) != 1)
		{
			I_Error("LOAD_DEMO: Corrupt demo file (reached EOF).\n");
			last_error = 1;
			return 0;
		}

		current_crc += c;

#if (DEBUG_GETBYTE)
		{ 
			static int pos=0; pos++;
			L_WriteDebug("%08X: %02X \n", read_fp->Position(), c);
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
		I_Error("LOAD_DEMO: Corrupt demo file (reached end of [%s] chunk).\n", cur->s_mark);
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
// DEM_PushReadChunk
//
bool DEM_PushReadChunk(const char *id)
{
	DEV_ASSERT(chunk_stack_size < MAX_CHUNK_DEPTH,
		("DEM_PushReadChunk: Too many Pushes (missing Pop somewhere)."));

	// read chunk length
	unsigned int file_len = DEM_GetInt();

	// create new chunk_t
	chunk_t *cur = &chunk_stack[chunk_stack_size];

	strcpy(cur->s_mark, id);
	strcpy(cur->e_mark, id);
	strupr(cur->e_mark);

	// top level chunk ?
	if (chunk_stack_size == 0)
	{
		unsigned int i;

#if 0 // (COMPRESS_ENABLE)
		unsigned int orig_len;
		unsigned int decomp_len;
		byte *file_data;

		// read uncompressed size
		orig_len = DEM_GetInt();

		DEV_ASSERT2(file_len <= MAX_COMP_SIZE(orig_len));

		file_data = Z_New(byte, file_len);

		for (i=0; (i < file_len) && !last_error; i++)
			file_data[i] = DEM_GetByte();

		DEV_ASSERT2(!last_error);

		cur->start = Z_New(byte, orig_len);
		cur->end = cur->start + orig_len;

		// decompress data
		decomp_len = orig_len;

		i = lzo1x_decompress_safe(file_data, file_len,
			cur->start, &decomp_len, NULL);

		if (i != LZO_E_OK)
		{
			I_Error("LOAD_DEMO: ReadChunk [%s] failed: Decompress error.\n", id);
		}

		DEV_ASSERT2(decomp_len == orig_len);

		for (i=0; i < decomp_len; i++)
			cur->start[i] ^= (byte)(XOR_STRING[i % XOR_LEN]);
#else
		cur->start = Z_New(byte, file_len);
		cur->end = cur->start + file_len;

		for (i=0; (i < file_len) && !last_error; i++)
			cur->start[i] = DEM_GetByte();

		DEV_ASSERT2(!last_error);
#endif
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

///---	// check for matching markers
///---
///---	DEM_GetMarker(marker);
///---
///---	if (strcmp(cur->e_mark, marker) != 0)
///---	{
///---		I_Error("LOAD_DEMO: ReadChunk [%s] failed: Bad markers.\n", id);
///---	}

	// let the DEM_GetByte routine (etc) see the new chunk
	chunk_stack_size++;
	return true;
}

// DEM_PopReadChunk
//
bool DEM_PopReadChunk(void)
{
	DEV_ASSERT(chunk_stack_size > 0, 
		("DEM_PopReadChunk: Too many Pops (missing Push somewhere)."));

	chunk_t *cur = &chunk_stack[chunk_stack_size - 1];

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
// DEM_RemainingChunkSize
//
int DEM_RemainingChunkSize(void)
{
	DEV_ASSERT2(chunk_stack_size > 0);

	chunk_t *cur = &chunk_stack[chunk_stack_size - 1];

	DEV_ASSERT2(cur->pos >= cur->start);
	DEV_ASSERT2(cur->pos <= cur->end);

	return (cur->end - cur->pos);
}

//
// DEM_SkipReadChunk
//
bool DEM_SkipReadChunk(const char *id)
{
	if (! DEM_PushReadChunk(id))
		return false;

	return DEM_PopReadChunk();
}


//----------------------------------------------------------------------------
//  WRITING PRIMITIVES
//----------------------------------------------------------------------------

//
// DEM_OpenWriteFile
//
bool DEM_OpenWriteFile(const char *filename, int version)
{
	chunk_stack_size = 0;
	last_error = 0;

	demo_version = version;

	current_crc.Reset();

	write_fp = fopen(filename, "wb");

	if (! write_fp)
	{
		I_Warning("SAVE_DEMO: Couldn't open file: %s\n", filename);
		return false;
	}

	// write header

	PutMagic();
	PutPadding();
	DEM_PutInt(version);

	return true;
}

//
// DEM_CloseWriteFile
//
bool DEM_CloseWriteFile(void)
{
	DEV_ASSERT2(write_fp);

	DEV_ASSERT(chunk_stack_size == 0,
		("DEM_CloseWriteFile: Too many Pushes (missing Pop somewhere).\n"));

	// write trailer

	DEM_PutMarker(DEMO_END_MARKER);

	epi::crc32_c final_crc(current_crc);

	DEM_PutInt(final_crc.crc);

	if (last_error)
		I_Warning("SAVE_DEMO: Error(s) occurred during writing.\n");

	fclose(write_fp);

	return true;
}

//
// DEM_PushWriteChunk
//
bool DEM_PushWriteChunk(const char *id)
{
	DEV_ASSERT(chunk_stack_size < MAX_CHUNK_DEPTH,
		("DEM_PushWriteChunk: Too many Pushes (missing Pop somewhere)."));

	// create new chunk_t
	chunk_t *cur = &chunk_stack[chunk_stack_size];
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
// DEM_PopWriteChunk
//
bool DEM_PopWriteChunk(void)
{
	int i;
	int len;

	DEV_ASSERT(chunk_stack_size > 0, 
		("DEM_PopWriteChunk: Too many Pops (missing Push somewhere)."));

	chunk_t *cur = &chunk_stack[chunk_stack_size - 1];

	DEV_ASSERT2(cur->start);
	DEV_ASSERT2(cur->pos >= cur->start);
	DEV_ASSERT2(cur->pos <= cur->end);

	len = cur->pos - cur->start;

	// pad chunk to multiple of 4 characters
	for (; len & 3; len++)
		DEM_PutByte(0);

	// decrement stack size, so future PutBytes go where they should
	chunk_stack_size--;

	// firstly, write out marker
	DEM_PutMarker(cur->s_mark);

	// write out data.  For top-level chunks, compress it.

	if (chunk_stack_size == 0)
	{
#if 0 // (COMPRESS_ENABLE)
		unsigned char *out_buf;
		unsigned int out_len;   // unsigned due to LZO

		for (i=0; i < len; i++)
			cur->start[i] ^= (byte)(XOR_STRING[i % XOR_LEN]);

		out_buf = Z_New(byte, MAX_COMP_SIZE(len));

		i = lzo1x_1_compress(cur->start, len, out_buf, &out_len,
			compress_wrkmem);

		if (i != LZO_E_OK)
		{
			I_Error("SAVE_DEMO: WriteChunk [%s] failed: Decompress error.\n",
				cur->e_mark);
		}

		DEV_ASSERT2((int)out_len <= MAX_COMP_SIZE(len));

		// write compressed length
		DEM_PutInt(out_len);

		// write original length to parent
		DEM_PutInt(len);

		for (i=0; (i < (int)out_len) && !last_error; i++)
			DEM_PutByte(out_buf[i]);

		DEV_ASSERT2(!last_error);

		Z_Free(out_buf);
#else
		// write chunk length to parent
		DEM_PutInt(len);

		for (i=0; i < len; i++)
			DEM_PutByte(cur->start[i]);
#endif
	}
	else
	{
		// write chunk length to parent
		DEM_PutInt(len);

		// FIXME: optimise this (transfer data directly into parent)
		for (i=0; i < len; i++)
			DEM_PutByte(cur->start[i]);
	}

///---	// write end marker
///---	DEM_PutMarker(cur->e_mark);

	// all done, free stuff
	Z_Free(cur->start);

	cur->start = cur->pos = cur->end = NULL;
	return true;
}

//
// DEM_PutByte
//
void DEM_PutByte(unsigned char value) 
{
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
		fputc(value, write_fp);

		if (ferror(write_fp))
		{
			I_Warning("SAVE_DEMO: Write error occurred !\n");
			last_error = 3;
			return;
		}

		current_crc += (byte) value;
		return;
	}

	chunk_t *cur = &chunk_stack[chunk_stack_size - 1];

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

void DEM_PutShort(unsigned short value) 
{
	DEM_PutByte(value & 0xff);
	DEM_PutByte(value >> 8);
}

void DEM_PutInt(unsigned int value) 
{
	DEM_PutShort(value & 0xffff);
	DEM_PutShort(value >> 16);
}

unsigned short DEM_GetShort(void) 
{ 
	// -ACB- 2004/02/08 Force the order of execution; otherwise 
	// compilr optimisations may reverse the order of execution
	byte b1 = DEM_GetByte();
	byte b2 = DEM_GetByte();
	return b1 | (b2 << 8);
}

unsigned int DEM_GetInt(void) 
{ 
	// -ACB- 2004/02/08 Force the order of execution; otherwise 
	// compiler optimisations may reverse the order of execution
	unsigned short s1 = DEM_GetShort();
	unsigned short s2 = DEM_GetShort();
	return s1 | (s2 << 16);
}


//----------------------------------------------------------------------------

//
//  ANGLES
//


void DEM_PutAngle(angle_t value) 
{ 
	DEM_PutInt((unsigned int) value);
}

angle_t DEM_GetAngle(void) 
{
	return (angle_t) DEM_GetInt();
}


//----------------------------------------------------------------------------

//
//  FLOATING POINT
//

void DEM_PutFloat(float value) 
{ 
	int exp;
	int mant;
	bool neg;

	neg = (value < 0.0f);
	if (neg) value = -value;

	mant = (int) ldexp(frexp(value, &exp), 30);

	DEM_PutShort(256 + exp);
	DEM_PutInt((unsigned int) (neg ? -mant : mant));
}

float DEM_GetFloat(void) 
{ 
	int exp;
	int mant;

	exp = DEM_GetShort() - 256;
	mant = (int) DEM_GetInt();

	return (float)ldexp((float) mant, -30 + exp);
}


//----------------------------------------------------------------------------

//
//  STRINGS & MARKERS
//

void DEM_PutString(const char *str) 
{ 
	if (str == NULL)
	{
		DEM_PutByte(NULLSTR_MARKER);
		return;
	}

	DEM_PutByte(STRING_MARKER);
	DEM_PutShort(strlen(str));

	for (; *str; str++)
		DEM_PutByte(*str);
}

void DEM_PutMarker(const char *id)
{
	int i;

	DEV_ASSERT2(id);
	DEV_ASSERT2(strlen(id) == 4);

	for (i=0; i < 4; i++)
		DEM_PutByte((unsigned char) id[i]);
}

const char *DEM_GetString(void) 
{
	int len, i;
	char *result;

	int type = DEM_GetByte();

	if (type == NULLSTR_MARKER)
		return NULL;

	if (type != STRING_MARKER)
		I_Error("Corrupt Demo file (invalid string).\n");

	len = DEM_GetShort();

	result = Z_New(char, len + 1);
	result[len] = 0;

	for (i = 0; i < len; i++)
		result[i] = (char) DEM_GetByte();

	// Intentional Const Override
	return (const char *) result;
}

bool DEM_GetMarker(char id[5])
{ 
	int i;

	for (i=0; i < 4; i++)
		id[i] = (char) DEM_GetByte();

	id[4] = 0;

	return VerifyMarker(id);
}


//----------------------------------------------------------------------------

//
//  TICCMDS
//

void DEM_PutTiccmd(const ticcmd_t *cmd)
{ 
	DEM_PutShort((unsigned short) cmd->angleturn);
	DEM_PutShort((unsigned short) cmd->mlookturn);
	DEM_PutShort((unsigned short) cmd->consistency);
	DEM_PutShort((unsigned short) cmd->unused1);

	DEM_PutByte((unsigned char) cmd->forwardmove);
	DEM_PutByte((unsigned char) cmd->sidemove);
	DEM_PutByte((unsigned char) cmd->upwardmove);

	DEM_PutByte(cmd->buttons);
	DEM_PutByte(cmd->extbuttons);

	DEM_PutByte(cmd->chatchar);
	DEM_PutByte(cmd->special[0]);
	DEM_PutByte(cmd->special[1]);
}

void DEM_GetTiccmd(ticcmd_t *cmd)
{ 
    cmd->angleturn   = (short) DEM_GetShort();
    cmd->mlookturn   = (short) DEM_GetShort();
    cmd->consistency = (short) DEM_GetShort();
    cmd->unused1     = (short) DEM_GetShort();

    cmd->forwardmove = (signed char) DEM_GetByte();
    cmd->sidemove    = (signed char) DEM_GetByte();
    cmd->upwardmove  = (signed char) DEM_GetByte();

    cmd->buttons    = DEM_GetByte();
    cmd->extbuttons = DEM_GetByte();

    cmd->chatchar   = DEM_GetByte();
    cmd->special[0] = DEM_GetByte();
    cmd->special[1] = DEM_GetByte();
}


//----------------------------------------------------------------------------

//
//  Player SYNC
//

void DEM_PutPlayerSync(const player_t *p)
{
	DEV_ASSERT2(p->mo);

	DEM_PutShort(p->pnum);

	DEM_PutFloat(p->mo->x);
	DEM_PutFloat(p->mo->y);
	DEM_PutFloat(p->mo->z);
	DEM_PutFloat(p->viewheight);

	DEM_PutAngle(p->mo->angle);
	DEM_PutAngle(p->mo->vertangle);

	DEM_PutFloat(p->mo->mom.x);
	DEM_PutFloat(p->mo->mom.y);
	DEM_PutFloat(p->mo->mom.z);
	DEM_PutFloat(p->mo->health);
}

