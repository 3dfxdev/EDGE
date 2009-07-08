/*
 *  s_merge.cc
 *  Merge sectors
 *  AYM 1998-02-03
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
#include "dialog.h"
#include "levels.h"
#include "objects.h"
#include "objid.h"
#include "selectn.h"


/*
   merge two or more Sectors into one
*/

void MergeSectors (SelPtr *slist)
{
SelPtr cur;
int    n, olds, news;

/* save the first Sector number */
news = (*slist)->objnum;
UnSelectObject (slist, news);


/* change all SideDefs references to the other Sectors */
for (cur = *slist; cur; cur = cur->next)
{
   olds = cur->objnum;
   for (n = 0; n < NumSideDefs; n++)
   {
      if (SideDefs[n].sector == olds)
   SideDefs[n].sector = news;
   }
}

/* delete the Sectors */
DeleteObjects (OBJ_SECTORS, slist);

/* the returned list contains only the first Sector */
SelectObject (slist, news);
}



/*
   delete one or several two-sided LineDefs and join the two Sectors
*/

void DeleteLineDefsJoinSectors (SelPtr *ldlist)
{
SelPtr cur, slist;
int    sd1, sd2;
int    s1, s2;
char   msg[80];

/* first, do the tests for all LineDefs */
for (cur = *ldlist; cur; cur = cur->next)
   {
   
   sd1 = LineDefs[cur->objnum].sidedef1;
   sd2 = LineDefs[cur->objnum].sidedef2;
   if (sd1 < 0 || sd2 < 0)
      {
      Beep ();
      sprintf (msg, "ERROR: Linedef #%d has only one side", cur->objnum);
      Notify (-1, -1, msg, NULL);
      return;
      }
   
   s1 = SideDefs[sd1].sector;
   s2 = SideDefs[sd2].sector;
   if (s1 < 0 || s2 < 0)
      {
      Beep ();
      sprintf (msg, "ERROR: Linedef #%d has two sides, but one", cur->objnum);
      Notify (-1, -1, msg, "side is not bound to any sector");
      return;
      }
   }

/* then join the Sectors and delete the LineDefs */
for (cur = *ldlist; cur; cur = cur->next)
   {
   
   sd1 = LineDefs[cur->objnum].sidedef1;
   sd2 = LineDefs[cur->objnum].sidedef2;
   
   s1 = SideDefs[sd1].sector;
   s2 = SideDefs[sd2].sector;
   slist = NULL;
   SelectObject (&slist, s2);
   SelectObject (&slist, s1);
   MergeSectors (&slist);
   ForgetSelection (&slist);
   }
DeleteObjects (OBJ_LINEDEFS, ldlist);
}



