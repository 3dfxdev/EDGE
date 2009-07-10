/*
 *  xref.cc
 *  Cross reference stuff (who references who ?)
 *  and miscellaneous test/debug/exploration funcs.
 *  AYM 1999-03-31
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


#include "main.h"
#include "levels.h"
#include "selectn.h"


void xref_sidedef ()
{
  int sidedef_no;
  printf ("Enter sidedef number : ");
  scanf ("%d", &sidedef_no);
  printf ("Sidedef %d is used by linedefs:", sidedef_no);
  int count = 0;
  for (int n = 0; n < NumLineDefs; n++)
  {
    if (LineDefs[n].sidedef1 == sidedef_no)
    {
      printf (" %dR", n);
      count++;
    }
    if (LineDefs[n].sidedef2 == sidedef_no)
    {
      printf (" %dL", n);
      count++;
    }
  }
  printf (" (total %d linedefs)\n", count);
}


void secret_sectors ()
{
  printf ("Secret sectors:");
  int count = 0;
  for (int n = 0; n < NumSectors; n++)
    if (Sectors[n].special ==  9)  // FIXME hard-coded
    {
      printf (" %d", n);
      count++;
    }
  printf (" (total %d)\n", count);

}


void unknown_linedef_type (SelPtr *list)
{
  for (int n = 0; n < NumLineDefs; n++)
    if (*GetLineDefTypeName (LineDefs[n].type) == '?')
      SelectObject (list, n);
}


void bad_sector_number (SelPtr *list)
{
  for (int n = 0; n < NumLineDefs; n++)
  {
    int s1 = LineDefs[n].sidedef1;
    int s2 = LineDefs[n].sidedef2;
    if (s1 >= 0 && s1 < NumSideDefs
  && SideDefs[s1].sector < 0 || SideDefs[s1].sector >= NumSectors
     || s2 >= 0 && s2 < NumSideDefs
        && SideDefs[s2].sector < 0 || SideDefs[s2].sector >= NumSectors)
      SelectObject (list, n);
  }
}


/*
 *  A stopgap to please IvL
 */
void list_tagged_sectors (int tag)
{
  printf ("tag %d:", tag);
  int count = 0;

  for (int n = 0; n < NumSectors; n++)
    if (Sectors[n].tag == tag)
    {
      printf (" %d", n);
      count++;
    }
  printf (" (total %d)\n", count);
}


/*
 *  A stopgap to please IvL
 */
void list_tagged_linedefs (int tag)
{
  printf ("tag %d:", tag);
  int count = 0;

  for (int n = 0; n < NumLineDefs; n++)
    if (LineDefs[n].tag == tag)
    {
      printf (" %d", n);
      count++;
    }
  printf (" (total %d)\n", count);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
