//----------------------------------------------------------------------------
//  EDGE Misc: Screenshots, Menu and defaults Code
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
// 1998/07/02 -MH- Added key_flyup and key_flydown
//

#ifndef __M_MISC__
#define __M_MISC__

#include "dm_type.h"

//
// MISC
//
typedef struct
{
  char *name;
  int *location;
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

boolean_t M_WriteFile(char const *name, void *source, int length);
boolean_t M_LoadDefaults(void);
void M_SaveDefaults(void);
int M_ReadFile(char const *name, byte **buffer);
void M_DisplayDisk(void);
void M_DisplayAir(void);
void M_ScreenShot(void);
void M_MakeSaveScreenShot(void);
exttype_e M_CheckExtension(const char *ext, const char* filename);
byte *M_GetFileData(char *filename, int *length);
char *M_ComposeFileName(const char *dir, const char *file);
void M_WarnError(const char *error,...) GCCATTR(format(printf, 1, 2));

int L_CompareTimeStamps(i_time_t *A, i_time_t *B);

extern unsigned short save_screenshot[160][100];
extern boolean_t save_screenshot_valid;

extern boolean_t display_disk;

extern int cfgnormalfov, cfgzoomedfov;

extern int key_right;
extern int key_left;
extern int key_lookup;
extern int key_lookdown;
extern int key_lookcenter;

// -ES- 1999/03/28 Zoom Key
extern int key_zoom;
extern int key_up;
extern int key_down;

extern int key_strafeleft;
extern int key_straferight;

// -ACB- for -MH- 1998/07/19 Flying keys
extern int key_flyup;
extern int key_flydown;

extern int key_fire;
extern int key_use;
extern int key_strafe;
extern int key_speed;
extern int key_autorun;
extern int key_nextweapon;
extern int key_jump;
extern int key_map;
extern int key_180;
extern int key_talk;
extern int key_mlook;  // -AJA- 1999/07/27.
extern int key_secondatk;

extern int mousebfire;
extern int mousebstrafe;
extern int mousebforward;

extern int joybfire;
extern int joybstrafe;
extern int joybuse;
extern int joybspeed;

extern boolean_t autorunning;

extern default_t defaults[];

#endif
