/*
 *  s_slice.cc
 *  Cut a slice out of a sector
 *  AYM 2001-09-11
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
#include <map>
#include "dialog.h"
#include "levels.h"
#include "objects.h"
#include "objid.h"
#include "s_slice.h"


class Secref      // Reference to a sector
{
  public :
    Secref () : linedef1 ('\0'), linedef2 ('\0') { }
    char linedef1;
    char linedef2;
};


/*
 *  sector_slice - cut a slice out of a sector
 */
void sector_slice (obj_no_t linedef1, obj_no_t linedef2)
{
  /* We have between 0 and 4 sidedefs. We need to make sure that
     there are exactly 2 that face the same sector and that they
     belong to different linedefs.

     If a linedef has more than one sidedef that faces the same
     sector, we can't decide which one faces the other linedef.
     Well, we can but it's hard for a dummy like me.

     The problem is the same if the linedefs have two sectors in
     common. Consider the following setup :
    ____________________________________
         |                                    |
         |              sector 0              |
         |      ________________________      |
         |     |                        |     |
         |     |        sector 1        |     |
         |     |      ____________      |     |
         |     |     |            |     |     |
         |     |     |  sector 0  |-L1  |-L2  |
         |     |     |____________|     |     |
         |     |                        |     |
         |     |                        |     |
         |     |________________________|     |
         |                                    |
         |                                    |
         |____________________________________|

     How do you decide to which vertex of L2 should the start
     vertex of L1 be linked ? */

  if (! is_linedef (linedef1))    // Paranoia
  {
    char buf[100];
    y_snprintf (buf, sizeof buf,
  "First argument is not a valid linedef number");
    Notify (-1, -1, buf, 0);
    return;
  }

  if (! is_linedef (linedef2))    // Paranoia
  {
    char buf[100];
    y_snprintf (buf, sizeof buf,
  "Second argument is not a valid linedef number");
    Notify (-1, -1, buf, 0);
    return;
  }

  if (linedef1 == linedef2)
  {
    char buf[100];
    y_snprintf (buf, sizeof buf, "Both arguments are the same linedef");
    Notify (-1, -1, buf, 0);
    return;
  }

  obj_no_t l1start = LineDefs[linedef1].start;
  obj_no_t l1end   = LineDefs[linedef1].end;
  obj_no_t l2start = LineDefs[linedef2].start;
  obj_no_t l2end   = LineDefs[linedef2].end;

  if (l1start == l2start && l1end == l2end
    || l1start == l2end && l1end == l2start)
  {
    char buf[100];
    y_snprintf (buf, sizeof buf, "Linedefs %d and %d are superimposed",
        int (linedef1), int (linedef2));
    Notify (-1, -1, buf, 0);
    return;
  }

  obj_no_t l1sdr = LineDefs[linedef1].sidedef1;
  obj_no_t l1sdl = LineDefs[linedef1].sidedef2;
  obj_no_t l2sdr = LineDefs[linedef2].sidedef1;
  obj_no_t l2sdl = LineDefs[linedef2].sidedef2;

  obj_no_t l1sr = is_sidedef (l1sdr) ? SideDefs[l1sdr].sector : OBJ_NO_NONE;
  obj_no_t l1sl = is_sidedef (l1sdl) ? SideDefs[l1sdl].sector : OBJ_NO_NONE;
  obj_no_t l2sr = is_sidedef (l2sdr) ? SideDefs[l2sdr].sector : OBJ_NO_NONE;
  obj_no_t l2sl = is_sidedef (l2sdl) ? SideDefs[l2sdl].sector : OBJ_NO_NONE;

  if (is_sector (l1sr) && is_sector (l1sl) && l1sr == l1sl)
  {
    char buf[100];
    y_snprintf (buf, sizeof buf, "Linedef %d has both sides in the same sector",
  int (linedef1));
    Notify (-1, -1, buf, 0);
    return;
  }

  if (is_sector (l2sr) && is_sector (l2sl) && l2sr == l2sl)
  {
    char buf[100];
    y_snprintf (buf, sizeof buf, "Linedef %d has both sides in the same sector",
  int (linedef2));
    Notify (-1, -1, buf, 0);
    return;
  }

  // Verify that the linedefs have exactly one sector in common
  typedef std::map<obj_no_t, Secref> secref_list_t;
  secref_list_t secref;
  if (is_sector (l1sr))
    secref[l1sr].linedef1 = 'r';
  if (is_sector (l1sl))
    secref[l1sl].linedef1 = 'l';
  if (is_sector (l2sr))
    secref[l2sr].linedef2 = 'r';
  if (is_sector (l2sl))
    secref[l2sl].linedef2 = 'l';
  obj_no_t sector;
  int count = 0;
  for (secref_list_t::iterator i = secref.begin (); i != secref.end (); i++)
  {
    if (i->second.linedef1 != '\0' && i->second.linedef2 != '\0')
    {
      sector = i->first;
      count++;
    }
  }
  if (count < 1)
  {
    char buf[100];
    y_snprintf (buf, sizeof buf,
  "Linedefs %d and %d don't face the same sector",
  int (linedef1), int (linedef2));
    Notify (-1, -1, buf, 0);
    return;
  }
  if (count > 1)
  {
    char buf[100];
    y_snprintf (buf, sizeof buf,
  "Linedefs %d and %d have more than one sector in common",
  int (linedef1), int (linedef2));
    Notify (-1, -1, buf, 0);
    return;
  }

  // Insert new sector between linedefs
  obj_no_t la0, la1;    // Start and end of the first linedef (the
        // one that goes from linedef1 to linedef2)
  obj_no_t lb0, lb1;    // Start and end of the second linedef (the
        // one that goes from linedef2 to linedef1)
  char side = secref[sector].linedef1;
  if (side == 'r')
  {
    la0 = l1end;
    lb1 = l1start;
  }
  else if (side == 'l')
  {
    la0 = l1start;
    lb1 = l1end;
  }
  else          // Can't happen
  {
    nf_bug ("sector %d: linedef1 = %02Xh", int (sector), side);
    return;
  }

  side = secref[sector].linedef2;
  if (side == 'r')
  {
    la1 = l2start;
    lb0 = l2end;
  }
  else if (side == 'l')
  {
    la1 = l2end;
    lb0 = l2start;
  }
  else          // Can't happen
  {
    nf_bug ("sector %d: linedef2 = %02Xh", int (sector), side);
    return;
  }

  // Verify that there's no linedef already between linedef1 and linedef2
  {
    for (int n = 0; n < NumLineDefs; n++)
    {
      if (n == linedef1 || n == linedef2)
  continue;
      if (LineDefs[n].start == la0 && LineDefs[n].end == la1
       || LineDefs[n].start == la1 && LineDefs[n].end == la0
       || LineDefs[n].start == lb0 && LineDefs[n].end == lb1
       || LineDefs[n].start == lb1 && LineDefs[n].end == lb0)
      {
  char buf[100];
  y_snprintf (buf, sizeof buf,
      "A linedef already exists between linedefs %d and %d (linedef %d)",
      int (linedef1), int (linedef2), int (n));
  Notify (-1, -1, buf, 0);
  return;
      }
    }
  }
  
  // Create new sector
  InsertObject (OBJ_SECTORS, sector, 0, 0);

  // Create new linedef from linedef1 to linedef2
  if (la0 != la1)
  {
    InsertObject (OBJ_LINEDEFS, -1, 0, 0);
    LineDefs[NumLineDefs - 1].start = la0;
    LineDefs[NumLineDefs - 1].end   = la1;
    LineDefs[NumLineDefs - 1].flags = 4;
    InsertObject (OBJ_SIDEDEFS, -1, 0, 0);    // Right sidedef
    SideDefs[NumSideDefs - 1].sector = NumSectors - 1;  // Redundant
    strncpy (SideDefs[NumSideDefs - 1].middle, "-", 8);
    LineDefs[NumLineDefs - 1].sidedef1 = NumSideDefs - 1;
    InsertObject (OBJ_SIDEDEFS, -1, 0, 0);    // Left sidedef
    SideDefs[NumSideDefs - 1].sector = sector;
    strncpy (SideDefs[NumSideDefs - 1].middle, "-", 8);
    LineDefs[NumLineDefs - 1].sidedef2 = NumSideDefs - 1;
  }

  // Create new linedef from linedef2 to linedef1
  if (lb0 != lb1)
  {
    InsertObject (OBJ_LINEDEFS, -1, 0, 0);
    LineDefs[NumLineDefs - 1].start = lb0;
    LineDefs[NumLineDefs - 1].end   = lb1;
    LineDefs[NumLineDefs - 1].flags = 4;
    InsertObject (OBJ_SIDEDEFS, -1, 0, 0);    // Right sidedef
    SideDefs[NumSideDefs - 1].sector = NumSectors - 1;  // Redundant
    strncpy (SideDefs[NumSideDefs - 1].middle, "-", 8);
    LineDefs[NumLineDefs - 1].sidedef1 = NumSideDefs - 1;
    InsertObject (OBJ_SIDEDEFS, -1, 0, 0);    // Left sidedef
    SideDefs[NumSideDefs - 1].sector = sector;
    strncpy (SideDefs[NumSideDefs - 1].middle, "-", 8);
    LineDefs[NumLineDefs - 1].sidedef2 = NumSideDefs - 1;
  }

  // Adjust sector references for linedef1
  side = secref[sector].linedef1;
  if (side == 'r')
    SideDefs[LineDefs[linedef1].sidedef1].sector = NumSectors - 1;
  else if (side == 'l')
    SideDefs[LineDefs[linedef1].sidedef2].sector = NumSectors - 1;

  // Adjust sector references for linedef2
  side = secref[sector].linedef2;
  if (side == 'r')
    SideDefs[LineDefs[linedef2].sidedef1].sector = NumSectors - 1;
  else if (side == 'l')
    SideDefs[LineDefs[linedef2].sidedef2].sector = NumSectors - 1;

  MadeChanges = 1;
  MadeMapChanges = 1;
}

