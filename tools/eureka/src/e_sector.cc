//------------------------------------------------------------------------
//  SECTOR STUFF
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

#include "m_dialog.h"
#include "levels.h"
#include "e_linedef.h"
#include "e_sector.h"
#include "m_bitvec.h"
#include "objects.h"
#include "selectn.h"


/*
 *  centre_of_sector
 *  Return the coordinates of the centre of a sector.
 *
 *  FIXME The algorithm is desesperatingly simple-minded and
 *  does not take into account concave sectors and sectors
 *  that are enclosed by several distinct paths of linedefs.
 */
void centre_of_sector (obj_no_t s, int *x, int *y)
{
bitvec_c *vertices = bv_vertices_of_sector (s);
long x_sum  = 0;
long y_sum  = 0;
int  nitems = 0;

for (int n = 0; n < vertices->size(); n++)
   if (vertices->get (n))
      {
      x_sum += Vertices[n].x;
      y_sum += Vertices[n].y;
      nitems++;
      }
if (nitems == 0)
   {
   *x = 0;
   *y = 0;
   }
else
   {
   *x = (int) (x_sum / nitems);
   *y = (int) (y_sum / nitems);
   }
delete vertices;
}


/*
 *  centre_of_sectors
 *  Return the coordinates of the centre of a group of sectors.
 */
void centre_of_sectors (SelPtr list, int *x, int *y)
{
bitvec_c *vertices;
int nitems;
long x_sum;
long y_sum;
int n;

vertices = bv_vertices_of_sectors (list);
x_sum = 0;
y_sum = 0;
nitems = 0;
for (n = 0; n < NumVertices; n++)
   if (vertices->get (n))
      {
      x_sum += Vertices[n].x;
      y_sum += Vertices[n].y;
      nitems++;
      }
if (nitems == 0)
   {
   *x = 0;
   *y = 0;
   }
else
   {
   *x = (int) (x_sum / nitems);
   *y = (int) (y_sum / nitems);
   }
delete vertices;
}


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
   
   sd1 = LineDefs[cur->objnum].side_R;
   sd2 = LineDefs[cur->objnum].side_L;
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
   
   sd1 = LineDefs[cur->objnum].side_R;
   sd2 = LineDefs[cur->objnum].side_L;
   
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



/*
   turn a sector into a door: change the linedefs and sidedefs
*/

void MakeDoorFromSector (int sector)
{
int    sd1, sd2;
int    n, s;
SelPtr ldok, ldflip, ld1s;

ldok = NULL;
ldflip = NULL;
ld1s = NULL;
s = 0;
/* build lists of linedefs that border the sector */
for (n = 0; n < NumLineDefs; n++)
{
   
   sd1 = LineDefs[n].side_R;
   sd2 = LineDefs[n].side_L;
   if (sd1 >= 0 && sd2 >= 0)
   {
      
      if (SideDefs[sd2].sector == sector)
      {
   SelectObject (&ldok, n); /* already ok */
   s++;
      }
      if (SideDefs[sd1].sector == sector)
      {
   SelectObject (&ldflip, n); /* must be flipped */
   s++;
      }
   }
   else if (sd1 >= 0 && sd2 < 0)
   {
      
      if (SideDefs[sd1].sector == sector)
   SelectObject (&ld1s, n); /* wall (one-sided) */
   }
}
/* a normal door has two sides... */
if (s < 2)
{
   Beep ();
   Notify (-1, -1, "The door must be connected to two other Sectors.", NULL);
   ForgetSelection (&ldok);
   ForgetSelection (&ldflip);
   ForgetSelection (&ld1s);
   return;
}
if ((s > 2) && !(Expert || Confirm (-1, -1, "The door will have more than two sides.", "Do you still want to create it?")))
{
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
/* change the linedefs and sidedefs */
while (ldok != NULL)
{
   /* give the "normal door" type and flags to the linedef */
   
   n = ldok->objnum;
   LineDefs[n].type = 1;
   LineDefs[n].flags = 0x04;
   sd1 = LineDefs[n].side_R; /* outside */
   sd2 = LineDefs[n].side_L; /* inside */
   /* adjust the textures for the sidedefs */
   
   if (strncmp (SideDefs[sd1].mid_tex, "-", WAD_TEX_NAME))
   {
      if (!strncmp (SideDefs[sd1].upper_tex, "-", WAD_TEX_NAME))
   strncpy (SideDefs[sd1].upper_tex, SideDefs[sd1].mid_tex, WAD_TEX_NAME);
      strncpy (SideDefs[sd1].mid_tex, "-", WAD_TEX_NAME);
   }
   if (!strncmp (SideDefs[sd1].upper_tex, "-", WAD_TEX_NAME))
      strncpy (SideDefs[sd1].upper_tex, "BIGDOOR2", WAD_TEX_NAME);
   strncpy (SideDefs[sd2].mid_tex, "-", WAD_TEX_NAME);
   UnSelectObject (&ldok, n);
}
while (ld1s != NULL)
{
   /* give the "door side" flags to the linedef */
   
   n = ld1s->objnum;
   LineDefs[n].flags = 0x11;
   sd1 = LineDefs[n].side_R;
   /* adjust the textures for the sidedef */
   
   if (!strncmp (SideDefs[sd1].mid_tex, "-", WAD_TEX_NAME))
      strncpy (SideDefs[sd1].mid_tex, "DOORTRAK", WAD_TEX_NAME);
   strncpy (SideDefs[sd1].upper_tex, "-", WAD_TEX_NAME);
   strncpy (SideDefs[sd1].lower_tex, "-", WAD_TEX_NAME);
   UnSelectObject (&ld1s, n);
}
/* adjust the ceiling height */

Sectors[sector].ceilh = Sectors[sector].floorh;
}



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
   
   sd1 = LineDefs[n].side_R;
   sd2 = LineDefs[n].side_L;
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
   sd1 = LineDefs[n].side_R; /* outside */
   sd2 = LineDefs[n].side_L; /* inside */
   /* adjust the textures for the sidedef visible from the outside */
   
   if (strncmp (SideDefs[sd1].mid_tex, "-", WAD_TEX_NAME))
      {
      if (!strncmp (SideDefs[sd1].lower_tex, "-", WAD_TEX_NAME))
   strncpy (SideDefs[sd1].lower_tex, SideDefs[sd1].mid_tex, WAD_TEX_NAME);
      strncpy (SideDefs[sd1].mid_tex, "-", WAD_TEX_NAME);
      }
   if (!strncmp (SideDefs[sd1].lower_tex, "-", WAD_TEX_NAME))
      strncpy (SideDefs[sd1].lower_tex, "SHAWN2", WAD_TEX_NAME);
   /* adjust the textures for the sidedef visible from the lift */
   strncpy (SideDefs[sd2].mid_tex, "-", WAD_TEX_NAME);
   s = SideDefs[sd1].sector;
   
   if (Sectors[s].floorh > minh)
      {
      
      if (strncmp (SideDefs[sd2].mid_tex, "-", WAD_TEX_NAME))
      {
   if (!strncmp (SideDefs[sd2].lower_tex, "-", WAD_TEX_NAME))
      strncpy (SideDefs[sd2].lower_tex, SideDefs[sd1].mid_tex, WAD_TEX_NAME);
   strncpy (SideDefs[sd2].mid_tex, "-", WAD_TEX_NAME);
      }
      if (!strncmp (SideDefs[sd2].lower_tex, "-", WAD_TEX_NAME))
   strncpy (SideDefs[sd2].lower_tex, "SHAWN2", WAD_TEX_NAME);
      }
   else
      {
      
      strncpy (SideDefs[sd2].lower_tex, "-", WAD_TEX_NAME);
      }
   strncpy (SideDefs[sd2].mid_tex, "-", WAD_TEX_NAME);
   
   /* if the ceiling of the sector is lower than that of the lift */
   if (Sectors[s].ceilh < Sectors[sector].ceilh)
      {
      
      if (strncmp (SideDefs[sd2].upper_tex, "-", WAD_TEX_NAME))
   strncpy (SideDefs[sd2].upper_tex, default_upper_texture, WAD_TEX_NAME);
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
   sd1 = LineDefs[n].side_R;
   /* adjust the textures for the sidedef */
   
   if (!strncmp (SideDefs[sd1].mid_tex, "-", WAD_TEX_NAME))
      strncpy (SideDefs[sd1].mid_tex, default_middle_texture, WAD_TEX_NAME);
   strncpy (SideDefs[sd1].upper_tex, "-", WAD_TEX_NAME);
   strncpy (SideDefs[sd1].lower_tex, "-", WAD_TEX_NAME);
   UnSelectObject (&ld1s, n);
   }
}



/*
 *  linedefs_of_sector
 *  Return a bit vector of all linedefs used by the sector.
 *  It's up to the caller to delete the bit vector after use.
 */
bitvec_c *linedefs_of_sector (obj_no_t s)
{
bitvec_c *linedefs = new bitvec_c (NumLineDefs);
for (int n = 0; n < NumLineDefs; n++)
   if (is_sidedef (LineDefs[n].side_R)
       && SideDefs[LineDefs[n].side_R].sector == s
    || is_sidedef (LineDefs[n].side_L)
       && SideDefs[LineDefs[n].side_L].sector == s)
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
   if (   is_sidedef (LineDefs[n].side_R)
          && sectors->get (SideDefs[LineDefs[n].side_R].sector)
       || is_sidedef (LineDefs[n].side_L)
          && sectors->get (SideDefs[LineDefs[n].side_L].sector))
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
   if (   is_sidedef (LineDefs[n].side_R)
          && SideDefs[LineDefs[n].side_R].sector == s
       || is_sidedef (LineDefs[n].side_L)
          && SideDefs[LineDefs[n].side_L].sector == s)
      count++;
if (count > 0)
   {
   array = new obj_no_t[count];
   count = 0;
   for (int n = 0; n < NumLineDefs; n++)
      if (   is_sidedef (LineDefs[n].side_R)
       && SideDefs[LineDefs[n].side_R].sector == s
    || is_sidedef (LineDefs[n].side_L)
       && SideDefs[LineDefs[n].side_L].sector == s)
   array[count++] = n;
   }
return count;
}


void swap_flats (SelPtr list)
{
  for (SelPtr cur = list; cur != NULL; cur = cur->next)
  {
    wad_flat_name_t tmp;
    struct Sector *s = Sectors + cur->objnum;

    memcpy (tmp,          s->floor_tex, sizeof tmp);
    memcpy (s->floor_tex, s->ceil_tex,  sizeof s->floor_tex);
    memcpy (s->ceil_tex,  tmp,          sizeof s->ceil_tex);

    MadeChanges = 1;
  }
}



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

  vertices_bitvec = bv_vertices_of_sectors (list);

  for (int n = 0; n < vertices_bitvec->size(); n++)
  {
    if (vertices_bitvec->get (n))
      SelectObject (&vertices_list, n);
  }
  delete vertices_bitvec;
  return vertices_list;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
