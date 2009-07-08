/*
 *  locate.cc - Locate class
 *  AYM 2003-03-24
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


#include "yadex.h"

#include <sys/types.h>
#include <sys/stat.h>

#include "locate.h"
#include "macro.h"


static void trace (const char *domain, const char *fmt, ...)
{
  if (verbose)
  {
    va_list args;

    fprintf (stdout, "%s: ", domain);
    va_start (args, fmt);
    vprintf (fmt, args);
    va_end (args);
    putchar ('\n');
  }
}


/*
 *  Locate::Locate - ctor
 *
 *  - search_path points to a NULL-terminated array of C
 *    strings (char *) constituting the search path. The
 *    paths may contains the following macros, which will be
 *    expanded on the fly :
 *    %v  the version of Yadex
 *    ~   $HOME
 *
 *  - name is the basename of the file to locate. It may
 *    include slashes. If it's absolute, the search path
 *    won't be used.
 *
 *  - backwards is the direction in which the search path is
 *    walked. true is front-to-back, false is back-to-front.
 */
Locate::Locate (const char *const *search_path, const char *name, bool backw)
{
  this->search_path = search_path;
  this->name        = name;
  this->backwards   = backw;
  absolute          = is_absolute (name);
  rewound           = true;
  rewind ();
}


/*
 *  Locate::rewind - rewind the cursor
 *
 *  Calling this method will cause the next call to
 *  get_next() to return the first match, as if get_next()
 *  had never been called.
 */
void Locate::rewind ()
{
  rewound = true;

  if (backwards)
  {
    // Advance to the end of the list
    for (cursor = search_path; *cursor != NULL; cursor++)
      ;
  }
  else
  {
    cursor = search_path;
  }
}


/*
 *  Locate::get_next - return the next match
 *
 *  Returns a pointer to the pathname of the next match, or
 *  a null pointer if there are no more matches left. The
 *  returned pointer is valid until get_next() is called
 *  again or the Locate object is destroyed.
 */
const char *Locate::get_next ()
{
  if (absolute)
  {
    if (! rewound)      // Result has exactly one element
      return NULL;
    rewound = false;
    if (strlen (name) > sizeof pathname - 1)
    {
      warn ("%s: file name too long\n", name);
      return NULL;
    }
    strcpy (pathname, name);
    return pathname;
  }

  const char *home = getenv ("HOME");

  // Walk the list
  for (;;)
  {
    // Make dirname point to the current path in the search path
    const char *dirname;

    if (backwards)
    {
      if (cursor == search_path)
  break;
      cursor--;
      dirname = *cursor;
    }
    else
    {
      if (*cursor == NULL)
  break;
      dirname = *cursor;
      cursor++;
    }

fprintf(stderr, "MACRO_EXPAND: %s\n", dirname);

    // Expand the macros in the path into the result buffer
    int r = macro_expand (pathname, sizeof pathname - 1, dirname,
      "%v", yadex_version,
      "~",  home,
      (const char *) 0);
    if (r != 0)
    {
      trace ("locate", "%s: Could not expand macro #%d", dirname, r);
      continue;
    }

    // Append the basename
    if (strlen (pathname) > 0 && pathname[strlen (pathname)] - 1 != '/')
    {
      if (strlen (pathname) + 2 >= sizeof pathname)
      {
  warn ("%s: file name too long, skipping\n", dirname);
  continue;
      }
      strcat (pathname, "/");
    }
    if (strlen (pathname) + strlen (name) + 1 >= sizeof pathname)
    {
      warn ("%s: file name too long, skipping\n", dirname);
      continue;
    }
    strcat (pathname, name);

    // Look for a file of that name.
    struct stat s;
    r = stat (pathname, &s);
    if (r == 0 && ! S_ISDIR (s.st_mode))
    {
      trace ("locate", "%s: %s: hit", name, pathname);
      return pathname;
    }
    else if (r == 0 && S_ISDIR (s.st_mode))
    {
      errno = EISDIR;
    }
    else if (r != 0 && errno != ENOENT)
    {
      warn ("%s: %s\n", pathname, strerror (errno));
    }
    trace ("locate", "%s: %s: miss (%s)", name, pathname, strerror (errno));
  }

  return NULL;
}

