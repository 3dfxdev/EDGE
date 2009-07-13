//------------------------------------------------------------------------
//  THING OPERATIONS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor (C) 2001-2009 Andrew Apted
//                     (C) 1997-2003 André Majorel et al
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#define YC_THINGS

#include "main.h"

#include "game.h"
#include "things.h"
#include "levels.h"
#include "selectn.h"
#include "m_bitvec.h"
#include "w_structs.h"


// This is the structure of a table of things attributes.
// In this table, things are sorted by increasing number.
// It is searched in a dichotomic fashion by get_thing_*().
// This table is only here for speed.
typedef struct
  {
  wad_ttype_t type;
  char    flags;
  short         radius;
  acolour_t     colour;
  const char    *desc;
  const char  *sprite;
  } thing_attributes_t;

static thing_attributes_t *things_table;
static size_t nthings;
int _max_radius;
static size_t last_table_idx = (size_t) -1;


static int things_table_cmp (const void *a, const void *b);


static const int default_radius = 16;


/*
 *  create_things_table
 *  Build things_table, a table of things attributes
 *  that's used by get_thing_*() to speed things up.
 *  Call delete_things_table to delete that table.
 */
void create_things_table ()
{
size_t n;

_max_radius = default_radius;
nthings = thingdef.size();
if (nthings == 0)
   {
   things_table = NULL;
   return;
   }
things_table = (thing_attributes_t *) malloc (nthings * sizeof *things_table);
if (! things_table)
   FatalError("Not enough memory");


for (n = 0; n < nthings; n++)
{
    things_table[n].type   = thingdef[n]->number;
    things_table[n].flags  = thingdef[n]->flags;
    things_table[n].radius = thingdef[n]->radius;
    _max_radius = MAX(_max_radius, thingdef[n]->radius);

///!!!    // Fetch the app colour no. for the thinggroup
///!!!    for (al_lrewind (thinggroup); ! al_leol (thinggroup); al_lstep (thinggroup))
///!!!    {
///!!!        if (CUR_THINGGROUP  /* don't segfault if zero thinggroup ! */
///!!!                && CUR_THINGGROUP->thinggroup == CUR_THINGDEF->thinggroup)
///!!!        {
///!!!            things_table[n].colour = CUR_THINGGROUP->acn;
///!!!            break;
///!!!        }
///!!!    }

    things_table[n].desc   = thingdef[n]->desc;
    things_table[n].sprite = thingdef[n]->sprite;
}

// Sort the table by increasing thing type
qsort (things_table, nthings, sizeof *things_table, things_table_cmp);

#if 0
printf ("Type  Colour Radius Desc\n");
for (n = 0; n < nthings; n++)
   printf ("%5d %-6d %3d    %s\n",
      things_table[n].type,
      things_table[n].colour,
      things_table[n].radius,
      things_table[n].desc);
#endif
}


/*
 *  delete_things_table
 *  Free what create_things_table() allocated.
 */
void delete_things_table (void)
{
if (things_table)
   {
   free (things_table);
   nthings = 0;
   }
}


/*
 *  things_table_cmp
 *  Used by create_things_table() to sort the table
 *  by increasing THING type.
 */
static int things_table_cmp (const void *a, const void *b)
{
return ((const thing_attributes_t *) a)->type
     - ((const thing_attributes_t *) b)->type;
}


/*
 *  lookup_thing
 *  Does a binary search in things_table for the thing of type <type>.
 *  If succeeds, returns the index of the thing.
 *  If fails, returns ((size_t) -1).
 *  To further speed things up, the last index found is kept
 *  between invocations in a static variable. Thus, if several
 *  attributes of the same thing type are queried in a row,
 *  the table search is done only once.
 */
inline int lookup_thing (wad_ttype_t type)
{
if (last_table_idx < nthings && things_table[last_table_idx].type == type)
   return last_table_idx;

if (things_table == NULL)
   return (size_t) -1;

size_t nmin = 0;
size_t nmax = nthings - 1;
for (;;)
   {
   last_table_idx = (nmin + nmax) / 2;
   if (type > things_table[last_table_idx].type)
      {
      if (nmin >= nmax)
         break;
      nmin = last_table_idx + 1;
      }
   else if (type < things_table[last_table_idx].type)
      {
      if (nmin >= nmax)
         break;
      if (last_table_idx < 1)
         break;
      nmax = last_table_idx - 1;
      }
   else
      return last_table_idx;
   }
return (size_t) -1;
}


/*
 *  is_thing_type - is given type valid (i.e. defined in the ygd)
 *
 *  Return true if the thing is defined, false otherwise.
 */
bool is_thing_type (wad_ttype_t type)
{
size_t table_idx = lookup_thing (type);
return table_idx < nthings;
}


/*
 *  get_thing_colour - return the colour of the thing of given type.
 *
 *  Return the colour. If the 
 */
acolour_t get_thing_colour (wad_ttype_t type)
{
return FL_RED;

size_t table_idx = lookup_thing (type);
if (table_idx == (size_t) -1)
   return FL_RED;  // Not found.
else
   return things_table[table_idx].colour;
}


/*
 *  get_thing_name - return the description of the thing of given type.
 */
const char *get_thing_name (wad_ttype_t type)
{
  size_t table_idx = lookup_thing (type);
  if (table_idx == (size_t) -1)
  {
    static char buf[20];
    sprintf (buf, "??? unknown ???");  // Not found.
    return buf;
  }
  else
    return things_table[table_idx].desc;
}


/*
 *  get_thing_sprite
 *  Return the root of the sprite name for the thing of given type.
 */
const char *get_thing_sprite (wad_ttype_t type)
{
size_t table_idx = lookup_thing (type);
if (table_idx == (size_t) -1)
   return NULL;  // Not found
else
   return things_table[table_idx].sprite;
}


/*
 *  get_thing_flags
 *  Return the flags for the thing of given type.
 */
char get_thing_flags (wad_ttype_t type)
{
size_t table_idx = lookup_thing (type);
if (table_idx == (size_t) -1)
   return 0;  // Not found
else
   return things_table[table_idx].flags;
}


/*
 *  get_thing_radius
 *  Return the radius of the thing of given type.
 */
int get_thing_radius (wad_ttype_t type)
{
size_t table_idx = lookup_thing (type);
if (table_idx == (size_t) -1)
   return default_radius;  // Not found.
else
   return things_table[table_idx].radius;
}


/*
 *  get_max_thing_radius
 *  Return the radius of the largest thing that exists.
 *  This is a speedup function, used by GetCurObject()
 *  to avoid calculating the distance for the things
 *  that are obviously too far away.
 */

/* It's now inlined in things.h
int get_max_thing_radius ()
{
return _max_radius;
}
*/


/*
 *  GetAngleName
 *  Get the name of the angle
 */
const char *GetAngleName (int angle)
{
static char buf[30];

switch (angle)
   {
   case 0:
      return "East";
   case 45:
      return "North-east";
   case 90:
      return "North";
   case 135:
      return "North-west";
   case 180:
      return "West";
   case 225:
      return "South-west";
   case 270:
      return "South";
   case 315:
      return "South-east";
   }
sprintf (buf, "ILLEGAL (%d)", angle);
return buf;
}


/*
 *  GetWhenName
 *  get string of when something will appear
 */
const char *GetWhenName (int when)
{
static char buf[16+3+1];
// "N" is a Boom extension ("not in deathmatch")
// "C" is a Boom extension ("not in cooperative")
// "F" is an MBF extension ("friendly")
const char *flag_chars = "????" "????" "FCNM" "D431";
int n;

char *b = buf;
for (n = 0; n < 16; n++)
   {
   if (n != 0 && n % 4 == 0)
      *b++ = ' ';
   if (when & (0x8000u >> n))
      *b++ = flag_chars[n];
   else
      *b++ = '-';
   }
*b = '\0';
return buf;

#if 0
static char buf[30];
char *ptr = buf;
*ptr = '\0';
if (when & 0x01)
   {
   strcpy (ptr, "D12");
   ptr += 3;
   }
if (when & 0x02)
   {
   if (ptr != buf)
      *ptr++ = ' ';
   strcpy (ptr, "D3");
   ptr += 2;
   }
if (when & 0x04)
   {
   if (ptr != buf)
      *ptr++ = ' ';
   strcpy (ptr, "D45");
   ptr += 3;
   }
if (when & 0x08)
   {
   if (ptr != buf)
      *ptr++ = ' ';
   strcpy (ptr, "Deaf");
   ptr += 4;
   }
if (when & 0x10)
   {
   if (ptr != buf)
      *ptr++ = ' ';
   strcpy (ptr, "Multi");
   ptr += 5;
   }
return buf;
#endif
}


/*
 *  spin_thing - change the angle of things
 */
void spin_things (SelPtr obj, int degrees)
{
  SelPtr cur;

  if (! obj)
    return;
  for (cur = obj; cur; cur = cur->next)
  {
    Things[cur->objnum].angle += degrees;
    while (Things[cur->objnum].angle >= 360)  // No we can't use %
      Things[cur->objnum].angle -= 360;
    while (Things[cur->objnum].angle < 0) // (negatives...)
      Things[cur->objnum].angle += 360;
  }
  things_angles++;
  MadeChanges = 1;
}


/*
 *  frob_things_flags - set/reset/toggle things flags
 *
 *  For all the things in <list>, apply the operator <op>
 *  with the operand <operand> on the flags field.
 */
void frob_things_flags (SelPtr list, int op, int operand)
{
  SelPtr cur;
  s16_t mask;

  if (op == YO_CLEAR || op == YO_SET || op == YO_TOGGLE)
    mask = 1 << operand;
  else
    mask = operand;

  for (cur = list; cur; cur = cur->next)
  {
    if (op == YO_CLEAR)
      Things[cur->objnum].options &= ~mask;
    else if (op == YO_SET)
      Things[cur->objnum].options |= mask;
    else if (op == YO_TOGGLE)
      Things[cur->objnum].options ^= mask;
    else
    {
      nf_bug ("frob_things_flags: op=%02X", op);
      return;
    }
  }
  MadeChanges = 1;
}



/*
 *  centre_of_things
 *  Return the coordinates of the centre of a group of things.
 */
void centre_of_things (SelPtr list, int *x, int *y)
{
SelPtr cur;
int nitems;
long x_sum;
long y_sum;

x_sum = 0;
y_sum = 0;
for (nitems = 0, cur = list; cur; cur = cur->next, nitems++)
   {
   x_sum += Things[cur->objnum].x;
   y_sum += Things[cur->objnum].y;
   }
if (nitems == 0)
   {
   *x = 0;
   *y = 0;
   }
else
   {
   *x = (int) (x_sum / nitems);
   *y = (int) (y_sum / nitems);
   }
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
