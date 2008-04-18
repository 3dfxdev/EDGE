//------------------------------------------------------------------------
//  Basic image storage
//------------------------------------------------------------------------
// 
//  Copyright (c) 2008  Andrew J Apted
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

#include "headers.h"

#include "im_image.h"


rgb_image_c::rgb_image_c(int _w, int _h) : width(_w), height(_h), is_solid(false)
{
  pixels = new u32_t[width * height];
}

rgb_image_c::~rgb_image_c()
{
  delete[] pixels;

  pixels = NULL;
  width = height = 0;
}


void rgb_image_c::Clear()
{
  memset(pixels, 0, width * height * sizeof(u32_t));
  
  is_solid = false;
}


rgb_image_c * rgb_image_c::Duplicate() const
{
  rgb_image_c *copy = new rgb_image_c(width, height);

  memcpy(copy->pixels, pixels, width * height * sizeof(u32_t));

  return copy;
}


void rgb_image_c::SwapEndian()
{
  u32_t *pos = pixels;
  u32_t *p_end = pos + (width * height);

  for (; pos < p_end; pos++)
  {
    *pos = UT_Swap32(*pos);
  }
}


void rgb_image_c::Mirror()
{
  for (int y=0; y < height; y++)
  {
    for (int x=0; x < width/2; x++)
    {
      int x2 = width - 1 - x;

      u32_t tmp     = PixelAt(x, y);
      PixelAt(x, y) = PixelAt(x2, y);
      PixelAt(x2,y) = tmp;
    }
  }
}


void rgb_image_c::Invert()
{
  u32_t *buffer = new u32_t[width + 1];

  for (int y=0; y < height/2; y++)
  {
    int y2 = height - 1 - y;

    memcpy(buffer,          &PixelAt(0, y),  width * sizeof(u32_t));
    memcpy(&PixelAt(0, y),  &PixelAt(0, y2), width * sizeof(u32_t));
    memcpy(&PixelAt(0, y2), buffer,          width * sizeof(u32_t));
  }

  delete[] buffer;
}


void rgb_image_c::RemoveAlpha()
{
  u32_t *pos   = pixels;
  u32_t *p_end = pos + (width * height);

  while (pos < p_end)
  {
    int a = 255 - RGB_A(*pos);

    u8_t r = RGB_R(*pos) * a / 255;
    u8_t g = RGB_G(*pos) * a / 255;
    u8_t b = RGB_B(*pos) * a / 255;

    *pos++ = MAKE_RGB(r, g, b);
  }
}


void rgb_image_c::ThresholdAlpha(u8_t alpha)
{
  u32_t *pos   = pixels;
  u32_t *p_end = pos + (width * height);

  for (; pos < p_end; pos++)
  {
    if (RGB_A(*pos) < alpha)
      *pos &= 0x00FFFFFFU;
    else
      *pos |= 0xFF000000U;
  }
}


void rgb_image_c::UpdateSolid()
{
  is_solid = true;

  const u32_t *pos   = pixels;
  const u32_t *p_end = pos + (width * height);

  for (; pos < p_end; pos++)
  {
    if (RGB_A(*pos) != ALPHA_SOLID)
    {
      is_solid = false;
      break;
    }
  }
}


rgb_image_c * rgb_image_c::ScaledDup(int new_w, int new_h)
{
  rgb_image_c *copy = new rgb_image_c(new_w, new_h);

  for (int y = 0; y < new_h; y++)
  for (int x = 0; x < new_w; x++)
  {
    int old_x = x * width  / new_w;
    int old_y = y * height / new_h;

    copy->PixelAt(x, y) = PixelAt(old_x, old_y);
  }
  
  return copy;
}


rgb_image_c * rgb_image_c::NiceMip()
{
  SYS_ASSERT((width  & 1) == 0);
  SYS_ASSERT((height & 1) == 0);

  int new_w = width  / 2;
  int new_h = height / 2;

  rgb_image_c *copy = new rgb_image_c(new_w, new_h);

  for (int y = 0; y < new_h; y++)
  for (int x = 0; x < new_w; x++)
  {
    int R = 0, G = 0, B = 0, A = 0;

    for (int dy = 0; dy < 2; dy++)
    for (int dx = 0; dx < 2; dx++)
    {
      u32_t cur = PixelAt(x*2+dx, y*2+dy);

      R += RGB_R(cur); G += RGB_G(cur);
      B += RGB_B(cur); A += RGB_A(cur);
    }

    R /= 4; G /= 4;
    B /= 4; A /= 4;

    copy->PixelAt(x, y) = MAKE_RGBA(R, G, B, A);
  }

  return copy;
}


//--- editor settings ---
// vi:ts=2:sw=2:expandtab
