//----------------------------------------------------------------------------
//  EDGE Intermission Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
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
// -KM- 1998/12/16 Nuked half of this for DDF. DOOM 1 works now!
//

#include "i_defs.h"
#include "wi_stuff.h"

#include "dm_state.h"
#include "g_game.h"
#include "hu_lib.h"
#include "m_random.h"
#include "m_menu.h"
#include "m_swap.h"
#include "p_local.h"
#include "r_local.h"
#include "r_view.h"
#include "s_sound.h"
#include "v_ctx.h"
#include "v_colour.h"
#include "v_res.h"
#include "w_image.h"
#include "w_wad.h"
#include "z_zone.h"

//
// Data needed to add patches to full screen intermission pics.
// Patches are statistics messages, and animations.
// Loads of by-pixel layout and placement, offsets etc.
//

// GLOBAL LOCATIONS
#define WI_TITLEY   6
#define WI_SPACINGY 33

// SINGPLE-PLAYER STUFF
#define SP_STATSX 50
#define SP_STATSY 50

#define SP_TIMEX 16
#define SP_TIMEY (200-32)

//
// GENERAL DATA
//

//
// Locally used stuff.
//

// States for single-player
#define SP_KILLS  0
#define SP_ITEMS  2
#define SP_SECRET 4
#define SP_FRAGS  6
#define SP_TIME   8
#define SP_PAR    ST_TIME

#define SP_PAUSE 1

// in seconds
#define SHOWNEXTLOCDELAY 4
#define SHOWLASTLOCDELAY SHOWNEXTLOCDELAY

// used to accelerate or skip a stage
static bool acceleratestage;

// wbs->pnum
static int me;

 // specifies current state
static stateenum_t state;

// contains information passed into intermission
static wbstartstruct_t *wbs;

static wbplayerstruct_t *plrs;  // wbs->plyr[]

// used for general timing
static int cnt;

// used for timing of background animation
static int bcnt;

// signals to refresh everything for one frame
static int firstrefresh;

static int cnt_kills[10];
static int cnt_items[10];
static int cnt_secret[10];
static int cnt_time;
static int cnt_par;
static int cnt_pause;

// GRAPHICS

// background
static const image_t *bg_image;

// You Are Here graphic
static const image_t *yah[2] = {NULL, NULL};

// splat
static const image_t *splat[2] = {NULL, NULL};

// %, : graphics
static const image_t *percent;
static const image_t *colon;

// 0-9 graphic
static const image_t *digits[10];

// minus sign
static const image_t *wiminus;

// "Finished!" graphics
static const image_t *finished;

// "Entering" graphic
static const image_t *entering;

// "secret"
static const image_t *sp_secret;

 // "Kills", "Scrt", "Items", "Frags"
static const image_t *kills;
static const image_t *secret;
static const image_t *items;
static const image_t *frags;

// Time sucks.
static const image_t *time_image; // -ACB- 1999/09/19 Removed Conflict with <time.h>
static const image_t *par;
static const image_t *sucks;

// "killers", "victims"
static const image_t *killers;
static const image_t *victims;

// "Total", your face, your dead face
static const image_t *total;
static const image_t *star;
static const image_t *bstar;

// Name graphics of each level (centered)
static const image_t *lnames[2];

// -KM- 1998/12/17 Needs access from savegame code
wi_map_t worldmap;

//
// CODE
//

// Called everytime the map changes.
// -KM- 1998/12/17 Beefed up:  if worldmap.name == NULL we haven't
//   inited yet.  if map == NULL reload the intermission anyway
//   (we are starting a new game.)
void WI_MapInit(const wi_map_t * map)
{
  if (!map)
    worldmap.ddf.name = NULL;
  else
  {
    if (!worldmap.ddf.name || strcmp(map->ddf.name, worldmap.ddf.name))
    {
      worldmap = *map;
      Z_Clear(worldmap.mapdone, bool, worldmap.nummaps);
    }
  }
}

// Draws "<Levelname> Finished!"
static void DrawLevelFinished(void)
{
  int y = WI_TITLEY;
  int w, w2, h;

  // draw <LevelName> 
  DEV_ASSERT2(lnames[0]);

  w = IM_WIDTH(lnames[0]);
  h = IM_HEIGHT(lnames[0]);

  w2 = IM_WIDTH(finished);

  VCTX_ImageEasy320(160 - w/2,  y, lnames[0]);
  VCTX_ImageEasy320(160 - w2/2, y + h * 5/4, finished);
}

// Draws "Entering <LevelName>"
static void DrawEnteringLevel(void)
{
  int y = WI_TITLEY;
  int w, w2, h;

  // -KM- 1998/11/25 If there is no level to enter, don't draw it.
  //      (Stop Map30 from crashing)
  if (! lnames[1])
    return;

  h = IM_HEIGHT(entering);
  w = IM_WIDTH(entering);
  w2 = IM_WIDTH(lnames[1]);

  VCTX_ImageEasy320(160 - w/2,  y, entering);
  VCTX_ImageEasy320(160 - w2/2, y + h * 5/4, lnames[1]);
}

static void DrawOnLnode(int n, const image_t * images[2])
{
  int i;
  int left, top, right, bottom;

  bool fits = false;
  mappos_t *mappos = &worldmap.mappos[n];

  for (i=0; i < 2; i++)
  {
    // -AJA- something fishy going on here. The following is just a
    //       band-aid solution.
    if (images[i] == NULL)
    {
      i++;
      continue;
    }

    left = mappos->pos.x - images[i]->offset_x;
    top  = mappos->pos.y - images[i]->offset_y;
    right  = left + IM_WIDTH(images[i]);
    bottom = top + IM_HEIGHT(images[i]);

    if (left >= 0 && right < 320 && top >= 0 && bottom < 200)
    {
      fits = true;
      break;
    }
    else
    {
      i++;
    }
  }

  if (fits && i < 2)
  {
    VCTX_ImageEasy320(mappos->pos.x, mappos->pos.y, images[i]);
  }
  else
  {
    L_WriteDebug("Could not place patch on level '%s'\n", mappos[n].name);
  }
}

//
// Draws a number.
//
// If numdigits > 0, then use that many digits minimum,
//  otherwise only use as many as necessary.
// Returns new x position.
//
static int DrawNum(int x, int y, int n, int numdigits)
{
  int neg;
  int temp;

  neg = n < 0;

  if (neg)
    n = -n;

  // if non-number, do not draw it
  if (n == 1994)
    return 0;

  if (numdigits < 0)
  {
    if (!n)
    {
      // make variable-length zeros 1 digit long
      numdigits = 1;
    }
    else
    {
      // figure out # of digits in #
      numdigits = 0;
      temp = n;

      while (temp)
      {
        temp /= 10;
        numdigits++;
      }
    }
  }

  // draw the new number
  for (; numdigits > 0; n /= 10, numdigits--)
  {
    x -= IM_WIDTH(digits[0]);
    VCTX_ImageEasy320(x, y, digits[n % 10]);
  }

  // draw a minus sign if necessary
  if (neg)
  {
    x -= IM_WIDTH(wiminus);
    VCTX_ImageEasy320(x, y, wiminus);
  }

  return x;
}

static void DrawPercent(int x, int y, int p)
{
  if (p < 0)
    return;

  VCTX_ImageEasy320(x, y, percent);

  DrawNum(x, y, p, -1);
}

//
// Display level completion time and par,
//  or "sucks" message if overflow.
//
static void DrawTime(int x, int y, int t)
{
  int div;
  int n;

  if (t < 0)
    return;

  if (t <= 61 * 59)
  {
    div = 1;

    do
    {
      n = (t / div) % 60;
      x = DrawNum(x, y, n, 2) - IM_WIDTH(colon);
      div *= 60;

      // draw
      if (div == 60 || t / div)
        VCTX_ImageEasy320(x, y, colon);
    }
    while (t / div);
  }
  else
  {
    // "sucks"
    VCTX_ImageEasy320(x - IM_WIDTH(sucks), y, sucks);
  }
}

static void UnloadData(void)
{
#if 0  // OLD STUFF
  int i, j;

  W_DoneWithLump(bg);

  for (i = 0; i < 2; i++)
  {
    if (lnames[i])
      W_DoneWithLump(lnames[i]);
    if (yah[i])
      W_DoneWithLump(yah[i]);
    if (splat[i])
      W_DoneWithLump(splat[i]);
    lnames[i] = NULL;
    yah[i] = NULL;
    splat[i] = NULL;
  }
  W_DoneWithLump(wiminus);
  W_DoneWithLump(percent);
  W_DoneWithLump(finished);
  W_DoneWithLump(entering);
  W_DoneWithLump(kills);
  W_DoneWithLump(secret);
  W_DoneWithLump(sp_secret);
  W_DoneWithLump(items);
  W_DoneWithLump(frags);
  W_DoneWithLump(colon);
  W_DoneWithLump(time_image);
  W_DoneWithLump(sucks);
  W_DoneWithLump(par);
  W_DoneWithLump(killers);
  W_DoneWithLump(victims);
  W_DoneWithLump(total);
  W_DoneWithLump(star);
  W_DoneWithLump(bstar);

  for (i = 0; i < 10; i++)
    W_DoneWithLump(digits[i]);

  for (i = 0; i < worldmap.numanims; i++)
    for (j = 0; j < worldmap.anims[i].numframes; j++)
      W_DoneWithLump(worldmap.anims[i].frames[j].p);
#endif
}

static void WI_End(void)
{
  UnloadData();

  background_camera_mo = NULL;
  R_ExecuteSetViewSize();
}

static void InitNoState(void)
{
  state = NoState;
  acceleratestage = false;
  cnt = 10;
}

static void UpdateNoState(void)
{
  if (!--cnt)
  {
    WI_End();
    G_WorldDone();
  }

}

static bool snl_pointeron = false;

static void InitShowNextLoc(void)
{
  int i;

  state = ShowNextLoc;
  acceleratestage = false;
  cnt = SHOWNEXTLOCDELAY * TICRATE;

  for (i = 0; i < worldmap.nummaps; i++)
    if (!strcmp(worldmap.mappos[i].name, wbs->last->ddf.name))
      worldmap.mapdone[i] = true;
}

static void UpdateShowNextLoc(void)
{
  if (!--cnt || acceleratestage)
    InitNoState();
  else
    snl_pointeron = (cnt & 31) < 20;
}

static void DrawShowNextLoc(void)
{
  int i;

  for (i = 0; i < worldmap.nummaps; i++)
  {
    if (worldmap.mapdone[i])
      DrawOnLnode(i, splat);

    if (wbs->next)
      if (snl_pointeron && !strcmp(wbs->next->ddf.name, worldmap.mappos[i].name))
        DrawOnLnode(i, yah);
  }

  DrawEnteringLevel();
}

static void DrawNoState(void)
{
  snl_pointeron = true;
  DrawShowNextLoc();
}

static int dm_state;
static int dm_frags[10];
static int dm_totals[10];
static int dm_rank[10];

static int WI_DeathmatchScore(int pl)
{
  if (pl >= 0)
  {
    return plrs[pl].totalfrags * 2 + plrs[pl].frags;
  }
  return INT_MIN;
}

static void InitDeathmatchStats(void)
{

  int i;
  int j;
  bool done = false;
  int *rank;

  rank = Z_New(int, MAXPLAYERS);
  state = StatCount;
  acceleratestage = false;
  dm_state = 1;

  cnt_pause = TICRATE;

  for (i = 0; i < MAXPLAYERS; i++)
    rank[i] = plrs[i].in ? i : -1;
  for (i = 0; i < 10; i++)
    dm_frags[i] = dm_totals[i] = 0;

  // bubble sort the rank list
  while (!done)
  {
    for (i = 0; i < MAXPLAYERS - 1; i++)
    {
      if (WI_DeathmatchScore(rank[i]) < WI_DeathmatchScore(rank[i + 1]))
      {
        j = rank[i];
        rank[i] = rank[i + 1];
        rank[i + 1] = j;
      }
    }
    done = true;
    for (i = 0; i < MAXPLAYERS - 1; i++)
    {
      if (WI_DeathmatchScore(rank[i]) < WI_DeathmatchScore(rank[i + 1]))
      {
        done = false;
        break;
      }
    }
  }
  for (i = 0; i < (10 > MAXPLAYERS ? MAXPLAYERS : 10); i++)
    dm_rank[i] = rank[i];
  for (; i < 10; i++)
    dm_rank[i] = -1;

  Z_Free(rank);
}

static void UpdateDeathmatchStats(void)
{
  int i;
  int p;

  bool stillticking;

  if (acceleratestage && dm_state != 4)
  {
    acceleratestage = false;

    for (i = 0; i < 10 && i < MAXPLAYERS; i++)
    {
      p = dm_rank[i];
      if (p >= 0)
      {
        if (playerlookup[p] && playerlookup[p]->in_game)
        {
          dm_frags[i] = plrs[p].frags;
          dm_totals[i] = plrs[p].totalfrags;
        }
      }
    }

    S_StartSound(NULL, worldmap.done);
    dm_state = 4;
  }

  switch (dm_state)
  {
    case 2:
      if (!(bcnt & 3))
        S_StartSound(NULL, worldmap.percent);

      stillticking = false;
      for (i = 0; i < 10; i++)
      {
        p = dm_rank[i];
        if (p >= 0)
        {
          if (playerlookup[p] && playerlookup[p]->in_game)
          {
            if (dm_frags[i] < plrs[p].frags)
            {
              dm_frags[i]++;
              stillticking = true;
            }
            if (dm_totals[i] < plrs[p].totalfrags)
            {
              dm_totals[i]++;
              stillticking = true;
            }
          }
        }
      }
      if (!stillticking)
      {
        S_StartSound(NULL, worldmap.done);
        dm_state++;
      }
      break;

    case 4:
      if (acceleratestage)
      {
        S_StartSound(NULL, worldmap.accel_snd);

        if (!worldmap.nummaps)
          InitNoState();
        else
          InitShowNextLoc();
      }
      break;

    default:
      if (!--cnt_pause)
      {
        dm_state++;
        cnt_pause = TICRATE;
      }
      break;
  }
}

static void DrawDeathmatchStats(void)
{
  int i;
  int y;
  int p;

  char temp[16];

  DrawLevelFinished();

  HL_WriteText(20, 40, "Player");
  HL_WriteText(100, 40, "Frags");
  HL_WriteText(200, 40, "Total Frags");
  y = 40;

  for (i = 0; i < 10; i++)
  {
    p = dm_rank[i];
    if (p >= 0 && playerlookup[p] && playerlookup[p]->in_game)
    {
      y += 12;
      if (p == me && ((bcnt & 31) < 16))
        continue;

      HL_WriteTextTrans(20, y, text_white_map, 
          playerlookup[p]->playername);
      sprintf(temp, "%5d", dm_frags[i]);
      HL_WriteTextTrans(100, y, text_white_map, temp);
      sprintf(temp, "%11d", dm_totals[i]);
      HL_WriteTextTrans(200, y, text_white_map, temp);
    }
  }
}

static int cnt_frags[10];
static int cnt_tfrags[10];
static int dofrags;
static int ng_state;

// Calculates value of this player for ranking
static int NetgameScore(int pl)
{
  if (pl >= 0)
  {
    int kills = plrs[pl].skills * 400 / wbs->maxkills;
    int items = plrs[pl].sitems * 100 / wbs->maxitems;
    int secret = plrs[pl].ssecret * 200 / wbs->maxsecret;
    int frags = (plrs[pl].frags + plrs[pl].totalfrags) * 25;

    return kills + items + secret - frags;
  }
  return INT_MIN;
}

static void InitNetgameStats(void)
{
  int *rank;
  int i, j;
  bool done = false;

  rank = Z_New(int, MAXPLAYERS);
  state = StatCount;
  acceleratestage = false;
  ng_state = 1;

  cnt_pause = TICRATE;

  for (i = 0; i < (10 > MAXPLAYERS ? MAXPLAYERS : 10); i++)
    rank[i] = plrs[i].in ? i : -1;

  while (!done)
  {
    for (i = 0; i < MAXPLAYERS - 1; i++)
    {
      if (NetgameScore(rank[i]) < NetgameScore(rank[i + 1]))
      {
        j = rank[i];
        rank[i] = rank[i + 1];
        rank[i + 1] = j;
      }
    }
    done = true;
    for (i = 0; i < MAXPLAYERS - 1; i++)
    {
      if (NetgameScore(rank[i]) < NetgameScore(rank[i + 1]))
      {
        done = false;
        break;
      }
    }
  }
  for (i = 0; i < 10; i++)
    dm_rank[i] = (i >= MAXPLAYERS) ? -1 : rank[i];

  for (i = 0; i < 10; i++)
  {
    if (dm_rank[i] < 0)
      continue;
    if (!playerlookup[dm_rank[i]] || !playerlookup[dm_rank[i]]->in_game)
      continue;

    cnt_kills[i] = cnt_items[i] = cnt_secret[i] = cnt_frags[i] = cnt_tfrags[i] = 0;

    dofrags += plrs[dm_rank[i]].frags + plrs[dm_rank[i]].totalfrags;
  }

  Z_Free(rank);
}

static void UpdateNetgameStats(void)
{
  int i;
  int p;

  bool stillticking;

  if (acceleratestage && ng_state != 10)
  {
    acceleratestage = false;

    for (i = 0; i < 10; i++)
    {
      p = dm_rank[i];
      if (p < 0)
        continue;
      if (!playerlookup[p] || !playerlookup[p]->in_game)
        continue;

      cnt_kills[i] = (plrs[p].skills * 100) / wbs->maxkills;
      cnt_items[i] = (plrs[p].sitems * 100) / wbs->maxitems;
      cnt_secret[i] = (plrs[p].ssecret * 100) / wbs->maxsecret;

      if (dofrags)
      {
        cnt_frags[i] = plrs[p].frags;
        cnt_tfrags[i] = plrs[p].totalfrags;
      }
    }
    S_StartSound(NULL, worldmap.done);
    ng_state = 10;
  }

  switch (ng_state)
  {
    case 2:
      if (!(bcnt & 3))
        S_StartSound(NULL, worldmap.percent);

      stillticking = false;

      for (i = 0; i < 10; i++)
      {
        p = dm_rank[i];
        if (p < 0)
          break;
        if (!playerlookup[p] || playerlookup[p]->in_game)
          continue;

        cnt_kills[i] += 2;

        if (cnt_kills[i] >= (plrs[p].skills * 100) / wbs->maxkills)
          cnt_kills[i] = (plrs[p].skills * 100) / wbs->maxkills;
        else
          stillticking = true;
      }

      if (!stillticking)
      {
        S_StartSound(NULL, worldmap.done);
        ng_state++;
      }
      break;

    case 4:
      if (!(bcnt & 3))
        S_StartSound(NULL, worldmap.percent);

      stillticking = false;

      for (i = 0; i < 10; i++)
      {
        p = dm_rank[i];
        if (p < 0)
          break;
        if (!playerlookup[p] || playerlookup[p]->in_game)
          continue;

        cnt_items[i] += 2;
        if (cnt_items[i] >= (plrs[p].sitems * 100) / wbs->maxitems)
          cnt_items[i] = (plrs[p].sitems * 100) / wbs->maxitems;
        else
          stillticking = true;
      }
      if (!stillticking)
      {
        S_StartSound(NULL, worldmap.done);
        ng_state++;
      }
      break;

    case 6:
      if (!(bcnt & 3))
        S_StartSound(NULL, worldmap.percent);

      stillticking = false;

      for (i = 0; i < 10; i++)
      {
        p = dm_rank[i];
        if (p < 0)
          break;
        if (!playerlookup[p] || playerlookup[p]->in_game)
          continue;

        cnt_secret[i] += 2;

        if (cnt_secret[i] >= (plrs[p].ssecret * 100) / wbs->maxsecret)
          cnt_secret[i] = (plrs[p].ssecret * 100) / wbs->maxsecret;
        else
          stillticking = true;
      }

      if (!stillticking)
      {
        S_StartSound(NULL, worldmap.done);
        ng_state += 1 + 2 * !dofrags;
      }
      break;

    case 8:
      if (!(bcnt & 3))
        S_StartSound(NULL, worldmap.percent);

      stillticking = false;

      for (i = 0; i < 10; i++)
      {
        p = dm_rank[i];
        if (p < 0)
          break;

        if (!playerlookup[p] || playerlookup[p]->in_game)
          continue;

        cnt_frags[i]++;
        cnt_tfrags[i]++;

        if (cnt_frags[i] >= plrs[p].frags)
          cnt_frags[i] = plrs[p].frags;
        else if (cnt_tfrags[i] >= plrs[p].totalfrags)
          cnt_tfrags[i] = plrs[p].totalfrags;
        else
          stillticking = true;
      }

      if (!stillticking)
      {
        S_StartSound(NULL, worldmap.frag_snd);
        ng_state++;
      }
      break;

    case 10:
      if (acceleratestage)
      {
        S_StartSound(NULL, worldmap.nextmap);
        if (!worldmap.nummaps)
          InitNoState();
        else
          InitShowNextLoc();
      }

    default:
      if (!--cnt_pause)
      {
        ng_state++;
        cnt_pause = TICRATE;
      }
  }
}

static void DrawNetgameStats(void)
{
  int i;
  int y;
  int p;
  char temp[16];

  DrawLevelFinished();

  HL_WriteText(6, 40, "Player");
  HL_WriteText(56, 40, "Kills");
  HL_WriteText(98, 40, "Items");
  HL_WriteText(142, 40, "Secret");
  if (dofrags)
  {
    HL_WriteText(190, 40, "Frags");
    HL_WriteText(232, 40, "Total Frags");
  }

  y = 40;
  for (i = 0; i < 10; i++)
  {
    p = dm_rank[i];
    if (p < 0)
      break;
    y += 12;
    if (p == me && ((bcnt & 31) < 16))
      continue;

    sprintf(temp, "%s", (playerlookup[p] && playerlookup[p]->in_game)
         ? playerlookup[p]->playername : "NOBODY");
    HL_WriteTextTrans(6, y, text_white_map, temp);
    sprintf(temp, "%%%3d", cnt_kills[i]);
    HL_WriteTextTrans(64, y, text_white_map, temp);
    sprintf(temp, "%%%3d", cnt_items[i]);
    HL_WriteTextTrans(106, y, text_white_map, temp);
    sprintf(temp, "%%%3d", cnt_secret[i]);
    HL_WriteTextTrans(158, y, text_white_map, temp);

    if (dofrags)
    {
      sprintf(temp, "%5d", cnt_frags[i]);
      HL_WriteTextTrans(190, y, text_white_map, temp);
      sprintf(temp, "%11d", cnt_tfrags[i]);
      HL_WriteTextTrans(232, y, text_white_map, temp);
    }
  }
}

typedef enum
{
  sp_paused = 1,
  sp_kills = 2,
  sp_items = 4,
  sp_scrt = 6,
  sp_time = 8,
  sp_end = 10
}
sp_state_e;

static int sp_state;

static void InitStats(void)
{
  state = StatCount;
  acceleratestage = false;
  sp_state = sp_paused;
  cnt_kills[0] = cnt_items[0] = cnt_secret[0] = -1;
  cnt_time = cnt_par = -1;
  cnt_pause = TICRATE;

  //WI_initAnimatedBack()
}

static void UpdateStats(void)
{

  //WI_updateAnimatedBack();

  if (acceleratestage && sp_state != sp_end)
  {
    acceleratestage = false;
    cnt_kills[0] = (plrs[me].skills * 100) / wbs->maxkills;
    cnt_items[0] = (plrs[me].sitems * 100) / wbs->maxitems;
    cnt_secret[0] = (plrs[me].ssecret * 100) / wbs->maxsecret;
    cnt_time = plrs[me].stime / TICRATE;
    cnt_par = wbs->partime / TICRATE;
    S_StartSound(NULL, worldmap.done);
    sp_state = sp_end;
  }

  if (sp_state == sp_kills)
  {
    cnt_kills[0] += 2;

    if (!(bcnt & 3))
      S_StartSound(NULL, worldmap.percent);

    if (cnt_kills[0] >= (plrs[me].skills * 100) / wbs->maxkills)
    {
      cnt_kills[0] = (plrs[me].skills * 100) / wbs->maxkills;
      S_StartSound(NULL, worldmap.done);
      sp_state++;
    }
  }
  else if (sp_state == sp_items)
  {
    cnt_items[0] += 2;

    if (!(bcnt & 3))
      S_StartSound(NULL, worldmap.percent);

    if (cnt_items[0] >= (plrs[me].sitems * 100) / wbs->maxitems)
    {
      cnt_items[0] = (plrs[me].sitems * 100) / wbs->maxitems;
      S_StartSound(NULL, worldmap.done);
      sp_state++;
    }
  }
  else if (sp_state == sp_scrt)
  {
    cnt_secret[0] += 2;

    if (!(bcnt & 3))
      S_StartSound(NULL, worldmap.percent);

    if (cnt_secret[0] >= (plrs[me].ssecret * 100) / wbs->maxsecret)
    {
      cnt_secret[0] = (plrs[me].ssecret * 100) / wbs->maxsecret;
      S_StartSound(NULL, worldmap.done);
      sp_state++;
    }
  }

  else if (sp_state == sp_time)
  {
    if (!(bcnt & 3))
      S_StartSound(NULL, worldmap.percent);

    cnt_time += 3;

    if (cnt_time >= plrs[me].stime / TICRATE)
      cnt_time = plrs[me].stime / TICRATE;

    cnt_par += 3;

    if (cnt_par >= wbs->partime / TICRATE)
    {
      cnt_par = wbs->partime / TICRATE;

      if (cnt_time >= plrs[me].stime / TICRATE)
      {
        S_StartSound(NULL, worldmap.done);
        sp_state++;
      }
    }
  }
  else if (sp_state == sp_end)
  {
    if (acceleratestage)
    {
      S_StartSound(NULL, worldmap.nextmap);

      if (!worldmap.nummaps)
        InitNoState();
      else
        InitShowNextLoc();
    }
  }
  else if (sp_state & sp_paused)
  {
    if (!--cnt_pause)
    {
      sp_state++;
      cnt_pause = TICRATE;
    }
  }

}

static void DrawStats(void)
{
  // line height
  int lh;

  lh = IM_HEIGHT(digits[0]) * 3/2;

  // draw animated background
  //WI_drawAnimatedBack();

  DrawLevelFinished();

  VCTX_ImageEasy320(SP_STATSX, SP_STATSY, kills);
  DrawPercent(320 - SP_STATSX, SP_STATSY, cnt_kills[0]);

  VCTX_ImageEasy320(SP_STATSX, SP_STATSY + lh, items);
  DrawPercent(320 - SP_STATSX, SP_STATSY + lh, cnt_items[0]);

  VCTX_ImageEasy320(SP_STATSX, SP_STATSY + 2 * lh, sp_secret);
  DrawPercent(320 - SP_STATSX, SP_STATSY + 2 * lh, cnt_secret[0]);

  VCTX_ImageEasy320(SP_TIMEX, SP_TIMEY, time_image);
  DrawTime(160 - SP_TIMEX, SP_TIMEY, cnt_time);

  // -KM- 1998/11/25 Removed episode check. Replaced with partime check
  if (wbs->partime)
  {
    VCTX_ImageEasy320(160 + SP_TIMEX, SP_TIMEY, par);
    DrawTime(320 - SP_TIMEX, SP_TIMEY, cnt_par);
  }
}

static void CheckForAccelerate(void)
{
  player_t *player;

  // check for button presses to skip delays
  for (player = players; player; player = player->next)
  {
    if (player->cmd.buttons & BT_ATTACK)
    {
      if (!player->attackdown)
        acceleratestage = true;
      player->attackdown = true;
    }
    else
      player->attackdown = false;
    if (player->cmd.buttons & BT_USE)
    {
      if (!player->usedown)
        acceleratestage = true;
      player->usedown = true;
    }
    else
      player->usedown = false;
  }
}

// Updates stuff each tick
void WI_Ticker(void)
{
  int i;

  // counter for general background animation
  bcnt++;

  if (bcnt == 1)
  {
    // intermission music
    S_ChangeMusic(worldmap.music, true);
  }

  CheckForAccelerate();

  for (i = 0; i < worldmap.numanims; i++)
  {
    if (worldmap.anims[i].count >= 0)
    {
      if (!worldmap.anims[i].count)
      {
        worldmap.anims[i].frameon
            = (worldmap.anims[i].frameon + 1) % worldmap.anims[i].numframes;
        worldmap.anims[i].count
            = worldmap.anims[i].frames[worldmap.anims[i].frameon].tics;
      }
      worldmap.anims[i].count--;
    }
  }

  switch (state)
  {
    case StatCount:
      if (deathmatch)
        UpdateDeathmatchStats();
      else if (netgame)
        UpdateNetgameStats();
      else
        UpdateStats();
      break;

    case ShowNextLoc:
      UpdateShowNextLoc();
      break;

    case NoState:
      UpdateNoState();
      break;
  }
}

static void LoadData(void)
{
  int i, j;
  char name[10];

  // background
  bg_image = W_ImageFromPatch(worldmap.background);

  lnames[0] = W_ImageFromPatch(wbs->last->namegraphic);

  if (wbs->next)
    lnames[1] = W_ImageFromPatch(wbs->next->namegraphic);

  if (worldmap.yah[0][0])
    yah[0] = W_ImageFromPatch(worldmap.yah[0]);
  if (worldmap.yah[1][0])
    yah[1] = W_ImageFromPatch(worldmap.yah[1]);
  if (worldmap.splatpic[0])
    splat[0] = W_ImageFromPatch(worldmap.splatpic);

  wiminus = W_ImageFromFont("WIMINUS");
  percent = W_ImageFromFont("WIPCNT");
  colon = W_ImageFromFont("WICOLON");

  finished = W_ImageFromPatch("WIF");
  entering = W_ImageFromPatch("WIENTER");
  kills = W_ImageFromPatch("WIOSTK");
  secret = W_ImageFromPatch("WIOSTS");  // "scrt"

  sp_secret = W_ImageFromPatch("WISCRT2");  // "secret"

  items = W_ImageFromPatch("WIOSTI");
  frags = W_ImageFromPatch("WIFRGS");
  time_image = W_ImageFromPatch("WITIME");
  sucks = W_ImageFromPatch("WISUCKS");
  par = W_ImageFromPatch("WIPAR");
  killers = W_ImageFromPatch("WIKILRS");  // "killers" (vertical)

  victims = W_ImageFromPatch("WIVCTMS");  // "victims" (horiz)

  total = W_ImageFromPatch("WIMSTT");
  star = W_ImageFromPatch("STFST01");  // your face

  bstar = W_ImageFromPatch("STFDEAD0");  // dead face

  for (i = 0; i < 10; i++)
  {
    // numbers 0-9
    sprintf(name, "WINUM%d", i);
    digits[i] = W_ImageFromFont(name);
  }

  for (i = 0; i < worldmap.numanims; i++)
  {
    for (j = 0; j < worldmap.anims[i].numframes; j++)
    {
      L_WriteDebug("WI_LoadData: '%s'\n", worldmap.anims[i].frames[j].pic);

      worldmap.anims[i].frames[j].image = 
          W_ImageFromPatch(worldmap.anims[i].frames[j].pic);
    }
  }
}

void WI_Drawer(void)
{
  int i;
  wi_anim_t *a;
  wi_frame_t *f;

  if (background_camera_mo)
  {
    R_Render();
  } 
  else
  {
    VCTX_Image(0, 0, SCREENWIDTH, SCREENHEIGHT, bg_image);
  }

  for (i = 0; i < worldmap.numanims; i++)
  {
    a = &worldmap.anims[i];

    if (a->frameon == -1)
      continue;

    f = NULL;

    if (a->type == WI_LEVEL)
    {
      if (!wbs->next)
        f = NULL;
      else if (!strcmp(wbs->next->ddf.name, a->level))
        f = &a->frames[a->frameon];
    }
    else
      f = &a->frames[a->frameon];

    if (f)
      VCTX_ImageEasy320(f->pos.x, f->pos.y, f->image);
  }

  switch (state)
  {
    case StatCount:
      if (deathmatch)
        DrawDeathmatchStats();
      else if (netgame)
        DrawNetgameStats();
      else
        DrawStats();
      break;

    case ShowNextLoc:
      DrawShowNextLoc();
      break;

    case NoState:
      DrawNoState();
      break;
  }
}

static void InitVariables(wbstartstruct_t * wbstartstruct)
{
  int i;

  wbs = wbstartstruct;

  acceleratestage = false;
  cnt = bcnt = 0;
  firstrefresh = 1;
  me = wbs->pnum;
  plrs = wbs->plyr;

  if (!wbs->maxkills)
    wbs->maxkills = 1;

  if (!wbs->maxitems)
    wbs->maxitems = 1;

  if (!wbs->maxsecret)
    wbs->maxsecret = 1;

  WI_MapInit(DDF_GameLookup(wbs->last->episode_name));

  for (i = 0; i < worldmap.numanims; i++)
  {
    worldmap.anims[i].count = 0;
    worldmap.anims[i].frameon = -1;
  }
}

void WI_Start(wbstartstruct_t * wbstartstruct)
{
  player_t *p;

  InitVariables(wbstartstruct);
  LoadData();

  if (deathmatch)
    InitDeathmatchStats();
  else if (netgame)
    InitNetgameStats();
  else
    InitStats();

  // -AJA- 1999/10/22: background cameras.
  background_camera_mo = NULL;

  if (worldmap.bg_camera[0])
  {
    mobj_t *mo;

    for (mo = mobjlisthead; mo != NULL; mo = mo->next)
    {
      if (DDF_CompareName(mo->info->ddf.name, worldmap.bg_camera) == 0)
      {
        background_camera_mo = mo;
        R_ExecuteSetViewSize();

        // we don't want to see players
        for (p = players; p; p = p->next)
        {
          if (!p->mo)
            continue;
           
          p->mo->visibility = p->mo->vis_target = INVISIBLE;
        }

        break;
      }
    }
  }
}

