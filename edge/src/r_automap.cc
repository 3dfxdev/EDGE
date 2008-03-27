//----------------------------------------------------------------------------
//  EDGE Automap Functions
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2008  The EDGE Team.
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

#include "i_defs.h"
#include "i_defs_gl.h"

#include "con_cvar.h"
#include "con_main.h"
#include "e_input.h"
#include "hu_lib.h"
#include "m_argv.h"
#include "m_bbox.h"
#include "m_cheat.h"
#include "m_misc.h"
#include "n_network.h"
#include "p_local.h"
#include "r_automap.h"
#include "st_stuff.h"
#include "r_draw.h"
#include "r_colormap.h"
#include "r_modes.h"

#include <stdio.h>
#include <float.h>
#include <math.h>

#define DEBUG_TRUEBSP  0
#define DEBUG_COLLIDE  0

// Automap colors

static rgbcol_t am_colors[AM_NUM_COLORS] =
{
	RGB_MAKE(112,112,112),  // AMCOL_Grid

    RGB_MAKE(255,  0,  0),  // AMCOL_Wall
    RGB_MAKE(192,128, 80),  // AMCOL_Step
    RGB_MAKE(192,128, 80),  // AMCOL_Ledge
    RGB_MAKE(220,220,  0),  // AMCOL_Ceil
    RGB_MAKE(  0,200,200),  // AMCOL_Secret
    RGB_MAKE(136,136,136),  // AMCOL_Allmap

    RGB_MAKE(255,255,255),  // AMCOL_Player
    RGB_MAKE(  0,255,  0),  // AMCOL_Monster
    RGB_MAKE(220,  0,  0),  // AMCOL_Corpse
    RGB_MAKE(  0,  0,255),  // AMCOL_Item
    RGB_MAKE(255,188,  0),  // AMCOL_Missile
    RGB_MAKE(120, 60, 30)   // AMCOL_Scenery
};

///---#define YOUR_COL    (WHITE)
///---#define WALL_COL    (RED)
///---#define FLOOR_COL   (BROWN + BROWN_LEN/2)
///---#define CEIL_COL    (YELLOW)
///---#define REGION_COL  (BEIGE)
///---#define TELE_COL    (RED + RED_LEN/2)
///---#define SECRET_COL  (CYAN + CYAN_LEN/4)
///---#define GRID_COL    (GRAY + GRAY_LEN*3/4)
///---#define ALLMAP_COL  (GRAY + GRAY_LEN/2)
///---#define MINI_COL    (BLUE + BLUE_LEN/2)
///---
///---#define THING_COL   (BROWN + BROWN_LEN*2/3)
///---#define MONST_COL   (GREEN + 2)
///---#define DEAD_COL    (RED + 2)
///---#define MISSL_COL   (ORANGE)
///---#define ITEM_COL    (BLUE+1)

// Automap keys
// Ideally these would be configurable...

#define AM_PANDOWNKEY KEYD_DOWNARROW
#define AM_PANUPKEY   KEYD_UPARROW
#define AM_PANRIGHTKEY    KEYD_RIGHTARROW
#define AM_PANLEFTKEY     KEYD_LEFTARROW
#define AM_ZOOMINKEY  '='
#define AM_ZOOMOUTKEY '-'
#define AM_STARTKEY   key_map
#define AM_ENDKEY     key_map
#define AM_GOBIGKEY   '0'
#define AM_FOLLOWKEY  'f'
#define AM_GRIDKEY    'g'
#define AM_MARKKEY    'm'
#define AM_CLEARMARKKEY    'c'

#define AM_NUMMARKPOINTS  9

//
// NOTE:
//   `F' in the names here means `Framebuffer', i.e. on-screen coords.
//   `M' in the names means `Map', i.e. coordinates in the level.
//

// scale on entry
#define INIT_MSCALE (4.0f)
#define  MAX_MSCALE (200.0f)

// how much the automap moves window per tic in frame-buffer coordinates
// moves a whole screen-width in 1.5 seconds
#define F_PANINC 6.1

// how much zoom-in per tic
// goes to 3x in 1 second
#define M_ZOOMIN  1.03f

// how much zoom-in for each mouse-wheel click
// goes to 3x in 4 clicks
#define WHEEL_ZOOMIN  1.32f


static int cheating = 0;
static int grid = 0;

static int leveljuststarted = 1;  // kludge until LevelInit() is called

int automapactive = 0;


// location and size of window on screen
static int f_x, f_y;
static int f_w, f_h;

// scale value which makes the whole map fit into the on-screen area
// (multiplying map coords by this value).
static float f_scale;


// location on map which the map is centred on
static float m_cx, m_cy;

// relative scaling: 1.0 = map fits the on-screen area,
//                   2.0 = map is twice as big
//                   8.0 = map is eight times as big
static float m_scale;


// translates between frame-buffer and map distances
#define MTOF(xx) (  (int)((xx) * m_scale * f_scale))
#define FTOM(xx) ((float)((xx) / m_scale / f_scale))

// translates between frame-buffer and map coordinates
#define CXMTOF(xx)  (f_x + f_w/2 + MTOF((xx) - m_cx))
#define CYMTOF(yy)  (f_y + f_h/2 + MTOF((yy) - m_cy))


// largest size of map along X or Y axis
static float map_size;

static float map_min_x;
static float map_min_y;
static float map_max_x;
static float map_max_y;


// how far the window pans each tic (map coords)
static float panning_x = 0;
static float panning_y = 0;

// how far the window zooms in each tic (map coords)
static float zooming = -1;


// where the points are
static mpoint_t markpoints[AM_NUMMARKPOINTS];

// next point to be assigned
static int markpointnum = 0;

// specifies whether to follow the player around
static bool followplayer = true;


cheatseq_t cheat_amap = {0, 0};

static bool stopped = true;

static bool bigstate = false;

bool map_overlay = false;

extern style_c *automap_style;  // FIXME: put in header


//
// adds a marker at the current location
//
static void AddMark(void)
{
	markpoints[markpointnum].x = m_cx;
	markpoints[markpointnum].y = m_cy;

	markpointnum = (markpointnum + 1) % AM_NUMMARKPOINTS;
}

//
// Determines bounding box of all vertices,
// sets global variables controlling zoom range.
//
static void FindMinMaxBoundaries(void)
{
	map_min_x = +9e9;
	map_min_y = +9e9;

	map_max_x = -9e9;
	map_max_y = -9e9;

	for (int i = 0; i < numvertexes; i++)
	{
		map_min_x = MIN(map_min_x, vertexes[i].x);
		map_max_x = MAX(map_max_x, vertexes[i].x);

		map_min_y = MIN(map_min_y, vertexes[i].y);
		map_max_y = MAX(map_max_y, vertexes[i].y);
	}

	float map_w = map_max_x - map_min_x;
	float map_h = map_max_y - map_min_y;

	map_size = MAX(map_w, map_h);
}


static void ClearMarks(void)
{
	int i;

	for (i = 0; i < AM_NUMMARKPOINTS; i++)
		markpoints[i].x = -1;  // means empty

	markpointnum = 0;
}


//
// should be called at the start of every level
// right now, i figure it out myself
//
static void LevelInit(void)
{
	if (!cheat_amap.sequence)
		cheat_amap.sequence = language["iddt"];

	leveljuststarted = 0;

	ClearMarks();

	FindMinMaxBoundaries();

	m_scale = INIT_MSCALE;
}

void AM_InitResolution(void)
{
	LevelInit();  // -ES- 1998/08/20

	CON_CreateCVarBool("newhud", cf_normal, &map_overlay);

	if (M_CheckParm("-newmap"))
		map_overlay = true;
}


void AM_Stop(void)
{
	automapactive = 0;
	stopped = true;

	panning_x = 0;
	panning_y = 0;
	zooming   = -1;
	bigstate  = false;
}

static void AM_Hide(void)
{
	panning_x = 0;
	panning_y = 0;
	zooming   = -1;
	automapactive = 0;
	bigstate = false;
}

static void AM_Show(void)
{
	if (map_overlay == true)
		automapactive = 1;
	else
		automapactive = 2;

	if (! stopped)
	///	AM_Stop();
		return;


	LevelInit();

	player_t *p = players[displayplayer];

	stopped = false;

	SYS_ASSERT(p);

	if (map_overlay == true)
		automapactive = 1;
	else
		automapactive = 2;

	panning_x = 0;
	panning_y = 0;
	zooming   = -1;
	bigstate  = false;

	m_cx = p->mo->x;
	m_cy = p->mo->y;

}


//
// Zooming
//
static void ChangeWindowScale(float factor)
{
	m_scale *= factor;

	m_scale = MAX(m_scale, 1.0);
	m_scale = MIN(m_scale, MAX_MSCALE);
}


//
// Handle events (user inputs) in automap mode
//
bool AM_Responder(event_t * ev)
{
	if (! automapactive)
	{
		if (ev->type == ev_keydown &&
			(ev->value.key == (AM_STARTKEY >> 16) ||
			 ev->value.key == (AM_STARTKEY & 0xffff)))
		{
			AM_Show();
			return true;
		}

		return false;
	}

	// handle key releases

	if (ev->type == ev_keyup)
	{
		switch (ev->value.key)
		{
		case AM_PANRIGHTKEY:
		case AM_PANLEFTKEY:
			panning_x = 0;
			break;

		case AM_PANUPKEY:
		case AM_PANDOWNKEY:
			panning_y = 0;
			break;

		case AM_ZOOMOUTKEY:
		case AM_ZOOMINKEY:
			zooming = -1;
			break;
		}

		return false;
	}


	// handle key presses

	bool rc = false;

	if (ev->type == ev_keydown)
	{
		rc = true;
		switch (ev->value.key)
		{
		case AM_PANRIGHTKEY:
			// pan right
			if (!followplayer)
				panning_x = FTOM(F_PANINC);
			else
				rc = false;
			break;

		case AM_PANLEFTKEY:
			// pan left
			if (!followplayer)
				panning_x = -FTOM(F_PANINC);
			else
				rc = false;
			break;

		case AM_PANUPKEY:
			// pan up
			if (!followplayer)
				panning_y = FTOM(F_PANINC);
			else
				rc = false;
			break;

		case AM_PANDOWNKEY:
			// pan down
			if (!followplayer)
				panning_y = -FTOM(F_PANINC);
			else
				rc = false;
			break;

		case AM_ZOOMOUTKEY:
			// zoom out
			zooming = 1.0 / M_ZOOMIN;
			break;

		case AM_ZOOMINKEY:
			// zoom in
			zooming = M_ZOOMIN;
			break;

		// -AJA- 2007/04/18: mouse-wheel support
		case KEYD_MWHEEL_DN:
			ChangeWindowScale(1.0 / WHEEL_ZOOMIN);
			break;

		case KEYD_MWHEEL_UP:
			ChangeWindowScale(WHEEL_ZOOMIN);
			break;

		case AM_GOBIGKEY:
			bigstate = !bigstate;
			break;

		case AM_FOLLOWKEY:
			followplayer = !followplayer;

			// -ACB- 1998/08/10 Use DDF Lang Reference
			if (followplayer)
				CON_PlayerMessageLDF(consoleplayer, "AutoMapFollowOn");
			else
				CON_PlayerMessageLDF(consoleplayer, "AutoMapFollowOff");
			break;

		case AM_GRIDKEY:
			grid = !grid;
			// -ACB- 1998/08/10 Use DDF Lang Reference
			if (grid)
				CON_PlayerMessageLDF(consoleplayer, "AutoMapGridOn");
			else
				CON_PlayerMessageLDF(consoleplayer, "AutoMapGridOff");
			break;

		case AM_MARKKEY:
			// -ACB- 1998/08/10 Use DDF Lang Reference
			CON_PlayerMessage(consoleplayer, "%s %d",
				language["AutoMapMarkedSpot"], markpointnum);
			AddMark();
			break;

		case AM_CLEARMARKKEY:
			ClearMarks();
			// -ACB- 1998/08/10 Use DDF Lang Reference
			CON_PlayerMessageLDF(consoleplayer, "AutoMapMarksClear");
			break;

		default:
			if (ev->value.key == (AM_ENDKEY >> 16) || 
				ev->value.key == (AM_ENDKEY & 0xffff))
			{
				if (automapactive == 1)
				{
					automapactive = 2;
				}
				else
				{
					AM_Hide();
				}
			}
			else
			{
				rc = false;
			}
		}
		// -ACB- 1999/09/28 Proper casting
		if (!DEATHMATCH() && M_CheckCheat(&cheat_amap, (char)ev->value.key))
		{
			rc = false;
			cheating = (cheating + 1) % 3;
		}
	}

	return rc;
}


//
// Updates on game tick
//
void AM_Ticker(void)
{
	if (!automapactive)
		return;

	// Change x,y location
	if (followplayer)
	{
		player_t *p = players[displayplayer];

		m_cx = p->mo->x;
		m_cy = p->mo->y;
	}
	else
	{
		m_cx += panning_x;
		m_cy += panning_y;

		// limit position, don't go outside of the map
		m_cx = MIN(m_cx, map_max_x);
		m_cx = MAX(m_cx, map_min_x);

		m_cy = MIN(m_cy, map_max_y);
		m_cy = MAX(m_cy, map_min_y);
	}

	// Change the zoom if necessary
	if (zooming > 0)
		ChangeWindowScale(zooming);

}

//
// Rotation in 2D.
// Used to rotate player arrow line character.
//
static inline void Rotate(float * x, float * y, angle_t a)
{
	float new_x = *x * M_Cos(a) - *y * M_Sin(a);
	float new_y = *x * M_Sin(a) + *y * M_Cos(a);

	*x = new_x;
	*y = new_y;
}

static inline void GetRotatedCoords(player_t *p, float sx, float sy,
									float *dx, float *dy)
{
	*dx = sx;
	*dy = sy;

	if (rotatemap)
	{
		// rotate coordinates so they are on the map correctly
		*dx -= p->mo->x;
		*dy -= p->mo->y;

		Rotate(dx, dy, ANG90 - p->mo->angle);

		*dx += p->mo->x;
		*dy += p->mo->y;
	}
}

static inline angle_t GetRotatedAngle(player_t *p, angle_t src)
{
	if (rotatemap)
		return src + ANG90 - p->mo->angle;

	return src;
}


//
// Draw visible parts of lines.
//
static void DrawMline(mline_t * ml, rgbcol_t rgb)
{
	int x1, y1, x2, y2;
	int f_x2 = f_x + f_w - 1;
	int f_y2 = f_y + f_h - 1;

	// transform to frame-buffer coordinates.
	x1 = CXMTOF(ml->a.x);
	y1 = CYMTOF(ml->a.y);
	x2 = CXMTOF(ml->b.x);
	y2 = CYMTOF(ml->b.y);

	// trivial rejects
	if ((x1 < f_x && x2 < f_x) || (x1 > f_x2 && x2 > f_x2) ||
		(y1 < f_y && y2 < f_y) || (y1 > f_y2 && y2 > f_y2))
	{
		return;
	}

	if (!var_smoothmap)
	{
		if (x1 == x2 or y1 == y2)
		{
			RGL_SolidBox(x1, y1, x2-x1+1, y2-y1+1, rgb);
			return;
		}
	}

	RGL_SolidLine(x1, y1, x2, y2, rgb);
}


//
// Draws flat (floor/ceiling tile) aligned grid lines.
//
static void DrawGrid()
{
#if 0  // FIXME !!!!!!
	int colour = AMCOL_Grid;

	float x, y;
	float start, end;
	mline_t ml;

	// Figure out start of vertical gridlines
	start = m_x + (float)fmod((float)BLOCKMAP_UNIT - (m_x - bmap_orgx), (float)BLOCKMAP_UNIT);
	end = m_x + m_w;

	// draw vertical gridlines
	ml.a.y = m_y;
	ml.b.y = m_y + m_h;
	for (x = start; x < end; x += BLOCKMAP_UNIT)
	{
		ml.a.x = x;
		ml.b.x = x;
		DrawMline(&ml, colour);
	}

	// Figure out start of horizontal gridlines
	start = m_y + (float)fmod((float)BLOCKMAP_UNIT - (m_y - bmap_orgy), (float)BLOCKMAP_UNIT);
	end = m_y + m_h;

	// draw horizontal gridlines
	ml.a.x = m_x;
	ml.b.x = m_x + m_w;
	for (y = start; y < end; y += BLOCKMAP_UNIT)
	{
		ml.a.y = y;
		ml.b.y = y;
		DrawMline(&ml, colour);
	}
#endif
}


//
// Checks whether the two sectors' regions are similiar.  If they are
// different enough, a line will be drawn on the automap.
//
// -AJA- 1999/12/07: written.
//
static bool CheckSimiliarRegions(sector_t *front, sector_t *back)
{
	extrafloor_t *F, *B;

	if (front->tag == back->tag)
		return true;

	// Note: doesn't worry about liquids

	F = front->bottom_ef;
	B = back->bottom_ef;

	while (F && B)
	{
		if (F->top_h != B->top_h)
			return false;

		if (F->bottom_h != B->bottom_h)
			return false;

		F = F->higher;
		B = B->higher;
	}

	return (F || B) ? false : true;
}


//
// Determines visible lines, draws them.
//
// -AJA- This is now *lineseg* based, not linedef.
//
static void AM_WalkSeg(seg_t *seg, player_t *p)
{
	mline_t l;
	line_t *line;

	sector_t *front = seg->frontsector;
	sector_t *back  = seg->backsector;

	if (seg->miniseg)
	{
#if (DEBUG_TRUEBSP == 1)
		if (seg->partner && seg > seg->partner)
			return;

		GetRotatedCoords(p,seg->v1->x, seg->v1->y, &l.a.x, &l.a.y);
		GetRotatedCoords(p,seg->v2->x, seg->v2->y, &l.b.x, &l.b.y);

		DrawMline(&l, RGB_MAKE(0,0,128));
#endif
		return;
	}

	line = seg->linedef;
	SYS_ASSERT(line);

	// only draw segs on the _right_ side of linedefs
	if (line->side[1] == seg->sidedef)
		return;

	GetRotatedCoords(p,seg->v1->x, seg->v1->y, &l.a.x, &l.a.y);
	GetRotatedCoords(p,seg->v2->x, seg->v2->y, &l.b.x, &l.b.y);

	if ((line->flags & MLF_Mapped) || cheating)
	{
		if ((line->flags & MLF_DontDraw) && !cheating)
			return;

		if (!front || !back)
		{
			DrawMline(&l, am_colors[AMCOL_Wall]);
		}
		else
		{
			if (line->flags & MLF_Secret)
			{  
				// secret door
				if (cheating)
					DrawMline(&l, am_colors[AMCOL_Secret]);
				else
					DrawMline(&l, am_colors[AMCOL_Wall]);
			}
			else if (back->f_h != front->f_h)
			{
				float diff = fabs(back->f_h - front->f_h);

				// floor level change
				if (diff > 24)
					DrawMline(&l, am_colors[AMCOL_Ledge]);
				else
					DrawMline(&l, am_colors[AMCOL_Step]);
			}
			else if (back->c_h != front->c_h)
			{
				// ceiling level change
				DrawMline(&l, am_colors[AMCOL_Ceil]);
			}
			else if ((front->exfloor_used > 0 || back->exfloor_used > 0) &&
				(front->exfloor_used != back->exfloor_used ||
				! CheckSimiliarRegions(front, back)))
			{
				// -AJA- 1999/10/09: extra floor change.
				DrawMline(&l, am_colors[AMCOL_Ledge]);
			}
			else if (cheating)
			{
				DrawMline(&l, am_colors[AMCOL_Allmap]);
			}
		}
	}
	else if (p->powers[PW_AllMap])
	{
		if (! (line->flags & MLF_DontDraw))
			DrawMline(&l, am_colors[AMCOL_Allmap]);
	}
}


static void DrawLineCharacter(player_t *p,mline_t *lineguy, int lineguylines, 
							  float radius, angle_t angle, rgbcol_t rgb, float x, float y)
{
	int i;
	mline_t l;
	float ch_x, ch_y;

	if (radius < 2)
		radius = 2;

	GetRotatedCoords(p,x, y, &ch_x, &ch_y);
	angle = GetRotatedAngle(p,angle);

	for (i = 0; i < lineguylines; i++)
	{
		l.a.x = lineguy[i].a.x * radius;
		l.a.y = lineguy[i].a.y * radius;

		if (angle)
			Rotate(&l.a.x, &l.a.y, angle);

		l.a.x += ch_x;
		l.a.y += ch_y;

		l.b.x = lineguy[i].b.x * radius;
		l.b.y = lineguy[i].b.y * radius;

		if (angle)
			Rotate(&l.b.x, &l.b.y, angle);

		l.b.x += ch_x;
		l.b.y += ch_y;

		DrawMline(&l, rgb);
	}
}


#if (DEBUG_COLLIDE == 1)
static void DrawObjectBounds(player_t *p, mobj_t *mo, rgbcol_t rgb)
{
	float R = mo->radius;

	if (R < 2)
		R = 2;

	float lx = mo->x - R;
	float ly = mo->y - R;
	float hx = mo->x + R;
	float hy = mo->y + R;

	mline_t ml;

	GetRotatedCoords(p, lx, ly, &ml.a.x, &ml.a.y);
	GetRotatedCoords(p, lx, hy, &ml.b.x, &ml.b.y);
	DrawMline(&ml, rgb);

	GetRotatedCoords(p, lx, hy, &ml.a.x, &ml.a.y);
	GetRotatedCoords(p, hx, hy, &ml.b.x, &ml.b.y);
	DrawMline(&ml, rgb);

	GetRotatedCoords(p, hx, hy, &ml.a.x, &ml.a.y);
	GetRotatedCoords(p, hx, ly, &ml.b.x, &ml.b.y);
	DrawMline(&ml, rgb);

	GetRotatedCoords(p, hx, ly, &ml.a.x, &ml.a.y);
	GetRotatedCoords(p, lx, ly, &ml.b.x, &ml.b.y);
	DrawMline(&ml, rgb);
}
#endif


static rgbcol_t player_colors[8] =
{
	RGB_MAKE(  5,255,  5),  // GREEN,
	RGB_MAKE( 80, 80, 80),  // GRAY + GRAY_LEN*2/3,
	RGB_MAKE(160,100, 50),  // BROWN,
	RGB_MAKE(255,255,255),  // RED + RED_LEN/2,
	RGB_MAKE(255,176,  5),  // ORANGE,
	RGB_MAKE(170,170,170),  // GRAY + GRAY_LEN*1/3,
	RGB_MAKE(255,  5,  5),  // RED,
	RGB_MAKE(255,185,225),  // PINK
};

//
// The vector graphics for the automap.
//
// A line drawing of the player pointing right, starting from the
// middle.

static mline_t player_arrow[] =
{
	{{-0.875f, 0.0f}, {1.0f, 0.0f}},   // -----

	{{1.0f, 0.0f}, {0.5f,  0.25f}},  // ----->
	{{1.0f, 0.0f}, {0.5f, -0.25f}},

	{{-0.875f, 0.0f}, {-1.125f,  0.25f}},  // >---->
	{{-0.875f, 0.0f}, {-1.125f, -0.25f}},

	{{-0.625f, 0.0f}, {-0.875f,  0.25f}},  // >>--->
	{{-0.625f, 0.0f}, {-0.875f, -0.25f}}
};

#define NUMPLYRLINES (sizeof(player_arrow)/sizeof(mline_t))

static mline_t cheat_player_arrow[] =
{
	{{-0.875f, 0.0f}, {1.0f, 0.0f}},    // -----

	{{1.0f, 0.0f}, {0.5f,  0.167f}},  // ----->
	{{1.0f, 0.0f}, {0.5f, -0.167f}},

	{{-0.875f, 0.0f}, {-1.125f,  0.167f}},  // >----->
	{{-0.875f, 0.0f}, {-1.125f, -0.167f}},

	{{-0.625f, 0.0f}, {-0.875f,  0.167f}},  // >>----->
	{{-0.625f, 0.0f}, {-0.875f, -0.167f}},

	{{-0.5f, 0.0f}, {-0.5f, -0.167f}},      // >>-d--->
	{{-0.5f, -0.167f}, {-0.5f + 0.167f, -0.167f}},
	{{-0.5f + 0.167f, -0.167f}, {-0.5f + 0.167f, 0.25f}},

	{{-0.167f, 0.0f}, {-0.167f, -0.167f}},  // >>-dd-->
	{{-0.167f, -0.167f}, {0.0f, -0.167f}},
	{{0.0f, -0.167f}, {0.0f, 0.25f}},

	{{0.167f, 0.25f}, {0.167f, -0.143f}},  // >>-ddt->
	{{0.167f, -0.143f}, {0.167f + 0.031f, -0.143f - 0.031f}},
	{{0.167f + 0.031f, -0.143f - 0.031f}, {0.167f + 0.1f, -0.143f}}
};

#define NUMCHEATPLYRLINES (sizeof(cheat_player_arrow)/sizeof(mline_t))

static mline_t thin_triangle_guy[] =
{
	{{-0.5f, -0.7f}, {1.0f, 0.0f}},
	{{1.0f, 0.0f}, {-0.5f, 0.7f}},
	{{-0.5f, 0.7f}, {-0.5f, -0.7f}}
};

#define NUMTHINTRIANGLEGUYLINES (sizeof(thin_triangle_guy)/sizeof(mline_t))

static void AM_DrawPlayer(mobj_t *mo)
{
	player_t *p = players[displayplayer];

#if (DEBUG_COLLIDE == 1)
	DrawObjectBounds(p, mo, am_colors[AMCOL_Player]);
#endif

	if (!netgame)
	{
		if (cheating)
			DrawLineCharacter(p, cheat_player_arrow, NUMCHEATPLYRLINES, 
				mo->radius, mo->angle, am_colors[AMCOL_Player], mo->x, mo->y);
		else
			DrawLineCharacter(p, player_arrow, NUMPLYRLINES, 
				mo->radius, mo->angle, am_colors[AMCOL_Player], mo->x, mo->y);

		return;
	}

#if 0 //!!!! TEMP DISABLED, NETWORK DEBUGGING
	if ((DEATHMATCH() && !singledemo) && mo->player != p)
		return;
#endif

	DrawLineCharacter(p, player_arrow, NUMPLYRLINES, 
		mo->radius, mo->angle,
		player_colors[mo->player->pnum & 0x07],
		mo->x, mo->y);
}


static void AM_WalkThing(mobj_t *mo)
{
	int index = AMCOL_Scenery;

	if (mo->player && mo->player->mo == mo)
	{
		AM_DrawPlayer(mo);
		return;
	}

	if (cheating != 2)
		return;

	// -AJA- more colourful things
	if (mo->flags & MF_SPECIAL)
		index = AMCOL_Item;
	else if (mo->flags & MF_MISSILE)
		index = AMCOL_Missile;
	else if (mo->extendedflags & EF_MONSTER && mo->health <= 0)
		index = AMCOL_Corpse;
	else if (mo->extendedflags & EF_MONSTER)
		index = AMCOL_Monster;

#if (DEBUG_COLLIDE == 1)
	DrawObjectBounds(players[displayplayer], mo, am_colors[index]);
	return;
#endif

	DrawLineCharacter(players[displayplayer],
		thin_triangle_guy, NUMTHINTRIANGLEGUYLINES,
		mo->radius, mo->angle, am_colors[index], mo->x, mo->y);
}


//
// Visit a subsector and draw everything.
//
static void AM_WalkSubsector(unsigned int num)
{
	subsector_t *sub = &subsectors[num];

	// handle each seg
	for (seg_t *seg = sub->segs; seg; seg = seg->sub_next)
	{
		AM_WalkSeg(seg, players[displayplayer]);
	}

	// handle each thing
	for (mobj_t *mo = sub->thinglist; mo; mo = mo->snext)
	{
		AM_WalkThing(mo);
	}
}


//
// Checks BSP node/subtree bounding box.
// Returns true if some part of the bbox might be visible.
//
static bool AM_CheckBBox(float *bspcoord)
{
	float xl = bspcoord[BOXLEFT];
	float yt = bspcoord[BOXTOP];
	float xr = bspcoord[BOXRIGHT];
	float yb = bspcoord[BOXBOTTOM];

	if (rotatemap)
	{
		// FIXME: quick'n'dirty hack, removes benefit of BSP render
		return true;
		if (! followplayer)
			return true;

		// HACKITUDE: just make tested area bigger
#if 0
		float d = MAX(m_x2 - m_x, m_y2 - m_y) / 2.0f;

		return ! (xr < (m_x - d) || xl > (m_x2 + d) ||
 				  yt < (m_y - d) || yb > (m_y2 + d));
#endif
	}

	int x1 = CXMTOF(xl);
	int x2 = CXMTOF(xr);

	int y1 = CYMTOF(yb);
	int y2 = CYMTOF(yt);

	if (x2 < f_x || y2 < f_y || x1 >= f_x+f_w || y1 >= f_y+f_h)
		return false;

	// some part of bbox is visible
	return true;
}


//
// Walks all subsectors below a given node, traversing subtree
// recursively.  Just call with BSP root.
//
static void AM_WalkBSPNode(unsigned int bspnum)
{
	node_t *node;
	int side;

	// Found a subsector?
	if (bspnum & NF_V5_SUBSECTOR)
	{
		AM_WalkSubsector(bspnum & (~NF_V5_SUBSECTOR));
		return;
	}

	node = &nodes[bspnum];
	side = 0;

	// Recursively divide right space
	if (AM_CheckBBox(node->bbox[0]))
		AM_WalkBSPNode(node->children[side]);

	// Recursively divide back space
	if (AM_CheckBBox(node->bbox[side ^ 1]))
		AM_WalkBSPNode(node->children[side ^ 1]);
}


static void DrawMarks(void)
{
	for (int i = 0; i < AM_NUMMARKPOINTS; i++)
	{
		if (markpoints[i].x != -1)
		{
			float cx = CXMTOF(markpoints[i].x);
			float cy = CYMTOF(markpoints[i].y);

			float scale = 1.0f; /// f_w / 320.0f;

			font_c *am_font = automap_style->fonts[0];
			SYS_ASSERT(am_font);

			// oh fuck me!
			cx = cx * 320.f / SCREENWIDTH;
			cy = 200.0 - (cy * 200.0f / SCREENHEIGHT);

			am_font->DrawChar(cx, cy, '1'+i, scale,1.0f, NULL,1.0f);
		}
	}
}

static void AM_RenderScene(void)
{
	glEnable(GL_SCISSOR_TEST);

	glScissor(f_x, f_y, f_w, f_h);

	if (var_smoothmap)
	{
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_BLEND);
		glLineWidth(1.5f);
	}

	// walk the bsp tree
	AM_WalkBSPNode(root_node);

	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_BLEND);
	glLineWidth(1.0f);

	glDisable(GL_SCISSOR_TEST);
}


void AM_Drawer(int x, int y, int w, int h)
{
	if (!automapactive)
		return;
	
	f_x = x;
	f_y = y;
	f_w = w;
	f_h = h;

	f_scale = MAX(f_w, f_h) / map_size / 2.0f;

	SYS_ASSERT(automap_style);

	if (grid && !rotatemap)
		DrawGrid();

	AM_RenderScene();

	DrawMarks();
}


void AM_SetColor(int which, rgbcol_t color)
{
	SYS_ASSERT(0 <= which && which < AM_NUM_COLORS);

	am_colors[which] = color;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
