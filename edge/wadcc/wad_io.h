//----------------------------------------------------------------------------
//  EDGE Tool WAD I/O Header
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2000  The EDGE Team.
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
#ifndef __WAD_IO_HEADER__
#define __WAD_IO_HEADER__

#include "i_defs.h"

#define WAD_LUMPNAME_LEN 8
 
/*
boolean_t WAD_AddLump(byte* data, int size, char* name);
int WAD_LumpExists(char* name);
boolean_t WAD_WriteFile(char* name);
boolean_t WAD_ReadFile(char* name);
void WAD_Shutdown(void);
*/
boolean_t WAD_Init();
void WAD_Shutdown();

int WAD_Open(const char* name);
//boolean_t WAD_Read(int wad_id, const char* name, byte **dest, int **len);
boolean_t WAD_Write(int wad_id, 
                    const char* name,
                    const byte* data, 
                    const int size);

boolean_t WAD_Close(int wad_id);

#endif /* __WAD_IO_HEADER__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
