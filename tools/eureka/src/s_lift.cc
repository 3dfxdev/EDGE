/*
 *  s_lift.cc
 *  Make lift from sector
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
   turn a Sector into a lift: change the linedefs and sidedefs
*/

void MakeLiftFromSector (int sector)
{
int    sd1, sd2;
int    n, s, tag;
SelPtr ldok, ldflip, ld1s;
SelPtr sect, curs;
int    minh, maxh;

ldok = NULL;
ldflip = NULL;
ld1s = NULL;
sect = NULL;
/* build lists of linedefs that border the Sector */
for (n = 0; n < NumLineDefs; n++)
{
   
   sd1 = LineDefs[n].sidedef1;
   sd2 = LineDefs[n].sidedef2;
   if (sd1 >= 0 && sd2 >= 0)
   {
      
      if (SideDefs[sd2].sector == sector)
      {
   SelectObject (&ldok, n); /* already ok */
   s = SideDefs[sd1].sector;
   if (s != sector && !IsSelected (sect, s))
      SelectObject (&sect, s);
      }
      if (SideDefs[sd1].sector == sector)
      {
   SelectObject (&ldflip, n); /* will be flipped */
   s = SideDefs[sd2].sector;
   if (s != sector && !IsSelected (sect, s))
      SelectObject (&sect, s);
      }
   }
   else if (sd1 >= 0 && sd2 < 0)
   {
      
      if (SideDefs[sd1].sector == sector)
   SelectObject (&ld1s, n); /* wall (one-sided) */
   }
}
/* there must be a way to go on the lift... */
if (sect == NULL)
{
   Beep ();
   Notify (-1, -1, "The lift must be connected to at least one other Sector.", NULL);
   ForgetSelection (&ldok);
   ForgetSelection (&ldflip);
   ForgetSelection (&ld1s);
   return;
}
/* flip the linedefs that have the wrong orientation */
if (ldflip != NULL)
   FlipLineDefs (ldflip, 1);
/* merge the two selection lists */
while (ldflip != NULL)
{
   if (!IsSelected (ldok, ldflip->objnum))
      SelectObject (&ldok, ldflip->objnum);
   UnSelectObject (&ldflip, ldflip->objnum);
}

/* find a free tag number */
tag = FindFreeTag ();

/* find the minimum and maximum altitudes */

minh = 32767;
maxh = -32767;
for (curs = sect; curs; curs = curs->next)
   {
   if (Sectors[curs->objnum].floorh < minh)
      minh = Sectors[curs->objnum].floorh;
   if (Sectors[curs->objnum].floorh > maxh)
      maxh = Sectors[curs->objnum].floorh;
   }
ForgetSelection (&sect);

/* change the lift's floor height if necessary */
if (Sectors[sector].floorh < maxh)
   Sectors[sector].floorh = maxh;

/* change the lift's ceiling height if necessary */
if (Sectors[sector].ceilh < maxh + DOOM_PLAYER_HEIGHT)
   Sectors[sector].ceilh = maxh + DOOM_PLAYER_HEIGHT;

/* assign the new tag number to the lift */
Sectors[sector].tag = tag;

/* change the linedefs and sidedefs */
while (ldok != NULL)
   {
   /* give the "lower lift" type and flags to the linedef */
   
   n = ldok->objnum;
   LineDefs[n].type = 62; /* lower lift (switch) */
   LineDefs[n].flags = 0x04;
   LineDefs[n].tag = tag;
   sd1 = LineDefs[n].sidedef1; /* outside */
   sd2 = LineDefs[n].sidedef2; /* inside */
   /* adjust the textures for the sidedef visible from the outside */
   
   if (strncmp (SideDefs[sd1].middle, "-", WAD_TEX_NAME))
      {
      if (!strncmp (SideDefs[sd1].lower, "-", WAD_TEX_NAME))
   strncpy (SideDefs[sd1].lower, SideDefs[sd1].middle, WAD_TEX_NAME);
      strncpy (SideDefs[sd1].middle, "-", WAD_TEX_NAME);
      }
   if (!strncmp (SideDefs[sd1].lower, "-", WAD_TEX_NAME))
      strncpy (SideDefs[sd1].lower, "SHAWN2", WAD_TEX_NAME);
   /* adjust the textures for the sidedef visible from the lift */
   strncpy (SideDefs[sd2].middle, "-", WAD_TEX_NAME);
   s = SideDefs[sd1].sector;
   
   if (Sectors[s].floorh > minh)
      {
      
      if (strncmp (SideDefs[sd2].middle, "-", WAD_TEX_NAME))
      {
   if (!strncmp (SideDefs[sd2].lower, "-", WAD_TEX_NAME))
      strncpy (SideDefs[sd2].lower, SideDefs[sd1].middle, WAD_TEX_NAME);
   strncpy (SideDefs[sd2].middle, "-", WAD_TEX_NAME);
      }
      if (!strncmp (SideDefs[sd2].lower, "-", WAD_TEX_NAME))
   strncpy (SideDefs[sd2].lower, "SHAWN2", WAD_TEX_NAME);
      }
   else
      {
      
      strncpy (SideDefs[sd2].lower, "-", WAD_TEX_NAME);
      }
   strncpy (SideDefs[sd2].middle, "-", WAD_TEX_NAME);
   
   /* if the ceiling of the sector is lower than that of the lift */
   if (Sectors[s].ceilh < Sectors[sector].ceilh)
      {
      
      if (strncmp (SideDefs[sd2].upper, "-", WAD_TEX_NAME))
   strncpy (SideDefs[sd2].upper, default_upper_texture, WAD_TEX_NAME);
      }
   
   /* if the floor of the sector is above the lift */
   if (Sectors[s].floorh >= Sectors[sector].floorh)
      {
      
      LineDefs[n].type = 88; /* lower lift (walk through) */
      /* flip it, just for fun */
      curs = NULL;
      SelectObject (&curs, n);
      FlipLineDefs (curs, 1);
      ForgetSelection (&curs);
      }
   /* done with this linedef */
   UnSelectObject (&ldok, n);
   }

while (ld1s != NULL)
   {
   /* these are the lift walls (one-sided) */
   
   n = ld1s->objnum;
   LineDefs[n].flags = 0x01;
   sd1 = LineDefs[n].sidedef1;
   /* adjust the textures for the sidedef */
   
   if (!strncmp (SideDefs[sd1].middle, "-", WAD_TEX_NAME))
      strncpy (SideDefs[sd1].middle, default_middle_texture, WAD_TEX_NAME);
   strncpy (SideDefs[sd1].upper, "-", WAD_TEX_NAME);
   strncpy (SideDefs[sd1].lower, "-", WAD_TEX_NAME);
   UnSelectObject (&ld1s, n);
   }
}



