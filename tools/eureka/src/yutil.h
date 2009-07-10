/*
 *  aym.h
 *  Misc. functions
 *  AYM 1999-03-15
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


#ifndef YH_AYM  /* DO NOT INSERT ANYTHING BEFORE THIS LINE */
#define YH_AYM


int levelname2levelno (const char *name);
int levelname2rank (const char *name);
const char *spec_path (const char *spec);
int fncmp (const char *name1, const char *name2);
int is_absolute (const char *filename);
int y_stricmp (const char *s1, const char *s2);
int y_strnicmp (const char *s1, const char *s2, size_t len);
int y_snprintf (char *buf, size_t size, const char *fmt, ...);
int y_vsnprintf (char *buf, size_t size, const char *fmt, va_list args);
void y_strupr (char *string);
bool file_exists (const char *);
void y_filename (char *buf, size_t size, const char *path);

void check_types ();


/*
 *  within
 *  Is <value> >= <lower_bound> and <= <upper_bound> ?
 */
inline int within (int value, int lower_bound, int upper_bound)
{
  return value >= lower_bound && value <= upper_bound;
}


/*
 *  outside
 *  Is <value> < <lower_bound> or > <upper_bound> ?
 */
inline int outside (int value, int lower_bound, int upper_bound)
{
  return value < lower_bound || value > upper_bound;
}


/*
 *  dectoi
 *  If <c> is a decimal digit ("[0-9]"), return its value.
 *  Else, return a negative number.
 */
inline int dectoi (char c)
{
  if (isdigit ((unsigned char) c))
    return c - '0';
  else
    return -1;
}


/*
 *  hextoi
 *  If <c> is a hexadecimal digit ("[0-9A-Fa-f]"), return its value.
 *  Else, return a negative number.
 */
inline int hextoi (char c)
{
  if (isdigit ((unsigned char) c))
    return c - '0';
  else if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  else if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  else
    return -1;
}


/*
 *  b36toi
 *  If <c> is a base 36 digit ("[0-9A-Za-z]"), return its value.
 *  Else, return a negative number.
 */
inline int b36toi (char c)
{
  if (isdigit ((unsigned char) c))
    return c - '0';
  else if (islower (c))
    return c - 'a' + 10;
  else if (isupper (c))
    return c - 'A' + 10;
  else
    return -1;
}


/*
 *  y_isident - return true iff <c> is one of a-z, A-Z, 0-9 or "_".
 *
 *  Intentionally not using isalpha() and co. because I
 *  don't want the results to depend on the locale.
 */
inline bool y_isident (char c)
{
  switch (c)
  {
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z':

    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z':

    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9':

    case '_':

      return true;

    default:

      return false;
  }
}


/*
 *  fnewline
 *  Write a newline to a binary file. For Unix, LF. For
 *  other platforms, CR LF.
 *  Return EOF on failure.
 */
inline int fnewline (FILE *fd)
{
#ifdef Y_UNIX
  return putc ('\n', fd);
#else
  return putc ('\r', fd), putc ('\n', fd);
#endif
}


/*
 *  round_up
 *  Round a value up to the next multiple of quantum.
 *
 *  Both the value and the quantum are supposed to be positive.
 */
inline void round_up (int& value, int quantum)
{
  value = ((value + quantum - 1) / quantum) * quantum;
}


/*
 *  y_isprint
 *  Is <c> a printable character in ISO-8859-1 ?
 */
inline bool y_isprint (char c)
{
  return (c & 0x60) && (c != 0x7f);
}


#endif  /* DO NOT ADD ANYTHING AFTER THIS LINE */
