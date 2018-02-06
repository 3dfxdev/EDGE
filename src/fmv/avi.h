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

#ifndef AVI_H
#define AVI_H


#define NUM_GL_TEXTURES 3
#define NUM_AUD_BUFFERS 4


typedef struct
{
	s32_t TimeBetweenFrames;
	s32_t MaximumDataRate;
	s32_t PaddingGranularity;
	s32_t Flags;
	s32_t TotalNumberOfFrames;
	s32_t NumberOfInitialFrames;
	s32_t NumberOfStreams;
	s32_t SuggestedBufferSize;
	s32_t Width;
	s32_t Height;
	s32_t TimeScale;
	s32_t DataRate;
	s32_t StartTime;
	s32_t DataLength;
}
avi_header_t;

typedef struct 
{
	char DataType[5];
	char DataHandler[5];
	int Flags;
	int Priority;
	int InitialFrames;
	int TimeScale;
	int DataRate;
	int StartTime;
	int DataLength;
	int SuggestedBufferSize;
	int Quality;
	int SampleSize;
}
stream_header_t;

typedef struct 
{
	int header_size;
	int image_width;
	int image_height;
	int number_of_planes;
	int bits_per_pixel;
	int compression_type;
	int image_size_in_bytes;
	int x_pels_per_meter;
	int y_pels_per_meter;
	int colors_used;
	int colors_important;
	int *palette;
}
stream_format_t;

struct stream_format_auds_t
{
	int header_size;
	int format;
	int channels;
	int samples_per_second;
	int bytes_per_second;
	int block_size_of_data;
	int bits_per_sample;
	int extended_size;
};

typedef struct 
{
	char ckid[5];
	int dwFlags;
	int dwChunkOffset;
	int dwChunkLength;
}
index_entry_t;

int parse_movi(epi::file_c *in, int *length, bool retv, GLuint *tex, bool reta, short *buf, int samples, float ascale);
int parse_idx1(epi::file_c *in, int frame);
int parse_avi(epi::file_c *in);

#endif
