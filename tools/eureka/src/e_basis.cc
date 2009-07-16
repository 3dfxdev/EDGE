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

static void RawInsertThing(int objnum, int *ptr)
{
	SYS_ASSERT(0 <= objnum && objnum <= NumThings);

	Things.push_back(NULL);

	for (int n = NumThings-1; n > objnum; n--)
		Things[n] = Things[n - 1];

	Things[objnum] = (Thing*) ptr;
}

static void RawInsertLineDef(int objnum, int *ptr)
{
	SYS_ASSERT(0 <= objnum && objnum <= NumThings);

	LineDefs.push_back(NULL);

	for (int n = NumLineDefs-1; n > objnum; n--)
		LineDefs[n] = LineDefs[n - 1];

	LineDefs[objnum] = (LineDef*) ptr;
}

static void RawInsertVertex(int objnum, int *ptr)
{
	SYS_ASSERT(0 <= objnum && objnum <= NumVertices);

	Vertices.push_back(NULL);

	for (int n = NumVertices-1; n > objnum; n--)
		Vertices[n] = Vertices[n - 1];

	Vertices[objnum] = (Vertex*) ptr;

	// fix references in linedefs

	if (objnum+1 < NumVertices)
	{
		for (int n = NumLineDefs-1; n >= 0; n--)
		{
			LineDef *L = LineDefs[n];

			if (L->start >= objnum)
				L->start++;

			if (L->end >= objnum)
				L->end++;
		}
	}
}

static void RawInsertSideDef(int objnum, int *ptr)
{
	SYS_ASSERT(0 <= objnum && objnum <= NumSideDefs);

	SideDefs.push_back(NULL);

	for (int n = NumSideDefs-1; n > objnum; n--)
		SideDefs[n] = SideDefs[n - 1];

	SideDefs[objnum] = (SideDef*) ptr;

	// fix the linedefs references

	if (objnum+1 < NumSideDefs)
	{
		for (int n = NumLineDefs-1; n >= 0; n--)
		{
			LineDef *L = LineDefs[n];

			if (L->right >= objnum)
				L->right++;

			if (L->left >= objnum)
				L->left++;
		}
	}
}

static void RawInsertSector(int objnum, int *ptr)
{
	SYS_ASSERT(0 <= objnum && objnum <= NumSectors);

	Sectors.push_back(NULL);

	for (int n = NumSectors-1; n > objnum; n--)
		Sectors[n] = Sectors[n - 1];

	Sectors[objnum] = (Sector*) ptr;

	// fix all sidedef references

	if (objnum+1 < NumSectors)
	{
		for (int n = NumSideDefs-1; n >= 0; n--)
		{
			SideDef *S = SideDefs[n];

			if (S->sector >= objnum)
				S->sector++;
		}
	}
}

static void RawInsertRadTrig(int objnum, int *ptr)
{
	SYS_ASSERT(0 <= objnum && objnum <= NumRadTrigs);

	RadTrigs.push_back(NULL);

	for (int n = NumRadTrigs-1; n > objnum; n--)
		RadTrigs[n] = RadTrigs[n - 1];

	RadTrigs[objnum] = (RadTrig*) ptr;
}


static int * RawDeleteThing(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumThings);

	int * result = (int*) Things[objnum];

	for (int n = objnum; n < NumThings-1; n++)
		Things[n] = Things[n + 1];

	Things.pop_back();

	return result;
}

static void RawInsert(obj_type_t objtype, int objnum, int *ptr)
{
	switch (objtype)
	{
		case OBJ_THINGS:
			RawInsertThing(objnum, ptr);

		case OBJ_LINEDEFS:
			RawInsertLineDef(objnum, ptr);

		case OBJ_VERTICES:
			RawInsertVertex(objnum, ptr);

		case OBJ_SIDEDEFS:
			RawInsertSideDef(objnum, ptr);
		
		case OBJ_SECTORS:
			RawInsertSector(objnum, ptr);

		case OBJ_RADTRIGS:
			RawInsertRadTrig(objnum, ptr);

		default:
			nf_bug ("RawInsert: bad objtype %d", (int) objtype);
	}
}


static int * RawDeleteLineDef(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumLineDefs);

	int * result = (int*) LineDefs[objnum];

	for (int n = objnum; n < NumLineDefs-1; n++)
		LineDefs[n] = LineDefs[n + 1];
	
	LineDefs.pop_back();

	return result;
}

static int * RawDeleteVertex(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumVertices);

	int * result = (int*) Vertices[objnum];

	for (int n = objnum; n < NumVertices-1; n++)
		Vertices[n] = Vertices[n + 1];

	Vertices.pop_back();

	// fix the linedef references

	if (objnum < NumVertices)
	{
		for (int n = NumLineDefs-1; n >= 0; n--)
		{
			LineDef *L = LineDefs[n];

			if (L->start > objnum)
				L->start--;

			if (L->end > objnum)
				L->end--;
		}
	}

	return result;
}

static int * RawDeleteSideDef(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumSideDefs);

	int * result = (int*) SideDefs[objnum];

	for (int n = objnum; n < NumSideDefs-1; n++)
		SideDefs[n] = SideDefs[n + 1];

	SideDefs.pop_back();

	// fix the linedefs references

	if (objnum < NumSideDefs)
	{
		for (int n = NumLineDefs-1; n >= 0; n--)
		{
			LineDef *L = LineDefs[n];

			if (L->right > objnum)
				L->right--;

			if (L->left > objnum)
				L->left--;
		}
	}

	return result;
}

static int * RawDeleteSector(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumSectors);

	int * result = (int*) Sectors[objnum];

	for (int n = objnum; n < NumSectors-1; n++)
		Sectors[n] = Sectors[n + 1];

	Sectors.pop_back();

	// fix sidedef references

	if (objnum < NumSectors)
	{
		for (int n = NumSideDefs-1; n >= 0; n--)
		{
			SideDef *S = SideDefs[n];

			if (S->sector > objnum)
				S->sector--;
		}
	}

	return result;
}

static int * RawDeleteRadTrig(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumRadTrigs);

	int * result = (int*) RadTrigs[objnum];

	for (int n = objnum; n < NumRadTrigs-1; n++)
		RadTrigs[n] = RadTrigs[n + 1];

	RadTrigs.pop_back();

	return result;
}

static int * RawDelete(obj_type_t objtype, int objnum)
{
	switch (objtype)
	{
		case OBJ_THINGS:
			return RawDeleteThing(objnum);

		case OBJ_LINEDEFS:
			return RawDeleteLineDef(objnum);

		case OBJ_VERTICES:
			return RawDeleteVertex(objnum);

		case OBJ_SIDEDEFS:
			return RawDeleteSideDef(objnum);
		
		case OBJ_SECTORS:
			return RawDeleteSector(objnum);

		case OBJ_RADTRIGS:
			return RawDeleteRadTrig(objnum);

		default:
			nf_bug ("RawDelete: bad objtype %d", (int) objtype);
	}
}


void edit_op_c::Apply()
{
	int * pos;

	switch (action)
	{
		case OP_CHANGE:
			pos = RawGetBase(objtype, objnum);
			std::swap(pos[field], value);
			return;

		case OP_DELETE:
			ptr = RawDelete(objtype, objnum);
			action = OP_INSERT;  // reverse the operation
			return;

		case OP_INSERT:
			RawInsert(objtype, objnum, ptr);
			ptr = NULL;
			action = OP_DELETE;  // reverse the operation
			return;

		default:
			nf_bug("edit_op_c::Apply");
	}
}


void ApplyGroup(op_group_c& grp)
{
	int total = (int)grp.size();

	for (int i = 0; i < total; i++)
		grp[i].Apply();

	// reverse order of all operations
	
	for (int i = 0; i < total/2; i++)
		std::swap(grp[i], grp[total-1-i]);
}


//------------------------------------------------------------------------


void AnalyseChange(op_group_c& grp, obj_type_t type, int objnum,
                   byte field, int new_val)
{
	edit_op_c op;

	op.action  = OP_CHANGE;
	op.objtype = type;
	op.field   = field;
	op.objnum  = objnum;
	op.value   = new_val;

	grp.push_back(op);

	op->Apply();
}


void AnalyseDelete(op_group_c& grp, obj_type_t type, int objnum);


static void AnalyseDeleteVertex(op_group_c& grp, int objnum)
{
	// delete any linedefs bound to this vertex

	for (int n = NumLineDefs-1; n >= 0; n--)
	{
		LineDef *L = LineDefs[n];

		if (L->start == objnum || L->end == objnum)
			AnalyseDelete(grp, OBJ_LINEDEFS, n);
	}
}

static void AnalyseDeleteSideDef(op_group_c& grp, int objnum)
{
	// unbind from linedefs

	for (int n = NumLineDefs-1; n >= 0; n--)
	{
		LineDef *L = LineDefs[n];

		if (L->right == objnum)
			AnalyseChange(grp, OBJ_LINEDEFS, n, LineDef::F_RIGHT, -1);

		if (L->left == objnum)
			AnalyseChange(grp, OBJ_LINEDEFS, n, LineDef::F_LEFT, -1);
	}
}

static void AnalyseDeleteSector(op_group_c& grp, int objnum)
{
	// delete the sidedefs bound to this sector

	for (int n = NumSideDefs-1; n >= 0; n--)
	{
		SideDef *S = SideDefs[n];

		if (S->sector == objnum)
			AnalyseDelete(grp, OBJ_SIDEDEFS, n);
	}
}


void AnalyseDelete(op_group_c& grp, obj_type_t type, int objnum)
{
	edit_op_c op;

	op.action  = OP_DELETE;
	op.objtype = type;
	op.objnum  = objnum;

	// must analyse sidedefs before the deletion
	if (type == OBJ_SIDEDEFS)
		AnalyseDeleteSideDef(grp, objnum);

	grp.push_back(op);

	op->Apply();

	// these must be done after the deletion
	if (type == OBJ_VERTICES)
		AnalyseDeleteVertex(grp, objnum);

	if (type == OBJ_SECTORS)
		AnalyseDeleteSector(grp, objnum);
}


void AnalyseCreate(op_group_c& grp, obj_type_t type)
{
	edit_op_c op;

	op.action  = OP_INSERT;
	op.objtype = type;

	switch (type)
	{
		case OBJ_THINGS:   op.objnum = NumThings;   break;
		case OBJ_VERTICES: op.objnum = NumVertices; break;
		case OBJ_SIDEDEFS: op.objnum = NumSideDefs; break;
		case OBJ_LINEDEFS: op.objnum = NumLineDefs; break;
		case OBJ_SECTORS:  op.objnum = NumSectors;  break;
		case OBJ_RADTRIGS: op.objnum = NumRadTrigs; break;
	}

	grp.push_back(op);

	op->Apply();
}


#if 0

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




#endif

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
