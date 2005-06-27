//----------------------------------------------------------------------------
//  EDGE Specials Lines & Floor Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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
// -KM- 1998/09/01 Lines.ddf
//
//

#include "i_defs.h"
#include "p_spec.h"

#include "con_main.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "g_game.h"
#include "m_argv.h"
#include "m_inline.h"
#include "m_random.h"
#include "p_local.h"
#include "r_local.h"
#include "r_state.h"
#include "r2_defs.h"
#include "rad_trig.h"
#include "s_sound.h"
#include "v_colour.h"
#include "w_textur.h"
#include "w_wad.h"
#include "z_zone.h"

#include "ddf_main.h"  // -KM- 1998/07/31 Need animation definitions

// Level exit timer
bool levelTimer;
int levelTimeCount;

// -AJA- temp structure for BOOM water (compatibility)
static const image_t ** sp_new_floors;

//
// Animating line and sector specials
//
line_t * line_speciallist;
sector_t * sect_speciallist;

typedef struct sectorsfx_s
{
	sector_t *sector;
	sfx_t *sfx;
	bool sfxstarted;

	// tics to go before next update
	int count;

	// link in list
	struct sectorsfx_s *next;
}
sectorsfx_t;

#define SECSFX_TIME  7  // every 7 tics (i.e. 5 times per second)

static sectorsfx_t *sectorsfx_list;

static bool P_DoSectorsFromTag(int tag, const void *p1, void *p2,
		bool(*func) (sector_t *, const void *, void *));

#if 0	// -ACB- Unfinished
//
// DoElevator_wrapper
//
// -ACB- 2001/01/14 Added
//
static bool DoElevator_wrapper(sector_t *s, const void *p1, void *p2) 
{
	return EV_DoElevator(s, (const elevatordef_c*)p1, (sector_t*)p2);
}
#endif

//
// DoPlane_wrapper
//
static bool DoPlane_wrapper(sector_t *s, const void *p1, void *p2)
{
	return EV_DoPlane(s, (const movplanedef_c*)p1, (sector_t*)p2);
}

//
// DoLights_wrapper
//
static bool DoLights_wrapper(sector_t *s, const void *p1, void *p2)
{
	return EV_Lights(s, (const lightdef_c*)p1);
}

//
// DoDonut_wrapper
//
static bool DoDonut_wrapper(sector_t *s, const void *p1, void *p2)
{
	return EV_DoDonut(s, (sfx_t**)p2);
}

//
// Creates a sector sfx and adds it to the list.
//
static sectorsfx_t *NewSectorSFX(void)
{
	sectorsfx_t *sfx;

	sfx = Z_New(sectorsfx_t, 1);

	sfx->next = sectorsfx_list;
	sectorsfx_list = sfx;

	return sfx;
}

//
// P_DestroyAllSectorSFX
//
void P_DestroyAllSectorSFX(void)
{
	sectorsfx_t *sfx, *next;

	for (sfx = sectorsfx_list; sfx; sfx = next)
	{
		next = sfx->next;
		Z_Free(sfx);
	}
	sectorsfx_list = NULL;
}

//
// DoSectorSFX
//
// -KM- 1998/09/27
//
// This function is called every so often to keep a sector's ambient
// sound going.  The sound should be looped.
//
static void DoSectorSFX(sectorsfx_t *sec)
{
	if (--sec->count)
		return;

	sec->count = SECSFX_TIME;

	if (!sec->sfxstarted)
        sound::StartFX(sec->sfx, SNCAT_Level, &sec->sector->sfx_origin);
}

//
// P_RunSectorSFX
//
void P_RunSectorSFX(void)
{
	// -AJA- FIXME: 

	sectorsfx_t *sfx;

	for (sfx = sectorsfx_list; sfx; sfx = sfx->next)
	{
		DoSectorSFX(sfx);
	}
}

//
// UTILITIES
//

//
// P_GetSide
//
// Will return a side_t * given the number of the current sector,
// the line number, and the side (0/1) that you want.
//
side_t *P_GetSide(int currentSector, int line, int side)
{
	line_t *linedef = sectors[currentSector].lines[line];

	return linedef->side[side];
}

//
// P_GetSector
//
// Will return a sector_t*
//  given the number of the current sector,
//  the line number and the side (0/1) that you want.
//
sector_t *P_GetSector(int currentSector, int line, int side)
{
	line_t *linedef = sectors[currentSector].lines[line];

	return side ? linedef->backsector : linedef->frontsector;
}

//
// P_TwoSided
//
// Given the sector number and the line number, it will tell you whether the
// line is two-sided or not.
//
int P_TwoSided(int sector, int line)
{
	return (sectors[sector].lines[line])->flags & ML_TwoSided;
}

//
// P_GetNextSector
//
// Return sector_t * of sector next to current; NULL if not two-sided line
//
sector_t *P_GetNextSector(const line_t * line, const sector_t * sec)
{
	if (!(line->flags & ML_TwoSided))
		return NULL;

	if (line->frontsector == sec)
		return line->backsector;

	return line->frontsector;
}

//
// P_FindSurroundingHeight
//
// -AJA- 2001/05/29: this is an amalgamation of the previous bunch of
//       routines, using the new REF_* flag names.  Now there's just
//       this one big routine -- the compiler's optimiser had better
//       kick in !
//
#define F_C_HEIGHT(sector)  \
((ref & REF_CEILING) ? (sector)->c_h : (sector)->f_h)

float P_FindSurroundingHeight(const heightref_e ref, const sector_t *sec)
{
	int i, count;
	float height;
	float base = F_C_HEIGHT(sec);

	if (ref & REF_INCLUDE)
		height = base;
	else if (ref & REF_HIGHEST)
		height = -32000.0f;  // BOOM compatible value
	else
		height = +32000.0f;

	for (i = count = 0; i < sec->linecount; i++)
	{
		sector_t *other = P_GetNextSector(sec->lines[i], sec);
		bool satisfy;
		float other_h;

		if (!other)
			continue;

		other_h = F_C_HEIGHT(other);

		if (ref & REF_NEXT)
		{
			// this may seem reversed, but is OK (see note in ddf_sect.c)
			if (ref & REF_HIGHEST)
				satisfy = (other_h < base);  // next lowest
			else
				satisfy = (other_h > base);  // next highest

			if (! satisfy)
				continue;
		}

		count++;

		if (ref & REF_HIGHEST)
			height = MAX(height, other_h);
		else
			height = MIN(height, other_h);
	}

	if ((ref & REF_NEXT) && count == 0)
		return base;

	return height;
}

//
// P_FindRaiseToTexture
//
// FIND THE SHORTEST BOTTOM TEXTURE SURROUNDING sec
// AND RETURN IT'S TOP HEIGHT
//
// -KM- 1998/09/01 Lines.ddf; used to be inlined in p_floors
//
float P_FindRaiseToTexture(sector_t * sec)
{
	int i;
	side_t *side;
	float minsize = INT_MAX;
	int secnum = sec - sectors;

	for (i = 0; i < sec->linecount; i++)
	{
		if (P_TwoSided(secnum, i))
		{
			side = P_GetSide(secnum, i, 0);

			if (side->bottom.image)
			{
				if (IM_HEIGHT(side->bottom.image) < minsize)
					minsize = IM_HEIGHT(side->bottom.image);
			}

			side = P_GetSide(secnum, i, 1);

			if (side->bottom.image)
			{
				if (IM_HEIGHT(side->bottom.image) < minsize)
					minsize = IM_HEIGHT(side->bottom.image);
			}
		}
	}

	return sec->f_h + minsize;
}

//
// P_FindSectorFromTag
//
// Returns the FIRST sector that tag refers to.
//
// -KM- 1998/09/27 Doesn't need a line.
// -AJA- 1999/09/29: Now returns a sector_t, and has no start.
//
sector_t *P_FindSectorFromTag(int tag)
{
	int i;

	for (i = 0; i < numsectors; i++)
	{
		if (sectors[i].tag == tag)
			return sectors + i;
	}

	return NULL;
}

//
// P_FindMinSurroundingLight
//
// Find minimum light from an adjacent sector
//
int P_FindMinSurroundingLight(sector_t * sector, int max)
{
	int i;
	int min;
	line_t *line;
	sector_t *check;

	min = max;
	for (i = 0; i < sector->linecount; i++)
	{
		line = sector->lines[i];
		check = P_GetNextSector(line, sector);

		if (!check)
			continue;

		if (check->props.lightlevel < min)
			min = check->props.lightlevel;
	}
	return min;
}

//
// P_AddSpecialLine
//
// UTILITY FUNCS FOR SPECIAL LIST 
//
void P_AddSpecialLine(line_t *ld)
{
	line_t *check;

	// check if already linked
	for (check=line_speciallist; check; check=check->animate_next)
	{
		if (check == ld)
			return;
	}

	ld->animate_next = line_speciallist;
	line_speciallist = ld;
}

//
// P_AddSpecialSector
//
void P_AddSpecialSector(sector_t *sec)
{
	sector_t *check;

	// check if already linked
	for (check=sect_speciallist; check; check=check->animate_next)
	{
		if (check == sec)
			return;
	}

	sec->animate_next = sect_speciallist;
	sect_speciallist = sec;
}

static void AdjustScrollParts(side_t *side, bool left,
		scroll_part_e parts, float x_speed, float y_speed)
{
	float xmul = (left && (parts & SCPT_LeftRevX)) ? -1 : 1;
	float ymul = (left && (parts & SCPT_LeftRevY)) ? -1 : 1;

	if (! side)
		return;

	// -AJA- this is an inconsistency, needed for compatibility with
	//       original DOOM and Boom.  (Should be SCPT_RIGHT | SCPT_LEFT).
	if (parts == SCPT_None)
		parts = SCPT_RIGHT;

	if (parts & (left ? SCPT_LeftUpper : SCPT_RightUpper))
	{
		side->top.scroll.x += x_speed * xmul;
		side->top.scroll.y += y_speed * ymul;
	}
	if (parts & (left ? SCPT_LeftMiddle : SCPT_RightMiddle))
	{
		side->middle.scroll.x += x_speed * xmul;
		side->middle.scroll.y += y_speed * ymul;
	}
	if (parts & (left ? SCPT_LeftLower : SCPT_RightLower))
	{
		side->bottom.scroll.x += x_speed * xmul;
		side->bottom.scroll.y += y_speed * ymul;
	}
}

static void AdjustScaleParts(side_t *side, bool left,
		scroll_part_e parts, float scale)
{
	if (! side)
		return;

	if (parts == SCPT_None)
		parts = (scroll_part_e)(SCPT_LEFT | SCPT_RIGHT);

	if (parts & (left ? SCPT_LeftUpper : SCPT_RightUpper))
		side->top.x_mat.x = side->top.y_mat.y = scale;

	if (parts & (left ? SCPT_LeftMiddle : SCPT_RightMiddle))
		side->middle.x_mat.x = side->middle.y_mat.y = scale;

	if (parts & (left ? SCPT_LeftLower : SCPT_RightLower))
		side->bottom.x_mat.x = side->bottom.y_mat.y = scale;
}

static void AdjustSkewParts(side_t *side, bool left,
		scroll_part_e parts, float skew)
{
	if (! side)
		return;

	if (parts == SCPT_None)
		parts = (scroll_part_e)(SCPT_LEFT | SCPT_RIGHT);

	if (parts & (left ? SCPT_LeftUpper : SCPT_RightUpper))
		side->top.y_mat.x = skew * side->top.y_mat.y;

	if (parts & (left ? SCPT_LeftMiddle : SCPT_RightMiddle))
		side->middle.y_mat.x = skew * side->middle.y_mat.y;

	if (parts & (left ? SCPT_LeftLower : SCPT_RightLower))
		side->bottom.y_mat.x = skew * side->bottom.y_mat.y;
}

static void AdjustLightParts(side_t *side, bool left,
		scroll_part_e parts, region_properties_t *p)
{
	if (! side)
		return;

	if (parts == SCPT_None)
		parts = (scroll_part_e)(SCPT_LEFT | SCPT_RIGHT);

	if (parts & (left ? SCPT_LeftUpper : SCPT_RightUpper))
		side->top.override_p = p;

	if (parts & (left ? SCPT_LeftMiddle : SCPT_RightMiddle))
		side->middle.override_p = p;

	if (parts & (left ? SCPT_LeftLower : SCPT_RightLower))
		side->bottom.override_p = p;
}


//
// P_EFTransferTrans
//
static void P_EFTransferTrans(sector_t *ctrl, sector_t *sec, line_t *line, 
		const extrafloordef_c *ef, float trans)
{
	int i;

	// floor and ceiling

	if (ctrl->floor.translucency > trans)
		ctrl->floor.translucency = trans;

	if (ctrl->ceil.translucency > trans)
		ctrl->ceil.translucency = trans;

	// sides

	if (! (ef->type & EXFL_Thick))
		return;

	if (ef->type & (EXFL_SideUpper | EXFL_SideLower))
	{
		for (i=0; i < sec->linecount; i++)
		{
			line_t *L = sec->lines[i];
			side_t *S = NULL;

			if (L->frontsector == sec)
				S = L->side[1];
			else if (L->backsector == sec)
				S = L->side[0];

			if (! S)
				continue;

			if (ef->type & EXFL_SideUpper)
				S->top.translucency = trans;
			else  // EXFL_SideLower
				S->bottom.translucency = trans;
		}

		return;
	}

	line->side[0]->middle.translucency = trans;
}

//
// P_LineEffect
//
// Handles BOOM's line -> tagged line transfers.
//
static void P_LineEffect(line_t *target, line_t *source,
		const linetype_c *special)
{
	float length = R_PointToDist(0, 0, source->dx, source->dy);

	if ((special->line_effect & LINEFX_Translucency) && (target->flags & ML_TwoSided))
	{
		target->side[0]->middle.translucency = 0.5f;
		target->side[1]->middle.translucency = 0.5f;
	}

	if (special->line_effect & LINEFX_VectorScroll)
	{
		// -AJA- Note: these values are the same as in BOOM, which doesn't
		//       exactly match the description given in boomref.txt, which
		//       suggests that the horizontal speed is proportional to the
		//       tagging line's length.

		float xspeed = source->dx / 32.0f;
		float yspeed = source->dy / 32.0f;

		AdjustScrollParts(target->side[0], 0, special->line_parts,
				xspeed, yspeed);

		AdjustScrollParts(target->side[1], 1, special->line_parts,
				xspeed, yspeed);

		P_AddSpecialLine(target);
	}

	if ((special->line_effect & LINEFX_OffsetScroll) && target->side[0])
	{
		float xspeed = -target->side[0]->middle.offset.x;
		float yspeed =  target->side[0]->middle.offset.y;

		AdjustScrollParts(target->side[0], 0, special->line_parts,
				xspeed, yspeed);

		P_AddSpecialLine(target);
	}

	// experimental: unblock line(s)
	if (special->line_effect & LINEFX_UnblockThings)
	{
		if (target->side[0] && target->side[1] && (target != source))
			target->flags &= ~(ML_Blocking | ML_BlockMonsters);
	}

	// experimental: block bullets/missiles
	if (special->line_effect & LINEFX_BlockShots)
	{
		if (target->side[0] && target->side[1])
			target->flags |= ML_ShootBlock;
	}

	// experimental: block monster sight
	if (special->line_effect & LINEFX_BlockSight)
	{
		if (target->side[0] && target->side[1])
			target->flags |= ML_SightBlock;
	}

	// experimental: scale wall texture(s) by line length
	if (special->line_effect & LINEFX_Scale)
	{
		AdjustScaleParts(target->side[0], 0, special->line_parts, length/128);
		AdjustScaleParts(target->side[1], 1, special->line_parts, length/128);
	}

	// experimental: skew wall texture(s) by sidedef Y offset
	if ((special->line_effect & LINEFX_Skew) && source->side[0])
	{
		float skew = source->side[0]->top.offset.x / 128.0f;

		AdjustSkewParts(target->side[0], 0, special->line_parts, skew);
		AdjustSkewParts(target->side[1], 1, special->line_parts, skew);

		if (target == source)
		{
			source->side[0]->middle.offset.x = 0;
			source->side[0]->bottom.offset.x = 0;
		}
	}

	// experimental: transfer lighting to wall parts
	if (special->line_effect & LINEFX_LightWall)
	{
		AdjustLightParts(target->side[0], 0, special->line_parts,
				&source->frontsector->props);
		AdjustLightParts(target->side[1], 1, special->line_parts,
				&source->frontsector->props);
	}
}

//
// P_SectorEffect
//
// Handles BOOM's line -> tagged sector transfers.
//
static void P_SectorEffect(sector_t *target, line_t *source,
		const linetype_c *special)
{
	float length = R_PointToDist(0, 0, source->dx, source->dy);
	angle_t angle = R_PointToAngle(0, 0, source->dx, source->dy);

	if (special->sector_effect & SECTFX_LightFloor)
		target->floor.override_p = &source->frontsector->props;

	if (special->sector_effect & SECTFX_LightCeiling)
		target->ceil.override_p = &source->frontsector->props;

	if (special->sector_effect & SECTFX_ScrollFloor)
	{
		target->floor.scroll.x -= source->dx / 32.0f;
		target->floor.scroll.y -= source->dy / 32.0f;

		P_AddSpecialSector(target);
	}
	if (special->sector_effect & SECTFX_ScrollCeiling)
	{
		target->ceil.scroll.x -= source->dx / 32.0f;
		target->ceil.scroll.y -= source->dy / 32.0f;

		P_AddSpecialSector(target);
	}

	if (special->sector_effect & SECTFX_PushThings)
	{
		target->props.push.x += source->dx / 320.0f;
		target->props.push.y += source->dy / 320.0f;
	}

	if (special->sector_effect & SECTFX_ResetFloor)
	{
		target->floor.override_p = NULL;
		target->floor.scroll.x = target->floor.scroll.y = 0;
		target->props.push.x = target->props.push.y = target->props.push.z = 0;
	}
	if (special->sector_effect & SECTFX_ResetCeiling)
	{
		target->ceil.override_p = NULL;
		target->ceil.scroll.x = target->ceil.scroll.y = 0;
	}

	// experimental: set texture scale
	if (special->sector_effect & SECTFX_ScaleFloor)
		target->floor.x_mat.x = target->floor.y_mat.y = length;

	if (special->sector_effect & SECTFX_ScaleCeiling)
		target->ceil.x_mat.x = target->ceil.y_mat.y = length;

	// experimental: set texture alignment
	if (special->sector_effect & SECTFX_AlignFloor)
	{
		target->floor.offset.x = -source->v1->x;
		target->floor.offset.y = -source->v1->y;

		M_Angle2Matrix(angle, &target->floor.x_mat, &target->floor.y_mat);
	}
	if (special->sector_effect & SECTFX_AlignCeiling)
	{
		target->ceil.offset.x = -source->v1->x;
		target->ceil.offset.y = -source->v1->y;

		M_Angle2Matrix(angle, &target->ceil.x_mat, &target->ceil.y_mat);
	}
}

//
// EVENTS
//
// Events are operations triggered by using, crossing,
// or shooting special lines, or by timed thinkers.
//

//
// P_ActivateSpecialLine
//
// Called when a special line is activated.
//
// line is the line to be activated, side is the side activated from,
// (as lines can only be activated from the right), thing is the thing
// activating, to check for player/monster only lines trig is how it
// was activated, ie shot/crossed/pushed.  `line' can be NULL for
// non-line activations.
//
// -KM- 1998/09/01 Procedure Written.  
//
// -ACB- 1998/09/11 Return Success or Failure.
//
// -AJA- 1999/09/29: Updated for new tagged sector links.
//
// -AJA- 1999/10/21: Allow non-line activation (line == NULL), and
//                   added `typenum' and `tag' parameter.
//
// -AJA- 2000/01/02: New trigger method `line_Any'.
//
// -ACB- 2001/01/14: Added Elevator Sector Type
//
static bool P_ActivateSpecialLine(line_t * line,
		const linetype_c * special, int tag, int side, mobj_t * thing,
		trigger_e trig, int can_reach, int no_care_who)
{
	bool texSwitch = false;
	bool failedsecurity;  // -ACB- 1998/09/11 Security pass/fail check
	bool playedSound = false;
	sfx_t *sfx[4];
	sector_t *tsec;

	int i;

#ifdef DEVELOPERS
	if (!special)
	{
		if (line == NULL)
			I_Error("P_ActivateSpecialLine: Special type is 0\n");
		else
			I_Error("P_ActivateSpecialLine: Line %d is not Special\n", 
					(int)(line - lines));
	}
#endif

	if (!G_CheckWhenAppear(special->appear))
	{
		if (line)
			line->special = NULL;

		return true;
	}

	if (trig != line_Any && special->type != trig &&
			!(special->type == line_manual && trig == line_pushable))
		return false;

	// Check for use once.
	if (line && line->count == 0)
		return false;

	// Single sided line
	if (trig != line_Any && special->singlesided && side == 1)
		return false;

	// -AJA- 1999/12/07: Height checking.
	if (line && thing && thing->player && 
        (special->special_flags & LINSP_MustReach) && !can_reach)
	{
        sound::StartFX(thing->info->noway_sound, 
                       P_MobjGetSfxCategory(thing), thing);

		return false;
	}

	// Check this type of thing can trigger
	if (!no_care_who)
	{
		if (thing && thing->player)
		{
			// Players can only trigger if the trig_player is set
			if (!(special->obj & trig_player))
				return false;
		}
		else if (thing && (thing->info->extendedflags & EF_MONSTER))
		{
			// Monsters can only trigger if the trig_monster flag is set
			if (!(special->obj & trig_monster))
				return false;

			// Monsters don't trigger secrets
			if (line && (line->flags & ML_Secret))
				return false;
		}
		else
		{
			// Other stuff can only trigger if trig_other is set
			if (!(special->obj & trig_other))
				return false;

			// Other stuff doesn't trigger secrets
			if (line && (line->flags & ML_Secret))
				return false;
		}
	}

	if (thing && !no_care_who)
	{
		// Check for keys
		// -ACB- 1998/09/11 Key possibilites extended
		if (special->keys != KF_NONE)
		{
			keys_e req = (keys_e)(special->keys & KF_MASK);
			keys_e cards;

			// Monsters/Missiles have no keys
			if (!thing->player)
				return false;

			//
			// New Security Checks, allows for any combination of keys in
			// an AND or OR function. Therefore it extends the possibilities
			// of security above 3 possible combinations..
			//
			// -AJA- Reworked this for the 10 new keys.
			//
			cards = thing->player->cards;
			failedsecurity = false;

			if (special->keys & KF_BOOM_SKCK)
			{
				// Boom compatibility: treat card and skull types the same
				cards = (keys_e)(EXPAND_KEYS(cards));
			}

			if (special->keys & KF_STRICTLY_ALL)
			{
				if ((cards & req) != req)
					failedsecurity = true;
			}
			else
			{
				if ((cards & req) == 0)
					failedsecurity = true;
			}

			if (failedsecurity)
			{
				if (special->failedmessage)
					CON_PlayerMessageLDF(thing->player->pnum, special->failedmessage);

				return false;
			}
		}
	}

	// Check if button already pressed
	if (line && buttonlist.IsPressed(line))
		return false;

	// Do lights
	// -KM- 1998/09/27 Generalised light types.
	switch (special->l.type)
	{
		case LITE_Set:
			EV_LightTurnOn(tag, special->l.level);
			texSwitch = true;
			break;

		case LITE_None:
			break;

		default:
			texSwitch = P_DoSectorsFromTag(tag, &special->l, NULL, DoLights_wrapper);
			break;
	}

	// -ACB- 1998/09/13 Use teleport define..
	if (special->t.teleport)
	{
		texSwitch = EV_Teleport(line, tag, thing, &special->t);
	}

	if (special->e_exit == EXIT_Normal)
	{
		G_ExitLevel(5);
		texSwitch = true;
	}
	else if (special->e_exit == EXIT_Secret)
	{
		G_SecretExitLevel(5);
		texSwitch = true;
	}

	if (special->d.dodonut)
	{
		// Proper ANSI C++ Init
		sfx[0] = special->d.d_sfxout;
		sfx[1] = special->d.d_sfxoutstop;
		sfx[2] = special->d.d_sfxin;
		sfx[3] = special->d.d_sfxinstop;

		texSwitch = P_DoSectorsFromTag(tag, NULL, sfx, DoDonut_wrapper);
	}

	//
	// - Plats/Floors -
	//
	if (special->f.type != mov_undefined)
	{
		if (!tag || special->type == line_manual)
		{
			if (line)
				texSwitch = EV_ManualPlane(line, thing, &special->f);
		}
		else
		{
			texSwitch = P_DoSectorsFromTag(tag, &special->f, 
					line ? line->frontsector : NULL, DoPlane_wrapper);
		}
	}

	//
	// - Doors/Ceilings -
	//
	if (special->c.type != mov_undefined)
	{
		if (!tag || special->type == line_manual)
		{
			if (line)
				texSwitch = EV_ManualPlane(line, thing, &special->c);
		}
		else
		{
			texSwitch = P_DoSectorsFromTag(tag, &special->c,
					line ? line->frontsector : NULL, DoPlane_wrapper);
		}
	}

#if 0  // -AJA- DISABLED (Unfinished)
	//
	// - Elevators -
	//
	// -ACB- 2001/01/14 Added
	//
	if (special->e.type != mov_undefined)
	{
		if (!tag || special->type == line_manual)
		{
			if (line)
				texSwitch = EV_ManualElevator(line, thing, &special->e);
		}
		else
		{
			texSwitch = P_DoSectorsFromTag(tag, &special->e,
					line ? line->frontsector : NULL, DoElevator_wrapper);
		}
	}
#endif

	if (special->use_colourmap && tag > 0)
	{
		for (tsec = P_FindSectorFromTag(tag); tsec; tsec = tsec->tag_next)
		{
			tsec->props.colourmap = special->use_colourmap;
			texSwitch = true;
		}
	}

	if (special->gravity != FLO_UNUSED && tag > 0)
	{
		for (tsec = P_FindSectorFromTag(tag); tsec; tsec = tsec->tag_next)
		{
			tsec->props.gravity = special->gravity;
			texSwitch = true;
		}
	}

	if (special->friction != FLO_UNUSED && tag > 0)
	{
		for (tsec = P_FindSectorFromTag(tag); tsec; tsec = tsec->tag_next)
		{
			tsec->props.friction = special->friction;
			texSwitch = true;
		}
	}

	if (special->viscosity != FLO_UNUSED && tag > 0)
	{
		for (tsec = P_FindSectorFromTag(tag); tsec; tsec = tsec->tag_next)
		{
			tsec->props.viscosity = special->viscosity;
			texSwitch = true;
		}
	}

	if (special->drag != FLO_UNUSED && tag > 0)
	{
		for (tsec = P_FindSectorFromTag(tag); tsec; tsec = tsec->tag_next)
		{
			tsec->props.drag = special->drag;
			texSwitch = true;
		}
	}

	// Extrafloor transfers
	if (line && special->ef.type && tag > 0)
	{
		sector_t *ctrl = line->frontsector;

		for (tsec = P_FindSectorFromTag(tag); tsec; tsec = tsec->tag_next)
		{
			P_AddExtraFloor(tsec, line);

			// Handle the BOOMTEX flag (Boom compatibility)
			if ((special->ef.type & EXFL_BoomTex) && sp_new_floors)
			{
				if (! sp_new_floors[ctrl - sectors])
					sp_new_floors[ctrl - sectors] = tsec->floor.image;

				if (! sp_new_floors[tsec - sectors])
					sp_new_floors[tsec - sectors] = ctrl->floor.image;
			}

			// transfer any translucency
			if (PERCENT_2_FLOAT(special->translucency) <= 0.99f)
			{
				P_EFTransferTrans(ctrl, tsec, line, &special->ef,
						PERCENT_2_FLOAT(special->translucency));
			}

			// update the line gaps & things:
			P_RecomputeTilesInSector(tsec);
			P_RecomputeGapsAroundSector(tsec);

			// FIXME: tele-frag any things in the way

			P_FloodExtraFloors(tsec);

			texSwitch = true;
		}
	}

	// Tagged line effects
	if (line && special->line_effect)
	{
		if (!tag)
		{
			P_LineEffect(line, line, special);
			texSwitch = true;
		}
		else
		{
			for (i=0; i < numlines; i++)
			{
				if (lines[i].tag == tag)
				{
					P_LineEffect(lines + i, line, special);
					texSwitch = true;
				}
			}
		}
	}

	// Tagged sector effects
	if (line && special->sector_effect && tag > 0)
	{
		for (tsec = P_FindSectorFromTag(tag); tsec; tsec = tsec->tag_next)
		{
			P_SectorEffect(tsec, line, special);
			texSwitch = true;
		}
	}

	if (special->trigger_effect && tag > 0)
	{
		RAD_EnableByTag(thing, tag, special->trigger_effect < 0);
		texSwitch = true;
	}

	if (special->ambient_sfx && tag > 0)
	{
		for (tsec = P_FindSectorFromTag(tag); tsec; tsec = tsec->tag_next)
		{
			sectorsfx_t *sfx = NewSectorSFX();

			sfx->count = SECSFX_TIME;
			sfx->sector = tsec;
			sfx->sfx = special->ambient_sfx;
			sfx->sfxstarted = false;

			texSwitch = true;
		}
	}

	if (special->music)
	{
		S_ChangeMusic(special->music, true);
		texSwitch = true;
	}

	if (special->activate_sfx)
	{
		if (line)
        {
			sound::StartFX(special->activate_sfx, 
                           SNCAT_Level,
                           &line->frontsector->sfx_origin);
		}
        else if (thing)
        {
            sound::StartFX(special->activate_sfx, 
                           P_MobjGetSfxCategory(thing),
                           thing);
        }

		playedSound = true;
	}

	if (special->s.type != SLIDE_None && line)
	{
		EV_DoSlider(line, thing, &special->s);

		// Note: sliders need special treatment
		return true;
	}

	// reduce count & clear special if necessary
	if (line && texSwitch)
	{
		if (line->count != -1)
		{
			line->count--;

			if (!line->count)
				line->special = NULL;
		}
		// -KM- 1998/09/27 Reversable linedefs.
		if (line->special && special->newtrignum)
			line->special = (special->newtrignum <= 0) ? NULL :
				playsim::LookupLineType(special->newtrignum);

		P_ChangeSwitchTexture(line, line->special && (special->newtrignum == 0),
				special->special_flags, playedSound);
	}

	return true;
}

//
// P_CrossSpecialLine - TRIGGER
//
// Called every time a thing origin is about
// to cross a line with a non 0 special.
//
// -KM- 1998/09/01 Now much simpler
// -ACB- 1998/09/12 Return success/failure
//
bool P_CrossSpecialLine(line_t *ld, int side, mobj_t * thing)
{
	return P_ActivateSpecialLine(ld, ld->special, ld->tag, 
			side, thing, line_walkable, 1, 0);
}

//
// P_ShootSpecialLine - IMPACT SPECIALS
//
// Called when a thing shoots a special line.
//
void P_ShootSpecialLine(line_t * ld, int side, mobj_t * thing)
{
	P_ActivateSpecialLine(ld, ld->special, ld->tag, 
			side, thing, line_shootable, 1, 0);
}

//
// P_UseSpecialLine
//
// Called when a thing uses a special line.
// Only the front sides of lines are usable.
//
// -KM- 1998/09/01 Uses new lines.ddf code in p_spec.c
//
// -ACB- 1998/09/07 Uses the return value to discern if a move if possible.
//
// -AJA- 1999/12/07: New parameters `open_bottom' and `open_top',
//       which give a vertical range through which the linedef is
//       accessible.  Could be used for smarter switches, like one on
//       a lower wall-part which is out of reach (e.g. MAP02).
//
bool P_UseSpecialLine(mobj_t * thing, line_t * line, int side,
		float open_bottom, float open_top)
{
	int can_reach = (thing->z < open_top) &&
		(thing->z + thing->height + USE_Z_RANGE >= open_bottom);

	return P_ActivateSpecialLine(line, line->special, line->tag, side,
			thing, line_pushable, can_reach, 0);
}

//
// P_RemoteActivation
//
// Called by the RTS `ACTIVATE_LINETYPE' primitive, and also the code
// pointer in things.ddf of the same name.  Thing can be NULL.
//
// -AJA- 1999/10/21: written.
//
void P_RemoteActivation(mobj_t * thing, int typenum, int tag, 
		int side, trigger_e method)
{
	const linetype_c *spec = playsim::LookupLineType(typenum);

	P_ActivateSpecialLine(NULL, spec, tag, side, thing, method, 1,
			(thing == NULL));
}


static INLINE void PlayerInProperties(player_t *player,
		float bz, float tz, float f_h, float c_h,
		region_properties_t *props)
{
	const sectortype_c *special = props->special;
	float damage, factor;

	if (!special || c_h < f_h)
		return;

	if (!G_CheckWhenAppear(special->appear))
	{
		props->special = NULL;
		return;
	}

	// breathing support
	// (Mouth is where the eye is !)
	//
	if ((special->special_flags & SECSP_AirLess) &&
			player->viewz >= f_h && player->viewz <= c_h &&
			player->powers[PW_Scuba] <= 0)
	{
		player->air_in_lungs--;
		player->underwater = true;

		if (player->air_in_lungs <= 0 &&
				(leveltime % (1 + player->mo->info->choke_damage.delay)) == 0)
		{
			DAMAGE_COMPUTE(damage, &player->mo->info->choke_damage);

			if (damage)
				P_DamageMobj(player->mo, NULL, NULL, damage,
						&player->mo->info->choke_damage);
		}
	}

	if ((special->special_flags & SECSP_Swimming) &&
			player->viewz >= f_h && player->viewz <= c_h)
	{
		player->swimming = true;
	}

	factor = 1.0f;

	if (special->special_flags & SECSP_WholeRegion)
	{
		if (special->special_flags & SECSP_Proportional)
		{
			// only partially in region -- mitigate damage

			if (tz > c_h)
				factor -= factor * (tz - c_h) / (tz-bz);

			if (bz < f_h)
				factor -= factor * (f_h - bz) / (tz-bz);
		}
		else
		{
			if (bz > c_h || tz < f_h)
				factor = 0;
		}
	}
	else
	{

		// Not touching the floor ?
		if (player->mo->z > f_h + 2.0f)
			return;
	}

	if (player->powers[PW_AcidSuit])
		factor = 0;

	if (factor > 0 &&
			(leveltime % (1 + special->damage.delay)) == 0)
	{
		DAMAGE_COMPUTE(damage, &special->damage);

		if (damage)
			P_DamageMobj(player->mo, NULL, NULL, damage * factor,
					&special->damage);
	}

	if (special->secret)
	{
		player->secretcount++;
		props->special = NULL;
	}

	if (special->e_exit == EXIT_Normal)
	{
		player->cheats &= ~CF_GODMODE;
		if (player->health <= special->damage.nominal)
		{
            sound::StartFX(player->mo->info->deathsound,
                           P_MobjGetSfxCategory(player->mo),
                           player->mo);

			// -KM- 1998/12/16 We don't want to alter the special type,
			//   modify the sector's attributes instead.
			props->special = NULL;
			G_ExitLevel(1);
			return;
		}
	}
	else if (special->e_exit == EXIT_Secret)
	{
		player->cheats &= ~CF_GODMODE;
		if (player->health <= special->damage.nominal)
		{
            sound::StartFX(player->mo->info->deathsound,
                           P_MobjGetSfxCategory(player->mo),
                           player->mo);

			props->special = NULL;
			G_SecretExitLevel(1);
			return;
		}
	}
}


//
// P_PlayerInSpecialSector
//
// Called every tic frame that the player origin is in a special sector
//
// -KM- 1998/09/27 Generalised for sectors.ddf
// -AJA- 1999/10/09: Updated for new sector handling.
//
void P_PlayerInSpecialSector(player_t * player, sector_t * sec)
{
	extrafloor_t *S, *L, *C;
	float floor_h;

	float bz = player->mo->z;
	float tz = player->mo->z + player->mo->height;

	bool was_underwater = player->underwater;

	player->swimming = false;
	player->underwater = false;

	// traverse extrafloor list
	floor_h = sec->f_h;

	S = sec->bottom_ef;
	L = sec->bottom_liq;

	while (S || L)
	{
		if (!L || (S && S->bottom_h < L->bottom_h))
		{
			C = S;  S = S->higher;
		}
		else
		{
			C = L;  L = L->higher;
		}

		DEV_ASSERT2(C);

		// ignore "hidden" liquids
		if (C->bottom_h < floor_h || C->bottom_h > sec->c_h)
			continue;

		PlayerInProperties(player, bz, tz, floor_h, C->top_h, C->p);

		floor_h = C->top_h;
	}

	PlayerInProperties(player, bz, tz, floor_h, sec->c_h, sec->p);

	// breathing support: handle gasping when leaving the water
	if (was_underwater && !player->underwater)
	{
		if (player->air_in_lungs <=
				(player->mo->info->lung_capacity - player->mo->info->gasp_start))
		{
			if (player->mo->info->gasp_sound)
            {
                sound::StartFX(player->mo->info->gasp_sound,
                               P_MobjGetSfxCategory(player->mo),
                               player->mo);
            }
		}

		player->air_in_lungs = player->mo->info->lung_capacity;
	} 
}

static INLINE void ApplyScroll(vec2_t& offset, const vec2_t& delta)
{
	// prevent loss of precision (eventually stops the scrolling)

	offset.x = fmod(offset.x + delta.x, 4096.0f);
	offset.y = fmod(offset.y + delta.y, 4096.0f);
}

//
// P_UpdateSpecials
//
// Animate planes, scroll walls, etc.
//
void P_UpdateSpecials(void)
{
	line_t *line;
	sector_t *sec;
	const linetype_c *special;

	// LEVEL TIMER
	if (levelTimer == true)
	{
		levelTimeCount--;

		if (!levelTimeCount)
			G_ExitLevel(1);
	}

	// ANIMATE FLATS AND TEXTURES GLOBALLY

	W_UpdateImageAnims();

	// ANIMATE LINE SPECIALS
	// -KM- 1998/09/01 Lines.ddf
	for (line = line_speciallist; line; line = line->animate_next)
	{
		special = line->special;

		// -KM- 1999/01/31 Use new method.
		// -AJA- 1999/07/01: Handle both sidedefs.
		if (line->side[0])
		{
			ApplyScroll(line->side[0]->top.offset,    line->side[0]->top.scroll);
			ApplyScroll(line->side[0]->middle.offset, line->side[0]->middle.scroll);
			ApplyScroll(line->side[0]->bottom.offset, line->side[0]->bottom.scroll);
		}

		if (line->side[1])
		{
			ApplyScroll(line->side[1]->top.offset,    line->side[1]->top.scroll);
			ApplyScroll(line->side[1]->middle.offset, line->side[1]->middle.scroll);
			ApplyScroll(line->side[1]->bottom.offset, line->side[1]->bottom.scroll);
		}
	}

	// ANIMATE SECTOR SPECIALS
	for (sec = sect_speciallist; sec; sec = sec->animate_next)
	{
		ApplyScroll(sec->floor.offset, sec->floor.scroll);
		ApplyScroll(sec->ceil.offset,  sec->ceil.scroll);
	}

	// DO BUTTONS
	epi::array_iterator_c it;
	button_t *b;
	
	for (it=buttonlist.GetBaseIterator(); it.IsValid(); it++)
	{
		b = ITERATOR_TO_PTR(it, button_t);

		if (b->btimer == 0)
			continue;

		b->btimer--;

		if (b->btimer != 0)
			continue;

		switch (b->where)
		{
			case BWH_Top:
				b->line->side[0]->top.image = b->bimage;
				break;

			case BWH_Middle:
				b->line->side[0]->middle.image = b->bimage;
				break;

			case BWH_Bottom:
				b->line->side[0]->bottom.image = b->bimage;
				break;

			case BWH_None:
				I_Error("INTERNAL ERROR: bwhere is BWH_None !\n");
		}

		if (b->off_sound)
        {
            sound::StartFX(b->off_sound, SNCAT_Level,
                           &b->line->frontsector->sfx_origin);
        }

		memset(b, 0, sizeof(button_t));	
	}
}

//
//  SPECIAL SPAWNING
//

//
// P_SpawnSpecials
//
// This function is called at the start of every level.  It parses command line
// parameters for level timer, spawns passive special sectors, (ie sectors that
// act even when a player is not in them, and counts total secrets) spawns
// passive lines, (ie scrollers) and resets floor/ceiling movement.
//
// -KM- 1998/09/27 Generalised for sectors.ddf
// -KM- 1998/11/25 Lines with auto tag are automatically triggered.
//
void P_SpawnSpecials(int autotag)
{
	sector_t *sector;
	const sectortype_c *secSpecial;
	const linetype_c *special;
	const char *s;

	int i;

	// See if -TIMER needs to be used.
	levelTimer = false;

	i = M_CheckParm("-avg");
	if (i && DEATHMATCH())
	{
		levelTimer = true;
		levelTimeCount = 20 * 60 * TICRATE;
	}

	s = M_GetParm("-timer");
	if (s && DEATHMATCH())
	{
		int time;

		time = atoi(s) * 60 * TICRATE;
		levelTimer = true;
		levelTimeCount = time;
	}

	buttonlist.Clear();

	sp_new_floors = Z_ClearNew(const image_t *, numsectors);

	//
	// Init special SECTORs.
	//
	sect_speciallist = NULL;

	sector = sectors;
	for (i = 0; i < numsectors; i++, sector++)
	{
		if (!sector->props.special)
			continue;

		secSpecial = sector->props.special;

		if (! G_CheckWhenAppear(secSpecial->appear))
		{
			sector->props.special = NULL;
			continue;
		}

		if (secSpecial->l.type != LITE_None)
			EV_Lights(sector, &secSpecial->l);

		if (secSpecial->secret)
			totalsecret++;

		if (secSpecial->use_colourmap)
			sector->props.colourmap = secSpecial->use_colourmap;

		if (secSpecial->ambient_sfx)
		{
			sectorsfx_t *sfx = NewSectorSFX();

			sfx->count = SECSFX_TIME;
			sfx->sector = sector;
			sfx->sfx = secSpecial->ambient_sfx;
			sfx->sfxstarted = false;
		}

		// - Plats/Floors -
		if (secSpecial->f.type != mov_undefined)
			EV_DoPlane(sector, &secSpecial->f, sector);

		// - Doors/Ceilings -
		if (secSpecial->c.type != mov_undefined)
			EV_DoPlane(sector, &secSpecial->c, sector);

		sector->props.gravity   = secSpecial->gravity;
		sector->props.friction  = secSpecial->friction;
		sector->props.viscosity = secSpecial->viscosity;
		sector->props.drag      = secSpecial->drag;

		// compute pushing force
		if (secSpecial->push_speed > 0 || secSpecial->push_zspeed > 0)
		{
			float mul = secSpecial->push_speed / 100.0f;

			sector->props.push.x += M_Cos(secSpecial->push_angle) * mul;
			sector->props.push.y += M_Sin(secSpecial->push_angle) * mul;
			sector->props.push.z += secSpecial->push_zspeed / 100.0f;
		}

		// Scrollers
		if (secSpecial->f.scroll_speed > 0)
		{
			float dx = M_Cos(secSpecial->f.scroll_angle);
			float dy = M_Sin(secSpecial->f.scroll_angle);

			sector->floor.scroll.x -= dx * secSpecial->f.scroll_speed / 32.0f;
			sector->floor.scroll.y -= dy * secSpecial->f.scroll_speed / 32.0f;

			P_AddSpecialSector(sector);
		}
		if (secSpecial->c.scroll_speed > 0)
		{
			float dx = M_Cos(secSpecial->c.scroll_angle);
			float dy = M_Sin(secSpecial->c.scroll_angle);

			sector->ceil.scroll.x -= dx * secSpecial->c.scroll_speed / 32.0f;
			sector->ceil.scroll.y -= dy * secSpecial->c.scroll_speed / 32.0f;

			P_AddSpecialSector(sector);
		}
	}

	//
	// Init special LINEs.
	//
	// -ACB- & -JC- 1998/06/10 Implemented additional scroll effects
	//
	// -ACB- Added the code
	// -JC-  Designed and contributed code
	// -KM-  Removed Limit
	// -KM- 1998/09/01 Added lines.ddf support
	//
	line_speciallist = NULL;

	for (i = 0; i < numlines; i++)
	{
		special = lines[i].special;

		if (! special)
		{
			lines[i].count = 0;
			continue;
		}

		// -AJA- 1999/10/23: weed out non-appearing lines.
		if (! G_CheckWhenAppear(special->appear))
		{
			lines[i].special = NULL;
			continue;
		}

		lines[i].count = special->count;

		if (special->s_xspeed || special->s_yspeed)
		{
			AdjustScrollParts(lines[i].side[0], 0, special->scroll_parts,
					special->s_xspeed, special->s_yspeed);

			AdjustScrollParts(lines[i].side[1], 1, special->scroll_parts,
					special->s_xspeed, special->s_yspeed);

			P_AddSpecialLine(lines + i);
		}

		// -AJA- 1999/06/30: Translucency effect.
		if (PERCENT_2_FLOAT(special->translucency) <= 0.99f && lines[i].side[0])
			lines[i].side[0]->middle.translucency = PERCENT_2_FLOAT(special->translucency);

		if (PERCENT_2_FLOAT(special->translucency) <= 0.99f && lines[i].side[1])
			lines[i].side[1]->middle.translucency = PERCENT_2_FLOAT(special->translucency);

		if (special->autoline)
		{
			P_ActivateSpecialLine(&lines[i], lines[i].special,
					lines[i].tag, 0, NULL, line_Any, 1, 1);
		}

		// -KM- 1998/11/25 This line should be pushed automatically
		if (autotag && lines[i].special && lines[i].tag == autotag)
		{
			P_ActivateSpecialLine(&lines[i], lines[i].special,
					lines[i].tag, 0, NULL, line_pushable, 1, 1);
		}
	}

	for (i=0; i < numsectors; i++)
	{
		if (sp_new_floors[i])
			sectors[i].floor.image = sp_new_floors[i];
	}

	Z_Free(sp_new_floors);
	sp_new_floors = NULL;
}

//
// P_DoSectorsFromTag
//
// -KM- 1998/09/27 This helper function is used to do stuff to all the
//                 sectors with the specified line's tag.
//
// -AJA- 1999/09/29: Updated for new tagged sector links.
//
static bool P_DoSectorsFromTag(int tag, const void *p1, void *p2,
		bool(*func) (sector_t *, const void *, void *))
{
	sector_t *tsec;
	bool rtn = false;

	for (tsec = P_FindSectorFromTag(tag); tsec; tsec = tsec->tag_next)
	{
		if ((*func) (tsec, p1, p2))
			rtn = true;
	}

	return rtn;
}
