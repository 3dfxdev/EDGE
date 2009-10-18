//----------------------------------------------------------------------------
//  EDGE <-> Wolf3D (Setup)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2000  The EDGE Team.
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
// TODO HERE
//   + DOORS: need separate sectors for each side
//   + DOORS: choose proper linetype (269[012345])
//   + ELEVATOR tile.
//   + consolidate the matrixes into 1 (use a struct)
//   + detect push-wall sectors and create 3 new sectors
//   + handle catch-all pushwalls as a lowering sector
//   + choose proper wall textures
//   + choose better floor/ceiling textures
//   + optimise
//

#include "i_defs.h"
#include "wf_local.h"

#include "dm_data.h"
#include "dm_state.h"
#include "e_player.h"
#include "m_argv.h"
#include "m_bbox.h"
#include "p_local.h"
#include "r_defs.h"
#include "r_sky.h"
#include "rgl_sky.h"
#include "s_sound.h"
#include "s_music.h"
#include "w_textur.h"
#include "z_zone.h"

#include "epi/endianess.h"
#include "epi/math_crc.h"


#define TILE_ELEVATOR     0x15
#define TILE_SECRET_ELEV  0x6B


extern void ShutdownLevel(void);
extern void GroupLines(void);
extern void GenerateBlockMap(int min_x, int min_y, int max_x, int max_y);

extern void P_ComputeLinedefData(line_t *ld, int side0, int side1);

extern void SetupExtrafloors(void);
extern void SetupWallTiles(void);
extern void SetupVertGaps(void);
extern void SetupRootNode(void);


static int cur_map_num;

bool level_active;


int max_vertexes;
int max_lines;
int max_sides;
int max_segs;

#define M_INDEX(x,y)    ((y) * WLFMAP_W + (x))
#define MP1_INDEX(x,y)  ((y) * (WLFMAP_W+1) + (x))

// these keep track of points to analyse
typedef struct analysis_point_s
{
	int x, y;
}
analysis_point_t;

static analysis_point_t *analysis_points = NULL;
static int num_analysis_points = 0;
static int max_analysis_points = 0;

// tiles that have been processed have (Sec# + 1) here
static short *sector_matrix = NULL;
static int marked_cur_sec=0;

// array (65x65 for example) of (Vert# + 1).
static short *vertex_matrix = NULL;

static int marked_cur_vert=0;

// array (65x65x2) of (Sec# or -2 for seg or -1 for void).  First in
// pair is horizontal top of tile, second is vertical left side.
short *wlf_seg_matrix = NULL;

// array of ((Door# + 1) * 4 + (2 type bits) + vert bit) values.
ushort *door_matrix = NULL;
int door_vertexes;
int num_doors;


#define OBJTILE_PL_STARTS  19
#define OBJTILE_PUSHABLE   98

#define MAPTILE_DOORS    90
#define MAPTILE_AREAS   108
#define MAPTILE_AMBUSH  106


static void FindPlayer(int *px, int *py)
{
	int x, y;

	for (y=0; y < WLFMAP_H; y++)
	for (x=0; x < WLFMAP_W; x++)
	{
		int obj = wlf_obj_tiles[y*WLFMAP_W + x];

		if (OBJTILE_PL_STARTS <= obj && obj <= OBJTILE_PL_STARTS+3)
		{
			(*px) = x;
			(*py) = y;

			return;
		}
	}

	I_Error("WF_SetupLevel: missing player start !\n");
}


static void AddAnalysisPoint(int x, int y, int ox, int oy)
{
	analysis_point_t *pnt;
	int tile, obj;

	SYS_ASSERT(0 <= x && x < WLFMAP_W);
	SYS_ASSERT(0 <= y && y < WLFMAP_H);
	SYS_ASSERT(0 <= ox && ox < WLFMAP_W);
	SYS_ASSERT(0 <= oy && oy < WLFMAP_H);

	// don't bother adding point if already handled
	if (sector_matrix[M_INDEX(x,y)])
		return;

	if (num_analysis_points == max_analysis_points)
	{
		max_analysis_points++;
		Z_Resize(analysis_points, analysis_point_t, max_analysis_points);
	}

	pnt = &analysis_points[num_analysis_points];
	num_analysis_points++;

	pnt->x = x;
	pnt->y = y;

	// Check for special things, like doors and pushwalls.

	if (x == ox && y == oy)
		return;

	tile = wlf_map_tiles[M_INDEX(x,y)];
	obj  = wlf_obj_tiles[M_INDEX(x,y)];

	if (tile < MAPTILE_DOORS && obj == OBJTILE_PUSHABLE)
	{
		int dx = x - ox;
		int dy = y - oy;

		if (x + 2*dx < 0 || x + 2*dx >= WLFMAP_W ||
			y + 2*dy < 0 || y + 2*dy >= WLFMAP_H)
		{
			// nowhere for the pushwall to go
			// !!! CONVERT TO lowering wall
		}

		// handle pushwall !!!
		return;
	}

	if (MAPTILE_DOORS <= tile && tile <= MAPTILE_DOORS+11)
	{
		int vert_bit = (tile & 1) ^ 1;
		int kind = (tile - MAPTILE_DOORS) / 2;

		if (num_doors == 255)
			I_Warning("WOLF: too many doors !\n");
		else
		{
			num_doors++;
			door_matrix[M_INDEX(x,y)] = (num_doors * 16) + ((tile-MAPTILE_DOORS)^1);
		}
		return;
	}
}

//
// TestBoundary
//
// Test for a boundary between the current point (x1,y1) and the
// neighbour point (x2, y2).  Returns 0 if none, 1 if soft, 2 if hard.
//
static int TestBoundary(int x1, int y1, int x2, int y2)
{
	int tile1, obj1;
	int tile2, obj2;

	SYS_ASSERT(0 <= x1 && x1 < WLFMAP_W);
	SYS_ASSERT(0 <= y1 && y1 < WLFMAP_H);

	SYS_ASSERT(ABS(x1 - x2) <= 1 && ABS(y1 - y2) <= 1);

	if (x2 < 0 || y2 < 0 || x2 >= WLFMAP_W || y2 >= WLFMAP_H)
	{
		// going outside map: hard boundary
		return 2;
	}

	tile1 = wlf_map_tiles[y1*WLFMAP_W + x1];
	obj1  = wlf_obj_tiles[y1*WLFMAP_W + x1];

	tile2 = wlf_map_tiles[y2*WLFMAP_W + x2];
	obj2  = wlf_obj_tiles[y2*WLFMAP_W + x2];

	if (tile1 < MAPTILE_DOORS && obj1 == OBJTILE_PUSHABLE)
		tile1 = MAPTILE_DOORS;

	if (tile2 < MAPTILE_DOORS && obj2 == OBJTILE_PUSHABLE)
		tile2 = MAPTILE_DOORS;

	// always hard when other tile is solid
	if (tile2 < MAPTILE_DOORS)
		return 2;

	// always none when both tiles are areas
	if (tile1 >= MAPTILE_AREAS && tile2 >= MAPTILE_AREAS)
		return 0;

	return 1;
}

vertex_t *WF_GetVertex(int vx, int vy)
{
	int v_num;

	SYS_ASSERT(0 <= vx && vx <= WLFMAP_W);
	SYS_ASSERT(0 <= vy && vy <= WLFMAP_H);

	v_num = vertex_matrix[MP1_INDEX(vx,vy)];
	SYS_ASSERT(0 < v_num && v_num <= numvertexes);

	return vertexes + (v_num - 1);
}

static vertex_t *GetDoorVertex(int tx, int ty, int which)
{
	int d_num;
	vertex_t *v;

	SYS_ASSERT(0 <= tx && tx < WLFMAP_W);
	SYS_ASSERT(0 <= ty && ty < WLFMAP_H);

	d_num = door_matrix[M_INDEX(tx,ty)];

	SYS_ASSERT(0 < (d_num/16) <= num_doors);

	v = vertexes + (door_vertexes + (d_num/16 - 1) * 2 + which);

	// initialise if not already
	if (v->x == 0 && v->y == 0)
	{
		if (d_num & 1)
		{
			// vertical
			v->x = 64*(tx) + 32;
			v->y = 64*(ty + which);
		}
		else
		{
			// horizontal
			v->x = 64*(tx + which);
			v->y = 64*(ty) + 32;
		}
	}

	return v;
}


//----------------------------------------------------------------------------

//
// FillMapArea
//
// Determine the boundaries from the current point, creating a new
// sector with the given number.  Essentially just a FLOOD-FILL, but
// also checks for tiles on the other side of a soft boundary, adding
// them as new analysis points.
//
static void FillMapArea(short ax, short ay, short sec_num)
{
	short top, last_top;
	short bot, last_bot;
	short bx;

	SYS_ASSERT(0 <= ax && ax < WLFMAP_W);
	SYS_ASSERT(0 <= ay && ay < WLFMAP_H);

	// ignore point if already handled
	if (sector_matrix[M_INDEX(ax,ay)])
		return;

	// move left until we hit the leftmost boundary
	for (; ; ax--)
	{
		top = TestBoundary(ax, ay, ax-1, ay);
		if (top != 0)
			break;
	}

	if (top == 1)
		AddAnalysisPoint(ax-1, ay, ax, ay);

	// move right as far as we can, marking all the tiles
	for (bx=ax; ; bx++)
	{
		sector_matrix[M_INDEX(bx,ay)] = sec_num;

		bot = TestBoundary(bx, ay, bx+1, ay);
		if (bot != 0)
			break;
	}

	if (bot == 1)
		AddAnalysisPoint(bx+1, ay, bx, ay);

	last_top = 2;
	last_bot = 2;

	// Move left to right again, this time checking for spaces above and
	// below the current tile.  Recursively call this routine to handle
	// tiles in the same sector (unless already marked).

	for (; ax <= bx; ax++)
	{
		top = TestBoundary(ax, ay, ax, ay+1);

		if (top != last_top)
		{
			if (top == 0)
				FillMapArea(ax, ay+1, sec_num);
			else if (top == 1)
				AddAnalysisPoint(ax, ay+1, ax, ay);

			last_top = top;
		}

		bot = TestBoundary(ax, ay, ax, ay-1);

		if (bot != last_bot)
		{
			if (bot == 0)
				FillMapArea(ax, ay-1, sec_num);
			else if (bot == 1)
				AddAnalysisPoint(ax, ay-1, ax, ay);

			last_bot = bot;
		}
	}
}

//
// BuildSectors
//
static void BuildSectors(void)
{
	int i;

	numsectors = marked_cur_sec;
	sectors = Z_ClearNew(sector_t, numsectors);

	for (i=0; i < numsectors; i++)
	{
		sector_t *sec = sectors + i;

		sec->f_h = 0;
		sec->c_h = 64;

		sec->floor.translucency = 1.0;
		sec->floor.x_mat.x = 1;  sec->floor.x_mat.y = 0;
		sec->floor.y_mat.x = 0;  sec->floor.y_mat.y = 1;

		sec->ceil = sec->floor;

		// FIXME: floor/ceiling flats -- what/how ?

		sec->floor.image = W_ImageLookup("FLAT19", INS_Flat);
		sec->ceil.image  = W_ImageLookup("FLAT1",  INS_Flat);

		sec->tag = 0;

		sec->props.type = 0;
		sec->props.lightlevel = 160;
		sec->props.special = P_LookupSectorType(sec->props.type);

		sec->props.colourmap = colourmaps.Lookup("NORMAL");
		
		sec->props.gravity   = GRAVITY;
		sec->props.friction  = FRICTION;
		sec->props.viscosity = VISCOSITY;
		sec->props.drag      = DRAG;

		sec->p = &sec->props;

		sec->exfloor_max = 0;
		sec->sound_player = -1;

		// GroupSectorTags(sec, sectors, i);
	}
}

static INLINE void TryVertex(int x1, int y1, int dx, int dy)
{
	int vx = x1 + ((dx > 0) ? 1 : 0);
	int vy = y1 + ((dy > 0) ? 1 : 0);

	SYS_ASSERT(dx != 0 && dy != 0);

	SYS_ASSERT(0 <= vx <= WLFMAP_W);
	SYS_ASSERT(0 <= vy <= WLFMAP_H);

	// already marked ?
	if (vertex_matrix[MP1_INDEX(vx,vy)] > 0)
		return;

	marked_cur_vert++;
	vertex_matrix[MP1_INDEX(vx,vy)] = marked_cur_vert;
}

//
// BuildVertices
//
static void BuildVertices(void)
{
	int x, y;

	// find which vertices are needed
	for (y=0; y < WLFMAP_H; y++)
		for (x=0; x < WLFMAP_W; x++)
		{
			int sec = sector_matrix[M_INDEX(x,y)];

			if (! sec)
				continue;

			TryVertex(x, y, +1, +1);
			TryVertex(x, y, +1, -1);
			TryVertex(x, y, -1, +1);
			TryVertex(x, y, -1, -1);
		}

	SYS_ASSERT(marked_cur_vert > 0);

	// build vertex array

	door_vertexes = marked_cur_vert;
	numvertexes = door_vertexes + num_doors * 2;
	max_vertexes = numvertexes + 0;
	vertexes = Z_ClearNew(vertex_t, max_vertexes);

	for (y=0; y <= WLFMAP_H; y++)
	for (x=0; x <= WLFMAP_W; x++)
	{
		int v_num = vertex_matrix[y * (WLFMAP_W + 1) + x];

		if (! v_num)
			continue;

		vertexes[v_num - 1].x = 64*(x);
		vertexes[v_num - 1].y = 64*(y);
	}

	// Note: door vertexes are created on-the-fly
}

void WF_ComputeLinedefData(line_t *ld)
{
	vertex_t *v1 = ld->v1;
	vertex_t *v2 = ld->v2;

	ld->dx = v2->x - v1->x;
	ld->dy = v2->y - v1->y;

	if (ld->dx == 0)
		ld->slopetype = ST_VERTICAL;
	else if (ld->dy == 0)
		ld->slopetype = ST_HORIZONTAL;
	else if (ld->dy / ld->dx > 0)
		ld->slopetype = ST_POSITIVE;
	else
		ld->slopetype = ST_NEGATIVE;

	ld->length = R_PointToDist(0, 0, ld->dx, ld->dy);

	if (v1->x < v2->x)
	{
		ld->bbox[BOXLEFT] = v1->x;
		ld->bbox[BOXRIGHT] = v2->x;
	}
	else
	{
		ld->bbox[BOXLEFT] = v2->x;
		ld->bbox[BOXRIGHT] = v1->x;
	}

	if (v1->y < v2->y)
	{
		ld->bbox[BOXBOTTOM] = v1->y;
		ld->bbox[BOXTOP] = v2->y;
	}
	else
	{
		ld->bbox[BOXBOTTOM] = v2->y;
		ld->bbox[BOXTOP] = v1->y;
	}
}

static void MakeOneSided(line_t *ld, side_t *sd, int f_sec,
		int special_kind, const char *texname)
{
	SYS_ASSERT(ld->v1 != ld->v2);

	ld->flags = ML_Blocking;

	WF_ComputeLinedefData(ld);

  if (special_kind > 0)
    ld->special = linetypes.Lookup(special_kind);

	ld->side[0] = sd;

	ld->side[0]->middle.image = W_ImageLookup(texname, INS_Texture);

	ld->side[0]->sector = sectors + (f_sec - 1);
	ld->frontsector = ld->side[0]->sector;

	ld->side[0]->middle.translucency = 1.0;
	ld->side[0]->middle.x_mat.x = 1;  ld->side[0]->middle.x_mat.y = 0;
	ld->side[0]->middle.y_mat.x = 0;  ld->side[0]->middle.y_mat.y = 1;

///---	P_ComputeGaps(ld);
}

static void MakeTwoSided(line_t *ld, side_t *sd, int f_sec, int b_sec,
		const char *texname)
{
	SYS_ASSERT(ld->v1 != ld->v2);

	ld->flags = ML_TwoSided;

	WF_ComputeLinedefData(ld);


	ld->side[0] = sd;

	ld->side[0]->middle.image = (texname == NULL) ? NULL :
		W_ImageLookup(texname, INS_Texture);

	ld->side[0]->sector = sectors + (f_sec - 1);

	ld->side[0]->middle.translucency = 1.0;
	ld->side[0]->middle.x_mat.x = 1;  ld->side[0]->middle.x_mat.y = 0;
	ld->side[0]->middle.y_mat.x = 0;  ld->side[0]->middle.y_mat.y = 1;

	ld->side[1] = sd + 1;

	ld->side[1]->middle.image = ld->side[0]->middle.image;

	ld->side[1]->sector = sectors + (b_sec - 1);

	ld->side[1]->middle.translucency = 1.0;
	ld->side[1]->middle.x_mat.x = 1;  ld->side[1]->middle.x_mat.y = 0;
	ld->side[1]->middle.y_mat.x = 0;  ld->side[1]->middle.y_mat.y = 1;

	ld->frontsector = ld->side[0]->sector;
	ld->backsector  = ld->side[1]->sector;

///---	P_ComputeGaps(ld);
}

static void AddLinedef(int *l_index, int *s_index,
		int sx, int sy, int ex, int ey, int f_sec, int b_sec, 
		int f_door, int f_dx, int f_dy, bool count_only)
{
	line_t *ld = NULL;
	side_t *sd = NULL;

	bool door_jam;
	bool door;

	int vertical = (sx == ex) ? 1 : 0;

	if (! count_only)
	{
		SYS_ASSERT(0 <= (*l_index) && (*l_index) < numlines);
		SYS_ASSERT(0 <= (*s_index) && (*s_index) < numsides);

		ld = lines + (*l_index);
		sd = sides + (*s_index);
	}

	SYS_ASSERT(f_sec > 0);
	SYS_ASSERT(b_sec >= 0);

	SYS_ASSERT(0 <= sx && sx <= WLFMAP_W);
	SYS_ASSERT(0 <= sy && sy <= WLFMAP_H);
	SYS_ASSERT(0 <= ex && ex <= WLFMAP_W);
	SYS_ASSERT(0 <= ey && ey <= WLFMAP_H);

	(*l_index)++;
	(*s_index) += (b_sec > 0 && f_sec > 0) ? 2 : 1;

	// check for door jam (must be one-sided line)
	door_jam = (f_door > 0 && b_sec == 0 && (f_door & 1) != vertical);
	if (door_jam)
	{
		(*l_index)++;
		(*s_index)++;
	}

	// check for door itself
	door = (f_door > 0 && f_sec > 0 && (f_door & 1) == vertical &&
			ex - sx >= 0 && ey - sy >= 0);
	if (door)
	{
		(*l_index)++;
		(*s_index) += 2;
	}

	if (count_only)
		return;

	// handle the door jam
	if (door_jam)
	{
		int which = (ex - sx <= 0 && ey - sy >= 0) ? 0 : 1;

		ld[0].v1 = WF_GetVertex(sx, sy);
		ld[0].v2 = GetDoorVertex(f_dx, f_dy, which);

		ld[1].v1 = ld[0].v2;
		ld[1].v2 = WF_GetVertex(ex, ey);

		MakeOneSided(ld+0, sd+0, f_sec, 0, vertical ? "DOORJAMV" : "DOORJAMH");
		MakeOneSided(ld+1, sd+1, f_sec, 0, vertical ? "DOORJAMV" : "DOORJAMH");

		// door jam overrides
		return;
	}

	// handle the door itself
	if (door)
	{
		if (vertical)
		{
			ld->v1 = GetDoorVertex(f_dx, f_dy, 0);
			ld->v2 = GetDoorVertex(f_dx, f_dy, 1);
		}
		else
		{
			ld->v2 = GetDoorVertex(f_dx, f_dy, 0);
			ld->v1 = GetDoorVertex(f_dx, f_dy, 1);
		}

    int door_kind = (f_door >> 1) & 7;

		ld->special = linetypes.Lookup(10+door_kind);

		if (door_kind == 5)
			MakeTwoSided(ld, sd, f_sec, f_sec, vertical ? "ELVDOORV" : "ELVDOORH");
		else if (door_kind == 1 || door_kind == 2)
			MakeTwoSided(ld, sd, f_sec, f_sec, vertical ? "LCKDOORV" : "LCKDOORH");
		else
			MakeTwoSided(ld, sd, f_sec, f_sec, vertical ? "DOORV" : "DOORH");

		ld++;
		sd += 2;
	}

	// determine vertices

	ld->v1 = WF_GetVertex(sx, sy);
	ld->v2 = WF_GetVertex(ex, ey);

	if (b_sec != 0)
	{
		MakeTwoSided(ld, sd, f_sec, b_sec, NULL);
		return;
	}

	// determine texture on wall
	int back_kind;
  int front_kind;

	if (vertical)
	{
    back_kind  = (sx <= 0) ? -1 : wlf_map_tiles[M_INDEX(sx-1, sy)];
		front_kind = (sx >= WLFMAP_W) ? -1 : wlf_map_tiles[M_INDEX(sx, ey)];
	}
	else
	{
    back_kind  = (sy <= 0) ? -1 : wlf_map_tiles[M_INDEX(ex, sy-1)];
    front_kind = (sy >= WLFMAP_H) ? -1 : wlf_map_tiles[M_INDEX(sx, sy)];
	}

  if (ex > sx || ey < sy)
  {
    int tmp = back_kind;
    back_kind = front_kind;
    front_kind = tmp;
  }

	if (back_kind < 1 || back_kind > 49)
		MakeOneSided(ld, sd, f_sec, 0, "WEIRD");
	else
  {
    char texname[32];
    sprintf(texname, "WALL%03d%c", back_kind, vertical ? 'V' : 'H');

    int special = 0;

    if (back_kind == TILE_ELEVATOR)
      special = (front_kind == TILE_SECRET_ELEV) ? 3 : 2;

		MakeOneSided(ld, sd, f_sec, special, texname);
  }
}

//
// BuildLines
//
// Also builds the seg matrix.
//
static void BuildLines(bool count_only)
{
	int l_index = 0;
	int s_index = 0;

	int x, y;
	int f_sec,  b_sec;
	int f_door, b_door;

	// first check the horizontal lines
	for (y=0; y <= WLFMAP_H; y++)
	{
		for (x=0; x < WLFMAP_W; x++)
		{
			f_sec = (y <= 0) ? 0 : sector_matrix[M_INDEX(x,y-1)];
			b_sec = (y >= WLFMAP_H) ? 0 : sector_matrix[M_INDEX(x,y)];

			if (f_sec == 0 && b_sec == 0)
				continue;

			if (f_sec == b_sec)
			{
				wlf_seg_matrix[MP1_INDEX(x,y) * 2 + 0] = f_sec;
				continue;
			}

			wlf_seg_matrix[MP1_INDEX(x,y) * 2 + 0] = -2;

			f_door = (y <= 0) ? 0 : door_matrix[M_INDEX(x,y-1)];
			b_door = (y >= WLFMAP_H) ? 0 : door_matrix[M_INDEX(x,y)];

			if (f_sec == 0)
				AddLinedef(&l_index, &s_index, x+1, y, x, y, b_sec, f_sec, 
						b_door, x, y, count_only);
			else
				AddLinedef(&l_index, &s_index, x, y, x+1, y, f_sec, b_sec, 
						f_door, x, y-1, count_only);
		}
	}

	// second check the vertical lines
	for (x=0; x <= WLFMAP_W; x++)
	{
		for (y=0; y < WLFMAP_H; y++)
		{
			f_sec = (x >= WLFMAP_W) ? 0 : sector_matrix[M_INDEX(x,y)];
			b_sec = (x <= 0) ? 0 : sector_matrix[M_INDEX(x-1,y)];

			if (f_sec == 0 && b_sec == 0)
				continue;

			if (f_sec == b_sec)
			{
				wlf_seg_matrix[MP1_INDEX(x,y) * 2 + 1] = f_sec;
				continue;
			}

			wlf_seg_matrix[MP1_INDEX(x,y) * 2 + 1] = -2;

			f_door = (x >= WLFMAP_W) ? 0 : door_matrix[M_INDEX(x,y)];
			b_door = (x <= 0) ? 0 : door_matrix[M_INDEX(x-1,y)];

			if (f_sec == 0)
				AddLinedef(&l_index, &s_index, x, y+1, x, y, b_sec, f_sec, 
						b_door, x-1, y, count_only);
			else
				AddLinedef(&l_index, &s_index, x, y, x, y+1, f_sec, b_sec, 
						f_door, x, y, count_only);
		}
	}

	if (! count_only)
		return;

	// setup linedef & sidedef arrays

	numlines = l_index;
	max_lines = numlines + 0;
	lines = Z_ClearNew(line_t, max_lines);

	numsides = s_index;
	max_sides = numsides + 0;
	sides = Z_ClearNew(side_t, max_sides);

}

//
// AnalyseMap
//
// Determine from the tile data where all the sectors, lines, etc...
// need to be put.
//
static void AnalyseMap(void)
{
	int pl_x = 0;
  int pl_y = 0;

	int ax, ay;

	// first find the player.  This location will serve as the start for
	// analysing the map, looking for boundaries.  Each boundary becomes
	// a linedef, and each enclosed space becomes a sector.
	//
	// Boundaries are hard or soft.  Hard boundaries occur on solid
	// walls and at the outer edges of the map -- these become one-sided
	// linedefs.  Soft boundaries occur at a door or a pushable wall --
	// these become two-sided linedefs.

	FindPlayer(&pl_x, &pl_y);

	// allocate the tile marking matrix, which keeps track of which
	// tiles have been processed.

	sector_matrix = Z_ClearNew(short, WLFMAP_W * WLFMAP_H);
	marked_cur_sec = 0;

	vertex_matrix = Z_ClearNew(short, (WLFMAP_W + 1) *
			(WLFMAP_H + 1));
	marked_cur_vert = 0;

	{
		int numelem = (WLFMAP_W + 1) * (WLFMAP_H + 1) * 2;
		wlf_seg_matrix = Z_New(short, numelem);
		memset(wlf_seg_matrix, -1, numelem * sizeof(short));
	}

	door_matrix = Z_ClearNew(ushort, WLFMAP_W * WLFMAP_H);
	num_doors = 0;

	// initialise analysis point list, which keeps track of tiles where
	// a new analysis should begin.  Think: flood fill.

	analysis_points = NULL;
	num_analysis_points = 0;
	max_analysis_points = 0;

	AddAnalysisPoint(pl_x, pl_y, pl_x, pl_y);

	while (num_analysis_points > 0)
	{
		// pop the last point of the stack
		num_analysis_points--;

		ax = analysis_points[num_analysis_points].x;
		ay = analysis_points[num_analysis_points].y;

		// check if already processed
		if (sector_matrix[M_INDEX(ax,ay)] > 0)
			continue;

		// create new sector number
		marked_cur_sec++;

		FillMapArea(ax, ay, marked_cur_sec);
	}

	BuildSectors();
	BuildVertices();

	BuildLines(true);
	BuildLines(false);

	I_Printf("WOLF: Sector count: %d\n", marked_cur_sec);
	I_Printf("WOLF: Vertex count: %d\n", marked_cur_vert);
	I_Printf("WOLF: Linedef count: %d\n", numlines);
	I_Printf("WOLF: Sidedef count: %d\n", numsides);

	WF_BuildBSP();

///---	SetupRootNode();
	
	// free stuff
	if (analysis_points)
		Z_Free(analysis_points);

	Z_Free(sector_matrix);
	Z_Free(vertex_matrix);
	Z_Free(wlf_seg_matrix);
}


//----------------------------------------------------------------------------

//
// LookupWolfObject
//
// Convert the object values into numbers that can be looked-up in
// DDF, taking into account the all-too-ad-hoc encodings of direction
// and skill within the object values.
//

static const mobjtype_c * LookupWolfObject(int obj, int *options, angle_t *angle)
{
	bool patrol = false;

	// the player
	if (OBJTILE_PL_STARTS <= obj && obj <= OBJTILE_PL_STARTS+3)
	{
		*angle = FLOAT_2_ANG(((obj - OBJTILE_PL_STARTS) ^ 1) * 90);

		obj = OBJTILE_PL_STARTS;
	}

	// static items
	if (23 <= obj && obj <= 97)
	{ }
  else
  {
    // these only occur on hard difficulty
    if ((180 <= obj && obj <= 195) ||
        (198 <= obj && obj <= 213))
    {
      (*options) &= ~(MTF_NORMAL | MTF_EASY);
      obj -= 72;
    }
    else if (252 <= obj && obj <= 259)
    {
      (*options) &= ~(MTF_NORMAL | MTF_EASY);
      obj -= 36;
    }

    // these only occur on medium difficulty or higher
    if ((144 <= obj && obj <= 159) || (162 <= obj && obj <= 177))
    {
      (*options) &= ~MTF_EASY;
      obj -= 36;
    }
    else if ((234 <= obj && obj <= 241))
    {
      (*options) &= ~MTF_EASY;
      obj -= 18;
    }

    // these enemies are patrolling versions
    if ((112 <= obj && obj <= 115) ||
        (120 <= obj && obj <= 123) || 
        (130 <= obj && obj <= 133) ||
//!!!   (138 <= obj && obj <= 141) ||
        (220 <= obj && obj <= 223))
    {
      patrol = true;
      obj -= 4;
    }

    // enemies that have direction
    if ((108 <= obj && obj <= 123) ||
        (216 <= obj && obj <= 223))
    {
      *angle = FLOAT_2_ANG(((obj & 3) ^ 0) * 90);
      
      obj &= ~3;
    }
    else if ((126 <= obj && obj <= 141))
    {
      *angle = ((obj & 3) ^ 1) * 90;

      obj -= (obj - 126) & 3;
    }
  }

////  if (patrol)
////    (*options) |= MTF_MEANDER;

L_WriteDebug("LOOKING FOR OBJ: %d\n", obj);
  return mobjtypes.Lookup(obj);
}


//
// AnalyseObjects
//
// Determine from the tile data where all the objects need to be
// spawned.
//
static void AnalyseObjects(void)
{
	int x, y;

	for (y=0; y < WLFMAP_H; y++)
  for (x=0; x < WLFMAP_W; x++)
  {
    int tile = wlf_map_tiles[M_INDEX(x,y)];
    int obj  = wlf_obj_tiles[M_INDEX(x,y)];

    if (tile < MAPTILE_AMBUSH || obj == 0 ||
        obj == OBJTILE_PUSHABLE)
    {
      continue;
    }

    angle_t angle = 0;

    int options = MTF_EASY | MTF_NORMAL | MTF_HARD;

    const mobjtype_c *info = LookupWolfObject(obj, &options, &angle);

    if (! info)
    {
			if (!no_warnings)
				I_Warning("Unknown thing type %i at block (%d,%d)\n", obj, x, y);
      continue;
    }

    float tx = 64*(x) + 32;
    float ty = 64*(y) + 32;

    if (tile == MAPTILE_AMBUSH)
      options |= MTF_AMBUSH;

    P_SpawnMapThing(info, tx, ty, ONFLOORZ, angle, options, 0);
  }
}


static void DumpMap(int mode)
{
	int x, y;
	char linebuf[2050];
	char numbuf[10];

	L_WriteDebug("\n");

	for (y=WLFMAP_H-1; y >= 0; y--)
	{
		linebuf[0] = 0;

		for (x=0; x < WLFMAP_W; x++)
		{
			int tile = wlf_map_tiles[M_INDEX(x,y)];
			int obj  = wlf_obj_tiles[M_INDEX(x,y)];

			if (mode == 0)
			{
				sprintf(numbuf, "% 4d", tile);
				strcat(linebuf, numbuf);
				continue;
			}

			if (tile < MAPTILE_DOORS && obj == OBJTILE_PUSHABLE)
			{
				strcat(linebuf, "@@@@");
				continue;
			}
			else if (tile < MAPTILE_DOORS)
			{
				strcat(linebuf, "####");
				continue;
			}
			else if (tile < MAPTILE_AMBUSH)
			{
				strcat(linebuf, "////");
				continue;
			}
			else if (obj == 0)
			{
				strcat(linebuf, "    ");
				continue;
			}

			sprintf(numbuf, "% 4d", obj);
			strcat(linebuf, numbuf);
		}

		L_WriteDebug("WLFDUMP: [%s]\n", linebuf);
	}
}


static void DoReject(void)
{
	rejectmatrix = NULL;
}

static void DoBlockMap(void)
{
	int count;

	GenerateBlockMap(0, 0,
			64*(WLFMAP_W), 64*(WLFMAP_H));

	// clear out mobj chains
	count = bmapwidth * bmapheight;

	blocklinks  = Z_ClearNew(mobj_t *, count);
	blocklights = Z_ClearNew(mobj_t *, count);
}

//
// WF_SetupLevel
//
// Sets up the current level using the skill passed and the
// information in currentmap.
//
void WF_SetupLevel(skill_t skill)
{
	if (level_active)
		ShutdownLevel();

	// -ACB- 1998/08/27 NULL the head pointers for the linked lists....
	itemquehead = NULL;
	mobjlisthead = NULL;

	totalkills = totalitems = totalsecret = wminfo.maxfrags = 0;

	wminfo.partime = 90; /// currmap->partime;

	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];
		if (! p) continue;

		p->killcount = p->secretcount = p->itemcount = 0;
		p->mo = NULL;
	}

	// Initial height of PointOfView
	// will be set by player think.
	players[consoleplayer]->viewz = FLO_UNUSED;

	leveltime = 0;

	numextrafloors = 0;
	numwalltiles = 0;
	numvertgaps = 0;

// <-----  COMMON TO P_SetupLevel


//!!!!
int mapnum;
const char *s=M_GetParm("-wolfmap");
if (s) mapnum = atoi(s) - 1;
else mapnum = 0;

  // read in stuff from files
  cur_map_num = mapnum;
  WF_LoadMap(cur_map_num);

  // initialise lists
  numlines = 0;       lines = NULL;
  numsectors = 0;     sectors = NULL;
  numsides = 0;       sides = NULL;
  numvertexes = 0;    vertexes = NULL;

  // Ugh, this ain't very pretty, but it works
  max_segs = WLFMAP_W * WLFMAP_H * 2;

  segs = Z_New(seg_t, max_segs);
  numsegs = 0;

  nodes = Z_New(node_t, max_segs);
  numnodes = 0;

  subsectors = Z_New(subsector_t, max_segs);
  numsubsectors = 0;
 
//!!!!
DumpMap(0);
DumpMap(1);
// I_Error("DONE");

  AnalyseMap();

  DoReject();
  DoBlockMap();

///  WF_FreeMap();


#if 0
  // if deathmatch, randomly spawn the active players
  if (deathmatch)
  {
    for (p = players; p; p = p->next)
    {
      if (p->in_game)
      {
        p->mo = NULL;
        G_DeathMatchSpawnPlayer(p);
        if (p->level_init)
          p->level_init(p, p->data);
      }
    }
  }
#endif


// ----->  COMMON TO P_SetupLevel

	SetupExtrafloors();
	SetupWallTiles();
	SetupVertGaps();

	GroupLines();
	{
		int j;
		for (j=0; j < numsectors; j++)
			P_RecomputeTilesInSector(sectors + j);
	}

	R_ComputeSkyHeights();

	// compute sector and line gaps
	for (int j=0; j < numsectors; j++)
		P_RecomputeGapsAroundSector(sectors + j);

	bodyqueslot = 0;

	// -AJA- 1999/10/21: Clear out player starts (ready to load).
	dm_starts.Clear();
	coop_starts.Clear();
	voodoo_doll_starts.Clear();


// ---!!--- difference from P_SetupLevel
  AnalyseObjects();


	// set up world state
	P_SpawnSpecials(0);

	RGL_UpdateSkyBoxTextures();

	// preload graphics
	if (precache)
		R_PrecacheLevel();

	// setup categories based on game mode (SP/COOP/DM)
	S_ChangeChannelNum();

	// FIXME: cache sounds (esp. for player)

///!!!	S_ChangeMusic(currmap->music, true); // start level music

	level_active = true;
}

