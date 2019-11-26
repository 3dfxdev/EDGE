//----------------------------------------------------------------------------
//  EDGE2 Sound System
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2009  The EDGE2 Team.
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

#include <kos.h>

#include "i_defs.h"

#include "dm_state.h"
#include "m_argv.h"
#include "m_misc.h"
#include "m_random.h"
#include "w_wad.h"
#include "e_main.h"
#include "r_image.h"

#include "s_sound.h"

#include "../epi/file.h"
#include "../epi/filesystem.h"
#include "../epi/file_memory.h"
#include "../epi/sound_data.h"
#include "../epi/sound_wav.h"

#include "p_local.h" // P_ApproxDistance

#include <dc/sound/sound.h>
#include <dc/sound/sfxmgr.h>

#define AICA_VOL_LOW 128.0f
#define AICA_VOL_HIGH 255.0f

typedef struct wavhdr_t {
    char hdr1[4];
    long totalsize;

    char hdr2[8];
    long hdrsize;
    short format;
    short channels;
    long freq;
    long byte_per_sec;
    short blocksize;
    short bits;

    char hdr3[4];
    long datasize;
} wavhdr_t;

typedef struct wavhdr2_t {
    char hdr1[4];
    long totalsize;

    char hdr2[8];
    long hdrsize;
    short format;
    short channels;
    long freq;
    long byte_per_sec;
    short blocksize;
    short bits;
    short pad;

    char hdr3[4];
    unsigned short datasizelo,datasizehi; // not long aligned - access as words
} wavhdr2_t;

typedef struct sfx_channel_c {
    int state;
    int category;
    int split;
    int achan; // aica channel
    sfxhnd_t handle; // sfxmgr handle
    int vol;
    int pan;
    bool loop; // loop ONCE
    bool boss;
    sfxdef_c *def;
    position_c *pos;
} sfx_channel_c;


int au_sfx_volume, var_music_dev;

static sfx_channel_c sfx_chan[64];
static sfxhnd_t *sfx_handles;

static int num_chan;

static bool allow_hogs = true;

static bool sfxpaused = false;

extern bool nomusic;
extern bool nocdmusic;

// Yamaha ADPCM vars and data
static int pred, step;

static const int16_t indexscale[] =
{
    230, 230, 230, 230, 307, 409, 512, 614,
    230, 230, 230, 230, 307, 409, 512, 614
};

static const int8_t difflookup[] =
{
     1,  3,  5,  7,  9,  11,  13,  15,
    -1, -3, -5, -7, -9, -11, -13, -15
};


static const int category_limit_table[2][8][3] =
{
    /*   SP CO DM  */
    /* 64 channels (no music) */
    {
        { 4, 4, 4 }, // UI
        { 4, 4, 4 }, // Player
        { 6, 6, 6 }, // Weapon

        { 0,10,24 }, // Opponent
        {28,20, 4 }, // Monster
        {14,12,14 }, // Object
        { 8, 8, 8 }, // Level
    },

    /* 62 channels (2 channels for music) */
    {
        { 3, 3, 3 }, // UI
        { 4, 4, 4 }, // Player
        { 6, 6, 6 }, // Weapon

        { 0,10,24 }, // Opponent
        {28,20, 4 }, // Monster
        {14,12,14 }, // Object
        { 7, 7, 7 }, // Level
    }
    // NOTE: never put a '0' on the WEAPON line, since the top
    // four categories should never be merged with the rest.
};


static int cat_limits[SNCAT_NUMTYPES];
static int cat_counts[SNCAT_NUMTYPES];


static void SetupCategoryLimits(void)
{
    // Assumes: DEATHMATCH() and COOP_MATCH() macros are working.

    int mode = 0; // single player
    if (COOP_MATCH()) mode = 1;
    if (DEATHMATCH()) mode = 2;

    int idx = nomusic ? 0 : 1;

    for (int t = 0; t < SNCAT_NUMTYPES; t++)
    {
        cat_limits[t] = category_limit_table[idx][t][mode];
        cat_counts[t] = 0;
    }
}

static void S_KillChannel(int k)
{
    sfx_channel_c *chan = &sfx_chan[k];

    if (chan->state != CHAN_Empty)
    {
        if (chan->achan != -1)
        {
            //I_Printf("S_KillChannel on aica ch#%d\n", chan->achan);
            snd_sfx_stop(chan->achan);
            snd_sfx_chn_free(chan->achan);
        }

        chan->achan = -1;
        chan->state = CHAN_Empty;
    }
}

static int FindFreeChannel(void)
{
    for (int i=0; i < num_chan; i++)
    {
        sfx_channel_c *chan = &sfx_chan[i];

        if (chan->state == CHAN_Playing)
        {
            int done = snd_sfx_update_chn(chan->achan, chan->handle, -1, -1, -1);
            if (done > 0)
            {
                if (chan->loop)
                {
                    // loop once - clear loop flag
                    chan->loop = false;
                    // restart sfx
                    snd_sfx_play_chn(chan->achan, chan->handle, chan->vol, chan->pan);
                }
                else
                    chan->state = CHAN_Finished;
            }
        }

        if (chan->state == CHAN_Finished)
            S_KillChannel(i);

        if (chan->state == CHAN_Empty)
            return i;
    }

    return -1; // not found
}

static int FindPlayingFX(sfxdef_c *def, int cat, position_c *pos)
{
    for (int i=0; i < num_chan; i++)
    {
        sfx_channel_c *chan = &sfx_chan[i];

        if (chan->state == CHAN_Playing && chan->category == cat && chan->pos == pos)
        {
            if (chan->def == def)
                return i;

            if (chan->def->singularity > 0 && chan->def->singularity == def->singularity)
                return i;
        }
    }

    return -1; // not found
}

static int FindBiggestHog(int real_cat)
{
    int biggest_hog = -1;
    int biggest_extra = 0;

    for (int hog = 0; hog < SNCAT_NUMTYPES; hog++)
    {
        if (hog == real_cat)
            continue;

        int extra = cat_counts[hog] - cat_limits[hog];

        if (extra <= 0)
            continue;

        // found a hog!
        if (biggest_hog < 0 || extra > biggest_extra)
        {
            biggest_hog = hog;
            biggest_extra = extra;
        }
    }

    SYS_ASSERT(biggest_hog >= 0);

    return biggest_hog;
}

static void CountPlayingCats(void)
{
    for (int c=0; c < SNCAT_NUMTYPES; c++)
        cat_counts[c] = 0;

    for (int i=0; i < num_chan; i++)
    {
        sfx_channel_c *chan = &sfx_chan[i];

        if (chan->state == CHAN_Playing)
            cat_counts[chan->category] += 1;
    }
}

static int ChannelScore(sfxdef_c *def, int category, mobj_t *listener, position_c *pos, bool boss)
{
    // for full-volume sounds, use the priority from DDF
    if (category <= SNCAT_Weapon)
    {
        return 200 - def->priority;
    }

    // for stuff in the level, use the distance
    SYS_ASSERT(pos);

    float dist = boss ? 0 : P_ApproxDistance(listener->x - pos->x, listener->y - pos->y, listener->z - pos->z);

    int base_score = 999 - (int)(dist / 10.0);

    return base_score * 100 - def->priority;
}

static int FindChannelToKill(mobj_t *listener, int kill_cat, int real_cat, int new_score)
{
    int kill_idx = -1;
    int kill_score = (1<<30);

    //I_Printf("FindChannelToKill: cat:%d new_score:%d\n", kill_cat, new_score);
    for (int j=0; j < num_chan; j++)
    {
        sfx_channel_c *chan = &sfx_chan[j];

        if (chan->state != CHAN_Playing)
            continue;

        if (chan->category != kill_cat)
            continue;

        int score = ChannelScore(chan->def, chan->category, listener,
                                 chan->pos, chan->boss);
        //I_Printf("> [%d] '%s' = %d\n", j, chan->def->name.c_str(), score);

        // find one with LOWEST score
        if (score < kill_score)
        {
            kill_idx = j;
            kill_score = score;
        }
    }
    //I_Printf("kill_idx = %d\n", kill_idx);
    SYS_ASSERT(kill_idx >= 0);

    if (kill_cat != real_cat)
        return kill_idx;

    // if the score for new sound is worse than any existing
    // channel, then simply discard the new sound.
    if (new_score >= kill_score)
        return kill_idx;

    return -1;
}

static int ComputeVol(int category, bool boss, mobj_t *listener, position_c *pos)
{
    float listen_x = listener ? listener->x : 0.0f;
    float listen_y = listener ? listener->y : 0.0f;
    float listen_z = listener ? listener->z : 0.0f;
//  angle_t listen_angle = listener ? listener->angle : 0;

    float mul = 1.0f;
    float vol = (float)au_sfx_volume * (AICA_VOL_HIGH - AICA_VOL_LOW) / (float)(SND_SLIDER_NUM - 1) + AICA_VOL_LOW;

    if (pos && category >= SNCAT_Opponent)
    {
        if (!boss)
        {
            float dist = P_ApproxDistance(listen_x - pos->x, listen_y - pos->y, listen_z - pos->z);

            // -AJA- this equation was chosen to mimic the DOOM falloff
            //       function, but instead of cutting out @ dist=1600 it
            //       tapers off exponentially.
            mul = exp(-MAX(1.0f, dist - S_CLOSE_DIST) / 800.0f);
        }
    }
    int rv = (int)(vol * mul);
    if (rv > 255)
        rv = 255; // clamp volume to max
    return rv;
}

static int ComputePan(int category, mobj_t *listener, position_c *pos)
{
    float listen_x = listener ? listener->x : 0.0f;
    float listen_y = listener ? listener->y : 0.0f;
//  float listen_z = listener ? listener->z : 0.0f;
    angle_t listen_angle = listener ? listener->angle : 0;

    float sep = 0.5f;

    if (pos && category >= SNCAT_Opponent)
    {
        angle_t angle = R_PointToAngle(listen_x, listen_y, pos->x, pos->y);

        // same equation from original DOOM
        sep = 0.5f - 0.38f * M_Sin(angle - listen_angle);
    }

    return (int)(sep * 256);
}

static void WriteWavHdr(FILE *out, int rate, int chans, int adpcm, int samples)
{
    wavhdr_t wh;

    memcpy(wh.hdr1, "RIFF", 4);
    memcpy(wh.hdr2, "WAVEfmt ", 8);
    memcpy(wh.hdr3, "data", 4);

    // relies on SH4 being LE
    wh.hdrsize = 16;
    wh.datasize = samples * chans * 2 / (adpcm ?  4 : 1);
    wh.format = adpcm ? 20 : 1; // Yamaha ADPCM : PCM
    wh.channels = chans;
    wh.freq = rate;
    wh.bits = adpcm ? 4 : 16;
    wh.totalsize = wh.datasize + sizeof(wavhdr_t) - 8;

    fwrite(&wh, 1, sizeof(wavhdr_t), out);
}

static inline int AdpcmEncode(int sample)
{
    int nibble, delta;

    delta = sample - pred;
    nibble = MIN(7, (abs(delta) * 4 / step));
    if (delta < 0)
        nibble |= 8;

    pred += ((step * difflookup[nibble]) / 8);
    if (pred < -32768)
        pred = -32768;
    else if (pred > 32767)
        pred = 32767;

    step = (step * indexscale[nibble]) >> 8;
    if (step < 127)
        step = 127;
    else if (step > 24567)
        step = 24567;

    return nibble;
}

static inline int sum_samples_8(byte *data, int scale)
{
    int sample = 0;
    int idx = 0;

    switch (scale)
    {
        case 4:
            sample += (int16_t)((data[idx++] ^ 0x80) << 8);
            sample += (int16_t)((data[idx++] ^ 0x80) << 8);
            sample += (int16_t)((data[idx++] ^ 0x80) << 8);
            sample += (int16_t)((data[idx++] ^ 0x80) << 8);
            sample += (int16_t)((data[idx++] ^ 0x80) << 8);
            sample += (int16_t)((data[idx++] ^ 0x80) << 8);
            sample += (int16_t)((data[idx++] ^ 0x80) << 8);
            sample += (int16_t)((data[idx++] ^ 0x80) << 8);
        case 3:
            sample += (int16_t)((data[idx++] ^ 0x80) << 8);
            sample += (int16_t)((data[idx++] ^ 0x80) << 8);
            sample += (int16_t)((data[idx++] ^ 0x80) << 8);
            sample += (int16_t)((data[idx++] ^ 0x80) << 8);
        case 2:
            sample += (int16_t)((data[idx++] ^ 0x80) << 8);
            sample += (int16_t)((data[idx++] ^ 0x80) << 8);
        case 1:
            sample += (int16_t)((data[idx++] ^ 0x80) << 8);
        case 0:
            sample += (int16_t)((data[idx] ^ 0x80) << 8);
    }
    return sample;
}

static inline int sum_samples_16(byte *data, int scale)
{
    int sample = 0;
    int idx = 0;

    switch (scale)
    {
        case 4:
            sample += (int16_t)(data[idx] | (data[idx+1] << 8));
            idx += 2;
            sample += (int16_t)(data[idx] | (data[idx+1] << 8));
            idx += 2;
            sample += (int16_t)(data[idx] | (data[idx+1] << 8));
            idx += 2;
            sample += (int16_t)(data[idx] | (data[idx+1] << 8));
            idx += 2;
            sample += (int16_t)(data[idx] | (data[idx+1] << 8));
            idx += 2;
            sample += (int16_t)(data[idx] | (data[idx+1] << 8));
            idx += 2;
            sample += (int16_t)(data[idx] | (data[idx+1] << 8));
            idx += 2;
            sample += (int16_t)(data[idx] | (data[idx+1] << 8));
            idx += 2;
        case 3:
            sample += (int16_t)(data[idx] | (data[idx+1] << 8));
            idx += 2;
            sample += (int16_t)(data[idx] | (data[idx+1] << 8));
            idx += 2;
            sample += (int16_t)(data[idx] | (data[idx+1] << 8));
            idx += 2;
            sample += (int16_t)(data[idx] | (data[idx+1] << 8));
            idx += 2;
        case 2:
            sample += (int16_t)(data[idx] | (data[idx+1] << 8));
            idx += 2;
            sample += (int16_t)(data[idx] | (data[idx+1] << 8));
            idx += 2;
        case 1:
            sample += (int16_t)(data[idx] | (data[idx+1] << 8));
            idx += 2;
        case 0:
            sample += (int16_t)(data[idx] | (data[idx+1] << 8));
    }
    return sample;
}

static int S_LoadSfx(sfxdef_c *def, int downsmp, int adpcm, sfxhnd_t *handle)
{
    I_Debugf("\nS_LoadSfx: [%s]\n", def->name.c_str());

    // open the file or lump, and read it into memory
    epi::file_c *F;

    if (def->file_name && def->file_name[0])
    {
        std::string fn = M_ComposeFileName(game_dir.c_str(), def->file_name.c_str());

        F = epi::FS_Open(fn.c_str(), epi::file_c::ACCESS_READ | epi::file_c::ACCESS_BINARY);

        if (!F)
        {
            M_WarnError("SFX Loader: Can't Find File '%s'\n", fn.c_str());
            return 0;
        }
    }
    else
    {
        int lump = W_CheckNumForName(def->lump_name.c_str());
        if (lump < 0)
        {
            M_WarnError("SFX Loader: Missing sound lump: %s\n", def->lump_name.c_str());
            return 0;
        }

        F = W_OpenLump(lump);
        SYS_ASSERT(F);
    }

    int length;
    byte *data;
    if (handle)
    {
        I_Printf("R");
        length = F->GetLength();
        data = F->LoadIntoMemory();
        //data = new byte[length];
        //F->Read(data, length);
    }
    else
    {
        I_Printf("H");
        // no handle -> just get header to compute # samples
        length = F->GetLength();
        data = new byte[48];
        F->Read(data, 48);
    }

    // no longer need the epi::file_c
    delete F;
    F = NULL;

    if (!data || length < 4)
    {
        M_WarnError("SFX Loader: Error loading data.\n");
        if (data)
            delete[] data;
        return 0;
    }

#if 0
    // enable to measure just load time
    delete[] data;
    return 0;
#endif

    I_Printf("C");

    int iw, ix, iz, rate, sample, samples;
    byte *buf = NULL;
    samples = 0; // suppress warning

    if (adpcm)
    {
        // init adpcm encoder vars
        pred = 0;
        step = 127;
    }

    // convert sound into wav file in /ram for snd_sfx_load()
    if (memcmp(data, "RIFF", 4) == 0)
    {
        // wav
        wavhdr_t *wh = (wavhdr_t *)data;

        // relies on SH4 being LE
        if (memcmp(wh->hdr2, "WAVEfmt ", 8)
            || memcmp(wh->hdr3, "data", 4)
            || wh->hdrsize != 16
            || wh->format != 1
            || (wh->channels != 1 && wh->channels != 2)
            || (wh->bits != 16 && wh->bits != 8))
        {
            wavhdr2_t *wh2 = (wavhdr2_t *)data;
            // check for oddball format

            // relies on SH4 being LE
            if (memcmp(wh2->hdr2, "WAVEfmt ", 8)
                || memcmp(wh2->hdr3, "data", 4)
                || wh2->hdrsize != 18
                || wh2->format != 1
                || (wh2->channels != 1 && wh2->channels != 2)
                || (wh2->bits != 16 && wh2->bits != 8))
            {
                M_WarnError("SFX Loader: WAV file unsupported format.\n");
                delete[] data;
                return 0;
            }
            wh->datasize = wh2->datasizelo | (wh2->datasizehi << 16);
        }

        rate = wh->freq;
        if (rate < 7000 || rate > 48000)
        {
            I_Warning("  Sound Load: weird sample rate: %d Hz\n", rate);
            rate = (rate < 7000) ? 7000 : 48000;
        }

        samples = wh->datasize / (wh->channels * wh->bits / 8);
        while ((samples >> downsmp) > 65536)
            downsmp++;

        if (handle)
            buf = new byte[(samples >> downsmp) * 2 / (adpcm ? 4 : 1) + 2];

        if (handle && buf)
        {
            if (adpcm)
            {
                for (ix=0, iw=0, iz=0; ix<(samples >> downsmp); ix+=2)
                {
                    int val;
                    if (wh->bits == 8)
                    {
                        sample = sum_samples_8(&data[iz], downsmp + wh->channels - 1);
                        iz += 1 << (downsmp + wh->channels - 1);
                    }
                    else
                    {
                        sample = sum_samples_16(&data[iz], downsmp + wh->channels - 1);
                        iz += 1 << (downsmp + wh->channels);
                    }
                    sample >>= (downsmp + wh->channels - 1);
                    val = AdpcmEncode(sample);
                    if (wh->bits == 8)
                    {
                        sample = sum_samples_8(&data[iz], downsmp + wh->channels - 1);
                        iz += 1 << (downsmp + wh->channels - 1);
                    }
                    else
                    {
                        sample = sum_samples_16(&data[iz], downsmp + wh->channels - 1);
                        iz += 1 << (downsmp + wh->channels);
                    }
                    sample >>= (downsmp + wh->channels - 1);
                    val |= (AdpcmEncode(sample) << 4);
                    buf[iw++] = val;
                }
            }
            else
            {
                for (ix=0, iw=0, iz=0; ix<(samples >> downsmp); ix++)
                {
                    if (wh->bits == 8)
                    {
                        sample = sum_samples_8(&data[iz], downsmp + wh->channels - 1);
                        iz += 1 << (downsmp + wh->channels - 1);
                    }
                    else
                    {
                        sample = sum_samples_16(&data[iz], downsmp + wh->channels - 1);
                        iz += 1 << (downsmp + wh->channels);
                    }
                    sample >>= (downsmp + wh->channels - 1);
                    buf[iw++] = sample & 0x00FF;
                    buf[iw++] = (sample >> 8) && 0x00FF;
                }
            }
        }
    }
    else if (memcmp(data, "Ogg", 3) == 0)
    {
        // OGG Vorbis/OPUS
        M_WarnError("SFX Loader: OGG SFX not yet supported.\n");

    }
    else
    {
        // assume Doom format
        rate = data[2] + (data[3] << 8);
        if (rate < 7000 || rate > 48000)
        {
            I_Warning("  Sound Load: weird sample rate: %d Hz\n", rate);
            rate = (rate < 7000) ? 7000 : 48000;
        }

        samples = length - 8;
        while ((samples >> downsmp) > 65536)
            downsmp++;

        if (handle)
            buf = new byte[(samples >> downsmp) * 2 / (adpcm ? 4 : 1) + 2];

        if (handle && buf)
        {
            if (adpcm)
            {
                for (ix=0, iw=0, iz=0; ix<(samples >> downsmp); ix+=2)
                {
                    int val;
                    sample = sum_samples_8(&data[iz+8], downsmp);
                    iz += 1 << downsmp;
                    sample >>= downsmp;
                    val = AdpcmEncode(sample);
                    sample = sum_samples_8(&data[iz+8], downsmp);
                    iz += 1 << downsmp;
                    sample >>= downsmp;
                    val |= (AdpcmEncode(sample) << 4);
                    buf[iw++] = val;
                }
            }
            else
            {
                for (ix=0, iw=0, iz=0; ix<(samples >> downsmp); ix++)
                {
                    sample = sum_samples_8(&data[iz+8], downsmp);
                    iz += 1 << downsmp;
                    sample >>= downsmp;
                    buf[iw++] = sample & 0x00FF;
                    buf[iw++] = (sample >> 8) && 0x00FF;
                }
            }
        }
    }
    delete[] data;

    if (buf)
    {
        FILE *out = fopen("/ram/temp.wav", "wb");
        if (!out)
        {
            M_WarnError("\nSFX Loader: Can't open temp file for write.\n");
            delete[] buf;
            return 0;
        }
        WriteWavHdr(out, rate >> downsmp, 1, adpcm, samples >> downsmp);
        fwrite(buf, 1, (samples >> downsmp) / 2, out);
        fclose(out);
        delete[] buf;
    }

    if (handle)
    {
        I_Printf("U");

        *handle = snd_sfx_load("/ram/temp.wav");
        remove("/ram/temp.wav");
#ifdef PLAY_SND_ON_LOAD
        I_Printf("P");
        int achan = snd_sfx_play(*handle, 175, 128);
        while (snd_sfx_update_chn(achan, *handle, -1, -1, -1) <= 0)
            thd_sleep(5);
        snd_sfx_stop(achan);
        snd_sfx_chn_free(achan);
        thd_sleep(100);
#endif
    }

    return (((samples >> downsmp) * 2 / (adpcm ? 4 : 1)) + 31) & ~31;
}

#define SND_RAM_SIZE (0x200000 - 0x060000)

#ifdef SHOW_SCREEN
extern void E_FontPrintStringDC(char * string);
#endif

char smsg[128];

void S_Init(void)
{
#ifdef NO_SOUND
	nosound = true;
#endif

    if (nosound)
    {
        I_Printf("S_Init: sound disabled\n");
        return;
    }

    num_chan = nomusic ? 64 : 62;

    I_Printf("S_Init: Init %d hardware channels\n", num_chan);

    SetupCategoryLimits();

    int ix, total, downsmp;
    int numsfx = sfxdefs.GetSize();

    // clear all channels
    for (ix=0; ix<64; ix++)
    {
        sfx_chan[ix].achan = -1;
        sfx_chan[ix].state = CHAN_Empty;
    }

    // fit sfx to available sound ram
    downsmp = -1;
    do
    {
        downsmp++;
        total = 0;
        I_Printf("\n  Calculating space with downsample by %d: ", 1 << downsmp);
        for (ix=0; ix<numsfx; ix++)
        {
            total += S_LoadSfx(sfxdefs[ix], downsmp, 1, NULL);
            I_Printf(".");
        }
        I_Printf("\n  %d sound ram used with downsample by %d: ", total, 1 << downsmp);
    } while (total > SND_RAM_SIZE);

    sfx_handles = (sfxhnd_t *)malloc(sizeof(sfxhnd_t) * numsfx);
    if (!sfx_handles)
    {
        I_Printf("S_Init: couldn't get memory for sfx handles\n");
        nosound = true;
        return;
    }

    // load all sfx to sound memory
    total = 0;
    I_Printf("\n  Loading sound effects: ");
    for (ix=0; ix<numsfx; ix++)
    {
        total += S_LoadSfx(sfxdefs[ix], downsmp, 1, &sfx_handles[ix]);
        I_Printf(".");
#ifdef SHOW_SCREEN
        sprintf(smsg, "S_Init: %d%% sfx loaded", ix * 100 / numsfx);
        E_FontPrintStringDC(smsg);
#else
        E_LocalProgress(ix, numsfx);
#endif
    }
    I_Printf("\n  %d effects loaded, %d bytes sound ram used\n", numsfx, total);

    sfxpaused = false;
}

void S_Shutdown(void)
{
    if (nosound)
        return;

    snd_sfx_stop_all();
    snd_sfx_unload_all();

    if (sfx_handles)
        free(sfx_handles);
}

static void S_UpdateSounds(mobj_t *listener)
{
    for (int i = 0; i < num_chan; i++)
    {
        sfx_channel_c *chan = &sfx_chan[i];

        if (chan->state == CHAN_Playing)
        {
            //I_Printf("S_UpdateSounds on channel#%d\n", i);
            if (chan->split)
            {
                chan->vol = ComputeVol(chan->category, chan->boss, listener, NULL);
                chan->pan = (chan->split == 1) ? 0 : 255;
            }
            else
            {
                chan->vol = ComputeVol(chan->category, chan->boss, listener, chan->pos);
                chan->pan = ComputePan(chan->category, listener, chan->pos);
            }
            int done = snd_sfx_update_chn(chan->achan, chan->handle, -1, chan->vol, chan->pan);

            //I_Printf("  snd_sfx_update_chn on aica ch#%d: %d %d %s\n", chan->achan, chan->vol, chan->pan, done < 0 ? "STOPPED" : done > 0 ? "DONE" : "PLAYING");
            if (done > 0)
            {
                if (chan->loop)
                {
                    // loop once - clear loop flag
                    chan->loop = false;
                    // restart sfx
                    snd_sfx_play_chn(chan->achan, chan->handle, chan->vol, chan->pan);
                }
                else
                    chan->state = CHAN_Finished;
            }
        }

        if (chan->state == CHAN_Finished)
            S_KillChannel(i);
    }
}

void S_ChangeSoundVolume(void)
{
    if (nosound)
        return;

    I_LockAudio();
    S_UpdateSounds(NULL);
    I_UnlockAudio();
}

void S_PauseSound(void)
{
    if (nosound)
        return;

    sfxpaused = true;
}

void S_ResumeSound(void)
{
    if (nosound)
        return;

    sfxpaused = false;
}

static int LookupEffect(const sfx_t *s)
{
    SYS_ASSERT(s->num >= 1);

    // need to use M_Random here to prevent demos and net games
    // getting out of sync.

    int num;

    if (s->num > 1)
        num = s->sounds[M_Random() % s->num];
    else
        num = s->sounds[0];

    SYS_ASSERT(0 <= num && num < sfxdefs.GetSize());

    return num;
}

static void S_PlaySound(int idx, sfxdef_c *def, int category, mobj_t *listener, position_c *pos, int flags, int sfxnum)
{
    //I_Printf("S_PlaySound on chan #%d DEF:%p\n", idx, def);

    sfx_channel_c *chan = &sfx_chan[idx];

    chan->state = CHAN_Playing;
    chan->handle = sfx_handles[sfxnum];
    if (!chan->handle)
    {
        M_WarnError("S_PlaySound: SFX %d has no handle!\n", sfxnum);
        chan->achan = -1;
        chan->state = CHAN_Empty;
        return;
    }

    //I_Printf("chan=%p handle=%d\n", chan, chan->handle);

    chan->def = def;
    chan->pos = pos;
    chan->category = category; // used in channel arbitration when need to steal a channel

    chan->loop = false;
    chan->boss = (flags & FX_Boss) ? true : false;
    chan->split = 0;

    if (splitscreen_mode && pos && consoleplayer1 >= 0 && consoleplayer2 >= 0)
    {
        if (pos == players[consoleplayer1]->mo)
            chan->split = 1;
        else if (pos == players[consoleplayer2]->mo)
            chan->split = 2;
        I_Debugf("%s : split %d  cat %d\n", def->name.c_str(), chan->split, category);
    }

    if (chan->split)
    {
        chan->vol = ComputeVol(category, chan->boss, listener, NULL);
        chan->pan = (chan->split == 1) ? 0 : 255;
    }
    else
    {
        chan->vol = ComputeVol(category, chan->boss, listener, pos);
        chan->pan = ComputePan(category, listener, pos);
    }
    chan->achan = snd_sfx_play(chan->handle, chan->vol, chan->pan);
    if (chan->achan == -1)
    {
        // no more aica channels!
        M_WarnError("S_PlaySound: Ran out of hardware channels!\n");
        chan->state = CHAN_Empty;
    }
    //I_Printf("S_PlaySound on aica ch#%d\n", chan->achan);
}

static void DoStartFX(sfxdef_c *def, int category, mobj_t *listener, position_c *pos, int flags, int sfxnum)
{
    CountPlayingCats();

    int k = FindPlayingFX(def, category, pos);

    if (k >= 0)
    {
        //I_Printf("@ already playing on #%d\n", k);
        sfx_channel_c *chan = &sfx_chan[k];

        if (def->looping && def == chan->def)
        {
            //I_Printf("@@ RE-LOOPING\n");
            chan->loop = true;
            return;
        }
        else if (flags & FX_Single)
        {
            if (flags & FX_Precious)
                return;

            //I_Printf("@@ Killing sound for SINGULAR\n");
            S_KillChannel(k);
            S_PlaySound(k, def, category, listener, pos, flags, sfxnum);
            return;
        }
    }

    k = FindFreeChannel();

    if (!allow_hogs)
    {
        if (cat_counts[category] >= cat_limits[category])
            k = -1;
    }

    //I_Printf("@ free channel = #%d\n", k);
    if (k < 0)
    {
        // all channels are in use.
        // either kill one, or drop the new sound.

        int new_score = ChannelScore(def, category, listener, pos, (flags & FX_Boss) ? true : false);

        // decide which category to kill a sound in.
        int kill_cat = category;

        if (cat_counts[category] < cat_limits[category])
        {
            // we haven't reached our quota yet, hence kill a hog.
            kill_cat = FindBiggestHog(category);
            //I_Printf("@ biggest hog: %d\n", kill_cat);
        }

        SYS_ASSERT(cat_counts[kill_cat] >= cat_limits[kill_cat]);

        k = FindChannelToKill(listener, kill_cat, category, new_score);

        //if (k<0) I_Printf("- new score too low\n");
        if (k < 0)
            return;

        //I_Printf("- killing channel %d (kill_cat:%d)  my_cat:%d\n", k, kill_cat, category);
        S_KillChannel(k);
    }

    S_PlaySound(k, def, category, listener, pos, flags, sfxnum);
}


void S_StartFX(sfx_t *sfx, int category, position_c *pos, int flags)
{
    if (nosound || !sfx)
        return;

    if (fast_forward_active)
        return;

    SYS_ASSERT(0 <= category && category < SNCAT_NUMTYPES);

    if (category >= SNCAT_Opponent && !pos)
        I_Error("S_StartFX: position missing for category: %d\n", category);

    int snum = LookupEffect(sfx);
    sfxdef_c *def = sfxdefs[snum];
    SYS_ASSERT(def);

    mobj_t *listener = NULL;
    if (gamestate == GS_LEVEL)
        listener = ::players[displayplayer]->mo;

    // ignore very far away sounds
    if (category >= SNCAT_Opponent && !(flags & FX_Boss))
    {
        float dist = P_ApproxDistance(listener->x - pos->x, listener->y - pos->y, listener->z - pos->z);

        if (dist > def->max_distance)
            return;
    }

    if (def->singularity > 0)
    {
        flags |= FX_Single;
        flags |= (def->precious ? FX_Precious : 0);
    }

    //I_Printf("StartFX: '%s' cat:%d flags:0x%04x\n", def->name.c_str(), category, flags);

    while (cat_limits[category] == 0)
        category++;

    I_LockAudio();
    DoStartFX(def, category, listener, pos, flags, snum);
    I_UnlockAudio();
}


void S_StopFX(position_c *pos)
{
    if (nosound)
        return;

    I_LockAudio();

    {
        for (int i = 0; i < num_chan; i++)
        {
            sfx_channel_c *chan = &sfx_chan[i];

            if (chan->state == CHAN_Playing && chan->pos == pos)
            {
                //I_Printf("S_StopFX: killing #%d\n", i);
                S_KillChannel(i);
            }
        }
    }

    I_UnlockAudio();
}

void S_StopLevelFX(void)
{
    if (nosound)
        return;

    I_LockAudio();

    {
        for (int i = 0; i < num_chan; i++)
        {
            sfx_channel_c *chan = &sfx_chan[i];

            if (chan->state != CHAN_Empty && chan->category != SNCAT_UI)
            {
                S_KillChannel(i);
            }
        }
    }

    I_UnlockAudio();
}


void S_SoundTicker(void)
{
    if (nosound)
        return;

    I_LockAudio();

    {
        if (gamestate == GS_LEVEL)
        {
            SYS_ASSERT(::numplayers > 0);

            mobj_t *listener = ::players[displayplayer]->mo;
            SYS_ASSERT(listener);

            S_UpdateSounds(listener);
        }
        else
        {
            S_UpdateSounds(NULL);
        }
    }

    I_UnlockAudio();
}

void S_ChangeChannelNum(void)
{
    if (nosound)
        return;

    I_LockAudio();
    SetupCategoryLimits();
    I_UnlockAudio();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
