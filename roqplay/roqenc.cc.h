/* ------------------------------------------------------------------------
 * Id Software's RoQ video file format decoder
 *
 * Dr. Tim Ferguson, 2001.
 * For more details on the algorithm:
 *         http://www.csse.monash.edu.au/~timf/videocodec.html
 *
 * This is a simple decoder for the Id Software RoQ video format.  In
 * this format, audio samples are DPCM coded and the video frames are
 * coded using motion blocks and vector quantisation.
 *
 * Note: All information on the RoQ file format has been obtained through
 *   pure reverse engineering.  This was achieved by giving known input
 *   audio and video frames to the roq.exe encoder and analysing the
 *   resulting output text and RoQ file.  No decompiling of the Quake III
 *   Arena game was required.
 *
 * You may freely use this source code.  I only ask that you reference its
 * source in your projects documentation:
 *       Tim Ferguson: http://www.csse.monash.edu.au/~timf/
 * ------------------------------------------------------------------------ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "roq.h"

#define DBUG	0

/* -------------------------------------------------------------------------- */
static unsigned int get_word(FILE *fp)
{
unsigned int ret;

	ret  = ((fgetc(fp)) & 0xff);
	ret |= ((fgetc(fp)) & 0xff) << 8;
	return(ret);
}


/* -------------------------------------------------------------------------- */
static unsigned long get_long(FILE *fp)
{
unsigned long ret;

	ret  = ((fgetc(fp)) & 0xff);
	ret |= ((fgetc(fp)) & 0xff) << 8;
	ret |= ((fgetc(fp)) & 0xff) << 16;
	ret |= ((fgetc(fp)) & 0xff) << 24;
	return(ret);
}


/* -------------------------------------------------------------------------- */
static int roq_parse_file(FILE *fp, roq_info *ri)
{
unsigned int head1, head3, chunk_id, chunk_arg;
long head2, chunk_size;
long fpos;
int max_frame;

	ri->num_audio_bytes = ri->num_frames = max_frame = 0;
	ri->audio_channels = 0;
	ri->frame_offset = NULL;
	ri->buf_size = 0;
	head1 = get_word(fp);
	head2 = get_long(fp);
	head3 = get_word(fp);
	if(head1 != 0x1084 && head2 != 0xffffffff && head3 != 0x1e)
		{
		printf("Not an RoQ file.\n");
		return 1;
		}

	ri->roq_start = ftell(fp);
	while(!feof(fp))
		{
#if DBUG > 20
		printf("---------------------------------------------------------------------------\n");
#endif
		fpos = ftell(fp);
		chunk_id = get_word(fp);
		chunk_size = get_long(fp);
		chunk_arg = get_word(fp);
		if(chunk_size > ri->buf_size) ri->buf_size = chunk_size;
		if(feof(fp)) break;
#if DBUG > 20
		printf("%03d  0x%06lx: chunk: 0x%02x size: %ld  cells: 2x2=%d,4x4=%d\n", i,
			fpos, chunk_id, chunk_size, v1>>8,v1&0xff);
#endif

		if(chunk_id == RoQ_INFO)		/* video info */
			{
			ri->width = get_word(fp);
			ri->height = get_word(fp);
			get_word(fp);
			get_word(fp);
			}
		else
			{
			if(chunk_id == RoQ_QUAD_VQ)
				{
				ri->num_frames++;
				if(ri->num_frames > max_frame)
					{
					max_frame += 5000;
					if((ri->frame_offset = realloc(ri->frame_offset, sizeof(long) * max_frame)) == NULL) return 1;
					}
				ri->frame_offset[ri->num_frames] = fpos;
				}
			if(chunk_id == RoQ_SOUND_MONO || chunk_id == RoQ_SOUND_STEREO)
				{
				if(chunk_id == RoQ_SOUND_MONO) ri->audio_channels = 1;
				else ri->audio_channels = 2;
				ri->num_audio_bytes += chunk_size;
				}
			fseek(fp, chunk_size, SEEK_CUR);
			}
		}

	return 0;
}


/* -------------------------------------------------------------------------- */
static void apply_vector_2x2(roq_info *ri, int x, int y, roq_cell *cell)
{
unsigned char *yptr;

	yptr = ri->y[0] + (y * ri->width) + x;
	*yptr++ = cell->y0;
	*yptr++ = cell->y1;
	yptr += (ri->width - 2);
	*yptr++ = cell->y2;
	*yptr++ = cell->y3;
	ri->u[0][(y/2) * (ri->width/2) + x/2] = cell->u;
	ri->v[0][(y/2) * (ri->width/2) + x/2] = cell->v;
}


/* -------------------------------------------------------------------------- */
static void apply_vector_4x4(roq_info *ri, int x, int y, roq_cell *cell)
{
unsigned long row_inc, c_row_inc;
register unsigned char y0, y1, u, v;
unsigned char *yptr, *uptr, *vptr;

	yptr = ri->y[0] + (y * ri->width) + x;
	uptr = ri->u[0] + (y/2) * (ri->width/2) + x/2;
	vptr = ri->v[0] + (y/2) * (ri->width/2) + x/2;

	row_inc = ri->width - 4;
	c_row_inc = (ri->width/2) - 2;
	*yptr++ = y0 = cell->y0; *uptr++ = u = cell->u; *vptr++ = v = cell->v;
	*yptr++ = y0;
	*yptr++ = y1 = cell->y1; *uptr++ = u; *vptr++ = v;
	*yptr++ = y1;

	yptr += row_inc;

	*yptr++ = y0;
	*yptr++ = y0;
	*yptr++ = y1;
	*yptr++ = y1;

	yptr += row_inc; uptr += c_row_inc; vptr += c_row_inc;

	*yptr++ = y0 = cell->y2; *uptr++ = u; *vptr++ = v;
	*yptr++ = y0;
	*yptr++ = y1 = cell->y3; *uptr++ = u; *vptr++ = v;
	*yptr++ = y1;

	yptr += row_inc;

	*yptr++ = y0;
	*yptr++ = y0;
	*yptr++ = y1;
	*yptr++ = y1;
}


/* -------------------------------------------------------------------------- */
static void apply_motion_4x4(roq_info *ri, int x, int y, unsigned char mv, char mean_x, char mean_y)
{
int i, mx, my;
unsigned char *pa, *pb;

	mx = x + 8 - (mv >> 4) - mean_x;
	my = y + 8 - (mv & 0xf) - mean_y;

	pa = ri->y[0] + (y * ri->width) + x;
	pb = ri->y[1] + (my * ri->width) + mx;
	for(i = 0; i < 4; i++)
		{
		pa[0] = pb[0];
		pa[1] = pb[1];
		pa[2] = pb[2];
		pa[3] = pb[3];
		pa += ri->width;
		pb += ri->width;
		}

	pa = ri->u[0] + (y/2) * (ri->width/2) + x/2;
	pb = ri->u[1] + (my/2) * (ri->width/2) + (mx + 1)/2;
	for(i = 0; i < 2; i++)
		{
		pa[0] = pb[0];
		pa[1] = pb[1];
		pa += ri->width/2;
		pb += ri->width/2;
		}

	pa = ri->v[0] + (y/2) * (ri->width/2) + x/2;
	pb = ri->v[1] + (my/2) * (ri->width/2) + (mx + 1)/2;
	for(i = 0; i < 2; i++)
		{
		pa[0] = pb[0];
		pa[1] = pb[1];
		pa += ri->width/2;
		pb += ri->width/2;
		}
}


/* -------------------------------------------------------------------------- */
static void apply_motion_8x8(roq_info *ri, int x, int y, unsigned char mv, char mean_x, char mean_y)
{
int mx, my, i;
unsigned char *pa, *pb;

	mx = x + 8 - (mv >> 4) - mean_x;
	my = y + 8 - (mv & 0xf) - mean_y;

	pa = ri->y[0] + (y * ri->width) + x;
	pb = ri->y[1] + (my * ri->width) + mx;
	for(i = 0; i < 8; i++)
		{
		pa[0] = pb[0];
		pa[1] = pb[1];
		pa[2] = pb[2];
		pa[3] = pb[3];
		pa[4] = pb[4];
		pa[5] = pb[5];
		pa[6] = pb[6];
		pa[7] = pb[7];
		pa += ri->width;
		pb += ri->width;
		}

	pa = ri->u[0] + (y/2) * (ri->width/2) + x/2;
	pb = ri->u[1] + (my/2) * (ri->width/2) + (mx + 1)/2;
	for(i = 0; i < 4; i++)
		{
		pa[0] = pb[0];
		pa[1] = pb[1];
		pa[2] = pb[2];
		pa[3] = pb[3];
		pa += ri->width/2;
		pb += ri->width/2;
		}

	pa = ri->v[0] + (y/2) * (ri->width/2) + x/2;
	pb = ri->v[1] + (my/2) * (ri->width/2) + (mx + 1)/2;
	for(i = 0; i < 4; i++)
		{
		pa[0] = pb[0];
		pa[1] = pb[1];
		pa[2] = pb[2];
		pa[3] = pb[3];
		pa += ri->width/2;
		pb += ri->width/2;
		}
}


/* -------------------------------------------------------------------------- */
roq_info *roq_open(char *fname)
{
FILE *fp;
roq_info *ri;
int i;

	if((fp = fopen(fname, "rb")) == NULL)
		{
		printf("Error opening %s\n", fname);
		return NULL;
		}

	if((ri = malloc(sizeof(roq_info))) == NULL)
		{
		printf("Error allocating memory.\n");
		return NULL;
		}

	ri->fp = fp;
	if(roq_parse_file(fp, ri)) return NULL;

	ri->stream_length = (ri->num_frames * 1000)/30;

	for(i = 0; i < 128; i++)
		{
		ri->snd_sqr_arr[i] = i * i;
		ri->snd_sqr_arr[i + 128] = -(i * i);
		}

	for(i = 0; i < 2; i++)
		{
		if((ri->y[i] = calloc(ri->width * ri->height, 1)) == NULL ||
			(ri->u[i] = calloc((ri->width * ri->height)/4, 1)) == NULL ||
			(ri->v[i] = calloc((ri->width * ri->height)/4, 1)) == NULL)
			{
			printf("Memory allocation error.\n");
			return NULL;
			}
		}

	ri->buf_size *= 2;
	if((ri->buf = calloc(ri->buf_size, 1)) == NULL)
		{
		printf("Memory allocation error.\n");
		return NULL;
		}
	ri->audio_buf_size = 0;
	ri->audio = NULL;

	ri->frame_num = 0;
	ri->aud_pos = ri->vid_pos = ri->roq_start;

	return ri;
}


/* -------------------------------------------------------------------------- */
void roq_close(roq_info *ri)
{
int i;

	if(ri == NULL) return;
	fclose(ri->fp);
	for(i = 0; i < 2; i++)
		{
		if(ri->y[i] != NULL) free(ri->y[i]);
		if(ri->u[i] != NULL) free(ri->u[i]);
		if(ri->v[i] != NULL) free(ri->v[i]);
		}
	if(ri->buf != NULL) free(ri->buf);
	free(ri);
}


/* -------------------------------------------------------------------------- */
int roq_read_frame(roq_info *ri)
{
FILE *fp = ri->fp;
unsigned int chunk_id = 0, chunk_arg = 0;
unsigned long chunk_size = 0;
int i, j, k, nv1, nv2, vqflg = 0, vqflg_pos = -1, vqid, bpos, xpos, ypos, xp, yp, x, y;
unsigned char *tp, *buf;
int frame_stats[2][4] = {{0},{0}};
roq_qcell *qcell;

	fseek(fp, ri->vid_pos, SEEK_SET);

	while(!feof(fp))
		{
		chunk_id = get_word(fp);
		chunk_size = get_long(fp);
		chunk_arg = get_word(fp);
		if(feof(fp)) break;
		if(chunk_id == RoQ_QUAD_VQ) break;
		if(chunk_id == RoQ_QUAD_CODEBOOK)
			{
			if((nv1 = chunk_arg >> 8) == 0) nv1 = 256;
			if((nv2 = chunk_arg & 0xff) == 0 && nv1 * 6 < chunk_size) nv2 = 256;
			for(i = 0; i < nv1; i++)
				{
				ri->cells[i].y0 = fgetc(fp);
				ri->cells[i].y1 = fgetc(fp);
				ri->cells[i].y2 = fgetc(fp);
				ri->cells[i].y3 = fgetc(fp);
				ri->cells[i].u = fgetc(fp);
				ri->cells[i].v = fgetc(fp);
				}
			for(i = 0; i < nv2; i++)
				for(j = 0; j < 4; j++) ri->qcells[i].idx[j] = fgetc(fp);
			}
		else fseek(fp, chunk_size, SEEK_CUR);
		}

	if(chunk_id != RoQ_QUAD_VQ)
		{
		ri->vid_pos = ftell(fp);
		return 0;
		}

	ri->frame_num++;
	if(ri->buf_size < chunk_size)
		{
		ri->buf_size *= 2;
		free(ri->buf);
		if((ri->buf = calloc(ri->buf_size, 1)) == NULL)
			{
			printf("Memory allocation error.\n");
			return -1;
			}
		}
	fread(ri->buf, chunk_size, 1, fp);
	buf = ri->buf;

	bpos = xpos = ypos = 0;
	while(bpos < chunk_size)
		{
		for(yp = ypos; yp < ypos + 16; yp += 8)
			for(xp = xpos; xp < xpos + 16; xp += 8)
				{
				if(vqflg_pos < 0)
					{
					vqflg = buf[bpos++]; vqflg |= (buf[bpos++] << 8);
					vqflg_pos = 7;
					}
				vqid = (vqflg >> (vqflg_pos * 2)) & 0x3;
				frame_stats[0][vqid]++;
				vqflg_pos--;

				switch(vqid)
					{
					case RoQ_ID_MOT: break;
					case RoQ_ID_FCC:
						apply_motion_8x8(ri, xp, yp, buf[bpos++], chunk_arg >> 8, chunk_arg & 0xff);
						break;
					case RoQ_ID_SLD:
						qcell = ri->qcells + buf[bpos++];
						apply_vector_4x4(ri, xp, yp, ri->cells + qcell->idx[0]);
						apply_vector_4x4(ri, xp+4, yp, ri->cells + qcell->idx[1]);
						apply_vector_4x4(ri, xp, yp+4, ri->cells + qcell->idx[2]);
						apply_vector_4x4(ri, xp+4, yp+4, ri->cells + qcell->idx[3]);
						break;
					case RoQ_ID_CCC:
						for(k = 0; k < 4; k++)
							{
							x = xp; y = yp;
							if(k & 0x01) x += 4;
							if(k & 0x02) y += 4;

							if(vqflg_pos < 0)
								{
								vqflg = buf[bpos++]; vqflg |= (buf[bpos++] << 8);
								vqflg_pos = 7;
								}
							vqid = (vqflg >> (vqflg_pos * 2)) & 0x3;
							frame_stats[1][vqid]++;
							vqflg_pos--;
							switch(vqid)
								{
								case RoQ_ID_MOT: break;
								case RoQ_ID_FCC:
									apply_motion_4x4(ri, x, y, buf[bpos++], chunk_arg >> 8, chunk_arg & 0xff);
									break;
								case RoQ_ID_SLD:
									qcell = ri->qcells + buf[bpos++];
									apply_vector_2x2(ri, x, y, ri->cells + qcell->idx[0]);
									apply_vector_2x2(ri, x+2, y, ri->cells + qcell->idx[1]);
									apply_vector_2x2(ri, x, y+2, ri->cells + qcell->idx[2]);
									apply_vector_2x2(ri, x+2, y+2, ri->cells + qcell->idx[3]);
									break;
								case RoQ_ID_CCC:
									apply_vector_2x2(ri, x, y, ri->cells + buf[bpos]);
									apply_vector_2x2(ri, x+2, y, ri->cells + buf[bpos+1]);
									apply_vector_2x2(ri, x, y+2, ri->cells + buf[bpos+2]);
									apply_vector_2x2(ri, x+2, y+2, ri->cells + buf[bpos+3]);
									bpos += 4;
									break;
								}
							}
						break;
					default:
						printf("Unknown vq code: %d\n", vqid);
					}
				}

		xpos += 16;
		if(xpos >= ri->width)
			{
			xpos -= ri->width;
			ypos += 16;
			}
		if(ypos >= ri->height) break;
		}

#if 0
	frame_stats[0][3] = 0;
	printf("<%d  0x%04x -> %d,%d>\n", ri->frame_num, chunk_arg, (char)(chunk_arg >> 8), (char)(chunk_arg & 0xff));
	printf("for 08x08 CCC = %d, FCC = %d, MOT = %d, SLD = %d, PAT = 0\n", frame_stats[0][3], frame_stats[0][1], frame_stats[0][0], frame_stats[0][2]);
	printf("for 04x04 CCC = %d, FCC = %d, MOT = %d, SLD = %d, PAT = 0\n", frame_stats[1][3], frame_stats[1][1], frame_stats[1][0], frame_stats[1][2]);
#endif

	ri->vid_pos = ftell(fp);

	if(ri->frame_num == 1)
		{
		memcpy(ri->y[1], ri->y[0], ri->width * ri->height);
		memcpy(ri->u[1], ri->u[0], (ri->width * ri->height)/4);
		memcpy(ri->v[1], ri->v[0], (ri->width * ri->height)/4);
		}
	else
		{
		tp = ri->y[0]; ri->y[0] = ri->y[1]; ri->y[1] = tp;
		tp = ri->u[0]; ri->u[0] = ri->u[1]; ri->u[1] = tp;
		tp = ri->v[0]; ri->v[0] = ri->v[1]; ri->v[1] = tp;
		}

	return 1;
}


/* -------------------------------------------------------------------------- */
int roq_read_audio(roq_info *ri)
{
FILE *fp = ri->fp;
unsigned int chunk_id = 0, chunk_arg = 0;
unsigned long chunk_size = 0;
int i, snd_left, snd_right;

	fseek(fp, ri->aud_pos, SEEK_SET);
	ri->audio_size = 0;

	while(!feof(fp))
		{
		chunk_id = get_word(fp);
		chunk_size = get_long(fp);
		chunk_arg = get_word(fp);
		if(feof(fp) || chunk_id == RoQ_SOUND_MONO || chunk_id == RoQ_SOUND_STEREO)
			break;
		fseek(fp, chunk_size, SEEK_CUR);
		}

	if(ri->audio_buf_size < chunk_size * 2)
		{
		if(ri->audio != NULL) free(ri->audio);
		ri->audio_buf_size = chunk_size * 3;
		if((ri->audio = calloc(ri->audio_buf_size, 1)) == NULL) return -1;
		}

	if(chunk_id == RoQ_SOUND_MONO)
		{
		ri->audio_size = chunk_size * 2;
		snd_left = chunk_arg;
		for(i = 0; i < chunk_size; i++)
			{
			snd_left += ri->snd_sqr_arr[fgetc(fp)];
			ri->audio[i * 2] = snd_left & 0xff;
			ri->audio[i * 2 + 1] = (snd_left & 0xff00) >> 8;
			}
		ri->aud_pos = ftell(fp);
		return 1;
		}

	if(chunk_id == RoQ_SOUND_STEREO)
		{
		ri->audio_size = chunk_size * 2;
		snd_left = (chunk_arg & 0xFF00);
		snd_right = (chunk_arg & 0xFF) << 8;
		for(i = 0; i < chunk_size; i += 2)
			{
			snd_left += ri->snd_sqr_arr[fgetc(fp)];
			snd_right += ri->snd_sqr_arr[fgetc(fp)];
			ri->audio[i * 2] = snd_left & 0xff;
			ri->audio[i * 2 + 1] = (snd_left & 0xff00) >> 8;
			ri->audio[i * 2 + 2] = snd_right & 0xff;
			ri->audio[i * 2 + 3] = (snd_right & 0xff00) >> 8;
			}
		ri->aud_pos = ftell(fp);
		return 1;
		}

	ri->aud_pos = ftell(fp);
	return 0;
}


