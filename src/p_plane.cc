//----------------------------------------------------------------------------
//  EDGE Floor/Ceiling/Stair/Elevator Action Code
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
// -KM-  1998/09/01 Changed for DDF.
// -ACB- 1998/09/13 Moved the teleport procedure here
//

#include "i_defs.h"

#include "z_zone.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "m_random.h"
#include "p_local.h"
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

static elev_move_t *P_SetupElevatorAction(sector_t * sector, 
                                          const elevatordef_c * type, sector_t * model);

static void MoveElevator(elev_move_t *elev);
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
static float GetSecHeightReference(heightref_e ref, sector_t * sec)
{
    switch (ref & REF_MASK)
    {
        case REF_Absolute:
            return 0;

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

#if 0   // Unfinished
//
// GetElevatorHeightReference
//
// This is essentially the same as above, but used for elevator
// calculations. The reason behind this is that elevators have to
// work as dummy sectors.
//
// -ACB- 2001/02/04 Written
//
static float GetElevatorHeightReference(heightref_e ref, sector_t * sec)
{
    return -1;
}
#endif

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
    elev_move_t *elev;
    plane_move_t *plane;
    slider_move_t *slider;

    switch(movpart->whatiam)
    {
        case MDT_ELEVATOR:
            elev = (elev_move_t*)movpart;
            elev->sector->ceil_move = NULL;
            elev->sector->floor_move = NULL;
            break;

        case MDT_PLANE:
            plane = (plane_move_t*)movpart;
            if (plane->is_ceiling)
                plane->sector->ceil_move = NULL;
            else
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
            plane->sfxstarted = false;
            break;

        case DIRECTION_DOWN:
            res = AttemptMovePlane(plane->sector, plane->speed,
                                   MIN(plane->startheight, plane->destheight),
                                   plane->crush && plane->is_ceiling,
                                   plane->is_ceiling, plane->direction);

            if (!plane->sfxstarted)
            {
                S_StartSound((mobj_t *) &plane->sector->soundorg, 
                             plane->type->sfxdown);
                plane->sfxstarted = true;
            }

            if (res == RES_PastDest)
            {
                S_StartSound((mobj_t *) &plane->sector->soundorg, 
                             plane->type->sfxstop);
                plane->speed = plane->type->speed_up;

                if (plane->newspecial != -1)
                {
                    plane->sector->props.special = (plane->newspecial <= 0) ? NULL :
                        playsim::LookupSectorType(plane->newspecial);
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
                    case mov_Stairs:
                    case mov_Once:
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
                    S_StartSound((mobj_t *) &plane->sector->soundorg,
                                 plane->type->sfxstart);
                }

                plane->direction = dir;  // time to go back
                plane->sfxstarted = false;
            }
            break;

        case DIRECTION_UP:
            res = AttemptMovePlane(plane->sector, plane->speed,
                                   MAX(plane->startheight, plane->destheight),
                                   plane->crush && !plane->is_ceiling,
                                   plane->is_ceiling, plane->direction);

            if (!plane->sfxstarted)
            {
                S_StartSound((mobj_t *) &plane->sector->soundorg, 
                             plane->type->sfxup);
                plane->sfxstarted = true;
            }

            if (res == RES_PastDest)
            {
                S_StartSound((mobj_t *) &plane->sector->soundorg, 
                             plane->type->sfxstop);

                if (plane->newspecial != -1)
                {
                    plane->sector->props.special = (plane->newspecial <= 0) ? NULL :
                        sectortypes.Lookup(plane->newspecial);
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
                    case mov_Once:
                    case mov_Stairs:
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
            case MDT_ELEVATOR:
                MoveElevator((elev_move_t*)part);
                break;

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

    dest = GetSecHeightReference(def->destref, sector);
    dest += def->dest;

    if (def->type == mov_Plat || def->type == mov_Continuous ||
        def->type == mov_Toggle)
    {
        start = GetSecHeightReference(def->otherref, sector);
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

    // -ACB- 10/01/2001 Trigger starting sfx
    S_StopLoopingSound((mobj_t *) & sector->soundorg);
    if (def->sfxstart)
        S_StartSound((mobj_t *) & sector->soundorg, def->sfxstart);

    // change to surrounding
    if (def->tex[0] == '-')
    {
        model = P_GetSectorSurrounding(sector, plane->destheight, def->is_ceiling);
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
                sector->props.special = (plane->newspecial <= 0) ? NULL :
                    sectortypes.Lookup(plane->newspecial);
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
                    sector->props.special = (plane->newspecial <= 0) ? NULL :
                        sectortypes.Lookup(plane->newspecial);
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

//----------------------------------------------------------------------------
// TELEPORT CODE       (FIXME: move to separate file ?)
//----------------------------------------------------------------------------

#define TELE_FUDGE  0.1f

static mobj_t *FindTeleportMan(int tag, const mobjtype_c *info)
{
    for (int i = 0; i < numsectors; i++)
    {
        if (sectors[i].tag != tag)
            continue;

        for (subsector_t *sub = sectors[i].subsectors; sub; sub = sub->sec_next)
        {
            for (mobj_t *mo = sub->thinglist; mo; mo = mo->snext)
                if (mo->info == info)
                    return mo;
        }
    }

    return NULL;  // not found
}

static line_t *FindTeleportLine(int tag, line_t *original)
{
    for (int i = 0; i < numlines; i++)
    {
        if (lines[i].tag != tag)
            continue;

        if (lines + i == original)
            continue;

        return lines + i;
    }

    return NULL;  // not found
}

//
// EV_Teleport
//
// Teleportation is an effect which is simulated by searching for the first
// special[MOBJ_TELEPOS] in a sector with the same tag as the activation line,
// moving an object from one sector to another upon the MOBJ_TELEPOS found, and
// possibly spawning an effect object (i.e teleport flash) at either the entry &
// exit points or both.
//
// -KM- 1998/09/01 Added stuff for lines.ddf (mostly sounds)
//
// -ACB- 1998/09/11 Reformatted and cleaned up.
//
// -ACB- 1998/09/12 Teleport delay setting from linedef.
//
// -ACB- 1998/09/13 used effect objects: the objects themselves make any sound and
//                  the in effect object can be different to the out object.
//
// -ACB- 1998/09/13 Removed the missile checks: no need since this would have been
//                  Checked at the linedef stage.
//
// -KM- 1998/11/25 Changed Erik's code a bit, Teleport flash still appears.
//  if def faded_teleportation == 1, doesn't if faded_teleportation == 2
//
// -ES- 1998/11/28 Changed Kester's code a bit :-) Teleport method can now be
//  toggled in the menu. (That is the way it should be. -KM)
//
// -KM- 1999/01/31 Search only the target sector, not the entire map.
//
// -AJA- 1999/07/12: Support for TELEPORT_SPECIAL in lines.ddf.
// -AJA- 1999/07/30: Updated for extra floor support.
// -AJA- 1999/10/21: Allow line to be NULL, and added `tag' param.
// -AJA- 2004/10/08: Reworked for Silent and Line-to-Line teleporters
//                   (based on the logic in prBoom's p_telept.c code).
//
bool EV_Teleport(line_t* line, int tag, mobj_t* thing,
                 const teleportdef_c *def)
{
    if (!thing)
        return false;

    float oldx = thing->x;
    float oldy = thing->y;
    float oldz = thing->z;

    float new_x;
    float new_y;
    float new_z;

    angle_t new_ang;

    angle_t dest_ang;
    angle_t source_ang = ANG90 + (line ? R_PointToAngle(0, 0, line->dx, line->dy) : 0);

    mobj_t *currmobj = NULL;
    line_t *currline = NULL;

	bool flipped = (def->special & TELSP_Flipped) ? true : false;

    if (def->special & TELSP_Line)
    {
        if (!line || tag <= 0)
            return false;

        currline = FindTeleportLine(tag, line);

        if (! currline)
            return false;

        new_x = currline->v1->x + currline->dx / 2.0f;
        new_y = currline->v1->y + currline->dy / 2.0f;

        new_z = currline->frontsector ? currline->frontsector->f_h : -32000;

        if (currline->backsector)
            new_z = MAX(new_z, currline->backsector->f_h);

        dest_ang = R_PointToAngle(0, 0, currline->dx, currline->dy) + ANG90;

		flipped = ! flipped;  // match Boom's logic
    }
    else  /* thing-based teleport */
    {
        if (! def->outspawnobj)
            return false;

        currmobj = FindTeleportMan(tag, def->outspawnobj);

        if (! currmobj)
            return false;

        new_x = currmobj->x;
        new_y = currmobj->y;
        new_z = currmobj->z;

        dest_ang = currmobj->angle;
    }

    /* --- Angle handling --- */

    if (flipped)
        dest_ang += ANG180;

    if (def->special & TELSP_Relative)
        new_ang = thing->angle + (dest_ang - source_ang);
    else if (def->special & TELSP_SameAbsDir)
        new_ang = thing->angle;
    else if (def->special & TELSP_Rotate)
        new_ang = thing->angle + dest_ang;
    else
        new_ang = dest_ang;

    /* --- Offset handling --- */

    if (line && def->special & TELSP_SameOffset)
    {
        float dx = 0;
        float dy = 0;

        float pos = 0;

        if (fabs(line->dx) > fabs(line->dy))
            pos = (oldx - line->v1->x) / line->dx;
        else
            pos = (oldy - line->v1->y) / line->dy;

        if (currline)
        {
            dx = currline->dx * (pos - 0.5f);
            dy = currline->dy * (pos - 0.5f);

            if (flipped)
            {
                dx = -dx; dy = -dy;
            }

            new_x += dx;
            new_y += dy;

            // move a little distance away from the line, in case that line
            // is special (e.g. another teleporter), in order to prevent it
            // from being triggered.

            new_x += TELE_FUDGE * M_Cos(dest_ang);
            new_y += TELE_FUDGE * M_Sin(dest_ang);
        }
        else if (currmobj)
        {
            dx = line->dx * (pos - 0.5f);
            dy = line->dy * (pos - 0.5f);

            // we need to rotate the offset vector
            angle_t offset_ang = dest_ang - source_ang;

            float s = M_Sin(offset_ang);
            float c = M_Cos(offset_ang);

            new_x += dx * c - dy * s;
            new_y += dy * c + dx * s;
        }
    }

    if (def->special & TELSP_SameHeight)
    {
        new_z += (thing->z - thing->floorz);
    }
    else if (thing->flags & MF_MISSILE)
    {
        new_z += thing->origheight;
    }

    if (!P_TeleportMove(thing, new_x, new_y, new_z))
        return false;
    
    // FIXME: deltaviewheight may need adjustment
    if (thing->player)
        thing->player->viewz = thing->z + thing->player->viewheight;

    /* --- Momentum handling --- */

    if (thing->player && !(def->special & TELSP_SameSpeed))
    {
        // don't move for a bit
        thing->reactiontime = def->delay;

        // -ES- 1998/10/29 Start the fading
        if (telept_effect && thing->player == players[displayplayer])
            R_StartFading(0, (def->delay * 5) / 2);

        thing->mom.x = thing->mom.y = thing->mom.z = 0;
    }
    else if (thing->flags & MF_MISSILE)
    {
        thing->mom.x = thing->speed * M_Cos(new_ang);
        thing->mom.y = thing->speed * M_Sin(new_ang);
    }
    else if (def->special & TELSP_SameSpeed)
    {
        // we need to rotate the momentum vector
        angle_t mom_ang = new_ang - thing->angle;

        float s = M_Sin(mom_ang);
        float c = M_Cos(mom_ang);

        float mx = thing->mom.x;
        float my = thing->mom.y;

        thing->mom.x = mx * c - my * s;
        thing->mom.y = my * c + mx * s;
    }

    thing->angle = new_ang;

    if (currmobj && 0 == (def->special & (TELSP_Relative | TELSP_SameAbsDir |
                                          TELSP_Rotate)))
    {
        thing->vertangle = currmobj->vertangle;
    }

    /* --- Spawning teleport fog (source and/or dest) --- */

    if (! (def->special & TELSP_Silent))
    {
        mobj_t *fog;

        if (def->inspawnobj)
        {
            fog = P_MobjCreateObject(oldx, oldy, oldz, def->inspawnobj);

            if (fog->info->chase_state)
                P_SetMobjStateDeferred(fog, fog->info->chase_state, 0);
        }

        if (def->outspawnobj)
        {
            //
            // -ACB- 1998/09/06 Switched 40 to 20. This by my records is
            //                  the original setting.
            //
            // -ES- 1998/10/29 When fading, we don't want to see the fog.
            //
            fog = P_MobjCreateObject(new_x + 20.0f * M_Cos(thing->angle),
                                     new_y + 20.0f * M_Sin(thing->angle),
                                     new_z, def->outspawnobj);

            if (fog->info->chase_state)
                P_SetMobjStateDeferred(fog, fog->info->chase_state, 0);

            if (thing->player && !telept_flash)
                fog->vis_target = fog->visibility = INVISIBLE;
        }
    }

    return true;
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
            if (!(sec->lines[i]->flags & ML_TwoSided))
                continue;

            if (sec != sec->lines[i]->frontsector)
                continue;

            tsec = sec->lines[i]->backsector;

            if (tsec->floor.image != image)
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
        if (sec->ceil_move && def->is_ceiling)
            continue;

        // Already moving?  If so, keep going...
        if (sec->floor_move && !def->is_ceiling)
            continue;

        if (EV_BuildOneStair(sec, def))
            rtn = true;
    }

    return rtn;
}

//
// EV_DoPlane
//
// Do Platforms/Floors/Stairs/Ceilings/Doors
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

    if (def->is_ceiling)
    {
        if (sec->ceil_move)
            return false;
    }
    else
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
            S_StartSound((mobj_t *) & sec->soundorg, def->sfxstart);
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
            s2->props.special = s3->props.special;
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
                    S_StartSound((mobj_t *) & sec->soundorg, smov->info->sfx_start);
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
                S_StartSound((mobj_t *) & sec->soundorg, smov->info->sfx_open);
                smov->sfxstarted = true;
            }

            smov->opening += smov->info->speed;

            // mark line as non-blocking (at some point)
            P_ComputeGaps(smov->line);

            if (smov->opening >= smov->target)
            {
                S_StartSound((mobj_t *) & sec->soundorg, smov->info->sfx_stop);
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
                S_StartSound((mobj_t *) & sec->soundorg, smov->info->sfx_close);
                smov->sfxstarted = true;
            }

            smov->opening -= smov->info->speed;

            // mark line as blocking (at some point)
            P_ComputeGaps(smov->line);

            if (smov->opening <= 0.0f)
            {
                S_StartSound((mobj_t *) & sec->soundorg, smov->info->sfx_stop);
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

    S_StartSound((mobj_t *) & sec->soundorg, s->sfx_start);

    // Must handle line count here, since the normal code in p_spec.c
    // will clear the line->special pointer, confusing various bits of
    // code that deal with sliding doors (--> crash).
    // 
    if (line->count > 0)
        line->count--;
}

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
                S_StartSound((mobj_t *) & elev->sector->soundorg, elev->type->sfxdown);
                elev->sfxstarted = true;
            }

            if (res == RES_PastDest || res == RES_Impossible)
            {
                S_StartSound((mobj_t *) & elev->sector->soundorg, elev->type->sfxstop);
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
                S_StartSound((mobj_t *) & elev->sector->soundorg, elev->type->sfxdown);
                elev->sfxstarted = true;
            }

            if (res == RES_PastDest || res == RES_Impossible)
            {
                S_StartSound((mobj_t *) & elev->sector->soundorg, elev->type->sfxstop);
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

//
// EV_DoElevator
//
// Do Elevators
//
bool EV_DoElevator(sector_t * sec, const elevatordef_c * def, sector_t * model)
{
#if 0
    if (!sec->controller)
        return false;
#endif

    if (sec->ceil_move || sec->floor_move)
        return false;

    // Do Elevator action
    return P_SetupElevatorAction(sec, def, model) ? true : false;
}

//
// EV_ManualElevator
//
bool EV_ManualElevator(line_t * line, mobj_t * thing,  const elevatordef_c * def)
{
    return false;
}

//
// P_SetupElevatorAction
//
static elev_move_t *P_SetupElevatorAction(sector_t * sector,
                                          const elevatordef_c * def, sector_t * model)
{
    elev_move_t *elev;
    float start, dest;

    // new door thinker
    elev = Z_New(elev_move_t, 1);

    sector->ceil_move = (gen_move_t*)elev;
    sector->floor_move = (gen_move_t*)elev;

    elev->whatiam = MDT_ELEVATOR;
    elev->sector = sector;
    elev->sfxstarted = false;

// -ACB- BEGINNING OF THE HACKED TO FUCK BIT (START)

    start = sector->c_h;
    dest  = 192.0f;

// -ACB- FINISH OF THE HACKED TO FUCK BIT (END)

    if (dest > start)
    {
        elev->direction = DIRECTION_UP;

        if (def->speed_up >= 0)
            elev->speed = def->speed_up;
        else
            elev->speed = dest - start;
    }
    else if (start > dest)
    {
        elev->direction = DIRECTION_DOWN;

        if (def->speed_down >= 0)
            elev->speed = def->speed_down;
        else
            elev->speed = start - dest;
    }
    else
    {
        sector->ceil_move = NULL;
        sector->floor_move = NULL;

        Z_Free(elev);
        return NULL;
    }

    elev->destheight = dest;
    elev->startheight = start;
    elev->tag = sector->tag;
    elev->type = def;

    // -ACB- 10/01/2001 Trigger starting sfx
    S_StopLoopingSound((mobj_t *) & sector->soundorg);
    if (def->sfxstart)
        S_StartSound((mobj_t *) & sector->soundorg, def->sfxstart);

    P_AddActivePart((gen_move_t*)elev);
    return elev;
}

