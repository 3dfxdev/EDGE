//----------------------------------------------------------------------------
//  EDGE WIN32 MUS Handling Code
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

#include "..\i_defs.h"
#include "i_sysinc.h"

typedef unsigned short int UWORD;

typedef struct
{
	char ID[4];           // identifier - Always "MUS" 0x1A
	UWORD scorelen;       // Score Length
	UWORD scorestart;     // Score Start Offset in bytes
	UWORD channels;       // number of primary channels
	UWORD sec_channels;   // number of secondary channels
	UWORD instrcnt;       // Number of instruments
	UWORD dummy;
}
musheader_t;

typedef struct
{
	byte channel : 4;
	byte event   : 3;
	byte last    : 1;
}
museventdesc_t;

// MUS event types.
typedef enum 
{
	MUS_EV_RELEASE_NOTE,
	MUS_EV_PLAY_NOTE,
	MUS_EV_PITCH_WHEEL,
	MUS_EV_SYSTEM,                  // Valueless controller.
	MUS_EV_CONTROLLER,       
	MUS_EV_FIVE,                    // ?
	MUS_EV_SCORE_END,
	MUS_EV_SEVEN                    // ?
}
musevent_e;

// MUS controllers.
typedef enum 
{
	MUS_CTRL_INSTRUMENT,
	MUS_CTRL_BANK,
	MUS_CTRL_MODULATION,
	MUS_CTRL_VOLUME,
	MUS_CTRL_PAN,
	MUS_CTRL_EXPRESSION,
	MUS_CTRL_REVERB,
	MUS_CTRL_CHORUS,
	MUS_CTRL_SUSTAIN_PEDAL,
	MUS_CTRL_SOFT_PEDAL,

	MUS_CTRL_SOUNDS_OFF,
	MUS_CTRL_NOTES_OFF,
	MUS_CTRL_MONO,
	MUS_CTRL_POLY,
	MUS_CTRL_RESET_ALL,
	NUM_MUS_CTRLS
}
musctrl_e;

static char ctrl_mus2midi[NUM_MUS_CTRLS] =
{
	0,              // Not used.
	0,              // Bank select.
	1,              // Modulation.
	7,              // Volume.
	10,             // Pan.
	11,             // Expression.
	91,             // Reverb.
	93,             // Chorus.
	64,             // Sustain pedal.
	67,             // Soft pedal.

	120,            // All sounds off.
	123,            // All notes off.
	126,            // Mono.
	127,            // Poly.
	121             // Reset all controllers.
};

static HMIDIOUT midioutput;             // OUTPUT port...
static win32_mixer_t *mixer = NULL;

static musheader_t *song = NULL;        // The song.
static byte *playpos;                   // The current play position.
static bool midiavailable = false;		// Available?
static bool playing       = false;		// The song is playing.
static bool looping       = false;		// The song is looping.
static int waitticks = 0;
static byte chanVols[16];				// Last volume for each channel.

// ================ INTERNALS =================

//
// SongStartAddress
//
// Address of the start of the MUS track.
//
static byte *SongStartAddress(void)
{
	if (!song)
		return 0;

	return (byte*)song + song->scorestart;
}

// ============ END OF INTERNALS ==============

//
// I_StartupMUS
//
// Returns true if no problems.
//
bool I_StartupMUS()
{
	MMRESULT res;

	if (midiavailable)
		return true; // Already initialized.

	// Insert code here...
	mixer = I_MusicLoadMixer(MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER);
	if (!mixer)
	{
		I_PostMusicError("I_StartupMUS: Couldn't load the midi mixer");
		return false;
	}

	// Open the midi stream.
	res = midiOutOpen(&midioutput, MIDI_MAPPER, 0, 0, CALLBACK_NULL);
	if (res != MMSYSERR_NOERROR)
	{
		I_MusicReleaseMixer(mixer);
		mixer = NULL;
		
		I_PostMusicError("I_StartupMUS: Couldn't open the midi device");
		return false;
	}

	if (!I_MusicGetMixerVol(mixer, &mixer->originalvol))
	{
		I_MusicReleaseMixer(mixer);
		mixer = NULL;
		
		midiOutClose(midioutput);
		I_PostMusicError("I_StartupMUS: Unable to get original volume");
		return false;
	}
	
	// All is quiet on startup
	I_MusicSetMixerVol(mixer, 0);
    
	// Non-mixer defaults
	midiavailable = true;
	song          = NULL;
	playpos       = 0;
	playing       = false;
	return true;
}

//
// I_MUSPlayTrack
//
int I_MUSPlayTrack(byte *data, int length, bool loopy, float gain)
{
	if (!midiavailable)
		return -1;

	// Kill any previous music
	if (song)
		I_MUSStop();

	song = (musheader_t*)new byte[length];
	if (!song)
	{
		I_PostMusicError("Unable to allocate for MUS Song");
		return -1;
	}

	memcpy(song, data, length);

	playpos = SongStartAddress();       // Go to the beginning of the song.
	playing = true;
	looping = loopy;

	I_MUSSetVolume(gain);
	return 1;
}

//
// I_MUSPause
//
void I_MUSPause(void)
{
	playing = false;

	// Stop all the notes.
	midiOutReset(midioutput);
}

//
// I_MUSResume
//
void I_MUSResume(void)
{
	playing = true;
}

//
// I_MUSStop
//
void I_MUSStop(void)
{
	int i;
	byte *data;

	if (!midiavailable)
		return;

	// Reset channel settings.
	for (i=0; i<=0xf; i++)              
		midiOutShortMsg(midioutput, 0xe0+i); 

	midiOutReset(midioutput); 

	// Free resources
	data = (byte*)song;
	delete [] data;
	song = NULL;

	playing = false;
	playpos = 0;
}

//
// I_MUSPlaying
//
bool I_MUSPlaying(void)
{
	return playing;
}

//
// I_ShutdownMUS
//
void I_ShutdownMUS(void)
{
	if (!midiavailable)
		return;

	// If there is a registered song, unregister it.
	I_MUSStop();

	// Restore the original volume.

	// Close all our handled objects
	if (mixer)
	{
		I_MusicSetMixerVol(mixer, mixer->originalvol);
		I_MusicReleaseMixer(mixer);
		mixer = NULL;
	}

	midiOutClose(midioutput);

	// Switch off...
	midiavailable = false;
	song          = NULL;
	playpos       = 0;
	playing       = false;
	
	return;
}

//
// I_MUSSetVolume
//
void I_MUSSetVolume(float gain)
{
	if (!mixer)
		return;

	DWORD actualvol;

	// Too small...
	if (gain < 0)
		gain = 0;

	// Too big...
	if (gain > 1)
		gain = 1;

	// Calculate a given value in range
	actualvol = int(gain * (mixer->maxvol - mixer->minvol));
	actualvol += mixer->minvol;

	I_MusicSetMixerVol(mixer, actualvol);
}

//
// I_MUSTicker
//
// Called at 140Hz, interprets the MUS data to MIDI events.
//
void I_MUSTicker(void)
{
	museventdesc_t *evDesc;
	byte midiStatus;
	byte midiChan;
	byte midiParm1;
	byte midiParm2;
	byte scratch;
	int scoreEnd;
	long pitchwheel;

	scoreEnd = 0;

	// Sanity Checks...
	if (!appactive || !midiavailable || !song || !playing)
		return;

	// This is the wait counter.
	if (--waitticks > 0)
		return;

	//
	// We assume playpos is OK. We'll keep playing events until the
	// 'last' bit is set, and then the waitticks is set.
	//
	while(1) // Infinite loop in an interrupt handler. Good call.
	{
		evDesc = (museventdesc_t*)playpos++;
		midiStatus = midiChan = midiParm1 = midiParm2 = 0;

		// Construct the MIDI event.
		switch(evDesc->event)
		{
			case MUS_EV_RELEASE_NOTE:
			{
				midiStatus = 0x80;
				midiParm1  = *playpos++; // Note
				midiParm2  = 0x40;       // Velocity (64)
				break;
			}

			case MUS_EV_PLAY_NOTE:
			{
				midiStatus = 0x90;
				midiParm1 = *playpos++;

				// Is the volume there, too?
				if (midiParm1 & 0x80) 
					chanVols[evDesc->channel] = *playpos++;

				midiParm1 &= 0x7f;
				midiParm2 = chanVols[evDesc->channel];
				break;                   
			}

			case MUS_EV_PITCH_WHEEL:
			{
				int scratch;

				// Scale to MIDI Pitch range
				pitchwheel = *playpos++;
				pitchwheel *= 16384;
				pitchwheel /= 256;

				// Assemble to 14-bit MIDI pitch value
				midiStatus |= 0xE0;
				scratch = (pitchwheel & 7);	midiParm1 = (byte)scratch;
				scratch = (pitchwheel & 0x3F80) >> 7; midiParm2 = (byte)scratch;
				break;
			}

			case MUS_EV_SYSTEM:
			{
				midiStatus = 0xb0;
				scratch = *playpos++;
				midiParm1 = ctrl_mus2midi[scratch];
				midiParm2 = (scratch==12)?song->instrcnt+1:0;
				break;
			}

			case MUS_EV_CONTROLLER:
			{
				midiStatus = 0xb0;
				midiParm1 = *playpos++;
				midiParm2 = *playpos++;

				// The instrument control is mapped to another kind of MIDI event.
				if (midiParm1 == MUS_CTRL_INSTRUMENT)
				{
					midiStatus = 0xc0;
					midiParm1  = midiParm2;
					midiParm2  = 0;
				}
				else
				{
					// Use the conversion table.
					midiParm1 = ctrl_mus2midi[midiParm1];
				}
				break;
			}

			case MUS_EV_SCORE_END:
			{
				// We're done.
				scoreEnd = 1;
				break;
			}

			default:
			{
				I_Warning("MUS_SongPlayer: Unhandled MUS event %d.\n",evDesc->event);
				break;
			}
		}

		if (scoreEnd)
			break;

		// Choose the channel.
		midiChan = evDesc->channel;

		// Redirect MUS channel 16 to MIDI channel 10 (drums).
		if (midiChan == 15)
			midiChan = 9;
		else if(midiChan == 9)
			midiChan = 15;

		// Send out the MIDI event.
		midiOutShortMsg(midioutput, midiChan | midiStatus | (midiParm1<<8) | (midiParm2<<16));

		// Check if this was the last event in a group.
		if (evDesc->last)
			break;
	}

	waitticks = 0;

	// Check for end of score.
	if (scoreEnd)
	{
		playpos = SongStartAddress();
		if (!looping)
			playing = false;

		// Reset the MIDI output so no notes are left playing when the song ends.
		midiOutReset(midioutput);
	}
	else
	{
		// Read the number of ticks to wait.
		while(1)
		{
			midiParm1 = *playpos++;
			waitticks = (waitticks*128) + (midiParm1 & 0x7f);
			if (!(midiParm1 & 0x80))
				break;
		}
	}
}
