/*
 *
 * ParseAVI - Copyright 2015-2016 by Chilly Willy <ChillyWillyGuru@gmail.com>
 *
 * based on
 *  readavi - Copyright 2004-2013 by Michael Kohn <mike@mikekohn.net>
 *
 * This code falls under the BSD license.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "avi.h"
#include "fmv.h"
#include "roq/roq.h"
#include "pcm/pcm.h"


int movi_pos, movi_size;
int idx1_pos, idx1_size;

avi_header_t avi_header;
stream_header_t stream_header;
stream_format_t stream_format;
stream_header_t astream_header;
stream_format_auds_t astream_format;
stream_header_t tstream_header;

unsigned char *read_buffer;

short *pcm_buffer;
int pcm_samples, pcm_rix, pcm_wix, pcm_cnt;
float arix;
int lstep, lpred;
int rstep, rpred;

unsigned short *frame[2];
GLuint rtex, wtex, ctex;
GLuint tex[NUM_GL_TEXTURES];
int texture_width, texture_height;
float scale_width, scale_height;

static int hex_dump_chunk(FILE *in, int chunk_len)
{
#ifdef FMV_DUMP_EXTRA
	char chars[17];
	int ch,n;

	chars[16]=0;

	for (n=0; n<chunk_len; n++)
	{
		if ((n%16)==0)
		{
			if (n!=0)
				I_Printf("%s\n", chars);
			I_Printf("      ");
			memset(chars, ' ', 16);
		}
		ch=getc(in);
		if (ch==EOF)
			break;
		I_Printf("%02x ", ch);
		if (ch>=' ' && ch<=126)
		{
			chars[n%16]=ch;
		}
		else
		{
			chars[n%16]='.';
		}
	}

	if ((n%16)!=0)
	{
		for (ch=n%16; ch<16; ch++)
		{
			I_Printf("   ");
		}
	}
	I_Printf("%s\n", chars);
#endif

	return 0;
}

//
// Find index for frame, return offset in 'movi' chunk
//
int parse_idx1(FILE *in, int frame)
{
	struct index_entry_t index_entry;
	int t, chunk_len = idx1_size;

	in->Seek(idx1_pos, SEEKPOINT_START);

#ifdef FMV_DEBUG
	I_Printf("   IDX1\n");
	I_Printf("   --------------------------------\n");
	I_Printf("   Chunk Flags    Offset   Length  \n");
#endif

	for (t=0; t<chunk_len/16; t++)
	{
		read_chars(in,index_entry.ckid,4);
		index_entry.ckid[4] = 0;
		index_entry.dwFlags=read_long(in);
		index_entry.dwChunkOffset=read_long(in);
		index_entry.dwChunkLength=read_long(in);

#ifdef FMV_DEBUG
		I_Printf("   %s  0x%08x 0x%08x 0x%08x\n",
			 index_entry.ckid,
			 index_entry.dwFlags,
			 index_entry.dwChunkOffset,
			 index_entry.dwChunkLength);
#endif

		if (!memcmp(index_entry.ckid, "00dc", 4))
		{
			frame--;
			if (!frame)
				break;
		}
	}

#ifdef FMV_DEBUG
	I_Printf("   --------------------------------\n");
#endif

	return index_entry.dwChunkOffset;
}

static int read_avi_header(FILE *in)
{
	long offset=f_tell(in);

	avi_header.TimeBetweenFrames=read_long(in);
	avi_header.MaximumDataRate=read_long(in);
	avi_header.PaddingGranularity=read_long(in);
	avi_header.Flags=read_long(in);
	avi_header.TotalNumberOfFrames=read_long(in);
	avi_header.NumberOfInitialFrames=read_long(in);
	avi_header.NumberOfStreams=read_long(in);
	avi_header.SuggestedBufferSize=read_long(in);
	avi_header.Width=read_long(in);
	avi_header.Height=read_long(in);
	avi_header.TimeScale=read_long(in);
	avi_header.DataRate=read_long(in);
	avi_header.StartTime=read_long(in);
	avi_header.DataLength=read_long(in);

#ifdef FMV_DEBUG
	I_Printf("   offset=0x%lx\n",offset);
	I_Printf("       TimeBetweenFrames: %d\n",avi_header.TimeBetweenFrames);
	I_Printf("         MaximumDataRate: %d\n",avi_header.MaximumDataRate);
	I_Printf("      PaddingGranularity: %d\n",avi_header.PaddingGranularity);
	I_Printf("                   Flags: %d\n",avi_header.Flags);
	I_Printf("     TotalNumberOfFrames: %d\n",avi_header.TotalNumberOfFrames);
	I_Printf("   NumberOfInitialFrames: %d\n",avi_header.NumberOfInitialFrames);
	I_Printf("         NumberOfStreams: %d\n",avi_header.NumberOfStreams);
	I_Printf("     SuggestedBufferSize: %d\n",avi_header.SuggestedBufferSize);
	I_Printf("                   Width: %d\n",avi_header.Width);
	I_Printf("                  Height: %d\n",avi_header.Height);
	I_Printf("               TimeScale: %d\n",avi_header.TimeScale);
	I_Printf("                DataRate: %d\n",avi_header.DataRate);
	I_Printf("               StartTime: %d\n",avi_header.StartTime);
	I_Printf("              DataLength: %d\n",avi_header.DataLength);
#endif

	return FMV_ERR_NONE;
}

#ifdef FMV_DEBUG
static void print_data_chars(unsigned char *string)
{
	int t;

	for (t=0; t<4; t++)
	{
		if ((string[t]>='a' && string[t]<='z') ||
			(string[t]>='A' && string[t]<='Z') ||
			(string[t]>='0' && string[t]<='9'))
		{
			I_Printf("%c",string[t]);
		}
		else
		{
			if (t==0)
				I_Printf("%s", "[0x");
			I_Printf("%02x",string[t]);
			if (t==3)
				I_Printf("%s", "]");
		}
	}
}
#endif

static int read_stream_header(FILE *in)
{
	long offset=f_tell(in);

	read_chars(in,stream_header.DataType,4);
	read_chars(in,stream_header.DataHandler,4);
	stream_header.Flags=read_long(in);
	stream_header.Priority=read_long(in);
	stream_header.InitialFrames=read_long(in);
	stream_header.TimeScale=read_long(in);
	stream_header.DataRate=read_long(in);
	stream_header.StartTime=read_long(in);
	stream_header.DataLength=read_long(in);
	stream_header.SuggestedBufferSize=read_long(in);
	stream_header.Quality=read_long(in);
	stream_header.SampleSize=read_long(in);

#ifdef FMV_DEBUG
	I_Printf("    offset=0x%lx\n",offset);
	I_Printf("              DataType: %s\n",stream_header.DataType);
	I_Printf("           DataHandler: ");
	print_data_chars((unsigned char *)stream_header.DataHandler);
	I_Printf("\n");
	I_Printf("                 Flags: %d\n",stream_header.Flags);
	I_Printf("              Priority: %d\n",stream_header.Priority);
	I_Printf("         InitialFrames: %d\n",stream_header.InitialFrames);
	I_Printf("             TimeScale: %d\n",stream_header.TimeScale);
	I_Printf("              DataRate: %d\n",stream_header.DataRate);
	I_Printf("             StartTime: %d\n",stream_header.StartTime);
	I_Printf("            DataLength: %d\n",stream_header.DataLength);
	I_Printf("   SuggestedBufferSize: %d\n",stream_header.SuggestedBufferSize);
	I_Printf("               Quality: %d\n",stream_header.Quality);
	I_Printf("            SampleSize: %d\n",stream_header.SampleSize);
#endif

  return FMV_ERR_NONE;
}

static int read_stream_header_auds(FILE *in)
{
	long offset=f_tell(in);

	read_chars(in,astream_header.DataType,4);
	read_chars(in,astream_header.DataHandler,4);
	astream_header.Flags=read_long(in);
	astream_header.Priority=read_long(in);
	astream_header.InitialFrames=read_long(in);
	astream_header.TimeScale=read_long(in);
	astream_header.DataRate=read_long(in);
	astream_header.StartTime=read_long(in);
	astream_header.DataLength=read_long(in);
	astream_header.SuggestedBufferSize=read_long(in);
	astream_header.Quality=read_long(in);
	astream_header.SampleSize=read_long(in);

#ifdef FMV_DEBUG
	I_Printf("    offset=0x%lx\n",offset);
	I_Printf("              DataType: %s\n",astream_header.DataType);
	I_Printf("           DataHandler: ");
	print_data_chars((unsigned char *)astream_header.DataHandler);
	I_Printf("\n");
	I_Printf("                 Flags: %d\n",astream_header.Flags);
	I_Printf("              Priority: %d\n",astream_header.Priority);
	I_Printf("         InitialFrames: %d\n",astream_header.InitialFrames);
	I_Printf("             TimeScale: %d\n",astream_header.TimeScale);
	I_Printf("              DataRate: %d\n",astream_header.DataRate);
	I_Printf("             StartTime: %d\n",astream_header.StartTime);
	I_Printf("            DataLength: %d\n",astream_header.DataLength);
	I_Printf("   SuggestedBufferSize: %d\n",astream_header.SuggestedBufferSize);
	I_Printf("               Quality: %d\n",astream_header.Quality);
	I_Printf("            SampleSize: %d\n",astream_header.SampleSize);
#endif

  return FMV_ERR_NONE;
}

static int read_stream_format(FILE *in)
{
	int t,r,g,b;
	long offset=f_tell(in);

	stream_format.header_size=read_long(in);
	stream_format.image_width=read_long(in);
	stream_format.image_height=read_long(in);
	stream_format.number_of_planes=read_word(in);
	stream_format.bits_per_pixel=read_word(in);
	stream_format.compression_type=read_long(in);
	stream_format.image_size_in_bytes=read_long(in);
	stream_format.x_pels_per_meter=read_long(in);
	stream_format.y_pels_per_meter=read_long(in);
	stream_format.colors_used=read_long(in);
	stream_format.colors_important=read_long(in);
	stream_format.palette=0;

	if (stream_format.colors_important!=0)
	{
		stream_format.palette=malloc(stream_format.colors_important*sizeof(int));
		for (t=0; t<stream_format.colors_important; t++)
		{
			b=getc(in);
			g=getc(in);
			r=getc(in);
			stream_format.palette[t]=(r<<16)+(g<<8)+b;
		}
	}

#ifdef FMV_DEBUG
	I_Printf("    offset=0x%lx\n",offset);
	I_Printf("           header_size: %d\n",stream_format.header_size);
	I_Printf("           image_width: %d\n",stream_format.image_width);
	I_Printf("          image_height: %d\n",stream_format.image_height);
	I_Printf("      number_of_planes: %d\n",stream_format.number_of_planes);
	I_Printf("        bits_per_pixel: %d\n",stream_format.bits_per_pixel);
	I_Printf("      compression_type: ");
	print_data_chars((unsigned char *)&stream_format.compression_type);
	I_Printf("\n");
	I_Printf("   image_size_in_bytes: %d\n",stream_format.image_size_in_bytes);
	I_Printf("      x_pels_per_meter: %d\n",stream_format.x_pels_per_meter);
	I_Printf("      y_pels_per_meter: %d\n",stream_format.y_pels_per_meter);
	I_Printf("           colors_used: %d\n",stream_format.colors_used);
	I_Printf("      colors_important: %d\n",stream_format.colors_important);
#endif

	return FMV_ERR_NONE;
}

static int read_stream_format_auds(FILE *in)
{
	long offset=f_tell(in);

	astream_format.format=read_word(in);
	astream_format.channels=read_word(in);
	astream_format.samples_per_second=read_long(in);
	astream_format.bytes_per_second=read_long(in);
	int block_align=read_word(in);
	astream_format.block_size_of_data=read_word(in);
	astream_format.bits_per_sample=read_word(in);
	//astream_format.extended_size=read_word(in);

#ifdef FMV_DEBUG
	I_Printf("    offset=0x%lx\n",offset);
	I_Printf("                format: %d\n",astream_format.format);
	I_Printf("              channels: %d\n",astream_format.channels);
	I_Printf("    samples_per_second: %d\n",astream_format.samples_per_second);
	I_Printf("      bytes_per_second: %d\n",astream_format.bytes_per_second);
	I_Printf("           block_align: %d\n",block_align);
	I_Printf("    block_size_of_data: %d\n",astream_format.block_size_of_data);
	I_Printf("       bits_per_sample: %d\n",astream_format.bits_per_sample);
#endif

	return FMV_ERR_NONE;
}

static int read_stream_header_txts(FILE *in)
{
	long offset=f_tell(in);

	read_chars(in,tstream_header.DataType,4);
	read_chars(in,tstream_header.DataHandler,4);
	tstream_header.Flags=read_long(in);
	tstream_header.Priority=read_long(in); // priority | language << 16
	tstream_header.InitialFrames=read_long(in);
	tstream_header.TimeScale=read_long(in);
	tstream_header.DataRate=read_long(in);
	tstream_header.StartTime=read_long(in);
	tstream_header.DataLength=read_long(in);
	tstream_header.SuggestedBufferSize=read_long(in);
	tstream_header.Quality=read_long(in);
	tstream_header.SampleSize=read_long(in);

#ifdef FMV_DEBUG
	I_Printf("    offset=0x%lx\n",offset);
	I_Printf("              DataType: %s\n",tstream_header.DataType);
	I_Printf("           DataHandler: ");
	print_data_chars((unsigned char *)tstream_header.DataHandler);
	I_Printf("\n");
	I_Printf("                 Flags: %d\n",tstream_header.Flags);
	I_Printf("              Priority: %d\n",tstream_header.Priority & 0xFFFF);
	I_Printf("              Language: %04x\n",tstream_header.Priority >> 16);
	I_Printf("         InitialFrames: %d\n",tstream_header.InitialFrames);
	I_Printf("             TimeScale: %d\n",tstream_header.TimeScale);
	I_Printf("              DataRate: %d\n",tstream_header.DataRate);
	I_Printf("             StartTime: %d\n",tstream_header.StartTime);
	I_Printf("            DataLength: %d\n",tstream_header.DataLength);
	I_Printf("   SuggestedBufferSize: %d\n",tstream_header.SuggestedBufferSize);
	I_Printf("               Quality: %d\n",tstream_header.Quality);
	I_Printf("            SampleSize: %d\n",tstream_header.SampleSize);
#endif

  return FMV_ERR_NONE;
}

static int parse_hdrl_list(FILE *in)
{
	char chunk_id[5];
	int chunk_size;
	char chunk_type[5];
	int end_of_chunk;
	int next_chunk;
	long offset=f_tell(in);
	int stream_type=0;     // 0=video 1=sound 2=subtitles
	int ret;

	read_chars(in,chunk_id,4);
	chunk_size=read_long(in);
	read_chars(in,chunk_type,4);

#ifdef FMV_DEBUG
	I_Printf("  AVI Header LIST (id=%s size=%d type=%s offset=0x%lx)\n",chunk_id,chunk_size,chunk_type,offset);
	I_Printf("  {\n");
#endif

	end_of_chunk=f_tell(in)+chunk_size-4;
	if ((end_of_chunk%4)!=0)
	{
#ifdef FMV_ERROR
		I_Printf("  Adjusting end of chunk %d\n", end_of_chunk);
		end_of_chunk=end_of_chunk+(4-(end_of_chunk%4));
		I_Printf("  Adjusting end of chunk %d\n", end_of_chunk);
#endif
	}

	if (memcmp(chunk_id,"JUNK",4)==0)
	{
		in->Seek(end_of_chunk,SEEKPOINT_START);
#ifdef FMV_DEBUG
		I_Printf("  }\n");
#endif
		return FMV_ERR_NONE;
	}

	while (f_tell(in)<end_of_chunk)
	{
		long offset=f_tell(in);
		read_chars(in,chunk_type,4);
		chunk_size=read_long(in);
		next_chunk=f_tell(in)+chunk_size;
		if ((chunk_size%4)!=0)
		{
#ifdef FMV_ERROR
			I_Printf("  Chunk size not a multiple of 4?\n");
			chunk_size=chunk_size+(4-(chunk_size%4));
#endif
		}

#ifdef FMV_DEBUG
		I_Printf("  %.4s (size=%d offset=0x%lx)\n",chunk_type,chunk_size,offset);
		I_Printf("  {\n");
#endif

		if (strcasecmp("strh",chunk_type)==0)
		{
			long marker=f_tell(in);
			char buffer[5];
			read_chars(in,buffer,4);
			in->Seek(marker,SEEKPOINT_START);

			if (memcmp(buffer, "vids", 4)==0)
			{
				stream_type=0;
				ret = read_stream_header(in);
				if (ret < FMV_ERR_NONE)
					return ret;
			}
			else if (memcmp(buffer, "auds", 4)==0)
			{
				stream_type=1;
				ret = read_stream_header_auds(in);
				if (ret < FMV_ERR_NONE)
					return ret;
			}
			else if (memcmp(buffer, "txts", 4)==0)
			{
				stream_type=2;
				ret = read_stream_header_txts(in);
				if (ret < FMV_ERR_NONE)
					return ret;
			}
			else
			{
				I_Printf("  Unknown stream type %s\n", buffer);
				return FMV_ERR_UNKNOWN;
			}
		}
		else if (strcasecmp("strf",chunk_type)==0)
		{
			if (stream_type==0)
			{
				ret = read_stream_format(in);
				if (ret < FMV_ERR_NONE)
					return ret;
			}
			else if (stream_type==1)
			{
				ret = read_stream_format_auds(in);
				if (ret < FMV_ERR_NONE)
					return ret;
			}
		}
		else if (strcasecmp("strd",chunk_type)==0)
		{

		}
		else
		{
			I_Printf("  Unknown chunk type: %s\n",chunk_type);
		}

#ifdef FMV_DEBUG
		I_Printf("  }\n");
#endif
		// skip chunk (if needed)
		in->Seek(next_chunk,SEEKPOINT_START);
	}

#ifdef FMV_DEBUG
	I_Printf("@@@@ %ld %d\n", f_tell(in), end_of_chunk);
	I_Printf("  }\n");
#endif
	in->Seek(end_of_chunk,SEEKPOINT_START);

	return 0;
}

static int parse_hdrl(FILE *in, unsigned int size)
{
	char chunk_id[5];
	int chunk_size;
	int end_of_chunk;
	long offset=f_tell(in);
	int ret;

	read_chars(in,chunk_id,4);
	chunk_size=read_long(in);

#ifdef FMV_DEBUG
	I_Printf("  AVI Header Chunk (id=%s size=%d offset=0x%lx)\n",chunk_id,chunk_size,offset);
	I_Printf("  {\n");
#endif

	end_of_chunk=f_tell(in)+chunk_size;
	if ((end_of_chunk%4)!=0)
	{
		end_of_chunk=end_of_chunk+(4-(end_of_chunk%4));
	}

	ret = read_avi_header(in);
#ifdef FMV_DEBUG
	I_Printf("  }\n");
#endif
	if (ret < FMV_ERR_NONE)
		return ret;

	while(f_tell(in)<offset+size-4)
	{
#ifdef FMV_DEBUG
		I_Printf("  Should end at 0x%lx  0x%lx\n",offset+size,f_tell(in));
#endif
		ret = parse_hdrl_list(in);
	}

	return ret;
}

//
// parse next sub-chunk of movie data
//
int parse_movi(FILE *in, int *length, bool retv, GLuint *txr, bool reta, short *buf, int samples, float ascale)
{
	char chunk_id[5];
	int chunk_size;
	int end_of_subchunk;
	int ret = FMV_ERR_NONE;

	if (*length == -1)
	{
		F->Seek(movi_pos, SEEKPOINT_START);
		*length = movi_size;

		ctex = rtex = wtex = 0;
		pcm_cnt = pcm_rix = pcm_wix = 0;
		arix = 0.0f
		return FMV_ERR_NONE
	}

	// check if needed data in buffers
	if (retv)
		if (ctex)
		{
			ctex--;
			*txr = tex[rtex++];
			if (rtex == NUM_GL_TEXTURES)
				rtex = 0; // circular buffer
			ret = FMV_VID_UPDATE;
		}

	if (reta)
		if (pcm_cnt >= (int)((float)samples  * (float)astream_format.channels * ascale + 0.5f))
		{
			// fill audio chunk from circular buffer
			for (int i=0; i<samples; i++)
			{
				buf[i*2] = pcm_buffer[pcm_rix]; // left sample
				arix += ascale;
				if ((int)arix > pcm_rix)
				{
					pcm_rix++;
					pcm_cnt--;
					if (pcm_rix >= pcm_samples)
					{
						pcm_rix = 0; // circular buffer
						arix -= (float)pcm_samples;
					}
				}
				if (astream_format.channels == 2)
				{
					buf[i*2] = pcm_buffer[pcm_rix]; // right sample
					arix += ascale;
					if ((int)arix > pcm_rix)
					{
						pcm_rix++;
						pcm_cnt--;
						if (pcm_rix >= pcm_samples)
						{
							pcm_rix = 0; // circular buffer
							arix -= (float)pcm_samples;
						}
					}
				}
				else
				{
					// mono - right sample = left sample
					buf[i*2+1] = buf[i*2]; // right sample
				}
			}
			ret = ret == FMV_VID_UPDATE ? FMV_AV_UPDATE : FMV_AUD_UPDATE;
		}

	// if got decoded data, return
	if (ret > FMV_ERR_NONE)
		return ret;

//
//// Not enough decoded data in buffers, parse more movi chunks
//

	if (*length == 0)
		return FMV_ERR_ENDMOVI;

	read_chars(in,chunk_id,4);
	chunk_size=read_long(in);

#ifdef FMV_DEBUG
	I_Printf("  %s   0x%06X\n",chunk_id,chunk_size);
#endif

	if ((chunk_size%2)!=0)
		chunk_size++;
	end_of_subchunk=f_tell(in)+chunk_size;
	*length -= (chunk_size + 8);

	// parse chunk
	if (!memcmp(chunk_id, "00dc", 4))
	{
		// decode compressed video chunk into frame buffer
		if (!memcmp(stream_header.DataHandler, (int)0, 4))
			ret = decode_RoQ(in, chunk_size); // ROQ_SUCCESS = FMV_ERR_NONE
#if 0
		else if (!memcmp(stream_header.DataHandler, "CVID", 4))
			ret = decode_Cinepak(in, chunk_size);
		else if (!memcmp(stream_header.DataHandler, "XVID", 4))
			ret = decode_Xvid(in, chunk_size);
#endif
		else
			ret = FMV_ERR_UNKNOWN;

		if (ret >= 0)
		{
			// upload decoded video into circular opengl buffer
			glBindTexture(GL_TEXTURE_2D, tex[wtex]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                 texture_width, texture_height, 0, GL_RGB,
                 GL_UNSIGNED_SHORT_5_6_5, frame[ret]);
			ctex++;
            wtex++;
            if (wtex == NUM_GL_TEXTURES)
				wtex = 0; // circular buffer

			ret = FMV_ERR_NONE;
		}

		return ret;
	}
	else if (!memcmp(chunk_id, "01wb", 4))
	{
		// decode audio chunk into circular pcm buffer
		if (astream_format.format == 1)
			ret = decode_PCM(in, chunk_size);
		else if (astream_format.format == 17)
			ret = decode_ADPCM_IMA(in, chunk_size);
		else if (astream_format.format == 32)
			ret = decode_ADPCM_Yamaha(in, chunk_size);
#if 0
		else if (astream_format.format == 85)
			ret = decode_MP3(in, chunk_size);
#endif
		else
			ret = FMV_ERR_UNKNOWN;

		return ret;
	}

	return FMV_ERR_UNKNOWN;
}

int parse_avi(epi::file_c *in)
{
	char chunk_id[5];
	int chunk_size;
	char chunk_type[5];
	int end_of_chunk,end_of_subchunk;
	int offset = f_tell(in);
	int ret = FMV_ERR_CORRUPT;

	read_chars(in,chunk_id,4);
	chunk_size=read_long(in);
	read_chars(in,chunk_type,4);

#ifdef FMV_DEBUG
	I_Printf("RIFF Chunk (id=%s size=%d type=%s offset=0x%lx)\n",chunk_id,chunk_size,chunk_type, offset);
	I_Printf("{\n");
#endif

	if (strcasecmp("RIFF",chunk_id)!=0)
	{
		I_Printf(" Not a RIFF file.\n");
		return FMV_ERR_CORRUPT;
	}
    else if (strcasecmp("AVI ",chunk_type)!=0)
	{
		I_Printf(" Not an AVI file.\n");
		return FMV_ERR_CORRUPT;
	}

	end_of_chunk=f_tell(in)+chunk_size-4;

	while (f_tell(in)<end_of_chunk)
	{
		long offset=f_tell(in);
		read_chars(in,chunk_id,4);
		chunk_size=read_long(in);
		end_of_subchunk=f_tell(in)+chunk_size;

		if (strcasecmp("JUNK",chunk_id)==0 || strcasecmp("PAD ",chunk_id)==0)
		{
			chunk_type[0]=0;
		}
		else
		{
			read_chars(in,chunk_type,4);
		}

#ifdef FMV_DEBUG
		I_Printf(" New Chunk (id=%s size=%d type=%s offset=0x%lx)\n",chunk_id,chunk_size,chunk_type,offset);
		I_Printf(" {\n");
#endif

		if (strcasecmp("JUNK",chunk_id)==0 || strcasecmp("PAD ",chunk_id)==0)
		{
			if ((chunk_size%4)!=0)
			{
				chunk_size=chunk_size+(4-(chunk_size%4));
			}
			hex_dump_chunk(in, chunk_size);
		}
		else if (strcasecmp("INFO",chunk_type)==0)
		{
			if ((chunk_size%4)!=0)
			{
				chunk_size=chunk_size+(4-(chunk_size%4));
			}
			hex_dump_chunk(in, chunk_size);
		}
		else if (strcasecmp("hdrl",chunk_type)==0)
		{
			ret = parse_hdrl(in, chunk_size);
		}
		else if (strcasecmp("movi",chunk_type)==0)
		{
			movi_pos = f_tell(in);
			movi_size = chunk_size - 4;
		}
		else if (strcasecmp("idx1",chunk_id)==0)
		{
			idx1_pos = f_tell(in) - 4;
			idx1_size = chunk_size;
		}
		else
		{
#ifdef FMV_DEBUG
			I_Printf("  Unknown chunk at %d (%4s)\n",(int)f_tell(in)-8,chunk_type);
#endif
			if (chunk_size==0)
				break;
		}

		// skip subchunk (if needed)
		in->Seek(end_of_subchunk,SEEKPOINT_START);
#ifdef FMV_DEBUG
		I_Printf(" }\n");
#endif
	}

	if (stream_format.palette!=0)
	{
		free(stream_format.palette);
	}

#ifdef FMV_DEBUG
	I_Printf("}\n");
#endif

	if (ret < FMV_ERR_NONE)
		return ret;

	// setup for video decode
	if (avi_header.Width <= 256)
		texture_width = 256;
	else if (avi_header.Width <= 512)
		texture_width = 512;
	else if (avi_header.Width <= 1024)
		texture_width = 1024;

	if (avi_header.Height <= 256)
		texture_height = 256;
	else if (avi_header.Height <= 512)
		texture_height = 512;
	else if (avi_header.Height <= 1024)
		texture_height = 1024;

	// setup OpenGL circular texture buffer
	ctex = rtex = wtex = 0;
	glGenTextures(NUM_GL_TEXTURES, tex);

	scale_width = (float)avi_header.Width / (float)texture_width;
	scale_height = (float)avi_header.Height / (float)texture_height;

	// setup circular pcm buffer
	pcm_cnt = pcm_rix = pcm_wix = 0;
	int bufsmps = astream_header.SuggestedBufferSize;
	if (astream_format.block_size_of_data == 16)
		bufsmps >>= 1;
	else if (astream_format.block_size_of_data == 4)
		bufsmps <<= 1;
	pcm_samples = bufsmps * NUM_AUD_BUFFERS;
	pcm_buffer = new short[pcm_samples];

	// allocate frame buffers for decoding video
	frame[0] = new unsigned short[texture_width * texture_height];
	frame[1] = new unsigned short[texture_width * texture_height];

	// allocate read buffer for parsing
	read_buffer = new unsigned char[MAX_BUF_SIZE];

	return FMV_ERR_NONE;
}
