//----------------------------------------------------------------------------
//  EDGE View Bitmap Initialisation code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2001  The EDGE Team.
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
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------
//
// Here are all the composition mode dependent initialisation routines and
// callbacks for viewbitmaps and its substructs (ie views and cameras).
//

#include "i_defs.h"
#include "r_vbinit.h"

#include "con_defs.h"
#include "dm_state.h"
#include "r_main.h"
#include "r_segs.h"
#include "r_state.h"
#include "r_view.h"
#include "v_colour.h"
#include "v_res.h"
#include "z_zone.h"

byte **dv_ylookup;
int *dv_columnofs;
int dv_viewwidth;
int dv_viewheight;

funclist_t enlarge8_2_2_funcs;
funclist_t enlarge16_2_2_funcs;

static void R_EnlargeView8_2_2_CVersion(void)
{
  int x, y;
  int w, h;
  byte *dest1, *dest2, *src;

  w = dv_viewwidth;
  h = dv_viewheight;

  for (y = 0; y < h; y++)
  {
    dest1 = dv_ylookup[y * 2 - h] + dv_columnofs[-w];
    dest1 += 2 * w;
    dest2 = dest1 + vb_pitch;
    src = dv_ylookup[y] + dv_columnofs[0];
    src += w;
    for (x = -w; x; x++)
    {
      dest1[2 * x + 0] = dest1[2 * x + 1] = dest2[2 * x + 0] = dest2[2 * x + 1] = src[x];
    }
  }
}

static void R_EnlargeView16_2_2_CVersion(void)
{
  int x, y;
  int w, h;
  unsigned long *dest1, *dest2;
  unsigned short *src;
  unsigned long pixel;

  w = dv_viewwidth;
  h = dv_viewheight;

  for (y = 0; y < h; y++)
  {
    dest1 = (unsigned long *)(dv_ylookup[y * 2 - h] + dv_columnofs[-w]);
    dest1 += w;
    dest2 = (unsigned long *)((char *)dest1 + vb_pitch);
    src = (unsigned short *)(dv_ylookup[y] + dv_columnofs[0]);
    src += w;
    for (x = -w; x; x++)
    {
      pixel = src[x];
      pixel |= pixel << 16;
      dest1[x] = dest2[x] = pixel;
    }
  }
}

//
// Anti-aliased scaling. The 'quick blur' setting.
//
static void R_EnlargeView8_2_2_Blur(void)
{
  int x, y;
  int w, h;
  byte *dest, *src;
  unsigned long c, c1;

  w = dv_viewwidth;
  h = dv_viewheight;

  for (y = 0; y < h; y++)
  {
    dest = dv_ylookup[y * 2 - h] + dv_columnofs[-w];
    src = dv_ylookup[y] + dv_columnofs[0];
    for (x = 0; x < w; x++)
    {
      c1 = col2rgb8[32][src[0]];
      c = (c1 + c1) | 0x07c1fc1f;
      dest[0] = rgb_32k[0][0][c & (c >> 17)];

      c = (c1 + col2rgb8[32][src[1]]) | 0x07c1fc1f;
      dest[1] = rgb_32k[0][0][c & (c >> 17)];

      c = (c1 + col2rgb8[32][src[vb_pitch]]) | 0x07c1fc1f;
      dest[vb_pitch] = rgb_32k[0][0][c & (c >> 17)];

      c = (col2rgb8[16][src[0]] + col2rgb8[16][src[1]] +
          col2rgb8[16][src[vb_pitch]] + col2rgb8[16][src[vb_pitch + 1]]) | 0x07c1fc1f;

      dest[vb_pitch + 1] = rgb_32k[0][0][c & (c >> 17)];

      dest += 2;
      src++;
    }
    dest[-1] = dest[-2];
    dest[vb_pitch - 1] = dest[vb_pitch - 2];
  }
  dest = dv_ylookup[h - 1] + dv_columnofs[-w];
  Z_MoveData(dest, dest - vb_pitch, byte, vb_pitch);
}

void (*R_DoEnlargeView_2_2) (void);

static void R_EnlargeView_2_2(void *data)
{
  view_t *v = (view_t*)data;

  dv_viewwidth = v->screen.width;
  dv_viewheight = v->screen.height;
  dv_ylookup = v->ylookup;
  dv_columnofs = v->columnofs;

  R_DoEnlargeView_2_2();
}

static void BlurViewStrong8(void *data)
{
// DRAW_PIXEL: Draws one pixel
#define DRAW_PIXEL(x1,x2,x3) \
  (c = (line1[x1] + 2 * line1[x2] + line1[x3] + \
        2 * line2[x1] + 4 * line2[x2] + 2 * line2[x3] + \
        line3[x1] + 2 * line3[x2] + line3[x3]) | 0x07c1fc1f, \
  dest[x2] = rgb_32k[0][0][c & (c >> 17)])

  view_t *v = (view_t*)data;
  unsigned int i;
  unsigned int x, y;
  unsigned int w, h;
  byte *dest, *src;
  unsigned long c;
  unsigned long *tmp;
  unsigned long *col2rgb;

  // We store the RGB values of three lines in memory.
  // This way, we never need to look up each pixel more than once,
  // and the special cases for top/bottom lines is quite easy to handle.
  // the line above the current one
  static unsigned long *line1 = NULL;

  // the current line
  static unsigned long *line2 = NULL;

  // the line below the current one
  static unsigned long *line3 = NULL;

  // width of the screen
  static unsigned long linew = 0;

  col2rgb = col2rgb8[4];

  w = v->screen.width;
  h = v->screen.height;

  if (w > linew)
  {
    // WHY THE FUCK ARE WE USING REALLOC??? - WE HAVE A ZONE ALLOC SYSTEM!!
    line1 = (unsigned long*)realloc(line1, w * sizeof(unsigned long));
    line2 = (unsigned long*)realloc(line2, w * sizeof(unsigned long));
    line3 = (unsigned long*)realloc(line3, w * sizeof(unsigned long));

    linew = w;
  }

  src = v->ylookup[0] + v->columnofs[0];

  for (i = 0; i < linew; i++)
  {
    line2[i] = line1[i] = col2rgb[src[i]];
  }

  for (y = 0; y < h; y++)
  {
    src = v->ylookup[y > h - 2 ? h - 1 : y + 1] + columnofs[0];

    for (i = 0; i < linew; i++)
    {
      line3[i] = col2rgb[src[i]];
    }

    dest = v->ylookup[y] + v->columnofs[0];

    // first draw the leftmost pixel
    DRAW_PIXEL(0, 0, 1);

    // now draw all the other pixels
    for (x = 1; x < w - 1; x++)
    {
      DRAW_PIXEL(x - 1, x, x + 1);
    }

    // finally, draw the rightmost pixel
    DRAW_PIXEL(w - 2, w - 1, w - 1);

    tmp = line1;
    line1 = line2;
    line2 = line3;
    line3 = tmp;
  }
#undef DRAW_PIXEL
}

static void BlurViewLight8(void *data)
{
// DRAW_PIXEL: Draws one pixel
#define DRAW_PIXEL(x1,x2,x3) \
  (c = (line1[x2] + line2[x1] + 4*line2[x2] + line2[x3] + line3[x2]) | 0x07c1fc1f, \
  dest[x2] = rgb_32k[0][0][c & (c >> 17)])

  view_t *v = (view_t*)data;
  unsigned int i;
  unsigned int x, y;
  unsigned int w, h;
  byte *dest, *src;
  unsigned long c;
  unsigned long *tmp;
  unsigned long *col2rgb;

  // We store the RGB values of three lines in memory.
  // This way, we never need to look up each pixel more than once,
  // and the special cases for top/bottom lines is quite easy to handle.
  // the line above the current one
  static unsigned long *line1 = NULL;

  // the current line
  static unsigned long *line2 = NULL;

  // the line below the current one
  static unsigned long *line3 = NULL;

  // width of the screen
  static unsigned long linew = 0;

  col2rgb = col2rgb8[8];

  w = v->screen.width;
  h = v->screen.height;

  if (w > linew)
  {
    // WHY THE FUCK ARE WE USING REALLOC????
    line1 = (unsigned long*)realloc(line1, w * sizeof(unsigned long));
    line2 = (unsigned long*)realloc(line2, w * sizeof(unsigned long));
    line3 = (unsigned long*)realloc(line3, w * sizeof(unsigned long));

    linew = w;
  }

  src = v->ylookup[0] + v->columnofs[0];

  for (i = 0; i < linew; i++)
  {
    line2[i] = line1[i] = col2rgb[src[i]];
  }

  for (y = 0; y < h; y++)
  {
    src = v->ylookup[y > h - 2 ? h - 1 : y + 1] + columnofs[0];

    for (i = 0; i < linew; i++)
    {
      line3[i] = col2rgb[src[i]];
    }

    dest = v->ylookup[y] + v->columnofs[0];

    // first draw the leftmost pixel
    DRAW_PIXEL(0, 0, 1);

    // now draw all the other pixels
    for (x = 1; x < w - 1; x++)
    {
      DRAW_PIXEL(x - 1, x, x + 1);
    }

    // finally, draw the rightmost pixel
    DRAW_PIXEL(w - 2, w - 1, w - 1);

    tmp = line1;
    line1 = line2;
    line2 = line3;
    line3 = tmp;
  }
#undef DRAW_PIXEL
}

typedef struct
{
  int curdet;
  int mindet;
  int maxdet;
  unsigned long time0,time1;
  unsigned long tic0,tic1;
  unsigned long tic;
}
auto_vbdata_t;

//
// Checks what we should swith the detail level to.
//
// We aim to allow an error of about 10%
#define ALLOWED_ERROR (0.1)
// this is the minimal time we can allow between two time measurements
// to keep the measurement error within ALLOWED_ERROR.
#define MIN_TIME_INTERVAL (1000000/ALLOWED_ERROR/2/microtimer_granularity)
// returns the time interval between time1 and time2
#define TIME_INTERVAL(time1,time2) ((time2) - (time1) + 0.5 / microtimer_granularity)
static void CheckDetailChange(void *data)
{
  auto_vbdata_t *d = (auto_vbdata_t*)data;
  flo_t fps;
  unsigned long curtime;
  int detail;

  d->tic++;

  curtime = I_ReadMicroSeconds();
  L_WriteDebug("\ntime %ld/%ld", curtime, d->time0);

  // first-time stuff
  if (d->time0 == 0)
    d->time0 = curtime;

  // Always try to keep two timers going simultaneously, to get more frequent
  // FPS updates on systems with poor timers.
  if (curtime > d->time0 + MIN_TIME_INTERVAL / 2 && d->time0 >= d->time1)
  {
    d->time1 = curtime;
    d->tic1 = d->tic;
  }

  // stop now if not enough time has elapsed to get a decent frame rate value.
  if (curtime <= d->time0 + MIN_TIME_INTERVAL)
    return;

  fps = (d->tic - d->tic0) * 1000000.0 / TIME_INTERVAL(d->time0, curtime);
  d->time0 = d->time1;
  d->tic0 = d->tic1;

  detail = d->curdet;
  if (fps > 35.0)
  {
    detail++;
  }
  if (fps < 30.0)
  {
    detail--;
  }

  // clip to max/min values
  if (detail > d->maxdet)
    detail = d->maxdet;
  if (detail < d->mindet)
    detail = d->mindet;
  d->curdet = detail;

  L_WriteDebug("\nFPS %f, detail %d\n", fps, detail);
}

typedef struct
{
  view_t *v;
  int lowx;
  int lowy;
  int loww;
  int lowh;
  int hix;
  int hiy;
  int hiw;
  int hih;
  int lowdet;
  int hidet;
  int xscale;
  int yscale;
  int last_det;
  int *detail;
}
auto_viewdata_t;

//
// Callback that is used for all the views that resize in auto detail
//
static void AutoDetailViewFunc(void *data)
{
  auto_viewdata_t *a = (auto_viewdata_t*) data;
  int x, y, w, h;
  int deltadet;
  int detail;

  detail = *(a->detail);
  if (detail < a->lowdet)
    detail = a->lowdet;
  if (detail > a->hidet)
    detail = a->hidet;
  if (detail == a->last_det && detail != -1)
    return;

  a->last_det = detail;

  detail = detail - a->lowdet;

  deltadet = a->hidet - a->lowdet;
  x = a->lowx + detail * (a->hix - a->lowx) / deltadet;
  y = a->lowy + detail * (a->hiy - a->lowy) / deltadet;
  w = a->loww + detail * (a->hiw - a->loww) / deltadet;
  h = a->lowh + detail * (a->hih - a->lowh) / deltadet;

  if (x < 0)
    x = 0;
  if (y < 0)
    y = 0;
  if (x > a->v->aspect->maxwidth - w)
    x = a->v->aspect->maxwidth - w;
  if (y > a->v->aspect->maxheight - h)
    y = a->v->aspect->maxheight - h;

  R_ViewSetXPosition(a->v, a->xscale * x + w * (a->xscale - 1), x, w);
  R_ViewSetYPosition(a->v, a->yscale * y + h * (a->yscale - 1), y, h);
}

//
// MakeViewAutoDetail
//
// Makes a view use the automatic detail level feature.
// When the detail level is <= lowdet, the view will have the low* dimensions,
// when it's >= hidet the view will have the hi* dimensions. When detail level
// is between lowdet and hidet, the view dimensions will be somewhere between
// of low* and hi*, depending on how close it is to lowdet/hidet.
//
// *** All the dimensions refer to aspect position ***
// xscale and yscale tell how many times bigger the view will get on the screen.
// detail is the value returned by make_vb_auto_detail.
//
static void MakeViewAutoDetail(view_t * v, int lowx, int lowy, int loww, int lowh,
    int hix, int hiy, int hiw, int hih, int lowdet, int hidet,
    int xscale, int yscale, int *detail)
{
  auto_viewdata_t *a;

  a = Z_ClearNew(auto_viewdata_t, 1);

  a->v = v;
  a->lowx = lowx;
  a->lowy = lowy;
  a->loww = loww;
  a->lowh = lowh;
  a->hix = hix;
  a->hiy = hiy;
  a->hiw = hiw;
  a->hih = hih;
  a->lowdet = lowdet;
  a->hidet = hidet;
  a->xscale = xscale;
  a->yscale = yscale;
  a->last_det = -1;  // current detail level. We don't need to resize view if detail hasn't changed since last frame.

  a->detail = detail;

  R_AddStartCallback(&v->frame_start, AutoDetailViewFunc, a, Z_Free);
}

//
// MakeVBAutoDetail
//
// Makes a viewbitmap use the automatic detail level feature.
// This must be called *before* any of its affected views is created, and
// the returned pointer should be passed as the 'detail' parameter to all the
// childrens' make_view_auto_detail calls.
// mindet and maxdet show the minimal and maximal detail levels that will
// be used by this viewbitmap.
static int *MakeVBAutoDetail(viewbitmap_t * vb, int mindet, int maxdet)
{
  auto_vbdata_t *a;

  a = Z_ClearNew(auto_vbdata_t, 1);

  a->mindet = mindet;
  a->maxdet = maxdet;
  // current detail level: start right between min and max
  a->curdet = (mindet + maxdet) / 2;

  R_AddStartCallback(&vb->frame_start, CheckDetailChange, a, Z_Free);

  return &a->curdet;
}

//
// init_vb_*
// Helper functions that compose the vb in different ways.
// All of them assume that vb already is created and that it has
// one aspect and one view: those that normally are used for psprite.
// Also, viewwidth and viewheight are set to viewwindowwidth and
// viewwindowheight.
// If you want to replace any of those, you can destroy them and create new
// ones.

//
// the normal one, with only one view, at full detail
//
static void InitVB_Classic(viewbitmap_t * vb)
{
  aspect_t *a = vb->aspects;

  R_CreateView(vb, a, 0, 0, camera, VRF_VIEW, 0);
}

//
// looks like the normal one, but is split up in two views. This can increase
// the frame rate very much in some situations, especially when you're close
// to a wall and a high resolution is chosen.
static void InitVB_Classic_2y(viewbitmap_t * vb)
{
  aspect_t *a = vb->aspects;
  view_t *v;

  v = R_CreateView(vb, a, 0, 0, camera, VRF_VIEW, 0);
  R_ViewSetYPosition(v, 0, 0, viewheight / 2);
  v = R_CreateView(vb, a, 0, 0, camera, VRF_VIEW, 0);
  R_ViewSetYPosition(v, viewheight / 2, viewheight / 2, viewheight / 2);
}

// The old low detail mode. The pixels are doubled y- and x-wise.
static void InitVB_LowDetail(viewbitmap_t * vb)
{
  aspect_t *a;
  view_t *v;

  a = R_CreateAspect( vb, x_distunit / 2, y_distunit / 2,
                      ((focusxfrac-0.5)/2), topslope, bottomslope,
                      viewwidth / 2, viewheight / 2);

  v = R_CreateView(vb, a, 0, 0, camera, VRF_VIEW, 0);
  R_ViewSetYPosition(v, viewheight / 2, 0, viewheight / 2);
  R_ViewSetXPosition(v, viewwidth / 2, 0, viewwidth / 2);
  R_AddEndCallback(&v->frame_end, R_EnlargeView_2_2, v, NULL);
}

// low detail at the edges of the viewbitmap.
static void InitVB_LDE(viewbitmap_t * vb)
{
  aspect_t *a1 = vb->aspects;
  aspect_t *a2;
  view_t *v;

  a2 = R_CreateAspect(vb, x_distunit / 2, y_distunit / 2,
      ((focusxfrac - 0.5)/2),
      topslope, bottomslope,
      viewwidth / 2, viewheight / 2);

  // left view
  v = R_CreateView(vb, a2, 0, 0, camera, VRF_VIEW, 0);
  R_ViewSetXPosition(v, viewwidth / 8, 0, viewwidth / 8);
  R_ViewSetYPosition(v, viewheight / 2, 0, viewheight / 2);
  R_AddEndCallback(&v->frame_end, R_EnlargeView_2_2, v, NULL);

  // right view
  v = R_CreateView(vb, a2, 0, 0, camera, VRF_VIEW, 0);
  R_ViewSetXPosition(v, 7 * viewwidth / 8, 3 * viewwidth / 8, viewwidth / 8);
  R_ViewSetYPosition(v, viewheight / 2, 0, viewheight / 2);
  R_AddEndCallback(&v->frame_end, R_EnlargeView_2_2, v, NULL);

  // middle view
  v = R_CreateView(vb, a1, 0, 0, camera, VRF_VIEW, 0);
  R_ViewSetXPosition(v, viewwidth / 4, viewwidth / 4, viewwidth / 2);
  R_ViewSetYPosition(v, 0, 0, viewheight * 3 / 4);

  // bottom view
  v = R_CreateView(vb, a2, 0, 0, camera, VRF_VIEW, 1);
  R_ViewSetXPosition(v, viewwidth / 2, viewwidth / 8, viewwidth / 4);
  R_ViewSetYPosition(v, viewheight * 7 / 8, viewheight * 3 / 8, viewheight / 8);
  R_AddEndCallback(&v->frame_end, R_EnlargeView_2_2, v, NULL);
}

//
// Automatic detail level. Dynamically changes the detail level, so that
// the frame rate always is 30-35 FPS.
//
static void InitVB_AutoDetail(viewbitmap_t * vb) // naming conventions?
{
#if 0
  InitVB_LDE(vb);
#else
  aspect_t *a1 = vb->aspects;
  aspect_t *a2;
  view_t *v;
  int *detail;

  a2 = R_CreateAspect(vb, x_distunit / 2, y_distunit / 2,
      (focusxfrac - 0.5) / 2,
      topslope, bottomslope,
      viewwidth / 2, viewheight / 2);

/*
   Viewbitmap will look like this (approximately) in different detail levels:

   Lowest:  Low:     Medium:  High:    Highest:
   LLLLRRRR LLLMMRRR LLMMMMRR LMMMMMMR MMMMMMMM
   LLLLRRRR LLLMMRRR LLMMMMRR LMMMMMMR MMMMMMMM
   LLLLRRRR LLLMMRRR LLMMMMRR LMMMMMMR MMMMMMMM
   LLLLRRRR LLLMMRRR LLMMMMRR LMMMMMMR MMMMMMMM
   LLLLRRRR LLLMMRRR LLMMMMRR LMMMMMMR MMMMMMMM
   LLLLRRRR LLLBBRRR LLMMMMRR LMMMMMMR MMMMMMMM
   LLLLRRRR LLLBBRRR LLBBBBRR LMMMMMMR MMMMMMMM
   LLLLRRRR LLLBBRRR LLBBBBRR LBBBBBBR MMMMMMMM

   where L (left), R (right) and B (bottom) are low detail views, and M (middle)
   is a high detail view.
 */

  detail = MakeVBAutoDetail(vb, 0, viewwidth / 8);

  // left view (low detail)
  v = R_CreateView(vb, a2, 0, 0, camera, VRF_VIEW, 0);
  MakeViewAutoDetail(v, 0, 0, viewwidth / 4, viewheight / 2, 0, 0, 0, viewheight / 2, 0, viewwidth / 8, 2, 2, detail);
  R_AddEndCallback(&v->frame_end, R_EnlargeView_2_2, v, NULL);

  // right view (low detail)
  v = R_CreateView(vb, a2, 0, 0, camera, VRF_VIEW, 2);
  MakeViewAutoDetail(v, viewwidth / 2, 0, viewwidth / 4, viewheight / 2, viewwidth / 2, 0, 0, viewheight / 2, 0, viewwidth / 8, 2, 2, detail);
  R_AddEndCallback(&v->frame_end, R_EnlargeView_2_2, v, NULL);

  // bottom view (low detail)
  v = R_CreateView(vb, a2, 0, 0, camera, VRF_VIEW, 3);
  MakeViewAutoDetail(v, viewwidth / 4, viewheight / 2, 0, viewheight / 4, 0, viewheight / 2, viewwidth / 2, 0, 0, viewwidth / 8, 2, 2, detail);
  R_AddEndCallback(&v->frame_end, R_EnlargeView_2_2, v, NULL);

  // middle view (high detail)
  v = R_CreateView(vb, a1, 0, 0, camera, VRF_VIEW, 1);
  MakeViewAutoDetail(v, viewwidth / 2, 0, 0, viewheight / 2, 0, 0, viewwidth, viewheight, 0, viewwidth / 8, 1, 1, detail);
#endif
}

//
// Use light blur filter.
//
static void InitVB_BlurLight(viewbitmap_t * vb)
{
  aspect_t *a = vb->aspects;
  view_t *v;

  v = R_CreateView(vb, a, 0, 0, camera, VRF_VIEW, 0);

  if (BPP == 1)
    R_AddEndCallback(&v->frame_end, BlurViewLight8, v, NULL);
}

//
// Use strong blur filter.
//
static void InitVB_BlurStrong(viewbitmap_t * vb)
{
  aspect_t *a = vb->aspects;
  view_t *v;

  v = R_CreateView(vb, a, 0, 0, camera, VRF_VIEW, 0);

  if (BPP == 1)
    R_AddEndCallback(&v->frame_end, BlurViewStrong8, v, NULL);
}

//
// Multiplies FOV by nviews by creating nviews views.
//
static void InitVB_NViews(viewbitmap_t * vb, int nviews)
{
  int w;
  int i;
  aspect_t *a;
  view_t *v;
  static camera_t *cameras[20];

  w = (viewwidth + nviews - 1) / nviews;
  a = R_CreateAspect(vb,
      x_distunit * w / viewwidth,
      y_distunit * w / viewwidth,
      focusxfrac * w / viewwidth,
      topslope * viewwidth / w,
      bottomslope * viewwidth / w,
      w, viewheight);

  for (i = 0; i < nviews; i++)
  {
    if (cameras[i])
      R_DestroyCamera(cameras[i]);

    cameras[i] = R_CreateCamera();
    
    R_InitCamera_ViewOffs(cameras[i], (flo_t)(viewanglebaseoffset+(((nviews - 1 - i * 2) * FIELDOFVIEW / 2) << ANGLETOFINESHIFT)) );
    
    v = R_CreateView(vb, a, 0, 0, cameras[i], VRF_VIEW, 0);
    
    R_ViewSetXPosition(v, viewwidth * i / nviews, 0,
        viewwidth * (i + 1) / nviews - viewwidth * i / nviews);
  }
}

static void InitVB_3Views(viewbitmap_t * vb)
{
  InitVB_NViews(vb, 3);
}

static void InitVB_10Views(viewbitmap_t * vb)
{
  InitVB_NViews(vb, 10);
}

// the screen drawing mode. Index in the screencomplist.
int screencomposition = 0;
vbinit_t screencomplist[] =
{
  {"Classic", InitVB_Classic, "The original screen setting"},
  {"DoubleY", InitVB_Classic_2y, "Looks like the original, but might be faster"},
  {"LowDetail", InitVB_LowDetail, "Low detail mode"},
  {"LightBlur", InitVB_BlurLight, "Use Light Blur Filter"},
  {"StrongBlur", InitVB_BlurStrong, "Use Strong Blur Filter"},
  {"LDE", InitVB_LDE, "Low Detail at the Edges of the screen"},
  {"AutoDetail", InitVB_AutoDetail, "Automatic Detail Level"},
  {"3XFOV", InitVB_3Views, "Gives you three times as big FOV"},
  {"10XFOV", InitVB_10Views, "Gives you ten times as big FOV"},
  {NULL,NULL,NULL}
};


/***** Camera routines *****/
typedef struct
{
  camera_t *c;
  mobj_t *mo;
}
camera_start_stdobj_t;

static void CameraFrameInit_StdObject(void *data)
{
  camera_start_stdobj_t *o = (camera_start_stdobj_t*) data;
  camera_t *c;
  mobj_t *mo;

  mo = o->mo;
  c = o->c;

  c->view_obj = mo;

  viewx = mo->x;
  viewy = mo->y;
  viewangle = mo->angle;
  viewvertangle = mo->vertangle;
  viewsubsector = mo->subsector;

  if (mo->player)
  {
    // -AJA- NOTE: CameraFrameInit_StdPlayer is used instead !!
    
    viewz = mo->player->viewz;

    extralight = mo->player->extralight;
    effect_colourmap = mo->player->effect_colourmap;
    effect_strength = mo->player->effect_strength;
    effect_infrared = mo->player->effect_infrared;
  }
  else
  {
    viewz = mo->z + mo->height * 9 / 10;

    extralight = 0;
    effect_colourmap = NULL;
    effect_strength = 0;
    effect_infrared = false;
  }

  view_props = R_PointGetProps(viewsubsector, viewz);
}

void R_InitCamera_StdObject(camera_t * c, mobj_t * mo)
{
  camera_start_stdobj_t *o;

  o = Z_New(camera_start_stdobj_t, 1);
  o->c = c;
  o->mo = mo;
  R_AddStartCallback(&c->frame_start, CameraFrameInit_StdObject, o, Z_Free);
}

static void CameraFrameInit_StdPlayer(void *data)
{
  camera_t *c = (camera_t*)data;
  player_t *player;

  player = displayplayer;
  c->view_obj = player->mo;

  viewx = player->mo->x;
  viewy = player->mo->y;
  viewangle = player->mo->angle;
  viewz = player->viewz;
  extralight = player->extralight;

  viewsubsector = player->mo->subsector;
  viewvertangle = player->mo->vertangle + player->kick_offset;
  view_props = R_PointGetProps(viewsubsector, viewz);

  effect_colourmap = player->effect_colourmap;
  effect_strength = player->effect_strength;
  effect_infrared = player->effect_infrared;
}
  

void R_InitCamera_StdPlayer(camera_t * c)
{
  R_AddStartCallback(&c->frame_start, CameraFrameInit_StdPlayer, c, NULL);
}

typedef struct
{
  angle_t offs;
}
camera_start_viewoffs_t;

static void CameraFrameInit_ViewOffs(void *data)
{
  camera_start_viewoffs_t *o = (camera_start_viewoffs_t*)data;

  viewangle += o->offs;
}

void R_InitCamera_ViewOffs(camera_t * c, angle_t offs)
{
  camera_start_viewoffs_t *data;

  R_InitCamera_StdPlayer(c);

  data = Z_New(camera_start_viewoffs_t, 1);
  data->offs = offs;

  R_AddStartCallback(&c->frame_start, CameraFrameInit_ViewOffs, data, Z_Free);
}

flo_t camera_3d_offset = 4.0;

static void CameraFrameInit_3D_Left(void *data)
{
  viewx += M_Sin(viewangle) * camera_3d_offset;
  viewy -= M_Cos(viewangle) * camera_3d_offset;
}

static void CameraFrameInit_3D_Right(void *data)
{
  viewx -= M_Sin(viewangle) * camera_3d_offset;
  viewy += M_Cos(viewangle) * camera_3d_offset;
}

void R_InitCamera_3D_Left(camera_t * c)
{
  CON_CreateCVarReal("3doffset", cf_normal, &camera_3d_offset);
  // use std as a base
  R_InitCamera_StdPlayer(c);
  // add callback that moves view to the left
  R_AddStartCallback(&c->frame_start, CameraFrameInit_3D_Left, NULL, NULL);
}
void R_InitCamera_3D_Right(camera_t * c)
{
  CON_CreateCVarReal("3doffset", cf_normal, &camera_3d_offset);
  // use std as a base
  R_InitCamera_StdPlayer(c);
  // add callback that moves view to the right
  R_AddStartCallback(&c->frame_start, CameraFrameInit_3D_Right, NULL, NULL);
}

typedef struct
{
  viewbitmap_t *right;
  viewbitmap_t *left;
}
vb_3d_right_t;

static void FinishVB_3D_Left(void *data)
{
  viewbitmap_t *vb = (viewbitmap_t*)data;

  R_RenderViewBitmap(vb);
}

// LUTs for conversion to red/cyan format
static unsigned short cyan_3d[2][256];
static unsigned short red_3d[2][256];

// -ES- 2000/01/24 Changed 3d glasses effect to 16 bit. It is much cleaner
// that way.
static void FinishVB_3D_Right(void *data)
{
  vb_3d_right_t *vb = (vb_3d_right_t*)data;
  int x;
  int y;
  viewbitmap_t *left;
  viewbitmap_t *right;
  unsigned char *dest;
  unsigned char *src;

  left = vb->left;
  right = vb->right;
  for (y = 0; y < right->screen.height; y++)
  {
    src = right->screen.data + right->screen.pitch * y;
    dest = left->screen.data + left->screen.pitch * y;
    for (x = 0; x < right->screen.width; x ++)
    {
      *(short *)dest = cyan_3d[0][dest[0]] + cyan_3d[1][dest[1]] +
          red_3d[0][src[0]] + red_3d[1][src[1]];
      dest += 2;
      src += 2;
    }
  }
}

void R_InitVB_3D_Left(viewbitmap_t * r, viewbitmap_t * l)
{
  R_AddEndCallback(&l->frame_end, FinishVB_3D_Left, r, NULL);
}

void R_InitVB_3D_Right(viewbitmap_t * r, viewbitmap_t * l)
{
  vb_3d_right_t *data;
  int i;
  truecol_info_t ti;

  data = Z_New(vb_3d_right_t, 1);
  data->left = l;
  data->right = r;
  R_AddEndCallback(&r->frame_end, FinishVB_3D_Right, data, Z_Free);
  // -ES- These are the weights for red/green/blue
  // in the colour conversion.
  // Theoretically, the weights should be 30*30, 59*59 and 11*11, but
  // that made all blue objects too dark. Using 1:1:1 looks much better IMO
#define RW 1
#define GW 1
#define BW 1
  I_GetTruecolInfo(&ti);
  for (i = 0; i < 256; i++)
  {
    int r, g, b, c;
    int n;

    c = i;
    for (n = 0; n < 2; n++)
    {
      r = ((c & ti.red_mask) >> ti.red_shift) << (6 - ti.red_bits);
      g = ((c & ti.green_mask) >> ti.green_shift) << (6 - ti.green_bits);
      b = ((c & ti.blue_mask) >> ti.blue_shift) << (6 - ti.blue_bits);
      c = (g * GW + b * BW + r * RW) / (RW + GW + BW);
      r = (c >> (6 - ti.red_bits)) << ti.red_shift;
      g = (c >> (6 - ti.green_bits)) << ti.green_shift;
      b = (c >> (6 - ti.blue_bits)) << ti.blue_shift;
      cyan_3d[n][i] = g | b;
      red_3d[n][i] = r;
      c = i << 8;
    }
  }
#undef RW
#undef GW
#undef BW
}

//
// R_InitVBFunctions
//
// Initialises Function Lists
void R_InitVBFunctions(void)
{
  CON_InitFunctionList(&enlarge8_2_2_funcs, "Enlarge8", R_EnlargeView8_2_2_CVersion, NULL);
  CON_AddFunctionToList(&enlarge8_2_2_funcs, "blur", "Interpolate between pixels", R_EnlargeView8_2_2_Blur, NULL);
#ifndef NOHICOLOUR
  CON_InitFunctionList(&enlarge16_2_2_funcs, "Enlarge16", R_EnlargeView16_2_2_CVersion, NULL);
#endif
}

