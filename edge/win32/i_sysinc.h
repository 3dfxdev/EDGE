//----------------------------------------------------------------------------
//  EDGE Win32 System Specific header
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

#ifndef __SYSTEM_INTERNAL_H__
#define __SYSTEM_INTERNAL_H__

#define DIRECTDRAW_VERSION 0x0300
#define DIRECTINPUT_VERSION 0x0300
#define DIRECTSOUND_VERSION 0x0300

#define DUMMYUNIONNAMEN(n)

#include "..\i_defs.h"
#include <windows.h>
#include <objbase.h>
#include <wchar.h>
#include <winbase.h>
#include <winnls.h>
#include <mmsystem.h>
#include <signal.h>
#include <shlwapi.h>

#include <ddraw.h>
#include <dinput.h>
#include <dsound.h>

#ifdef __cplusplus
extern "C"
{
#endif

// I_CD.C - MCI CD Handling
boolean_t I_CDStartPlayback(int tracknum);
boolean_t I_CDPausePlayback(void);
boolean_t I_CDResumePlayback(void);
void I_CDStopPlayback(void);
boolean_t I_CDFinished(void);

// I_CONWIN.C - For console output when not is graphics mode
void I_StartWinConsole(void);
void I_SetConsoleTitle(const char *title);
void I_WinConPrintf(const char* message, ...);
void I_ShutdownWinConsole(void);

// I_CTRL.C
void I_HandleKeypress(int key, boolean_t keydown); // handle message loop key presses

// I_DIGMID.C - DirectMusic MIDI Handler
boolean_t I_StartupDirectMusic(void* sysinfo);
int I_DirectMusicPlayback(char* filename, boolean_t loops);
void I_DirectMusicTicker(int* handle);
void I_DirectMusicPause(int* handle);
void I_DirectMusicResume(int* handle);
void I_DirectMusicStop(void);
void I_ShutdownDirectMusic(void);
char* I_DirectMusicReturnError(void);

// I_MUS.C - Win32 MUS Handling
boolean_t I_StartupMUS(void);
int I_MUSPlayTrack(byte* data, int length, boolean_t loopy);
void I_MUSPause(void);
void I_MUSResume(void);
void I_MUSStop(void);
void I_MUSTicker(void);
boolean_t I_MUSPlaying(void);
void I_MUSSetVolume(int vol);
void I_ShutdownMUS(void);

// I_MUSIC.C
void I_PostMusicError(char* error);

// I_SOUND.C
LPDIRECTSOUND I_SoundReturnObject(void); // for DirectMusic

// I_SYSTEM.C
long FAR PASCAL I_WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// Window stuff
extern HWND mainwindow;
extern HINSTANCE maininstance;
extern HACCEL accelerator;

#ifdef __cplusplus
}
#endif

#endif __SYSTEM_INTERNAL_H__


