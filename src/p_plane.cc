//----------------------------------------------------------------------------
//  EDGE Floor/Ceiling/Stair/Elevator Action Code
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

#include "i_defs.h"

#include "z_zone.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "m_random.h"
#include "p_local.h"
#include "r_sky.h"
#include "r_state.h"
#include "s_sound.h"

typedef enum
{
    RES_Ok,
    RES_Crushed,
    RES_PastDest,
    RES_Impossible
}
move_result_e;

// Linked list of moving parts.
gen_move_t *active_movparts = NULL;

static bool P_StasifySector(sector_t * sec);
static bool P_ActivateInStasis(int tag);

/// static void MoveElevator(elev_move_t *elev);
static void MovePlane(plane_move_t *plane);
static void MoveSlider(slider_move_t *smov);

// -AJA- Perhaps using a pointer to `plane_info_t' would be better
//       than f**king about with the floorOrCeiling stuff all the
//       time.

static float HEIGHT(sector_t * sec, bool is_ceiling)
{
    if (is_ceiling)
        return sec->c_h;

    return sec->f_h;
}

static const image_t * SECPIC(sector_t * sec, bool is_ceiling,
                              const image_t *new_image)
{
    if (new_image)
    {
        if (is_ceiling) 
            sec->ceil.image = new_image;
        else
            sec->floor.image = new_image;

		if (new_image == skyflatimage)
			R_ComputeSkyHeights();
    }

    return is_ceiling ? sec->ceil.image : sec->floor.image;
}

//
// GetSecHeightReference
//
// Finds a sector height, using the reference provided; will select
// the approriate method of obtaining this value, if it cannot
// get it directly.
//
// -KM-  1998/09/01 Wrote Procedure.
// -ACB- 1998/09/06 Remarked and Reformatted.
// -ACB- 2001/02/04 Move to p_plane.c
//
static float GetSecHeightReference(heightref_e ref,
	sector_t * sec, sector_t * model)
{
    switch (ref & REF_MASK)
    {
        case REF_Absolute:
            return 0;

        case REF_Trigger:
			if (model)
				return (ref & REF_CEILING) ? model->c_h : model->f_h;

			return 0; // ick!

        case REF_Current:
            return (ref & REF_CEILING) ? sec->c_h : sec->f_h;

        case REF_Surrounding:
            return P_FindSurroundingHeight(ref, sec);

        case REF_LowestLoTexture:
            return P_FindRaiseToTexture(sec);

        default:
            I_Error("GetSecHeightReference: undefined reference %d\n", ref);
    }

    return 0;
}

//
// P_AddActivePart
//
// Adds to the tail of the list.
//
void P_AddActivePart(gen_move_t *movpart)
{
    gen_move_t *tmp;

    movpart->next = NULL;
  
    if (!active_movparts)
    {
        movpart->prev = NULL;
        active_movparts = movpart;
        return;
    }
  
    for(tmp = active_movparts; tmp->next; tmp = tmp->next)
    { /* Do Nothing */ };

    movpart->prev = tmp;
    tmp->next = movpart;
}

//
// P_RemoveActivePart
//
static void P_RemoveActivePart(gen_move_t *movpart)
{
///---    elev_move_t *elev;
    plane_move_t *plane;
    slider_move_t *slider;

    switch(movpart->whatiam)
    {
///---        case MDT_ELEVATOR:
///---            elev = (elev_move_t*)movpart;
///---            elev->sector->ceil_move = NULL;
///---            elev->sector->floor_move = NULL;
///---            break;

        case MDT_PLANE:
            plane = (plane_move_t*)movpart;
            if (plane->is_ceiling || plane->is_elevator)
                plane->sector->ceil_move = NULL;
            
            if (!plane->is_ceiling)
                plane->sector->floor_move = NULL;
            break;

        case MDT_SLIDER:
            slider = (slider_move_t*)movpart;
            slider->line->slider_move = NULL;
            break;

        default:
            break;
    }

    if (movpart->prev)
        movpart->prev->next = movpart->next;
    else
        active_movparts = movpart->next;

    if (movpart->next)
        movpart->next->prev = movpart->prev;
    
    Z_Free(movpart);
}


//
// P_RemoveAllActiveParts
//
// -ACB- This is a clear-the-decks function: we don't care
// with tyding up the loose ends inbetween removing active
// parts - that is pointless since we are nuking the entire
// thing. Hence the lack of call to P_RemoveActivePart() for
// each individual part.
//
void P_RemoveAllActiveParts(void)
{
    gen_move_t *movpart, *next;

    for (movpart = active_movparts; movpart; movpart = next)
    {
        next = movpart->next;
        Z_Free(movpart);            
    }
  
    active_movparts = NULL;
}

//
// FLOORS
//

//
// AttemptMovePlane
//
// Move a plane (floor or ceiling) and check for crushing
//
// Returns:
//    RES_Ok - the move was completely successful.
//
//    RES_Impossible - the move was not possible due to another solid
//    surface (e.g. an extrafloor) getting in the way.  The plane will
//    remain at its current height.
//
//    RES_PastDest - the destination height has been reached.  The
//    actual height in the sector may not be the target height, which
//    means some objects got in the way (whether crushed or not).
//
//    RES_Crushed - some objects got in the way.  When `crush'
//    parameter is true, those object will have been crushed (take
//    damage) and the plane height will be the new height, otherwise
//    the plane height will remain at its current height.
//
static move_result_e AttemptMovePlane(sector_t * sector, 
                                      float speed, float dest, bool crush, 
                                      bool is_ceiling, int direction)
{
    bool past = false;
    bool nofit;

    //
    // check whether we have gone past the destination height
    //
    if (direction == DIRECTION_UP && 
        HEIGHT(sector, is_ceiling) + speed > dest)
    {
        past = true;
        speed = dest - HEIGHT(sector, is_ceiling);
    }
    else if (direction == DIRECTION_DOWN && 
             HEIGHT(sector, is_ceiling) - speed < dest)
    {
        past = true;
        speed = HEIGHT(sector, is_ceiling) - dest;
    }
 
    if (speed <= 0)
        return RES_PastDest;

    if (direction == DIRECTION_DOWN)
        speed = -speed;

    // check if even possible
    if (! P_CheckSolidSectorMove(sector, is_ceiling, speed))
    {
        return RES_Impossible;
    }
   
    //
    // move the actual sector, including all things in it
    //
    nofit = P_SolidSectorMove(sector, is_ceiling, speed, crush, false);
    
    if (! nofit)
        return past ? RES_PastDest : RES_Ok;

    // bugger, something got in our way !
 
    if (! crush)
    {
        // undo the change
        P_SolidSectorMove(sector, is_ceiling, -speed, false, false);
    }

    return past ? RES_PastDest : RES_Crushed;
}

static move_result_e AttemptMoveSector(sector_t * sector,
		plane_move_t *pmov, float dest, bool crush)
{
	move_result_e res;

	if (! pmov->is_elevator)
	{
		return AttemptMovePlane(sector, pmov->speed, dest, crush,
				  pmov->is_ceiling, pmov->direction);
	}

	//-------------------//
	//   ELEVATOR MOVE   //
	//-------------------//

	if (pmov->direction == DIRECTION_UP)
	{
		AttemptMovePlane(sector, 32768.0, 
				MIN(sector->f_h + pmov->speed, dest) + pmov->elev_height,
				false, true, DIRECTION_UP);
	}

	res = AttemptMovePlane(sector, pmov->speed, dest, crush,
			  false, pmov->direction);
	
	if (pmov->direction == DIRECTION_DOWN)
	{
		AttemptMovePlane(sector, 32768.0, sector->f_h + pmov->elev_height,
				false, true, DIRECTION_DOWN);
	}

	return res;
}

//
// MovePlane
//
// Move a floor to it's destination (up or down).
//
static void MovePlane(plane_move_t *plane)
{
    move_result_e res;

    switch (plane->direction)
    {
        case DIRECTION_STASIS:
            if (plane->sfxstarted) 
            {
                // We've in stasis, therefore stop making any sound
                // -ACB- 2005/09/17 (Unashamed 11th hour hacking)
                sound::StopLoopingFX(&plane->sector->sfx_origin);
                plane->sfxstarted = false;
            }

            break;

        case DIRECTION_DOWN:
            res = AttemptMoveSector(plane->sector, plane,
                                   MIN(plane->startheight, plane->destheight),
                                   plane->crush && plane->is_ceiling);

            if (!plane->sfxstarted)
            {
                sound::StartFX(plane->type->sfxdown, 
                               SNCAT_Level,
                               &plane->sector->sfx_origin);

                plane->sfxstarted = true;
            }

            if (res == RES_PastDest)
            {
                sound::StartFX(plane->type->sfxstop, 
                               SNCAT_Level,
                               &plane->sector->sfx_origin);

                plane->speed = plane->type->speed_up;

                if (plane->newspecial != -1)
                {
					P_SectorChangeSpecial(plane->sector, plane->newspecial);
                }

                SECPIC(plane->sector, plane->is_ceiling, plane->new_image);

                switch (plane->type->type)
                {
                    case mov_Plat:
                    case mov_Continuous:
                        plane->direction = DIRECTION_WAIT;
                        plane->waited = plane->type->wait;
                        plane->speed = plane->type->speed_up;
                        break;

                    case mov_MoveWaitReturn:
                        if (HEIGHT(plane->sector, plane->is_ceiling) == plane->startheight)
                        {
                            P_RemoveActivePart((gen_move_t*)plane);
                        }
                        else  // assume we reached the destination
                        {
                            plane->direction = DIRECTION_WAIT;
                            plane->waited = plane->type->wait;
                            plane->speed = plane->type->speed_up;
                        }
                        break;

                    case mov_Toggle:
                        plane->direction = DIRECTION_STASIS;
                        plane->olddirection = DIRECTION_UP;
                        break;

                    default:
                        P_RemoveActivePart((gen_move_t*)plane);
                        break;
                }
            }
            else if (res == RES_Crushed || res == RES_Impossible)
            {
                if (plane->crush)
                {
                    plane->speed = plane->type->speed_down / 8;
                }
                else if (plane->type->type == mov_MoveWaitReturn)  // Go back up
                {
                    plane->direction = DIRECTION_UP;
                    plane->sfxstarted = false;
                    plane->waited = 0;
                    plane->speed = plane->type->speed_up;
                }
            }

            break;

        case DIRECTION_WAIT:
            if (--plane->waited <= 0)
            {
                int dir;
                float dest;

                if (HEIGHT(plane->sector, plane->is_ceiling) == plane->destheight)
                    dest = plane->startheight;
                else
                    dest = plane->destheight;

                if (HEIGHT(plane->sector, plane->is_ceiling) > dest)
                {
                    dir = DIRECTION_DOWN;
                    plane->speed = plane->type->speed_down;
                }
                else
                {
                    dir = DIRECTION_UP;
                    plane->speed = plane->type->speed_up;
                }

                if (dir)
                {
                    sound::StartFX(plane->type->sfxstart, 
                                   SNCAT_Level,
                                   &plane->sector->sfx_origin);
                }

                plane->direction = dir;  // time to go back
                plane->sfxstarted = false;
            }
            break;

        case DIRECTION_UP:
            res = AttemptMoveSector(plane->sector, plane,
                                   MAX(plane->startheight, plane->destheight),
                                   plane->crush && !plane->is_ceiling);

            if (!plane->sfxstarted)
            {
                sound::StartFX(plane->type->sfxup, 
                               SNCAT_Level,
                               &plane->sector->sfx_origin);

                plane->sfxstarted = true;
            }

            if (res == RES_PastDest)
            {
                sound::StartFX(plane->type->sfxstop, 
                               SNCAT_Level,
                               &plane->sector->sfx_origin);

                if (plane->newspecial != -1)
                {
					P_SectorChangeSpecial(plane->sector, plane->newspecial);
                }

                SECPIC(plane->sector, plane->is_ceiling, plane->new_image);

                switch (plane->type->type)
                {
                    case mov_Plat:
                    case mov_Continuous:
                        plane->direction = DIRECTION_WAIT;
                        plane->waited = plane->type->wait;
                        plane->speed = plane->type->speed_down;
                        break;

                    case mov_MoveWaitReturn:
                        if (HEIGHT(plane->sector, plane->is_ceiling) == plane->startheight)
                        {
                            P_RemoveActivePart((gen_move_t*)plane);
                        }
                        else  // assume we reached the destination
                        {
                            plane->direction = DIRECTION_WAIT;
                            plane->speed = plane->type->speed_down;
                            plane->waited = plane->type->wait;
                        }
                        break;

                    case mov_Toggle:
                        plane->direction = DIRECTION_STASIS;
                        plane->olddirection = DIRECTION_DOWN;
                        break;

                    default:
                        P_RemoveActivePart((gen_move_t*)plane);
                        break;
                }

            }
            else if (res == RES_Crushed || res == RES_Impossible)
            {
                if (plane->crush)
                {
                    plane->speed = plane->type->speed_up / 8;
                }
                else if (plane->type->type == mov_MoveWaitReturn)  // Go back down
                {
                    plane->direction = DIRECTION_DOWN;
                    plane->sfxstarted = false;
                    plane->waited = 0;
                    plane->speed = plane->type->speed_down;
                }
            }
            break;

        default:
            I_Error("MovePlane: Unknown direction %d", plane->direction);
    }
}

//
// P_RunActiveSectors
//
// Executes one tic's plane_move_t thinking.
// Active sectors can destroy themselves, but not each other.
// We do not have to bother about a removal queue, but we can not rely on
// sec still being in memory after MovePlane.
//
// -AJA- 2000/08/06 Now handles horizontal sliding doors too.
// -ACB- 2001/01/14 Now handles elevators too.
// -ACB- 2001/02/08 Now generic routine with gen_move_t;
//
void P_RunActiveSectors(void)
{
    gen_move_t *part, *part_next;

    for (part = active_movparts; part; part = part_next)
    {
        part_next = part->next;

        switch (part->whatiam)
        {
            case MDT_PLANE:
                MovePlane((plane_move_t*)part);
                break;

            case MDT_SLIDER:
                MoveSlider((slider_move_t*)part);
                break;

            default:
                break;
        }
    }
}

//
// P_GSS
//
static sector_t *P_GSS(sector_t * sec, float dest, bool forc)
{
    int i;
    int secnum = sec - sectors;
    sector_t *sector;

    for (i = sec->linecount-1; i; i--)
    {
        if (P_TwoSided(secnum, i))
        {
            if (P_GetSide(secnum, i, 0)->sector - sectors == secnum)
            {
                sector = P_GetSector(secnum, i, 1);

                if (SECPIC(sector, forc, NULL) != SECPIC(sec, forc, NULL)
                    && HEIGHT(sector, forc) == dest)
                {
                    return sector;
                }

            }
            else
            {
                sector = P_GetSector(secnum, i, 0);
        
                if (SECPIC(sector, forc, NULL) != SECPIC(sec, forc, NULL)
                    && HEIGHT(sector, forc) == dest)
                {
                    return sector;
                }
            }
        }
    }

    for (i = sec->linecount; i--;)
    {
        if (P_TwoSided(secnum, i))
        {
            if (P_GetSide(secnum, i, 0)->sector - sectors == secnum)
            {
                sector = P_GetSector(secnum, i, 1);
            }
            else
            {
                sector = P_GetSector(secnum, i, 0);
            }
            if (sector->validcount != validcount)
            {
                sector->validcount = validcount;
                sector = P_GSS(sector, dest, forc);
                if (sector)
                    return sector;
            }
        }
    }

    return NULL;
}

//
// P_GetSectorSurrounding
//
static sector_t *P_GetSectorSurrounding(sector_t * sec, float dest, bool forc)
{
    validcount++;
    sec->validcount = validcount;
    return P_GSS(sec, dest, forc);
}

//
// P_SetupPlaneDirection
//
void P_SetupPlaneDirection(plane_move_t *plane, const movplanedef_c *def, 
                           float start, float dest)
{
    plane->startheight = start;
    plane->destheight = dest;

    if (dest > start)
    {
        plane->direction = DIRECTION_UP;

        if (def->speed_up >= 0)
            plane->speed = def->speed_up;
        else
            plane->speed = dest - start;
    }
    else if (start > dest)
    {
        plane->direction = DIRECTION_DOWN;

        if (def->speed_down >= 0)
            plane->speed = def->speed_down;
        else
            plane->speed = start - dest;
    }

    return;
}

//
// P_SetupSectorAction
//
// Setup the Floor Action, depending on the linedeftype trigger and the
// sector info.
//
static plane_move_t *P_SetupSectorAction(sector_t * sector, 
                                         const movplanedef_c * def, 
                                         sector_t * model)
{
    plane_move_t *plane;
    float start, dest;

    // new door thinker
    plane = Z_New(plane_move_t, 1);

    if (def->is_ceiling)
        sector->ceil_move = (gen_move_t*)plane;
    else
        sector->floor_move = (gen_move_t*)plane;

    plane->whatiam = MDT_PLANE;
    plane->sector = sector;
    plane->crush = def->crush;
    plane->sfxstarted = false;

    start = HEIGHT(sector, def->is_ceiling);

    dest = GetSecHeightReference(def->destref, sector, model);
    dest += def->dest;

    if (def->type == mov_Plat || def->type == mov_Continuous ||
        def->type == mov_Toggle)
    {
        start = GetSecHeightReference(def->otherref, sector, model);
        start += def->other;
    }

#if 0  // DEBUG
    L_WriteDebug("SEC_ACT: %d type %d %s start %1.0f dest %1.0f\n",
                 sector - sectors, def->type, 
                 def->is_ceiling ? "CEIL" : "FLOOR", 
                 start, dest);
#endif

    if (def->prewait)
    {
        plane->direction = DIRECTION_WAIT;
        plane->waited = def->prewait;
        plane->destheight = dest;
        plane->startheight = start;
    }
    else if (def->type == mov_Continuous)
    {
        plane->direction = (P_Random() & 1) ? DIRECTION_UP : DIRECTION_DOWN;

        if (plane->direction == DIRECTION_UP)
            plane->speed = def->speed_up;
        else
            plane->speed = def->speed_down;

        plane->destheight = dest;
        plane->startheight = start;
    }
    else if (start != dest)
    {
        P_SetupPlaneDirection(plane, def, start, dest);
    }
    else
    {
        if (def->is_ceiling)
            sector->ceil_move = NULL;
        else
            sector->floor_move = NULL;

        Z_Free(plane);
        return NULL;
    }

    plane->tag = sector->tag;
    plane->type = def;
    plane->new_image = SECPIC(sector, def->is_ceiling, NULL);
    plane->newspecial = -1;
    plane->is_ceiling = def->is_ceiling;
	plane->is_elevator = (def->type == mov_Elevator);
	plane->elev_height = sector->c_h - sector->f_h;

    // -ACB- 10/01/2001 Trigger starting sfx
    sound::StopLoopingFX(&sector->sfx_origin);
    if (def->sfxstart)
    {
        sound::StartFX(def->sfxstart, 
                       SNCAT_Level,
                       &sector->sfx_origin);
    }

    // change to surrounding
    if (def->tex[0] == '-')
    {
        model = P_GetSectorSurrounding(sector, 
                                       plane->destheight, 
                                       def->is_ceiling);
        if (model)
        {
            plane->new_image = SECPIC(model, def->is_ceiling, NULL);

            plane->newspecial = model->props.special ?
                model->props.special->ddf.number : 0;
        }

        if (plane->direction == (def->is_ceiling ? DIRECTION_DOWN : DIRECTION_UP))
        {
            SECPIC(sector, def->is_ceiling, plane->new_image);
            if (plane->newspecial != -1)
            {
				P_SectorChangeSpecial(sector, plane->newspecial);
            }
        }
    }
    else if (def->tex[0] == '+')
    {
        if (model)
        {
            if (SECPIC(model,  def->is_ceiling, NULL) == 
                SECPIC(sector, def->is_ceiling, NULL))
            {
                model = P_GetSectorSurrounding(model, plane->destheight,
                                               def->is_ceiling);
            }
        }

        if (model)
        {
            plane->new_image = SECPIC(model, def->is_ceiling, NULL);
            plane->newspecial = model->props.special ?
                model->props.special->ddf.number : 0;

            if (plane->direction == (def->is_ceiling ? DIRECTION_DOWN : DIRECTION_UP))
            {
                SECPIC(sector, def->is_ceiling, plane->new_image);

                if (plane->newspecial != -1)
                {
					P_SectorChangeSpecial(sector, plane->newspecial);
                }
            }
        }
    }
    else if (def->tex[0])
    {
        plane->new_image = W_ImageLookup(def->tex, INS_Flat);
    }

    P_AddActivePart((gen_move_t*)plane);
    return plane;
}


//
// EV_BuildOneStair
//
// BUILD A STAIRCASE!
//
// -AJA- 1999/07/04: Fixed the problem on MAP20. The next stair's
// dest height should be relative to the previous stair's dest height
// (and not just the current height).
//
// -AJA- 1999/07/29: Split into two functions. The old code could do bad
// things (e.g. skip a whole staircase) when 2 or more stair sectors
// were tagged.
//
static bool EV_BuildOneStair(sector_t * sec, const movplanedef_c * def)
{
    int i;
    float next_height;
    bool more;

    plane_move_t *step;
    sector_t *tsec;
    float stairsize = def->dest;

    const image_t *image = sec->floor.image;

    // new floor thinker

    step = P_SetupSectorAction(sec, def, sec);
    if (!step)
        return false;

    next_height = step->destheight + stairsize;

    do
    {
        more = false;

        // Find next sector to raise
        //
        // 1. Find 2-sided line with same sector side[0]
        // 2. Other side is the next sector to raise
        //
        for (i = 0; i < sec->linecount; i++)
        {
            if (! (sec->lines[i]->flags & ML_TwoSided))
                continue;

            if (sec != sec->lines[i]->frontsector)
                continue;

            if (sec == sec->lines[i]->backsector)
                continue;

            tsec = sec->lines[i]->backsector;

            if (tsec->floor.image != image && !def->ignore_texture)
                continue;

            if (def->is_ceiling && tsec->ceil_move)
                continue;
            if (!def->is_ceiling && tsec->floor_move)
                continue;

            step = P_SetupSectorAction(tsec, def, tsec);
            if (step)
            {
                // Override the destination height
                P_SetupPlaneDirection(step, def, 
                                      step->startheight, 
                                      next_height);

                next_height += stairsize;
                sec = tsec;
                more = true;
            }

            break;
        }
    }
    while (more);

    return true;
}

//
// EV_BuildStairs
//
static bool EV_BuildStairs(sector_t * sec, const movplanedef_c * def)
{
    bool rtn = false;

    while (sec->tag_prev)
        sec = sec->tag_prev;

    for (; sec; sec = sec->tag_next)
    {
        // Already moving?  If so, keep going...
        if (def->is_ceiling && sec->ceil_move)
            continue;
		if (!def->is_ceiling && sec->floor_move)
            continue;

        if (EV_BuildOneStair(sec, def))
            rtn = true;
    }

    return rtn;
}

//
// EV_DoPlane
//
// Do Platforms/Floors/Stairs/Ceilings/Doors/Elevators
//
bool EV_DoPlane(sector_t * sec, const movplanedef_c * def, sector_t * model)
{
    // Activate all <type> plats that are in_stasis
    switch (def->type)
    {
        case mov_Plat:
        case mov_Continuous:
        case mov_Toggle:
            if (P_ActivateInStasis(sec->tag))
                return true;
            break;

        case mov_Stairs:
            return EV_BuildStairs(sec, def);

        case mov_Stop:
            return P_StasifySector(sec);

        default:
            break;
    }

    if (def->is_ceiling || def->type == mov_Elevator)
    {
        if (sec->ceil_move)
            return false;
    }

    if (!def->is_ceiling)
    {
        if (sec->floor_move)
            return false;
    }

    // Do Floor action
    return P_SetupSectorAction(sec, def, model) ? true : false;
}

//
// EV_ManualPlane
//
bool EV_ManualPlane(line_t * line, mobj_t * thing, const movplanedef_c * def)
{
    sector_t *sec;
    plane_move_t *msec;
    int side;
    int dir = DIRECTION_UP;
    int olddir = DIRECTION_UP;

    side = 0;  // only front sides can be used

    // if the sector has an active thinker, use it
    sec = side ? line->frontsector : line->backsector;
    if (!sec)
        return false;

    if (def->is_ceiling)
        msec = (plane_move_t *)sec->ceil_move;
    else
        msec = (plane_move_t *)sec->floor_move;

    if (msec && thing)
    {
        switch (def->type)
        {
            case mov_MoveWaitReturn:
                olddir = msec->direction;

                // Only players close doors
                if ((msec->direction != DIRECTION_DOWN) && thing->player)
                    dir = msec->direction = DIRECTION_DOWN;
                else
                    dir = msec->direction = DIRECTION_UP;
                break;
        
            default:
                break;
        }

        if (dir != olddir)
        {
            sound::StartFX(def->sfxstart, 
                           SNCAT_Level,
                           &sec->sfx_origin);

            msec->sfxstarted = !(thing->player);
            return true;
        }

        return false;
    }

    return EV_DoPlane(sec, def, sec);
}

//
// P_ActivateInStasis
//
static bool P_ActivateInStasis(int tag)
{
    bool rtn;
    gen_move_t *movpart;
    plane_move_t *plane;

    rtn = false;
    for (movpart = active_movparts; movpart; movpart = movpart->next)
    {
        if (movpart->whatiam == MDT_PLANE)
        {
            plane = (plane_move_t*)movpart;
            if(plane->direction == DIRECTION_STASIS && plane->tag == tag)
            {
                plane->direction = plane->olddirection;
                rtn = true;
            }
        }
    }

    return rtn;
}

//
// P_StasifySector
//
static bool P_StasifySector(sector_t * sec)
{
    bool rtn;
    gen_move_t *movpart;
    plane_move_t *plane;

    rtn = false;
    for (movpart = active_movparts; movpart; movpart = movpart->next)
    {
        if (movpart->whatiam == MDT_PLANE)
        {
            plane = (plane_move_t*)movpart;
            if(plane->direction != DIRECTION_STASIS && plane->tag == sec->tag)
            {
                plane->olddirection = plane->direction;
                plane->direction = DIRECTION_STASIS;
                rtn = true;
            }
        }
    }

    return rtn;
}

// -AJA- 1999/12/07: cleaned up this donut stuff

linetype_c donut[2];
static int donut_setup = 0;

//
// EV_DoDonut
//
// Special Stuff that can not be categorized
// Mmmmmmm....  Donuts....
//
bool EV_DoDonut(sector_t * s1, sfx_t *sfx[4])
{
    sector_t *s2;
    sector_t *s3;
    bool result = false;
    int i;
    plane_move_t *sec;

    if (! donut_setup)
    {
        donut[0].Default();
        donut[0].count = 1;
        donut[0].specialtype = 0;
        donut[0].f.Default(movplanedef_c::DEFAULT_DonutFloor);
        donut[0].f.tex.Set("-");

        donut[1].Default();
        donut[1].count = 1;
        donut[1].f.Default(movplanedef_c::DEFAULT_DonutFloor);
        donut[1].f.dest = -32000.0f;

        donut_setup++;
    }
  
    // ALREADY MOVING?  IF SO, KEEP GOING...
    if (s1->floor_move)
        return false;

    s2 = P_GetNextSector(s1->lines[0], s1);

    for (i = 0; i < s2->linecount; i++)
    {
        if (!(s2->lines[i]->flags & ML_TwoSided) || (s2->lines[i]->backsector == s1))
            continue;

        s3 = s2->lines[i]->backsector;

        result = true;

        // Spawn rising slime
        donut[0].f.sfxup = sfx[0];
        donut[0].f.sfxstop = sfx[1];
    
        sec = P_SetupSectorAction(s2, &donut[0].f, s3);

        if (sec)
        {
            sec->destheight = s3->f_h;
            s2->floor.image = sec->new_image = s3->floor.image;

            /// s2->props.special = s3->props.special;
			P_SectorChangeSpecial(s2, s3->props.type);
        }

        // Spawn lowering donut-hole
        donut[1].f.sfxup = sfx[2];
        donut[1].f.sfxstop = sfx[3];

        sec = P_SetupSectorAction(s1, &donut[1].f, s1);

        if (sec)
            sec->destheight = s3->f_h;
        break;
    }

    return result;
}

//
// SliderCanClose
//
static INLINE bool SliderCanClose(line_t *line)
{
    return ! P_ThingsOnLine(line);
}

//
// MoveSlider
//
static void MoveSlider(slider_move_t *smov)
{
    sector_t *sec = smov->line->frontsector;

    switch (smov->direction)
    {
        // WAITING
        case 0:
            if (--smov->waited <= 0)
            {
                if (SliderCanClose(smov->line))
                {
                    sound::StartFX(smov->info->sfx_start, 
                                   SNCAT_Level,
                                   &sec->sfx_origin);

                    smov->sfxstarted = false;
                    smov->direction = DIRECTION_DOWN;
                }
                else
                {
                    // try again soon
                    smov->waited = TICRATE / 3;
                }
            }
            break;

		// OPENING
        case 1:
            if (! smov->sfxstarted)
            {
                sound::StartFX(smov->info->sfx_open, 
                               SNCAT_Level,
                               &sec->sfx_origin);

                smov->sfxstarted = true;
            }

            smov->opening += smov->info->speed;

            // mark line as non-blocking (at some point)
            P_ComputeGaps(smov->line);

            if (smov->opening >= smov->target)
            {
                sound::StartFX(smov->info->sfx_stop, 
                               SNCAT_Level,
                               &sec->sfx_origin);

                smov->opening = smov->target;
                smov->direction = DIRECTION_WAIT;
                smov->waited = smov->info->wait;

                if (smov->final_open)
                {
                    line_t *ld = smov->line;

                    // clear line special
                    ld->special = NULL;

                    P_RemoveActivePart((gen_move_t*)smov);

                    // clear the side textures
                    ld->side[0]->middle.image = NULL;
                    ld->side[1]->middle.image = NULL;

                    P_ComputeWallTiles(ld, 0);
                    P_ComputeWallTiles(ld, 1);

                    return;
                }
            }
            break;

		// CLOSING
        case -1:
            if (! smov->sfxstarted)
            {
                sound::StartFX(smov->info->sfx_close, 
                               SNCAT_Level,
                               &sec->sfx_origin);

                smov->sfxstarted = true;
            }

            smov->opening -= smov->info->speed;

            // mark line as blocking (at some point)
            P_ComputeGaps(smov->line);

            if (smov->opening <= 0.0f)
            {
                sound::StartFX(smov->info->sfx_stop, 
                               SNCAT_Level,
                               &sec->sfx_origin);

                P_RemoveActivePart((gen_move_t*)smov);
                return;
            }
            break;

        default:
            I_Error("MoveSlider: Unknown direction %d", smov->direction);
    }
}

//
// EV_DoSlider
//
// Handle thin horizontal sliding doors.
//
void EV_DoSlider(line_t * line, mobj_t * thing, const sliding_door_c * s)
{
    sector_t *sec = line->frontsector;
    slider_move_t *smov;

    if (! thing || ! sec || ! line->side[0] || ! line->side[1])
        return;

    // if the line has an active thinker, use it
    if (line->slider_move)
    {
        smov = line->slider_move;

        // only players close doors
        if (smov->direction == DIRECTION_WAIT && thing->player)
        {
            smov->waited = 0;
        }
        return;
    }

    // new sliding door thinker
    smov = Z_New(slider_move_t, 1);

    smov->whatiam = MDT_SLIDER;
    smov->info = &line->special->s;
    smov->line = line;
    smov->opening = 0.0f;
    smov->line_len = R_PointToDist(0, 0, line->dx, line->dy);
    smov->target = smov->line_len * PERCENT_2_FLOAT(smov->info->distance);

    smov->direction = DIRECTION_UP;
    smov->sfxstarted = ! thing->player;
    smov->final_open = (line->count == 1);

    line->slider_move = smov;

    P_AddActivePart((gen_move_t*)smov);

    sound::StartFX(s->sfx_start, SNCAT_Level, &sec->sfx_origin);

    // Must handle line count here, since the normal code in p_spec.c
    // will clear the line->special pointer, confusing various bits of
    // code that deal with sliding doors (--> crash).
    // 
    if (line->count > 0)
        line->count--;
}


#if 0  // ELEVATOR CRUD


//
// AttemptMoveElevator
//
static move_result_e AttemptMoveElevator(sector_t *sec, float speed, 
                                         float dest, int direction)
{
#if 0  // -AJA- FIXME: exfloorlist[] removed
    move_result_e res;
    bool didnotfit;
    float currdest;
    float lastfh;
    float lastch;
    float diff;
    sector_t *parentsec;
    int i;

    res = RES_Ok;

    currdest = 0.0f;

    if (direction == DIRECTION_UP)
        currdest = sec->c_h + speed;
    else if (direction == DIRECTION_DOWN)
        currdest = sec->f_h - speed;

    for (i=0; i<sec->exfloornum; i++)
    {
        parentsec = sec->exfloorlist[i];

        if (direction == DIRECTION_UP)
        {
            if (currdest > dest)
            {
                lastch = sec->c_h;
                lastfh = sec->f_h;

                diff = lastch - dest;

                sec->c_h = dest;
                sec->f_h -= diff;
                didnotfit = P_ChangeSector(sec, false);
                if (didnotfit)
                {
                    sec->c_h = lastch;
                    sec->f_h = lastfh;
                    P_ChangeSector(sec, false);
                }
                res = RES_PastDest;
            } 
            else
            {
                lastch = sec->c_h;
                lastfh = sec->f_h;

                diff = lastch - currdest;

                sec->c_h = currdest;
                sec->f_h -= diff;
                didnotfit = P_ChangeSector(sec, false);
                if (didnotfit)
                {
                    sec->c_h = lastch;
                    sec->f_h = lastfh;
                    P_ChangeSector(sec, false);
                    res = RES_PastDest;
                }
            }
        }
        else if (direction == DIRECTION_DOWN)
        {
            if (currdest < dest)
            {
                lastch = sec->c_h;
                lastfh = sec->f_h;

                diff = lastfh - dest;

                sec->c_h -= diff;
                sec->f_h = dest;
                didnotfit = P_ChangeSector(sec, false);
                if (didnotfit)
                {
                    sec->c_h = lastch;
                    sec->f_h = lastfh;
                    P_ChangeSector(sec, false);
                }
                res = RES_PastDest;
            } 
            else
            {
                lastch = sec->c_h;
                lastfh = sec->f_h;

                diff = lastfh - currdest;

                sec->c_h -= diff;
                sec->f_h = currdest;
                didnotfit = P_ChangeSector(sec, false);
                if (didnotfit)
                {
                    sec->c_h = lastch;
                    sec->f_h = lastfh;
                    P_ChangeSector(sec, false);
                    res = RES_PastDest;
                }
            }
        }
    }
    return res;
#endif

    return RES_Ok;
}

//
// MoveElevator
//
static void MoveElevator(elev_move_t *elev)
{
    move_result_e res;
    float num;

    switch (elev->direction)
    {
        case DIRECTION_DOWN:
            res = AttemptMoveElevator(elev->sector,
                                      elev->speed,
                                      elev->destheight,
                                      elev->direction);

            if (!elev->sfxstarted)
            {    
                sound::StartFX(elev->type->sfxdown, 
                               SNCAT_Level, 
                               &elev->sector->sfx_origin);

                elev->sfxstarted = true;
            }

            if (res == RES_PastDest || res == RES_Impossible)
            {
                sound::StartFX(elev->type->sfxstop, 
                               SNCAT_Level, 
                               &elev->sector->sfx_origin);

                elev->speed = elev->type->speed_up;

// ---> ACB 2001/03/25 Quick hack to get continous movement
//              P_RemoveActivePart((gen_move_t*)elev);
                elev->direction = DIRECTION_UP;

                num = elev->destheight;
                elev->destheight = elev->startheight;
                elev->startheight = num;
// ---> ACB 2001/03/25 Quick hack to get continous movement
            }
            break;
      
        case DIRECTION_WAIT:
            break;
      
        case DIRECTION_UP:
            res = AttemptMoveElevator(elev->sector,
                                      elev->speed,
                                      elev->destheight,
                                      elev->direction);

            if (!elev->sfxstarted)
            {
                sound::StartFX(elev->type->sfxdown, 
                               SNCAT_Level, 
                               &elev->sector->sfx_origin);

                elev->sfxstarted = true;
            }

            if (res == RES_PastDest || res == RES_Impossible)
            {
                sound::StartFX(elev->type->sfxstop, 
                               SNCAT_Level, 
                               &elev->sector->sfx_origin);

                elev->speed = elev->type->speed_down;

// ---> ACB 2001/03/25 Quick hack to get continous movement
//              P_RemoveActivePart((gen_move_t*)elev);
                elev->direction = DIRECTION_DOWN; 
                num = elev->destheight;
                elev->destheight = elev->startheight;
                elev->startheight = num;
// ---> ACB 2001/03/25 Quick hack to get continous movement

            }
            break;
      
        default:
            break;
    }

    return;
}

#endif  // ELEVATOR CRUD

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
