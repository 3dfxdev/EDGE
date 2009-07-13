//------------------------------------------------------------------------
//  LINEDEFS
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
#include "r_misc.h"
#include "levels.h"
#include "selectn.h"
#include "r_images.h"

#include "e_linedef.h"
#include "m_bitvec.h"
#include "objects.h"
#include "w_structs.h"

#include <math.h>


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
  if (LineDefs[l].side_R == sidedef)
    {
    otherside = LineDefs[l].side_L;
    break;
    }
  if (LineDefs[l].side_L == sidedef)
    {
    otherside = LineDefs[l].side_R;
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

SideDefs[(*sdlist)->objnum].y_offset = 0;
UnSelectObject (sdlist, (*sdlist)->objnum);

/* adjust Y offset in all other SideDefs */
while (*sdlist)
  {
  h = GetTextureRefHeight ((*sdlist)->objnum);
  
  SideDefs[(*sdlist)->objnum].y_offset = (refh - h) % 128;
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
  while (*sdlist && LineDefs[ldef].side_R!=sd1 && ldef<=NumLineDefs)
    {
    ldef++;
    if (LineDefs[ldef].side_L == sd1)
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
  while (LineDefs[ldef].side_L!=sd1 && ldef<=NumLineDefs)
    {
    ldef++;
    if (LineDefs[ldef].side_R == sd1)
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
     while (LineDefs[ldef].side_R!=sd1 && ldef<=NumLineDefs)
       {
       ldef++;
       if (LineDefs[ldef].side_L == sd1)
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
     while (LineDefs[ldef].side_L!=sd1 && ldef<=NumLineDefs)
       {
       ldef++;
       if (LineDefs[ldef].side_R == sd1)
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
     while (LineDefs[ldef].side_R != sd1 && ldef < NumLineDefs)
  ldef++;
     }
   else
     {
     while (LineDefs[ldef].side_L != sd1 && ldef < NumLineDefs)
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
       SideDefs[sd1].x_offset = useroffset;
       xoffset = useroffset;
       }
     else
  SideDefs[sd1].x_offset = 0;
     }
   else     /* put new xoffset into the sidedef */
     SideDefs[sd1].x_offset = xoffset;

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


/*
 *  centre_of_linedefs
 *  Return the coordinates of the centre of a group of linedefs.
 */
void centre_of_linedefs (SelPtr list, int *x, int *y)
{
bitvec_c *vertices;
int nitems;
long x_sum;
long y_sum;
int n;

vertices = bv_vertices_of_linedefs (list);
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
 *  frob_linedefs_flags
 *  For all the linedefs in <list>, apply the operator <op>
 *  with the operand <operand> on the flags field.
 */
void frob_linedefs_flags (SelPtr list, int op, int operand)
{
  SelPtr cur;
  s16_t mask;

  if (op == YO_CLEAR || op == YO_SET || op == YO_TOGGLE)
    mask = 1 << operand;
  else
    mask = operand;

  for (cur = list; cur; cur = cur->next)
  {
    if (op == YO_CLEAR)
      LineDefs[cur->objnum].flags &= ~mask;
    else if (op == YO_SET)
      LineDefs[cur->objnum].flags |= mask;
    else if (op == YO_TOGGLE)
      LineDefs[cur->objnum].flags ^= mask;
    else
    {
      nf_bug ("frob_linedef_flags: op=%02X", op);
      return;
    }
  }
  MadeChanges = 1;
}



static void SliceLinedef (int linedefno, int times);


/*
   flip one or several LineDefs
*/

void FlipLineDefs ( SelPtr obj, bool swapvertices)
{
SelPtr cur;
int    tmp;


for (cur = obj; cur; cur = cur->next)
{
   if (swapvertices)
   {
      /* swap starting and ending Vertices */
      tmp = LineDefs[ cur->objnum].end;
      LineDefs[ cur->objnum].end = LineDefs[ cur->objnum].start;
      LineDefs[ cur->objnum].start = tmp;
   }
   /* swap first and second SideDefs */
   tmp = LineDefs[ cur->objnum].side_R;
   LineDefs[ cur->objnum].side_R = LineDefs[ cur->objnum].side_L;
   LineDefs[ cur->objnum].side_L = tmp;
}
MadeChanges = 1;
MadeMapChanges = 1;
}


/*
   split one or more LineDefs in two, adding new Vertices in the middle
*/

void SplitLineDefs ( SelPtr obj)
{
SelPtr cur;
int    vstart, vend;


for (cur = obj; cur; cur = cur->next)
  {
  vstart = LineDefs[cur->objnum].start;
  vend   = LineDefs[cur->objnum].end;
  SliceLinedef (cur->objnum, 1);
  Vertices[NumVertices-1].x = (Vertices[vstart].x + Vertices[vend].x) / 2;
  Vertices[NumVertices-1].y = (Vertices[vstart].y + Vertices[vend].y) / 2;
  }
MadeChanges = 1;
MadeMapChanges = 1;
}


/*
 *  MakeRectangularNook - Make a nook or boss in a wall
 *  
 *  Before :    After :
 *          ^-->
 *          |  |
 *  +----------------->     +------->  v------->
 *      1st sidedef             1st sidedef
 *
 *  The length of the sides of the nook is sidelen.
 *  This is true when convex is false. If convex is true, the nook
 *  is actually a bump when viewed from the 1st sidedef.
 */
void MakeRectangularNook (SelPtr obj, int width, int depth, int convex)
{
SelPtr cur;


for (cur = obj; cur; cur = cur->next)
  {
  int vstart, vend;
  int x0;
  int y0;
  int dx0, dx1, dx2;
  int dy0, dy1, dy2;
  int line_len;
  double real_width;
  double angle;

  vstart = LineDefs[cur->objnum].start;
  vend   = LineDefs[cur->objnum].end;
  x0     = Vertices[vstart].x;
  y0     = Vertices[vstart].y;
  dx0    = Vertices[vend].x - x0;
  dy0    = Vertices[vend].y - y0;

  /* First split the line 4 times */
  SliceLinedef (cur->objnum, 4);

  /* Then position the vertices */
  angle  = atan2 (dy0, dx0);
 
  /* If line to split is not longer than sidelen,
     force sidelen to 1/3 of length */
  line_len   = ComputeDist (dx0, dy0);
  real_width = line_len > width ? width : line_len / 3;
    
  dx2 = (int) (real_width * cos (angle));
  dy2 = (int) (real_width * sin (angle));

  dx1 = (dx0 - dx2) / 2;
  dy1 = (dy0 - dy2) / 2;

  {
  double normal = convex ? angle-HALFPI : angle+HALFPI;
  Vertices[NumVertices-1-3].x = x0 + dx1;
  Vertices[NumVertices-1-3].y = y0 + dy1;
  Vertices[NumVertices-1-2].x = x0 + dx1 + (int) (depth * cos (normal));
  Vertices[NumVertices-1-2].y = y0 + dy1 + (int) (depth * sin (normal));
  Vertices[NumVertices-1-1].x = x0 + dx1 + dx2 + (int) (depth * cos (normal));
  Vertices[NumVertices-1-1].y = y0 + dy1 + dy2 + (int) (depth * sin (normal));
  Vertices[NumVertices-1  ].x = x0 + dx1 + dx2;
  Vertices[NumVertices-1  ].y = y0 + dy1 + dy2;
  }

  MadeChanges = 1;
  MadeMapChanges = 1;
  }
}


/*
 *  SliceLinedef - Split a linedef several times
 *
 *  Splits linedef no. <linedefno> <times> times.
 *  Side-effects : creates <times> new vertices, <times> new
 *  linedefs and 0, <times> or 2*<times> new sidedefs.
 *  The new vertices are put at (0,0).
 *  See SplitLineDefs() and MakeRectangularNook() for example of use.
 */
static void SliceLinedef (int linedefno, int times)
{
int prev_ld_no;
for (prev_ld_no = linedefno; times > 0; times--, prev_ld_no = NumLineDefs-1)
  {
  int sd;

  InsertObject (OBJ_VERTICES, -1, 0, 0);
  InsertObject (OBJ_LINEDEFS, linedefno, 0, 0);
  LineDefs[NumLineDefs-1].start = NumVertices - 1;
  LineDefs[NumLineDefs-1].end   = LineDefs[prev_ld_no].end;
  LineDefs[prev_ld_no   ].end   = NumVertices - 1;
  
  sd = LineDefs[linedefno].side_R;
  if (sd >= 0)
    {
    InsertObject (OBJ_SIDEDEFS, sd, 0, 0);
    
    LineDefs[NumLineDefs-1].side_R = NumSideDefs - 1;
    }
  sd = LineDefs[linedefno].side_L;
  if (sd >= 0)
    {
    InsertObject (OBJ_SIDEDEFS, sd, 0, 0);
    
    LineDefs[NumLineDefs-1].side_L = NumSideDefs - 1;
    }
  }
}


/*
 *  SetLinedefLength
 *  Move either vertex to set length of linedef to desired value
 */
void SetLinedefLength (SelPtr obj, int length, int move_2nd_vertex)
{
SelPtr cur;


for (cur = obj; cur; cur = cur->next)
  {
  VPtr vertex1 = Vertices + LineDefs[cur->objnum].start;
  VPtr vertex2 = Vertices + LineDefs[cur->objnum].end;
  double angle = atan2 (vertex2->y - vertex1->y, vertex2->x - vertex1->x);
  int dx       = (int) (length * cos (angle));
  int dy       = (int) (length * sin (angle));

  if (move_2nd_vertex)
    {
    vertex2->x = vertex1->x + dx;
    vertex2->y = vertex1->y + dy;
    }
  else
    {
    vertex1->x = vertex2->x - dx;
    vertex1->y = vertex2->y - dy;
    }

  MadeChanges = 1;
  MadeMapChanges = 1;
  }
}



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
      if (side1 && is_sidedef (LineDefs[n].side_R))
         sd_used_in.set (LineDefs[n].side_R);
      if (side2 && is_sidedef (LineDefs[n].side_L))
         sd_used_in.set (LineDefs[n].side_L);
      }
   else
      {
      if (is_sidedef (LineDefs[n].side_R))
   sd_used_out.set (LineDefs[n].side_R);
      if (is_sidedef (LineDefs[n].side_L))
   sd_used_out.set (LineDefs[n].side_L);
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
         if (side1 && LineDefs[cur->objnum].side_R == n)
            LineDefs[cur->objnum].side_R = NumSideDefs - 1;
         if (side2 && LineDefs[cur->objnum].side_L == n)
            LineDefs[cur->objnum].side_L = NumSideDefs - 1;
         }
      }
   }
}



/*
 *  bv_vertices_of_linedefs
 *  Return a bit vector of all vertices used by the linedefs.
 *  It's up to the caller to delete the bit vector after use.
 */
bitvec_c *bv_vertices_of_linedefs (bitvec_c *linedefs)
{
bitvec_c *vertices;
int n;

vertices = new bitvec_c (NumVertices);
for (n = 0; n < NumLineDefs; n++)
   if (linedefs->get (n))
      {
      vertices->set (LineDefs[n].start);
      vertices->set (LineDefs[n].end);
      }
return vertices;
}


/*
 *  bv_vertices_of_linedefs
 *  Return a bit vector of all vertices used by the linedefs.
 *  It's up to the caller to delete the bit vector after use.
 */
bitvec_c *bv_vertices_of_linedefs (SelPtr list)
{
bitvec_c *vertices;
SelPtr cur;

vertices = new bitvec_c (NumVertices);
for (cur = list; cur; cur = cur->next)
   {
   vertices->set (LineDefs[cur->objnum].start);
   vertices->set (LineDefs[cur->objnum].end);
   }
return vertices;
}


/*
 *  list_vertices_of_linedefs
 *  Return a list of all vertices used by the linedefs
 *  It's up to the caller to delete the list after use.
 */
SelPtr list_vertices_of_linedefs (SelPtr list)
{
bitvec_c *vertices_bitvec;
SelPtr vertices_list = 0;

vertices_bitvec = bv_vertices_of_linedefs (list);

for (int n = 0; n < vertices_bitvec->size(); n++)
   {
   if (vertices_bitvec->get (n))
      SelectObject (&vertices_list, n);
   }
delete vertices_bitvec;
return vertices_list;
}


/* The Superimposed_ld class is used to find all the linedefs that are
   superimposed with a particular reference linedef. Call the set()
   method to specify the reference linedef. Each call to the get()
   method returns the number of the next superimposed linedef, or -1
   when there are no more superimposed linedefs.

   Two linedefs are superimposed iff their ends have the same map
   coordinates, regardless of whether the vertex numbers are the same,
   and irrespective of start/end vertex distinctions. */

class Superimposed_ld
{
  public:
             Superimposed_ld ();
    int      set             (obj_no_t);
    obj_no_t get             ();
    void     rewind          ();

  private:
    obj_no_t refldno;     // Reference linedef
    obj_no_t ldno;      // get() will start from there
};


inline Superimposed_ld::Superimposed_ld ()
{
  refldno = -1;
  rewind ();
}


/*
 *  Superimposed_ld::set - set the reference linedef
 *
 *  If the argument is not a valid linedef number, does nothing and
 *  returns a non-zero value. Otherwise, set the linedef number,
 *  calls rewind() and returns a zero value.
 */
inline int Superimposed_ld::set (obj_no_t ldno)
{
  if (! is_linedef (ldno))    // Paranoia
    return 1;

  refldno = ldno;
  rewind ();
  return 0;
}


/*
 *  Superimposed_ld::get - return the next superimposed linedef
 *
 *  Returns the number of the next superimposed linedef, or -1 if
 *  there's none. If the reference linedef was not specified, or is
 *  invalid (possibly as a result of changes in the level), returns
 *  -1.
 *
 *  Linedefs that have invalid start/end vertices are silently
 *  skipped.
 */
inline obj_no_t Superimposed_ld::get ()
{
  if (refldno == -1)
    return -1;

  /* These variables are there to speed things up a bit by avoiding
     repetitive table lookups. Everything is re-computed each time as
     LineDefs could very well be realloc'd while we were out. */

  if (! is_linedef (refldno))
    return -1;
  const struct LineDef *const pmax = LineDefs + NumLineDefs;
  const struct LineDef *const pref = LineDefs + refldno;

  const wad_vn_t refv0 = pref->start;
  const wad_vn_t refv1 = pref->end;
  if (! is_vertex (refv0) || ! is_vertex (refv1))   // Paranoia
    return -1;

  const wad_coord_t refx0 = Vertices[refv0].x;
  const wad_coord_t refy0 = Vertices[refv0].y;
  const wad_coord_t refx1 = Vertices[refv1].x;
  const wad_coord_t refy1 = Vertices[refv1].y;

  for (const struct LineDef *p = LineDefs + ldno; ldno < NumLineDefs;
      p++, ldno++)
  {
    if (! is_vertex (p->start) || ! is_vertex (p->end))   // Paranoia
      continue;
    obj_no_t x0 = Vertices[p->start].x;
    obj_no_t y0 = Vertices[p->start].y;
    obj_no_t x1 = Vertices[p->end].x;
    obj_no_t y1 = Vertices[p->end].y;
    if ( x0 == refx0 && y0 == refy0 && x1 == refx1 && y1 == refy1
      || x0 == refx1 && y0 == refy1 && x1 == refx0 && y1 == refy0)
    {
      if (ldno == refldno)
  continue;
      return ldno++;
    }
  }

  return -1;
}


/*
 *  Superimposed_ld::rewind - rewind the counter
 *
 *  After calling this method, the next call to get() will start
 *  from the first linedef.
 */
inline void Superimposed_ld::rewind ()
{
  ldno = 0;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
