//------------------------------------------------------------------------
//  BASIC OBJECT HANDLING
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

#include "main.h"

#include <algorithm>

#include "e_basis.h"


std::vector<Thing *>   Things;
std::vector<Vertex *>  Vertices;
std::vector<Sector *>  Sectors;
std::vector<SideDef *> SideDefs;
std::vector<LineDef *> LineDefs;
std::vector<RadTrig *> RadTrigs;


static int RawCreateThing()
{
	int objnum = NumThings;

	NumThings++;

	if (objnum > 0)
		Things = (TPtr) ResizeMemory (Things,
				(unsigned long) NumThings * sizeof (struct Thing));
	else
		Things = (TPtr) GetMemory (sizeof (struct Thing));

	memset(&Things[objnum], 0, sizeof (struct Thing));

	return objnum;
}


static int RawCreateLineDef()
{
	int objnum = NumLineDefs;

	NumLineDefs++;

	if (objnum > 0)
		LineDefs = (LDPtr) ResizeMemory (LineDefs,
				(unsigned long) NumLineDefs * sizeof (struct LineDef));
	else
		LineDefs = (LDPtr) GetMemory (sizeof (struct LineDef));

	memset(&LineDefs[objnum], 0, sizeof (struct LineDef));

	return objnum;
}


static int RawCreateSideDef()
{
	int objnum = NumSideDefs;

	NumSideDefs++;

	if (objnum > 0)
		SideDefs = (SDPtr) ResizeMemory (SideDefs,
				(unsigned long) NumSideDefs * sizeof (struct SideDef));
	else
		SideDefs = (SDPtr) GetMemory (sizeof (struct SideDef));

	memset(&SideDefs[objnum], 0, sizeof (struct SideDef));

	return objnum;
}


static int RawCreateVertex()
{
	int objnum = NumVertices;

	NumVertices++;

	if (objnum > 0)
		Vertices = (VPtr) ResizeMemory (Vertices,
				(unsigned long) NumVertices * sizeof (struct Vertex));
	else
		Vertices = (VPtr) GetMemory (sizeof (struct Vertex));

	memset(&Vertices[objnum], 0, sizeof (struct Vertex));

	return objnum;
}


static int RawCreateSector()
{
	int objnum = NumSectors;

	NumSectors++;

	if (objnum > 0)
		Sectors = (SPtr) ResizeMemory (Sectors,
				(unsigned long) NumSectors * sizeof (struct Sector));
	else
		Sectors = (SPtr) GetMemory (sizeof (struct Sector));

	memset(&Sectors[objnum], 0, sizeof (struct Sector));

	return objnum;
}



static void RawDeleteThing(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumThings);

	// Delete the thing
	NumThings--;

	if (NumThings > 0)
	{
		for (int n = objnum; n < NumThings; n++)
			Things[n] = Things[n + 1];
	}
	else
	{
		FreeMemory (Things);
		Things = NULL;
	}
}


static void RawDeleteVertex(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumVertices);

	NumVertices--;

	if (NumVertices > 0)
	{
		for (int n = objnum; n < NumVertices; n++)
			Vertices[n] = Vertices[n + 1];
	}
	else
	{
		FreeMemory (Vertices);
		Vertices = NULL;
	}
}


static void RawDeleteSector(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumSectors);

	NumSectors--;

	if (NumSectors > 0)
	{
		for (int n = objnum; n < NumSectors; n++)
			Sectors[n] = Sectors[n + 1];
	}
	else
	{
		FreeMemory (Sectors);
		Sectors = NULL;
	}
}


static void RawDeleteLineDef(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumLineDefs);

	NumLineDefs--;

	if (NumLineDefs > 0)
	{
		for (int n = objnum; n < NumLineDefs; n++)
			LineDefs[n] = LineDefs[n + 1];
	}
	else
	{
		FreeMemory (LineDefs);
		LineDefs = NULL;
	}
}


static void RawDeleteSideDef(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumSideDefs);

	NumSideDefs--;

	if (NumSideDefs > 0)
	{
		for (int n = objnum; n < NumSideDefs; n++)
			SideDefs[n] = SideDefs[n + 1];
	}
	else
	{
		FreeMemory (SideDefs);
		SideDefs = NULL;
	}
}


/*
   delete a group of objects
*/
void DeleteObjects(selection_c *list)
{
	int objtype = list->what_type();


	// we need to process the object numbers from highest to lowest,
	// because each deletion invalidates all higher-numbered refs
	// in the selection.  Our selection iterator cannot give us
	// what we need, hence put them into a vector for sorting.

	std::vector<int> all_ids;

	selection_iterator_c it;
	for (list->begin(&it); !it.at_end(); ++it)
		all_ids.push_back(*it);

	std::sort(all_ids.begin(), all_ids.end());


	// the passed in selection is now invalid.  Hence we clear it,
	// but we also re-use it for sidedefs which must be deleted
	// (due to their sectors going away).

	list->change_type(OBJ_SIDEDEFS);

	SYS_ASSERT(list->empty());


	for (int i = (int)all_ids.size()-1; i >= 0; i--)
	{
		int objnum = all_ids[i];

		switch (objtype)
		{
			case OBJ_THINGS:
				RawDeleteThing(objnum);
				break;

			case OBJ_LINEDEFS:
				RawDeleteLineDef(objnum);
				break;

			case OBJ_VERTICES:
				RawDeleteVertex(objnum);

				// delete the linedefs bound to this vertex and
				// fix the references

				for (int n = NumLineDefs-1; n >= 0; n--)
				{
					LineDef *L = &LineDefs[n];

					if (L->start == objnum || L->end == objnum)
						RawDeleteLineDef(n);
					else
					{
						if (L->start > objnum)
							L->start--;
						if (L->end > objnum)
							L->end--;
					}
				}
				break;

			case OBJ_SIDEDEFS:
				RawDeleteSideDef(objnum);

				// fix the linedefs references

				for (int n = NumLineDefs-1; n >= 0; n--)
				{
					LineDef *L = &LineDefs[n];

					if (L->side_R == objnum)
						L->side_R = -1;
					else if (L->side_R > objnum)
						L->side_R--;

					if (L->side_L == objnum)
						L->side_L = -1;
					else if (L->side_L > objnum)
						L->side_L--;
				}
				break;
			
			case OBJ_SECTORS:
				RawDeleteSector(objnum);

				// delete the sidedefs bound to this sector and
				// fix the references

				for (int n = NumSideDefs-1; n >= 0; n--)
				{
					SideDef *S = &SideDefs[n];

					if (S->sector == objnum)
					{
						list->set(n);
						S->sector = -1;
					}
					else if (S->sector > objnum)
						S->sector--;
				}
				break;

			default:
				nf_bug ("DeleteObjects: bad objtype %d", (int) objtype);
		}
	}


	if (list->notempty())
	{
		DeleteObjects(list);
	}

	list->change_type(objtype);

	MadeChanges = 1;
}


void DeleteObject(const Objid& obj)
{
	// things and linedefs are easy, since nothing else references them
	if (obj.type == OBJ_THINGS)
	{
		RawDeleteVertex(obj.num);
		return;
	}
	if (obj.type == OBJ_LINEDEFS)
	{
		RawDeleteLineDef(obj.num);
		return;
	}

	selection_c list(obj.type, obj.num);

	DeleteObjects(&list);
}


/*
 *  InsertObject
 *
 *  Insert a new object of type <objtype> at map coordinates
 *  (<xpos>, <ypos>).
 *
 *  If <copyfrom> is a valid object number, the other properties
 *  of the new object are set from the properties of that object,
 *  with the exception of sidedef numbers, which are forced
 *  to OBJ_NO_NONE.
 *
 *  The object is inserted at the exact coordinates given.
 *  No snapping to grid is done.
 */
int InsertObject(obj_type_t objtype, obj_no_t copyfrom, int xpos, int ypos)
{
	MadeChanges = 1;

	switch (objtype)
	{
		case OBJ_THINGS:
		{
			int objnum = RawCreateThing();

			Thing *T = &Things[objnum];

			T->x = xpos;
			T->y = ypos;

			if (is_obj(copyfrom))
			{
				T->type    = Things[copyfrom].type;
				T->angle   = Things[copyfrom].angle;
				T->options = Things[copyfrom].options;
			}
			else
			{
				T->type = default_thing;
				T->angle = 0;
				T->options = 0x07;
			}

			return objnum;
		}

		case OBJ_VERTICES:
		{
			int objnum = RawCreateVertex();

			Vertex *V = &Vertices[objnum];

			V->x = xpos;
			V->y = ypos;

			if (xpos < MapMinX) MapMinX = xpos;
			if (ypos < MapMinY) MapMinY = ypos;
			if (xpos > MapMaxX) MapMaxX = xpos;
			if (ypos > MapMaxY) MapMaxY = ypos;

			MadeMapChanges = 1;

			return objnum;
		}

		case OBJ_LINEDEFS:
		{
			int objnum = RawCreateLineDef();

			LineDef *L = &LineDefs[objnum];

			if (is_obj (copyfrom))
			{
				L->start = LineDefs[copyfrom].start;
				L->end   = LineDefs[copyfrom].end;
				L->flags = LineDefs[copyfrom].flags;
				L->type  = LineDefs[copyfrom].type;
				L->tag   = LineDefs[copyfrom].tag;
			}
			else
			{
				L->start = 0;
				L->end   = NumVertices - 1;
				L->flags = 1;
			}

			L->side_R = OBJ_NO_NONE;
			L->side_L = OBJ_NO_NONE;

			return objnum;
		}

		case OBJ_SIDEDEFS:
		{
			int objnum = RawCreateSideDef();

			SideDef *S = &SideDefs[objnum];

			if (is_obj(copyfrom))
			{
				S->x_offset = SideDefs[copyfrom].x_offset;
				S->y_offset = SideDefs[copyfrom].y_offset;
				S->sector   = SideDefs[copyfrom].sector;

				strncpy(S->upper_tex, SideDefs[copyfrom].upper_tex, WAD_TEX_NAME);
				strncpy(S->lower_tex, SideDefs[copyfrom].lower_tex, WAD_TEX_NAME);
				strncpy(S->mid_tex,   SideDefs[copyfrom].mid_tex,   WAD_TEX_NAME);
			}
			else
			{
				S->sector = NumSectors - 1;

				strcpy(S->upper_tex, "-");
				strcpy(S->lower_tex, "-");
				strcpy(S->mid_tex, default_middle_texture);
			}
			
			MadeMapChanges = 1;

			return objnum;
		}

		case OBJ_SECTORS:
		{
			int objnum = RawCreateSector();

			Sector *S = &Sectors[objnum];

			if (is_obj(copyfrom))
			{
				S->floorh = Sectors[copyfrom].floorh;
				S->ceilh  = Sectors[copyfrom].ceilh;
				S->light  = Sectors[copyfrom].light;
				S->type   = Sectors[copyfrom].type;
				S->tag    = Sectors[copyfrom].tag;

				strncpy(S->floor_tex, Sectors[copyfrom].floor_tex, WAD_FLAT_NAME);
				strncpy(S->ceil_tex,  Sectors[copyfrom].ceil_tex, WAD_FLAT_NAME);
			}
			else
			{
				S->floorh = default_floor_height;
				S->ceilh  = default_ceiling_height;
				S->light  = default_light_level;

				strncpy(S->floor_tex, default_floor_texture, WAD_FLAT_NAME);
				strncpy(S->ceil_tex,  default_ceiling_texture, WAD_FLAT_NAME);
			}

			return objnum;
		}

		default:
			nf_bug ("InsertObject: bad objtype %d", (int) objtype);
	}

	return OBJ_NO_NONE;  /* NOT REACHED */
}



/*
   copy a group of objects to a new position
*/
void CopyObjects(selection_c *list)
{
#if 0  // FIXME !!!!! CopyObjects
	int        n, m;
	SelPtr     cur;
	SelPtr     list1, list2;
	SelPtr     ref1, ref2;

	if (list->empty())
		return;

	/* copy the object(s) */
	switch (objtype)
	{
		case OBJ_THINGS:
			for (cur = obj; cur; cur = cur->next)
			{
				InsertObject (OBJ_THINGS, cur->objnum, Things[cur->objnum].x,
						Things[cur->objnum].y);
				cur->objnum = NumThings - 1;
			}
			MadeChanges = 1;
			break;

		case OBJ_VERTICES:
			for (cur = obj; cur; cur = cur->next)
			{
				InsertObject (OBJ_VERTICES, cur->objnum, Vertices[cur->objnum].x,
						Vertices[cur->objnum].y);
				cur->objnum = NumVertices - 1;
			}
			MadeChanges = 1;
			MadeMapChanges = 1;
			break;

		case OBJ_LINEDEFS:
			list1 = 0;
			list2 = 0;

			// Create the linedefs and maybe the sidedefs
			for (cur = obj; cur; cur = cur->next)
			{
				int old = cur->objnum; // No. of original linedef
				int New;   // No. of duplicate linedef

				InsertObject (OBJ_LINEDEFS, old, 0, 0);
				New = NumLineDefs - 1;

				if (false) ///!!! copy_linedef_reuse_sidedefs)
				{
		/* AYM 1997-07-25: not very orthodox (the New linedef and 
		   the old one use the same sidedefs). but, in the case where
		   you're copying into the same sector, it's much better than
		   having to create the New sidedefs manually. plus it saves
		   space in the .wad and also it makes editing easier (editing
		   one sidedef impacts all linedefs that use it). */
					LineDefs[New].side_R = LineDefs[old].side_R; 
					LineDefs[New].side_L = LineDefs[old].side_L; 
				}
				else
				{
					/* AYM 1998-11-08: duplicate sidedefs too.
					   DEU 5.21 just left the sidedef references to -1. */
					if (is_sidedef (LineDefs[old].side_R))
					{
						InsertObject (OBJ_SIDEDEFS, LineDefs[old].side_R, 0, 0);
						LineDefs[New].side_R = NumSideDefs - 1;
					}
					if (is_sidedef (LineDefs[old].side_L))
					{
						InsertObject (OBJ_SIDEDEFS, LineDefs[old].side_L, 0, 0);
						LineDefs[New].side_L = NumSideDefs - 1; 
					}
				}
				cur->objnum = New;
				if (!IsSelected (list1, LineDefs[New].start))
				{
					SelectObject (&list1, LineDefs[New].start);
					SelectObject (&list2, LineDefs[New].start);
				}
				if (!IsSelected (list1, LineDefs[New].end))
				{
					SelectObject (&list1, LineDefs[New].end);
					SelectObject (&list2, LineDefs[New].end);
				}
			}

			// Create the vertices
			CopyObjects (OBJ_VERTICES, list2);


			// Update the references to the vertices
			for (ref1 = list1, ref2 = list2;
					ref1 && ref2;
					ref1 = ref1->next, ref2 = ref2->next)
			{
				for (cur = obj; cur; cur = cur->next)
				{
					if (ref1->objnum == LineDefs[cur->objnum].start)
						LineDefs[cur->objnum].start = ref2->objnum;
					if (ref1->objnum == LineDefs[cur->objnum].end)
						LineDefs[cur->objnum].end = ref2->objnum;
				}
			}
			ForgetSelection (&list1);
			ForgetSelection (&list2);
			break;

		case OBJ_SECTORS:

			list1 = 0;
			list2 = 0;
			// Create the linedefs (and vertices)
			for (cur = obj; cur; cur = cur->next)
			{
				for (n = 0; n < NumLineDefs; n++)
					if  ((((m = LineDefs[n].side_R) >= 0
									&& SideDefs[m].sector == cur->objnum)
								|| ((m = LineDefs[n].side_L) >= 0
									&& SideDefs[m].sector == cur->objnum))
							&& ! IsSelected (list1, n))
					{
						SelectObject (&list1, n);
						SelectObject (&list2, n);
					}
			}
			CopyObjects (OBJ_LINEDEFS, list2);
			/* create the sidedefs */

			for (ref1 = list1, ref2 = list2;
					ref1 && ref2;
					ref1 = ref1->next, ref2 = ref2->next)
			{
				if ((n = LineDefs[ref1->objnum].side_R) >= 0)
				{
					InsertObject (OBJ_SIDEDEFS, n, 0, 0);
					n = NumSideDefs - 1;

					LineDefs[ref2->objnum].side_R = n;
				}
				if ((m = LineDefs[ref1->objnum].side_L) >= 0)
				{
					InsertObject (OBJ_SIDEDEFS, m, 0, 0);
					m = NumSideDefs - 1;

					LineDefs[ref2->objnum].side_L = m;
				}
				ref1->objnum = n;
				ref2->objnum = m;
			}
			/* create the Sectors */
			for (cur = obj; cur; cur = cur->next)
			{
				InsertObject (OBJ_SECTORS, cur->objnum, 0, 0);

				for (ref1 = list1, ref2 = list2;
						ref1 && ref2;
						ref1 = ref1->next, ref2 = ref2->next)
				{
					if (ref1->objnum >= 0
							&& SideDefs[ref1->objnum].sector == cur->objnum)
						SideDefs[ref1->objnum].sector = NumSectors - 1;
					if (ref2->objnum >= 0
							&& SideDefs[ref2->objnum].sector == cur->objnum)
						SideDefs[ref2->objnum].sector = NumSectors - 1;
				}
				cur->objnum = NumSectors - 1;
			}
			ForgetSelection (&list1);
			ForgetSelection (&list2);
			break;
	}
#endif
}



/*
 *  MoveObjectsToCoords
 *
 *  Move a group of objects to a new position
 *
 *  You must first call it with obj == NULL and newx and newy
 *  set to the coordinates of the reference point (E.G. the
 *  object being dragged).
 *  Then, every time the object being dragged has changed its
 *  coordinates, call the it again with newx and newy set to
 *  the new position and obj set to the selection.
 *
 *  Returns <>0 iff an object was moved.
 */
bool MoveObjectsToCoords (int objtype, SelPtr obj, int newx, int newy, int grid)
{
#if 0  // FIXME !!!!!
	int        dx, dy;
	SelPtr     cur, vertices;
	static int refx, refy; /* previous position */

	if (grid > 0)
	{
		newx = (newx + grid / 2) & ~(grid - 1);
		newy = (newy + grid / 2) & ~(grid - 1);
	}

	// Only update the reference point ?
	if (! obj)
	{
		refx = newx;
		refy = newy;
		return true;
	}

	/* compute the displacement */
	dx = newx - refx;
	dy = newy - refy;
	/* nothing to do? */
	if (dx == 0 && dy == 0)
		return false;

	/* move the object(s) */
	switch (objtype)
	{
		case OBJ_THINGS:
			for (cur = obj; cur; cur = cur->next)
			{
				Things[cur->objnum].x += dx;
				Things[cur->objnum].y += dy;
			}
			refx = newx;
			refy = newy;
			MadeChanges = 1;
			break;

		case OBJ_VERTICES:
			for (cur = obj; cur; cur = cur->next)
			{
				Vertices[cur->objnum].x += dx;
				Vertices[cur->objnum].y += dy;
			}
			refx = newx;
			refy = newy;
			MadeChanges = 1;
			MadeMapChanges = 1;
			break;

		case OBJ_LINEDEFS:
			vertices = list_vertices_of_linedefs (obj);
			MoveObjectsToCoords (OBJ_VERTICES, vertices, newx, newy, grid);
			ForgetSelection (&vertices);
			break;

		case OBJ_SECTORS:

			vertices = list_vertices_of_sectors (obj);
			MoveObjectsToCoords (OBJ_VERTICES, vertices, newx, newy, grid);
			ForgetSelection (&vertices);
			break;
	}
	return true;
#endif

	return false;
}




//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
