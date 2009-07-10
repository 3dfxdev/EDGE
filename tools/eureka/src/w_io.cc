/*
 *  wads.cc
 *  Wad file routines
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
#include "w_io.h"


MDirPtr MasterDir = NULL;   // The master directory


/*
 *  file_read_i16 - read little-endian 16-bit signed integers from a file
 *
 *  Return 0 on success, non-zero on failure.
 */
int file_read_i16 (FILE *fp, s16_t *buf, long count)
{
  while (count-- > 0)
    *buf = getc (fp) | (getc (fp) << 8);
  return feof (fp) || ferror (fp);
}


/*
 *  file_read_i32 - read little-endian 32-bit signed integers from a file
 *
 *  Return 0 on success, non-zero on failure.
 */
int file_read_i32 (FILE *fp, s32_t *buf, long count)
{
  while (count-- > 0)
  {
    *buf++ = 
        ((s32_t) getc (fp))
      | ((s32_t) getc (fp) << 8)
      | ((s32_t) getc (fp) << 16)
      | ((s32_t) getc (fp) << 24);
  }

  return feof (fp) || ferror (fp);
}


/*
 *  file_read_vbytes - read bytes from file
 *
 *  Return the number of bytes read.
 */
long file_read_vbytes (FILE *fp, void *buf, long count)
{
  long bytes_read_total;
  size_t bytes_read;
  size_t bytes_to_read;

  bytes_read_total = 0;
  bytes_to_read    = 0x8000;
  while (count > 0)
  {
    if (count <= 0x8000)
      bytes_to_read = (size_t) count;
    bytes_read = fread (buf, 1, bytes_to_read, fp);
    bytes_read_total += bytes_read;
    if (bytes_read != bytes_to_read)
      break;
    buf = (char *) buf + bytes_read;
    count -= bytes_read;
  }
  return bytes_read_total;
}


/*
 *  file_read_bytes - read bytes from a file
 *
 *  Return 0 on success, non-zero on failure.
 */
int file_read_bytes (FILE *fp, void *buf, long count)
{
  return file_read_vbytes (fp, buf, count) != count;
}


/*
 *  file_write_i16 - write a little-endian 16-bit signed integer to a file
 *
 *  Does no error checking.
 */
void file_write_i16 (FILE *fd, s16_t buf)
{
  putc (       buf & 0xff, fd);
  putc ((buf >> 8) & 0xff, fd);
}


/*
 *  file_write_i32 - write little-endian 32-bit signed integers to a file
 *
 *  Does no error checking.
 */
void file_write_i32 (FILE *fd, s32_t buf, long count)
{
  while (count-- > 0)
  {
    putc ((buf      ) & 0xff, fd);
    putc ((buf >>  8) & 0xff, fd);
    putc ((buf >> 16) & 0xff, fd);
    putc ((buf >> 24) & 0xff, fd);
  }
}


/*
 *  file_write_name - write directory entry name to file
 *
 *  Write to file <fd> the directory entry name contained in
 *  <name>. The string written in the file is exactly the
 *  same as the string contained in <name> except that :
 *
 *  - only the first WAD_NAME characters of <name> are
 *    used, or up to the first occurrence of a NUL,
 *    
 *  - all letters are forced to upper case,
 *
 *  - if necessary, the string is padded to WAD_NAME
 *    characters with NULs.
 *
 *  Does no error checking.
 */
void file_write_name (FILE *fd, const char *name)
{
  const unsigned char *const p0 = (const unsigned char *) name;
  const unsigned char *p = p0;  // "unsigned" for toupper()'s sake

  for (; p - p0 < (ptrdiff_t) WAD_NAME && *p; p++)
    putc (toupper (*p), fd);
  for (; p - p0 < (ptrdiff_t) WAD_NAME; p++)
    putc ('\0', fd);
}


/*
   find an entry in the master directory
*/

MDirPtr FindMasterDir (MDirPtr from, const char *name)
{
  while (from)
  {
    if (! y_strnicmp (from->dir.name, name, WAD_NAME))
      break;
    from = from->next;
  }
  return from;
}


/*
 *  Find an entry in the master directory
 */
MDirPtr FindMasterDir (MDirPtr from, const char *name1, const char *name2)
{
  while (from)
    {
    if (! y_strnicmp (from->dir.name, name1, WAD_NAME)
     || ! y_strnicmp (from->dir.name, name2, WAD_NAME))
      break;
    from = from->next;
    }
  return from;
}


/*
   output bytes to a binary file with error checking
*/

void WriteBytes (FILE *file, const void *buf, long size)
{
  if (! Registered)
    return;
  while (size > 0x8000)
  {
    if (fwrite (buf, 1, 0x8000, file) != 0x8000)
      fatal_error ("error writing to file");
    buf = (const char *) buf + 0x8000;
    size -= 0x8000;
  }
  if (fwrite (buf, 1, size, file) != (size_t) size)
    fatal_error ("error writing to file");
}


/*
 *  copy_bytes - copy bytes from a binary file to another
 *
 *  FIXME it's silly to allocate such a large buffer on
 *  memory constrained systems. The function should be able
 *  to fall back on a smaller buffer.
 *
 *  Return 0 on success, 1 if there was a read error on
 *  source file, 2 if there was a write error on destination
 *  file.
 */
int copy_bytes (FILE *dest, FILE *source, long size)
{
  int          rc      = 0;
  void        *data    = 0;
  const size_t chunksz = 0x4000;

  data = GetMemory (chunksz + 2);
  while (size > chunksz)
  {
    if (fread (data, 1, chunksz, source) != chunksz)
    {
      rc = 1;
      goto byebye;
    }
    if (fwrite (data, 1, chunksz, dest) != chunksz)
    {
      rc = 2;
      goto byebye;
    }
    size -= chunksz;
  }
  if (fread (data, 1, size, source) != (size_t) size)
  {
    rc = 1;
    goto byebye;
  }
  if (fwrite (data, 1, size, dest) != (size_t) size)
  {
    rc = 2;
    goto byebye;
  }

byebye:
  if (data != 0)
    FreeMemory (data);
  return rc;
}

