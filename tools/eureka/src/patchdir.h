/*
 *  patchdir.h
 *  Patch_dir class
 *  AYM 1999-11-25
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


#ifndef YH_PATCHDIR
#define YH_PATCHDIR


#include <map>


/*
 *  Patch_dir 
 *  
 *  The purpose of this class is to hide the details of how
 *  patches are stored in the wads from Yadex. It provides
 *  two basic services :
 *
 *  - refresh() Must be called every time the directory
 *      changes in any way.
 *
 *  - loc_by_name() Return the lump location (WadPtr,
 *      offset, length) of a patch by name. Used
 *      by the patch browser. If the patch does
 *      not exist, returns a NULL WadPtr.
 *
 *  - loc_by_num()  Return the lump location (WadPtr,
 *      offset, length) of a patch by number (as
 *      in TEXTURE[12]). Used by the texture
 *      browser. If the patch does not exist,
 *      returns a NULL WadPtr.
 *
 *  - list()  Return a Patch_list object that provides
 *      what InputNameFromListWithFunc() needs
 *      to browse the patches. I suggest that
 *      this object be created immediately
 *      before it's needed and destroyed
 *      immediately after because it is not
 *      intended to remain valid across calls to
 *      refresh(). Ignoring this advice will
 *      cause some interesting crashes.
 *
 *  The lifetime of this class is about the same as the
 *  lifetime of Yadex itself. It is instantiated only once.
 *  After construction, it is "empty". You must call
 *  refresh() to really initialize it (and call it again
 *  every time the directory changes in any way or you'll
 *  get strange results--or worse).
 */

struct Pllik
{
  Pllik (const char *name);
  wad_pic_name_t _name;
};

struct Pllik_less
{
  bool operator() (const Pllik& p1, const Pllik& p2) const;
};

typedef std::map<Pllik, Lump_loc, Pllik_less> Patch_lumps_map;

class Patch_list
{
  public :
    Patch_list ();
    ~Patch_list ();
    const char **data ();
    size_t size ();
    void set (Patch_lumps_map& patch_lumps);
    void clear ();

  private :
    char **array;
    size_t nelements;
};

class Patch_dir
{
  public :
    Patch_dir ();
    ~Patch_dir ();
    void refresh (MDirPtr master_dir);
    void loc_by_name (const char *name, Lump_loc& loc);
    void loc_by_num (s16_t num, Lump_loc& loc);
    wad_pic_name_t *name_for_num (s16_t num);
    void list (Patch_list& pl);

  private :
    char *pnames;     // The contents of PNAMES
              // (block of npnames x 8 chars)
    size_t npnames;     // Number of entries in PNAMES
    Patch_lumps_map patch_lumps;  // List of patch lumps, sorted
              // by name (no duplicates), with
          // their location.
};


extern Patch_dir patch_dir;   // Only one instance and it's global


#endif  /* DO NOT ADD ANYTHING AFTER THIS LINE */
