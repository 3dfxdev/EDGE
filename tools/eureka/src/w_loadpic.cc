/*
 *  w_loadpic.cc
 *  Loading Doom-format pictures from a wad file.
 *  See the Unofficial Doom Specs, section [5-1].
 *  AYM 1998-??-??
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
#include "im_gamecol.h"  /* colour0 */
#include "w_loadpic.h"
#include "w_file.h"
#include "w_io.h"


typedef enum { _MT_BADOFS, _MT_TOOLONG, _MT_TOOMANY } _msg_type_t;


static int add_msg (char type, int arg);
static void do_add_msg (char type, int arg);
static void flush_msg (const char *picname);


/*
 *  LoadPicture - read a picture from a wad file into an Img object
 *
 *  If img->is_null() is false, LoadPicture() does not allocate the
 *  buffer itself. The buffer and the picture don't have to have the
 *  same dimensions. Thanks to this, it can also be used to compose
 *  textures : you allocate a single buffer for the whole texture
 *  and then you call LoadPicture() on it once for each patch.
 *  LoadPicture() takes care of all the necessary clipping.
 *
 *  If img->is_null() is true, LoadPicture() sets the size of img
 *  to match that of the picture. This is useful in display_pic().
 *
 *  Return 0 on success, non-zero on failure.
 *
 *  If pic_x_offset == INT_MIN, the picture is centred horizontally.
 *  If pic_y_offset == INT_MIN, the picture is centred vertically.
 */

int LoadPicture (
   Img& img,      // Game image to load picture into
   const char *picname,   // Picture lump name
   const Lump_loc& picloc,  // Picture lump location
   int pic_x_offset,    // Coordinates of top left corner of picture
   int pic_y_offset,    // relative to top left corner of buffer
   int *pic_width,    // To return the size of the picture
   int *pic_height)   // (can be NULL)
{
  MDirPtr dir;
  s16_t pic_width_;
  s16_t pic_height_;
  s16_t pic_intrinsic_x_ofs;
  s16_t pic_intrinsic_y_ofs;
  byte  *ColumnData;
  byte  *Column;
  s32_t *NeededOffsets;
  s32_t CurrentOffset;
  int ColumnInMemory;
  long  ActualBufLen;
  int pic_x;
  int pic_x0;
  int pic_x1;
  int pic_y0;
  int pic_y1;
  byte      *buf; /* This variable is set to point to the element of
                     the image buffer where the top of the current column
                     should be pasted. It can be off the image buffer! */

  // Locate the lump where the picture is
  if (picloc.wad != 0)
  {
    MasterDirectory dirbuf;
    dirbuf.wadfile   = picloc.wad;
    dirbuf.dir.start = picloc.ofs;
    dirbuf.dir.size  = picloc.len;
    dir = &dirbuf;
  }
  else
  {
    dir = (MDirPtr) FindMasterDir (MasterDir, picname);
    if (dir == NULL)
    {
      warn ("picture %.*s does not exist.\n", WAD_PIC_NAME, picname);
      return 1;
    }
  }

  // Read the picture header
  dir->wadfile->seek (dir->dir.start);
  if (dir->wadfile->error ())
  {
    warn ("picture %.*s: can't seek to header, giving up\n",
        WAD_PIC_NAME, picname);
    return 1;
  }
  bool dummy_bytes  = dir->wadfile->pic_format () == YGPF_NORMAL;
  bool long_header  = dir->wadfile->pic_format () != YGPF_ALPHA;
  bool long_offsets = dir->wadfile->pic_format () == YGPF_NORMAL;
  if (long_header)
  {
    dir->wadfile->read_i16 (&pic_width_);
    dir->wadfile->read_i16 (&pic_height_);
    dir->wadfile->read_i16 (&pic_intrinsic_x_ofs);  // Read but ignored
    dir->wadfile->read_i16 (&pic_intrinsic_y_ofs);  // Read but ignored
    if (dir->wadfile->error ())
    {
      warn ("picture %.*s: read error in header, giving up\n",
          WAD_PIC_NAME, picname);
      return 1;
    }
  }
  else
  {
    pic_width_          = dir->wadfile->read_u8 ();
    pic_height_         = dir->wadfile->read_u8 ();
    pic_intrinsic_x_ofs = dir->wadfile->read_u8 ();  // Read but ignored
    pic_intrinsic_y_ofs = dir->wadfile->read_u8 ();  // Read but ignored
    if (dir->wadfile->error ())
    {
      warn ("picture %.*s: read error in header, giving up\n",
          WAD_PIC_NAME, picname);
      return 1;
    }
  }

  // If no buffer given by caller, allocate one.
  if (img.is_null ())
  {
    // Sanity checks
    if (pic_width_  < 1 || pic_height_ < 1)
    {
      warn ("picture %.*s: delirious dimensions %dx%d, giving up\n",
          WAD_PIC_NAME, picname, (int) pic_width_, (int) pic_height_);
    }
    const int pic_width_max = 4096;
    if (pic_width_ > pic_width_max)
    {
      warn ("picture %.*s: too wide (%d), clipping to %d\n",
          WAD_PIC_NAME, picname, (int) pic_width_, pic_width_max);
      pic_width_ = pic_width_max;
    }
    const int pic_height_max = 4096;
    if (pic_height_ > pic_height_max)
    {
      warn ("picture %.*s: too high (%d), clipping to %d\n",
          WAD_PIC_NAME, picname, (int) pic_height_, pic_height_max);
      pic_height_ = pic_height_max;
    }
    img.resize (pic_width_, pic_height_);
  }
  int img_width = img.width ();

  // Centre the picture.
  if (pic_x_offset == INT_MIN)
    pic_x_offset = (img_width - pic_width_) / 2;
  if (pic_y_offset == INT_MIN)
    pic_y_offset = (img.height () - pic_height_) / 2;

  /* AYM 19971202: 17 kB is large enough for 128x128 patches. */
#define TEX_COLUMNBUFFERSIZE ((long) 17 * 1024)
  /* Maximum number of bytes per column. The worst case is a
     509-high column, with every second pixel transparent. That
     makes 255 posts of 1 pixel, and a final FFh. The total is
     (255 x 5 + 1) = 1276 bytes per column. */
#define TEX_COLUMNSIZE  1300

  ColumnData    = (byte *) GetMemory (TEX_COLUMNBUFFERSIZE);
  /* FIXME DOS and pic_width_ > 16000 */
  NeededOffsets = (s32_t *) GetMemory ((long) pic_width_ * 4);

  if (long_offsets)
    dir->wadfile->read_i32 (NeededOffsets, pic_width_);
  else
    for (int n = 0; n < pic_width_; n++)
    {
      s16_t ofs;
      dir->wadfile->read_i16 (&ofs);
      NeededOffsets[n] = ofs;
    }
  if (dir->wadfile->error ())
  {
    warn ("picture %.*s: read error in offset table, giving up\n",
        WAD_PIC_NAME, picname);
    FreeMemory (ColumnData);
    FreeMemory (NeededOffsets);
    return 1;
  }

  // Read first column data, and subsequent column data
  if (long_offsets && NeededOffsets[0] != 8 + (long) pic_width_ * 4
      || !long_offsets && NeededOffsets[0] != 4 + (long) pic_width_ * 2)
  {
    dir->wadfile->seek (dir->dir.start + NeededOffsets[0]);
    if (dir->wadfile->error ())
    {
      warn ("picture %.*s: can't seek to header, giving up\n",
          WAD_PIC_NAME, picname);
      FreeMemory (ColumnData);
      FreeMemory (NeededOffsets);
      return 1;
    }
  }
  ActualBufLen = dir->wadfile->read_vbytes (ColumnData, TEX_COLUMNBUFFERSIZE);
  // FIXME should catch I/O errors

  // Clip the picture horizontally and vertically
  pic_x0 = - pic_x_offset;
  if (pic_x0 < 0)
    pic_x0 = 0;

  pic_x1 = img_width - pic_x_offset - 1;
  if (pic_x1 >= pic_width_)
    pic_x1 = pic_width_ - 1;

  pic_y0 = - pic_y_offset;
  if (pic_y0 < 0)
    pic_y0 = 0;

  pic_y1 = img.height () - pic_y_offset - 1;
  if (pic_y1 >= pic_height_)
    pic_y1 = pic_height_ - 1;

  // For each (non clipped) column of the picture...
  for (pic_x = pic_x0,
      buf = img.wbuf () + al_amax (pic_x_offset, 0) + img_width * pic_y_offset;
      pic_x <= pic_x1;
      pic_x++, buf++)
  {
    byte *filedata;

    CurrentOffset  = NeededOffsets[pic_x];
    ColumnInMemory = CurrentOffset >= NeededOffsets[0]
      && CurrentOffset + TEX_COLUMNSIZE <= NeededOffsets[0] + ActualBufLen;
    if (ColumnInMemory)
      Column = ColumnData + CurrentOffset - NeededOffsets[0];
    else
    {
      Column = (byte *) GetFarMemory (TEX_COLUMNSIZE);
      dir->wadfile->seek (dir->dir.start + CurrentOffset);
      if (dir->wadfile->error ())
      {
        int too_many = add_msg (_MT_BADOFS, (short) pic_x);
        FreeFarMemory (Column);
        if (too_many)    // This picture has too many errors. Give up.
          goto pic_end;
        continue;      // Give up on this column
      }
      dir->wadfile->read_vbytes (Column, TEX_COLUMNSIZE);
      // FIXME should catch I/O errors
    }
    filedata = Column;

    // We now have the needed column data, one way or another, so write it

    // For each post of the column...
    {
      byte *post;
      for (post = filedata; *post != 0xff;)
      {
        int post_y_offset;  // Y-offset of top of post to origin of buffer
        int post_height;    // Height of post
        int post_pic_y0;    // Start and end of non-clipped part of post,
        int post_pic_y1;    // relative to top of picture
        int post_y0;    // Start and end of non-clipped part of post,
        int post_y1;    // relative to top of post

        if (post - filedata > TEX_COLUMNSIZE)
        {
          int too_many = add_msg (_MT_TOOLONG, (short) pic_x);
          if (too_many)    // This picture has too many errors. Give up.
          {
            if (! ColumnInMemory)
              FreeFarMemory (Column);
            goto pic_end;
          }
          break;       // Give up on this column
        }

        post_y_offset = *post++;
        post_height = *post++;
        if (dummy_bytes)
          post++;      // Skip that dummy byte

        post_pic_y0 = post_y_offset;  // Clip the post vertically
        if (post_pic_y0 < pic_y0)
          post_pic_y0 = pic_y0;

        post_pic_y1 = post_y_offset + post_height - 1;
        if (post_pic_y1 > pic_y1)
          post_pic_y1 = pic_y1;

        post_y0 = post_pic_y0 - post_y_offset;
        post_y1 = post_pic_y1 - post_y_offset;


        {         // "Paste" the post onto the buffer
          img_pixel_t *b;
          const byte *p          = post + post_y0;
          const byte *const pmax = post + post_y1;
          int buf_width = img_width;

          for (b = buf + buf_width * (post_y_offset + post_y0);
              p <= pmax;
              b += buf_width, p++)
          {
#ifdef PARANOIA
            if (b < img.buf ())
            {
              nf_bug ("Picture %.*s(%d): b < buffer",
                  WAD_PIC_NAME, picname, (int) pic_x);
              goto next_column;
            }
#endif
            *b = (*p == IMG_TRANSP) ? colour0 : *p;
          }
        }

        post += post_height;
        if (dummy_bytes)
          post++;      // Skip the trailing dummy byte
      }  // Post loop
    }

#ifdef PARANOIA
next_column :
#endif
    if (!ColumnInMemory)
      FreeFarMemory (Column);
  }  // Column loop

pic_end:
  FreeMemory (ColumnData);
  FreeMemory (NeededOffsets);
  flush_msg (picname);
#if 0
  c->flags |= HOOK_DRAWN;
#endif
  if (pic_width)
    *pic_width  = pic_width_;
  if (pic_height)
    *pic_height = pic_height_;
  return 0;
}


/*
 *  List to hold pending warning messages
 */
typedef struct
{
  char type;
  short arg;
} _msg_t;
static _msg_t *_msg_list  = 0;
static size_t _nmsg       = 0;
const size_t _granularity = 128;
const size_t _max_msg     = 20;


/*
 *  add_msg
 *  Add a warning message to the list
 *
 *  Return 0 on success, <>0 if the max number of messages
 *  has been reached.
 */
static int add_msg (char type, int arg)
{
  if (_nmsg >= _max_msg)
  {
    if (_nmsg == _max_msg)  // Test in case the caller ignores our return value
      do_add_msg (_MT_TOOMANY, arg);
    return 1;
  }
  do_add_msg (type, arg);
  return 0;
}


static void do_add_msg (char type, int arg)
{
  if ((_nmsg + 1) % _granularity == 1)  // Grow list if necessary
  {
    _msg_t *new_list = (_msg_t *) realloc (_msg_list,
        (_nmsg / _granularity + 1) * _granularity * sizeof *_msg_list);
    if (new_list == 0)  // Not enough memory ? Ignore the new message
      return;
    _msg_list = new_list;
  }
  _msg_list[_nmsg].type = type;
  _msg_list[_nmsg].arg  = arg;
  _nmsg++;
  return;
}


/*
 *  flush_msg
 *  Output all pending warning messages in an smart fashion
 */
static void flush_msg (const char *picname)
{
  if (_nmsg == 0 || _msg_list == 0)
    return;

  for (_msg_type_t t = _MT_BADOFS; t <= _MT_TOOLONG; ((int &) t)++)
  {
    int first_msg = (1 << 30);
    int last_msg = (1 << 30);
    const char *str = "unknown error";
    if (t == _MT_BADOFS)
      str = "bad file offset";
    else if (t == _MT_TOOLONG)
      str = "post too long";

    for (size_t n = 0; n < _nmsg; n++)
    {
      if (_msg_list[n].type == t)
      {
        if (first_msg == (1 << 30))
        {
          first_msg = n;
          last_msg = n;
        }
        else
        {
          if (_msg_list[last_msg].arg != _msg_list[n].arg - 1)
          {
            warn ("picture %.*s(%d",
                WAD_PIC_NAME, picname, (int) _msg_list[first_msg].arg);
            if (last_msg != first_msg)
              warn ("-%d", (int) _msg_list[last_msg].arg);
            warn ("): %s. Corrupt wad ?\n", str);
            first_msg = n;
            last_msg = n;
          }
          else
            last_msg = n;
        }
      }
    }
    if (first_msg != (1 << 30))
    {
      warn ("picture %.*s(%d",
          WAD_PIC_NAME, picname, (int) _msg_list[first_msg].arg);
      if (last_msg != first_msg)
        warn ("-%d", (int) _msg_list[last_msg].arg);
      warn ("): %s. Corrupt wad ?\n", str);
    }
  }

  if (_msg_list[_nmsg - 1].type == _MT_TOOMANY)
    warn ("picture %.*s: too many errors. Giving up.\n", WAD_PIC_NAME, picname);
  _nmsg = 0;
  free (_msg_list);
  _msg_list = 0;
}



