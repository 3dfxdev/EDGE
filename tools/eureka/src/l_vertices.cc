/*
 *  l_vertices.cc
 *  AYM 1998-11-22
 */


/*
This file is part of Yadex.

Yadex incorporates code from DEU 5.21 that was put in the public domain in
1994 by Raphaël Quinet and Brendon Wyber.

The rest of Yadex is Copyright © 1997-2003 André Majorel and others.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA 02111-1307, USA.
*/


#include "yadex.h"
#include "m_bitvec.h"
#include "l_vertices.h"
#include "levels.h"


/*
 *  bv_vertices_of_linedefs
 *  Return a bit vector of all vertices used by the linedefs.
 *  It's up to the caller to delete the bit vector after use.
 */
bitvec_c *bv_vertices_of_linedefs (bitvec_c *linedefs)
{
bitvec_c *vertices;
int n;

vertices = new bitvec_c (NumVertices);
for (n = 0; n < NumLineDefs; n++)
   if (linedefs->get (n))
      {
      vertices->set (LineDefs[n].start);
      vertices->set (LineDefs[n].end);
      }
return vertices;
}


/*
 *  bv_vertices_of_linedefs
 *  Return a bit vector of all vertices used by the linedefs.
 *  It's up to the caller to delete the bit vector after use.
 */
bitvec_c *bv_vertices_of_linedefs (SelPtr list)
{
bitvec_c *vertices;
SelPtr cur;

vertices = new bitvec_c (NumVertices);
for (cur = list; cur; cur = cur->next)
   {
   vertices->set (LineDefs[cur->objnum].start);
   vertices->set (LineDefs[cur->objnum].end);
   }
return vertices;
}


/*
 *  list_vertices_of_linedefs
 *  Return a list of all vertices used by the linedefs
 *  It's up to the caller to delete the list after use.
 */
SelPtr list_vertices_of_linedefs (SelPtr list)
{
bitvec_c *vertices_bitvec;
SelPtr vertices_list = 0;
size_t n;

vertices_bitvec = bv_vertices_of_linedefs (list);
for (n = 0; n < vertices_bitvec->nelements (); n++)
   {
   if (vertices_bitvec->get (n))
      SelectObject (&vertices_list, n);
   }
delete vertices_bitvec;
return vertices_list;
}


