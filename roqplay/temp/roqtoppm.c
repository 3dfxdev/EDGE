#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "roq.h"

/* -------------------------------------------------------------------------- */
typedef struct {
	FILE *fp;
	long data_size;
	unsigned int format, channels, block_align, sample_size;
	long sample_rate;
	long byte_per_sec;
} wav_out_info;

#define DFCC(a,b,c,d)   (a | b << 8 | c << 16 | d << 24)

/* -------------------------------------------------------------------------- */
static void wav_put_long(FILE *fp, unsigned long v)
{
	fputc(v & 0xff, fp);
	fputc((v & 0xff00) >> 8, fp);
	fputc((v & 0xff0000) >> 16, fp);
	fputc((v & 0xff000000) >> 24, fp);
}


/* -------------------------------------------------------------------------- */
static void wav_put_word(FILE *fp, unsigned int v)
{
	fputc(v & 0xff, fp);
	fputc((v & 0xff00) >> 8, fp);
}


/* -------------------------------------------------------------------------- */
static void wav_put_header(FILE *fp, wav_out_info *wi)
{
	wav_put_long(fp, DFCC('R','I','F','F'));
	wav_put_long(fp, wi->data_size + 36);
	wav_put_long(fp, DFCC('W','A','V','E'));

	wav_put_long(fp, DFCC('f','m','t',' '));
	wav_put_long(fp, 16);
	wav_put_word(fp, wi->format);
	wav_put_word(fp, wi->channels);
	wav_put_long(fp, wi->sample_rate);
	wav_put_long(fp, wi->byte_per_sec);
	wav_put_word(fp, wi->block_align);
	wav_put_word(fp, wi->sample_size);

	wav_put_long(fp, DFCC('d','a','t','a'));
	wav_put_long(fp, wi->data_size);
}


/* -------------------------------------------------------------------------- */
wav_out_info *wav_create(char *fname, int format, int channels, long sample_rate, int sample_size)
{
wav_out_info *wi;

	if((wi = calloc(sizeof(wav_out_info), 1)) == NULL) return NULL;
	if((wi->fp = fopen(fname, "wb")) == NULL)
		{
		printf("Error creating wav file `%s'\n", fname);
		free(wi);
		return NULL;
		}

	wi->data_size = 0;
	wi->format = format;
	wi->channels = channels;
	wi->sample_size = sample_size;
	wi->sample_rate = sample_rate;
	wi->byte_per_sec = channels * sample_size/8 * sample_rate;
	wi->block_align = (sample_size * channels)/8;

	wav_put_header(wi->fp, wi);
	return wi;
}


/* -------------------------------------------------------------------------- */
void wav_write_audio(wav_out_info *wi, unsigned char *buf, int size)
{
	wi->data_size += size;
	fwrite(buf, size, 1, wi->fp);
}


/* -------------------------------------------------------------------------- */
void wav_close(wav_out_info *wi)
{
	if(wi == NULL) return;
	if(wi->fp)
		{
		fseek(wi->fp, 0, SEEK_SET);
		wav_put_header(wi->fp, wi);
		fclose(wi->fp);
		}
	free(wi);
}



/* --------------------------------------------------------------------------
 * if (raw), writes as RAW bytes, otherwise writes as ASCII 
 * if ptype==4, (Full Color YUV420->RGB) a PPM file is written
 * if ptype==3, (Full Color RGB) a PPM file is written
 * if ptype==2, (Greyscale)  a PGM file is written
 * if ptype==1, (B/W stipple) a PBM file is written
 *
 */
int write_pnm(char *fname, unsigned char *pa, unsigned char *pb,
	unsigned char *pc, int ptype, int width, int height, int raw)
{
int magic;
int i, j, y1, y2, u, v, len;
FILE *fp;

	if((fp = fopen(fname, "wb")) == NULL)
		{
		printf("Error: PNM cannot open `%s' for saving.\n", fname);
		return(0);
		}

		/* calc the appropriate magic number for this file type */
	magic = (ptype > 3 ? 3 : ptype);
	if(raw && magic) magic += 3;

		/* write the header info */
	fprintf(fp,"P%d\n", magic);
	fprintf(fp,"# Created by pnm save.  T. C. F. 2002.\n");

	fprintf(fp, "%d %d\n",width, height);
	if(ptype != 1) fprintf(fp,"255\n");

	if (ferror(fp)) return(0);

		/* write the image data */

	if(ptype == 4)			/* 24bit RGB, 3 bytes per pixel */
		{
#define limit(x) ((((x) > 0xffffff) ? 0xff0000 : (((x) <= 0xffff) ? 0 : (x) & 0xff0000)) >> 16)
		for(j = 0; j < height; j++)
			{
			for(i = 0; i < width/2; i++)
				{
				int r, g, b;
				y1 = *(pa++); y2 = *(pa++);
				u = pb[i] - 128;
				v = pc[i] - 128;

				y1 *= 65536; y2 *= 65536;
				r = 91881 * v;
				g = -22554 * u + -46802 * v;
				b = 116130 * u;

				putc(limit(r + y1), fp);
				putc(limit(g + y1), fp);
				putc(limit(b + y1), fp);
				putc(limit(r + y2), fp);
				putc(limit(g + y2), fp);
				putc(limit(b + y2), fp);
				}
			if(j & 0x01) { pb += width/2; pc += width/2; }
			}
		}
	else if(ptype == 3)			/* 24bit RGB, 3 bytes per pixel */
		{
		for(i = 0, len = 0; i < height; i++)
			{
			for(j = 0; j < width; j++)
				{
				if(raw)
					{
					putc(*(pa++), fp);
					putc(*(pb++), fp);
					putc(*(pc++), fp);
					}
				else
					{
					fprintf(fp,"%3d %3d %3d ", (*pa++), (*pb++), (*pc++));
					len+=12;
					if(len > 58)
						{
						fprintf(fp,"\n");
						len=0;
						}
					}
				}
			}
		}

	else if(ptype == 2)		/* 8-bit greyscale */
		{
		for(i = 0, len = 0; i < width * height; i++)
			{
			if(raw) putc(*(pa++), fp);
			else
				{
				fprintf(fp, "%3d ", *(pa++));
				len += 4;
				if(len > 66)
					{
					fprintf(fp,"\n");
					len=0;
					}
				}
			}
		}
	
	else if(ptype == 1)		/* 1-bit B/W stipple */
		{
		}

	if(ferror(fp)) return(0);
	fclose(fp);
	return(1);
}


/* -------------------------------------------------------------------------- */
int main(int argc, char **argv)
{
roq_info *ri;
wav_out_info *wi;
char fname[80];

	if(argc < 3)
		{
		printf("roq <input roq> <output base>\n");
		return -1;
		}

	if((ri = roq_open(argv[1])) == NULL) return -1;

	printf("RoQ: length %ld:%02ld  video: %dx%d %ld frames  audio: %d channels.\n",
		ri->stream_length/60000, ((ri->stream_length + 500)/1000) % 60,
		ri->width, ri->height, ri->num_frames, ri->audio_channels);

	printf("Writing frames to %s####.ppm\n", argv[2]);
	while(roq_read_frame(ri) > 0)
		{
		sprintf(fname, "%s%04d.ppm", argv[2], ri->frame_num);
		write_pnm(fname, ri->y[0], ri->u[0], ri->v[0], 4, ri->width, ri->height, 1);
		}

	if(ri->audio_channels > 0)
		{
		sprintf(fname, "%s.wav", argv[2]);
		printf("Writing audio to %s\n", fname);
		if((wi = wav_create(fname, 1, ri->audio_channels, 22050, 16)) != NULL)
			{
			while(roq_read_audio(ri) > 0)
				wav_write_audio(wi, ri->audio, ri->audio_size);
			wav_close(wi);
			}
		}

	roq_close(ri);
	return 0;
}

