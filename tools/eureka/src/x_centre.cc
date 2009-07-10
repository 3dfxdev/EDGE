/*
 *  x_centre.cc
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
#include "linedefs.h"
#include "levels.h"
#include "sectors.h"
#include "v_centre.h"
#include "x_centre.h"


/*
 *  centre_of_objects
 *  Return the coordinates of the centre of a group of objects
 *  of type <obj_type>.
 */
void centre_of_objects (int obj_type, SelPtr list, int *x, int *y)
{
  switch (obj_type)
  {
    case OBJ_LINEDEFS:
      centre_of_linedefs (list, x, y);
      break;
    case OBJ_SECTORS:
      centre_of_sectors (list, x, y);
      break;
    case OBJ_THINGS:
      centre_of_things (list, x, y);
      break;
    case OBJ_VERTICES:
      centre_of_vertices (list, x, y);
      break;
    default:
      fatal_error ("coo: bad obj_type %d", obj_type);
  }
}

