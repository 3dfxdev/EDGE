/*
 *  patchdir.cc
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


#include "yadex.h"
#include "patchdir.h"
#include "w_file.h"
#include "w_io.h"


/* This class has only one instance, global and shared by
   everyone. I don't think there would ever be a need for
   multiple instances. Unless you add the capability to edit
   levels from several different iwads simultaneously. But then,
   how do you decide which windows are affected by loading a
   pwad ? -- AYM 1999-11-27 */
Patch_dir patch_dir;


/*
 *  ctor
 */
Patch_dir::Patch_dir ()
{
  pnames = 0;
  npnames = 0;
}


/*
 *  dtor
 */
Patch_dir::~Patch_dir ()
{
  if (pnames != 0)
    FreeMemory (pnames);
  if (! patch_lumps.empty ())
    patch_lumps.clear ();
}


/*
 *  Patch_dir::refresh
 */
void Patch_dir::refresh (MDirPtr master_dir)
{
  /* refresh() can be called more than once on the same object.
     And usually is ! */
  if (pnames != 0)
  {
    FreeMemory (pnames);
    pnames = 0;
    npnames = 0;
  }
  if (! patch_lumps.empty ())
    patch_lumps.clear ();

  /* First load PNAMES so that we known in which order we should
     put the patches in the array. */
  {
    bool success = false;
    const char *lump_name = "PNAMES";
    do
    {
      MDirPtr dir;
      s32_t npatches_head = -42;
      s32_t npatches_body = -42;

      dir = FindMasterDir (master_dir, lump_name);
      if (dir == 0)
      {
  warn ("No %s lump, won't be able to render textures\n", lump_name);
  break;
      }
      s32_t pnames_body_size = dir->dir.size - 4;
      if (pnames_body_size < 0)
      {
  warn ("Bad %s (negative size %ld), won't be able to render textures\n",
      lump_name, (long) dir->dir.size);
  break;
      }
      if (pnames_body_size % 8)
      {
  warn ("%s lump has weird size %ld, discarding last %d bytes\n",
      (long) dir->dir.size, (int) (pnames_body_size % 8));
      }
      npatches_body = pnames_body_size / 8;
      const Wad_file *wf = dir->wadfile;
      wf->seek (dir->dir.start);
      if (wf->error ())
      {
  warn ("%s: seek error\n", lump_name);
  break;
      }
      wf->read_i32 (&npatches_head);
      if (wf->error ())
      {
  warn ("%s: error reading header\n", lump_name);
  break;
      }
      if (npatches_head > 0 && npatches_head < npatches_body)
  npnames = npatches_head;
      else
  npnames = npatches_body;
      if (npatches_head != npatches_body)
      {
  warn("%s: header says %ld patches, lump size suggests %ld,"
      " going for %lu\n",
      lump_name, long (npatches_head), long (npatches_body),
      (unsigned long) npnames);
      }
      if (npnames > 32767)
      {
  warn ("%s: too big (%lu patches), keeping only first 32767\n",
      lump_name, (unsigned long) npnames);
  npnames = 32767;
      }
      pnames = (char *) GetMemory (npnames * WAD_PIC_NAME);
      wf->read_bytes (pnames, npnames * WAD_PIC_NAME);
      if (wf->error ())
      {
  warn ("%s: error reading names\n", lump_name);
  break;
      }
      success = true;
    }
    while (0);
    if (! success)
      warn ("%s: errors found, won't be able to render textures\n", lump_name);
  }

  /* Get list of patches in the master directory. Everything
     that is between P_START/P_END or PP_START/PP_END and that
     is not a label is supposed to be a patch. */
  {
    for (MDirPtr dir = master_dir;
  dir && (dir = FindMasterDir (dir, "P_START", "PP_START"));)
    {
      MDirPtr start_label = dir;
      const char *end_label = 0;
      if (! y_strnicmp (dir->dir.name, "P_START", WAD_NAME))
  end_label = "P_END";
      else if (! y_strnicmp (dir->dir.name, "PP_START", WAD_NAME))
  end_label = "PP_END";
      else
  fatal_error ("Bad start label \"%.*s\"", (int) WAD_NAME, dir->dir.name);
      //printf ("[%-8.8s ", dir->dir.name);  // DEBUG
      //fflush (stdout);
      dir = dir->next;
      for (;; dir = dir->next)
      {
  if (! dir)
  {
    warn ("%.128s: no matching %s for %.*s\n",
        start_label->wadfile->pathname (),
        end_label,
        (int) WAD_NAME, start_label->dir.name);
    break;
  }
  if (! y_strnicmp (dir->dir.name, end_label, WAD_NAME))
  {
    if (dir->dir.size != 0)
      warn ("%.128s: label %.*s has non-zero size %ld\n",
    dir->wadfile->pathname (),
    (int) WAD_NAME, dir->dir.name,
    (long) dir->dir.size);
    dir = dir->next;
    //printf ("%-8.8s]\n", dir->dir.name);  // DEBUG
    break;
  } 
  if (dir->dir.start == 0 || dir->dir.size == 0)
  {
    if (! (toupper (dir->dir.name[0]) == 'P'
        && (dir->dir.name[1] == '1'
         || dir->dir.name[1] == '2'
         || dir->dir.name[1] == '3')
        && dir->dir.name[2] == '_'
        && (! y_strnicmp (dir->dir.name + 3, "START", WAD_NAME - 3)
      || ! y_strnicmp (dir->dir.name + 3, "END", WAD_NAME - 3))))
      warn ("%.128s: unexpected label \"%.*s\" among patches.\n",
    dir->wadfile->pathname (), (int) WAD_NAME, dir->dir.name);
    continue;
  }
  //printf ("%-9.8s ", dir->dir.name); fflush (stdout);  // DEBUG
  wad_flat_name_t name;
  memcpy (name, dir->dir.name, sizeof name);
  patch_lumps[name]
    = Lump_loc (dir->wadfile, dir->dir.start, dir->dir.size);
      }
      if (dir)
        dir = dir->next;
    }
    //putchar ('\n');  // DEBUG
  }
#ifdef DEBUG
  for (Patch_lumps_map::const_iterator i = patch_lumps.begin ();
      i != patch_lumps.end (); i++)
  {
    printf ("%-8.8s %p %08lX %ld\n",
      i->first._name,
      i->second.wad,
      i->second.ofs,
      i->second.len);
  }
#endif
}


/*
 *  loc_by_name
 *  Return the (wad, offset, length) location of the lump
 *  that contains patch <name>.
 */
void Patch_dir::loc_by_name (const char *name, Lump_loc& loc)
{
  Patch_lumps_map::const_iterator i = patch_lumps.find (name);
  if (i == patch_lumps.end ())
  {
    loc.wad = 0;
    return;
  }
  loc = i->second;
}


/*
 *  loc_by_num
 *  Return the (wad, offset, length) location of the lump
 *  that contains patch# <num>.
 */
void Patch_dir::loc_by_num (s16_t num, Lump_loc& loc)
{
  wad_pic_name_t *nm = name_for_num (num);
  if (nm == 0)
  {
    loc.wad = 0;
    return;
  }
  loc_by_name ((const char *) nm, loc);
}


/*
 *  name_for_num
 *  Return a pointer on the name of the patch of number <num>
 *  or 0 if no such patch.
 */
wad_pic_name_t *Patch_dir::name_for_num (s16_t num)
{
  if (num < 0 || (size_t) num >= npnames)  // Cast to silence GCC warning
    return 0;
  return (wad_pic_name_t *) (pnames + WAD_PIC_NAME * num);
}


/*
 *  list
 *  Put a list of all existing patch lump, sorted by name
 *  and without duplicates, in <pl>.
 */
void Patch_dir::list (Patch_list& pl)
{
  pl.set (patch_lumps);
}


/*-------------------------- Patch_list --------------------------*/


Patch_list::Patch_list ()
{
  array = 0;
  nelements = 0;
}


Patch_list::~Patch_list ()
{
  clear ();
}


void Patch_list::set (Patch_lumps_map& patch_lumps)
{
  clear ();
  nelements = patch_lumps.size ();
  array = new char *[nelements];

  Patch_lumps_map::const_iterator i = patch_lumps.begin ();
  for (size_t n = 0; n < nelements; n++)
  {
    array[n] = new char[WAD_PIC_NAME + 1];
    *array[n] = '\0';
    strncat (array[n], i++->first._name, WAD_PIC_NAME);
  }
}


void Patch_list::clear ()
{
  if (array != 0)
  {
    for (size_t n = 0; n < nelements; n++)
      delete[] array[n];
    delete[] array;
  }
}


const char **Patch_list::data ()
{
  return (const char **) array;
}


size_t Patch_list::size ()
{
  return nelements;
}


/*---------------------------- Pllik -----------------------------*/


Pllik::Pllik (const char *name)
{
  memcpy (_name, name, sizeof _name);
}


/*-------------------------- Pllik_less --------------------------*/


bool Pllik_less::operator () (const Pllik &p1, const Pllik &p2) const
{
  return y_strnicmp ((const char *) &p1, (const char *) &p2, WAD_PIC_NAME) < 0;
}


