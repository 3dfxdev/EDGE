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
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#ifndef __DEM_CHUNK__
#define __DEM_CHUNK__

#include "i_defs.h"
#include "p_local.h"
#include "epi/files.h"

#define DEMO_END_MARKER  "ENDE"

extern int demo_version;

int DEM_GetError(void);

//
//  READING
//

bool DEM_OpenReadFile(epi::file_c *fp);
bool DEM_CloseReadFile(void);
bool DEM_VerifyHeader(int *version);

bool DEM_PushReadChunk(const char *id);
bool DEM_PopReadChunk(void);
int  DEM_RemainingChunkSize(void);
bool DEM_SkipReadChunk(const char *id);

unsigned char  DEM_GetByte(void);
unsigned short DEM_GetShort(void);
unsigned int   DEM_GetInt(void);

angle_t DEM_GetAngle(void);
float DEM_GetFloat(void);

const char *DEM_GetString(void);
bool DEM_GetMarker(char id[5]);

void DEM_GetTiccmd(ticcmd_t *cmd);

//
//  WRITING
//

bool DEM_OpenWriteFile(const char *filename, int version);
bool DEM_CloseWriteFile(void);

bool DEM_PushWriteChunk(const char *id);
bool DEM_PopWriteChunk(void);

void DEM_PutByte(unsigned char value);
void DEM_PutShort(unsigned short value);
void DEM_PutInt(unsigned int value);

void DEM_PutAngle(angle_t value);
void DEM_PutFloat(float value);

void DEM_PutString(const char *str);
void DEM_PutMarker(const char *id);

void DEM_PutTiccmd(const ticcmd_t *cmd);

#endif  // __DEM_CHUNK__
