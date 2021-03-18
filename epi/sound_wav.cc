//----------------------------------------------------------------------------
//  WAV Format Sound Loading
//----------------------------------------------------------------------------
//
//  Copyright (c) 2007-2008  The EDGE Team.
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
#include "endianess.h"

#include "file_memory.h"
#include "sound_voc.h"
#include "sound_wav.h"
#include "sound_gather.h"
#include "../src/system/i_sdlinc.h"

#if 1
#define SND_DEBUG(args)  I_Debugf(args)
#else
#define SND_DEBUG(args)  do {} while (0)
#endif

#define READ_L16(a, i) (a[i]+(a[i+1]<<8))

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
		return read_le_u16(f, (u16_t *)val);
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
		return read_le_u32(f, (u32_t *)val);
	}

	/* This is just cleaner on the caller's end... */
	static inline bool read_uint8(file_c *f, u8_t *val)
	{
		return (1 == f->Read(val, 1));
	}

	/* Chunk management code... */

#define ID_RIFF  0x46464952U  /* "RIFF", in ascii. */
#define ID_WAVE  0x45564157U  /* "WAVE", in ascii. */
#define ID_FACT  0x74636166U  /* "fact", in ascii. */
#define ID_VOC	 0x4372656174697665U	  /* "VOC", in ascii. */

/*********************************************************
 * The FORMAT chunk...                                   *
 *********************************************************/

#define ID_FMT  0x20746D66U  /* "fmt ", in ascii. */

#define FMT_NORMAL 0x0001    /* Uncompressed waveform data.     */
#define FMT_ADPCM  0x0002    /* ADPCM compressed waveform data. */
#define FMT_IMA    0x0011    /* IMA ADPCM compressed waveform data. */

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

	typedef struct
	{
		s16_t iPrevSamp;
		u8_t iStepIndex;
	} IMAADPCMDATA;

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

	typedef struct S_IMA_FMT_T
	{
				/* per channel decoder state */
				IMAADPCMDATA* d;
				/* auxiliary buffer */
				u8_t* buf;

				u16_t block_frames;
				u16_t block_framesets;
				u16_t enc_frameset_size;
				u16_t headerset_size;
				u16_t dec_frame_size;
				u16_t dec_frameset_size;
				u16_t rem_block_framesets;

				/* whether the next word(s) are the start of a new block */
				int read_header;
			} ima;


			/* put other format-specific data here... */
		} fmt;
	}
	fmt_t;

	static int(*read_sample)(s16_t *buffer, int max_samples);

	/*
	 * Read in a fmt_t from disk. This makes this process safe regardless of
	 *  the processor's byte order or how the fmt_t structure is packed.
	 * Note that the union "fmt" is not read in here; that is handled as
	 *  needed in the read_fmt_* functions.
	 */
	//we do not use SDL_RWops *rw, instead file_c *f
	//SDLRWtell -> f->GetPosition 
	static bool read_fmt_chunk(file_c *f, fmt_t *fmt)
	{
		/* skip reading the chunk ID, since it was already read at this point... */
		fmt->chunkID = ID_FMT;

		if (!read_le_s32(f, &fmt->chunkSize))
			return false;

		if (fmt->chunkSize < 16)
		{
			I_Printf("WAV: Invalid chunk size");
			return false;
		}

		fmt->next_chunk_offset = f->GetPosition() + fmt->chunkSize;

		return (read_le_s16(f, &fmt->wFormatTag) &&
			read_le_u16(f, &fmt->wChannels) &&
			read_le_u32(f, &fmt->dwSamplesPerSec) &&
			read_le_u32(f, &fmt->dwAvgBytesPerSec) &&
			read_le_u16(f, &fmt->wBlockAlign) &&
			read_le_u16(f, &fmt->wBitsPerSample));
	}

	/*********************************************************
	 * The DATA chunk...                                     *
	 *********************************************************/

#define ID_DATA  0x61746164U  /* "data", in ascii. */

	 /*****************************************************************************
	  * this is what we store in our internal->decoder_private field...           *
	  *****************************************************************************/

	static fmt_t decoder_fmt;

	typedef struct
	{
		fmt_t *fmt;

		int bytes_left;
	}
	wav_t;

	//below EDGE specific

	static wav_t decoder_wavt;

	static file_c *decode_F;

	static bool decode_eof;
	static bool decode_error;

	/*****************************************************************************
	 * Normal, uncompressed waveform handler...                                  *
	 *****************************************************************************/

	 /*
	  * Sound_Decode() lands here for uncompressed WAVs...
	  */
	static int read_sample_fmt_normal(s16_t *buffer, int max_samples)
	{
		wav_t *w = &decoder_wavt;
		fmt_t *fmt = w->fmt;

		bool is_stereo = (fmt->wChannels == 2);

		int bytes_each = (fmt->wBitsPerSample / 8) * (is_stereo ? 2 : 1);

		int want = MIN(max_samples, w->bytes_left / bytes_each);

		if (want == 0)
		{
			decode_eof = true;
			return 0;
		}

		/*
		 * We don't actually do any decoding, so we read the wav data
		 * directly into the internal buffer...
		 */
		//u32_t retval = SDL_RWread(internal->rw, internal->buffer, 1, max);
		//decode_F->Read is RWread, remember, internal->rw = buffer, then ext bullshit is always want * bytes (each)
		int got_bytes = decode_F->Read(buffer, want * bytes_each);  // FIXME: DECODE U8 --> S16

		if (got_bytes < 0)
		{
			decode_error = true;
			return got_bytes;
		}

		/* Make sure the read went smoothly... */
		if (got_bytes == 0)
		{
			decode_eof = true;
			return 0;
		}

		// FIXME: handle case of F->Read() returns odd number (for S16)

		if (fmt->wBitsPerSample == 8)
		{
			// convert U8 samples to S16
			for (int i = got_bytes - 1; i >= 0; i--)
			{
				u8_t src = ((u8_t *)buffer)[i];

				buffer[i] = (src ^ 0x80) << 8;
			}
		}
		else
		{
			// endian swap 16-bit samples
			for (int i = 0; i < got_bytes / 2; i++)
				buffer[i] = EPI_LE_S16(buffer[i]);
		}

#ifdef LOW_PASS_FILTER
		// Perform a low-pass filter on the upscaled sound to filter
		// out high-frequency noise from the conversion process.

		{
			float rc, dt, alpha;

			// Low-pass filter for cutoff frequency f:
			//
			// For sampling rate r, dt = 1 / r
			// rc = 1 / 2*pi*f
			// alpha = dt / (rc + dt)

			// Filter to the half sample rate of the original sound effect
			// (maximum frequency, by nyquist)

			dt = 1.0f / mixer_freq;
			rc = 1.0f / (3.14f * samplerate);
			alpha = dt / (rc + dt);

			// Both channels are processed in parallel, hence [i-2]:

			for (i = 2; i<expanded_length * 2; ++i)
			{
				expanded[i] = (Sint16)(alpha * expanded[i]
					+ (1 - alpha) * expanded[i - 2]);
			}
		}
#endif /* #ifdef LOW_PASS_FILTER */

		w->bytes_left -= got_bytes;

		return got_bytes / bytes_each;
	}

	static bool read_fmt_normal(file_c *f, fmt_t *fmt)
	{
		/* (don't need to read more from the RWops...) */
		read_sample = read_sample_fmt_normal;

		return true; //OK
	}

	/*********************************************************
	 * ADPCM compression handler...                          *
	 *********************************************************/

#define FIXED_POINT_COEF_BASE      256
#define FIXED_POINT_ADAPTION_BASE  256
#define SMALLEST_ADPCM_DELTA       16

	static inline bool read_adpcm_block_headers(file_c *f)
	{
		wav_t *w = &decoder_wavt;
		fmt_t *fmt = w->fmt;

		ADPCMBLOCKHEADER *headers = fmt->fmt.adpcm.blockheaders;

		if (w->bytes_left < fmt->wBlockAlign)
		{
			decode_eof = true;
			return(0);
		}

		w->bytes_left -= fmt->wBlockAlign;

		int i;
		for (i = 0; i < fmt->wChannels; i++)
			if (!read_uint8(f, &headers[i].bPredictor))
				return false;

		for (i = 0; i < fmt->wChannels; i++)
			if (!read_le_u16(f, &headers[i].iDelta))
				return false;

		for (i = 0; i < fmt->wChannels; i++)
			if (!read_le_s16(f, &headers[i].iSamp1))
				return false;

		for (i = 0; i < fmt->wChannels; i++)
			if (!read_le_s16(f, &headers[i].iSamp2))
				return false;

		fmt->fmt.adpcm.samples_left_in_block = fmt->fmt.adpcm.wSamplesPerBlock;
		fmt->fmt.adpcm.nibble_state = 0;

		return true; //OK
	}

	static inline void do_adpcm_nibble(u8_t nib,
		ADPCMBLOCKHEADER *header,
		s32_t lPredSamp)
	{
		static const s32_t max_audioval = (1 << 15) - 1;
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

		delta = ((s32_t)header->iDelta * AdaptionTable[nib]) /
			FIXED_POINT_ADAPTION_BASE;

		if (delta < SMALLEST_ADPCM_DELTA)
			delta = SMALLEST_ADPCM_DELTA;

		header->iDelta = delta;
		header->iSamp2 = header->iSamp1;
		header->iSamp1 = lNewSamp;
	}

	static inline int decode_adpcm_sample_frame()
	{
		wav_t *w = &decoder_wavt;
		fmt_t *fmt = w->fmt;

		ADPCMBLOCKHEADER *headers = fmt->fmt.adpcm.blockheaders;

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
				if (!read_uint8(decode_F, &nib)) // FIXME !!!
					I_Error("WAV DECODE ERROR: adcpm read\n");

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
		u16_t *buf = (u16_t *)_buf;
		ADPCMBLOCKHEADER *headers = fmt->fmt.adpcm.blockheaders;

		for (int i = 0; i < fmt->wChannels; i++)
			*(buf++) = headers[i].iSamp1;
	}

	static inline void put_adpcm_sample_frame2(void *_buf, fmt_t *fmt)
	{
		u16_t *buf = (u16_t *)_buf;
		ADPCMBLOCKHEADER *headers = fmt->fmt.adpcm.blockheaders;
		int i;
		for (i = 0; i < fmt->wChannels; i++)
			*(buf++) = headers[i].iSamp2;
	}

	/*
	 * Sound_Decode() lands here for ADPCM-encoded WAVs...
	 */
	static int read_sample_fmt_adpcm(s16_t *buffer, int max_samples)
	{
		wav_t *w = &decoder_wavt;
		fmt_t *fmt = w->fmt;
		int bw = 0;

		while (bw < max_samples)
		{
			/* write ongoing sample frame before reading more data... */
			switch (fmt->fmt.adpcm.samples_left_in_block)
			{
			case 0:  /* need to read a new block... */
				if (!read_adpcm_block_headers(decode_F))
				{
					if (!decode_eof)
						decode_error = true;
					return(bw);
				} /* if */

				/* only write first sample frame for now. */
				put_adpcm_sample_frame2((u8_t *)buffer + bw, fmt);
				fmt->fmt.adpcm.samples_left_in_block--;
				bw += fmt->sample_frame_size;
				break;

			case 1:  /* output last sample frame of block... */
				put_adpcm_sample_frame1((u8_t *)buffer + bw, fmt);
				fmt->fmt.adpcm.samples_left_in_block--;
				bw += fmt->sample_frame_size;
				break;

			default: /* output latest sample frame and read a new one... */
				put_adpcm_sample_frame1((u8_t *)buffer + bw, fmt);
				fmt->fmt.adpcm.samples_left_in_block--;
				bw += fmt->sample_frame_size;

				if (!decode_adpcm_sample_frame())
				{
					decode_error = true;
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
	static bool read_fmt_adpcm(file_c *rw, fmt_t *fmt)
	{
		memset(&fmt->fmt.adpcm, 0, sizeof(fmt->fmt.adpcm));

		read_sample = read_sample_fmt_adpcm;

		if (!read_le_u16(rw, &fmt->fmt.adpcm.cbSize) ||
			!read_le_u16(rw, &fmt->fmt.adpcm.wSamplesPerBlock) ||
			!read_le_u16(rw, &fmt->fmt.adpcm.wNumCoef))
		{
			return false;
		}

		fmt->fmt.adpcm.aCoef = new ADPCMCOEFSET[fmt->fmt.adpcm.wNumCoef];

		for (int i = 0; i < fmt->fmt.adpcm.wNumCoef; i++)
		{
			if (!read_le_s16(rw, &fmt->fmt.adpcm.aCoef[i].iCoef1) ||
				!read_le_s16(rw, &fmt->fmt.adpcm.aCoef[i].iCoef2))
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

		I_Debugf("WAV Loader: Format 0x%X is unknown.\n", (u32_t)fmt->wFormatTag);

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

			if (!read_le_u32(f, &got_id))
				return false;

			if (got_id == id)
				return true;  // found it!

			// skip ahead to the next chunk is...
			s32_t len;

			if (!read_le_s32(f, &len))
				return false;

			SYS_ASSERT(len >= 0);

			pos += sizeof(u32_t) * 2 + len;

			if (len > 0)
			{
				if (!f->Seek(pos, file_c::SEEKPOINT_START))
					return false;
			}
		}

		return false; /* NOT REACHED */
	}



#if 0
	char* WAV_ConvertVOCtoWAV(sound_data_c *buf, int sndnum, int size)
	{
		file_c *f;
		//WAVEFILEHEADER *wfh;
		decode_F = f;
		int GetLength = f->GetLength();

		memset(&decoder_fmt, 0, sizeof(decoder_fmt));

		decoder_wavt.fmt = &decoder_fmt;

		wav_t *w = &decoder_wavt; //WAVEFILEHEADER I assume
		fmt_t *fmt = w->fmt;

		u32_t header_id;
		u32_t header_file_len;
		char *pData, *wavdata, *vocdata;

		// From this point, we assume VOC has been decoded and figured out.

		pData = (char *)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, GetLength + sizeof(wav_t));
		//reset our sound mem

		memset(pData, 0x80, GetLength + sizeof(w->fmt));

		w = (wav_t *)pData;
		lstrcpy((LPSTR)ID_RIFF, (LPSTR)"RIFF");    //Contains the letters "RIFF" in ASCII form (0x52494646 big-endian form).
		w->fmt->chunkSize = GetLength + 36;     //36 + SubChunk2Size, or more precisely:
		lstrcpy((LPSTR)ID_WAVE, (LPSTR)"WAVE");     //Contains the letters "WAVE"
		lstrcpy((LPSTR)ID_FMT, (LPSTR)"fmt ");
		wfh->Subchunk1Size = 16;
		wfh->AudioFormat = 1;
		w->fmt->wChannels = psFileInfo.ucChannels;
		wfh->SampleRate = 11025;//psFileInfo.lSamplesPerSeconds;
		wfh->ByteRate = 11025;//psFileInfo.lSamplesPerSeconds;
		wfh->BlockAlign = 1;
		wfh->BitsPerSample = psFileInfo.ucBitsPerSample;
		lstrcpy((LPSTR)wfh->Subchunk2ID, (LPSTR)"data");//Contains the letters "fmt "
		wfh->Subchunk2Size = psFileInfo.lTotalLength;

		wavdata = pData + sizeof(WAVEFILEHEADER);
		memcpy(wavdata, (vocdata), psFileInfo.lTotalLength);

		GlobalFree(vocdata);
		return (char*)pData;

	}
#endif // 0


#if 0
	bool vocToWav(mem_file_c& in, mem_file_c& out)
	{
		//SLADE difference: header has already been checked as being a VOC file, so we can remove that here.

		memset(&decoder_fmt, 0, sizeof(decoder_fmt));

		decoder_wavt.fmt = &decoder_fmt;

		wav_t *w = &decoder_wavt;
		fmt_t *fmt = w->fmt;
		// --- Prepare WAV ---
		wav_chunk_t whdr, wdhdr;
		wav_fmtchunk_t fmtchunk;

		// --- Pre-process the file to make sure we can convert it ---
		int codec = -1;
		int blockcount = 0;
		size_t datasize = 0;
		size_t i = 26, e = in.GetLength();
		bool gotextra = false;
		while (i < e)
		{
			// Parses through blocks
			uint8_t blocktype = in[i];
			size_t blocksize = (i + 4 < e) ? READ_L24(in, i + 1) : 0x1000000;
			i += 4;
			if (i + blocksize > e && i < e && blocktype != 0)
			{
				Global::error = S_FMT("VOC file cut abruptly in block %i", blockcount);
				return false;
			}
			blockcount++;
			switch (blocktype)
			{
			case 0: // Terminator, the rest should be ignored
				i = e; break;
			case 1: // Sound data
				if (!gotextra && codec >= 0 && codec != in[i + 1])
				{
					Global::error = "VOC files with different codecs are not supported";
					return false;
				}
				else if (codec == -1)
				{
					fmtchunk.samplerate = 1000000 / (256 - in[i]);
					fmtchunk.channels = 1;
					fmtchunk.tag = 1;
					codec = in[i + 1];
				}
				datasize += blocksize - 2;
				break;
			case 2: // Sound data continuation
				if (codec == -1)
				{
					Global::error = "Sound data without codec in VOC file";
					return false;
				}
				datasize += blocksize;
				break;
			case 3: // Silence
			case 4: // Marker
			case 5: // Text
			case 6: // Repeat start point
			case 7: // Repeat end point
				break;
			case 8: // Extra info, overrides any following sound data codec info
				if (codec != -1)
				{
					Global::error = "Extra info block must precede sound data info block in VOC file";
					return false;
				}
				else
				{
					fmtchunk.samplerate = 256000000 / ((in[i + 3] + 1) * (65536 - READ_L16(in, i)));
					fmtchunk.channels = in[i + 3] + 1;
					fmtchunk.tag = 1;
					codec = in[i + 2];
				}
				break;
			case 9: // Sound data in new format
				if (codec >= 0 && codec != READ_L16(in, i + 6))
				{
					Global::error = "VOC files with different codecs are not supported";
					return false;
				}
				else if (codec == -1)
				{
					fmtchunk.samplerate = READ_L32(in, i);
					fmtchunk.bps = in[i + 4];
					fmtchunk.channels = in[i + 5];
					fmtchunk.tag = 1;
					codec = READ_L16(in, i + 6);
				}
				datasize += blocksize - 12;
				break;
			}
			i += blocksize;
		}
		wdhdr.size = datasize;
		switch (codec)
		{
		case 0: // 8 bits unsigned PCM
			fmtchunk.bps = 8;
			fmtchunk.datarate = fmtchunk.samplerate;
			fmtchunk.blocksize = 1;
			break;
		case 4: // 16 bits signed PCM
			fmtchunk.bps = 16;
			fmtchunk.datarate = fmtchunk.samplerate << 1;
			fmtchunk.blocksize = 2;
			break;
		case 1: // 4 bits to 8 bits Creative ADPCM
		case 2: // 3 bits to 8 bits Creative ADPCM (AKA 2.6 bits)
		case 3: // 2 bits to 8 bits Creative ADPCM
		case 6: // alaw
		case 7: // ulaw
		case 0x200: // 4 bits to 16 bits Creative ADPCM (only valid in block type 0x09)
			Global::error = S_FMT("Unsupported codec %i in VOC file", codec);
			return false;
		default:
			Global::error = S_FMT("Unknown codec %i in VOC file", codec);
			return false;
		}

		// --- Write WAV ---

		// Setup data header
		char did[4] = { 'd', 'a', 't', 'a' };
		memcpy(&wdhdr.id, &did, 4);

		// Setup fmt chunk
		char fid[4] = { 'f', 'm', 't', ' ' };
		memcpy(&fmtchunk.header.id, &fid, 4);
		fmtchunk.header.size = 16;

		// Setup main header
		char wid[4] = { 'R', 'I', 'F', 'F' };
		memcpy(&whdr.id, &wid, 4);
		whdr.size = wdhdr.size + fmtchunk.header.size + 8;

		// Write chunks
		out.write(&whdr, 8);
		out.write("WAVE", 4);
		out.write(&fmtchunk, sizeof(wav_fmtchunk_t));
		out.write(&wdhdr, 8);

		// Now go and copy sound data
		const uint8_t* src = in.getData();
		i = 26;
		while (i < e)
		{
			// Parses through blocks again
			uint8_t blocktype = in[i];
			size_t blocksize = READ_L24(in, i + 1);
			i += 4;
			switch (blocktype)
			{
			case 1: // Sound data
				out.write(src + i + 2, blocksize - 2);
				break;
			case 2: // Sound data continuation
				out.write(src + i, blocksize);
				break;
			case 3: // Silence
					// Not supported yet
				break;
			case 9: // Sound data in new format
				out.write(src + i + 12, blocksize - 12);
			default:
				break;
			}
			i += blocksize;
		}

		return true;
	}

#endif // 0


	bool WAV_Load(sound_data_c *buf, file_c *f)
	{
		decode_F = f;

		memset(&decoder_fmt, 0, sizeof(decoder_fmt));

		decoder_wavt.fmt = &decoder_fmt;

		wav_t *w = &decoder_wavt;
		fmt_t *fmt = w->fmt;

		u32_t header_id;
		u32_t header_file_len;

		//if (!read_le_u32(f, &header_id) || header_id != ID_VOC)
		//{
		//	I_Warning("WAV Loader: Not a VOC file??\n");
			//return false;// false;
		//}

		if (!read_le_u32(f, &header_id) || header_id != ID_RIFF)
		{
			I_Warning("WAV Loader: Not a RIFF file.\n");
			return false;
		}

		read_le_u32(f, &header_file_len);

		if (!read_le_u32(f, &header_id) || header_id != ID_WAVE)
		{
			I_Warning("WAV Loader: Not a RIFF/WAVE file.\n");
			return false;// false;
		}

		if (!find_chunk(f, ID_FMT))
		{
			I_Warning("WAV Loader: Missing [fmt] chunk.\n");
			return false;
		}

		if (!read_fmt_chunk(f, fmt))
		{
			I_Warning("WAV Loader: Cannot decode [fmt] chunk.\n");
			return false;
		}

		int freq = fmt->dwSamplesPerSec;
		int bits = fmt->wBitsPerSample;
		int channels = fmt->wChannels;

		if (channels > 2)
		{
			I_Warning("WAV Loader: too many channels: %d\n", channels);
			return false;
		}

		bool is_stereo = (channels == 2);

		I_Debugf("WAV Loader: freq %d Hz, %d channels, %d sample bits\n",
			freq, channels, bits);

		buf->freq = freq;

		if (bits == 32)
		{
			I_Warning("WAV Loader: Floating point not supported.\n");
			return false;
		}
		// FIXME: 4 used for ADPCM
		else if (!(bits == 8 || bits == 16))
		{
			I_Warning("WAV Loader: Unsupported sample bits: %d\n", bits);
			return false;
		}

		if (!read_fmt(f, fmt))
			return false;

		f->Seek(fmt->next_chunk_offset, file_c::SEEKPOINT_START);

		if (!find_chunk(f, ID_DATA))
		{
			I_Warning("WAV Loader: Missing [data] chunk.\n");
			return false;
		}

		s32_t data_size;

		if (!read_le_s32(f, &data_size))
		{
			I_Warning("WAV Loader: Cannot get [data] chunk size.\n");
			return false;
		}

		fmt->total_bytes = w->bytes_left = data_size;

		fmt->sample_frame_size = (sizeof(s16_t) * channels); //!!!!! FIXME: made up shit

		decode_eof = decode_error = false;

		SND_DEBUG(("WAV: Accepting data stream.\n"));

		// load the data stream

		sound_gather_c gather;

		while (!decode_error)
		{
			int want = 2048;

			s16_t *buffer = gather.MakeChunk(want, is_stereo);

			int got_num = (*read_sample)(buffer, want);

			if (got_num < 0)
				I_Error("WAV Loader: Error reading file (%d)\n", got_num);

			if (got_num == 0)  // EOF
			{
				gather.DiscardChunk();
				break;
			}

			gather.CommitChunk(got_num);
		}

		if (!gather.Finalise(buf, false /* want_stereo */))
			I_Error("WAV Loader: no samples!\n");

		return true;
	}
} // namespace epi

  /********************************************************************/
  /*																	*/
  /* Function name : DecodeVoc			                            */
  /* Description   :													*/
  /*																	*/
  /********************************************************************/

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab