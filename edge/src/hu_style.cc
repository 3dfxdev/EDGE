//----------------------------------------------------------------------------
//  EDGE Heads-up-display Style code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2004  The EDGE Team.
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
#include "hu_style.h"

#include "dm_defs.h"
#include "dm_state.h"
#include "m_inline.h"
#include "r_local.h"
#include "v_colour.h"
#include "v_ctx.h"
#include "v_res.h"
#include "w_image.h"


// Edge has lots of style
style_container_c hu_styles;


style_c::style_c(styledef_c *_def) : def(_def), bg_image(NULL)
{
	for (int T = 0; T < styledef_c::NUM_TXST; T++)
		fonts[T] = NULL;
}

style_c::~style_c()
{
	/* nothing to do */
}


//
// style_c::Load
//
void style_c::Load()
{
	// FIXME !!!!
}


// ---> style_container_c class

//
// style_container_c::CleanupObject()
//
void style_container_c::CleanupObject(void *obj)
{
	style_c *a = *(style_c**)obj;

	if (a) delete a;
}

//
// style_container_c::Lookup()
//
// Never returns NULL.
//
style_c* style_container_c::Lookup(styledef_c *def)
{
	DEV_ASSERT2(def);

	for (epi::array_iterator_c it = GetIterator(0); it.IsValid(); it++)
	{
		style_c *st = ITERATOR_TO_TYPE(it, style_c*);

		if (def == st->def)
			return st;
	}

	style_c *new_st = new style_c(def);

	new_st->Load();
	Insert(new_st);

	return new_st;
}

