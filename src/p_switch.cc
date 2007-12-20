//----------------------------------------------------------------------------
//  EDGE Switch Handling Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2007  The EDGE Team.
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

#include "ddf/main.h"
#include "ddf/switch.h"

#include "dm_defs.h"
#include "dm_state.h"
#include "g_game.h"
#include "p_local.h"
#include "r_state.h"
#include "s_sound.h"
#include "w_texture.h"

// --> Button list class
class buttonlist_c : public epi::array_c
{
public:
	buttonlist_c() : epi::array_c(sizeof(button_t)) {}
	~buttonlist_c() { Clear(); }

private:
	void CleanupObject(void *obj) { /* ... */ }

public:
	int Find(button_t *b);
	button_t* GetNew();
	int GetSize() {	return array_entries; } 
	bool IsPressed(line_t* line);
	void SetSize(int count);
	
	button_t* operator[](int idx) { return (button_t*)FetchObject(idx); } 
};

buttonlist_c buttonlist;

//
// P_InitSwitchList
//
// Only called at game initialization.
//
void P_InitSwitchList(void)
{
	epi::array_iterator_c it;
	switchdef_c *sw;
	
	for (it = switchdefs.GetBaseIterator(); it.IsValid(); it++)
	{
		sw = ITERATOR_TO_TYPE(it, switchdef_c*);

		sw->cache.image[0] = W_ImageLookup(sw->name1, INS_Texture, ILF_Null);
		sw->cache.image[1] = W_ImageLookup(sw->name2, INS_Texture, ILF_Null);
	}
}

//
// Start a button counting down till it turns off.
//
static void StartButton(switchdef_c *sw, line_t *line, bwhere_e w,
		const image_c *image)
{
	// See if button is already pressed
	if (buttonlist.IsPressed(line))
		return;

	button_t *b;
	
	b = buttonlist.GetNew();

	b->line = line;
	b->where = w;
	b->btimer = sw->time;
	b->off_sound = sw->off_sfx;
	b->bimage = image;
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
	const linetype_c *type = line->special;
	position_c *sfx_origin;
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
		sfx_origin = &lines[j].frontsector->sfx_origin;

		pos = BWH_None;

		// Note: reverse order, give priority to newer switches.
		for (it = switchdefs.GetTailIterator(); 
             it.IsValid() && (pos == BWH_None); 
             it--)
		{
			switchdef_c *sw = ITERATOR_TO_TYPE(it, switchdef_c*);

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
					SYS_ASSERT(sfx_origin);
					S_StartFX(sw->on_sfx, SNCAT_Level, sfx_origin);
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

//
// int buttonlist_c::Find()
//
// FIXME! Optimise!
//
int buttonlist_c::Find(button_t *b)
{
	epi::array_iterator_c it;
	
	for (it=GetBaseIterator(); it.IsValid(); it++)
	{
		button_t *b2 = ITERATOR_TO_PTR(it, button_t);

		if (b == b2)
			return it.GetPos();
	}
	
	return -1;
}

//
// button_t* buttonlist_c::GetNew()
//
button_t* buttonlist_c::GetNew()
{
	epi::array_iterator_c it;
	button_t *b;
	
	for (it=GetBaseIterator(); it.IsValid(); it++)
	{
		b = ITERATOR_TO_PTR(it, button_t);
		
		if (!b->btimer)
			return b;
	}
	
	b = (button_t*)ExpandAtTail();
	return b;
}

//
// bool buttonlist_c::IsPressed()
//
bool buttonlist_c::IsPressed(line_t* line)
{
	epi::array_iterator_c it;
	
	for (it=GetBaseIterator(); it.IsValid(); it++)
	{
		button_t *b = ITERATOR_TO_PTR(it, button_t);

		if (b->line == line && b->btimer)
			return true;
	}
	
	return false;
}

//
// buttonlist_c::SetSize()
//
void buttonlist_c::SetSize(int count)
{
	Size(count);
	SetCount(count);
	
	memset(array, 0, array_entries*array_objsize);
}

void P_ClearButtons(void)
{
	buttonlist.Clear();
}

bool P_ButtonIsPressed(line_t *ld)
{
	return buttonlist.IsPressed(ld);
}

void P_UpdateButtons(void)
{
	epi::array_iterator_c it;
	button_t *b;
	
	for (it = buttonlist.GetBaseIterator(); it.IsValid(); it++)
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
            S_StartFX(b->off_sound, SNCAT_Level,
                           &b->line->frontsector->sfx_origin);
        }

		Z_Clear(b, button_t, 1);
	}
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
