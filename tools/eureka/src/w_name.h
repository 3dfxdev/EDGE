/*
 *  wadname.h
 *  AYM 2000-04-13
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


#ifndef YH_WADNAME
#define YH_WADNAME


#include <memory.h>
#include <ctype.h>

#include "w_structs.h"


/*
 *  Wad_name - the name of a wad directory entry
 *  
 *  This class is used to store a wad directory entry name.
 *  It provides the following guarantees :
 *
 *  - there is minimal memory overhead,
 *
 *  - the name is stored in CAPITALS and padded to the
 *    maximum length with NULs so that the comparison of two
 *    names can be done with memcmp() instead of
 *    y_strnicmp(). This is very important for performance
 *    reasons.
 */
struct Wad_name
{
  inline Wad_name ();
  inline Wad_name (const Wad_name& source);
  inline Wad_name (const char *source);
  inline Wad_name& operator=  (const char *source);
  inline bool      operator== (const Wad_name& ref) const;
  inline bool      operator== (const char *ref) const;
  inline bool      less       (const Wad_name& other) const;
  inline bool      has_prefix (const Wad_name& prefix) const;
  wad_name_t name;
};


/*
 *  default ctor
 */
inline Wad_name::Wad_name ()
{
  memset (name, '\0', sizeof name);
}


/*
 *  initialize from a trusted source
 *
 *  The source *must* be already upper-cased and padded with
 *  NULs.
 */

inline Wad_name::Wad_name (const Wad_name& source)
{
  memcpy (name, source.name, sizeof name);
}


/*
 *  initialize from an untrusted source
 */
inline Wad_name::Wad_name (const char *source)
{
  char *p;
  char *const pmax = name + sizeof name;
  for (p = name; *source != '\0' && p < pmax; p++)
    *p = toupper (*source++);
  // Pad with NULs to the end
  for (; p < pmax; p++)
    *p = '\0';
}


/*
 *  initialize from an untrusted source
 */
inline Wad_name& Wad_name::operator= (const char *source)
{
  char *p;
  char *const pmax = name + sizeof name;
  for (p = name; *source != '\0' && p < pmax; p++)
    *p = toupper (*source++);
  // Pad with NULs to the end
  for (; p < pmax; p++)
    *p = '\0';
  return *this;
}


/*
 *  compare two Wad_name objects for equality
 */
inline bool Wad_name::operator== (const Wad_name& ref) const
{
  return this == &ref || ! memcmp (name, ref.name, sizeof name);
}


/*
 *  compare for equality with an untrusted char[8]
 */
inline bool Wad_name::operator== (const char *ref) const
{
  return ! y_strnicmp (name, ref, sizeof name);
}


/*
 *  less - less operator for map
 *
 *  This is the operator suitable for use in a map. See
 *  Lump_dir for an example.
 *
 *  Return true iff <this> is "smaller" (lexicographically
 *  speaking) than <other>, false otherwise.
 */
inline bool Wad_name::less (const Wad_name& other) const
{
  const char *p1 = name;
  const char *p2 = other.name;
  if (*p1 < *p2) return true; if (*p1++ > *p2++) return false;
  if (*p1 < *p2) return true; if (*p1++ > *p2++) return false;
  if (*p1 < *p2) return true; if (*p1++ > *p2++) return false;
  if (*p1 < *p2) return true; if (*p1++ > *p2++) return false;
  if (*p1 < *p2) return true; if (*p1++ > *p2++) return false;
  if (*p1 < *p2) return true; if (*p1++ > *p2++) return false;
  if (*p1 < *p2) return true; if (*p1++ > *p2++) return false;
  if (*p1 < *p2) return true;
  return false;
  // Supposedly slower
  //return memcmp (name, other.name, sizeof name) < 0;
}


#endif
//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
