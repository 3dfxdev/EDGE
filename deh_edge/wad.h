//----------------------------------------------------------------------------
//  EDGE Tool WAD I/O Header
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
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

void WAD_Startup(void);
void WAD_Shutdown(void);

void WAD_NewLump(const char *name);
void WAD_AddData(const byte *data, int len);
void WAD_Printf(const char *str, ...);
void WAD_FinishLump(void);

void WAD_WriteFile(const char *name);

#endif /* __WAD_IO_HEADER__ */
