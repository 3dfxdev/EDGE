//----------------------------------------------------------------------------
//  EDGE WIN32 Music Subsystems
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2018  The EDGE Team.
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

#include "../i_defs.h"
#include "../win32/w32_sysinc.h"

#include "../i_sdlinc.h"

#include "../../../ddf/main.h"
#include "../../../ddf/playlist.h"

#include "../../s_sound.h"
#include "../../s_opl.h"
#include "../../s_tsf.h"

bool musicpaused;

#define MUSICERRLEN 256
static char errordesc[MUSICERRLEN];

void I_StartupMusic(void)
{
	
	// Clear the error message
	memset(errordesc, 0, sizeof(errordesc));

	if (nomusic) return;

	// Music is not paused by default
	musicpaused = false;

	// MUS Support -ACB- 2000/06/04
	if (I_StartupMUS())
	{
		I_Printf("I_StartupMusic: MUS Music Init OK\n");
		// Init music

		//samplesPerMusicTick = param_samplerate / 700;    // SDL_t0FastAsmService played at 700Hz
	}

	if (S_StartupOPL())
	{
		I_Printf("I_StartupMusic: OPL Init OK\n");
	}
	else
	{
		I_Printf("I_StartupMusic: OPL Init FAILED\n");
	}

	if (S_StartupTSF())
	{
		I_Printf("I_StartupMusic: TinySoundfont Init OK\n");
	}
	else
	{
		I_Printf("I_StartupMusic: TinySoundfont Init FAILED\n");
	}

	return;
}


void I_ShutdownMusic(void)
{
	I_ShutdownMUS();
	S_ShutdownOPL();
}


void I_PostMusicError(const char *message)
{
	memset(errordesc, 0, MUSICERRLEN*sizeof(char));

	if (strlen(message) > MUSICERRLEN)
		strncpy(errordesc, message, sizeof(char)*MUSICERRLEN);
	else
		strcpy(errordesc, message);
}


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
	if (!m) //TODO: V668 https://www.viva64.com/en/w/v668/ There is no sense in testing the 'm' pointer against null, as the memory was allocated using the 'new' operator. The exception will be generated in the case of memory allocation error.
	{
		mixerClose(mixer.handle);
		return NULL;
	}

	memcpy(m, &mixer, sizeof(win32_mixer_t));
	return m;
}

void I_MusicReleaseMixer(win32_mixer_t* mixer)
{
	if (!mixer)
		return;

	mixerClose(mixer->handle);
	delete mixer;
}

bool I_MusicGetMixerVol(win32_mixer_t* mixer, DWORD *vol)
{
	if (!mixer || !vol)
		return false;

	MIXERCONTROLDETAILS_UNSIGNED* details;
	MMRESULT res;
	
	details = new MIXERCONTROLDETAILS_UNSIGNED[mixer->channels];
	if (!details) //TODO: V668 https://www.viva64.com/en/w/v668/ There is no sense in testing the 'details' pointer against null, as the memory was allocated using the 'new' operator. The exception will be generated in the case of memory allocation error.
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
