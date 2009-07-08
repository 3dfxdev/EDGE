/*
 *  x_hover.cc
 *  AYM 2000-11-08
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
#include <vector>
#include <algorithm>

#include "grid2.h"
#include "levels.h"
#include "objid.h"
#include "x_hover.h"


class Close_obj
{
  public :
    Close_obj () { nil (); }
    void nil ()
    {
      obj.nil ();
      distance = ULONG_MAX;
      radius   = INT_MAX;
      inside   = false;
    }
    bool operator== (const Close_obj& other) const
    {
      if (inside == other.inside
   && radius == other.radius
   && distance == other.distance)
       return true;
      return false;
    }
    bool operator< (const Close_obj& other) const
    {
      if (inside && ! other.inside)
  return true;
      if (! inside && other.inside)
  return false;

      // Small objects should "mask" large objects
      if (radius < other.radius)
  return true;
      if (radius > other.radius)
  return false;

      if (distance < other.distance)
  return true;
      if (distance > other.distance)
  return false;

      return radius < other.radius;
    }
    bool operator<= (const Close_obj& other) const
    {
      return *this == other || *this < other;
    }
    Objid  obj;
    double distance;
    bool   inside;
    int    radius;
};


static const Close_obj& get_cur_linedef (int x, int y);
static const Close_obj& get_cur_sector  (int x, int y);
static const Close_obj& get_cur_thing   (int x, int y);
static const Close_obj& get_cur_vertex  (int x, int y);
static const Close_obj& get_cur_rts     (int x, int y);


/*
 *  GetCurObject - determine which object is under the pointer
 * 
 *  Set <o> to point to the object under the pointer (map
 *  coordinates (<x>, <y>). If several objects are close
 *  enough to the pointer, the smallest object is chosen.
 */
void GetCurObject (Objid& o, int objtype, int x, int y)
{
switch (objtype)
   {
   case OBJ_THINGS:
     {
       o = get_cur_thing (x, y).obj;
       return;
     }

   case OBJ_VERTICES:
     {
       o = get_cur_vertex (x, y).obj;
       return;
     }

   case OBJ_LINEDEFS:
     {
       o = get_cur_linedef (x, y).obj;
       return;
     }

   case OBJ_SECTORS:
     {
       o = get_cur_sector (x, y).obj;
       return;
     }

   case OBJ_RSCRIPT:
     {
       o = get_cur_rts (x, y).obj;
       return;
     }

   default:
     nf_bug ("GetCurObject: objtype %d", (int) objtype);
     o.nil ();
     return;
   }
}


/*
 *  get_cur_linedef - determine which linedef is under the pointer
 */
static const Close_obj& get_cur_linedef (int x, int y)
{
  static Close_obj object;
  const int screenslack = 15;     // Slack in pixels
  double mapslack = fabs (screenslack / grid.Scale);  // Slack in map units
  int xmin = (int) (x - mapslack + 0.5);
  int xmax = (int) (x + mapslack + 0.5);
  int ymin = (int) (y - mapslack + 0.5);
  int ymax = (int) (y + mapslack + 0.5);
  
  object.nil ();
  for (int n = 0; n < NumLineDefs; n++)
  {
    int x0 = Vertices[LineDefs[n].start].x;
    int y0 = Vertices[LineDefs[n].start].y;
    int x1 = Vertices[LineDefs[n].end].x;
    int y1 = Vertices[LineDefs[n].end].y;
    int dx;
    int dy;

    /* Skip all lines of which all points are more than <mapslack>
       units away from (x,y). In a typical level, this test will
       filter out all the linedefs but a handful. */
    if (x0 < xmin && x1 < xmin
     || x0 > xmax && x1 > xmax
     || y0 < ymin && y1 < ymin
     || y0 > ymax && y1 > ymax)
      continue;

    /* This is where it gets ugly. We're trying to calculate the
       distance between the pointer (x,y) and the linedef. Doing that
       rigorously involves doing an orthogonal projection. I use
       something simpler (for me).

       If the pointer is not within the ends of the linedef, I calculate
       the distance between the pointer and the closest end.

       If the pointer is within the ends of the linedef, I calculate
       what the ordinate would be for a point of the linedef that had
       the same abscissa as the pointer. Then I calculate the difference
       between that and the actual ordinate of the pointer and I call
       that the distance. It's a gross approximation but, in practice,
       it gives quite satisfactory results. Of course, for lines where
       dy > dx, the abscissa is y and the ordinate is x.
       -- AYM 1998-06-29 */
    dx = x1 - x0;
    dy = y1 - y0;
    double dist = ULONG_MAX;
    // The linedef is rather horizontal
    if (abs (dx) > abs (dy))
    {
      /* Are we to the left of the leftmost end or to the right of the
   rightmost end or between the two ? */
      if (x < (dx > 0 ? x0 : x1))
  dist = hypot (x - (dx > 0 ? x0 : x1),
          y - (dx > 0 ? y0 : y1));
      else if (x > (dx > 0 ? x1 : x0))
  dist = hypot (x - (dx > 0 ? x1 : x0),
          y - (dx > 0 ? y1 : y0));
      else
  dist = fabs (y0 + ((double) dy)/dx * (x - x0) - y);
    }
    // The linedef is rather vertical
    else
    {
      /* Are we above the high end or below the low end or in between ? */
      if (y < (dy > 0 ? y0 : y1))
  dist = hypot (x - (dy > 0 ? x0 : x1),
          y - (dy > 0 ? y0 : y1));
      else if (y > (dy > 0 ? y1 : y0))
  dist = hypot (x - (dy > 0 ? x1 : x0),
          y - (dy > 0 ? y1 : y0));
      else
  dist = fabs (x0 + ((double) dx)/dy * (y - y0) - x);
    }
    if (dist <= object.distance  /* "<=" because if there are superimposed
            linedefs, we want to return the
            highest-numbered one. */
       && dist <= mapslack)
    {
      object.obj.type = OBJ_LINEDEFS;
      object.obj.num  = n;
      object.distance = dist;
      object.radius   = (int) (hypot (dx, dy) / 2);
      object.inside   = dist < 2 * grid.Scale;
    }
  }
  return object;
}


/*
 *  get_cur_sector - determine which sector is under the pointer
 */
static const Close_obj& get_cur_sector (int x, int y)
{
  /* hack, hack...  I look for the first LineDef crossing
     an horizontal half-line drawn from the cursor */

  /* RQ & BW have been very smart here. Their method works remarkably
     well for normal sectors. However, self-referencing sectors are
     frequently unclosed. If your SRS has only horizontal linedefs, this
     method fails miserably. I suppose that the solution would be to look
     for intersections in BOTH directions and pick the closest. Remind me
     to look into it one of these days :-). -- AYM 1998-06-29 */

  /* Initializing curx to 32767 instead of MapMaxX + 1. Fixes the old
     DEU bug where sometimes you couldn't select a newly created sector
     to the west of the level until you saved. (MapMaxX got out of date
     and SaveLevelData() refreshed it.) -- AYM 1999-03-18 */
  static Close_obj object;
  int best_match = -1;
  int curx = 32767;  // Oh yes, one more hard-coded constant!
  
  for (int n = 0; n < NumLineDefs; n++)
    if ((Vertices[LineDefs[n].start].y > y)
     != (Vertices[LineDefs[n].end].y > y))
    {
      int lx0 = Vertices[LineDefs[n].start].x;
      int ly0 = Vertices[LineDefs[n].start].y;
      int lx1 = Vertices[LineDefs[n].end].x;
      int ly1 = Vertices[LineDefs[n].end].y;
      int m = lx0 + (int) ((long) (y - ly0) * (long) (lx1 - lx0)
              / (long) (ly1 - ly0));
      if (m >= x && m < curx)
      {
  curx = m;
  best_match = n;
      }
    }

  // Now look if this linedef has a sidedef bound to one sector
  object.nil ();
  if (best_match >= 0)
  {
    if (Vertices[LineDefs[best_match].start].y
      > Vertices[LineDefs[best_match].end].y)
      best_match = LineDefs[best_match].sidedef1;
    else
      best_match = LineDefs[best_match].sidedef2;
    if (best_match >= 0)
    {
      
      object.obj.type = OBJ_SECTORS;
      object.obj.num  = SideDefs[best_match].sector;
      object.distance = 0;  // Not meaningful for sectors
      object.radius   = 1;  // Not meaningful for sectors
      object.inside   = true; // Not meaningful for sectors
    }
    else
      ;
  }
  else
    ;
  return object;
}


/*
 *  get_cur_thing - determine which thing is under the pointer
 */
static const Close_obj& get_cur_thing (int x, int y)
{
  static Close_obj closest;
  const int screenslack = 15;     // Slack in pixels
  double mapslack = fabs (screenslack / grid.Scale);  // Slack in map units
  int max_radius = (int) (get_max_thing_radius () + mapslack);
  int xmin = x - max_radius;
  int xmax = x + max_radius;
  int ymin = y - max_radius;
  int ymax = y + max_radius;

  
  closest.nil ();
  for (int n = 0; n < NumThings; n++)
  {
    // Filter out things that are farther than <max_radius> units away.
    if (Things[n].x < xmin
     || Things[n].x > xmax
     || Things[n].y < ymin
     || Things[n].y > ymax)
       continue;

    // So how far is that thing exactly ?
#ifdef ROUND_THINGS
    double dist = hypot (x - Things[n].x, y - Things[n].y);
    if (dist < closest.distance
     && dist <= get_thing_radius (Things[n].type) + mapslack)
    {
      closest.obj.type = OBJ_THINGS;
      closest.obj.num  = n;
      closest.distance = dist;
      closest.radius   = get_thing_radius (Things[n].type);
      closest.inside   = dist < closest.radius;
    }
#else
    {
      int thing_radius = (int) (get_thing_radius (Things[n].type) + mapslack);
      if (x > Things[n].x - thing_radius
       && x < Things[n].x + thing_radius
       && y > Things[n].y - thing_radius
       && y < Things[n].y + thing_radius)
      {
  Close_obj current;
  current.obj.type = OBJ_THINGS;
  current.obj.num  = n;
  current.distance = hypot (x - Things[n].x, y - Things[n].y);
  current.radius   = get_thing_radius (Things[n].type);
  current.inside   = x > Things[n].x - current.radius
      && x < Things[n].x + current.radius
      && y > Things[n].y - current.radius
      && y < Things[n].y + current.radius;
       if (current <= closest)  /* "<=" because if there are superimposed
           things, we want to return the
           highest-numbered one. */
    closest = current;
      }
    }
#endif
  }
  return closest;
}


/*
 *  get_cur_vertex - determine which vertex is under the pointer
 */
static const Close_obj& get_cur_vertex (int x, int y)
{
  static Close_obj object;
  const int screenradius = vertex_radius (grid.Scale);  // Radius in pixels
  const int screenslack  = screenradius + 15;   // Slack in pixels
  double    mapslack     = fabs (screenslack / grid.Scale); // Slack in map units
  const int mapradius    = (int) (screenradius / grid.Scale);// Radius in map units
  int xmin = (int) (x - mapslack + 0.5);
  int xmax = (int) (x + mapslack + 0.5);
  int ymin = (int) (y - mapslack + 0.5);
  int ymax = (int) (y + mapslack + 0.5);

  
  object.nil ();
  for (int n = 0; n < NumVertices; n++)
  {
    /* Filter out objects that are farther than <radius> units away. */
    if (Vertices[n].x < xmin
     || Vertices[n].x > xmax
     || Vertices[n].y < ymin
     || Vertices[n].y > ymax)
       continue;
    double dist = hypot (x - Vertices[n].x, y - Vertices[n].y);
    if (dist <= object.distance  /* "<=" because if there are superimposed
           vertices, we want to return the
           highest-numbered one. */
       && dist <= mapslack)
    {
      object.obj.type = OBJ_VERTICES;
      object.obj.num  = n;
      object.distance = dist;
      object.radius   = mapradius;
      object.inside   = x > Vertices[n].x - mapradius
         && x < Vertices[n].x + mapradius
         && y > Vertices[n].y - mapradius
         && y < Vertices[n].y + mapradius;
    }
  }
  return object;
}


/*
 *  get_cur_rts - determine which RTS trigger is under the pointer
 */
static const Close_obj& get_cur_rts (int x, int y)
{
  static Close_obj object;
  const int screenradius = vertex_radius (grid.Scale);  // Radius in pixels
  const int screenslack  = screenradius + 15;   // Slack in pixels
  double    mapslack     = fabs (screenslack / grid.Scale); // Slack in map units
  const int mapradius    = (int) (screenradius / grid.Scale);// Radius in map units
  int xmin = (int) (x - mapslack + 0.5);
  int xmax = (int) (x + mapslack + 0.5);
  int ymin = (int) (y - mapslack + 0.5);
  int ymax = (int) (y + mapslack + 0.5);

  object.nil ();

  // TODO: get_cur_rts

  return object;
}

