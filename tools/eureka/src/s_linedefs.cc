/*
 *  s_linedefs.cc
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
#include "levels.h"
#include "objects.h"
#include "objid.h"
#include "s_linedefs.h"
#include "selectn.h"


/*
 *  linedefs_of_sector
 *  Return a bit vector of all linedefs used by the sector.
 *  It's up to the caller to delete the bit vector after use.
 */
bitvec_c *linedefs_of_sector (obj_no_t s)
{
bitvec_c *linedefs = new bitvec_c (NumLineDefs);
for (int n = 0; n < NumLineDefs; n++)
   if (is_sidedef (LineDefs[n].sidedef1)
       && SideDefs[LineDefs[n].sidedef1].sector == s
    || is_sidedef (LineDefs[n].sidedef2)
       && SideDefs[LineDefs[n].sidedef2].sector == s)
      linedefs->set (n);
return linedefs;
}


/*
 *  linedefs_of_sectors
 *  Return a bit vector of all linedefs used by the sectors.
 *  It's up to the caller to delete the bit vector after use.
 */
bitvec_c *linedefs_of_sectors (SelPtr list)
{
bitvec_c *sectors  = list_to_bitvec (list, NumSectors);
bitvec_c *linedefs = new bitvec_c (NumLineDefs);
for (int n = 0; n < NumLineDefs; n++)
   if (   is_sidedef (LineDefs[n].sidedef1)
          && sectors->get (SideDefs[LineDefs[n].sidedef1].sector)
       || is_sidedef (LineDefs[n].sidedef2)
          && sectors->get (SideDefs[LineDefs[n].sidedef2].sector))
      linedefs->set (n);
delete sectors;
return linedefs;
}


/*
 *  linedefs_of_sector
 *  Returns the number of linedefs that face sector <s>
 *  and, if that number is greater than zero, sets <array>
 *  to point on a newly allocated array filled with the
 *  numbers of those linedefs. It's up to the caller to
 *  delete[] the array after use.
 */
int linedefs_of_sector (obj_no_t s, obj_no_t *&array)
{
int count = 0;
for (int n = 0; n < NumLineDefs; n++)
   if (   is_sidedef (LineDefs[n].sidedef1)
          && SideDefs[LineDefs[n].sidedef1].sector == s
       || is_sidedef (LineDefs[n].sidedef2)
          && SideDefs[LineDefs[n].sidedef2].sector == s)
      count++;
if (count > 0)
   {
   array = new obj_no_t[count];
   count = 0;
   for (int n = 0; n < NumLineDefs; n++)
      if (   is_sidedef (LineDefs[n].sidedef1)
       && SideDefs[LineDefs[n].sidedef1].sector == s
    || is_sidedef (LineDefs[n].sidedef2)
       && SideDefs[LineDefs[n].sidedef2].sector == s)
   array[count++] = n;
   }
return count;
}


