//----------------------------------------------------------------------------
//  EDGE Misc System Interface Code Header
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
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#ifndef __I_SYSTEM__
#define __I_SYSTEM__

#include "dm_defs.h"
#include "dm_type.h"
#include "dm_state.h"
#include "e_event.h"
#include "e_ticcmd.h"

// I_System.C functions. -ACB- 1999/09/20 Removed system specific attribs.
boolean_t I_SystemStartup(void);
void I_EDGELoop(void);
void I_Printf(const char *message,...) GCCATTR(format(printf, 1, 2));
void I_PutTitle(const char *title);
void I_Error(const char *error,...) GCCATTR(format(printf, 1, 2));
void I_CheckCPU(void);
void I_DisplayExitScreen(void);
void I_SystemShutdown(void);
void I_CloseProgram(int exitnum) GCCATTR(noreturn);
char *I_PreparePath(const char *path);
boolean_t I_PathIsAbsolute(const char *path);
boolean_t I_PathIsDirectory(const char *path);
void I_Warning(const char *warning,...) GCCATTR(format(printf, 1, 2));
void I_WriteDebug(const char *message,...) GCCATTR(format(printf, 1, 2));
void I_TraceBack(void);

// -ES- 1999/10/30 i_asm.c
void I_RegisterAssembler(void);
void I_PrepareAssembler(void);

// -ES- 2000/02/07 DEV_ASSERT fails if the condition is false.
// The second parameter is the entire I_Error argument list, surrounded
// by parentheses (which makes it possible to use an arbitrary number of
// parameters even without GCC)
#ifdef DEVELOPERS
#define DEV_ASSERT(cond, arglist)  \
    ((cond)?(void) 0 : I_Error arglist)
#else
#define DEV_ASSERT(cond, arglist)  ((void) 0)
#endif

// -AJA- 2000/02/13: variation on a theme...
#ifdef DEVELOPERS
#define DEV_ASSERT2(cond)  \
    ((cond)? (void)0:I_Error("Assertion `" #cond "' failed at line %d of %s.\n",  \
         __LINE__, __FILE__))
#else
#define DEV_ASSERT2(cond)  ((void) 0)
#endif

// -ACB- 1999/09/19 moved from I_Input.H
void I_StartupControl(void);
void I_ControlGetEvents(void);
void I_CalibrateJoystick(int ch);
void I_ShutdownControl(void);
long I_PureRandom(void);
int I_GetTime(void);
unsigned long I_ReadMicroSeconds(void);
extern unsigned long microtimer_granularity;

// -ACB- 1999/09/19 moved from I_Music.H
boolean_t I_StartupMusic(void *sysinfo);
void I_SetMusicVolume(int *handle, int volume);
int I_MusicPlayback(char *strdata, int type, boolean_t file, boolean_t looping);
void I_MusicPause(int *handle);
void I_MusicResume(int *handle);
void I_MusicKill(int *handle);
void I_MusicTicker(int *handle);
void I_ShutdownMusic(void);
char *I_MusicReturnError(void);

// -ACB- 1999/09/20 Moved from I_Net.H
void I_InitNetwork(void);
void I_NetCmd(void);

// -ACB- 1999/09/20 Moved from I_Sound.H
extern boolean_t nosound;
boolean_t I_StartupSound(void *sysinfo);
boolean_t I_LoadSfx(const unsigned char *data, unsigned int length, unsigned int freq, unsigned int handle);
boolean_t I_UnloadSfx(unsigned int handle);
int I_SoundPlayback(unsigned int soundid, int pan, int vol, boolean_t looping);
boolean_t I_SoundAlter(unsigned int chanid, int pan, int vol);
boolean_t I_SoundCheck(unsigned int chanid);
boolean_t I_SoundPause(unsigned int chanid);
boolean_t I_SoundResume(unsigned int chanid);
boolean_t I_SoundKill(unsigned int chanid);
void I_SoundTicker(void);
void I_ShutdownSound(void);
const char *I_SoundReturnError(void);

// -ACB- 1999/09/20 Moved from I_Video.H
typedef struct truecol_info_s
{
  int red_bits, green_bits, blue_bits;
  int red_shift, green_shift, blue_shift;
  int red_mask, green_mask, blue_mask;
}
truecol_info_t;

void I_StartupGraphics(void);
void I_SetPalette(byte palette[256][3]);
void I_MakeColourmapRange(void *dest_colmaps, byte palette[256][3], const byte *src_colmaps, int num);
void I_StartFrame(void);
void I_FinishFrame(void);
void I_WaitVBL(int count);
void I_ReadScreen(byte *scr);
long I_Colour2Pixel(byte palette[256][3], int col);
void I_GetTruecolInfo(truecol_info_t *info);
boolean_t I_SetScreenSize(int width, int height, int bpp);
void I_ShutdownGraphics(void);

#ifdef DEVELOPERS
#define CHECKVAL(x) \
  (((x)!=0)?(void)0:I_Error("'" #x "' Value Zero in %s, Line: %d",__FILE__,__LINE__))
#else
#define CHECKVAL(x)  do {} while(0)
#endif

#endif
