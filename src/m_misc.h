//----------------------------------------------------------------------------
//  EDGE Misc: Screenshots, Menu and defaults Code
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
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------
//
// 1998/07/02 -MH- Added key_flyup and key_flydown
//

#ifndef __M_MISC__
#define __M_MISC__

#include "dm_type.h"

#include "epi/epistring.h"
#include "epi/epifile.h"

//
// MISC
//
typedef enum
{
	CFGT_Int = 0,
	CFGT_Boolean = 1,
	CFGT_Key = 2,
}
cfg_type_e;

#define CFGT_Enum  CFGT_Int

typedef struct
{
	int type;
	const char *name;
	void *location;
	int defaultvalue;
}
default_t;

typedef enum
{
	EXT_WEIRD       = 0x00,
	EXT_NONE        = 0x01,
	EXT_NOTMATCHING = 0x02,
	EXT_MATCHING    = 0x04
}
exttype_e;

bool M_WriteFile(char const *name, void *source, int length);
bool M_LoadDefaults(void);
void M_SaveDefaults(void);
int M_ReadFile(char const *name, byte **buffer);
void M_InitMiscConVars(void);
void M_DisplayDisk(void);
void M_DisplayAir(void);
void M_ScreenShot(void);
void M_MakeSaveScreenShot(void);
exttype_e M_CheckExtension(const char *ext, const char* filename);
byte *M_GetFileData(const char *filename, int *length);
void M_ComposeFileName(epi::string_c& fn, const char *dir, const char *file);
FILE *M_OpenComposedFile(const char *dir, const char *file);
epi::file_c *M_OpenComposedEPIFile(const char *dir, const char *file);
void M_WarnError(const char *error,...) GCCATTR(format(printf, 1, 2));
int L_CompareFileTimes(const char *wad_file, const char *gwa_file);

extern unsigned short save_screenshot[160][100];
extern bool save_screenshot_valid;

extern bool autorunning;
extern bool display_disk;
extern bool var_fadepower;

extern int cfgnormalfov, cfgzoomedfov;

#endif  /* __M_MISC__ */
