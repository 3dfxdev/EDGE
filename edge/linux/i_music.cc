//----------------------------------------------------------------------------
//  EDGE Linux Music System
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
// -ACB- 2000/03/17 Added CD Music
// -ACB- 2001/01/14 Replaced I_WriteDebug() with I_PostMusicError()
//

#include "../i_defs.h"
#include "i_sysinc.h"

#ifdef USE_AL
#define HOGGIE_OGG_SUPPORT
#endif

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
	support_MUS  = 0x04,
	support_OGG  = 0x08  // OGG Support - ACB- 2004/08/18
}
mussupport_e;

static byte capable;

#ifdef HOGGIE_OGG_SUPPORT
#include "../AL/oggplayer.h"

oggplayer_c *oggplayer = NULL;
#endif

#define MUSICERRLEN 256
static char errordesc[MUSICERRLEN];

bool musicpaused = false;

//
// I_StartupMusic
//
bool I_StartupMusic(void *sysinfo)
{
	// Clear the error message
	memset(errordesc, '\0', sizeof(char)*MUSICERRLEN);

	if (nomusic)
		return true;

	if (I_StartupCD())
		capable = support_CD;
	else
		I_Printf("%s\n", I_MusicReturnError());

#ifdef HOGGIE_OGG_SUPPORT
	oggplayer = new oggplayer_c;
	capable |= support_OGG;

	I_Printf("I_StartupMusic: OGG Music Init OK\n");
#endif

	// Music is not paused by default
	musicpaused = false;

	// Nothing went pear shaped
	return true;
}

//
// I_MusicPlayback
//
int I_MusicPlayback(i_music_info_t *musdat, int type, bool looping)
{
	int handle;

	if (!(capable & support_CD)   && type == MUS_CD)   return -1;
	if (!(capable & support_MIDI) && type == MUS_MIDI) return -1;
	if (!(capable & support_MUS)  && type == MUS_MUS)  return -1;
	if (!(capable & support_OGG)  && type == MUS_OGG)  return -1;

	switch (type)
	{
		// CD Support...
		case MUS_CD:
		{
			if (! I_CDStartPlayback(musdat->info.cd.track))
				handle = -1;
			else
				handle = MAKEHANDLE(MUS_CD, looping, musdat->info.cd.track);
			break;
		}

		case MUS_MIDI:
		{
			I_PostMusicError("I_MusicPlayback: MIDI not yet supported.\n");
			handle = -1;
			break;
		}

		case MUS_MUS:
		{
			I_PostMusicError("I_MusicPlayback: MUS not yet supported.\n");
			handle = -1;
			break;
		}

#ifdef HOGGIE_OGG_SUPPORT
		case MUS_OGG:
		{
			oggplayer->Open(musdat->info.file.name);
			oggplayer->Play(looping);
			handle = MAKEHANDLE(MUS_OGG, looping, 1);
			break;
		}
#endif

		case MUS_UNKNOWN:
		{
			I_PostMusicError("I_MusicPlayback: Unknown format type given.\n");
			handle = -1;
			break;
		}

		default:
		{
			I_PostMusicError("I_MusicPlayback: Weird Format given.\n");
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

	if (*handle == -1)
		return;

	type = GETTYPE(*handle);

	switch (type)
	{
		case MUS_CD:
		{
			I_CDPausePlayback();
			break;
		}

		case MUS_MUS:
		{
			break;
		}

#ifdef HOGGIE_OGG_SUPPORT
		case MUS_OGG:	{ oggplayer->Pause(); break; }
#endif
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

	if (*handle == -1)
		return;

	type = GETTYPE(*handle);

	switch (type)
	{
		case MUS_CD:
		{
			I_CDResumePlayback();
			break;
		}

		case MUS_MUS:
		{
			break;
		}
		
#ifdef HOGGIE_OGG_SUPPORT
		case MUS_OGG:  { oggplayer->Resume(); break; }
#endif

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

	if (*handle == -1)
		return;

	type = GETTYPE(*handle);

	switch (type)
	{
		case MUS_CD:
		{
			I_CDStopPlayback();
			break;
		}

		case MUS_MUS:
		{
			break;
		}

#ifdef HOGGIE_OGG_SUPPORT
		case MUS_OGG:  { oggplayer->Close(); break; }
#endif

		default:
			break;
	}

	*handle = -1;
	return;
}

//
// I_SetMusicVolume
//
void I_SetMusicVolume(int *handle, int volume)
{
	int type;

	if (*handle == -1)
		return;

	type = GETTYPE(*handle);

	switch (type)
	{
		case MUS_CD:
		{
			I_CDSetVolume(volume);
			break;
		}

		case MUS_MUS:
		{
			break;
		}
		
#ifdef HOGGIE_OGG_SUPPORT
		case MUS_OGG:  { oggplayer->SetVolume(volume); break; }
#endif

		default:
			break;
	}

	return;
}

//
// I_MusicTicker
//
void I_MusicTicker(int *handle)
{
	int type;
	int libhandle;
	bool looping;

	if (musicpaused)
		return;

	if (*handle == -1)
		return;

	type = GETTYPE(*handle);
	looping = GETLOOPBIT(*handle);
	libhandle = GETLIBHANDLE(*handle);

	switch (type)
	{
		case MUS_CD:
		{
			if (!(I_GetTime()%TICRATE))
			{
				if (looping && I_CDFinished())
				{
					if (!I_CDStartPlayback(libhandle))
					{
						*handle = -1;
						I_ShutdownCD();
					}
				}
			}
			break;
		}

#ifdef HOGGIE_OGG_SUPPORT
		case MUS_OGG:	{ oggplayer->Ticker(); break; }
#endif

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
#ifdef HOGGIE_OGG_SUPPORT
	if (oggplayer)
	{
		delete oggplayer;
		oggplayer = NULL;
	}
#endif

	I_ShutdownCD();
}

//
// I_PostMusicError
//
void I_PostMusicError(const char *message)
{
	memset(errordesc, 0, MUSICERRLEN);

	if (strlen(message) > MUSICERRLEN-1)
		strncpy(errordesc, message, MUSICERRLEN-1);
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

