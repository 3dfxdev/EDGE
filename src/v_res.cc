//----------------------------------------------------------------------------
//  EDGE Video Code  
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

#include "am_map.h"
#include "st_stuff.h"
#include "v_res.h"

#include "./epi/strings.h"

// Globals
int SCREENWIDTH;
int SCREENHEIGHT;
int SCREENBITS;
bool SCREENWINDOW;
bool graphicsmode = false;

scrmodelist_c scrmodelist;

//
// V_InitResolution
//
// Inits everything resolution-dependent to SCREENWIDTH x SCREENHEIGHT x BPP
//
void V_InitResolution(void)
{	
	ST_ReInit();
	AM_InitResolution();
}

//
// V_AddAvailableResolution
//
// Adds a resolution to the scrmodes list. This is used so we can
// select it within the video options menu.
//
// -ACB- 1999/10/03 Written
//
void V_AddAvailableResolution(i_scrmode_t *mode)
{
    int depth;

    depth = 0;
    switch(mode->depth)
    {
        case 15:
        case 16:
        {
            depth = 16;
            break;
        }

        case 24:
        case 32:
        {
            depth = 32;
            break;
        }

        default:
        {
            break;
        }
    }

    if (!depth)
        return;

    scrmode_t sm;

    sm.width = mode->width;
    sm.height = mode->height;
    sm.depth = depth;
    sm.windowed = mode->windowed;
    sm.sysdepth = mode->depth;

	{
		epi::string_c s;
		
	    s.Format("Adding: (%d x %d x %d [%d]) - %s\n", 
                 sm.width, sm.height, sm.depth, sm.sysdepth, 
                 sm.windowed ? "Windowed" : "Fullscreen");

        I_Printf(s);
	}	

    scrmodelist.Add(&sm);
	return;
}

//
// V_GetSysRes
//
// Helper function: gets us a i_scrmode_t from a scrmode_t. This is
// not _yet_ done as part of the scrmodelist_c object since
// the initial conversion is done in application code. Probably should change.
//
void V_GetSysRes(scrmode_t *src, i_scrmode_t *dest)
{
	dest->width = src->width;
	dest->height = src->height;
	dest->depth = src->sysdepth;
	dest->windowed = src->windowed;
}

// ---> scrmodelist_c

//
// int scrmodelist_c::Add()
//
int scrmodelist_c::Add(scrmode_t *sm)
{
    int diff, nearest, retval;

    // Find the nearest match
    diff = 0;
    nearest = FindNearest(sm->width, sm->height, sm->depth, sm->windowed);
    if (nearest >= 0)
    {
        scrmode_t *nsm = GetAt(nearest);

        diff = Compare(sm, nsm);
        if (diff == 0)
        {
            // Is the new entry a better match?
            if (nsm->depth != nsm->sysdepth &&    // Nearest is not perfect
                sm->sysdepth != nsm->sysdepth &&  // New entry is not duplicate
                sm->depth == sm->sysdepth)        // New entry is perfect
            {
                // Update the nearest to be the new and more preferable entry
                nsm->sysdepth = sm->sysdepth;
            }
      
            return nearest;
        }
    }

    // OK, we've got to add a new entry
    scrmode_t *new_sm = new scrmode_t;
    memcpy(new_sm, sm, sizeof(scrmode_t));

    if (diff == 0)
    {
        // We got when diff was zero, must be a new entry in a blank list
        retval = InsertObject((void*)&new_sm);
    }
    else if (diff < 0)
    {
        // Insert prior to nearest entry
        retval = InsertObject((void*)&new_sm, nearest);
    }
    else // if (diff > 0)
    {
        // Insert after the nearest entry
        retval = InsertObject((void*)&new_sm, nearest+1);
    }

	if (retval < 0)
        delete new_sm; // Failed for some reason, dispose of new entry
   
    return retval;
}

//
// int scrmodelist_c::Compare()
//
// Screen mode comparision function
//
#define MODE_AS_VALUE(w, h, bpp, win) \
    ((h & 0x7FF) + ((w & 0x7FF) << 11) + ((bpp & 0x3F) << 22) + (win?(1<<28):0))

int scrmodelist_c::Compare(scrmode_t* sm1, scrmode_t* sm2)
{
    return MODE_AS_VALUE(sm1->width, sm1->height, sm1->depth, sm1->windowed) -
           MODE_AS_VALUE(sm2->width, sm2->height, sm2->depth, sm2->windowed);
}

//
// void scrmodelist_c::Dump()
// 
void scrmodelist_c::Dump()
{
    epi::array_iterator_c it;
	epi::string_c s;
    scrmode_t *sm;

    for (it = GetBaseIterator(); it.IsValid(); it++)
    {
        sm = ITERATOR_TO_TYPE(it, scrmode_t*);
        
        s.Format("(%d x %d x %d [%d]) - %s\n", 
                 sm->width, sm->height, sm->depth, sm->sysdepth, 
                 sm->windowed ? "Windowed" : "Fullscreen");

        I_Printf(s);
     }	
}

//
// int scrmodelist_c::Find()
//
// Find an exact match for the given resolution
//
int scrmodelist_c::Find(int w, int h, int bpp, bool windowed)
{
    scrmode_t testsm;
    testsm.width = w;
    testsm.height = h;
    testsm.depth = bpp;
    testsm.windowed = windowed;

    epi::array_iterator_c it;
    scrmode_t *sm;

    for (it = GetBaseIterator(); it.IsValid(); it++)
    {
        sm = ITERATOR_TO_TYPE(it, scrmode_t*);

        if (Compare(&testsm, sm) == 0)
            return it.GetPos(); // Exact match
    }

    return -1;
}

//
// int scrmodelist_c::FindNearest()
//
// Find the nearest match for the 
//
int scrmodelist_c::FindNearest(int w, int h, int bpp, bool windowed)
{
    scrmode_t testsm;
    testsm.width = w;
    testsm.height = h;
    testsm.depth = bpp;
    testsm.windowed = windowed;

    epi::array_iterator_c it;
    scrmode_t *sm;
    int lm, lmdiff, diff;

    lmdiff = INT_MAX;
    lm = -1;

    for (it = GetBaseIterator(); it.IsValid(); it++)
    {
        sm = ITERATOR_TO_TYPE(it, scrmode_t*);

        diff = Compare(&testsm, sm);
        if (diff == 0)
            return it.GetPos(); // Exact match

        diff = ABS(diff);
        if (lmdiff > diff)
        {
            lm = it.GetPos();
            lmdiff = diff;
        }
    }

	return lm;
}

//
// FindWithDepthBias
//
// Find a mode, but placing emphasis on the depth and
// windowmode being correct
//
int scrmodelist_c::FindWithDepthBias(int w, int h, int bpp, bool windowed)
{
    int sel_mode = FindNearest(w, h, bpp, windowed);
     
    if (sel_mode >= 0) // Should always be true
    {
        scrmode_t *sm = GetAt(sel_mode);
        if (sm->windowed != windowed)
        {
            // A change in windowmode is unacceptable
            sel_mode = -1;
        }
        else if (sm->depth != bpp)
        {
            // A depth change was not possible
            sel_mode = -1;
        }
    }

    return sel_mode;
}

//
// FindWithWindowModeBias
//
int scrmodelist_c::FindWithWindowModeBias(int w, int h, int bpp, bool windowed)
{
    int sel_mode = FindNearest(w, h, bpp, windowed);
            
    if (sel_mode >= 0) // Should always be true
    {
        scrmode_t *sm = GetAt(sel_mode);
        if (sm->windowed != windowed)
        {
            // Not what we wanted
            sel_mode = -1;
        }
    }

    return sel_mode;
}

//
// int scrmodelist_c::Prev() 
//
// Get the next resolution using the given increment type 
//
int scrmodelist_c::Prev(int idx, scrmodelist_c::incrementtype_e type)
{
    DEV_ASSERT2(type == RES || type == DEPTH || type == WINDOWMODE);
    DEV_ASSERT2(idx >= 0 && idx < GetSize());

    scrmode_t *orig_sm = GetAt(idx);
    int sel_mode = -1;

    switch(type)
    {
        case RES:
        {
            //
            // We make use of the sort order here: Since the list is
            // break down by the window flag, depth and then the 
            // the resolution we just need to look the prev in the 
            // list. Failing that we'll work forwards and get
            // the last possible size for the current depth and
            // windowmode (fullscreen/window).
            //
            epi::array_iterator_c it = GetIterator(idx);
            scrmode_t *sm = NULL;

            it--; // Go to the previous entry

            if (it.IsValid())
            {
                sm = ITERATOR_TO_TYPE(it, scrmode_t*);
                if (sm->depth == orig_sm->depth && 
                    sm->windowed == orig_sm->windowed)
                {
                    sel_mode = it.GetPos();
                }
            }

            // There is no previous entry: Wrap around
            if (sel_mode < 0)
            {
                it = GetIterator(idx);
                it++;

                while (it.IsValid())
                {
                    sm = ITERATOR_TO_TYPE(it, scrmode_t*);
                	
                	if (sm->depth != orig_sm->depth ||
                        sm->windowed != orig_sm->windowed)
                    {
                        break;
                    }

                    sel_mode = it.GetPos();
                    it++;
                }
            }

            break;
        }

        case DEPTH:
        {
            // Since depth has only possible values we cheat a bit
            // here and treat it as a toggle
            sel_mode = FindWithDepthBias(orig_sm->width,
                                         orig_sm->height,
                                         orig_sm->depth==32?16:32,
                                         orig_sm->windowed);
            break;
        }

        case WINDOWMODE:
        {
            // Toggle..
            sel_mode = FindWithWindowModeBias(orig_sm->width,
                                              orig_sm->height,
                                              orig_sm->depth,
                                              !orig_sm->windowed);
            break;
        }

        default:
            break;
    }

    // Failure to find another res, just return the one we were given
    if (sel_mode < 0)
        sel_mode = idx;

    return sel_mode;
}

//
// int scrmodelist_c::Next() 
//
// Get the next resolution using the given increment type
//
int scrmodelist_c::Next(int idx, scrmodelist_c::incrementtype_e type)
{
    DEV_ASSERT2(type == RES || type == DEPTH || type == WINDOWMODE);
    DEV_ASSERT2(idx >= 0 && idx < GetSize());

    scrmode_t *orig_sm = GetAt(idx);
    int sel_mode = -1;

    switch(type)
    {
        case RES:
        {
            //
            // We make use of the sort order here: Since the list is
            // break down by the window flag, depth and then the 
            // the resolution we just need to look the next in the 
            // list. Failing that we'll work backwards and get
            // the first possible size for the current depth and
            // windowmode (fullscreen/window).
            //
            epi::array_iterator_c it = GetIterator(idx);
            scrmode_t *sm = NULL;

            it++; // Next!

            if (it.IsValid())
            {
                sm = ITERATOR_TO_TYPE(it, scrmode_t*);
                if (sm->depth == orig_sm->depth && 
                    sm->windowed == orig_sm->windowed)
                {
                    sel_mode = it.GetPos();
                }
            }

            // There is no next: Wrap around
            if (sel_mode < 0)
            {
                it = GetIterator(idx);
                it--;

                while (it.IsValid())
                {
                	sm = ITERATOR_TO_TYPE(it, scrmode_t*);
                    
                    if (sm->depth != orig_sm->depth ||
                        sm->windowed != orig_sm->windowed)
                    {
                        break;
                    }

                    sel_mode = it.GetPos();
                    it--;
                }
            }

            break;
        }

        case DEPTH:
        {
            // Since depth has only possible values we cheat a bit
            // here and treat it as a toggle
            sel_mode = FindWithDepthBias(orig_sm->width,
                                         orig_sm->height,
                                         orig_sm->depth==32?16:32,
                                         orig_sm->windowed);
            break;
        }

        case WINDOWMODE:
        {
            // Toggle..
            sel_mode = FindWithWindowModeBias(orig_sm->width,
                                              orig_sm->height,
                                              orig_sm->depth,
                                              !orig_sm->windowed);
            break;
        }

        default:
            break;
    }

    // Failure to find another res, just return the one we were given
    if (sel_mode < 0)
        sel_mode = idx;

    return sel_mode;
}
	
