//----------------------------------------------------------------------------
//  EDGE Platform Interface (EPI) Header
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

#ifndef __I_SYSTEM__
#define __I_SYSTEM__

#include "dm_defs.h"
#include "dm_type.h"
#include "dm_state.h"
#include "e_event.h"
#include "e_ticcmd.h"

#include "epi/timestamp.h"

//--------------------------------------------------------
//  SYSTEM functions.
//--------------------------------------------------------
//
// -ACB- 1999/09/20 Removed system specific attribs.

bool I_SystemStartup(void);
// This routine is responsible for getting things off the ground, in
// particular calling all the other platform initialisers (i.e.
// I_StartupControl, I_StartupGraphics, I_StartupMusic and
// I_StartupSound).  Can do whatever else the platform code needs to
// do.  Returns true for success, otherwise false (in which case any
// partially completed actions should be undone, e.g. freeing all
// resources obtained).

void I_Loop(void);
// This is called by EDGE to begin the main engine loop, and is not
// expected to return.  It must call engine::Tick() to perform a
// single loop in the system, which processes events, updates the play
// simulation, keeps sound and music playing, and most importantly
// renders a single frame of graphics.

void I_Printf(const char *message,...) GCCATTR(format(printf, 1, 2));
// The generic print function.  If in text mode, the message should be
// displayed on the text mode screen.  This function should also call
// L_WriteDebug() and CON_Printf().

void I_PutTitle(const char *title);
// This function should clear the text mode screen (or window), and
// write the given title in a banner at the top (in a stand-out way,
// e.g. bright white on a red background).

void I_Error(const char *error,...) GCCATTR(format(printf, 1, 2));
// The error function.  All fatal errors call I_Error().  This
// function should shut things down (e.g. via I_SystemShutdown),
// display the error message to the user (and possibly debugging info,
// like a traceback) and finally exit the program (e.g. by calling
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

void I_CloseProgram(int exitnum) GCCATTR(noreturn);
// Exit the program immediately, using the given `exitnum' as the
// program's exit status.  This is the very last thing done, and
// I_SystemShutdown() is guaranteed to have already been called.

void I_TraceBack(void);
// Aborts the program, and displays a traceback if possible (which is
// useful for debugging).  The system code should check for the
// "-traceback" option, and when present call this routine instead of
// I_CloseProgram() whenever a fatal error occurs.

bool I_Access(const char *filename);
// Returns true if the given file or directory exists.  For files it
// should check if it readable.

char *I_PreparePath(const char *path);
// Prepares the end of the path name, so it will be possible to
// concatenate a DIRSEPARATOR and a file name to it.  Allocates and
// returns the new string.  Doesn't fail.

bool I_PathIsAbsolute(const char *path);
// Returns true if the path should be treated as an absolute path,
// otherwise false.

bool I_PathIsDirectory(const char *path);
// Returns true if the given path refers to a directory, otherwise
// false (e.g. the path is a file, _or_ doesn't exist).

void I_Warning(const char *warning,...) GCCATTR(format(printf, 1, 2));
// Writes a warning to the console and the debug file (if any).  This
// function should call CON_Printf().

bool I_GetModifiedTime(const char *filename, epi::timestamp_c *time);
// -ACB- 2000/06/05 The returns the time of when the filename passed
// was modified.

void I_Sleep(unsigned long millisecs);
// -AJA- 2005/01/21: sleep for the given number of milliseconds.

//--------------------------------------------------------
//  ASM functions.
//--------------------------------------------------------
//
// -ES- 1999/10/30 i_asm.c

void I_CheckCPU(void);
// Determines the CPU type that EDGE is running under.


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

long I_PureRandom(void);
// Returns a fairly random value, used as seed for EDGE's internal
// random engine.  If this function would return a constant value,
// everything would still work great, except that random events before
// the first tic of a level (like random RTS spawn) would be
// predictable.

int I_GetTime(void);
// Returns a value that increases monotonically over time.  The value
// should increase by TICRATE every second (TICRATE is currently 35).
// The starting value should be close to zero.

unsigned long I_ReadMicroSeconds(void);
// Like I_GetTime(), this function returns a value that increases
// monotonically over time, but in this case the value increases by
// 1000000 every second (i.e. each unit is 1 microsecond).  Since this
// value will overflow regularly (assuming `long' is 32 bits), the
// caller should check for this situation.

extern unsigned long microtimer_granularity;
// This variable specifies the granularity of the I_ReadMicroSeconds()
// function.  It is the minimum difference (other than 0) that any two
// calls to I_ReadMicroSeconds() will have.


//--------------------------------------------------------
//  MUSIC functions.
//--------------------------------------------------------
//
// -ACB- 1999/09/19 moved from I_Music.H

extern bool nomusic;
// This variable enables/disables music.  Initially false, it is set
// to true by the "-nomusic" option.  Can also be set to true by the
// platform code when no working music device is found.

typedef enum 
{
	IMUSSF_DATA,
	IMUSSF_FILE,
	IMUSSF_LUMP,
	IMUSSF_CD
}
i_music_srcformat_e;

typedef struct i_music_info_s
{
	int format;

	union
	{
		struct { void *ptr; int size; } data;
		struct { const char *name; } file;
		struct { int handle; int pos; int size; } lump;
		struct { int track; } cd;
	}
	info;
}
i_music_info_t;
// Struct for setting up music playback. This 
// is here because it is an interface between engine and EPI Code.

bool I_StartupMusic(void *sysinfo);
// Initialises the music system.  Returns true if successful,
// otherwise false.  (You should set "nomusic" to true if it fails).
// The main code never calls this function, it should be called by
// I_SystemStartup(), and can be passed some platform-specific data
// via the `sysinfo' parameter.

int I_MusicPlayback(i_music_info_t* musdat, int type, bool looping,
	float gain);
// Starts music playing using the 'i_music_info_t' for info.
//
// Returns an integer handle that is used to refer to the music (in
// the other functions below), or -1 if an error occurred.  Note: any
// previously playing music should be killed (via I_MusicKill) before
// calling this function.

void I_MusicPause(int *handle);
// Pauses the music.

void I_MusicResume(int *handle);
// Resumes the music after being paused.

void I_MusicKill(int *handle);
// You can't stop the rock!!  This does.

void I_SetMusicVolume(int *handle, float gain);
// Sets the music's volume.  The gain is in the range 0.0 to 1.0
// from quietest to loudest.

void I_MusicTicker(int *handle);
// Called once in a while.  Should keep the music going.

void I_ShutdownMusic(void);
// Shuts down the music system.  This is the opposite to
// I_StartupMusic().  Must be called by I_SystemShutdown(), the main
// code never calls this function.

char *I_MusicReturnError(void);
// Returns an error message string that describes the error from the
// last music function that failed.  It may return an empty string if
// no errors have yet occurred, but never NULL.  The message is not
// cleared.


//--------------------------------------------------------
//  SOUND functions.
//--------------------------------------------------------
//
// -ACB- 1999/09/20 Moved from I_Sound.H

extern bool nosound;
// This variable enables/disables sound.  Initially false, it is set
// to true by the "-nosound" option.  Can also be set to true by the
// platform code when no working sound device is found.

bool I_StartupSound(void *sysinfo);
// Initialises the sound system.  Returns true if successful,
// otherwise false if something went wrong (NOTE: you must set nosound
// to false when it fails).   The main code never calls this function,
// it should be called by I_SystemStartup(), and can be passed some
// platform-specific data via the `sysinfo' parameter.

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

bool I_SoundSetListenerOrient(float *at, float *up);
// Sets the listener orientation. The 'at' and 'up' vectors
// are 3-entry tuples. Returns true if successful, otherwise false.

bool I_SoundSetListenerPos(float *pos, float *veloc);
// Sets the listener position in (x,y,z) format tuple. Returns true 
// if successful, otherwise false.

bool I_SoundSetPos(unsigned int chanid, float *pos, float *veloc);
// Set the sound position in an (x,y,z) format tuple. Returns true if 
// successful, otherwise false.

bool I_SoundSetRelative(unsigned int chanid, bool relative);
// Set the "position of sound is relative to listener" flag. Returns true if 
// successful, otherwise false.

bool I_SoundSetVolume(unsigned int chanid, float gain);
// Alters the volume of a currently playing sound.  Returns true
// if successful, otherwise false.

bool I_SoundStopLooping(unsigned int chanid);
// Stops the sound on this channel looping

bool I_SoundKill(unsigned int chanid);
// Kills a sound on the specified channel that was started with
// I_SoundPlayback(), and frees the channel to be used for future
// sound playbacks.  Note that this function must be called even if
// the sound finished played (i.e. I_SoundCheck() == false).  Returns
// true on success, otherwise false.

void I_SoundTicker(void);
// Called every tic to keep the sounds playing.

void I_ShutdownSound(void);
// Shuts down the sound system.  This is the companion function to
// I_StartupSound().  This must be called by I_SystemShutdown(), the
// main code never calls this function.

const char *I_SoundReturnError(void);
// Returns an error message string that describes the error from the
// last sound function that failed.  It will return an empty string if
// no errors have yet occurred, but never NULL.  This function may
// clear the error message.


//--------------------------------------------------------
//  NETWORKING functions.
//--------------------------------------------------------
//
// -ACB- 1999/09/20 Moved from I_Net.H

void I_StartupNetwork(void);
void I_ShutdownNetwork(void);

void I_InitNetwork(void);
// Initialise the platform's networking code.  Not currently used.
// Note well that networking in EDGE is not currently implemented, and
// if it ever does get implemented then this interface is likely to
// change significantly.

void I_NetCmd(void);
// Send or receive a network command packet.  Not currently used (but
// must be defined).


//--------------------------------------------------------
//  VIDEO functions.
//--------------------------------------------------------
//
// -ACB- 1999/09/20 Moved from I_Video.H

// System specific creen mode information.
typedef struct i_scrmode_s
{
	int width;
	int height;
	int depth;
	bool windowed;
}
i_scrmode_t;

void I_StartupGraphics(void);
// Initialises the graphics system.  This should be called by
// I_SystemStartup(), the main code never calls this function
// directly.  This function should determine what video modes are
// available, and call V_AddAvailableResolution() for them.

void I_StartFrame(void);
// Called to prepare the screen for rendering (if necessary).

void I_FinishFrame(void);
// Called when the current frame has finished being rendered.  This
// routine typically copies the screen buffer to the video memory.  It
// may also handle double/triple buffering here.

void I_WaitVBL(int count);
// Waits until the video card is in Vertical Blanking.  The count
// parameter is the number of frames to wait for (1 means this very
// frame).  This function may do nothing if VBL information is not
// available on the platform.

bool I_SetScreenSize(i_scrmode_t *mode);
// Tries to set the video card to the given mode (or open a window).
// If there already was a valid mode (or open window), this call
// should replace it.  The previous contents (including the palette)
// is assumed to be lost. 
//
// Returns true if successful, otherwise false.  The platform is free
// to select a working mode if the given mode was not possible, in
// which case the values of the global variables SCREENWIDTH,
// SCREENHEIGHT and SCREENBITS must be updated.

void I_ShutdownGraphics(void);
// Shuts down the graphics system.  This is the companion function to
// I_StartupGraphics.  Note that this should be called by
// I_SystemStartup(), the main code never calls this function.


//--------------------------------------------------------
//  ASSERTION macros.
//--------------------------------------------------------

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
	((cond)? (void)0:I_Error("Assertion `%s' failed at line %d of %s.\n",  \
#cond , __LINE__, __FILE__))
#else
#define DEV_ASSERT2(cond)  ((void) 0)
#endif

#ifdef DEVELOPERS
#define CHECKVAL(x) \
	(((x)!=0)?(void)0:I_Error("'" #x "' Value Zero in %s, Line: %d",__FILE__,__LINE__))
#else
#define CHECKVAL(x)  do {} while(0)
#endif

void L_WriteLog(const char *message,...) GCCATTR(format(printf, 1, 2));
void L_WriteDebug(const char *message,...) GCCATTR(format(printf, 1, 2));

// TEMP: another temporary "common lib" thing.
int L_ConvertToDB(int volume, int min, int max);

#endif // __I_SYSTEM__
