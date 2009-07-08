/*
 *  sanity.cc
 *  Sanity checks
 *  AYM 1998-06-14
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
#include "sanity.h"
#include "w_structs.h"


/*
 *  check_types
 *
 *  Sanity checks about the sizes and properties of certain types.
 *  Useful when porting.
 */

#define assert_size(type,size)            \
  do                  \
  {                 \
    if (sizeof (type) != size)            \
      fatal_error ("sizeof " #type " is %d (should be " #size ")",  \
  (int) sizeof (type));           \
  }                 \
  while (0)
   
#define assert_wrap(type,high,low)          \
  do                  \
  {                 \
    type n = high;              \
    if (++n != low)             \
      fatal_error ("Type " #type " wraps around to %lu (should be " #low ")",\
  (unsigned long) n);           \
  }                 \
  while (0)

void check_types ()
{
  assert_size (u8_t,  1);
  assert_size (s8_t,  1);
  assert_size (u16_t, 2);
  assert_size (s16_t, 2);
  assert_size (u32_t, 4);
  assert_size (s32_t, 4);

  assert_size (struct LineDef, 14);
  assert_size (struct Sector,  26);
  assert_size (struct SideDef, 30);
  assert_size (struct Thing,   10);
  assert_size (struct Vertex,   4);
}


/*
 *  check_charset
 *
 *  If this test fails on your platform, send me a postcard
 *  from your galaxy.
 */
void check_charset ()
{
  /* Always false, unless your platform uses a
     character set even worse than EBCDIC (yet
     unseen). */
  if (   '0' + 1 != '1'
      || '1' + 1 != '2'
      || '2' + 1 != '3'
      || '3' + 1 != '4'
      || '4' + 1 != '5'
      || '5' + 1 != '6'
      || '6' + 1 != '7'
      || '7' + 1 != '8'
      || '8' + 1 != '9'
      || 'a' + 1 != 'b'
      || 'b' + 1 != 'c'
      || 'c' + 1 != 'd'
      || 'd' + 1 != 'e'
      || 'e' + 1 != 'f'
      || 'A' + 1 != 'B'
      || 'B' + 1 != 'C'
      || 'C' + 1 != 'D'
      || 'D' + 1 != 'E'
      || 'E' + 1 != 'F')
    fatal_error ("This platform uses a terminally broken character set");
}


