//----------------------------------------------------------------------------
//  EDGE Win32 System Internal header
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

#include <objbase.h>
#include <wchar.h>
#include <winbase.h>
#include <winnls.h>
#include <mmsystem.h>
#include <signal.h>
#include <windowsx.h> // -ACB- 2000/07/19 Cracker API

#include <ddraw.h>
#include <dinput.h>
#include <dsound.h>

#include "edge32.rh" // Resources

// I_CD.C - MCI CD Handling
boolean_t I_CDStartPlayback(int tracknum);
boolean_t I_CDPausePlayback(void);
boolean_t I_CDResumePlayback(void);
void I_CDStopPlayback(void);
boolean_t I_CDFinished(void);

// I_CONWIN.C - For console output when not is graphics mode
void I_StartWinConsole(void);
void I_SetConsoleTitle(const char *title);
void I_WinConPrintf(const char *message, ...);
void I_ShutdownWinConsole(void);

// I_CTRL.C
void I_ControlTicker(void);
void I_HandleKeypress(int key, boolean_t keydown); // handle message loop key presses

// I_MP3.C - Win32 MP3 Handling
boolean_t I_StartupMP3(void);
int I_MP3PlayTrack(char *fn, boolean_t loopy);
void I_MP3BufferFill(void);
void I_MP3Ticker(void);
void I_MP3Pause(void);
void I_MP3Resume(void);
void I_MP3Stop(void);
boolean_t I_MP3Playing(void);
void I_MP3SetVolume(int vol);
void I_ShutdownMP3(void);

// I_MUS.C - Win32 MUS Handling
boolean_t I_StartupMUS(void);
int I_MUSPlayTrack(byte *data, int length, boolean_t loopy);
void I_MUSPause(void);
void I_MUSResume(void);
void I_MUSStop(void);
void I_MUSTicker(void);
boolean_t I_MUSPlaying(void);
void I_MUSSetVolume(int vol);
void I_ShutdownMUS(void);

// I_MUSIC.C
void I_PostMusicError(char *error);

// I_SOUND.C
LPDIRECTSOUND I_SoundReturnObject(void); // for MP3

// I_SYSTEM.C
long FAR PASCAL I_WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// I_VIDEO.C
void I_SizeWindow(void);

// Window stuff
extern HWND mainwindow;
extern HINSTANCE maininstance;
extern HACCEL accelerator;

// -ACB- 2000/07/04 We want to see this lot from elsewhere in the EPI
extern HWND conwinhandle;
extern boolean_t appactive;

#endif __SYSTEM_INTERNAL_H__


