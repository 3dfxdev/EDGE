//----------------------------------------------------------------------------
//  EDGE Switch Handling Code
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

#include "i_defs.h"

#include "dm_defs.h"
#include "dm_state.h"
#include "ddf_main.h"
#include "g_game.h"
#include "p_local.h"
#include "r_state.h"
#include "s_sound.h"
#include "w_textur.h"
#include "z_zone.h"

//
// CHANGE THE TEXTURE OF A WALL SWITCH TO ITS OPPOSITE
//
// -KM- 98/07/31 Move to DDF

int maxbuttons = 0;
button_t *buttonlist = NULL;

//
// P_InitSwitchList
//
// Only called at game initialization.
//
bool P_InitSwitchList(void)
{
	epi::array_iterator_c it;
	switchdef_c *sw;
	
	for (it = switchdefs.GetBaseIterator(); it.IsValid(); it++)
	{
		sw = ITERATOR_TO_TYPE(it, switchdef_c*);

		sw->cache.image[0] = W_ImageFromTexture(sw->name1, true);
		sw->cache.image[1] = W_ImageFromTexture(sw->name2, true);
	}

	return true;
}

bool P_ButtonCheckPressed(line_t * line)
{
	int i;

	for (i = 0; i < maxbuttons; i++)
	{
		if (buttonlist[i].btimer && buttonlist[i].line == line)
			return true;
	}

	return false;
}

//
// Start a button counting down till it turns off.
//
static void StartButton(switchdef_c *sw, line_t *line, bwhere_e w,
		const image_t *image)
{
	int index;

	// See if button is already pressed
	if (P_ButtonCheckPressed(line))
		return;

	for (index = 0; index < maxbuttons; index++)
	{
		if (!buttonlist[index].btimer)
			break;
	}

	if (index == maxbuttons)
	{
		// grow the button list
		Z_Resize(buttonlist, button_t, ++maxbuttons);
	}

	DEV_ASSERT2(index < maxbuttons);

	buttonlist[index].line = line;
	buttonlist[index].where = w;
	buttonlist[index].btimer = sw->time;
	buttonlist[index].off_sound = sw->off_sfx;
	buttonlist[index].bimage = image;
}

//
// P_ChangeSwitchTexture
//
// Function that changes wall texture.
// Tell it if switch is ok to use again.
//
// -KM- 1998/09/01 All switches referencing a certain tag are switched
//

#define CHECK_SW(PART)  (sw->cache.image[k] == side->PART.image)
#define SET_SW(PART)    side->PART.image = sw->cache.image[k^1] 
#define OLD_SW          sw->cache.image[k]

void P_ChangeSwitchTexture(line_t * line, bool useAgain,
		line_special_e specials, bool noSound)
{
	int j, k;
	int tag = line->tag;
	epi::array_iterator_c it;
	const linedeftype_t *type = line->special;
	mobj_t *soundorg;
	side_t *side;
	bwhere_e pos;

	for (j=0; j < numlines; j++)
	{
		if (line != &lines[j])
		{
			if (tag == 0 || (lines[j].tag != tag) || 
					(specials & LINSP_SwitchSeparate) ||
					(type != lines[j].special && type && 
					 lines[j].special && useAgain))
			{
				continue;
			}
		}

		side = lines[j].side[0];
		soundorg = (mobj_t *) &lines[j].frontsector->soundorg;

		pos = BWH_None;

		switchdef_c *sw;
		for (it = switchdefs.GetBaseIterator(); it.IsValid() && (pos == BWH_None); it++)
		{
			sw = ITERATOR_TO_TYPE(it, switchdef_c*);

			if (!sw->cache.image[0] && !sw->cache.image[1])
				continue;

			// some like it both ways...
			for (k=0; k < 2; k++)
			{
				if (CHECK_SW(top))
				{
					SET_SW(top);
					pos = BWH_Top;
					break;
				}
				else if (CHECK_SW(middle))
				{
					SET_SW(middle);
					pos = BWH_Middle;
					break;
				}
				else if (CHECK_SW(bottom))
				{
					SET_SW(bottom);
					pos = BWH_Bottom;
					break;
				}
			}   // k < 2

			if (pos != BWH_None)
			{
				// -KM- 98/07/31 Implement sounds
				if (! noSound && sw->on_sfx)
				{
					S_StartSound(soundorg, sw->on_sfx);
					noSound = true;
				}

				if (useAgain)
					StartButton(sw, &lines[j], pos, OLD_SW);

				break;
			}
		}   // it.IsValid() - switchdefs
	}   // j < numlines
}

#undef CHECK_SW
#undef SET_SW
#undef OLD_SW

