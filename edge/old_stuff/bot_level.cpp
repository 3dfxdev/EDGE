//----------------------------------------------------------------------------
//  BOT : Level analysis
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2005  The EDGE Team.
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

#include "i_defs.h"
#include "bot_level.h"
#include "bot_path.h"

#include "m_bbox.h"
#include "p_local.h"
#include "p_mobj.h"
#include "r_main.h"
#include "r_state.h"
#include "z_zone.h"

#define ALLOW_JUMP    (false)  // (level_flags.jumping)
#define ALLOW_CROUCH  (false)


///--- bool BOT_LineSelfActivated(line_t *ld);


typedef struct
{
	float x1, y1, x2, y2;

public:
	inline bool PointInside(float px, float py) const
	{
		divline_t div;

		div.x  = x1;
		div.y  = y1;
		div.dx = x2 - x1;
		div.dy = y2 - y1;

		return (P_PointOnDivlineSide(px, py, &div) == 0);
	}

	inline bool IntersectLine(float lx1, float ly1, float lx2, float ly2,
		float *px, float *py) const
	{
		float denom = (lx2-lx1) * (y2-y1) - (ly2-ly1) * (x2-x1);
		float frac  = ( x1-lx1) * (y2-y1) - ( y1-ly1) * (x2-x1); 

		// parallel ?
		if (fabs(denom) < 0.0001f)
		{
			*px = x2;
			*py = y2;

			return false;
		}
		             
		frac /= denom;

		*px = lx1 + frac * (lx2 - lx1);
		*py = ly1 + frac * (ly2 - ly1);

		if (frac < 0 || frac > 1)
			return false;

		return true;
	}

	inline bool BoxOnSide(const float *tmbox) const
	{
		divline_t div;

		div.x  = x1;
		div.y  = y1;
		div.dx = x2 - x1;
		div.dy = y2 - y1;

		return P_BoxOnDivLineSide(tmbox, &div);
	}
}
halfplane_t;


bool BOT_ThingIsAgent(mobj_t *mo)
{
	return (mo->player || (mo->extendedflags & EF_MONSTER));
}

bool BOT_ThingIsMissile(mobj_t *mo)
{
	return (mo->flags & MF_MISSILE) ? true : false;
}

bool BOT_ThingIsPickup(mobj_t *mo)
{
	return (mo->flags & MF_SPECIAL) ? true : false;
}

// Note: exact match on type
bool BOT_ThingMatchesKey(mobj_t *mo, keys_e type)
{
///---	if (! BOT_ThingIsPickup(mo))
///---		return false;

	for (benefit_t *BF = mo->info->pickup_benefits; BF; BF = BF->next)
	{
		if (BF->type == BENEFIT_Key)
		{
			if ((type & (keys_e) BF->sub.type) != KF_NONE)
				return true;
		}
	}

	return false;
}

//
// BOT_ThingIsBlocker
//
// Where "blocker" means a static, non-moving object
// (for example, lamps or trees).  
//
bool BOT_ThingIsBlocker(mobj_t *mo)
{
	if (! (mo->flags & MF_SOLID))
		return false;

	if (BOT_ThingIsAgent(mo))
		return false;
	
	if (BOT_ThingIsMissile(mo))
		return false;
	
	if (BOT_ThingIsPickup(mo))
		return false;

	return true;
}

bool LinetypeOpensSector(const linetype_c *special)
{
	if (special->f.type != mov_undefined)
		return true;

	if (special->c.type != mov_undefined)
		return true;

	return false;
}

//----------------------------------------------------------------------------

// true if any part of the bounding boxes overlap
static INLINE bool TravCheckBBox(const float *test, const float *bbox)
{
	return (test[BOXRIGHT]  < bbox[BOXLEFT] ||
			test[BOXLEFT]   > bbox[BOXRIGHT] ||
			test[BOXTOP]    < bbox[BOXBOTTOM] ||
			test[BOXBOTTOM] > bbox[BOXTOP]) ? false : true;
}

static bool DoTraverseBSP(unsigned int bspnum, const float *bbox,
		traverse_bsp_base_c *trav_info)
{
	// just a normal node ?
	if (! (bspnum & NF_V5_SUBSECTOR))
	{
		node_t *node = nodes + bspnum;

		// recursively check the children nodes
		int side = P_BoxOnDivLineSide(bbox, &node->div);

		if (side < 0 || side == 0)  // try the front space
		{
			if (! DoTraverseBSP(node->children[0], bbox, trav_info))
				return false;
		}

		if (side < 0 || side == 1)  // try the back space
		{
			if (! DoTraverseBSP(node->children[1], bbox, trav_info))
				return false;
		}

		return true;
	}

	// the sharp end: check all things & linedefs in the subsector
	subsector_t *sub = subsectors + (bspnum & ~NF_V5_SUBSECTOR);

	for (mobj_t *mo = sub->thinglist; mo; mo = mo->snext)
	{
		if (mo->x + mo->radius < bbox[BOXLEFT] ||
			mo->x - mo->radius > bbox[BOXRIGHT] ||
			mo->y + mo->radius < bbox[BOXBOTTOM] ||
			mo->y - mo->radius > bbox[BOXTOP])
		{
			continue;
		}

		if (! trav_info->GotThing(mo, sub))
			return false;
	}

	for (seg_t *seg = sub->segs; seg; seg = seg->sub_next)
	{
		if (! seg->linedef)
			continue;

		if (! TravCheckBBox(seg->linedef->bbox, bbox))
			continue;

		if (! trav_info->GotLine(seg->linedef, sub))
			return false;
	}

	return true;
}

//
// BOT_TraverseBoxBSP
//
// Iterate over all lines and things that touch a certain rectangle on
// the map, using the BSP tree.
//
// If any function returns false, then this routine returns false and
// nothing else is checked.  Otherwise true is returned.
//
// (FIXME: code was copied from p_maputl, merge them, have flags for lines/things).
//
bool BOT_TraverseBoxBSP(float lx, float ly, float hx, float hy,
	traverse_bsp_base_c *trav_info)
{
	float bbox[4];

	bbox[BOXLEFT]   = lx;
	bbox[BOXBOTTOM] = ly;
	bbox[BOXRIGHT]  = hx;
	bbox[BOXTOP]    = hy;

	return DoTraverseBSP(root_node, bbox, trav_info);
}

//----------------------------------------------------------------------------

#if 0 // NOTE: this stuff DOES NOT WORK !!!!
class trav_vect_test_c : public traverse_bsp_base_c
{
private:
	float sx, sy;
	float ex, ey;

	float R;
	halfplane_t hps[6];
	int num_hps;

public:

	trav_vect_test_c(float _sx, float _sy, float _ex, float _ey, 
		float _R, float *bbox) :
			traverse_bsp_base_c(),
			sx(_sx), sy(_sy), ex(_ex), ey(_ey), R(_R)
	{
		// compute the halfplanes making up the convex hull that
		// covers this traversal.
		// for near vertical or near horizontal vectors, we get an
		// optimisation (only need 4 halfplanes instead of 6).

		float lx = bbox[BOXLEFT];
		float ly = bbox[BOXBOTTOM];
		float hx = bbox[BOXRIGHT];
		float hy = bbox[BOXTOP];

		hps[0].x1 = lx; hps[0].y1 = ly; hps[0].x2 = lx; hps[0].y2 = hy;
		hps[1].x1 = lx; hps[1].y1 = hy; hps[1].x2 = hx; hps[1].y2 = hy;
		hps[2].x1 = hx; hps[2].y1 = hy; hps[2].x2 = hx; hps[2].y2 = ly;
		hps[3].x1 = hx; hps[3].y1 = ly; hps[3].x2 = lx; hps[3].y2 = ly;

		if (fabs(sx - ex) < 1 || fabs(sy - ey) < 1)
		{
			num_hps = 4;
		}
		else if ((ex > sx && ey > sy) || (ex < sx && ey < sy))
		{
			num_hps = 6;

			hps[4].x1 = lx;
			hps[4].y1 = MIN(sy, ey) + R;
			hps[4].x2 = MAX(sx, ex) - R;
			hps[4].y2 = hy;

			hps[5].x1 = hx;
			hps[5].y1 = MAX(sy, ey) - R;
			hps[5].x2 = MIN(sx, ex) + R;
			hps[5].y2 = ly;
		}
		else
		{
			num_hps = 6;

			hps[4].x1 = MAX(sx, ex) - R;
			hps[4].y1 = ly;
			hps[4].x2 = lx;
			hps[4].y2 = MAX(sy, ey) - R;

			hps[5].x1 = MIN(sx, ex) + R;
			hps[5].y1 = hy;
			hps[5].x2 = hx;
			hps[5].y2 = MIN(sy, ey) + R;
		}
	}

	virtual ~trav_vect_test_c()
	{ }

	virtual bool GotLine(line_t *ld,  subsector_t *sub)
	{
		// check line against all half-planes.  When both end-points
		// are outside of a half-plane, we know we can ignore this
		// line (no intersection).  If the line crosses the half-plane,
		// we must clip it.

		float x1 = ld->v1->x;
		float y1 = ld->v1->y;
		float x2 = ld->v2->x;
		float y2 = ld->v2->y;

		for (int hp_idx = 0; hp_idx < num_hps; hp_idx++)
		{
			halfplane_t& H = hps[hp_idx];

			bool ins_1 = H.PointInside(x1, y1);
			bool ins_2 = H.PointInside(x2, y2);

			if (! ins_1 && ! ins_2)
				return true;

			if (ins_1 && ins_2)
				continue;

			// clip the line to the half-plane
			float nx, ny;

			if (H.IntersectLine(x1,y1, x2,y2, &nx, &ny))
			{
				if (ins_1)
					x2 = nx, y2 = ny;
				else
					x1 = nx, y1 = ny;
			}
#if 1
			else
				L_WriteDebug("WARNING: intersect line failed.\n");
#endif
		}

		// made it through the half-plane tests, hence line
		// must intersect the sweep area.

		return (BOT_LineIsBlocker(ld, false, KF_NONE) == 0);
	}

	virtual bool GotThing(mobj_t *mo, subsector_t *sub)
	{
		// thing already checked against BBOX.  Hence only need
		// to check against the diagonal edges of the sweep area.
		if (num_hps == 6)
		{
			float tmp_box[4];

			tmp_box[BOXLEFT]   = mo->x - mo->radius;
			tmp_box[BOXRIGHT]  = mo->x + mo->radius;
			tmp_box[BOXBOTTOM] = mo->y - mo->radius;
			tmp_box[BOXTOP]    = mo->y + mo->radius;

			if (hps[4].BoxOnSide(tmp_box) == 1 ||
			    hps[5].BoxOnSide(tmp_box) == 1)
			{
				return true; // ignore this thing
			}
		}

		return ! BOT_ThingIsBlocker(mo);
	}
};

// FIXME: this stuff DOES NOT WORK !!!!
bool BOT_CanTravelVector(float sx, float sy, float ex, float ey)
{
	float R = 16.0f;

	// compute bounding box
	float lx = MIN(sx, ex) - R;
	float ly = MAX(sx, ex) + R;
	float hx = MIN(sy, ey) - R;
	float hy = MAX(sy, ey) + R;

	float bbox[4];

	bbox[BOXLEFT]   = lx;
	bbox[BOXRIGHT]  = ly;
	bbox[BOXBOTTOM] = hx;
	bbox[BOXTOP]    = hy;

	trav_vect_test_c TV_test(sx, sy, ex, ey, R, bbox);

	return DoTraverseBSP(root_node, bbox, &TV_test);
}
#endif

//----------------------------------------------------------------------------

bot_achievements_c::bot_achievements_c() : owned_keys(KF_NONE),
	sectors_opened(), tags_opened()
{
}

bot_achievements_c::~bot_achievements_c()
{
}

void bot_achievements_c::Clear()
{
	owned_keys = KF_NONE;

	sectors_opened.Clear();
	tags_opened.Clear();
}

void bot_achievements_c::AddKeys(keys_e K)
{
	owned_keys = (keys_e)(owned_keys | K);
}

void bot_achievements_c::AddSector(int sec_num)
{
	// Note: we DON'T check if already present
	sectors_opened.Insert(sec_num);
}

void bot_achievements_c::AddTag(int tag_num)
{
	// Note: we DON'T check if already present
	tags_opened.Insert(tag_num);
}

bool bot_achievements_c::HasKey(keys_e K) const
{
	keys_e req   = (keys_e)(K & KF_MASK);
	keys_e cards = owned_keys;

	if (K & KF_BOOM_SKCK)
	{
		// Boom compatibility: treat card and skull types the same
		cards = (keys_e)(EXPAND_KEYS(cards));
	}

	if (K & KF_STRICTLY_ALL)
	{
		return ((cards & req) == req);
	}

	return ((cards & req) != KF_NONE);
}

bool bot_achievements_c::HasSector(int sec_num) const
{
	for (int i=0; i < sectors_opened.GetSize(); i++)
		if (sec_num == sectors_opened[i])
			return true;
	
	return false;
}

bool bot_achievements_c::HasTag(int tag_num) const
{
	for (int i=0; i < tags_opened.GetSize(); i++)
		if (tag_num == tags_opened[i])
			return true;
	
	return false;
}

bool bot_achievements_c::AllowLine(line_t *ld) const
{
#if 0
L_WriteDebug("ACH: AllowLine %d (%d/%d) = (%c/%c)\n", ld - lines,
ld->frontsector->tag,
ld->backsector ? ld->backsector->tag : -1,
HasTag(ld->frontsector->tag) ? 'Y' : 'n',
ld->backsector ? HasTag(ld->backsector->tag) ? 'Y' : 'n' : '-');
#endif

	if (HasSector(ld->frontsector - sectors))
		return true;

	if (ld->backsector && HasSector(ld->backsector - sectors))
		return true;

	if (ld->frontsector->tag > 0 && HasTag(ld->frontsector->tag))
		return true;

	if (ld->backsector && ld->backsector->tag > 0 && HasTag(ld->backsector->tag))
		return true;

	return false;
}

void bot_achievements_c::DebugDump(const char *preface)
{
	L_WriteDebug("%s keys=0x%x sectors=< ", preface, owned_keys);

	for (int s = 0; s < sectors_opened.GetSize(); s++)
		L_WriteDebug("%d ", sectors_opened[s]);
	
	L_WriteDebug("> tags=[ ");

	for (int t = 0; t < sectors_opened.GetSize(); t++)
		L_WriteDebug("%d ", tags_opened[t]);
	
	L_WriteDebug("]\n");
}

//----------------------------------------------------------------------------

bool BOT_LineSelfActivated(line_t *ld)
{
	DEV_ASSERT2(ld);

	const linetype_c *special = ld->special;

	if (! special)
		return false;

	if (! LinetypeOpensSector(special))
		return false;

	if ((special->type == line_pushable && ld->tag <= 0) ||
		(special->type == line_manual))
		return true;

	// check for walk-over-edge-to-activate lifts.  Also handles the
	// shoot-door in MAP18 (debatable whether to allow shoot here)

	if ((special->type != line_manual) && (ld->tag > 0))
	{
		if (ld->tag == ld->frontsector->tag)
			return true;

		if (ld->backsector && (ld->tag == ld->backsector->tag))
			return true;
	}

	return false;
}

bool BOT_SectorRemoteActivated(sector_t *sector, seg_t ** SEG)
{
	DEV_ASSERT2(sector);
	DEV_ASSERT2(SEG);

	(*SEG) = NULL;

	if (sector->tag <= 0)
		return false;

	// !!!! FIXME: HORRIBLY INEFFICIENT

	for (int s = 0; s < numsegs; s++)
	{
		if (segs[s].miniseg)
			continue;

		line_t *ld = segs[s].linedef;

		const linetype_c *special = ld->special;

		if (! special)
			continue;

		if (ld->tag != sector->tag)
			continue;

		// check actually opens door
		if (LinetypeOpensSector(special))
		{
			(*SEG) = segs + s;
			return true;
		}
	}

	return false;  // switch not found
}

//
// BOT_LineIsBlocker
//
// seg may be NULL, in which case the front_floor_h value is taken
// into account for height checking.  (When seg is non-null, front_floor_h
// is ignored).
// 
int BOT_LineIsBlocker(line_t *ld, seg_t *seg, float front_floor_h,
	bool allow_selfact, bot_achievements_c *Ach, obstacle_base_c ** OBST)
{
// !!!!!! @@@@@ OBSTACLE STUFF

	if (! ld->backsector)  // one-sided lines always block
		return 2;

	if (ld->flags & ML_Blocking)
		return 2;

	// FIXME: doesn't test step up too big, since we cannot tell here
	//        which way we are moving (we may be dropping off).

	float floor_max = MAX(ld->frontsector->f_h, ld->backsector->f_h);
	float ceil_min  = MIN(ld->frontsector->c_h, ld->backsector->c_h);

	if (ceil_min >= floor_max + 56.0f);
		return 0;

	if (Ach->AllowLine(ld))
		return 0;

	if (allow_selfact && BOT_LineSelfActivated(ld) &&
		ld->special && (ld->special->keys != KF_NONE) &&
		Ach && Ach->HasKey(ld->special->keys))
	{
		return 1;
	}

	return 2;
}

//----------------------------------------------------------------------------

typedef struct
{
	float bbox[4];
	seg_t *seg;
	subsector_t *sub;
	line_t *linedef;
	bot_achievements_c *Ach;
}
trav_seg_data_t;

static trav_seg_data_t trav_seg_D;

static bool PIT_BotCheckLine(line_t * ld)
{
/// if (BTL_seg == 159) L_WriteDebug("-- Checking linedef %d:\n", ld - lines);

	// the seg's linedef is checked by other means
	if (trav_seg_D.linedef == ld)
	{
		return true;
	}

	if (trav_seg_D.bbox[BOXRIGHT]  <= ld->bbox[BOXLEFT] ||
		trav_seg_D.bbox[BOXLEFT]   >= ld->bbox[BOXRIGHT] ||
		trav_seg_D.bbox[BOXTOP]    <= ld->bbox[BOXBOTTOM] ||
		trav_seg_D.bbox[BOXBOTTOM] >= ld->bbox[BOXTOP])
	{
		// no intersection with line
		return true;
	}

	if (P_BoxOnLineSide(trav_seg_D.bbox, ld) != -1)
	{
		return true;
	}

	// A line has been hit

	float front_floor_h = trav_seg_D.sub->sector->f_h;

	return (BOT_LineIsBlocker(ld, NULL, front_floor_h, true, trav_seg_D.Ach, NULL) == 0);
}

//
// BOT_CanTraverseSeg
//
// Checks if any linedefs (EXCEPT the seg's linedef) cause blockage.
// Only tests the middle point of the seg.  Made to handle narrow gaps
// between walls, e.g. blocking bars at start of MAP02.
//
bool BOT_CanTraverseSeg(seg_t *seg, int sub, float R, bot_achievements_c *Ach)
{
	trav_seg_D.seg = seg;
	trav_seg_D.sub = subsectors + sub;
	trav_seg_D.Ach = Ach;

/// if (BTL_seg == 159) L_WriteDebug("** BOT_CanTraverseSeg (%d)\n", BTL_seg);

	float x = (seg->v1->x + seg->v2->x) / 2.0f;
	float y = (seg->v1->y + seg->v2->y) / 2.0f;

	trav_seg_D.bbox[BOXLEFT]   = x - R;
	trav_seg_D.bbox[BOXRIGHT]  = x + R;
	trav_seg_D.bbox[BOXBOTTOM] = y - R;
	trav_seg_D.bbox[BOXTOP]    = y + R;

	trav_seg_D.linedef = seg->linedef;

	// check lines

	int xl = BLOCKMAP_GET_X(trav_seg_D.bbox[BOXLEFT]);
	int xh = BLOCKMAP_GET_X(trav_seg_D.bbox[BOXRIGHT]);
	int yl = BLOCKMAP_GET_Y(trav_seg_D.bbox[BOXBOTTOM]);
	int yh = BLOCKMAP_GET_Y(trav_seg_D.bbox[BOXTOP]);;

	validcount++;  // essential !!

/// if (BTL_seg == 159) L_WriteDebug("  block range (%d,%d) -> (%d,%d)\n", xl, yl, xh, yh);

	for (int bx = xl; bx <= xh; bx++)
		for (int by = yl; by <= yh; by++)
			if (! P_BlockLinesIterator(bx, by, PIT_BotCheckLine))
				return false;

/// if (BTL_seg == 159) L_WriteDebug("****> FINE.\n");

	return true;
}

bool BOT_CanTraverseSegUltra(seg_t *seg, int sub, float R,
	bot_achievements_c *Ach, obstacle_base_c ** OBST)
{
	DEV_ASSERT2(Ach);
	DEV_ASSERT2(OBST);

	*OBST = NULL;

	// ---- Step 1. CHECK ACTUAL SEG (LINEDEF) ----

	if (! seg->miniseg)
	{
		sector_t *front = seg->frontsector;
		sector_t *back  = seg->backsector;

		float floor_max = MAX(front->f_h, back->f_h);
		float ceil_min  = MIN(front->c_h, back->c_h);

		float step_up = ALLOW_JUMP ? 48.0f : 24.0f;

		// check for blocking
		if ((ceil_min < floor_max + 56) ||
			(back->f_h > front->f_h + step_up))
		{
			line_t *ld = seg->linedef;
			seg_t *rem;

			if (BOT_LineSelfActivated(ld))
			{
				DEV_ASSERT2(ld->special);

				if ((ld->special->keys != KF_NONE) &&
					! Ach->HasKey(ld->special->keys))
				{
					sector_obstacle_c *SO = new sector_obstacle_c(back - sectors, ld);
					*OBST = SO;
					SO->MakeAchieved(Ach);
					return false;
				}
				else
					Ach->AddSector(back - sectors);
			}
			else if (BOT_SectorRemoteActivated(back, &rem))
			{
				DEV_ASSERT2(rem);
				DEV_ASSERT2(rem->linedef);
				DEV_ASSERT2(rem->linedef->special);

				sector_obstacle_c *SO = new sector_obstacle_c(back - sectors, rem->linedef);
				*OBST = SO;
				SO->MakeAchieved(Ach);
				return false;
			}
			else if (! BOT_SectorRemoteActivated(front, &rem)) // WTF ???
			{
				return false;
			}
		}
	}

	// ---- Step 2. CHECK NEARBY LINEDEFS ----

	//!!!! FIXME
	if (! BOT_CanTraverseSeg(seg, sub, R, Ach))
		return false;

	return true;
}

//----------------------------------------------------------------------------

void bot_switch_target_c::DebugDump(const char *preface)
{
	L_WriteDebug("%s SWITCH(seg %d on line %d, sub %d in sector %d)\n",
		preface, sw_face_seg - segs,
		sw_face_seg->linedef ? (sw_face_seg->linedef - lines) : -1,
		sw_subsec, subsectors[sw_subsec].sector - sectors);
}

void bot_walk_target_c::DebugDump(const char *preface)
{
	L_WriteDebug("%s WALK(seg %d on line %d, sub %d -> %d, %s)\n",
		preface, wk_seg - segs,
		wk_seg->linedef ? (wk_seg->linedef - lines) : -1,
		wk_front_sub, wk_back_sub,
		wk_one_way ? "two-way" : "ONE-WAY");
}

void bot_item_target_c::DebugDump(const char *preface)
{
	L_WriteDebug("%s ITEM(key 0x%x @ (%1.0f,%1.0f), sub %d in sector %d)\n",
		preface, ky_type, ky_pos_x, ky_pos_y,
		ky_subsec, subsectors[ky_subsec].sector - sectors);
}

//----------------------------------------------------------------------------

bool bot_switch_target_c::SignificantChange() const
{
	// once-only activation
	return (sw_face_seg->linedef && sw_face_seg->linedef->special &&
			sw_face_seg->linedef->special->count > 0);
}

void bot_switch_target_c::GetPressLocation(float *x, float *y, angle_t *ang)
{
	float mx = (sw_face_seg->v1->x + sw_face_seg->v2->x) / 2.0f;
	float my = (sw_face_seg->v1->y + sw_face_seg->v2->y) / 2.0f;

	(*ang) = sw_face_seg->angle + ANG90;

	(*x) = mx - 24.0f * M_Cos(*ang);
	(*y) = my - 24.0f * M_Sin(*ang);
}

bool bot_walk_target_c::SignificantChange() const
{
	return (wk_seg->linedef && wk_seg->linedef->special &&
			wk_seg->linedef->special->count > 0);
}

void bot_walk_target_c::GetPressLocation(float *x, float *y, angle_t *ang)
{
	float mx = (wk_seg->v1->x + wk_seg->v2->x) / 2.0f;
	float my = (wk_seg->v1->y + wk_seg->v2->y) / 2.0f;

	(*ang) = wk_seg->angle + ANG90;

	(*x) = mx - 24.0f * M_Cos(*ang);
	(*y) = my - 24.0f * M_Sin(*ang);
}

void bot_item_target_c::GetPressLocation(float *x, float *y, angle_t *ang)
{
	(*x) = ky_pos_x;
	(*y) = ky_pos_y;

	(*ang) = 0;  // doesn't matter
}


//----------------------------------------------------------------------------

#if (0 == 1)
//
// BOT_CanPassLine
//
// Called for two-sided lines that cannot be trivially passed by
// the path finder.  E.g. Doors, platforms, etc.  If false and
// the 'obst_list' parameter is not NULL, a suitable obstacle is
// added to that list (unless already present).
//
bool BOT_CanPassLine(seg_t *seg, int front_sub,
	bot_goal_c *achieved_goals, bot_goal_c **obst_list)
{
	DEV_ASSERT2(seg->linedef);

	const linetype_c *special = seg->linedef->special;

	const sector_t *front = seg->frontsector;
	const sector_t *back  = seg->backsector;

	bool switched = false;

	if (BOT_LineSelfActivated(seg->linedef))
	{
		DEV_ASSERT2(special);

		if (! (special->obj & trig_player))
		{
			L_WriteDebug("OBS-Line %d (sector %d) : self-act (MONSTER ONLY)\n",
				seg->linedef - lines, back - sectors);
			return false;
		}

		// check for keys
		if (special->keys == KF_NONE)
		{
			L_WriteDebug("OBS-Line %d (sector %d) : self-act (normal)\n",
				seg->linedef - lines, back - sectors);
			return true;
		}

		L_WriteDebug("OBS-Line %d (sector %d) : self-act (NEED KEY 0x%x)\n",
			seg->linedef - lines, back - sectors, special->keys);

		// NOTE: assumes a single key

		if (HaveAchievedKey(achieved_goals, special->keys))
			return true;
	}
	else if (back->tag > 0)
	{
		// requires a switch.  Look for it.

		// FIXME: tag may not imply movable, or move wrong end, or closer

		L_WriteDebug("OBS-Line %d (sector %d) : SWITCHED (tag %d)\n",
			seg->linedef - lines, back - sectors, back->tag);
		
		switched = true;
	}
	else
	{
		// ordinary line
		L_WriteDebug("OBS-Line %d : ORDINARY\n", seg->linedef - lines);
		return false;
	}

	// already there ?
	if (obst_list)
	{
		for (bot_goal_c *tmp = *obst_list; tmp; tmp = tmp->goal_next)
		{
			if (tmp->type != bot_goal_c::TYP_Obstacle)
				continue;

			if (segs[tmp->ob_seg].linedef == seg->linedef)
				return false;

			// not sure about this!!
			if (segs[tmp->ob_seg].backsector  == seg->backsector &&
			    segs[tmp->ob_seg].frontsector == seg->frontsector)
				return false;
		}

		bot_goal_c *g = bot_goal_c::NewObstacle(seg - segs, front_sub, switched);

		g->goal_next = *obst_list;
		*obst_list = g;
	}

	return false;
}

//
// BOT_CheckNewSwitch
//
// Check that this line is a switch.
// If it is a new switch (i.e. not in the list) AND capable of being
// activated (e.g. the keys we need are in the achieved_goals), then
// add it to the switch list.
// Returns true if added.
//
bool BOT_CheckNewSwitch(seg_t *seg, int front_sub,
		bot_goal_c *achieved_goals, bot_goal_c **swth_list)
{
	if (! seg->linedef || ! seg->linedef->special)
		return false;

	if (seg->linedef->tag <= 0)
		return false;

	if (BOT_LineSelfActivated(seg->linedef))
		return false;

	const linetype_c *special = seg->linedef->special;

	// FIXME: rule out lighting stuff (ETC...)

	// already present ?
	if (swth_list)
	{
		for (bot_goal_c *g = *swth_list; g; g = g->goal_next)
		{
			if (g->type != bot_goal_c::TYP_Switch)
				continue;

			if (g->sw_line == seg->linedef - lines)
				return false;
		}
	}

	L_WriteDebug("SWTH-Line %d (tag %d) keys 0x%x\n", seg->linedef - lines,
		seg->linedef->tag, (int) special->keys);

	// check if can activate it (KEYS)
	if (special->keys != KF_NONE)
		if (! HaveAchievedKey(achieved_goals, special->keys))
			return false;

	// add it to list
	if (swth_list)
	{
		bot_goal_c *g = bot_goal_c::NewSwitch(seg->linedef - lines, front_sub);

		g->goal_next = *swth_list;
		*swth_list = g;
	}

	return true;
}
#endif

//----------------------------------------------------------------------------

float BOT_BaseDistAlongSeg(int sub, int next, seg_t *seg)
{
	float cx = ss_info[sub].mid_x;
	float cy = ss_info[sub].mid_y;

	float nx = ss_info[next].mid_x;
	float ny = ss_info[next].mid_y;

	float sx = (seg->v1->x + seg->v2->x) / 2.0f;
	float sy = (seg->v1->y + seg->v2->y) / 2.0f;

	// distance computation (goes through seg's mid point)
	return R_PointToDist(cx, cy, sx, sy) + R_PointToDist(sx, sy, nx, ny);
}

fixed_target_base_c *BOT_FindLevelTarget(void)
{
	for (int i = 0; i < numsegs; i++)
	{
		seg_t *seg = segs + i;

		if (seg->miniseg)
			continue;

		line_t *L = seg->linedef;

		if (L->special && L->special->e_exit != EXIT_None)
		{
			if (L->special->type == line_pushable ||
			    L->special->type == line_manual)
			{
				return new bot_switch_target_c(seg, seg->front_sub - subsectors);
			}

			if (L->special->type == line_walkable && seg->back_sub)
			{
				return new bot_walk_target_c(seg, seg->front_sub - subsectors,
					seg->back_sub - subsectors, false);
			}

			// allow shootable ??
		}
	}

	return NULL;  // not found
}

static int BOT_FindKeySolutions(target_group_c *T, keys_e req_key)
{
	int count = 0;

	if (req_key & KF_BOOM_SKCK)
	{
		// Boom compatibility: treat card and skull types the same
		req_key = (keys_e)(EXPAND_KEYS(req_key));
	}

	for (mobj_t *mo = mobjlisthead; mo; mo = mo->next)
	{
		if (! BOT_ThingIsPickup(mo))
			continue;

		// loop over all keys.  That's because when req_key has multiple keys,
		// we should find solutions for each one.  This holds even when using
		// KF_STRICTLY_ALL, as the puzzle-solver should (in theory :) select
		// a single key to get first, and future iterations will decide to
		// get the remaining keys.
		//
		// Note: this loop _inside_ mobj search loop for efficiency.

		for (int knum = 0; knum < 16; knum++)
		{
			keys_e K = (keys_e)(1 << knum);

			if ((K & req_key) == KF_NONE)
				continue;

			if (BOT_ThingMatchesKey(mo, K))
			{
				// found a matching key !!

				bot_item_target_c *sol = new bot_item_target_c(K,
					mo->subsector - subsectors, mo->x, mo->y);

				sol->DebugDump("FOUND KEY:");

				T->AddSolution(sol);

				count++;
			}
		}
	} // for (mobj_t *mo)

	return count;
}

//----------------------------------------------------------------------------

sector_obstacle_c::sector_obstacle_c(int _sector, line_t *ld) :
	sector(_sector), line(ld)
{ }

sector_obstacle_c::~sector_obstacle_c()
{ }

void sector_obstacle_c::DebugDump(const char *preface)
{
	L_WriteDebug("%s SECTOR_OBST(%d line %d)\n",
		preface, sector, line ? line - lines : -1);
}

void sector_obstacle_c::MakeAchieved(bot_achievements_c *Ach) const
{
	if (line->tag > 0)
	{
		if (! Ach->HasTag(line->tag))
			Ach->AddTag(line->tag);
	}
	else if (! Ach->HasSector(sector))
		Ach->AddSector(sector);
}

int sector_obstacle_c::FindSolutions(target_group_c *T, keys_e owned_keys)
{
	// keys are the primary dependency
	if (line && line->special && (line->special->keys != KF_NONE))
	{
		// remove owned keys from the set of needed keys
		if (line->special->keys & KF_BOOM_SKCK)
			owned_keys = (keys_e) EXPAND_KEYS(owned_keys);

		keys_e need_k = (keys_e)(line->special->keys & ~owned_keys);

		if (need_k != KF_NONE)
			return BOT_FindKeySolutions(T, need_k);
	}

	seg_t *seg;

	// handle switches for remote doors (etc)

	if (BOT_SectorRemoteActivated(sectors + sector, &seg)) // !!!!! FIXME: want ALL here
	{
		line_t *L = seg->linedef;

// L_WriteDebug("::FindSolutions: sector %d seg %d line %d\n", sector, seg - segs, seg->linedef - lines);

		DEV_ASSERT2(L);
		DEV_ASSERT2(L->special);

		if (L->special->type == line_pushable ||
			L->special->type == line_manual)
		{
			T->AddSolution(
				new bot_switch_target_c(seg, seg->front_sub - subsectors));
			return 1;
		}

		if (L->special->type == line_walkable && seg->back_sub)
		{
			T->AddSolution(
				new bot_walk_target_c(seg, seg->front_sub - subsectors,
					seg->back_sub - subsectors, false));
			return 1;
		}

// L_WriteDebug("::FindSolutions BAD TYPE? %d\n", L->special->type);
		// FIXME: allow shootable
	}

	return 0;
}

//----------------------------------------------------------------------------

target_group_c::target_group_c(obstacle_base_c *_Obst) :
	obst(_Obst), num_sols(0)
{ }

target_group_c::~target_group_c()
{ }

void target_group_c::AddSolution(fixed_target_base_c *_Sol)
{
	if (num_sols >= MAX_SOL)
	{
		// ignore it (Ouch !!)
		L_WriteDebug("WARNING target_group_c::AddSolution overflow!\n");
		return;
	}

	solutions[num_sols++] = _Sol;
}

obstacle_base_c *target_group_c::GetObstacle() const
{
	return obst;
}

fixed_target_base_c *target_group_c::GetSolution(int idx) const
{
	DEV_ASSERT2(0 <= idx && idx < num_sols);

	return solutions[idx];
}

//----------------------------------------------------------------------------

target_group_stack_c::target_group_stack_c() : cur_size(0)
{ }

target_group_stack_c::~target_group_stack_c()
{ }

void target_group_stack_c::Push(target_group_c *_Group)
{
L_WriteDebug("target_group_stack_c::PUSH size before %d\n", cur_size);

	if (cur_size >= MAX_STK)
		I_Error("target_group_c::Push - overflow !\n");
	
	groups[cur_size] = _Group;
	solut_indexes[cur_size] = 0;

	cur_size++;
}

void target_group_stack_c::Pop()
{
	DEV_ASSERT2(! IsEmpty());

	cur_size--;
L_WriteDebug("target_group_stack_c::POP size after %d\n", cur_size);
}

bool target_group_stack_c::Bump()
{
	DEV_ASSERT2(! IsEmpty());

	solut_indexes[cur_size-1]++;

	if (solut_indexes[cur_size-1] < groups[cur_size-1]->NumSolutions())
		return true;

	Pop();

	return false;
}

bool target_group_stack_c::HasObstacle(obstacle_base_c *test) const
{
	DEV_ASSERT2(test);
	DEV_ASSERT2(! IsEmpty());

	for (int g = 0; g < cur_size; g++)
	{
		if (! groups[g]->GetObstacle())
			continue;

		if (test->Match(groups[g]->GetObstacle()))
			return true;
	}

	return false; // no match
}

obstacle_base_c *target_group_stack_c::CurObstacle() const
{
	DEV_ASSERT2(! IsEmpty());

	return groups[cur_size-1]->GetObstacle();
}

fixed_target_base_c *target_group_stack_c::CurTarget() const
{
	DEV_ASSERT2(! IsEmpty());

	int sol_idx = solut_indexes[cur_size-1];

	DEV_ASSERT2(sol_idx >= 0);
	DEV_ASSERT2(sol_idx < groups[cur_size-1]->NumSolutions());

	return groups[cur_size-1]->GetSolution(sol_idx);
}

