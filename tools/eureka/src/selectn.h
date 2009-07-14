//------------------------------------------------------------------------
//  SELECTION LISTS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor (C) 2001-2009 Andrew Apted
//                     (C) 1997-2003 André Majorel et al
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
//------------------------------------------------------------------------
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------


#ifndef YH_SELECTN  /* DO NOT INSERT ANYTHING BEFORE THIS LINE */
#define YH_SELECTN


#define IsSelected(list, num)  ((list)->get(num))

#define ForgetSelection(list)  ((list)->clear_all())

#define SelectObject(list, num)  ((list)->set(num))
#define UnSelectObject(list, num)  ((list)->clear(num))
#define ToggleObject(list, num)  ((list)->toggle(num))

void DumpSelection (selection_c * list);

bitvec_c *list_to_bitvec (SelPtr list, size_t bitvec_size);
SelPtr bitvec_to_list (const bitvec_c &b);


#endif  /* DO NOT ADD ANYTHING AFTER THIS LINE */
//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
