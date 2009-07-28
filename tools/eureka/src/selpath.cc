//------------------------------------------------------------------------
//  LINEDEF PATHS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
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

#include "main.h"

#include "m_bitvec.h"
#include "levels.h"
#include "selectn.h"
#include "w_structs.h"


static void select_linedefs_in_half_path (bitvec_c &sel,
    bitvec_c &ldseen,
    int linedef_no,
    int vertex_no,
    sel_op_e mode);

static void select_1s_linedefs_in_half_path (bitvec_c &sel,
    bitvec_c &ldseen,
    int linedef_no,
    int vertex_no,
    sel_op_e mode);


/*
 *  select_linedefs_path
 *  Add to selection <list> all linedefs in the same path as
 *  linedef <linedef_no>, stopping at forks. If <mode> is not
 *  YS_ADD but YS_REMOVE, those linedefs are removed from
 *  the selection. If <mode> is YS_TOGGLE, each linedef is
 *  removed if it was already in the selection, otherwise
 *  added.
 */
void select_linedefs_path (SelPtr *list, int linedef_no, sel_op_e mode)
{
#if 0  // FIXME
	bitvec_c *ldsel = list_to_bitvec (*list, NumLineDefs);
	bitvec_c ldseen (NumLineDefs);  // Linedef already seen ?

	if (! is_obj (linedef_no))  // Sanity check
		FatalError("select_linedef_path called with bad linedef_no=%d",
				linedef_no);

	LDPtr ld = LineDefs + linedef_no;
	ldsel->frob (linedef_no, mode);
	ldseen.set (linedef_no);
	select_linedefs_in_half_path (*ldsel, ldseen, linedef_no, ld->start, mode);
	select_linedefs_in_half_path (*ldsel, ldseen, linedef_no, ld->end, mode);
	ForgetSelection (list);
	*list = bitvec_to_list (*ldsel);
	delete ldsel;
#endif
}


/*
 *  select_linedefs_in_half_path
 *  The routine looks for all linedefs other than linedef_no
 *  that use vertex vertex_no. If there are none or more
 *  than one, the search stop there and the function returns
 *  without doing nothing. If there is exactly one, it is
 *  added to the list sel and the function recursively calls
 *  itself on the other vertex of that linedef.
 */
static void select_linedefs_in_half_path (bitvec_c &ldsel,
		bitvec_c &ldseen,
		int linedef_no,
		int vertex_no,
		sel_op_e mode)
{
	int next_linedef_no = OBJ_NO_NONE;
	int next_vertex_no = OBJ_NO_NONE;

	// Look for the next linedef in the path. It's the
	// linedef that uses vertex vertex_no and is not
	// linedef_no.
	for (int n = 0; n < NumLineDefs; n++)
	{
		if (n == linedef_no)
			continue;
		if (LineDefs[n]->start == vertex_no)
		{
			if (is_obj (next_linedef_no))
				return;  // There is a fork in the path. Stop here.
			// Continue search at the other end of the linedef
			next_vertex_no = LineDefs[n]->end;
			next_linedef_no = n;
		}
		if (LineDefs[n]->end == vertex_no)
		{
			if (is_obj (next_linedef_no))
				return;  // There is a fork in the path. Stop here.
			// Continue search at the other end of the linedef
			next_vertex_no = LineDefs[n]->start;
			next_linedef_no = n;
		}
	}
	if (! is_obj (next_linedef_no))  // None ? Reached end of path.
		return;

	// Already seen the next linedef ? The path must be
	// closed. No need to do like the Dupondt.
	if (ldseen.get (next_linedef_no))
		return;

	ldsel.frob (next_linedef_no, mode);
	ldseen.set (next_linedef_no);

	select_linedefs_in_half_path (ldsel, ldseen, next_linedef_no, next_vertex_no,
			mode);
}


/*
 *  select_1s_linedefs_path
 *
 *  Add to selection <list> all linedefs in the same path as
 *  linedef <linedef_no>, stopping at forks. If <mode> is not
 *  YS_ADD but YS_REMOVE, those linedefs are removed from
 *  the selection. If <mode> is YS_TOGGLE, each linedef is
 *  removed if it was already in the selection, otherwise
 *  added.
 */
void select_1s_linedefs_path (SelPtr *list, int linedef_no, sel_op_e mode)
{
#if 0  // FIXME
	bitvec_c *ldsel = list_to_bitvec (*list, NumLineDefs);
	bitvec_c ldseen (NumLineDefs);  // Linedef already seen ?

	if (! is_obj (linedef_no))  // Sanity check
		FatalError("select_linedef_path called with bad linedef_no=%d",
				linedef_no);

	LDPtr ld = LineDefs + linedef_no;
	if (! is_obj (ld->side_R)  // The first linedef is not single-sided. Quit.
			|| is_obj (ld->side_L))
		goto byebye;
	ldsel->frob (linedef_no, mode);
	ldseen.set (linedef_no);

	select_1s_linedefs_in_half_path (*ldsel, ldseen, linedef_no, ld->start, mode);
	select_1s_linedefs_in_half_path (*ldsel, ldseen, linedef_no, ld->end, mode);
	
	ForgetSelection (list);
	
	*list = bitvec_to_list (*ldsel);
byebye:
	delete ldsel;
#endif
}


/*
 *  select_1s_linedefs_in_half_path
 *  Like select_linedefs_in_half_path() except that only
 *  single-sided linedefs are selected.
 */
static void select_1s_linedefs_in_half_path (bitvec_c &ldsel,
		bitvec_c &ldseen,
		int linedef_no,
		int vertex_no,
		sel_op_e mode)
{
	int next_linedef_no = OBJ_NO_NONE;
	int next_vertex_no = OBJ_NO_NONE;

	// Look for the next linedef in the path. It's the
	// linedef that uses vertex vertex_no and is not
	// linedef_no.
	for (int n = 0; n < NumLineDefs; n++)
	{
		if (n == linedef_no)
			continue;
		if (LineDefs[n]->start == vertex_no
				&& is_obj (LineDefs[n]->right)
				&& ! is_obj (LineDefs[n]->left))
		{
			if (is_obj (next_linedef_no))
				return;  // There is a fork in the path. Stop here.
			// Continue search at the other end of the linedef
			next_vertex_no = LineDefs[n]->end;
			next_linedef_no = n;
		}
		if (LineDefs[n]->end == vertex_no
				&& is_obj (LineDefs[n]->right)
				&& ! is_obj (LineDefs[n]->left))
		{
			if (is_obj (next_linedef_no))
				return;  // There is a fork in the path. Stop here.
			// Continue search at the other end of the linedef
			next_vertex_no = LineDefs[n]->start;
			next_linedef_no = n;
		}
	}
	if (! is_obj (next_linedef_no))  // None ? Reached end of path.
		return;

	// Already seen the next linedef ? The path must be
	// closed. No need to do like the Dupondt.
	if (ldseen.get (next_linedef_no))
		return;

	ldsel.frob (next_linedef_no, mode);
	ldseen.set (next_linedef_no);

	select_1s_linedefs_in_half_path (ldsel, ldseen, next_linedef_no,
			next_vertex_no, mode);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
