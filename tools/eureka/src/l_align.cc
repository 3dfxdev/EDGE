/*
 *  l_align.cc
 *  Linedef/sidedef texture alignment
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
/// #include "entry.h"
#include "gfx.h"
#include "levels.h"
#include "objid.h"
#include "selectn.h"
#include "r_images.h"


/*
   get the absolute height from which the textures are drawn
*/

int GetTextureRefHeight (int sidedef)
{
int l, sector;
int otherside = OBJ_NO_NONE;

/* find the sidedef on the other side of the LineDef, if any */

for (l = 0; l < NumLineDefs; l++)
  {
  if (LineDefs[l].sidedef1 == sidedef)
    {
    otherside = LineDefs[l].sidedef2;
    break;
    }
  if (LineDefs[l].sidedef2 == sidedef)
    {
    otherside = LineDefs[l].sidedef1;
    break;
    }
  }
/* get the Sector number */

sector = SideDefs[sidedef].sector;
/* if the upper texture is displayed,
   then the reference is taken from the other Sector */
if (otherside >= 0)
  {
  l = SideDefs[otherside].sector;
  if (l > 0)
    {
    
    if (Sectors[l].ceilh < Sectors[sector].ceilh
     && Sectors[l].ceilh > Sectors[sector].floorh)
      sector = l;
    }
  }
/* return the altitude of the ceiling */

if (sector >= 0)
  return Sectors[sector].ceilh; /* textures are drawn from the ceiling down */
else
  return 0; /* yuck! */
}




/*
   Align all textures for the given SideDefs

   Note from RQ:
      This function should be improved!
      But what should be improved first is the way the SideDefs are selected.
      It is stupid to change both sides of a wall when only one side needs
      to be changed.  But with the current selection method, there is no
      way to select only one side of a two-sided wall.
*/

void AlignTexturesY (SelPtr *sdlist)
{
int h, refh;

if (! *sdlist)
   return;

/* get the reference height from the first sidedef */
refh = GetTextureRefHeight ((*sdlist)->objnum);

SideDefs[(*sdlist)->objnum].yoff = 0;
UnSelectObject (sdlist, (*sdlist)->objnum);

/* adjust Y offset in all other SideDefs */
while (*sdlist)
  {
  h = GetTextureRefHeight ((*sdlist)->objnum);
  
  SideDefs[(*sdlist)->objnum].yoff = (refh - h) % 128;
  UnSelectObject (sdlist, (*sdlist)->objnum);
  }
MadeChanges = 1;
}



/*
   Function is to align all highlighted textures in the X-axis

   Note from RJH:
   LineDefs highlighted are read off in reverse order of highlighting.
   The '*sdlist' is in the reverse order of the above mentioned LineDefs
   i.e. the first linedef sidedefs you highlighted will be processed first.

   Note from RQ:
   See also the note for the previous function.

   Note from RJH:
   For the menu for aligning textures 'X' NOW operates upon the fact that
   ALL the SIDEDEFS from the selected LINEDEFS are in the *SDLIST, 2nd
   sidedef is first, 1st sidedef is 2nd). Aligning textures X now does
   SIDEDEF 1's and SIDEDEF 2's.  If the selection process is changed,
   the following needs to be altered radically.
*/

void AlignTexturesX (SelPtr *sdlist)
{
#if 0 // TODO: AlignTexturesX

      /* FIRST texture name used in the highlited objects */
char texname[WAD_TEX_NAME + 1];
char errormessage[80];  /* area to hold the error messages produced */
int  ldef;    /* linedef number */
int  sd1;   /* current sidedef in *sdlist */
int  vert1, vert2;  /* vertex 1 and 2 for the linedef under scrutiny */
int  xoffset;   /* xoffset accumulator */
int  useroffset;  /* user input offset for first input */
s16_t  texlength;   /* the length of texture to format to */
int  length;    /* length of linedef under scrutiny */
s16_t  dummy;   /* holds useless data */
int  type_off;    /* do we have an initial offset to use */
int  type_tex;    /* do we check for same textures */
int  type_sd;   /* do we align sidedef 1 or sidedef 2 */

type_sd  = 0;   /* which sidedef to align, 1=SideDef1, 2=SideDef2 */
type_tex = 0;   /* do we test for similar textures, 0 = no, 1 = yes */
type_off = 0;   /* do we have an inital offset, 0 = no, 1 = yes */

vert1   = -1;
vert2   = -1;   /* 1st time round the while loop the -1 value is needed */
texlength  = 0;
xoffset    = 0;
useroffset = 0;

switch (vDisplayMenu (250, 110, "Aligning textures (X offset) :",

  " Sidedef 1, Check for identical textures.     ", YK_, 0,
  " Sidedef 1, As above, but with inital offset. ", YK_, 0,
  " Sidedef 1, No texture checking.              ", YK_, 0,
  " Sidedef 1, As above, but with inital offset. ", YK_, 0,

  " Sidedef 2, Check for identical textures.     ", YK_, 0,
  " Sidedef 2, As above, but with inital offset. ", YK_, 0,
  " Sidedef 2, No texture checking.              ", YK_, 0,
  " Sidedef 2, As above, but with inital offset. ", YK_, 0,
  NULL))
  {
  case 1:       /* Sidedef 1 with checking for same textures   */
    type_sd = 1; type_tex = 1; type_off = 0;
    break;

  case 2:       /* Sidedef 1 as above, but with inital offset  */
    type_sd = 1; type_tex = 1; type_off = 1;
    break;

  case 3:       /* Sidedef 1 regardless of same textures       */
    type_sd = 1; type_tex = 0; type_off = 0;
    break;

  case 4:       /* Sidedef 1 as above, but with inital offset  */
    type_sd = 1; type_tex = 0; type_off = 1;
    break;

  case 5:       /* Sidedef 2 with checking for same textures   */
    type_sd = 2; type_tex = 1; type_off = 0;
    break;

  case 6:       /* Sidedef 2 as above, but with initial offset */
    type_sd = 2; type_tex = 1; type_off = 1;
    break;

  case 7:       /* Sidedef 2 regardless of same textures       */
    type_sd = 2; type_tex = 0; type_off = 0;
    break;

  case 8:       /* Sidedef 2 as above, but with initial offset */
    type_sd = 2; type_tex = 0; type_off = 1;
    break;
  }

ldef = 0;
if (! *sdlist)
   {
   Notify (-1, -1, "Error in AlignTexturesX: list is empty", 0);
   return;
   }
sd1 = (*sdlist)->objnum;

if (type_sd == 1) /* throw out all 2nd SideDefs untill a 1st is found */
  {
  while (*sdlist && LineDefs[ldef].sidedef1!=sd1 && ldef<=NumLineDefs)
    {
    ldef++;
    if (LineDefs[ldef].sidedef2 == sd1)
      {
      UnSelectObject (sdlist, (*sdlist)->objnum);
      if (! *sdlist)
         return;
      sd1 = (*sdlist)->objnum;
      ldef = 0;
      }
    }
  }

if (type_sd == 2) /* throw out all 1st SideDefs untill a 2nd is found */
  {
  while (LineDefs[ldef].sidedef2!=sd1 && ldef<=NumLineDefs)
    {
    ldef++;
    if (LineDefs[ldef].sidedef1 == sd1)
      {
      UnSelectObject (sdlist, (*sdlist)->objnum);
      if (! *sdlist)
   return;
      sd1 = (*sdlist)->objnum;
      ldef = 0;
      }
    }
  }



/* get texture name of the sidedef in the *sdlist) */
strncpy (texname, SideDefs[(*sdlist)->objnum].middle, WAD_TEX_NAME);

/* test if there is a texture there */
if (texname[0] == '-')
  {
  Beep ();
  sprintf (errormessage, "No texture for sidedef #%d.", (*sdlist)->objnum);
  Notify (-1, -1, errormessage, 0);
  return;
  }

GetWallTextureSize (&texlength, &dummy, texname); /* clunky, but it works */

/* get initial offset to use (if required) */
if (type_off == 1)    /* source taken from InputObjectNumber */
   {
   int  x0;          /* left hand (x) window start     */
   int  y0;          /* top (y) window start           */
   int  key;         /* holds value returned by InputInteger */
   char prompt[80];  /* prompt for inital offset input */



   }

while (*sdlist)  /* main processing loop */
   {
   ldef = 0;
   sd1 = (*sdlist)->objnum;

   if (type_sd == 1) /* throw out all 2nd SideDefs untill a 1st is found */
   {
     while (LineDefs[ldef].sidedef1!=sd1 && ldef<=NumLineDefs)
       {
       ldef++;
       if (LineDefs[ldef].sidedef2 == sd1)
   {
   UnSelectObject (sdlist, (*sdlist)->objnum);
   sd1 = (*sdlist)->objnum;
   ldef = 0;
   if (! *sdlist)
      return;
   }
       }
     }

   if (type_sd == 2) /* throw out all 1st SideDefs untill a 2nd is found */
     {
     while (LineDefs[ldef].sidedef2!=sd1 && ldef<=NumLineDefs)
       {
       ldef++;
       if (LineDefs[ldef].sidedef1 == sd1)
   {
   UnSelectObject (sdlist, (*sdlist)->objnum);
   sd1 = (*sdlist)->objnum;
   ldef = 0;
   if (! *sdlist)
      return;
   }
       }
     }

   /* do we test for same textures for the sidedef in question?? */
   if (type_tex == 1)
     {
     
     if (strncmp (SideDefs[(*sdlist)->objnum].middle, texname,WAD_TEX_NAME))
       {
       Beep ();
       sprintf (errormessage, "No texture for sidedef #%d.", (*sdlist)->objnum);
       Notify (-1, -1, errormessage, 0);
       return;
       }
     }

   sd1 = (*sdlist)->objnum;
   ldef = 0;

   

   /* find out which linedef holds that sidedef */
   if (type_sd == 1)
     {
     while (LineDefs[ldef].sidedef1 != sd1 && ldef < NumLineDefs)
  ldef++;
     }
   else
     {
     while (LineDefs[ldef].sidedef2 != sd1 && ldef < NumLineDefs)
       ldef++;
     }

   vert1 = LineDefs[ldef].start;
   /* test for linedef highlight continuity */
   if (vert1 != vert2 && vert2 != -1)
     {
     Beep ();
     sprintf (errormessage, "Linedef #%d is not contiguous"
       " with the previous linedef, please reselect.", (*sdlist)->objnum);
     Notify (-1, -1, errormessage, 0);
     return;
     }
   /* is this the first time round here */
   if (vert1 != vert2)
     {
     if (type_off == 1)  /* do we have an initial offset ? */
       {
       SideDefs[sd1].xoff = useroffset;
       xoffset = useroffset;
       }
     else
  SideDefs[sd1].xoff = 0;
     }
   else     /* put new xoffset into the sidedef */
     SideDefs[sd1].xoff = xoffset;

   /* calculate length of linedef */
   vert2 = LineDefs[ldef].end;
   
   length = ComputeDist (Vertices[vert2].x - Vertices[vert1].x,
       Vertices[vert2].y - Vertices[vert1].y);

   xoffset += length;
   /* remove multiples of texlength from xoffset */
   xoffset = xoffset % texlength;
   /* move to next object in selected list */
   UnSelectObject (sdlist, (*sdlist)->objnum);
   }
MadeChanges = 1;

#endif
}


