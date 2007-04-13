//----------------------------------------------------------------------------
//  EDGE WIN32 Music Subsystems
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
// -ACB- 1999/11/13 Written
//

#include "i_defs.h"
#include "w32_sysinc.h"

#include "i_sdlinc.h"

#ifdef USE_OGG
#include "s_ogg.h"
#endif

#include "s_sound.h"

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
	support_MUS  = 0x04, // MUS Support - ACB- 2000/06/04
	support_OGG  = 0x08  // OGG Support - ACB- 2004/08/18
}
mussupport_e;

static byte capable;
static bool musicpaused;

#define MUSICERRLEN 256
static char errordesc[MUSICERRLEN];

#ifdef USE_OGG
oggplayer_c *oggplayer = NULL;
#endif

//
// I_StartupMusic
//
void I_StartupMusic(void)
{
	// Clear the error message
	memset(errordesc, 0, sizeof(errordesc));

	if (nomusic) return;

	// MCI CD Support
	if (I_StartupCD())
	{
		capable |= support_CD;
		I_Printf("I_StartupMusic: CD Music Init OK\n");
	}

	// Music is not paused by default
	musicpaused = false;

	// MUS Support -ACB- 2000/06/04
	if (I_StartupMUS())
	{
		capable |= support_MUS;
		I_Printf("I_StartupMusic: MUS Music Init OK\n");
	}

#ifdef USE_OGG
	if (! nosound)
	{
		oggplayer = new oggplayer_c;
		capable |= support_OGG;

		I_Printf("I_StartupMusic: OGG Music Init OK\n");
	}
	else
#endif
    {
		I_Printf("I_StartupMusic: OGG Music Disabled\n");
    }

	return;
}

//
// I_MusicPlayback
//
int I_MusicPlayback(i_music_info_t *musdat, int type, bool looping,
	float gain)
{
	int track;
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
			if (!I_CDStartPlayback(musdat->info.cd.track, looping, gain))
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
				musdat->info.data.size, looping, gain);

			if (track == -1)
				handle = -1;
			else
			{
				handle = MAKEHANDLE(MUS_MUS, looping, track);
			}

			break;
		}

		case MUS_OGG:
		{
#ifdef USE_OGG
			if (musdat->format == IMUSSF_DATA)
				oggplayer->Open(musdat->info.data.ptr, musdat->info.data.size);
			else // if (musdat->format == IMUSSF_FILE)
				oggplayer->Open(musdat->info.file.name);

			oggplayer->Play(looping, gain);

			handle = MAKEHANDLE(MUS_OGG, looping, 1);
#else // !USE_OGG
			I_PostMusicError("I_MusicPlayback: OGG-Vorbis not supported.\n");
			handle = -1
#endif
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

		case MUS_MIDI:	{ break; }
		case MUS_MUS:	{ I_MUSPause(); break; }
#ifdef USE_OGG
		case MUS_OGG:	{ oggplayer->Pause(); break; }
#endif

		default:
			break;
	}

	musicpaused = true;
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
#ifdef USE_OGG
		case MUS_OGG:  { oggplayer->Resume(); break; }
#endif

		default:
			break;
	}

	musicpaused = false;
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

		case MUS_OGG:  
        { 
#ifdef USE_OGG
            oggplayer->Close(); 
#endif
            break; 
        }

		default:
			break;
	}

	*handle = -1;
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
			if (I_GetTime() % TICRATE == 0)
			{
				if (! I_CDTicker())
					*handle = -1;
			}
			break;
		}

		case MUS_MIDI:	{ break; }	// MIDI Not used
		case MUS_MUS:	{ break; }	// MUS Ticker is called by a timer
#ifdef USE_OGG
		case MUS_OGG:	{ oggplayer->Ticker(); break; }
#endif

		default:
			break;
	}
}

//
// I_SetMusicVolume
//
void I_SetMusicVolume(int *handle, float gain)
{
	int type;
	int handleint;

	handleint = *handle;

	type = GETTYPE(handleint);

	switch (type)
	{
		case MUS_CD:   
		{ 
			I_CDSetVolume(gain);
			break; 
		}

		// MIDI Not Used
		case MUS_MIDI: { break; }

		case MUS_MUS:
		{
			I_MUSSetVolume(gain);
			break;
		}

#ifdef USE_OGG
		case MUS_OGG: { oggplayer->SetVolume(gain); break; }
#endif

		default:
			break;
	}
}

//
// I_ShutdownMusic
//
void I_ShutdownMusic(void)
{
#ifdef USE_OGG
	if (oggplayer)
	{
		delete oggplayer;
		oggplayer = NULL;
	}
#endif

	I_ShutdownMUS();
	I_ShutdownCD();
}

//
// I_PostMusicError
//
void I_PostMusicError(const char *message)
{
	memset(errordesc, 0, MUSICERRLEN*sizeof(char));

	if (strlen(message) > MUSICERRLEN)
		strncpy(errordesc, message, sizeof(char)*MUSICERRLEN);
	else
		strcpy(errordesc, message);
}

//
// I_MusicReturnError
//
char *I_MusicReturnError(void)
{
	return errordesc;
}

// MIXER STUFF (HACKED - WILL BE IN THE EPI)

//
// I_MusicLoadMixer
// 
win32_mixer_t *I_MusicLoadMixer(DWORD type)
{
	MMRESULT res;
	win32_mixer_t mixer;

	// Find ourselves the mixer of type
	MIXERCAPS mxcaps;
	UINT testmixer, mixercount;

	memset(&mixer, 0, sizeof(win32_mixer_t));

	testmixer = 0;
	mixercount = mixerGetNumDevs();
	while (testmixer < mixercount)
	{
		mixer.id = testmixer;
		
		res = mixerGetDevCaps((UINT_PTR)mixer.id, &mxcaps, sizeof(MIXERCAPS));
		if (res == MMSYSERR_NOERROR)
		{
			// Get the mixer handle
			res = mixerOpen((LPHMIXER)&mixer.handle, mixer.id, 0, 0, 0);
			if (res != MMSYSERR_NOERROR)
			{
				testmixer++;
				continue;
			}

			MIXERLINE mixline;
			memset(&mixline, 0, sizeof(MIXERLINE));
			mixline.cbStruct = sizeof(MIXERLINE);
			mixline.dwComponentType = type;

			res = mixerGetLineInfo((HMIXEROBJ)mixer.handle, 
                                   &mixline, MIXER_GETLINEINFOF_COMPONENTTYPE);
			if (res != MMSYSERR_NOERROR)
			{
				mixerClose(mixer.handle);
				testmixer++;
				continue;
			}
			
			MIXERCONTROL mixctrl;
			memset(&mixctrl, 0, sizeof(MIXERCONTROL));
			mixctrl.cbStruct = sizeof(MIXERCONTROL);

			MIXERLINECONTROLS mixlinectrls;
			memset(&mixlinectrls, 0, sizeof(MIXERLINECONTROLS));
			mixlinectrls.cbStruct = sizeof(MIXERLINECONTROLS);
			mixlinectrls.cControls = 1;
			mixlinectrls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
			mixlinectrls.dwLineID = mixline.dwLineID;
			mixlinectrls.cbmxctrl = sizeof(MIXERCONTROL);
			mixlinectrls.pamxctrl = &mixctrl;

			res = mixerGetLineControls((HMIXEROBJ)mixer.handle, 
                                       &mixlinectrls, 
                                       MIXER_GETLINECONTROLSF_ONEBYTYPE);
			if (res != MMSYSERR_NOERROR)
			{
				mixerClose(mixer.handle);
				testmixer++;
				continue;
			}

			if (mixctrl.fdwControl & MIXERCONTROL_CONTROLF_DISABLED)
			{
				mixerClose(mixer.handle);
				testmixer++;
				continue;
			} 

			mixer.channels = mixline.cChannels;
			mixer.volctrlid = mixctrl.dwControlID;
			mixer.minvol = mixctrl.Bounds.dwMinimum;
			mixer.maxvol = mixctrl.Bounds.dwMaximum;
			break;
		}
		else
		{
			testmixer++;
		}
	}

	if (testmixer == mixercount)
		return NULL;

	win32_mixer_t *m;

	m = new win32_mixer_t;
	if (!m)
	{
		mixerClose(mixer.handle);
		return NULL;
	}

	memcpy(m, &mixer, sizeof(win32_mixer_t));
	return m;
}

//
// I_MusicReleaseMixer
// 
void I_MusicReleaseMixer(win32_mixer_t* mixer)
{
	if (!mixer)
		return;

	mixerClose(mixer->handle);
	delete mixer;
}

//
// I_MusicGetMixerVol
//
bool I_MusicGetMixerVol(win32_mixer_t* mixer, DWORD *vol)
{
	if (!mixer || !vol)
		return false;

	MIXERCONTROLDETAILS_UNSIGNED* details;
	MMRESULT res;
	
	details = new MIXERCONTROLDETAILS_UNSIGNED[mixer->channels];
	if (!details)
		return false;

	MIXERCONTROLDETAILS ctrldetails;
	memset(&ctrldetails, 0, sizeof(MIXERCONTROLDETAILS));
	ctrldetails.cbStruct = sizeof(MIXERCONTROLDETAILS);
	ctrldetails.dwControlID = mixer->volctrlid;
	ctrldetails.cChannels = mixer->channels;
	ctrldetails.cMultipleItems = 0;
	ctrldetails.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
	ctrldetails.paDetails = details;

	res = mixerGetControlDetails((HMIXEROBJ)mixer->handle, 
                                 &ctrldetails, MIXER_GETCONTROLDETAILSF_VALUE);
	if (res != MMSYSERR_NOERROR)
	{
		delete [] details;
		return false;
	}

	*vol = details[0].dwValue;
	delete [] details;
	return true;
}

//
// I_MusicSetMixerVol
//
bool I_MusicSetMixerVol(win32_mixer_t* mixer, DWORD vol)
{
	if (!mixer)
		return false;

	MIXERCONTROLDETAILS_UNSIGNED* details;
	MMRESULT res;
	int i;

	details = new MIXERCONTROLDETAILS_UNSIGNED[mixer->channels];
	if (!details)
		return false;

	for (i = 0; i < mixer->channels; i++)
		details[i].dwValue = vol;
	
	MIXERCONTROLDETAILS ctrldetails;
	memset(&ctrldetails, 0, sizeof(MIXERCONTROLDETAILS));
	ctrldetails.cbStruct = sizeof(MIXERCONTROLDETAILS);
	ctrldetails.dwControlID = mixer->volctrlid;
	ctrldetails.cChannels = mixer->channels;
	ctrldetails.cMultipleItems = 0;
	ctrldetails.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
	ctrldetails.paDetails = details;

	res = mixerSetControlDetails((HMIXEROBJ)mixer->handle, 
                                 &ctrldetails, MIXER_SETCONTROLDETAILSF_VALUE);
	if (res != MMSYSERR_NOERROR)
	{
		delete [] details;
		return false;
	}

	delete [] details;
	return true;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
