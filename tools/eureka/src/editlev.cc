/*
 *  editlev.cc
 *  AYM 1998-09-06
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


#include "main.h"
#include <time.h>
#include "editlev.h"
#include "editloop.h"
#include "game.h"
#include "gfx.h"
#include "levels.h"
#include "patchdir.h"
#include "w_file.h"



/*
 *  find_level
 *  Look in the master directory for levels that match
 *  the name in <name_given>.
 *
 *  <name_given> can have one of the following formats :
 *  
 *    [Ee]n[Mm]m      EnMm
 *    [Mm][Aa][Pp]nm  MAPnm
 *    n               MAP0n
 *    nm              Either EnMn or MAPnm
 *    ijk             EiMjk (Doom alpha 0.4 and 0.5)
 *
 *  Return:
 *  - If <name_given> is either [Ee]n[Mm]m or [Mm][Aa][Pp]nm,
 *    - if the level was found, its canonical (uppercased)
 *      name in a freshly malloc'd buffer,
 *    - else, NULL.
 *  - If <name_given> is either n or nm,
 *    - if either EnMn or MAPnm was found, the canonical name
 *      of the level found, in a freshly malloc'd buffer,
 *    - if none was found, <error_none>,
 *    - if the <name_given> is invalid, <error_invalid>,
 *    - if several were found, <error_non_unique>.
 */
char *find_level (const char *name_given)
{
// Is it a shorthand name ? ("1", "23", ...)
if (isdigit(name_given[0])
    && (atoi (name_given) <= 99
      || atoi (name_given) <= 999 && yg_level_name == YGLN_E1M10))
   {
   int n = atoi (name_given);
   char *name1 = (char *) malloc (7);
   char *name2 = (char *) malloc (6);
   if (n > 99)
      sprintf (name1, "E%dM%02d", n / 100, n % 100);
   else
      sprintf (name1, "E%dM%d", n / 10, n % 10);
   sprintf (name2, "MAP%02d", n);
   int match1 = FindMasterDir (MasterDir, name1) != NULL;
   int match2 = FindMasterDir (MasterDir, name2) != NULL;
   if (match1 && ! match2)  // Found only ExMy
      {
      free (name2);
      return name1;
      }
   else if (match2 && ! match1) // Found only MAPxy
      {
      free (name1);
      return name2;
      }
   else if (match1 && match2) // Found both
      {
      free (name1);
      free (name2);
      return error_non_unique;
      }
   else       // Found none
      {
      free (name1);
      free (name2);
      return error_none;
      }
   }

#if 1
// Else look for <name_given>
if (FindMasterDir (MasterDir, name_given))
   return strdup (name_given);
else
   {
   if (levelname2levelno (name_given))
      return NULL;
   else
      return error_invalid;
   }
#else
// If <name_given> is "[Ee]n[Mm]m" or "[Mm][Aa][Pp]nm", look for that
if (levelname2levelno (name_given))
   {
   char *canonical_name = strdup (name_given);
   for (char *p = canonical_name; *p; p++)
      *p = toupper (*p);  // But shouldn't FindMasterDir() be case-insensitive ?
   if (FindMasterDir (MasterDir, canonical_name))
      return canonical_name;
   else
      {
      free (canonical_name);
      return NULL;
      }
   }
return error_invalid;
#endif
}


/*
   the driving program
*/

void EditLevel (const char *levelname, bool newlevel)
{
  ReadWTextureNames ();
  ReadFTextureNames ();
  patch_dir.refresh (MasterDir);
  if (InitGfx ())
    return;

  if (newlevel && ! levelname)  // "create"
  {
    EmptyLevelData (levelname);
    MapMinX = -2000;
    MapMinY = -2000;
    MapMaxX = 2000;
    MapMaxY = 2000;
    Level = 0;
  }
  else if (newlevel && levelname)  // "create <level_name>"
  {
    printf ("Sorry, \"create <level_name>\" is not implemented."
        " Try \"create\" without argument.\n");
    TermGfx ();
    return;
  }
  else  // "edit <level_name>" or "edit"
  {
#if 0
    if (levelname == 0 || ! levelname2levelno (levelname)
        || ! FindMasterDir (MasterDir, levelname))
      levelname = SelectLevel (atoi (levelname)); /* returns "" on Esc */
    if (levelname2levelno (levelname))
    {
#endif
      ClearScreen ();
      if (ReadLevelData (levelname))
      {
        goto done;  // Failure!
      }
#if 0
    }
#endif
  }
  LogMessage (": Editing %s...\n", levelname ? levelname : "new level");

///---   // Set the name of the window
///---   {
///--- #define BUFSZ 100
///---     char buf[BUFSZ + 1];
///--- 
///--- #ifdef OLD_TITLE
///---     al_scps (buf, "Yadex - ", BUFSZ);
///---     if (Level && Level->wadfile)
///---       al_saps (buf, Level->wadfile->filename, BUFSZ);
///---     else
///---       al_saps (buf, "New level", BUFSZ);
///---     if (Level)
///---     {
///---       al_saps (buf, " - ",           BUFSZ);
///---       al_saps (buf, Level->dir.name, BUFSZ);
///---     }
///---     else if (levelname)
///---     {
///---       al_saps (buf, " - ",     BUFSZ);
///---       al_saps (buf, levelname, BUFSZ);
///---     }
///--- #else
///---     al_scps (buf, "Yadex: ", BUFSZ);
///---     al_saps (buf, (levelname) ? levelname : "(null)", BUFSZ);
///--- #endif
///---     ///!!!!! XStoreName (dpy, win, buf);
///--- #undef BUFSZ
///---   }

  {
    time_t t0, t1;
    time (&t0);
    EditorLoop (levelname);
    time (&t1);
    LogMessage (": Finished editing %s...\n", levelname ? levelname : "new level");
    if (Level && Level->wadfile)
    {
      const char *const file_name =
        Level->wadfile ? Level->wadfile->pathname () : "(New level)";
    }
  }
done :
  TermGfx ();
  if (! Registered)
    printf ("Please register the game"
        " if you want to be able to save your changes.\n");

  ForgetLevelData ();
  /* forget the level pointer */
  Level = 0;
  ForgetWTextureNames ();
  ForgetFTextureNames ();
}


