//----------------------------------------------------------------------------
//  EDGE New SaveGame Handling (Chunks)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2001  The EDGE Team.
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
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------
//
// -AJA- 2000/07/13: Wrote this file.
//

#ifndef __SV_CHUNK_
#define __SV_CHUNK_

#include "i_defs.h"
#include "p_local.h"

#define DATA_END_MARKER  "ENDE"

boolean_t SV_ChunkInit(void);
void SV_ChunkShutdown(void);

int SV_GetError(void);

//
//  READING
//

boolean_t SV_OpenReadFile(const char *filename);
boolean_t SV_CloseReadFile(void);
boolean_t SV_VerifyHeader(int *version);
boolean_t SV_VerifyContents(void);

boolean_t SV_PushReadChunk(const char *id);
boolean_t SV_PopReadChunk(void);
int SV_RemainingChunkSize(void);
boolean_t SV_SkipReadChunk(const char *id);

unsigned char  SV_GetByte(void);
unsigned short SV_GetShort(void);
unsigned int   SV_GetInt(void);

fixed_t SV_GetFixed(void);
angle_t SV_GetAngle(void);
flo_t SV_GetFloat(void);

const char *SV_GetString(void);
boolean_t SV_GetMarker(char id[5]);

//
//  WRITING
//

boolean_t SV_OpenWriteFile(const char *filename, int version);
boolean_t SV_CloseWriteFile(void);

boolean_t SV_PushWriteChunk(const char *id);
boolean_t SV_PopWriteChunk(void);

void SV_PutByte(unsigned char value);
void SV_PutShort(unsigned short value);
void SV_PutInt(unsigned int value);

void SV_PutFixed(fixed_t value);
void SV_PutAngle(angle_t value);
void SV_PutFloat(float value);

void SV_PutString(const char *str);
void SV_PutMarker(const char *id);

#endif  // __SV_CHUNK_
