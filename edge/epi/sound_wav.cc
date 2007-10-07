//----------------------------------------------------------------------------
//  WAV Format Sound Loading
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2007  The EDGE Team.
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
//  Based heavily on the WAV decoder from the "SDL_sound" library
//  Copyright (C) 2001  Ryan C. Gordon.
// 
//----------------------------------------------------------------------------

#include "epi.h"

#include "sound_wav.h"
#include "endianess.h"

#if 1
#define SND_DEBUG(args)  I_Debugf(args)
#else
#define SND_DEBUG(args)  do {} while (0)
#endif

namespace epi
{

static inline bool read_le_u16(file_c *f, u16_t *val)
{
    if (2 != f->Read(val, 2))
		return false;

    *val = EPI_LE_U16(*val);
    return true;
}

static inline bool read_le_s16(file_c *f, s16_t *val)
{
	return read_le_u16(f, (u16_t *) val);
}


static inline bool read_le_u32(file_c *f, u32_t *val)
{
    if (4 != f->Read(val, 4))
		return false;

    *val = EPI_LE_U32(*val);
    return true;
}

static inline bool read_le_s32(file_c *f, s32_t *val)
{
	return read_le_u32(f, (u32_t *) val);
}


/* This is just cleaner on the caller's end... */
static inline bool read_uint8(file_c *f, u8_t *val)
{
	return (1 == f->Read(val, 1));
}


/* Chunk management code... */

#define ID_RIFF  0x46464952  /* "RIFF", in ascii. */
#define ID_WAVE  0x45564157  /* "WAVE", in ascii. */
#define ID_FACT  0x74636166  /* "fact", in ascii. */


/*********************************************************
 * The FORMAT chunk...                                   *
 *********************************************************/

#define ID_FMT  0x20746D66  /* "fmt ", in ascii. */

#define FMT_NORMAL 0x0001    /* Uncompressed waveform data.     */
#define FMT_ADPCM  0x0002    /* ADPCM compressed waveform data. */

typedef struct
{
    s16_t iCoef1;
    s16_t iCoef2;
}
ADPCMCOEFSET;

typedef struct
{
    u8_t bPredictor;
    u16_t iDelta;
    s16_t iSamp1;
    s16_t iSamp2;
}
ADPCMBLOCKHEADER;

typedef struct S_WAV_FMT_T
{
    u32_t chunkID;
    s32_t chunkSize;
    s16_t wFormatTag;
    u16_t wChannels;
    u32_t dwSamplesPerSec;
    u32_t dwAvgBytesPerSec;
    u16_t wBlockAlign;
    u16_t wBitsPerSample;

    u32_t next_chunk_offset;
    
    u32_t sample_frame_size;
    u32_t data_starting_offset;
    u32_t total_bytes;

    u32_t (*read_sample)(Sound_Sample *sample);

    union
    {
        struct
        {
            u16_t cbSize;
            u16_t wSamplesPerBlock;
            u16_t wNumCoef;
            ADPCMCOEFSET *aCoef;
            ADPCMBLOCKHEADER *blockheaders;
            u32_t samples_left_in_block;
            int nibble_state;
            s8_t nibble;
        } adpcm;

        /* put other format-specific data here... */
    } fmt;
}
fmt_t;


/*
 * Read in a fmt_t from disk. This makes this process safe regardless of
 *  the processor's byte order or how the fmt_t structure is packed.
 * Note that the union "fmt" is not read in here; that is handled as 
 *  needed in the read_fmt_* functions.
 */
static bool read_fmt_chunk(file_c *f, fmt_t *fmt)
{
    /* skip reading the chunk ID, since it was already read at this point... */
    fmt->chunkID = ID_FMT;

    if (! read_le_s32(f, &fmt->chunkSize))
		return false;

    if (fmt->chunkSize < 16)
	{
		I_Printf("WAV: Invalid chunk size");
		return false;
	}

    fmt->next_chunk_offset = f->GetPosition() + fmt->chunkSize;

    return ( read_le_s16(f, &fmt->wFormatTag) &&
			 read_le_u16(f, &fmt->wChannels) &&
			 read_le_u32(f, &fmt->dwSamplesPerSec) &&
			 read_le_u32(f, &fmt->dwAvgBytesPerSec) &&
			 read_le_u16(f, &fmt->wBlockAlign) &&
			 read_le_u16(f, &fmt->wBitsPerSample) );
}


/*********************************************************
 * The DATA chunk...                                     *
 *********************************************************/

#define ID_DATA  0x61746164  /* "data", in ascii. */

typedef struct
{
    u32_t chunkID;
    s32_t chunkSize;
    /* Then, (chunkSize) bytes of waveform data... */
}
data_chunk_t;


/*
 * Read in a data_t from disk. This makes this process safe regardless of
 *  the processor's byte order or how the fmt_t structure is packed.
 */
static bool read_data_chunk(file_c *f, data_chunk_t *data)
{
    /* skip reading the chunk ID, since it was already read at this point... */
    data->chunkID = ID_DATA;

    return read_le_s32(f, &data->chunkSize);
}




/*****************************************************************************
 * this is what we store in our internal->decoder_private field...           *
 *****************************************************************************/

typedef struct
{
    fmt_t *fmt;

    s32_t bytes_left;
}
wav_t;



/*****************************************************************************
 * Normal, uncompressed waveform handler...                                  *
 *****************************************************************************/

/*
 * Sound_Decode() lands here for uncompressed WAVs...
 */
static u32_t read_sample_fmt_normal(Sound_Sample *sample)
{
    u32_t retval;
    
	Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
	
    wav_t *w = (wav_t *) internal->decoder_private;

    u32_t max = (internal->buffer_size < (u32_t) w->bytes_left) ?
                 internal->buffer_size : (u32_t) w->bytes_left;

    SYS_ASSERT(max > 0);

	/*
	 * We don't actually do any decoding, so we read the wav data
	 * directly into the internal buffer...
	 */
    retval = internal->f->Read(internal->buffer, max);

    w->bytes_left -= retval;

        /* Make sure the read went smoothly... */
    if ((retval == 0) || (w->bytes_left == 0))
        sample->flags |= SOUND_SAMPLEFLAG_EOF;

    else if (retval == -1)
        sample->flags |= SOUND_SAMPLEFLAG_ERROR;

        /* (next call this EAGAIN may turn into an EOF or error.) */
    else if (retval < internal->buffer_size)
        sample->flags |= SOUND_SAMPLEFLAG_EAGAIN;

    return retval;
}


static bool read_fmt_normal(file_c *f, fmt_t *fmt)
{
    /* (don't need to read more from the RWops...) */
    fmt->read_sample = read_sample_fmt_normal;

    return true; //OK
}



/*********************************************************
 * ADPCM compression handler...                          *
 *********************************************************/

#define FIXED_POINT_COEF_BASE      256
#define FIXED_POINT_ADAPTION_BASE  256
#define SMALLEST_ADPCM_DELTA       16


static inline bool read_adpcm_block_headers(Sound_Sample *sample)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    file_c *rw = internal->rw;
    wav_t *w = (wav_t *) internal->decoder_private;
    fmt_t *fmt = w->fmt;
    ADPCMBLOCKHEADER *headers = fmt->fmt.adpcm.blockheaders;

    if (w->bytes_left < fmt->wBlockAlign)
    {
        sample->flags |= SOUND_SAMPLEFLAG_EOF;
        return(0);
    }

    w->bytes_left -= fmt->wBlockAlign;

	int i;
    for (i = 0; i < fmt->wChannels; i++)
        if (! read_uint8(f, &headers[i].bPredictor))
			return false;

    for (i = 0; i < fmt->wChannels; i++)
        if (! read_le_s16(f, &headers[i].iDelta))
			return false;

    for (i = 0; i < fmt->wChannels; i++)
        if (! read_le_s16(f, &headers[i].iSamp1))
			return false;

    for (i = 0; i < fmt->wChannels; i++)
        if (! read_le_s16(f, &headers[i].iSamp2))
			return false;

    fmt->fmt.adpcm.samples_left_in_block = fmt->fmt.adpcm.wSamplesPerBlock;
    fmt->fmt.adpcm.nibble_state = 0;

    return true; //OK
}


static inline void do_adpcm_nibble(u8_t nib,
                                   ADPCMBLOCKHEADER *header,
                                   s32_t lPredSamp)
{
	static const s32_t max_audioval = (1<<15) - 1;
	static const s32_t min_audioval = -max_audioval;

	static const s32_t AdaptionTable[] =
    {
		230, 230, 230, 230, 307, 409, 512, 614,
		768, 614, 512, 409, 307, 230, 230, 230
	};

    s32_t lNewSamp;
    s32_t delta;

    if (nib & 0x08)
        lNewSamp = lPredSamp + (header->iDelta * (nib - 0x10));
	else
        lNewSamp = lPredSamp + (header->iDelta * nib);

        /* clamp value... */
    if (lNewSamp < min_audioval)
        lNewSamp = min_audioval;
    else if (lNewSamp > max_audioval)
        lNewSamp = max_audioval;

    delta = ((s32_t) header->iDelta * AdaptionTable[nib]) /
              FIXED_POINT_ADAPTION_BASE;

	if (delta < SMALLEST_ADPCM_DELTA)
	    delta = SMALLEST_ADPCM_DELTA;

    header->iDelta = delta;
	header->iSamp2 = header->iSamp1;
	header->iSamp1 = lNewSamp;
}


static inline int decode_adpcm_sample_frame(Sound_Sample *sample)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    wav_t *w = (wav_t *) internal->decoder_private;
    fmt_t *fmt = w->fmt;
    ADPCMBLOCKHEADER *headers = fmt->fmt.adpcm.blockheaders;
    file_c *rw = internal->rw;

    s32_t delta;
    u8_t nib = fmt->fmt.adpcm.nibble;

    for (int i = 0; i < fmt->wChannels; i++)
    {
		s16_t iCoef1 = fmt->fmt.adpcm.aCoef[headers[i].bPredictor].iCoef1;
        s16_t iCoef2 = fmt->fmt.adpcm.aCoef[headers[i].bPredictor].iCoef2;
		
        s32_t lPredSamp = ((headers[i].iSamp1 * iCoef1) +
                            (headers[i].iSamp2 * iCoef2)) / 
                             FIXED_POINT_COEF_BASE;

        if (fmt->fmt.adpcm.nibble_state == 0)
        {
            BAIL_IF_MACRO(!read_uint8(f, &nib), NULL, 0);
            fmt->fmt.adpcm.nibble_state = 1;
            do_adpcm_nibble(nib >> 4, &headers[i], lPredSamp);
        }
        else
        {
            fmt->fmt.adpcm.nibble_state = 0;
            do_adpcm_nibble(nib & 0x0F, &headers[i], lPredSamp);
        }
    }

    fmt->fmt.adpcm.nibble = nib;

    return true; //OK
}


static inline void put_adpcm_sample_frame1(void *_buf, fmt_t *fmt)
{
    u16_t *buf = (u16_t *) _buf;
    ADPCMBLOCKHEADER *headers = fmt->fmt.adpcm.blockheaders;

    for (int i = 0; i < fmt->wChannels; i++)
        *(buf++) = headers[i].iSamp1;
}


static inline void put_adpcm_sample_frame2(void *_buf, fmt_t *fmt)
{
    u16_t *buf = (u16_t *) _buf;
    ADPCMBLOCKHEADER *headers = fmt->fmt.adpcm.blockheaders;
    int i;
    for (i = 0; i < fmt->wChannels; i++)
        *(buf++) = headers[i].iSamp2;
}


/*
 * Sound_Decode() lands here for ADPCM-encoded WAVs...
 */
static u32_t read_sample_fmt_adpcm(Sound_Sample *sample)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    wav_t *w = (wav_t *) internal->decoder_private;
    fmt_t *fmt = w->fmt;
    u32_t bw = 0;

    while (bw < internal->buffer_size)
    {
        /* write ongoing sample frame before reading more data... */
        switch (fmt->fmt.adpcm.samples_left_in_block)
        {
            case 0:  /* need to read a new block... */
                if (!read_adpcm_block_headers(sample))
                {
                    if ((sample->flags & SOUND_SAMPLEFLAG_EOF) == 0)
                        sample->flags |= SOUND_SAMPLEFLAG_ERROR;
                    return(bw);
                } /* if */

                /* only write first sample frame for now. */
                put_adpcm_sample_frame2((u8_t *) internal->buffer + bw, fmt);
                fmt->fmt.adpcm.samples_left_in_block--;
                bw += fmt->sample_frame_size;
                break;

            case 1:  /* output last sample frame of block... */
                put_adpcm_sample_frame1((u8_t *) internal->buffer + bw, fmt);
                fmt->fmt.adpcm.samples_left_in_block--;
                bw += fmt->sample_frame_size;
                break;

            default: /* output latest sample frame and read a new one... */
                put_adpcm_sample_frame1((u8_t *) internal->buffer + bw, fmt);
                fmt->fmt.adpcm.samples_left_in_block--;
                bw += fmt->sample_frame_size;

                if (!decode_adpcm_sample_frame(sample))
                {
                    sample->flags |= SOUND_SAMPLEFLAG_ERROR;
                    return(bw);
                } /* if */
        } /* switch */
    } /* while */

    return(bw);
}


/*
 * Sound_FreeSample() lands here for ADPCM-encoded WAVs...
 */
static void free_fmt_adpcm(fmt_t *fmt)
{
    delete fmt->fmt.adpcm.aCoef;
    delete fmt->fmt.adpcm.blockheaders;
}



/*
 * Read in the adpcm-specific info from disk. This makes this process
 * safe regardless of the processor's byte order or how the fmt_t 
 * structure is packed.
 */
static bool read_fmt_adpcm(file_c *f, fmt_t *fmt)
{
    size_t i;

    memset(&fmt->fmt.adpcm, '\0', sizeof(fmt->fmt.adpcm));

    fmt->read_sample = read_sample_fmt_adpcm;

    if (! read_le_s16(rw, &fmt->fmt.adpcm.cbSize) ||
        ! read_le_s16(rw, &fmt->fmt.adpcm.wSamplesPerBlock) ||
        ! read_le_s16(rw, &fmt->fmt.adpcm.wNumCoef))
	{
		return false;
	}

    fmt->fmt.adpcm.aCoef = new ADPCMCOEFSET[fmt->fmt.adpcm.wNumCoef];

    for (int i = 0; i < fmt->fmt.adpcm.wNumCoef; i++)
    {
        if (! read_le_s16(rw, &fmt->fmt.adpcm.aCoef[i].iCoef1) ||
            ! read_le_s16(rw, &fmt->fmt.adpcm.aCoef[i].iCoef2))
		{
			return false;
		}
    }

    fmt->fmt.adpcm.blockheaders = new ADPCMBLOCKHEADER[fmt->wChannels];

    return true; //OK
}


/*****************************************************************************
 * Everything else...                                                        *
 *****************************************************************************/

static bool read_fmt(file_c *f, fmt_t *fmt)
{
    // if it's in this switch statement, we support the format
    switch (fmt->wFormatTag)
    {
        case FMT_NORMAL:
            SND_DEBUG(("WAV: Appears to be uncompressed audio.\n"));
            return read_fmt_normal(f, fmt);

        case FMT_ADPCM:
            SND_DEBUG(("WAV: Appears to be ADPCM compressed audio.\n"));
            return read_fmt_adpcm(f, fmt);

        /* add other types here. */

        default:
			break;
	}

	SND_DEBUG(("WAV: Format 0x%X is unknown.\n",
			(unsigned int) fmt->wFormatTag));

    // FIXME BAIL_MACRO("WAV: Unsupported format", 0);

	return false;
}


/*
 * Locate a specific chunk in the WAVE file by ID...
 */
static bool find_chunk(file_c *f, u32_t id)
{
    int pos = f->GetPosition();

    for (;;)
    {
		u32_t got_id;

        if (! read_le_s32(f, &got_id))
			return false;

        if (got_id == id)
            return true;  // found it!

		// skip ahead to the next chunk is...
		s32_t len;

        if (! read_le_s32(f, &len))
			return false;

        SYS_ASSERT(len >= 0);

        pos += sizeof(u32_t) * 2 + size;

        if (size > 0)
		{
			if (! f->Seek(pos, file_c::SEEKPOINT_START))
				return false;
		}
    }

    return false; /* NOT REACHED */
}


sound_data_c * WAV_Load(file_c *f)
{
	fmt_t fmt;

    memset(&fmt, 0, sizeof(fmt_t));

    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    file_c *rw = internal->rw;
    data_t d;
    wav_t *w;
    u32_t pos;

	u32_t header_id;
	u32_t header_file_len;

	if (! read_le_u32(f, &header_id) || header_id != ID_RIFF)
		return NULL;  // FIXME "Not a RIFF file."

	read_le32(f, &header_file_len);

	if (! read_le_u32(f, &header_id) || header_id != ID_WAVE)
		return NULL;  // FIXME "Not a WAVE file."

    if (! find_chunk(f, ID_FMT))
		return NULL;  // FIXME: "Missing format chunk."

    if (! read_fmt_chunk(f, fmt))
		return NULL;  // FIXME "Can't read format chunk."

    sample->actual.channels = (u8_t) fmt->wChannels;
    sample->actual.rate = fmt->dwSamplesPerSec;

	SND_DEBUG(("WAV: %d bits per sample\n", (int) fmt->wBitsPerSample));

    if ((fmt->wBitsPerSample == 4) /*|| (fmt->wBitsPerSample == 0) */ )
        sample->actual.format = AUDIO_S16SYS;
    else if (fmt->wBitsPerSample == 8)
        sample->actual.format = AUDIO_U8;
    else if (fmt->wBitsPerSample == 16)
        sample->actual.format = AUDIO_S16LSB;
    else
    {
        // FIXME "WAV: Unsupported sample size."
		return NULL
    }

    if (! read_fmt(f, fmt))
		return NULL;

	f->Seek(fmt->next_chunk_offset, file_c::SEEKPOINT_START);

    if (! find_chunk(f, ID_DATA))
		// FIXME "Missing data chunk."
		return NULL;

    if (! read_data_chunk(f, &d))
		// FIXME "Can't read data chunk."
		return NULL

    w = (wav_t *) malloc(sizeof(wav_t));

    w->fmt = fmt;

    fmt->total_bytes = w->bytes_left = d.chunkSize;
    fmt->data_starting_offset = SDL_RWtell(rw);
    fmt->sample_frame_size = ( ((sample->actual.format & 0xFF) / 8) *
                               sample->actual.channels );

    internal->decoder_private = (void *) w;

    sample->flags = SOUND_SAMPLEFLAG_NONE;

    SND_DEBUG(("WAV: Accepting data stream.\n"));

	// !!!!!! TODO:  load the data stream !!!

    return wave;
}


} // namespace epi

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
