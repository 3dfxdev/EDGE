/* ------------------------------------------------------------------------
 * EDGE2 ROQ VECTOR AND MOTION MANAGER
 * 
 * (C) 2011 EDGE2 TEAM (Corbin, Dark_Photon, Team Xlink, ctd1500)
 * EDGE2 is based on 'EDGE2' by Andrew Apted and Andrew Baker.
 * 
 * This is a simple decoder for the Id Software RoQ video format.  In
 * this format, audio samples are DPCM coded and the video frames are
 * coded using motion blocks and vector quantisation.
 * Adapted for EDGE2 by Corbin!
 * Based on code written by Tim Ferguson. 
 * ------------------------------------------------------------------------ */

#include "roq.h"

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