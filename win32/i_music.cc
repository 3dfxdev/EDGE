//----------------------------------------------------------------------------
//  EDGE WIN32 Music Subsystems
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
// -ACB- 1999/11/13 Written
//

#include "..\i_defs.h"
#include "i_sysinc.h"

// #defines for handle information
#define GETLIBHANDLE(_handle) (_handle&0xFF)
#define GETLOOPBIT(_handle)   ((_handle&0x10000)>>16)
#define GETTYPE(_handle)      ((_handle&0xFF00)>>8)

#define MAKEHANDLE(_type,_loopbit,_libhandle) \
	(((_loopbit&1)<<16)+(((_type)&0xFF)<<8)+(_libhandle))

typedef enum
{
	support_CD   = 0x01,
	support_MIDI = 0x02,
	support_MUS  = 0x08  // MUS Support - ACB- 2000/06/04
}
mussupport_e;

static byte capable;
static bool musicpaused;

#define MUSICERRLEN 256
static char errordesc[MUSICERRLEN];

//
// I_StartupMusic
//
bool I_StartupMusic(void *sysinfo)
{
	// Clear the error message
	memset(errordesc, 0, sizeof(char)*MUSICERRLEN);

	// MCI CD Support Assumed
	capable = support_CD;

	// Music is not paused by default
	musicpaused = false;

	// MUS Support -ACB- 2000/06/04
	if (I_StartupMUS())
	{
		capable |= support_MUS;
		I_Printf("I_StartupMusic: MUS Music Init OK\n");
	}
	else
	{
		I_Printf("I_StartupMusic: MUS Music Failed OK\n");
	}

	return true;
}

//
// I_MusicPlayback
//
int I_MusicPlayback(i_music_info_t *musdat, int type, bool looping)
{
	int track;
	int handle;

	if (!(capable & support_CD)   && type == MUS_CD)   return -1;
	if (!(capable & support_MIDI) && type == MUS_MIDI) return -1;
	if (!(capable & support_MUS)  && type == MUS_MUS)  return -1;

	switch (type)
	{
		// CD Support...
	case MUS_CD:
		{
			if (!I_CDStartPlayback(musdat->info.cd.track))
			{
				handle = -1;
			}
			else
			{
				L_WriteDebug("CD Track Started\n");
				handle = MAKEHANDLE(MUS_CD, looping, musdat->info.cd.track);
			}
			break;
		}

	case MUS_MIDI:
		{
			handle = -1;
			break;
		}

	case MUS_MUS:
		{
			track = I_MUSPlayTrack((byte*)musdat->info.data.ptr,
				musdat->info.data.size,
				looping);

			if (track == -1)
				handle = -1;
			else
				handle = MAKEHANDLE(MUS_MUS, looping, track);

			break;
		}

	case MUS_UNKNOWN:
		{
			L_WriteDebug("I_MusicPlayback: Unknown format type given.\n");
			handle = -1;
			break;
		}

	default:
		{
			L_WriteDebug("I_MusicPlayback: Weird Format '%d' given.\n", type);
			handle = -1;
			break;
		}
	}

	return handle;
}

//
// I_MusicPause
//
void I_MusicPause(int *handle)
{
	int type;

	type = GETTYPE(*handle);

	switch (type)
	{
		case MUS_CD:
			{
				if(!I_CDPausePlayback())
				{
					*handle = -1;
					return;
				}

				break;
			}

		case MUS_MIDI: { break; }
		case MUS_MUS:  { I_MUSPause(); break; }

		default:
			break;
	}

	musicpaused = true;
	return;
}

//
// I_MusicResume
//
void I_MusicResume(int *handle)
{
	int type;

	type = GETTYPE(*handle);

	switch (type)
	{
		case MUS_CD:
			{
				if(!I_CDResumePlayback())
				{
					*handle = -1;
					return;
				}

				break;
			}

		case MUS_MIDI: { break; }
		case MUS_MUS:  { I_MUSResume(); break; }

		default:
			break;
	}

	musicpaused = false;
	return;
}

//
// I_MusicKill
//
// You can't stop the rock!! This does...
//
void I_MusicKill(int *handle)
{
	int type;

	type = GETTYPE(*handle);

	switch (type)
	{
	case MUS_CD:   { I_CDStopPlayback(); break; }
	case MUS_MIDI: { break; }
	case MUS_MUS:  { I_MUSStop(); break; }

	default:
		break;
	}

	*handle = -1;
	return;
}

//
// I_MusicTicker
//
void I_MusicTicker(int *handle)
{
	bool looping;
	int type;
	int libhandle;

	libhandle = GETLIBHANDLE(*handle);
	looping = GETLOOPBIT(*handle)?true:false;
	type = GETTYPE(*handle);

	if (musicpaused)
		return;

	switch (type)
	{
		case MUS_CD:
		{
			if (!(I_GetTime()%TICRATE))
			{
				if (looping && I_CDFinished())
				{
					I_CDStopPlayback();
					if (!I_CDStartPlayback(libhandle))
						*handle = -1;
				}
			}
			break;
		}

		case MUS_MIDI: { break; }	// MIDI Not used
		case MUS_MUS:  { break; }	// MUS Ticker is called by a timer

		default:
			break;
	}

	return;
}

//
// I_SetMusicVolume
//
void I_SetMusicVolume(int *handle, int volume)
{
	int type;
	int handleint;

	handleint = *handle;

	type = GETTYPE(handleint);

	switch (type)
	{
		// CD Vol not used
	case MUS_CD:   { break; }

				   // MIDI Not Used
	case MUS_MIDI: { break; }

	case MUS_MUS:
		{
			I_MUSSetVolume(volume);
			break;
		}

	default:
		break;
	}

	return;
}

//
// I_ShutdownMusic
//
void I_ShutdownMusic(void)
{
	I_CDStopPlayback();
	I_ShutdownMUS();
}

//
// I_PostMusicError
//
void I_PostMusicError(char *message)
{
	memset(errordesc, 0, MUSICERRLEN*sizeof(char));

	if (strlen(message) > MUSICERRLEN)
		strncpy(errordesc, message, sizeof(char)*MUSICERRLEN);
	else
		strcpy(errordesc, message);

	return;
}

//
// I_MusicReturnError
//
char *I_MusicReturnError(void)
{
	return errordesc;
}


