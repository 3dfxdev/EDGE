//----------------------------------------------------------------------------
//  EDGE Win32 System Internal header
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

#ifndef __SYSTEM_INTERNAL_H__
#define __SYSTEM_INTERNAL_H__

#define DUMMYUNIONNAMEN(n)

#include "..\i_defs.h"

#include <objbase.h>
#include <wchar.h>
#include <winbase.h>
#include <winnls.h>
#include <mmsystem.h>
#include <signal.h>
#include <windowsx.h> // -ACB- 2000/07/19 Cracker API

#include <sys/types.h> // Required for _stat()

#define DIRECTINPUT_VERSION 0x0700

#include <ddraw.h>
#include <dinput.h>
#include <dsound.h>

#include "edge32.rh" // Resources

// I_CD.C - MCI CD Handling
bool I_CDStartPlayback(int tracknum);
bool I_CDPausePlayback(void);
bool I_CDResumePlayback(void);
void I_CDStopPlayback(void);
bool I_CDFinished(void);

// I_CONWIN.C - For console output when not is graphics mode
void I_StartWinConsole(void);
void I_SetConsoleTitle(const char *title);
void I_WinConPrintf(const char *message, ...);
void I_ShutdownWinConsole(void);

// I_CTRL.C
void I_ControlTicker(void);
void I_HandleKeypress(int key, bool keydown); // handle message loop key presses

// I_MP3.C - Win32 MP3 Handling
bool I_StartupMP3(void);
int I_MP3PlayTrack(char *fn, bool loopy);
void I_MP3BufferFill(void);
void I_MP3Ticker(void);
void I_MP3Pause(void);
void I_MP3Resume(void);
void I_MP3Stop(void);
bool I_MP3Playing(void);
void I_MP3SetVolume(int vol);
void I_ShutdownMP3(void);

// I_MUS.C - Win32 MUS Handling
bool I_StartupMUS(void);
int I_MUSPlayTrack(byte *data, int length, bool loopy);
void I_MUSPause(void);
void I_MUSResume(void);
void I_MUSStop(void);
void I_MUSTicker(void);
bool I_MUSPlaying(void);
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
extern bool appactive;

#endif /* __SYSTEM_INTERNAL_H__ */


