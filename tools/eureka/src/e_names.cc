/*
 *  names.cc
 *  BW & RQ sometime in 1993 or 1994.
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
#include "game.h"


/*
   get the name of an object type
*/
const char *GetObjectTypeName (int objtype)
{
switch (objtype)
   {
   case OBJ_THINGS:   return "thing";
   case OBJ_LINEDEFS: return "linedef";
   case OBJ_SIDEDEFS: return "sidedef";
   case OBJ_VERTICES: return "vertex";
   case OBJ_SEGS:     return "segment";
   case OBJ_SSECTORS: return "ssector";
   case OBJ_NODES:    return "node";
   case OBJ_SECTORS:  return "sector";
   case OBJ_REJECT:   return "reject";
   case OBJ_BLOCKMAP: return "blockmap";
   }
return "< Bug! >";
}



/*
   what are we editing?
*/
const char *GetEditModeName (int objtype)
{
switch (objtype)
   {
   case OBJ_THINGS:   return "Things";
   case OBJ_LINEDEFS:
   case OBJ_SIDEDEFS: return "LD & SD";
   case OBJ_VERTICES: return "Vertices";
   case OBJ_SEGS:     return "Segments";
   case OBJ_SSECTORS: return "Seg-Sectors";
   case OBJ_NODES:    return "Nodes";
   case OBJ_SECTORS:  return "Sectors";
   }
return "< Bug! >";
}



/*
   get a short (16 char.) description of the type of a linedef
*/

const char *GetLineDefTypeName (int type)
{
for (int n = 0; n < (int)ldtdef.size(); n++)
  if (ldtdef[n]->number == type)
    return ldtdef[n]->shortdesc;
return "??  UNKNOWN";
}


/*
   get a short description of the flags of a linedef
*/

const char *GetLineDefFlagsName (int flags)
{
static char buf[20];
// "P" is a Boom extension ("pass through")
// "T" is for Strife ("translucent")
const char *flag_chars = "???T??PANBSLU2MI";
int n;

char *p = buf;
for (n = 0; n < 16; n++)
   {
   if (n != 0 && n % 4 == 0)
      *p++ = ' ';
   if (flags & (0x8000u >> n))
      *p++ = flag_chars[n];
   else
      *p++ = '-';
   }
*p = '\0';
return buf;

#if 0
static char temp[20];
if (flags & 0x0100)
   strcpy (temp, "A"); /* Already on the map (Ma) */
else
   strcpy (temp, "-");
if (flags & 0x80)
   strcat (temp, "V"); /* Invisible on the map (In) */
else
   strcat (temp, "-");
if (flags & 0x40)
   strcat (temp, "B"); /* Blocks sound (So) */
else
   strcat (temp, "-");
if (flags & 0x20)
   strcat (temp, "S"); /* Secret (normal on the map) (Se) */
else
   strcat (temp, "-");
if (flags & 0x10)
   strcat (temp, "L"); /* Lower texture offset changed (Lo) */
else
   strcat (temp, "-");
if (flags & 0x08)
   strcat (temp, "U"); /* Upper texture offset changed (Up) */
else
   strcat (temp, "-");
if (flags & 0x04)
   strcat (temp, "2"); /* Two-sided (2S) */
else
   strcat (temp, "-");
if (flags & 0x02)
   strcat (temp, "M"); /* Monsters can't cross this line (Mo) */
else
   strcat (temp, "-");
if (flags & 0x01)
   strcat (temp, "I"); /* Impassible (Im) */
else
   strcat (temp, "-");
if (strlen (temp) > 13)
{
   temp[13] = '|';
   temp[14] = '\0';
}
return temp;
#endif
}



/*
   get a long description of one linedef flag
*/

const char *GetLineDefFlagsLongName (int flags)
{
if (flags & 0x1000) return "Translucent [Strife]";
if (flags & 0x200)  return "Pass-through [Boom]";
if (flags & 0x100)  return "Always shown on the map";
if (flags & 0x80)   return "Never shown on the map";
if (flags & 0x40)   return "Blocks sound";
if (flags & 0x20)   return "Secret (shown as normal on the map)";
if (flags & 0x10)   return "Lower texture is \"unpegged\"";
if (flags & 0x08)   return "Upper texture is \"unpegged\"";
if (flags & 0x04)   return "Two-sided (may be transparent)";
if (flags & 0x02)   return "Monsters cannot cross this line";
if (flags & 0x01)   return "Impassible";
return "UNKNOWN";
}



/*
   get a short (14 char.) description of the type of a sector
*/

const char *GetSectorTypeName (int type)
{
/* KLUDGE: To avoid the last element which is bogus */
for (int n = 0; n < (int)stdef.size()-1; n++)
  if (stdef[n]->number == type)
    return stdef[n]->shortdesc;
static char buf[30];
sprintf (buf, "UNKNOWN (%d)", type);
return buf;
}



/*
   get a long description of the type of a sector
*/

const char *GetSectorTypeLongName (int type)
{
/* KLUDGE: To avoid the last element which is bogus */

for (int n = 0; n < (int)stdef.size()-1; n++)
  if (stdef[n]->number == type)
    return stdef[n]->longdesc;
static char buf[30];
sprintf (buf, "UNKNOWN (%d)", type);
return buf;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
