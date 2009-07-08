/*
 *  l_unlink.cc
 *  AYM 1998-11-07
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
#include "levels.h"
#include "objects.h"
#include "objid.h"
#include "selectn.h"


/*
 *  unlink_sidedef
 *
 *  For all linedefs in the <linedefs>, see whether the sidedefs
 *  are used by any other linedef _not_in_<linedefs>_. If they
 *  are, duplicate the sidedefs and assign the new duplicate to
 *  the linedef.
 *  If <side1> is set, take care of the first sidedef.
 *  If <side2> is set, take care of the second sidedef.
 *  Both can be set, of course.
 *
 *  This function is intended to "unlink" duplicated linedefs.
 */
void unlink_sidedef (SelPtr linedefs, int side1, int side2)
{
// Array of NumSideDefs bits that tell whether the
// sidedef is used by the linedefs in <linedefs>.
bitvec_c sd_used_in (NumSideDefs);

// Array of NumSideDefs bits that tell whether the
// sidedef is used by the linedefs _not_ in <linedefs>.
bitvec_c sd_used_out (NumSideDefs);

SelPtr cur;
int n;



// Put in sd_used_in a list of all sidedefs
// that are used by linedefs in <linedefs>.
// and in sd_used_out a list of all sidedefs
// that are used by linedefs not in <linedefs>

for (n = 0; n < NumLineDefs; n++)
   {
   if (IsSelected (linedefs, n))
      {
      if (side1 && is_sidedef (LineDefs[n].sidedef1))
         sd_used_in.set (LineDefs[n].sidedef1);
      if (side2 && is_sidedef (LineDefs[n].sidedef2))
         sd_used_in.set (LineDefs[n].sidedef2);
      }
   else
      {
      if (is_sidedef (LineDefs[n].sidedef1))
   sd_used_out.set (LineDefs[n].sidedef1);
      if (is_sidedef (LineDefs[n].sidedef2))
   sd_used_out.set (LineDefs[n].sidedef2);
      }
   }

// For all sidedefs that are used both by a linedef
// in <linedefs> and a linedef _not_ in <linedefs>,
// duplicate the sidedef and make all the linedefs
// in <linedefs> use the copy instead.

for (n = 0; n < NumSideDefs; n++)
   {
   if (sd_used_in.get (n) && sd_used_out.get (n))
      {
      InsertObject (OBJ_SIDEDEFS, n, 0, 0);
      for (cur = linedefs; cur; cur = cur->next)
         {
         if (side1 && LineDefs[cur->objnum].sidedef1 == n)
            LineDefs[cur->objnum].sidedef1 = NumSideDefs - 1;
         if (side2 && LineDefs[cur->objnum].sidedef2 == n)
            LineDefs[cur->objnum].sidedef2 = NumSideDefs - 1;
         }
      }
   }
}




