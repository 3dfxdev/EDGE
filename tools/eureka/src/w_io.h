//------------------------------------------------------------------------
//  WAD I/O
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


#ifndef YH_WADS  /* DO NOT INSERT ANYTHING BEFORE THIS LINE */
#define YH_WADS


int  file_read_i16    (FILE *,  s16_t *buf, long count = 1);
int  file_read_i32    (FILE *,  s32_t *buf, long count = 1);
long file_read_vbytes (FILE *, void *buf, long count);
int  file_read_bytes  (FILE *, void *buf, long count);
void file_write_i16   (FILE *,  s16_t buf);
void file_write_i32   (FILE *,  s32_t buf, long count = 1);
void file_write_name  (FILE *, const char *name);
void WriteBytes       (FILE *, const void *, long);
int  copy_bytes       (FILE *dest, FILE *source, long size);



#if 0
/*
 *  flat_name_cmp
 *  Compare two flat names like strcmp() compares two strings.
 */
inline int flat_name_cmp (const char *name1, const char *name2)
{
}


/*
 *  tex_name_cmp
 *  Compare two texture names like strcmp() compares two strings.
 *  T
 */
inline int tex_name_cmp (const char *name1, const char *name2)
{
}


/*
 *  sprite_name_cmp
 *  Compare two sprite names like strcmp() compares two strings.
 */
inline int sprite_name_cmp (const char *name1, const char *name2)
{
}
#endif


#endif  /* DO NOT ADD ANYTHING AFTER THIS LINE */
//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
