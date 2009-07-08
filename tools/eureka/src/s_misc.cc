/*
 *  s_misc.cc
 *  Misc. operations on sectors
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
#include <math.h>

/// #include "entry.h"
#include "gfx.h"
#include "levels.h"
#include "objid.h"
#include "selectn.h"
#include "objects.h"
#include "dialog.h"


/*
   Distribute sector floor heights
*/

void DistributeSectorFloors (SelPtr obj)
{
SelPtr cur;
int    n, num, floor1h, floor2h;


num = 0;
for (cur = obj; cur->next; cur = cur->next)
   num++;

floor1h = Sectors[obj->objnum].floorh;
floor2h = Sectors[cur->objnum].floorh;

n = 0;
for (cur = obj; cur; cur = cur->next)
   {
   Sectors[cur->objnum].floorh = floor1h + n * (floor2h - floor1h) / num;
   n++;
   }
MadeChanges = 1;
}



/*
   Distribute sector ceiling heights
*/

void DistributeSectorCeilings (SelPtr obj)
{
SelPtr cur;
int    n, num, ceil1h, ceil2h;


num = 0;
for (cur = obj; cur->next; cur = cur->next)
   num++;

ceil1h = Sectors[obj->objnum].ceilh;
ceil2h = Sectors[cur->objnum].ceilh;

n = 0;
for (cur = obj; cur; cur = cur->next)
   {
   Sectors[cur->objnum].ceilh = ceil1h + n * (ceil2h - ceil1h) / num;
   n++;
   }
MadeChanges = 1;
}


/*
   Raise or lower sectors
*/

void RaiseOrLowerSectors (SelPtr obj)
{
#if 0 // TODO: RaiseOrLowerSectors
SelPtr cur;
int  x0;          // left hand (x) window start
int  y0;          // top (y) window start
int  key;         // holds value returned by InputInteger
int  delta = 0;   // user input for delta


x0 = (ScrMaxX - 25 - 44 * FONTW) / 2;
y0 = (ScrMaxY - 7 * FONTH) / 2;
DrawScreenBox3D (x0, y0, x0 + 25 + 44 * FONTW, y0 + 7 * FONTH);
set_colour (WHITE);
DrawScreenText (x0+10, y0 + FONTH,     "Enter number of units to raise the ceilings");
DrawScreenText (x0+10, y0 + 2 * FONTH, "and floors of selected sectors by.");
DrawScreenText (x0+10, y0 + 3 * FONTH, "A negative number lowers them.");
while (1)
  {
  key = InputInteger (x0+10, y0 + 5 * FONTH, &delta, -32768, 32767);
  if (key == YK_RETURN || key == YK_ESC)
    break;
  Beep ();
  }
if (key == YK_ESC)
  return;

for (cur = obj; cur != NULL; cur = cur->next)
  {
  Sectors[cur->objnum].ceilh += delta;
  Sectors[cur->objnum].floorh += delta;
  }
MadeChanges = 1;
#endif
}


/*
   Brighten or darken sectors
*/

void BrightenOrDarkenSectors (SelPtr obj)
{
#if 0  // TODO: BrightenOrDarkenSectors
SelPtr cur;
int  x0;          // left hand (x) window start
int  y0;          // top (y) window start
int  key;         // holds value returned by InputInteger
int  delta = 0;   // user input for delta


x0 = (ScrMaxX - 25 - 44 * FONTW) / 2;
y0 = (ScrMaxY - 7 * FONTH) / 2;
DrawScreenBox3D (x0, y0, x0 + 25 + 44 * FONTW, y0 + 7 * FONTH);
set_colour (WHITE);
DrawScreenText (x0+10, y0 + FONTH,     "Enter number of units to brighten");
DrawScreenText (x0+10, y0 + 2 * FONTH, "the selected sectors by.");
DrawScreenText (x0+10, y0 + 3 * FONTH, "A negative number darkens them.");
while (1)
  {
  key = InputInteger (x0+10, y0 + 5 * FONTH, &delta, -255, 255);
  if (key == YK_RETURN || key == YK_ESC)
    break;
  Beep ();
  }
if (key == YK_ESC)
  return;

for (cur = obj; cur != NULL; cur = cur->next)
  {
  int light;
  light = Sectors[cur->objnum].light + delta;
  light = MAX(light, 0);
  light = MIN(light, 255);
  Sectors[cur->objnum].light = light;
  }
MadeChanges = 1;
#endif
}


static int find_linedef_for_area (int x, int y, int& side)
{
   int n, m, curx;
   int best_match = -1;

   curx = 32767;  // Oh yes, one more hard-coded constant!

   for (n = 0; n < NumLineDefs; n++)
      if ((Vertices[LineDefs[n].start].y > y)
       != (Vertices[LineDefs[n].end].y > y))
      {
         int lx0 = Vertices[LineDefs[n].start].x;
         int ly0 = Vertices[LineDefs[n].start].y;
         int lx1 = Vertices[LineDefs[n].end].x;
         int ly1 = Vertices[LineDefs[n].end].y;
         m = lx0 + (int) ((long) (y - ly0) * (long) (lx1 - lx0)
                                           / (long) (ly1 - ly0));
         if (m >= x && m < curx)
         {
            curx = m;
            best_match = n;
         }
      }

   /* now look if this linedef has a sidedef bound to one sector */
   if (best_match < 0)
      return OBJ_NO_NONE;

   if (Vertices[LineDefs[best_match].start].y
     > Vertices[LineDefs[best_match].end].y)
      side = 1;
   else
      side = 2;

   return best_match;
}

/*
   compute the angle between lines AB and BC, going anticlockwise.
   result is in degrees 0 - 359.  A, B and C are vertex indices.
   -AJA- 2001-05-09
 */
#define DEBUG_ANGLE  0

static double angle_between_linedefs (int A, int B, int C)
{
   int a_dx = Vertices[B].x - Vertices[A].x;
   int a_dy = Vertices[B].y - Vertices[A].y;
   
   int c_dx = Vertices[B].x - Vertices[C].x;
   int c_dy = Vertices[B].y - Vertices[C].y;
   
   double AB_angle = (a_dx == 0) ? (a_dy >= 0 ? 90 : -90) :
      atan2 (a_dy, a_dx) * 180 / M_PI;

   double CB_angle = (c_dx == 0) ? (c_dy >= 0 ? 90 : -90) :
      atan2 (c_dy, c_dx) * 180 / M_PI;

   double result = CB_angle - AB_angle;

   if (result >= 360)
      result -= 360;
   
   while (result < 0)
      result += 360;

#if (DEBUG_ANGLE)
   fprintf(stderr, "ANGLE %1.6f  (%d,%d) -> (%d,%d) -> (%d,%d)\n",
      result, Vertices[A].x, Vertices[A].y,
      Vertices[B].x, Vertices[B].y, Vertices[C].x, Vertices[C].y);
#endif

   return result;
}

/*
   follows the path clockwise from the given start line, adding each
   line into the appropriate set.  If the path is not closed, zero is
   returned.  
   
   -AJA- 2001-05-09
 */
#define DEBUG_PATH  0

static int select_sides_in_closed_path (bitvec_c& ld_side1,
    bitvec_c& ld_side2, int line, int side)
{
   int cur_vert, prev_vert, final_vert;
   
   if (side == 1)
   {
      ld_side1.set (line);
      cur_vert = LineDefs[line].end;
      prev_vert = final_vert = LineDefs[line].start;
   }
   else
   {
      ld_side2.set (line);
      cur_vert = LineDefs[line].start;
      prev_vert = final_vert = LineDefs[line].end;
   }

#if (DEBUG_PATH)
      fprintf(stderr, "PATH: line %d  side %d  cur %d  final %d\n",
         line, side, cur_vert, final_vert);
#endif

   while (cur_vert != final_vert)
   {
      int next_line = OBJ_NO_NONE;
      int next_vert = OBJ_NO_NONE;
      int next_side;
      double best_angle = 999;

      // Look for the next linedef in the path.  It's the linedef that
      // uses the current vertex and is not the current one.

      for (int n = 0; n < NumLineDefs; n++)
      {
         if (n == line)
            continue;

         int other_vert;
         int which_side;

         if (LineDefs[n].start == cur_vert)
         {
            other_vert = LineDefs[n].end;
            which_side = 1;
         }
         else if (LineDefs[n].end == cur_vert)
         {
            other_vert = LineDefs[n].start;
            which_side = 2;
         }
         else
            continue;

         // found adjoining linedef
         
         double angle = angle_between_linedefs (prev_vert, cur_vert,
            other_vert);
         
         if (! is_obj (next_line) || angle < best_angle)
         {
            next_line = n;
            next_vert = other_vert;
            next_side = which_side;
            
            best_angle = angle;
         }

         // Continue the search
      }
 
      line = next_line;
      side = next_side;

#if (DEBUG_PATH)
      fprintf(stderr, "PATH NEXT: line %d  side %d  vert %d  angle %1.6f\n",
         line, side, next_vert, best_angle);
#endif

      // None ?  Path cannot be closed
      if (! is_obj (line))
         return 0;

      // Line already seen ?  Under normal circumstances this won't
      // happen, but it _can_ happen and indicates a non-closed
      // structure
      if (ld_side1.get (line) || ld_side2.get (line))
         return 0;

      if (side == 1)
         ld_side1.set (line);
      else
         ld_side2.set (line);
       
      prev_vert = cur_vert;
      cur_vert = next_vert;
   }

#if (DEBUG_PATH)
      fprintf(stderr, "PATH CLOSED !\n");
#endif

   return 1;
}

/*
   update the side on a single linedef, using the given sector
   reference.  Will create a new sidedef if necessary.
 */
static void super_set_sector_on_side (int line, wad_sdn_t& side,
   wad_sdn_t& other, int side_no, int sector)
{
   if (is_obj (side) && SideDefs[side].sector == sector)
   {
      // there was no change.
      return;
   }
   
   int must_flip = 0;

   if (! is_obj (side))
   {
      // if we're adding a sidedef to a line that has no sides, and
      // the sidedef would be the 2nd one, then flip the linedef.
      // Thus we don't end up with invalid lines -- i.e. ones with a
      // left side but no right side.

      if (! is_obj (other) && side_no == 2)
         must_flip = 1;

      InsertObject (OBJ_SIDEDEFS, OBJ_NO_NONE, 0, 0);
      side = NumSideDefs - 1;

      // if we're adding a second side to the linedef, clear out some
      // of the properties that aren't needed anymore: middle texture,
      // two-sided flag, and impassible flag.
      
      if (is_obj (other))
      {
         strncpy (SideDefs[side].middle,  "-", WAD_TEX_NAME);
         strncpy (SideDefs[other].middle, "-", WAD_TEX_NAME);

         LineDefs[line].flags |=  4;  // Set the 2S bit
         LineDefs[line].flags &= ~1;  // Clear the Im bit
      }
   }

   SideDefs[side].sector = sector;
   
   if (must_flip)
   {
      int temp = LineDefs[line].start;
      LineDefs[line].start = LineDefs[line].end;
      LineDefs[line].end = temp;

      temp = side; 
      side = other; 
      other = temp;
   }

   MadeChanges = 1;
   MadeMapChanges = 1;
}

static int super_find_sector_model (bitvec_c& ld_side1,
    bitvec_c& ld_side2)
{
   for (int line=0; line < NumLineDefs; line++)
   {
      int side1 = LineDefs[line].sidedef1;
      int side2 = LineDefs[line].sidedef2;

      if (ld_side1.get (line))
         if (is_obj (side2))
            return SideDefs[side2].sector;

      if (ld_side2.get (line))
         if (is_obj (side1))
            return SideDefs[side1].sector;
   }

   return OBJ_NO_NONE;
}


/*
   Change the closed sector at the pointer

   "sector" here really means a bunch of sidedefs that all face
   inward to the current area under the mouse cursor.  Two basic
   operations: (a) set the sidedef sector references to a completely
   new sector, or (b) set them to an existing sector.  This is
   controlled by the `new_sec' parameter.

   -AJA- 2001-05-08
 */

void SuperSectorSelector (int map_x, int map_y, int new_sec)
{
   int line, side;
   char msg_buf[200];

   line = find_linedef_for_area (map_x, map_y, side);

   if (! is_obj (line))
   {
      Beep ();
      sprintf (msg_buf, "Chosen area is not closed");
      Notify (-1, -1, msg_buf, NULL);
      return;
   }

   bitvec_c ld_side1 (NumLineDefs);
   bitvec_c ld_side2 (NumLineDefs);
   
   int closed = select_sides_in_closed_path (ld_side1, ld_side2,
      line, side);

   if (! closed)
   {
      Beep ();
      sprintf (msg_buf, "Area chosen is not closed");
      Notify (-1, -1, msg_buf, NULL);
      return;
   }

   // -AJA- FIXME: look for "islands", closed linedef paths that lie
   // completely inside the area, i.e. not connected to the main path.
   // Example: the two pillars at the start of MAP01 of DOOM 2.  See
   // GetOppositeSector() and the end of SplitSector() for a possible
   // algorithm.
   
   if (! is_obj (new_sec))
   {
      int model = super_find_sector_model (ld_side1, ld_side2);
      InsertObject (OBJ_SECTORS, model, 0, 0);
      new_sec = NumSectors - 1;
   }
   
   for (line=0; line < NumLineDefs; line++)
   {
      if (ld_side1.get (line))
         super_set_sector_on_side (line, LineDefs[line].sidedef1,
            LineDefs[line].sidedef2, 1, new_sec);

      else if (ld_side2.get (line))
         super_set_sector_on_side (line, LineDefs[line].sidedef2,
            LineDefs[line].sidedef1, 2, new_sec);
   }
}

/* end of file */
