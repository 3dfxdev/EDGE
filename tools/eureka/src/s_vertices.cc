/*
 *  s_vertices.cc
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
#include "objid.h"
#include "s_linedefs.h"
#include "s_vertices.h"


/*
 *  bv_vertices_of_sector
 *  Return a bit vector of all vertices used by a sector.
 *  It's up to the caller to delete the bit vector after use.
 */
bitvec_c *bv_vertices_of_sector (obj_no_t s)
{
  bitvec_c *linedefs = linedefs_of_sector (s);
  bitvec_c *vertices = bv_vertices_of_linedefs (linedefs);
  delete linedefs;
  return vertices;
}


/*
 *  bv_vertices_of_sectors
 *  Return a bit vector of all vertices used by the sectors.
 *  It's up to the caller to delete the bit vector after use.
 */
bitvec_c *bv_vertices_of_sectors (SelPtr list)
{
  bitvec_c *linedefs;  // Linedefs used by the sectors
  bitvec_c *vertices;  // Vertices used by the linedefs

  linedefs = linedefs_of_sectors (list);
  vertices = bv_vertices_of_linedefs (linedefs);
  delete linedefs;
  return vertices;
}


/*
 *  list_vertices_of_sectors
 *  Return a list of all vertices used by the sectors.
 *  It's up to the caller to delete the list after use.
 */
SelPtr list_vertices_of_sectors (SelPtr list)
{
  bitvec_c *vertices_bitvec;
  SelPtr vertices_list = 0;
  size_t n;

  vertices_bitvec = bv_vertices_of_sectors (list);
  for (n = 0; n < vertices_bitvec->nelements (); n++)
  {
    if (vertices_bitvec->get (n))
      SelectObject (&vertices_list, n);
  }
  delete vertices_bitvec;
  return vertices_list;
}


