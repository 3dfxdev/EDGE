//----------------------------------------------------------------------------
//  EDGE Music handling Code
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
// -ACB- 1999/11/13 Written
//

#include "i_defs.h"
#include "ddf_main.h"
#include "m_misc.h"
#include "s_sound.h"
#include "w_wad.h"

// Current music handle
static int musichandle = -1;
static int musicvolume;

boolean_t nomusic = false;

// =================== INTERNALS ====================
// HELPER Functions
static INLINE int edgemin(int a, int b)
{
	return (a < b) ? a : b;
}

static INLINE int edgemax(int a, int b)
{
	return (a > b) ? a : b;
}

static INLINE int edgemid(int a, int b, int c)
{
	return edgemax(a, edgemin(b, c));
}
// ================ END OF INTERNALS =================

//
// S_ChangeMusic
//
void S_ChangeMusic(int entrynum, boolean_t looping)
{
	const playlist_t *play;
	byte *data;
	int datlength;
	int datnum;
	i_music_info_t musdat;

	if (nomusic)
		return;

	// -AJA- playlist number 0 reserved to mean "no music"
	if (entrynum <= 0)
	{
		S_StopMusic();
		return;
	}

	play = DDF_MusicLookupNum(entrynum);

	if (!play)
		return;

	S_StopMusic();

	// Exception one...
	if (play->type == MUS_MP3)
	{
		if (play->infotype != MUSINF_FILE)
		{
			I_Warning("S_ChangeMusic: MP3's only in file format\n");
			return;
		} 

		musdat.format = IMUSSF_FILE;
		musdat.info.file.name = (char*) play->info;

		musichandle = I_MusicPlayback(&musdat, play->type, looping);
		if (musichandle != -1)
			I_SetMusicVolume(&musichandle, musicvolume);

		return;
	}

	// -ACB- 2000/06/06 This is not system specific
	if (play->infotype == MUSINF_FILE)
	{
		data = M_GetFileData(play->info, &datlength);
		if (!data)
		{
			I_Warning("S_ChangeMusic: Can't Load File '%s'\n", play->info);
			return;
		}

		musdat.format = IMUSSF_DATA;
		musdat.info.data.ptr = data;
		musdat.info.data.size = datlength;

		musichandle = I_MusicPlayback(&musdat, play->type, looping);

		Z_Free(data);
	}

	if (play->infotype == MUSINF_LUMP)
	{
		datnum = W_CheckNumForName(play->info);
		if (datnum != -1)
		{
			datlength = W_LumpLength(datnum);
			data = (byte*)W_CacheLumpName(play->info);

			musdat.format = IMUSSF_DATA;
			musdat.info.data.ptr = data;
			musdat.info.data.size = datlength;

			musichandle = I_MusicPlayback(&musdat, play->type, looping);
			W_DoneWithLump(data);
		}
		else
		{
			I_Warning("S_ChangeMusic: LUMP '%s' not found.\n", play->info); 
			return;
		}
	}

	if (play->infotype == MUSINF_TRACK)
	{
		musdat.format = IMUSSF_CD;
		musdat.info.cd.track = atoi(play->info);

		musichandle = I_MusicPlayback(&musdat, play->type, looping);
	}

	// if the music handle is returned, set the volume.
	if (musichandle != -1)
		I_SetMusicVolume(&musichandle, musicvolume);

	return;
}

//
// S_ResumeMusic
//
void S_ResumeMusic(void)
{
	if (nomusic || musichandle == -1)
		return;

	I_MusicResume(&musichandle);
	return;
}

//
// S_PauseMusic
//
void S_PauseMusic(void)
{
	if (nomusic || musichandle == -1)
		return;

	I_MusicPause(&musichandle);
	return;
}

//
// S_StopMusic
//
void S_StopMusic(void)
{
	if (nomusic || musichandle == -1)
		return;

	I_MusicKill(&musichandle);
	return;
}

//
// S_MusicTicker
//
void S_MusicTicker(void)
{
	I_MusicTicker(&musichandle);
	return;
}

//
// S_GetMusicVolume
//
int S_GetMusicVolume(void)
{
	return musicvolume;
}

//
// S_SetMusicVolume
//
void S_SetMusicVolume(int volume)
{
	musicvolume = edgemid(S_MIN_VOLUME, volume, S_MAX_VOLUME);

	if (nomusic || musichandle == -1)
		return;

	I_SetMusicVolume(&musichandle, volume);
}

