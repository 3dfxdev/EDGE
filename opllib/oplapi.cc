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
#include <stdlib.h>
#include <string.h>
#include "oplapi.h"
#include "opldef.h"
#include "oplconfig.h"
#include "oplgenmidi.h"
#include "opltables.h"
#include "opl3.h"

opl_config opl2 = { 49716, false, true };

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Misc                                                                       //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////


Bit16u OPL_Player::MISC_Read16LE(Bit16u n)
{
    Bit8u *m = (Bit8u*)&n;
    return m[0] | (m[1] << 8);
}

Bit16u OPL_Player::MISC_Read16BE(Bit16u n)
{
    Bit8u *m = (Bit8u*)&n;
    return m[1] | (m[0] << 8);
}

Bit32u OPL_Player::MISC_Read32LE(Bit32u n)
{
    Bit8u *m = (Bit8u*)&n;
    return m[0] | (m[1] << 8) | (m[2] << 16) | (m[3] << 24);
}

Bit32u OPL_Player::MISC_Read32BE(Bit32u n)
{
    Bit8u *m = (Bit8u*)&n;
    return m[3] | (m[2] << 8) | (m[1] << 16) | (m[0] << 24);
}
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// OPL Synth                                                                  //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

typedef enum {
    OPL_SYNTH_CMD_NOTE_OFF,
    OPL_SYNTH_CMD_NOTE_ON,
    OPL_SYNTH_CMD_PITCH_BEND,
    OPL_SYNTH_CMD_PATCH,
    OPL_SYNTH_CMD_CONTROL
} opl_synth_cmd;

typedef enum {
    OPL_SYNTH_CONTROL_BANK,
    OPL_SYNTH_CONTROL_MODULATION,
    OPL_SYNTH_CONTROL_VOLUME,
    OPL_SYNTH_CONTROL_PAN,
    OPL_SYNTH_CONTROL_EXPRESSION,
    OPL_SYNTH_CONTROL_REVERB,
    OPL_SYNTH_CONTROL_CHORUS,
    OPL_SYNTH_CONTROL_SUSTAIN,
    OPL_SYNTH_CONTROL_SOFT,
    OPL_SYNTH_CONTROL_ALL_NOTE_OFF,
    OPL_SYNTH_CONTROL_MONO_MODE,
    OPL_SYNTH_CONTROL_POLY_MODE,
    OPL_SYNTH_CONTROL_RESET
} opl_synth_control;


void OPL_Player::OPL_Synth_Reset_Voice(opl_synth_voice *voice)
{
    voice->freq = 0;
    voice->voice_dual = false;
    voice->voice_data = NULL;
    voice->patch = NULL;
    voice->chan = NULL;
    voice->velocity = 0;
    voice->key = 0;
    voice->note = 0;
    voice->pan = 0x30;
    voice->finetune = 0;
    voice->tl[0] = 0x3f;
    voice->tl[1] = 0x3f;
}

void OPL_Player::OPL_Synth_Reset_Chip(opl3_chip *chip)
{
    for (Bit32u i = 0x40; i < 0x56; i++)
    {
        OPL3_WriteReg(chip, i, 0x3f);
    }

    for (Bit32u i = 0x60; i < 0xf6; i++)
    {
        OPL3_WriteReg(chip, i, 0x00);
    }

    for (Bit32u i = 0x01; i < 0x40; i++)
    {
        OPL3_WriteReg(chip, i, 0x00);
    }

    OPL3_WriteReg(chip, 0x01, 0x20);

    if (!opl_opl2mode)
    {
        OPL3_WriteReg(chip, 0x105, 0x01);
        for (Bit32u i = 0x140; i < 0x156; i++)
        {
            OPL3_WriteReg(chip, i, 0x3f);
        }

        for (Bit32u i = 0x160; i < 0x1f6; i++)
        {
            OPL3_WriteReg(chip, i, 0x00);
        }

        for (Bit32u i = 0x101; i < 0x140; i++)
        {
            OPL3_WriteReg(chip, i, 0x00);
        }
        OPL3_WriteReg(chip, 0x105, 0x01);
    }
}

void OPL_Player::OPL_Synth_Reset_MIDI(opl_synth_midi_channel *channel)
{
    channel->volume = 100;
    channel->volume_t = opl_voltable[channel->volume] + 1;
    channel->pan = 64;
    channel->reg_pan = 0x30;
    channel->pitch = 64;
    channel->patch = &genmidi.patch[0];
    channel->drum = false;
    if (channel == &opl_synth_midi_channels[15])
    {
        channel->drum = true;
    }
}

void OPL_Player::OPL_Synth_Init(void)
{
    for (Bit32u i = 0; i < 18; i++)
    {
        opl_synth_voices[i].bank = i / 9;
        opl_synth_voices[i].op_base = opl_slotoffset[i % 9];
        opl_synth_voices[i].ch_base = i % 9;
        OPL_Synth_Reset_Voice(&opl_synth_voices[i]);
    }


    for (Bit32u i = 0; i < 16; i++)
    {
        OPL_Synth_Reset_MIDI(&opl_synth_midi_channels[i]);
    }

    OPL_Synth_Reset_Chip(&chip);

    if (opl_opl2mode)
    {
        opl_synth_voice_num = 9;
    }
    else
    {
        opl_synth_voice_num = 18;
    }

    for (Bit8u i = 0; i < opl_synth_voice_num; i++)
    {
        opl_synth_voices_free[i] = &opl_synth_voices[i];
    }

    opl_synth_voices_allocated_num = 0;
    opl_synth_voices_free_num = opl_synth_voice_num;
}


void OPL_Player::OPL_Synth_WriteReg(Bit32u bank, Bit16u reg, Bit8u data)
{
    reg |= bank << 8;

    OPL3_WriteReg(&chip, reg, data);
}


void OPL_Player::OPL_Synth_Voice_Off(opl_synth_voice *voice)
{
    OPL_Synth_WriteReg(voice->bank, 0xb0 + voice->ch_base, voice->freq >> 8);
    voice->freq = 0;
    for (Bit32u i = 0; i < opl_synth_voices_allocated_num; i++)
    {
        if (opl_synth_voices_allocated[i] == voice)
        {
            for (Bit32u j = i; j < opl_synth_voices_allocated_num - 1; j++)
            {
                opl_synth_voices_allocated[j] =
                    opl_synth_voices_allocated[j + 1];
            }
            break;
        }
    }
    opl_synth_voices_allocated_num--;
    opl_synth_voices_free[opl_synth_voices_free_num++] = voice;
}

void OPL_Player::OPL_Synth_Voice_Freq(opl_synth_voice *voice)
{
    Bit32s freq;
    Bit32u block;
    
    freq = voice->chan->pitch + voice->finetune + 32 * voice->note;
    block = 0;
    
    if (freq < 0)
    {
        freq = 0;
    }
    else if (freq >= 284)
    {
        freq -= 284;
        block = freq / 384;
        if (block > 7)
        {
            block = 7;
        }
        freq %= 384;
        freq += 284;
    }

    freq = (block << 10) | opl_freqtable[freq];
    
    OPL_Synth_WriteReg(voice->bank, 0xa0 + voice->ch_base, freq & 0xff);
    OPL_Synth_WriteReg(voice->bank, 0xb0 + voice->ch_base, (freq >> 8) | 0x20);

    voice->freq = freq;
}

void OPL_Player::OPL_Synth_Voice_Volume(opl_synth_voice *voice)
{
    Bit8u volume = (Bit8u)(0x3f
                 - (voice->chan->volume_t * opl_voltable[voice->velocity])
                 / 256);

    if ((voice->tl[0] & 0x3f) != volume)
    {
        voice->tl[0] = (voice->tl[0] & 0xc0) | volume;
        OPL_Synth_WriteReg(voice->bank, 0x43 + voice->op_base, voice->tl[0]);
        if (voice->additive)
        {
            Bit8u volume2 = 0x3f - voice->additive;

            if (volume2 < volume)
            {
                volume2 = volume;
            }

            volume2 |= voice->tl[1] & 0xc0;

            if (volume2 != voice->tl[1])
            {
                voice->tl[1] = volume2;
                OPL_Synth_WriteReg(voice->bank, 0x40 + voice->op_base,
                    voice->tl[1]);
            }
        }
    }

}

Bit8u OPL_Player::OPL_Synth_Operator_Setup(Bit32u bank, Bit32u base,
    opl_genmidi_operator *op, bool volume)
{
    Bit8u tl = op->ksl;
    if (volume)
    {
        tl |= 0x3f;
    }
    else
    {
        tl |= op->level;
    }
    OPL_Synth_WriteReg(bank, 0x40 + base, tl);
    OPL_Synth_WriteReg(bank, 0x20 + base, op->mult);
    OPL_Synth_WriteReg(bank, 0x60 + base, op->attack);
    OPL_Synth_WriteReg(bank, 0x80 + base, op->sustain);
    OPL_Synth_WriteReg(bank, 0xE0 + base, op->wave);

    return tl;
}

void OPL_Player::OPL_Synth_Voice_On(opl_synth_midi_channel *channel,
    opl_genmidi_patch* patch, bool dual, Bit8u key, Bit8u velocity)
{
    opl_synth_voice *voice;
    opl_genmidi_voice *voice_data;
    Bit32u bank;
    Bit32u base;
    Bit32s note;

    if (opl_synth_voices_free_num == 0)
    {
        return;
    }

    voice = opl_synth_voices_free[0];

    opl_synth_voices_free_num--;

    for (Bit32u i = 0; i < opl_synth_voices_free_num; i++)
    {
        opl_synth_voices_free[i] = opl_synth_voices_free[i + 1];
    }

    opl_synth_voices_allocated[opl_synth_voices_allocated_num++] = voice;

    voice->chan = channel;
    voice->key = key;
    voice->velocity = velocity;
    voice->patch = patch;
    voice->voice_dual = dual;

    if (dual)
    {
        voice_data = &patch->voice[1];
        voice->finetune = (Bit32s)(patch->finetune >> 1) - 64;
    }
    else
    {
        voice_data = &patch->voice[0];
        voice->finetune = 0;
    }

    voice->pan = channel->reg_pan;

    if (voice->voice_data != voice_data)
    {
        voice->voice_data = voice_data;
        bank = voice->bank;
        base = voice->op_base;

        voice->tl[0] = OPL_Synth_Operator_Setup(bank, base + 3,
            &voice_data->car, true);

        if (voice_data->mod.feedback & 1)
        {
            voice->additive = 0x3f - voice_data->mod.level;
            voice->tl[1] = OPL_Synth_Operator_Setup(bank, base,
                &voice_data->mod, true);
        }
        else
        {
            voice->additive = 0;
            voice->tl[1] = OPL_Synth_Operator_Setup(bank, base,
                &voice_data->mod, false);
        }

        OPL_Synth_WriteReg(bank, 0xc0 + voice->ch_base,
            voice_data->mod.feedback | voice->pan);
    }

    if (MISC_Read16LE(patch->flags) & OPL_GENMIDI_FIXED)
    {
        note = patch->note;
    }
    else
    {
        if (channel->drum)
        {
            note = 60;
        }
        else
        {
            note = key;
            note += (Bit16s)MISC_Read16LE((Bit16u)voice_data->offset);
            while (note < 0)
            {
                note += 12;
            }
            while (note > 95)
            {
                note -= 12;
            }
        }
    }
    voice->note = note;

    OPL_Synth_Voice_Volume(voice);
    OPL_Synth_Voice_Freq(voice);
}

void OPL_Player::OPL_Synth_Kill_Voice(void)
{
    opl_synth_voice *voice;

    if (opl_synth_voices_free_num > 0)
    {
        return;
    }
    
    voice = opl_synth_voices_allocated[0];

    for (Bit32u i = 0; i < opl_synth_voices_allocated_num; i++)
    {
        if (opl_synth_voices_allocated[i]->voice_dual
            || opl_synth_voices_allocated[i]->chan >= voice->chan)
        {
            voice = opl_synth_voices_allocated[i];
        }
    }

    OPL_Synth_Voice_Off(voice);
}

void OPL_Player::OPL_Synth_Note_Off(opl_synth_midi_channel *channel,
    Bit8u note)
{
    for (Bit32u i = 0; i < opl_synth_voices_allocated_num;)
    {
        if (opl_synth_voices_allocated[i]->chan == channel
         && opl_synth_voices_allocated[i]->key == note)
        {
            OPL_Synth_Voice_Off(opl_synth_voices_allocated[i]);
        }
        else
        {
            i++;
        }
    }
}

void OPL_Player::OPL_Synth_Note_On(opl_synth_midi_channel *channel, Bit8u note,
    Bit8u velo)
{
    opl_genmidi_patch *patch;

    if (velo == 0)
    {
        OPL_Synth_Note_Off(channel, note);
        return;
    }

    if (channel->drum)
    {
        if (note < 35 || note > 81)
        {
            return;
        }
        patch = &genmidi.patch[note - 35 + 128];
    }
    else
    {
        patch = channel->patch;
    }

    OPL_Synth_Kill_Voice();

    OPL_Synth_Voice_On(channel, patch, false, note, velo);

    if (opl_synth_voices_free_num > 0
        && MISC_Read16LE(patch->flags) & OPL_GENMIDI_DUAL)
    {
        OPL_Synth_Voice_On(channel, patch, true, note, velo);
    }
}

void OPL_Player::OPL_Synth_PitchBend(opl_synth_midi_channel *channel,
    Bit8u pitch)
{
    opl_synth_voice *opl_synth_channel_voices[18];
    opl_synth_voice *opl_synth_other_voices[18];

    Bit32u cnt1 = 0;
    Bit32u cnt2 = 0;

    channel->pitch = pitch;

    for (Bit32u i = 0; i < opl_synth_voices_allocated_num; i++)
    {
        if (opl_synth_voices_allocated[i]->chan == channel)
        {
            OPL_Synth_Voice_Freq(opl_synth_voices_allocated[i]);
            opl_synth_channel_voices[cnt1++] = opl_synth_voices_allocated[i];
        }
        else
        {
            opl_synth_other_voices[cnt2++] = opl_synth_voices_allocated[i];
        }
    }

    for (Bit32u i = 0; i < cnt2; i++)
    {
        opl_synth_voices_allocated[i] = opl_synth_other_voices[i];
    }

    for (Bit32u i = 0; i < cnt1; i++)
    {
        opl_synth_voices_allocated[i + cnt2] = opl_synth_channel_voices[i];
    }
}

void OPL_Player::OPL_Synth_UpdatePatch(opl_synth_midi_channel *channel,
    Bit8u patch)
{
    channel->patch = &genmidi.patch[patch];
}

void OPL_Player::OPL_Synth_UpdatePan(opl_synth_midi_channel *channel,
    Bit8u pan)
{
    Bit8u new_pan = 0x30;

    if (pan <= 48)
    {
        new_pan = 0x20;
    }
    else if(pan >= 96)
    {
        new_pan = 0x10;
    }

    channel->pan = pan;

    if (channel->reg_pan != new_pan)
    {
        channel->reg_pan = new_pan;
        for (Bit32u i = 0; i < opl_synth_voices_allocated_num; i++)
        {
            if (opl_synth_voices_allocated[i]->chan == channel)
            {
                opl_synth_voices_allocated[i]->pan = new_pan;
                OPL_Synth_WriteReg(opl_synth_voices_allocated[i]->bank,
                    0xc0 + opl_synth_voices_allocated[i]->ch_base,
                    opl_synth_voices_allocated[i]->voice_data->mod.feedback | new_pan);
            }
        }
    }
}

void OPL_Player::OPL_Synth_UpdateVolume(opl_synth_midi_channel *channel, Bit8u volume)
{
    if (volume & 0x80)
    {
        volume = 0x7f;
    }
    if (channel->volume != volume)
    {
        channel->volume = volume;
        channel->volume_t = opl_voltable[channel->volume] + 1;
        for (Bit32u i = 0; i < opl_synth_voices_allocated_num; i++)
        {
            if (opl_synth_voices_allocated[i]->chan == channel)
            {
                OPL_Synth_Voice_Volume(opl_synth_voices_allocated[i]);
            }
        }
    }
}

void OPL_Player::OPL_Synth_NoteOffAll(opl_synth_midi_channel *channel)
{
    for (Bit32u i = 0; i < opl_synth_voices_allocated_num;)
    {
        if (opl_synth_voices_allocated[i]->chan == channel)
        {
            OPL_Synth_Voice_Off(opl_synth_voices_allocated[i]);
        }
        else
        {
            i++;
        }
    }
}

void OPL_Player::OPL_Synth_EventReset(opl_synth_midi_channel *channel)
{
    OPL_Synth_NoteOffAll(channel);
    channel->reg_pan = 0x30;
    channel->pan = 64;
    channel->pitch = 64;
}

void OPL_Player::OPL_Synth_Reset(void)
{
    for (Bit8u i = 0; i < 16; i++)
    {
        OPL_Synth_NoteOffAll(&opl_synth_midi_channels[i]);
        OPL_Synth_Reset_MIDI(&opl_synth_midi_channels[i]);
    }
}

void OPL_Player::OPL_Synth_Write(Bit8u command, Bit8u data1, Bit8u data2)
{
    opl_synth_midi_channel *channel = &opl_synth_midi_channels[command & 0x0f];
    command >>= 4;

    switch (command)
    {
    case OPL_SYNTH_CMD_NOTE_OFF:
        OPL_Synth_Note_Off(channel, data1);
        break;
    case OPL_SYNTH_CMD_NOTE_ON:
        OPL_Synth_Note_On(channel, data1, data2);
        break;
    case OPL_SYNTH_CMD_PITCH_BEND:
        OPL_Synth_PitchBend(channel, data2);
        break;
    case OPL_SYNTH_CMD_PATCH:
        OPL_Synth_UpdatePatch(channel, data1);
        break;
    case OPL_SYNTH_CMD_CONTROL:
        switch (data1)
        {
        case OPL_SYNTH_CONTROL_VOLUME:
            OPL_Synth_UpdateVolume(channel, data2);
            break;
        case OPL_SYNTH_CONTROL_PAN:
            OPL_Synth_UpdatePan(channel, data2);
            break;
        case OPL_SYNTH_CONTROL_ALL_NOTE_OFF:
            OPL_Synth_NoteOffAll(channel);
            break;
        case OPL_SYNTH_CONTROL_RESET:
            OPL_Synth_EventReset(channel);
            break;
        }
        break;
    }
}


////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Player                                                                     //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

// MIDI
char midh[4] = { 'M', 'T', 'h', 'd' };
char mtrkh[4] = { 'M', 'T', 'r', 'k' };

Bit32u OPL_Player::MIDI_ReadDelay(Bit8u **data)
{
    Bit8u *dn = *data;
    Bit32u delay = 0;
    do
    {
        delay = (delay << 7) | ((*dn) & 0x7f);
    } while (*dn++ & 0x80);
    *data = dn;
    return delay;
}

bool OPL_Player::MIDI_LoadSong(void *data)
{
    midi_header *mid = (midi_header*)data;
    Bit8u *midi_data;

    if (memcmp(mid->header, midh, 4) != 0
        || MISC_Read32BE(mid->length) != 6)
    {
        return false;
    }

    mid_count = MISC_Read16BE(mid->count);
    midi_data = mid->data;
    mid_timebase = MISC_Read16BE(mid->time);

    if (mid_tracks)
    {
        delete[] mid_tracks;
    }

    mid_tracks = new track[mid_count];
    if (!mid_tracks)
    {
        return false;
    }

    for (Bit32u i = 0; i < mid_count; i++)
    {
        midi_track *track = (midi_track*)midi_data;
        if (memcmp(track->header, mtrkh, 4) != 0)
        {
            free(mid_tracks);
            return 1;
        }
        mid_tracks[i].length = MISC_Read32BE(track->length);
        mid_tracks[i].data = track->data;
        midi_data += mid_tracks[i].length + 8;
        mid_tracks[i].num = i;
    }

    midifile = true;

    return true;
}

bool OPL_Player::MIDI_StartSong(void)
{
    if (!midifile || player_active)
    {
        return false;
    }

    for (Bit32u i = 0; i < mid_count; i++)
    {
        mid_tracks[i].pointer = mid_tracks[i].data;
        mid_tracks[i].time = MIDI_ReadDelay(&mid_tracks[i].pointer);
        mid_tracks[i].lastevent = 0x80;
        mid_tracks[i].finish = 0;
    }

    for (Bit32u i = 0; i < 16; i++)
    {
        mid_channels[i] = 0xff;
    }

    mid_channelcnt = 0;

    mid_rate = 1000000 / (500000 / mid_timebase);
    mid_callrate = song_rate;
    mid_timer = 0;
    mid_timechange = 0;
    mid_finished = 0;

    player_active = true;

    return true;
}

void OPL_Player::MIDI_StopSong(void)
{
    if (!midifile || !player_active)
    {
        return;
    }

    player_active = false;

    for (Bit32u i = 0; i < 16; i++)
    {
        OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | i,
            OPL_SYNTH_CONTROL_ALL_NOTE_OFF, 0);
    }

    OPL_Synth_Reset();
}

track* OPL_Player::MIDI_NextTrack(void)
{
    track *mintrack = &mid_tracks[0];
    for (Bit32u i = 1; i < mid_count; i++)
    {
        if ((mid_tracks[i].time < mintrack->time && !mid_tracks[i].finish)
            || mintrack->finish)
        {
            mintrack = &mid_tracks[i];
        }
    }
    return mintrack;
}

Bit8u OPL_Player::MIDI_GetChannel(Bit8u chan)
{
    if (chan == 9)
    {
        return 15;
    }
    
    if (mid_channels[chan] == 0xff)
    {
        mid_channels[chan] = mid_channelcnt++;
    }

    return mid_channels[chan];
}

void OPL_Player::MIDI_Command(Bit8u **datap, Bit8u evnt)
{
    Bit8u chan;
    Bit8u *data;
    Bit8u v1, v2;

    data = *datap;

    chan = MIDI_GetChannel(evnt & 0x0f);

    switch (evnt & 0xf0)
    {
    case 0x80:
        v1 = *data++;
        v2 = *data++;
        OPL_Synth_Write((OPL_SYNTH_CMD_NOTE_OFF << 4) | chan, v1, 0);
        break;
    case 0x90:
        v1 = *data++;
        v2 = *data++;
        OPL_Synth_Write((OPL_SYNTH_CMD_NOTE_ON << 4) | chan, v1, v2);
        break;
    case 0xa0:
        data += 2;
        break;
    case 0xb0:
        v1 = *data++;
        v2 = *data++;
        switch (v1)
        {
        case 0x00:
        case 0x20:
            OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | chan,
                OPL_SYNTH_CONTROL_BANK, v2);
            break;
        case 0x01:
            OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | chan,
                OPL_SYNTH_CONTROL_MODULATION, v2);
            break;
        case 0x07:
            OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | chan,
                OPL_SYNTH_CONTROL_VOLUME, v2);
            break;
        case 0x0a:
            OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | chan,
                OPL_SYNTH_CONTROL_PAN, v2);
            break;
        case 0x0b:
            OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | chan,
                OPL_SYNTH_CONTROL_EXPRESSION, v2);
            break;
        case 0x40:
            OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | chan,
                OPL_SYNTH_CONTROL_SUSTAIN, v2);
            break;
        case 0x43:
            OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | chan,
                OPL_SYNTH_CONTROL_SOFT, v2);
            break;
        case 0x5b:
            OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | chan,
                OPL_SYNTH_CONTROL_REVERB, v2);
            break;
        case 0x5d:
            OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | chan,
                OPL_SYNTH_CONTROL_CHORUS, v2);
            break;
        case 0x78:
            OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | chan,
                OPL_SYNTH_CONTROL_ALL_NOTE_OFF, v2);
            break;
        case 0x79:
            OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | chan,
                OPL_SYNTH_CONTROL_RESET, v2);
            break;
        case 0x7b:
            OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | chan,
                OPL_SYNTH_CONTROL_ALL_NOTE_OFF, v2);
            break;
        case 0x7e:
            OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | chan,
                OPL_SYNTH_CONTROL_MONO_MODE, v2);
            break;
        case 0x7f:
            OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | chan,
                OPL_SYNTH_CONTROL_POLY_MODE, v2);
            break;
        }
        break;
    case 0xc0:
        v1 = *data++;
        OPL_Synth_Write((OPL_SYNTH_CMD_PATCH << 4) | chan, v1, 0);
        break;
    case 0xd0:
        data += 1;
        break;
    case 0xe0:
        v1 = *data++;
        v2 = *data++;
        OPL_Synth_Write((OPL_SYNTH_CMD_PITCH_BEND << 4) | chan, v1, v2);
        break;
    }

    *datap = data;
}

void OPL_Player::MIDI_FinishTrack(track *trck)
{
    if (trck->finish)
    {
        return;
    }
    trck->finish = true;
    mid_finished++;
}

void OPL_Player::MIDI_AdvanceTrack(track *trck)
{
    Bit8u evnt;
    Bit8u meta;
    Bit8u length;
    Bit32u tempo;
    Bit8u *data;

    evnt = *trck->pointer++;

    if (!(evnt & 0x80))
    {
        evnt = trck->lastevent;
        trck->pointer--;
    }

    switch (evnt)
    {
    case 0xf0:
    case 0xf7:
        length = MIDI_ReadDelay(&trck->pointer);
        trck->pointer += length;
        break;
    case 0xff:
        meta = *trck->pointer++;
        length = MIDI_ReadDelay(&trck->pointer);
        data = trck->pointer;
        trck->pointer += length;
        switch (meta)
        {
        case 0x2f:
            MIDI_FinishTrack(trck);
            break;
        case 0x51:
            if (length == 0x03)
            {
                tempo = (data[0] << 16) | (data[1] << 8) | data[2];
                mid_timechange += (mid_timer * mid_rate) / mid_callrate;
                mid_timer = 0;
                mid_rate = 1000000 / (tempo / mid_timebase);
            }
            break;
        }
        break;
    default:
        MIDI_Command(&trck->pointer,evnt);
        break;
    }

    trck->lastevent = evnt;
    if (trck->pointer >= trck->data + trck->length)
    {
        MIDI_FinishTrack(trck);
    }
}

void OPL_Player::MIDI_Callback(void)
{
    track *trck;

    if (!midifile || !player_active)
    {
        return;
    }

    while (true)
    {
        trck = MIDI_NextTrack();
        if (trck->finish || trck->time >
            mid_timechange + (mid_timer * mid_rate) / mid_callrate)
        {
            break;
        }
        MIDI_AdvanceTrack(trck);
        if (!trck->finish)
        {
            trck->time += MIDI_ReadDelay(&trck->pointer);
        }
    }

    mid_timer++;

    if (mid_finished == mid_count)
    {
        if (!player_loop)
        {
            MIDI_StopSong();
        }
        for (Bit32u i = 0; i < mid_count; i++)
        {
            mid_tracks[i].pointer = mid_tracks[i].data;
            mid_tracks[i].time = MIDI_ReadDelay(&mid_tracks[i].pointer);
            mid_tracks[i].lastevent = 0x80;
            mid_tracks[i].finish = 0;
        }

        for (Bit32u i = 0; i < 16; i++)
        {
            mid_channels[i] = 0xff;
        }

        mid_channelcnt = 0;

        mid_rate = 1000000 / (500000 / mid_timebase);
        mid_timer = 0;
        mid_timechange = 0;
        mid_finished = 0;

        OPL_Synth_Reset();
    }
}

// MUS

char mush[4] = { 'M', 'U', 'S', 0x1a };

void OPL_Player::MUS_Callback(void)
{
    if (midifile || !player_active)
    {
        return;
    }

    while (mus_timer == mus_timeend)
    {
        Bit8u cmd;
        Bit8u evnt;
        Bit8u chan;
        Bit8u data1;
        Bit8u data2;

        cmd = *mus_pointer++;
        chan = cmd & 0x0f;
        evnt = (cmd >> 4) & 7;

        switch (evnt)
        {
        case 0x00:
            data1 = *mus_pointer++;
            OPL_Synth_Write((OPL_SYNTH_CMD_NOTE_OFF << 4) | chan, data1, 0);
            break;
        case 0x01:
            data1 = *mus_pointer++;
            if (data1 & 0x80)
            {
                data1 &= 0x7f;
                mus_chan_velo[chan] = *mus_pointer++;
            }
            OPL_Synth_Write((OPL_SYNTH_CMD_NOTE_ON << 4) | chan, data1,
                mus_chan_velo[chan]);
            break;
        case 0x02:
            data1 = *mus_pointer++;
            OPL_Synth_Write((OPL_SYNTH_CMD_PITCH_BEND << 4) | chan, (data1&1) << 6, data1 >> 1);
            break;
        case 0x03:
        case 0x04:
            data1 = *mus_pointer++;
            data2 = 0;
            if (evnt == 0x04)
            {
                data2 = *mus_pointer++;
            }
            switch (data1)
            {
            case 0x00:
                OPL_Synth_Write((OPL_SYNTH_CMD_PATCH << 4) | chan, data2, 0);
                break;
            case 0x01:
                OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | chan,
                    OPL_SYNTH_CONTROL_BANK, data2);
                break;
            case 0x02:
                OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | chan,
                    OPL_SYNTH_CONTROL_MODULATION, data2);
                break;
            case 0x03:
                OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | chan,
                    OPL_SYNTH_CONTROL_VOLUME, data2);
                break;
            case 0x04:
                OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | chan,
                    OPL_SYNTH_CONTROL_PAN, data2);
                break;
            case 0x05:
                OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | chan,
                    OPL_SYNTH_CONTROL_EXPRESSION, data2);
                break;
            case 0x06:
                OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | chan,
                    OPL_SYNTH_CONTROL_REVERB, data2);
                break;
            case 0x07:
                OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | chan,
                    OPL_SYNTH_CONTROL_CHORUS, data2);
                break;
            case 0x08:
                OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | chan,
                    OPL_SYNTH_CONTROL_SUSTAIN, data2);
                break;
            case 0x09:
                OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | chan,
                    OPL_SYNTH_CONTROL_SOFT, data2);
                break;
            case 0x0a:
                OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | chan,
                    OPL_SYNTH_CONTROL_ALL_NOTE_OFF, data2);
                break;
            case 0x0b:
                OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | chan,
                    OPL_SYNTH_CONTROL_ALL_NOTE_OFF, data2);
                break;
            case 0x0c:
                OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | chan,
                    OPL_SYNTH_CONTROL_MONO_MODE, data2);
                break;
            case 0x0d:
                OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | chan,
                    OPL_SYNTH_CONTROL_POLY_MODE, data2);
                break;
            case 0x0e:
                OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | chan,
                    OPL_SYNTH_CONTROL_RESET, data2);
                break;
            case 0x0f:
                break;
            }
            break;
        case 0x05:
            break;
        case 0x06:
            if (!player_loop)
            {
                MUS_StopSong();
                return;
            }
            mus_pointer = mus_data;
            cmd = 0;
            OPL_Synth_Reset();
            break;
        case 0x07:
            mus_pointer++;
            break;
        }

        if (cmd & 0x80)
        {
            mus_timeend += MIDI_ReadDelay(&mus_pointer);
            break;
        }
    }
    mus_timer++;
}

bool OPL_Player::MUS_LoadSong(void *data)
{
    mus_header *mus = (mus_header*)data;

    if (memcmp(mus->header, mush, 4) != 0)
    {
        return false;
    }

    mus_length = MISC_Read16LE(mus->length);
    mus_data = &((Bit8u*)data)[MISC_Read16LE(mus->offset)];

    midifile = false;

    return true;
}

bool OPL_Player::MUS_StartSong(void)
{
    if (midifile || player_active)
    {
        return 1;
    }

    mus_pointer = mus_data;
    mus_timer = 0;
    mus_timeend = 0;

    player_active = true;

    return 0;
}

void OPL_Player::MUS_StopSong(void)
{
    if (midifile || !player_active)
    {
        return;
    }

    player_active = false;

    for (Bit32u i = 0; i < 16; i++)
    {
        OPL_Synth_Write((OPL_SYNTH_CMD_CONTROL << 4) | i,
            OPL_SYNTH_CONTROL_ALL_NOTE_OFF, 0);
    }

    OPL_Synth_Reset();
}

void OPL_Player::Player_Init(void)
{
    song_rate = 140;
    player_active = false;
    midifile = false;

    player_loop = true;

    mid_tracks = NULL;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// API                                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

OPL_Player::OPL_Player()
{
    OPL3_Reset(&chip);

    genmidi_init = false;

    songdata = NULL;

    OPL_SendConfig(opl2);

    OPL_Synth_Init();

    Player_Init();

    opl_counter = 0;
}

OPL_Player::~OPL_Player()
{
    if (mid_tracks)
    {
        delete[] mid_tracks;
    }

    if (songdata)
    {
        delete[] songdata;
    }
}

void OPL_Player::OPL_SendConfig(opl_config conf)
{
    OPL3_SetRate(&chip, conf.samplerate);

    config = conf;

    opl_opl2mode = !conf.opl3mode;

    stereo = conf.stereo;

    OPL_Synth_Init();
}

char *genmidi_head = "#OPL_II#";

bool OPL_Player::OPL_LoadGENMIDI(void *data)
{
    if (memcmp(data, genmidi_head, 8) != 0)
    {
        return false;
    }

    memcpy(&genmidi, data, sizeof(opl_genmidi));

    genmidi_init = true;

    return true;
}

bool OPL_Player::OPL_LoadSong(void *data, Bit32u length)
{
    if (player_active || !genmidi_init)
    {
        return 0;
    }

    if (songdata)
    {
        delete[] songdata;
    }

    songdata = new Bit8u[length];

    if (!songdata)
    {
        return false;
    }

    memcpy(songdata, data, length);

    if (MUS_LoadSong(songdata) ||MIDI_LoadSong(songdata))
    {
        return true;
    }

    return false;
}

void OPL_Player::OPL_SetMUSRate(Bit32u rate)
{
    song_rate = rate;
}

bool OPL_Player::OPL_PlaySong(void)
{
    if (midifile)
    {
        return MIDI_StartSong();
    }

    return MUS_StartSong();
}

void OPL_Player::OPL_StopSong(void)
{
    if (midifile)
    {
        MIDI_StopSong();
        return;
    }

    MUS_StopSong();
}

void OPL_Player::OPL_Generate(Bit16s *buffer, Bit32u length)
{
    for (Bit32u i = 0; i < length; i++)
    {
        Bit16s accm[2];

        while (opl_counter >= (Bit32u)config.samplerate)
        {
            if (player_active)
            {
                if (midifile)
                {
                    MIDI_Callback();
                }
                else
                {
                    MUS_Callback();
                }
            }
            opl_counter -= config.samplerate;
        }

        opl_counter += song_rate;

        OPL3_GenerateResampled(&chip, accm);
        if (stereo)
        {
            buffer[i * 2] = accm[0];
            buffer[i * 2 + 1] = accm[1];
        }
        else
        {
            buffer[i] = (accm[0] + accm[1]) / 2;
        }
    }
}

void OPL_Player::OPL_Loop(bool loop)
{
    player_loop = loop;
}
