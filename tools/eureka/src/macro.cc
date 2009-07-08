/*
 *  macro.cc
 *  Expanding macros
 *  AYM 1999-11-30
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
#include "macro.h"


/*
 *  macro_expand, vmacro_expand - expand macro references in a string
 *
 *  Copies the NUL-terminated string in <fmt> into the
 *  buffer <buf>, replacing certain strings by other
 *  strings. <buf> must be at least (<size> + 1) characters
 *  long.
 *
 *  The macro definitions are provided by the optional
 *  arguments that follow <fmt>. You must pass two (const
 *  char *) for each macro. The first is the name of the
 *  macro, NUL-terminated. The second is the value of the
 *  macro, also NUL-terminated. The last macro value must be
 *  followed by a null pointer ((const char *) 0).
 *
 *  Matching is case-sensitive. The first match is used.
 *  For example,
 *
 *    macro_expand (buf, 99, "a%bcd", "%b", "1", "%bc", "2", (char *) 0);
 *
 *  puts "a1cd" in buf.
 *
 *  If the value of a macro to expand is a null pointer, the
 *  macro expands to an empty string.
 *
 *  If <fmt> expands to more than <size> characters, only
 *  the first <size> characters of the expansion are stored
 *  in <buf>. After the function returns, <buf> is
 *  guaranteed to be NUL-terminated.
 *
 *  Return 0 if successful, non-zero if one of the macros
 *  could not be expanded (value was a null pointer). In
 *  that case, the actual number returned is the number of
 *  the first macro that could not be expanded, starting
 *  from 1.
 */
int macro_expand (char *buf, size_t size, const char *fmt, ...)
{
  va_list list;
  va_start (list, fmt);
  return vmacro_expand (buf, size, fmt, list);
}


int vmacro_expand (char *buf, size_t size, const char *fmt, va_list list)
{
  int rc = 0;
  int macro_no = 0;

  *buf = '\0';
  // This is awful, but who cares ?
  while (*fmt)
  {
    va_list l;
    const char *macro_name = 0;
    const char *macro_value = 0;

#ifdef __va_copy
    __va_copy(l, list);
#else
    l = list;
#endif

    while ((macro_name = va_arg (l, const char *)))
    {
      macro_value = va_arg (l, const char *);
      size_t len1 = strlen (fmt);
      size_t len2 = strlen (macro_name);
      if (len1 >= len2 && ! memcmp (fmt, macro_name, len2))
  break;
    }
    if (macro_name != 0)
    {
      macro_no++;
      if (macro_value == 0)
      {
  if (rc == 0)
    rc = macro_no;
      }
      else
        strcat (buf, macro_value);
      fmt += strlen (macro_name);
    }
    else
    {
      char ch[2]; ch[0] = *fmt; ch[1] = 0;
      strcat(buf, ch);
      fmt++;
    }
  }
  return rc;
}


/*
 *  macro_expand - expand macro references in a string
 *
 *  Copies the NUL-terminated string in <fmt> into the
 *  buffer <buf>, replacing certain strings by other
 *  strings. <buf> must be at least (<size> + 1) characters
 *  long.
 *
 *  The macro definitions are obtained by scanning the
 *  iterator range <macdef_begin> through <macdef_end>. The
 *  only constraints on these iterators are that they must
 *  be input iterators and iterator->name and
 *  iterator->value must be defined and point to
 *  NUL-terminated strings.
 *
 *  Matching is case-sensitive. If several names would
 *  match, the first one is used.  For example,
 *
 *    struct macdef
 *    {
 *      const char *name;
 *      const char *value;
 *      macdef (const char *name, const char *value) :
 *        name (name), value (value);
 *    };
 *    std::list<macdef> macdefs;
 *    macdefs.push_back (macdef ("%b", "1"));
 *    macdefs.push_back (macdef ("%bc", "2"));
 *    macro_expand (buf, 99, "a%bcd", macdefs.begin (), macdefs.end ());
 *
 *  puts "a1cd" in buf, not "a2d".
 *
 *  If the value of a macro to expand is a null pointer, the
 *  macro expands to an empty string.
 *
 *  If <fmt> expands to more than <size> characters, only
 *  the first <size> characters of the expansion are stored
 *  in <buf>. After the function returns, <buf> is
 *  guaranteed to be NUL-terminated.
 *
 *  Return 0 if successful, non-zero if one of the macros
 *  could not be expanded (value was a null pointer). In
 *  that case, the actual number returned is the number of
 *  the first macro that could not be expanded, starting
 *  from 1.
 */
template <typename const_iterator>
int macro_expand (char *buf, size_t size, const char *fmt,
  const_iterator macdef_begin, const_iterator macdef_end)
{
  int rc = 0;
  int macro_no = 0;

  *buf = '\0';
  size_t fmt_len = strlen (fmt);
  while (*fmt != '\0')
  {
    const char *macro_name     = 0;
    const char *macro_value    = 0;
    size_t      macro_name_len = 0;
    bool        match          = false;
    for (const_iterator i = macdef_begin; i != macdef_end; ++i)
    {
      macro_name     = i->name;
      macro_value    = i->value;
      macro_name_len = strlen (macro_name);
      if (macro_name_len <= fmt_len
    && memcmp (fmt, macro_name, macro_name_len) == 0)
      {
  match = true;
  break;
      }
    }
    if (match)
    {
      macro_no++;
      if (macro_value == 0)
      {
  if (rc == 0)
    rc = macro_no;
      }
      else
        strcat (buf, macro_value);
      fmt     += macro_name_len;
      fmt_len -= macro_name_len;
    }
    else
    {
        char ch[2]; ch[0] = *fmt; ch[1] = 0;
      strcat (buf, ch);
      fmt++;
      fmt_len--;
    }
  }
  return rc;
}


#ifdef TEST
/*
  make CXXFLAGS='-DTEST -DY_UNIX -DY_X11'
    LDFLAGS=../obj/0/atclib/al_saps.o\ ../obj/0/atclib/al_sapc.o
    macro
*/

#include <list>
#include <algorithm>


struct macdef
{
  const char *name;
  const char *value;
  macdef (const char *name, const char *value) :
    name (name), value (value) { }
};


void dump (const macdef& m)
{
  fprintf (stderr, "%s=%s\n", m.name, m.value);
}


int main ()
{
  char buf[100];
  std::list<macdef> macdefs;
  macdefs.push_back (macdef ("%b", "1"));
  macdefs.push_back (macdef ("%bc", "2"));
  std::for_each (macdefs.begin (), macdefs.end (), dump);
  macro_expand (buf, sizeof buf - 1, "a%bcd", macdefs.begin (), macdefs.end ());
  puts (buf);
  return 0;
}


#endif
