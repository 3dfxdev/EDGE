//----------------------------------------------------------------------------
//  EDGE Rendering Data Handling Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2001  The EDGE Team.
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
//
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------
//
// -ACB- 1998/09/09 Reformatted File Layout.
// -KM- 1998/09/27 Colourmaps can be dynamically changed.
// -ES- 2000/02/12 Moved most of this module to w_texture.c.

#include "i_defs.h"
#include "r_data.h"

#include "e_search.h"
#include "dm_state.h"
#include "dm_defs.h"
#include "m_argv.h"
#include "m_misc.h"
#include "m_swap.h"
#include "p_local.h"
#include "r_sky.h"
#include "w_wad.h"
#include "w_textur.h"
#include "z_zone.h"

//
// R_AddFlatAnim
//
// Here are the rules for flats, they get a bit hairy, but are the
// simplest thing which achieves expected behaviour:
//
// 1. When two flats in different wads have the same name, the flat
//    in the _later_ wad overrides the flat in the earlier wad.  This
//    allows pwads to replace iwad flats -- as is usual.  For general
//    use of flats (e.g. in levels) their order is not an issue.
//
// 2. The flat animation sequence is determined by the _earliest_ wad
//    which contains _both_ the start and the end flat.  The sequence
//    contained in that wad becomes the animation sequence (the list
//    of flat names).  These names are then looked up normally, so
//    flats in newer wads will get used if their name matches one in
//    the sequence.
//
// Note that via rule 2, the _earliest_ wad sets the animation
// sequence.  HMMMMM !!
// 
// -AJA- 2001/01/28: reworked flat animations.
// 
void R_AddFlatAnim(animdef_t *anim)
{
  int start = W_CheckNumForName(anim->startname);
  int end   = W_CheckNumForName(anim->endname);

  int file;
  int s_offset, e_offset;
  const int *lumps;
  int total;

  int i;
  const image_t **flats;
  
  if (start == -1 || end == -1)
  {
    // sequence not valid.  Maybe it is the DOOM 1 IWAD.
    return;
  }

  file = W_FindFlatSequence(anim->startname, anim->endname, 
      &s_offset, &e_offset);

  if (file < 0)
  {
    I_Warning("Missing flat animation: %s-%s not in any wad.\n",
        anim->startname, anim->endname);
    return;
  }

  DEV_ASSERT2(s_offset <= e_offset);

  lumps = W_GetListLumps(file, LMPLST_Flats, &total);

  // determine animation sequence
  lumps += s_offset;
  total = e_offset - s_offset + 1;

  flats = Z_New(const image_t *, total);

  // lookup each flat
  for (i=0; i < total; i++)
  {
    const char *name = W_GetLumpName(lumps[i]);

    // Note we use W_ImageFromFlat() here.  It might seem like a good
    // optimisation to use the lump number directly, but we can't do
    // that -- the lump list does NOT take overriding flats (in newer
    // pwads) into account.
    
    flats[i] = W_ImageFromFlat(name);
  }

  W_AnimateImageSet(flats, total, anim->speed);
  Z_Free(flats);
}

//
// R_AddTextureAnim
//
// Here are the rules for textures:
//
// 1. The TEXTURE1/2 lumps require a PNAMES lump to complete their
//    meaning.  Some wads have the TEXTURE1/2 lump(s) but lack a
//    PNAMES lump -- in this case the next oldest PNAMES lump is used
//    (e.g. the one in the IWAD).
// 
// 2. When two textures in different wads have the same name, the
//    texture in the _later_ wad overrides the one in the earlier wad,
//    as is usual.  For general use of textures (e.g. in levels),
//    their ordering is not an issue.
//
// 3. The texture animation sequence is determined by the _latest_ wad
//    whose TEXTURE1/2 lump contains _both_ the start and the end
//    texture.  The sequence within that lump becomes the animation
//    sequence (the list of texture names).  These names are then
//    looked up normally, so textures in newer wads can get used if
//    their name matches one in the sequence.
// 
// -AJA- 2001/06/17: reworked texture animations.
// 
void R_AddTextureAnim(animdef_t *anim)
{
  int set, s_offset, e_offset;
  
  int i, total;
  const image_t **texs;
 
  set = W_FindTextureSequence(anim->startname, anim->endname,
      &s_offset, &e_offset);

  if (set < 0)
  {
    // sequence not valid.  Maybe it is the DOOM 1 IWAD.
    return;
  }

  DEV_ASSERT2(s_offset <= e_offset);

  total = e_offset - s_offset + 1;
  
  texs = Z_New(const image_t *, total);

  // lookup each texture
  for (i=0; i < total; i++)
  {
    const char *name = W_TextureNameInSet(set, s_offset + i);
    texs[i] = W_ImageFromFlat(name);
  }

  W_AnimateImageSet(texs, total, anim->speed);
  Z_Free(texs);
}

//
// R_InitFlats
// 
bool R_InitFlats(void)
{
  int max_file = W_GetNumFiles();
  int j, file;
  
  int *F_lumps = NULL;
  int numflats = 0;
   
  I_Printf("R_InitFlats...\n");

  // iterate over each file, creating our big array of flats

  for (file=0; file < max_file; file++)
  {
    const int *lumps;
    int lumpnum;

    lumps = W_GetListLumps(file, LMPLST_Flats, &lumpnum);

    if (lumpnum == 0)
      continue;

    Z_Resize(F_lumps, int, numflats + lumpnum);

    for (j=0; j < lumpnum; j++, numflats++)
      F_lumps[numflats] = lumps[j];
  }

  if (numflats == 0)
    I_Error("No flats found !  Make sure the chosen IWAD is valid.\n");

  // now sort the flats, primarily by increasing name, secondarily by
  // increasing lump number (a measure of newness).

#define CMP(a, b)  \
    (strcmp(W_GetLumpName(a), W_GetLumpName(b)) < 0 || \
     (strcmp(W_GetLumpName(a), W_GetLumpName(b)) == 0 && a < b))
  QSORT(int, F_lumps, numflats, CUTOFF);
#undef CMP

  // remove duplicate names.  We rely on the fact that newer lumps
  // have greater lump values than older ones.  Because the QSORT took
  // newness into account, only the last entry in a run of identically
  // named flats needs to be kept.

  for (j=1; j < numflats; j++)
  {
    int a = F_lumps[j - 1];
    int b = F_lumps[j];

    if (strcmp(W_GetLumpName(a), W_GetLumpName(b)) == 0)
      F_lumps[j - 1] = -1;
  }

#if 0  // DEBUGGING
  for (j=0; j < numflats; j++)
  {
    L_WriteDebug("FLAT #%d:  lump=%d  name=[%s]\n", j,
        F_lumps[j], W_GetLumpName(F_lumps[j]));
  }
#endif
  
  W_ImageCreateFlats(F_lumps, numflats); 
  Z_Free(F_lumps);

  return true;
}

//
// R_InitPicAnims
//
// -ACB- 1999/09/25 modified for new bool type
//
bool R_InitPicAnims(void)
{
  int i;

  // loop through animdefs, and add relevant anims.
  // Note: reverse order, give priority to newer anims.
  for (i = numanimdefs-1; i >= 0; i--)
  {
    animdef_t *A = animdefs[i];

    if (A->istexture == true)
      R_AddTextureAnim(A);
    else
      R_AddFlatAnim(A);
  }

  return true;
}

//
// R_PrecacheSprites
//
static void R_PrecacheSprites(void)
{
  int i;
  byte *sprite_present;
  mobj_t *mo;
    
  DEV_ASSERT2(numsprites > 0);

  sprite_present = Z_ClearNew(byte, numsprites);

  for (mo = mobjlisthead; mo; mo = mo->next)
  {
    if (mo->sprite < 0 || mo->sprite >= numsprites)
      continue;

    sprite_present[mo->sprite] = 1;
  }

  for (i=0; i < numsprites; i++)
  {
    spritedef_t *sp = sprites + i;
    int fr, rot;

    const image_t *cur_image;
    const image_t *last_image = NULL;  // an optimisation

    if (! sprite_present[i] || sp->numframes == 0)
      continue;

    DEV_ASSERT2(sp->frames);

    for (fr=0; fr < sp->numframes; fr++)
    {
      if (! sp->frames[fr].finished)
        continue;

      for (rot=0; rot < 16; rot++)
      {
        cur_image = sp->frames[fr].images[rot];

        if (cur_image == NULL || cur_image == last_image)
          continue;

        W_ImagePreCache(cur_image);

        last_image = cur_image;
      }
    }
  }
 
  Z_Free(sprite_present);
}

//
// R_PrecacheLevel
//
// Preloads all relevant graphics for the level.
//
// -AJA- 2001/06/18: Reworked for image system.
//                   
void R_PrecacheLevel(void)
{
  const image_t ** images;

  int max_image;
  int count = 0;
  int i;

  if (demoplayback)
    return;

  // do sprites first -- when memory is tight, they'll be the first
  // images to be removed from the cache(s).
 
  if (M_CheckParm("-fastsprite") || M_CheckParm("-fastsprites"))
  {
    R_PrecacheSprites();
  }
 
  // maximum possible images
  max_image = 1 + 3 * numsides + 2 * numsectors;
   
  images = Z_New(const image_t *, max_image);

  // Sky texture is always present.
  images[count++] = sky_image;

  // add in sidedefs
  for (i=0; i < numsides; i++)
  {
    if (sides[i].top.image)
      images[count++] = sides[i].top.image;
    
    if (sides[i].middle.image)
      images[count++] = sides[i].middle.image;

    if (sides[i].middle.image)
      images[count++] = sides[i].middle.image;
  }

  DEV_ASSERT2(count <= max_image);

  // add in planes
  for (i=0; i < numsectors; i++)
  {
    if (sectors[i].floor.image)
      images[count++] = sectors[i].floor.image;

    if (sectors[i].ceil.image)
      images[count++] = sectors[i].ceil.image;
  }

  DEV_ASSERT2(count <= max_image);

  // Sort the images, so we can ignore the duplicates
  
#define CMP(a, b)  (a < b)
  QSORT(const image_t *, images, count, CUTOFF);
#undef CMP

  for (i=0; i < count; i++)
  {
    DEV_ASSERT2(images[i]);
    
    if (i+1 < count && images[i] == images[i + 1])
      continue;

    if (images[i] == skyflatimage)
      continue;

    W_ImagePreCache(images[i]);
  }

  Z_Free(images);
}
