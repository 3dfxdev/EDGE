//
// Copyright (C) 2016 Alexey Khokholov (Nuke.YKT)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//
//  MUS/MIDI player.
//
// version: 1.0
//

#ifndef OPL_OPLAPI_H
#define OPL_OPLAPI_H
#include "opldef.h"
#include "oplconfig.h"
#include "oplgenmidi.h"
#include "opl3.h"

struct opl_synth_midi_channel {
    Bit8u volume;
    Bit16u volume_t;
    Bit8u pan;
    Bit8u reg_pan;
    Bit8u pitch;
    opl_genmidi_patch *patch;
    bool drum;
};

struct opl_synth_voice {
    Bit8u bank;

    Bit8u op_base;
    Bit8u ch_base;

    Bit32u freq;

    Bit8u tl[2];
    Bit8u additive;

    bool voice_dual;
    opl_genmidi_voice *voice_data;
    opl_genmidi_patch *patch;

    opl_synth_midi_channel *chan;

    Bit8u velocity;
    Bit8u key;
    Bit8u note;

    Bit32s finetune;

    Bit8u pan;

};

struct midi_header
{
    char header[4];
    Bit32u length;
    Bit16u format;
    Bit16u count;
    Bit16u time;
    Bit8u data[1];
};

struct midi_track
{
    char header[4];
    Bit32u length;
    Bit8u data[1];
};

struct track
{
    Bit8u *data;
    Bit8u *pointer;
    Bit32u length;
    Bit32u time;
    Bit8u lastevent;
    bool finish;
    Bit32u num;
};

struct mus_header
{
    char header[4];
    Bit16u length;
    Bit16u offset;
};

class OPL_Player
{
private:
    opl_config config;
    opl_genmidi genmidi;
    bool genmidi_init;
    opl3_chip chip;
    bool opl_opl2mode;
    bool stereo;

    opl_synth_midi_channel opl_synth_midi_channels[16];
    opl_synth_voice opl_synth_voices[18];
    Bit32u opl_synth_voice_num;
    opl_synth_voice *opl_synth_voices_allocated[18];
    Bit32u opl_synth_voices_allocated_num;
    opl_synth_voice *opl_synth_voices_free[18];
    Bit32u opl_synth_voices_free_num;

    Bit32u song_rate;
    bool player_active;
    bool midifile;
    bool player_loop;

    track *mid_tracks;
    Bit32u mid_count;
    Bit32u mid_timebase;
    Bit32u mid_callrate;
    Bit32u mid_timer;
    Bit32u mid_timechange;
    Bit32u mid_rate;
    Bit32u mid_finished;
    Bit8u mid_channels[16];
    Bit8u mid_channelcnt;

    Bit8u *mus_data;
    Bit8u *mus_pointer;
    Bit16u mus_length;
    Bit32u mus_timer;
    Bit32u mus_timeend;
    Bit8u mus_chan_velo[16];

    Bit32u opl_counter;

    Bit8u *songdata;

private:
    Bit16u MISC_Read16LE(Bit16u n);
    Bit16u MISC_Read16BE(Bit16u n);
    Bit32u MISC_Read32LE(Bit32u n);
    Bit32u MISC_Read32BE(Bit32u n);

    void OPL_Synth_Reset_Voice(opl_synth_voice *voice);
    void OPL_Synth_Reset_Chip(opl3_chip *chip);
    void OPL_Synth_Reset_MIDI(opl_synth_midi_channel *channel);
    void OPL_Synth_Init(void);
    void OPL_Synth_WriteReg(Bit32u bank, Bit16u reg, Bit8u data);
    void OPL_Synth_Voice_Off(opl_synth_voice *voice);
    void OPL_Synth_Voice_Freq(opl_synth_voice *voice);
    void OPL_Synth_Voice_Volume(opl_synth_voice *voice);
    Bit8u OPL_Synth_Operator_Setup(Bit32u bank, Bit32u base,
        opl_genmidi_operator *op, bool volume);
    void OPL_Synth_Voice_On(opl_synth_midi_channel *channel,
        opl_genmidi_patch* patch, bool dual, Bit8u key, Bit8u velocity);
    void OPL_Synth_Kill_Voice(void);
    void OPL_Synth_Note_Off(opl_synth_midi_channel *channel, Bit8u note);
    void OPL_Synth_Note_On(opl_synth_midi_channel *channel, Bit8u note,
        Bit8u velo);
    void OPL_Synth_PitchBend(opl_synth_midi_channel *channel, Bit8u pitch);
    void OPL_Synth_UpdatePatch(opl_synth_midi_channel *channel, Bit8u patch);
    void OPL_Synth_UpdatePan(opl_synth_midi_channel *channel, Bit8u pan);
    void OPL_Synth_UpdateVolume(opl_synth_midi_channel *channel, Bit8u volume);
    void OPL_Synth_NoteOffAll(opl_synth_midi_channel *channel);
    void OPL_Synth_EventReset(opl_synth_midi_channel *channel);
    void OPL_Synth_Reset(void);
    void OPL_Synth_Write(Bit8u command, Bit8u data1, Bit8u data2);

    Bit32u MIDI_ReadDelay(Bit8u **data);
    bool MIDI_LoadSong(void *data);
    bool MIDI_StartSong(void);
    void MIDI_StopSong(void);
    track* MIDI_NextTrack(void);
    Bit8u MIDI_GetChannel(Bit8u chan);
    void MIDI_Command(Bit8u **datap, Bit8u evnt);
    void MIDI_FinishTrack(track *trck);
    void MIDI_AdvanceTrack(track *trck);
    void MIDI_Callback(void);

    void MUS_Callback(void);
    bool MUS_LoadSong(void *data);
    bool MUS_StartSong(void);
    void MUS_StopSong(void);

    void Player_Init(void);

public:

    OPL_Player();
    ~OPL_Player();

    void OPL_SendConfig(opl_config config);
    bool OPL_LoadGENMIDI(void *data);
    bool OPL_LoadSong(void *data, Bit32u length);
    void OPL_SetMUSRate(Bit32u rate);
    bool OPL_PlaySong(void);
    void OPL_StopSong(void);
    void OPL_Generate(Bit16s *buffer, Bit32u length);
    void OPL_Loop(bool loop);
};

#endif
