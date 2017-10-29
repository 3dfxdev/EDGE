//----------------------------------------------------------------------------
//  Rise of the Triad Local Defines
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2016 Isotope SoftWorks
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
//----------------------------------------------------------------------------

#ifndef __ROTT_LOCAL_H__
#define __ROTT_LOCAL_H__

//#include "rott_data.h"
#include "rott_defs.h"

// WinROTT specific ----->
#include "tile.h"
#include "rott_resource.h"
// <----- END WINROTT specific

#include "../src/r_defs.h"

#include "../epi/image_data.h"

#include "../src/w_wad.h" ///WAD handling.

// --- globals ---

#define MAPSIZE 128	// maps are 64*64 max

// -- R_ROTTGFX.CC --
int CntNbOfNotBGbyteu8_ts(u8_t *src, int w, int h);
int CntNbOfBGbyteu8_ts(u8_t *src, int w, int h);
u8_t makecol(u8_t r, u8_t g, u8_t b);
int ConvertMaskedRottSpriteIntoPCX(u8_t  *RottSpriteMemptr, u8_t  *RottPCXptr);
int ConvertRottSpriteIntoPCX(u8_t  *RottSpriteMemptr, u8_t  *RottPCXptr);
void ConvertPicToPPic(int width, int height, u8_t *source, u8_t *target);
void ConvertPPicToPic(int width, int height, u8_t *src, u8_t *tgt);
int  ConvertPCXIntoRottSprite(u8_t  *RottSpriteMemptr, u8_t*srcdata, int lump, int lumplength, int origsize, int w, int h, int leftoffset, int topoffset);
void RotatePicture(u8_t *src, int iWidth, int  iHeight);
int ConvertPCXIntoRottTransSprite(u8_t  *RottSpriteMemptr, u8_t*srcdata, int lump, int lumplength, int origsize, int h, int w, int leftoffset, int topoffset, int translevel);
int ConvertPCXIntoRott_pic_t(u8_t  *RottSpriteMemptr, u8_t*srcdata, int lump, int lumplength, int h, int w);
void loadconvpal();
void FlipPictureUpsideDown(int width, int height, u8_t *src);
void ConvertPicToPPic(int width, int height, u8_t *source, u8_t *target);
int GetRightShiftCount(DWORD dwVal);
int GetLeftShiftCount(DWORD dwVal);
void ShrinkMemPictureExt(int orgw, int orgh, int neww, int newy, u8_t* src, u8_t * target);


#endif // __ROTT_LOCAL_H__