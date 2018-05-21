//----------------------------------------------------------------------------
//  EDGE2 Platform Interface (EPI) Header
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2009  The EDGE2 Team.
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

#ifndef __I_SYSTEM_H__
#define __I_SYSTEM_H__

#include "../../epi/timestamp.h"

//--------------------------------------------------------
//  SYSTEM functions.
//--------------------------------------------------------

// -ACB- 1999/09/20 Removed system specific attribs.

void I_SetupSignalHandlers(bool allow_coredump);
void I_CheckAlreadyRunning(void);

void I_SystemStartup(void);
// This routine is responsible for getting things off the ground, in
// particular calling all the other platform initialisers (i.e.
// I_StartupControl, I_StartupGraphics, I_StartupMusic and
// I_StartupSound).  Does whatever else the platform code needs.

//--- void I_Loop(void);
// This is called by EDGE2 to begin the main engine loop, and is not
// expected to return.  It must call engine::Tick() to perform a
// single loop in the system, which processes events, updates the play
// simulation, keeps sound and music playing, and most importantly
// renders a single frame of graphics.

void I_Printf(const char *message,...) GCCATTR((format(printf, 1, 2)));
// The generic print function.  If in text mode, the message should be
// displayed on the text mode screen.  This function should also call
// I_Debugf() and CON_Printf().

void I_Debugf(const char *message,...) GCCATTR((format(printf, 1, 2)));
// The generic debugging function.

void I_PutTitle(const char *title);
// This function should clear the text mode screen (or window), and
// write the given title in a banner at the top (in a stand-out way,
// e.g. bright white on a red background).

void I_Error(const char *error,...) GCCATTR((format(printf, 1, 2)));
// The error function.  All fatal errors call I_Error().  This
// I_CloseProgram).

void I_DisplayExitScreen(void);
// Displays the exit screen on the text mode screen (or window).  The
// text typically comes from the "ENDOOM" lump in the IWAD.

void I_SystemShutdown(void);
// The opposite of the I_SystemStartup routine.  This will shutdown
// everything running in the platform code, by calling the other
// termination functions (I_ShutdownSound, I_ShutdownMusic,
// I_ShutdownGraphics and I_ShutdownControl), and doing anything else
// the platform code needs to (e.g. freeing all other resources).

void I_CloseProgram(int exitnum) GCCATTR((noreturn));
// Exit the program immediately, using the given `exitnum' as the
// program's exit status.  This is the very last thing done, and
// I_SystemShutdown() is guaranteed to have already been called.

void I_TraceBack(void);
// Aborts the program, and displays a traceback if possible (which is
// useful for debugging).  The system code should check for the
// "-traceback" option, and when present call this routine instead of
// I_CloseProgram() whenever a fatal error occurs.

void I_Warning(const char *warning,...) GCCATTR((format(printf, 1, 2)));
// Writes a warning to the console and the debug file (if any).  This
// function should call CON_Printf().

void I_Sleep(int millisecs);
// -AJA- 2005/01/21: sleep for the given number of milliseconds.

void I_Tactile (int frequency, int intensity, int select);
// -CW- 2017/01/23 Force Feedback

void I_MessageBox(const char *message, const char *title);
// -AJA- 2007/04/13: display a system message box with the
// given message (typically a serious error message).

extern const char *win32_exe_path;


//--------------------------------------------------------
//  INPUT functions.
//--------------------------------------------------------
//
// -ACB- 1999/09/19 moved from I_Input.H

void I_StartupControl(void);
// Initialises all control devices (i.e. input devices), such as the
// keyboard, mouse and joysticks.  Should be called from
// I_SystemStartup() -- the main code never calls this function.

void I_ControlGetEvents(void);
// Causes all control devices to send their events to the engine via
// the E_PostEvent() function.

void I_CalibrateJoystick(int ch);
// Something for calibrating a joystick -- not currently used.

void I_ShutdownControl(void);
// Shuts down all control devices.  This is the opposite of
// I_StartupControl().  Should be called from I_SystemShutdown(), the
// main code never calls this function.

int I_PureRandom(void);
// Returns a fairly random value, used as seed for EDGE2's internal
// random engine.  If this function would return a constant value,
// everything would still work great, except that random events before
// the first tic of a level (like random RTS spawn) would be
// predictable.

int I_GetTime(void);
// Returns a value that increases monotonically over time.  The value
// should increase by TICRATE every second (TICRATE is currently 35).
// The starting value should be close to zero.

int I_GetMillies(void);

u32_t I_ReadMicroSeconds(void);
// Like I_GetTime(), this function returns a value that increases
// monotonically over time, but in this case the value increases by
// 1000000 every second (i.e. each unit is 1 microsecond).  Since this
// value will overflow regularly (assuming `long' is 32 bits), the
// caller should check for this situation.


//--------------------------------------------------------
//  MUSIC functions.
//--------------------------------------------------------
//
// -ACB- 1999/09/19 moved from I_Music.H

class abstract_music_c;

extern bool nomusic;
// This variable enables/disables music.  Initially false, it is set
// to true by the "-nomusic" option.  Can also be set to true by the
// platform code when no working music device is found.

extern bool nocdmusic;
// Similarly for CD music. Ignored if nomusic is true.

void I_StartupMusic(void);
// Initialises the music system.  Returns true if successful,
// otherwise false.  (You should set "nomusic" to true if it fails).
// The main code never calls this function, it should be called by
// I_SystemStartup().

void I_ShutdownMusic(void);
// Shuts down the music system.  This is the opposite to
// I_StartupMusic().  Must be called by I_SystemShutdown(), the main
// code never calls this function.

abstract_music_c * I_PlayNativeMusic(const byte *data, int length,
									 float volume, bool loop);

#if 0
abstract_music_c * I_PlayCDMusic(int track, float volume, bool looping);
#endif


//--------------------------------------------------------
//  SOUND functions.
//--------------------------------------------------------
//
// -ACB- 1999/09/20 Moved from I_Sound.H

extern bool nosound;
// This variable enables/disables sound.  Initially false, it is set
// to true by the "-nosound" option.  Can also be set to true by the
// platform code when no working sound device is found.

void I_StartupSound(void);
// Initialises the sound system.  Returns true if successful,
// otherwise false if something went wrong (NOTE: you must set nosound
// to false when it fails).   The main code never calls this function,
// it should be called by I_SystemStartup().

void I_ShutdownSound(void);
// Shuts down the sound system.  This is the companion function to
// I_StartupSound().  This must be called by I_SystemShutdown(), the
// main code never calls this function.

bool I_LoadSfx(const unsigned char *data, unsigned int length,
					unsigned int freq, unsigned int handle);
// Loads the given `handle' with the specified sound data.  Handle is
// a small integer value from 0 onwards.  If no such handle exists
// then it is created.  The handle must not already contain any sound
// data.  The data must be copied, e.g. into memory that the sound
// device can use.  Returns true if successful, otherwise false.
//
// The data format is unsigned, i.e. 0x80 is the zero point, ranging
// upto 0xFF for the peaks, and down to 0x00 for the valleys.  The
// sound data is also mono.  There is no support for 16-bit or stereo
// sounds yet.

bool I_UnloadSfx(unsigned int handle);
// Unloads the sound data for the given handle, previously loaded via
// I_LoadSfx().  This frees the sound data.  Returns true on success,
// otherwise false.

int I_SoundPlayback(unsigned int handle, float gain, bool looping,
	bool relative, float *pos, float *veloc);
// Starts the sound with the given handle playing, using the
// paramaters for panning, volume and looping.  Gain ranges from
// from 0.0 (silent) to 1.0 (loudest).
//
// Returns the channel ID where the sound is played, which is used to
// refer to the sound in the other functions below.  If something goes
// wrong, especially when there are no free channels, then -1 is
// returned.

bool I_SoundCheck(unsigned int chanid);
// Checks that the given sound is still playing (i.e. has not reached
// the end yet), and returns true if it is, otherwise false.

bool I_SoundPause(unsigned int chanid);
// Pauses the sound on the specified channel.  Returns true on
// success, otherwise false.

bool I_SoundResume(unsigned int chanid);
// Resumes the previously paused sound on the specified channel.
// Returns true on success, otherwise false.

bool I_SoundKill(unsigned int chanid);
// Kills a sound on the specified channel that was started with
// I_SoundPlayback(), and frees the channel to be used for future
// sound playbacks.  Note that this function must be called even if
// the sound finished played (i.e. I_SoundCheck() == false).  Returns
// true on success, otherwise false.

void I_SoundTicker(void);
// Called every tic to keep the sounds playing.

const char *I_SoundReturnError(void);
// Returns an error message string that describes the error from the
// last sound function that failed.  It will return an empty string if
// no errors have yet occurred, but never NULL.  This function may
// clear the error message.

void I_LockAudio(void);
void I_UnlockAudio(void);
// wrappers around the SDL functions of the same name,
// however I_UnlockAudio() may be called at any time,
// even when I_LockAudio() hasn't been called.


//--------------------------------------------------------
//  VIDEO functions.
//--------------------------------------------------------
//
// -ACB- 1999/09/20 Moved from I_Video.H

class scrmode_c;

void I_StartupGraphics(void);
// Initialises the graphics system.  This should be called by
// I_SystemStartup(), the main code never calls this function
// directly.  This function should determine what video modes are
// available, and call V_AddAvailableResolution() for them.

void I_ShutdownGraphics(void);
// Shuts down the graphics system.  This is the companion function to
// I_StartupGraphics.  Note that this should be called by
// I_SystemStartup(), the main code never calls this function.

void I_StartFrame(void);
// Called to prepare the screen for rendering (if necessary).

void I_FinishFrame(void);
// Called when the current frame has finished being rendered.  This
// routine typically copies the screen buffer to the video memory.  It
// may also handle double/triple buffering here.

void I_GetDesktopSize(int *width, int *height);
// returns the desktop resolution.

bool I_SetScreenSize(scrmode_c *mode);
// Tries to set the video card to the given mode (or open a window).
// If there already was a valid mode (or open window), this call
// should replace it.  The previous contents (including the palette)
// is assumed to be lost.
//
// Returns true if successful, otherwise false.  The platform is free
// to select a working mode if the given mode was not possible, in
// which case the values of the global variables SCREENWIDTH,
// SCREENHEIGHT and SCREENBITS must be updated.

void I_GrabCursor(bool enable);

void I_SetGamma(float gamma);


void I_Logf(const char *message,...) GCCATTR((format(printf, 1, 2)));

void I_GLf(const char *message, ...) GCCATTR((format(printf, 1, 2)));

void I_PrintGLSL(const char *message, ...) GCCATTR((format(printf, 1, 2)));

#define L_WriteDebug  I_Debugf
#define L_WriteLog    I_Logf
#define L_WriteOpenGL I_GLf

#define OBJSET(obj, val) (memset(&(obj), (val), sizeof(obj)))
#define OBJZERO(obj)     OBJSET((obj), 0)


#endif /*__I_SYSTEM_H__*/

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
