//----------------------------------------------------------------------------
//  EDGE WIN32 MUS Handling Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2007  The EDGE Team.
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

#include "i_defs.h"
#include "i_sdlinc.h"

#include "w32_sysinc.h"
#include "e_main.h"
#include "s_music.h"

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

// Timer Control
#define ACTUAL_TIMER_HZ   70  // -AJA- was 140 !!

#define TIMER_RES 7                     // Timer resolution.
static int timerID;                     // Timer ID


static SDL_sem *semaphore;  // thread protection


class w32_midi_player_c : public abstract_music_c
{
private:

	/* TODO : move static vars above in here */

public:
	 w32_midi_player_c() { }
	~w32_midi_player_c() { }

public:
	virtual void Close(void);

	virtual void Play(bool loop);
	virtual void Stop(void);

	virtual void Pause(void);
	virtual void Resume(void);

	virtual void Ticker(void);
	virtual void Volume(float gain);

private:

};


//
// Address of the start of the MUS track.
//
static byte *SongStartAddress(void)
{
	if (!song)
		return 0;

	return (byte*)song + song->scorestart;
}

//
// System Ticker Routine
//
static void I_MUSTicker(void);

void CALLBACK SysTicker(UINT id, UINT msg, DWORD user, DWORD dw1, DWORD dw2)
{
	// Sanity Checks...
	if (!(app_state & APP_STATE_ACTIVE) || !midiavailable || !song || !playing)
		return;

	// -AJA- we don't block on the semaphore, as they may be a
	//       problem for the operating system (the MSDN docs
	//       say this timer event callback is called from a
	//       system multimedia thread).

	if (SDL_SemTryWait(semaphore) != 0)
		return;

	I_MUSTicker();
	I_MUSTicker();

	SDL_SemPost(semaphore);
}


//
// Returns true if no problems.
//
bool I_StartupMUS()
{
	if (midiavailable)
		return true; // Already initialized.

	semaphore = SDL_CreateSemaphore(1);
	SYS_ASSERT(semaphore);

	// Insert code here...
	mixer = I_MusicLoadMixer(MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER);
	if (!mixer)
	{
		I_Printf("I_StartupMUS: Couldn't load the midi mixer\n");
		return false;
	}

	// Open the midi stream.
	MMRESULT res = midiOutOpen(&midioutput, MIDI_MAPPER, 0, 0, CALLBACK_NULL);
	if (res != MMSYSERR_NOERROR)
	{
		I_MusicReleaseMixer(mixer);
		mixer = NULL;
		
		I_Printf("I_StartupMUS: Couldn't open the midi device\n");
		return false;
	}

	if (!I_MusicGetMixerVol(mixer, &mixer->originalvol))
	{
		I_MusicReleaseMixer(mixer);
		mixer = NULL;
		
		midiOutClose(midioutput);

		I_Printf("I_StartupMUS: Unable to get original volume\n");
		return false;
	}
	
	// All is quiet on startup
	I_MusicSetMixerVol(mixer, 0);
    
	// Non-mixer defaults
	playing       = false;
	midiavailable = true;
	song          = NULL;
	playpos       = NULL;

	// Startup music clock
	int clockspeed;

	timeBeginPeriod(TIMER_RES);
	clockspeed = 1000 / ACTUAL_TIMER_HZ;
	timerID = timeSetEvent(clockspeed, TIMER_RES, SysTicker, 0, TIME_PERIODIC);

	return true;
}


void w32_midi_player_c::Close(void)
{
	// FIXME !!!!  ::Close
}

void w32_midi_player_c::Play(bool loop)
{
	// FIXME !!!!  ::Play
}

void w32_midi_player_c::Pause(void)
{
	playing = false;

	// Stop all the notes.
	midiOutReset(midioutput);
}


void w32_midi_player_c::Resume()
{
	playing = true;
}


void w32_midi_player_c::Stop()
{
	byte *data;

	if (!midiavailable)
		return;

	// -AJA- 2005/10/25: must set 'playing' to false _before_ deleting the
	//       song data, to prevent a race condition (I_MUSTicker may get
	//       called in-between deleting the data and setting 'song' to NULL).
	playing = false;
	playpos = NULL;

	// Reset pitchwheel on all channels
	for (int i=0; i <= 15; i++)              
		midiOutShortMsg(midioutput, 0xE0+i); 

	// send RESET_ALL_CONTROLLERS message
	midiOutShortMsg(midioutput, 0xB0+(121<<8));

	midiOutReset(midioutput); 

	// Free resources
	data = (byte*)song;
	delete [] data;
	song = NULL;
}


void w32_midi_player_c::Volume(float gain)
{
	if (!mixer)
		return;

	DWORD actualvol;

	gain = CLAMP(0, gain, 1);

	// Calculate a given value in range
	actualvol = int(gain * (mixer->maxvol - mixer->minvol));
	actualvol += mixer->minvol;

	I_MusicSetMixerVol(mixer, actualvol);
}


void w32_midi_player_c::Ticker()
{
	// this ticker function does nothing.  The 70Hz timer
	// callback (I_MUSTicker) does all the work.
}


void I_ShutdownMUS(void)
{
	if (!midiavailable)
		return;

	midiavailable = false;

	// wait for the timer handler to finish (if active).
	// No need to restore the semaphore since we are shutting down.
	SDL_SemWait(semaphore);

	// Kill timer
	timeKillEvent(timerID);
	timeEndPeriod(TIMER_RES);

//!!!!!	// If there is a registered song, unregister it.
//!!!!!	I_MUSStop();

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
	playing       = false;
	midiavailable = false;
	song          = NULL;
	playpos       = NULL;
}


//
// Called at 140Hz, interprets the MUS data to MIDI events.
//
static void I_MUSTicker(void)
{
	waitticks--;

	if (waitticks > 0)
		return;

	bool scoreEnd = false;

	//
	// We assume playpos is OK. We'll keep playing events until the
	// 'last' bit is set, and then the waitticks is set.
	//
	for (int iloop=0; iloop < 50; iloop++)
	{
		museventdesc_t *evDesc = (museventdesc_t*)playpos++;

		byte midiStatus = 0;
		byte midiChan   = 0;
		byte midiParm1  = 0;
		byte midiParm2  = 0;

		// Construct the MIDI event.
		switch (evDesc->event)
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
				long pitchwheel;

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
				byte scratch = *playpos++;

				midiStatus = 0xb0;
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
				scoreEnd = true;
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
		if (looping)
			playpos = SongStartAddress();
		else
			playing = false;

		// Reset the MIDI output so no notes are left playing when the song ends.
		midiOutReset(midioutput);
		return;
	}

	// Read the number of ticks to wait.
	for (int iloop = 0; iloop < 10; iloop++)
	{
		byte val = *playpos++;
		waitticks = (waitticks * 128) + (val & 0x7f);

		if ((val & 0x80) == 0)
			break;
	}
}


abstract_music_c * I_PlayHWMusic(const byte *data, int length, float  volume, bool loop)
{
	if (!midiavailable)
		return NULL;

//!!!!!	// Kill any previous music
//!!!!!	if (song)
//!!!!!		I_MUSStop();

	if (length < 16 ||
		! (data[0] == 'M' && data[1] == 'U' && data[2] == 'S' && data[3] == 0x1A))
	{
		I_Warning("I_PlayHWMusic: wrong format (not MUS)\n");
		return NULL;
	}

	// !!!!!! FIXME: grab semaphore (SDL_SemWait) to prevent I_MUSTicker running

	song = (musheader_t*) new byte[length];

	w32_midi_player_c *player = new w32_midi_player_c();

	memcpy(song, data, length);

	playpos = SongStartAddress();       // Go to the beginning of the song.
	looping = loop;
	playing = true;

	player->Volume(volume);
	player->Play(loop);

	return player;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
