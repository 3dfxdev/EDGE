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
#include "m_strings.h"


extern int MadeChanges;


std::vector<Thing *>   Things;
std::vector<Vertex *>  Vertices;
std::vector<Sector *>  Sectors;
std::vector<SideDef *> SideDefs;
std::vector<LineDef *> LineDefs;
std::vector<RadTrig *> RadTrigs;


string_table_c basis_strtab;


const char * Sector::FloorTex() const
{
	return basis_strtab.get(floor_tex);
}

const char * Sector::CeilTex() const
{
	return basis_strtab.get(ceil_tex);
}

const char * SideDef::UpperTex() const
{
	return basis_strtab.get(upper_tex);
}

const char * SideDef::MidTex() const
{
	return basis_strtab.get(mid_tex);
}

const char * SideDef::LowerTex() const
{
	return basis_strtab.get(lower_tex);
}


Sector * SideDef::SecRef() const
{
	return Sectors[sector];
}

Vertex * LineDef::Start() const
{
	return Vertices[start];
}

Vertex * LineDef::End() const
{
	return Vertices[end];
}

SideDef * LineDef::Right() const
{
	return (right >= 0) ? SideDefs[right] : NULL;
}

SideDef * LineDef::Left() const
{
	return (left >= 0) ? SideDefs[left] : NULL;
}


//------------------------------------------------------------------------

static void InsertThing(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum <= NumThings);

	Things.push_back(NULL);

	for (int n = NumThings-1; n > objnum; n--)
		Things[n] = Things[n - 1];

	Things[objnum] = new Thing;
}


static void InsertLineDef(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum <= NumThings);

	LineDefs.push_back(NULL);

	for (int n = NumLineDefs-1; n > objnum; n--)
		LineDefs[n] = LineDefs[n - 1];

	LineDefs[objnum] = new LineDef;
}


static void InsertVertex(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum <= NumVertices);

	Vertices.push_back(NULL);

	for (int n = NumVertices-1; n > objnum; n--)
		Vertices[n] = Vertices[n - 1];

	Vertices[objnum] = new Vertex;

	// fix references in linedefs

	if (objnum+1 == NumVertices)
		return;

	for (int n = NumLineDefs-1; n >= 0; n--)
	{
		LineDef *L = LineDefs[n];

		if (L->start >= objnum)
			L->start++;

		if (L->end >= objnum)
			L->end++;
	}
}


static void InsertSideDef(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum <= NumSideDefs);

	SideDefs.push_back(NULL);

	for (int n = NumSideDefs-1; n > objnum; n--)
		SideDefs[n] = SideDefs[n - 1];

	SideDefs[objnum] = new SideDef;

	// fix the linedefs references

	if (objnum+1 == NumSideDefs)
		return;

	for (int n = NumLineDefs-1; n >= 0; n--)
	{
		LineDef *L = LineDefs[n];

		if (L->right >= objnum)
			L->right++;

		if (L->left >= objnum)
			L->left++;
	}
}


static void InsertSector(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum <= NumSectors);

	Sectors.push_back(NULL);

	for (int n = NumSectors-1; n > objnum; n--)
		Sectors[n] = Sectors[n - 1];

	Sectors[objnum] = new Sector;

	// fix all references in sidedefs

	if (objnum+1 == NumSectors)
		return;

	for (int n = NumSideDefs-1; n >= 0; n--)
	{
		SideDef *S = SideDefs[n];

		if (S->sector >= objnum)
			S->sector++;
	}
}


static void InsertRadTrig(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum <= NumRadTrigs);

	RadTrigs.push_back(NULL);

	for (int n = NumRadTrigs-1; n > objnum; n--)
		RadTrigs[n] = RadTrigs[n - 1];

	RadTrigs[objnum] = new RadTrig;
}



static void DeleteThing(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumThings);

	delete Things[objnum];

	for (int n = objnum; n < NumThings-1; n++)
		Things[n] = Things[n + 1];

	Things.pop_back();
}


static void DeleteLineDef(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumLineDefs);

	delete LineDefs[objnum];

	for (int n = objnum; n < NumLineDefs-1; n++)
		LineDefs[n] = LineDefs[n + 1];
	
	LineDefs.pop_back();
}


static void DeleteVertex(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumVertices);

	delete Vertices[objnum];

	for (int n = objnum; n < NumVertices-1; n++)
		Vertices[n] = Vertices[n + 1];

	Vertices.pop_back();

	// delete the linedefs bound to this vertex and
	// fix the references

	for (int n = NumLineDefs-1; n >= 0; n--)
	{
		LineDef *L = LineDefs[n];

		if (L->start == objnum || L->end == objnum)
			DeleteLineDef(n);
		else
		{
			if (L->start > objnum)
				L->start--;

			if (L->end > objnum)
				L->end--;
		}
	}
}


static void DeleteSideDef(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumSideDefs);

	delete SideDefs[objnum];

	for (int n = objnum; n < NumSideDefs-1; n++)
		SideDefs[n] = SideDefs[n + 1];

	SideDefs.pop_back();

	// fix the linedefs references

	for (int n = NumLineDefs-1; n >= 0; n--)
	{
		LineDef *L = LineDefs[n];

		if (L->right == objnum)
			L->right = -1;
		else if (L->right > objnum)
			L->right--;

		if (L->left == objnum)
			L->left = -1;
		else if (L->left > objnum)
			L->left--;
	}
}


static void DeleteSector(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumSectors);

	delete Sectors[objnum];

	for (int n = objnum; n < NumSectors-1; n++)
		Sectors[n] = Sectors[n + 1];

	Sectors.pop_back();

	// delete the sidedefs bound to this sector and
	// fix all references

	for (int n = NumSideDefs-1; n >= 0; n--)
	{
		SideDef *S = SideDefs[n];

		if (S->sector == objnum)
			DeleteSideDef(n);
		else if (S->sector > objnum)
			S->sector--;
	}
}


static void DeleteRadTrig(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumRadTrigs);

	delete RadTrigs[objnum];

	for (int n = objnum; n < NumRadTrigs-1; n++)
		RadTrigs[n] = RadTrigs[n + 1];

	RadTrigs.pop_back();
}



void DeleteObject(obj_type_t objtype, int objnum)
{
	switch (objtype)
	{
		case OBJ_THINGS:
			DeleteThing(objnum);
			break;

		case OBJ_LINEDEFS:
			DeleteLineDef(objnum);
			break;

		case OBJ_VERTICES:
			DeleteVertex(objnum);
			break;

		case OBJ_SIDEDEFS:
			DeleteSideDef(objnum);
			break;
		
		case OBJ_SECTORS:
			DeleteSector(objnum);
			break;

		default:
			nf_bug ("DeleteObject: bad objtype %d", (int) objtype);
	}
}


void DeleteObject(const Objid& obj)
{
	DeleteObject(obj.type, obj.num);
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

	std::vector<int> objnums;

	selection_iterator_c it;
	for (list->begin(&it); !it.at_end(); ++it)
		objnums.push_back(*it);

	std::sort(objnums.begin(), objnums.end());

	for (int i = (int)objnums.size()-1; i >= 0; i--)
	{
		DeleteObject(objtype, objnums[i]);
	}

	// the passed in selection is now invalid.  Hence we clear it,
	// but we also re-use it for sidedefs which must be deleted
	// (due to their sectors going away).

	list->clear_all();

	MadeChanges = 1;
}


//------------------------------------------------------------------------



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

			Thing *T = Things[objnum];

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

			L->right = OBJ_NO_NONE;
			L->left = OBJ_NO_NONE;

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
					LineDefs[New].right = LineDefs[old].right; 
					LineDefs[New].left = LineDefs[old].left; 
				}
				else
				{
					/* AYM 1998-11-08: duplicate sidedefs too.
					   DEU 5.21 just left the sidedef references to -1. */
					if (is_sidedef (LineDefs[old].right))
					{
						InsertObject (OBJ_SIDEDEFS, LineDefs[old].right, 0, 0);
						LineDefs[New].right = NumSideDefs - 1;
					}
					if (is_sidedef (LineDefs[old].left))
					{
						InsertObject (OBJ_SIDEDEFS, LineDefs[old].left, 0, 0);
						LineDefs[New].left = NumSideDefs - 1; 
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
					if  ((((m = LineDefs[n].right) >= 0
									&& SideDefs[m].sector == cur->objnum)
								|| ((m = LineDefs[n].left) >= 0
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
				if ((n = LineDefs[ref1->objnum].right) >= 0)
				{
					InsertObject (OBJ_SIDEDEFS, n, 0, 0);
					n = NumSideDefs - 1;

					LineDefs[ref2->objnum].right = n;
				}
				if ((m = LineDefs[ref1->objnum].left) >= 0)
				{
					InsertObject (OBJ_SIDEDEFS, m, 0, 0);
					m = NumSideDefs - 1;

					LineDefs[ref2->objnum].left = m;
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





//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
