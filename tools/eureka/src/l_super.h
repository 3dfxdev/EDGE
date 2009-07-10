/*
 *  l_super.h - Superimposed_ld class
 *  AYM 2003-12-02
 */

/*
This file is copyright André Majorel 2003.

This program is free software; you can redistribute it and/or modify it under
the terms of version 2 of the GNU General Public License as published by the
Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA 02111-1307, USA.
*/


#ifndef YH_L_SUPER  /* DO NOT INSERT ANYTHING BEFORE THIS LINE */
#define YH_L_SUPER

#include "levels.h"


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


#endif  /* DO NOT ADD ANYTHING AFTER THIS LINE */
