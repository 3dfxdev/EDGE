//----------------------------------------------------------------------------
//  EDGE Sound Handling Code (ENGINE LEVEL)
//----------------------------------------------------------------------------
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
// -KM- 1998/09/27 Sounds.ddf. nosound is now global.
//   Reduced the pitching a little.
//
// -AJA- 1999/09/10: Made `volume' in various places have the range 0
//       to 255.  Note that value in snd_SfxVolume is still in the
//       range 0 to 15.
//
// -ACB- 1999/09/20 We are assuming nosound temporary like
//
// -ACB- 1999/10/06 Purged music info. This is a sound handling file
//

#include "i_defs.h"
#include "s_sound.h"

#include "dm_defs.h"
#include "dm_state.h"
#include "m_argv.h"
#include "m_random.h"
#include "p_local.h"
#include "w_wad.h"
#include "z_zone.h"

#define DEBUG_SOUND  0

// -KM- 1999/01/31 VSOUND, the speed of sound. Guessed at 500.
#define VSOUND  (500<<16)

// slider-scale volume control
static int soundvolume;

// for everything else
#define MAX_VOLUME 255

#define SLIDER_TO_VOL(v)  \
    (((v) - S_MIN_VOLUME) * MAX_VOLUME / (S_MAX_VOLUME-S_MIN_VOLUME))

// when to clip out sounds
// Does not fit the large outdoor areas. 1600 ** 2
// Distance to origin when sounds should be maxed out.
// This should relate to movement clipping resolution
#define S_CLOSE_DIST2    (S_CLOSE_DIST * S_CLOSE_DIST)
#define S_CLIPPING_DIST2 (S_CLIPPING_DIST * S_CLIPPING_DIST)

// Adjustable by menu.
#define NORM_PRIORITY           64
#define NORM_SEP                128
#define S_STEREO_SWING          96.0f

// If true, sound system is disabled/not working. Changed to false if sound init ok.
boolean_t nosound = false;

typedef struct
{
  sfxinfo_t *sfxinfo; // sound information (if null, channel avail.)
  mobj_t *origin;     // origin of sound
  int orig_vol;       // volume sound was started at (0 to 255).
  int channel;        // handle of the sound being played
  boolean_t paused;   // is sound paused?
  boolean_t looping;  // is sound looping?     -ACB- 2001/01/09 Added
}
playsfx_t;

typedef struct free_origin_s
{
  mobj_t *origin;
  void *block;
  struct free_origin_s *next;
}
free_origin_t;

static free_origin_t *free_queue = NULL;

// playing sfx list
#define PLAYINGSFXLIMIT 64
static playsfx_t playingsfx[PLAYINGSFXLIMIT];
static int playingsfxnum = PLAYINGSFXLIMIT;

// dummy head/tail in sound linked list
static sfxinfo_t sfxcachehead;

// number of sounds currently in cache.
static int numcachedsfx;

// ===================Internal====================
// HELPER Functions
static INLINE int edgemin(int a, int b)
{
  return (a < b) ? a : b;
}

static INLINE int edgemax(int a, int b)
{
  return (a > b) ? a : b;
}

static INLINE int edgemid(int a, int b, int c)
{
  return edgemax(a, edgemin(b, c));
}

// --- Small linking functions ---

//
// InsertAtTail
//
static INLINE void InsertAtTail(sfxinfo_t *sound)
{
  sound->next = &sfxcachehead;
  sound->prev = sfxcachehead.prev;
  sound->next->prev = sound;
  sound->prev->next = sound;
}

//
// InsertAtHead
//
static INLINE void InsertAtHead(sfxinfo_t *sound)
{
  sound->next = sfxcachehead.next;
  sound->prev = &sfxcachehead;
  sound->next->prev = sound;
  sound->prev->next = sound;
}

//
// UnlinkSound
//
static INLINE void UnlinkSound(sfxinfo_t *sound)
{
  sound->next->prev = sound->prev;
  sound->prev->next = sound->next;
  sound->next = NULL;
  sound->prev = NULL;
}

//
// RemoveSoundFromCache
//
static void RemoveSoundFromCache(sfxinfo_t *sound)
{
  int snd_num = sound->normal.sounds[0];

  DEV_ASSERT(sound->next, ("RemoveSoundFromCache: Sound not in cache"));
  UnlinkSound(sound);

  I_UnloadSfx(snd_num);
  numcachedsfx--;
}

//
// GetSfxLumpNum
//
// Retrieve the raw data lump index for a given SFX name.
//
// -KM- 1998/10/29 handles links correctly
// -KM- 1998/12/16 If an sfx doesn't exist, use pistol sound.
// -ACB- 1999/09/20 Renamed to S_GetSfxLumpNum. Moved from I_Sound.C
//
static int GetSfxLumpNum(sfxinfo_t *sfx)
{
  char *name = sfx->lump_name;
  int i;

  i = W_CheckNumForName(name);

  if (!strict_errors && i == -1)
  {
    I_Warning("Unknown sound lump %s, using DSPISTOL.\n", name);
    name = "DSPISTOL";
    i = W_CheckNumForName(name);
  }

  return i;
}

//
// CacheSound
//
// Loads the sound if it isn't already loaded, and moves it to the tail
// of the cache's linked list.  Returns true if successful, or false
// if sound didn't exist.
//
static boolean_t CacheSound(sfxinfo_t *sound)
{
  int snd_num = sound->normal.sounds[0];
  int length, freq;
  boolean_t success;
  const byte *lump;
  int lumpnum;
  const char *error;

  if (sound->next)
  {
    // already cached.
    // unlink it, so we can re-insert it at tail
    UnlinkSound(sound);
  }
  else
  {
    // cache the sound

    // get the lumpnumber
    lumpnum = GetSfxLumpNum(sound);

    if (lumpnum < 0)
      return false;

    // Cache the sound data
    lump = (const byte*)W_CacheLumpNum(lumpnum);

    freq   = lump[2] + (lump[3] << 8);
    length = W_LumpLength(lumpnum) - 8;

    // Load the sound effect. Jump over the sound header
    success = I_LoadSfx(lump + 8, length, freq, snd_num);

    // the lump is particularly useless, since it won't be needed until
    // the sound itself has been flushed. It should be flushed sometime
    // before the sound, so why not just flush it as early as possible.
    W_DoneWithLump_Flushable(lump);

    if (!success)
    {
      error = I_SoundReturnError();
      I_Warning("%s\n", error);
    }

    numcachedsfx++;
  }

  InsertAtTail(sound);

  return true;
}

//
// AdjustSoundParams
//
// Changes volume, and stereo-separation
// from the norm of a sound effect to be played.
//
// If the sound is not audible, returns a 0.
// Otherwise, modifies parameters and returns 1.
// The 2s here mean squared, eg S_CLOSE_DIST2 is S_CLOSE_DIST squared.
//
// -AJA- 1999/09/10: made static, and updated for volume in range
//       from 0 to 255.
//
// -AJA- 2000/04/21: max_distance for sounds.ddf, and 3D distance.
//
static int AdjustSoundParams(sfxinfo_t *sfx, mobj_t *listener, 
    mobj_t *source, int *vol, int *sep)
{
  flo_t approx_dist;
  flo_t adx, ady, adz;
  angle_t angle;

  // calculate the distance to sound origin
  //  and clip it if necessary
  adx = fabs(listener->x - source->x);
  ady = fabs(listener->y - source->y);
  adz = fabs(listener->z - source->z);

  // From _GG1_ p.428. Approx. euclidian distance fast.
  //    approx_dist = adx + ady - ((adx < ady ? adx : ady)>>1);
  // Pythagoras.  Results are optimised:
  // Square Root here cancels with square down there

  approx_dist = adx * adx + ady * ady + adz * adz;

  if (approx_dist > sfx->max_distance * sfx->max_distance)
    return 0;

  // angle of source to listener
  angle = R_PointToAngle(listener->x, listener->y, source->x, source->y);

  angle = angle - listener->angle;

  // stereo separation
  *sep = 128.0f - S_STEREO_SWING * M_Sin(angle);

  // volume calculation
  if (approx_dist > S_CLOSE_DIST2)
  {
    // Kester's Physics Model v1.1
    // -KM- 1998/07/31 Use Full dynamic range
    *vol *= (S_CLIPPING_DIST2 - approx_dist) / 
            (S_CLIPPING_DIST2 - S_CLOSE_DIST2);

    if (*vol > MAX_VOLUME)
      *vol = MAX_VOLUME;
  }

  return (*vol > 0);
}

//
// GetSoundChannel
//
// if none available, return -1.  Otherwise channel #.
//
// -AJA- 1999/09/10: made static.
//
static int GetSoundChannel(mobj_t *origin, sfxinfo_t *sfxinfo)
{
  // channel number to use
  int cnum = -1;
  int i;
  int lowest_priority;

  // Check all channels for singularity test
  if (sfxinfo->singularity)
  {
    for (i = 0; i < playingsfxnum; i++)
    {
      if (!playingsfx[i].sfxinfo)
        continue;

      // only ONE singular sound allowed from each object
      if (playingsfx[i].origin == origin && 
          playingsfx[i].sfxinfo->singularity == sfxinfo->singularity)
      {
        if (playingsfx[i].sfxinfo->precious)
          return -1;

        S_StopChannel(i);
        cnum = i;
        break;
      }
    }
  }

  // Find an open channel
  // -KM- 1998/12/16 New SFX code.
  for (i = 0; i < playingsfxnum; i++)
  {
    if (!playingsfx[i].sfxinfo)
    {
      cnum = i;
      break;
    }
  }

  // None available ?
  if (cnum == -1)
  {
    // Look for the lowest priority sound
    // (which has the highest priority value!).
    lowest_priority = sfxinfo->priority;
    for (i = 0; i < playingsfxnum; i++)
    {
      if (playingsfx[i].sfxinfo->priority >= lowest_priority)
      {
        // if equal priority, prefer to keep precious sounds
        if (playingsfx[i].sfxinfo->precious && 
            playingsfx[i].sfxinfo->priority == lowest_priority)
          continue;

        cnum = i;
        lowest_priority = playingsfx[i].sfxinfo->priority;
        break;
      }
    }

    // This is the lowest priority sound
    if (lowest_priority == sfxinfo->priority)
      return -1;

    // Stop lower priority sound
    S_StopChannel(cnum);
  }

  return cnum;
}

//
// StartSoundAtVolume
//
// Start playing a sound at the given volume (in the range 0 to 255).
//
// -ACB- 1998/08/10: Altered Error Messages
// -KM-  1998/09/01: Looping support
// -AJA- 1999/09/10: Made static.
//
static int StartSoundAtVolume(mobj_t *origin, sfxinfo_t *sfx, int volume)
{
  int snd_num = sfx->normal.sounds[0];
  int rc, sep;
  int cnum, orig_vol;

  boolean_t looping;
  const char *error;

  if (nosound)
    return -1;

  looping = false;

  if (! CacheSound(sfx))
    return -1;

  volume = MIN(255, (int)(volume * sfx->volume));
  orig_vol = volume;

  if (origin && sfx->looping)
    looping = true;
  
  // Check to see if it is audible,
  //  and if not, modify the params

  if (origin && origin != consoleplayer->mo)
  {
    rc = AdjustSoundParams(sfx, consoleplayer->mo, origin,
        &volume, &sep);

    if (!rc)
      return -1;

    if (origin->x == consoleplayer->mo->x && origin->y == consoleplayer->mo->y)
    {
      sep = NORM_SEP;
    }
  }
  else
  {
    sep = NORM_SEP;
    volume = edgemin(volume, MAX_VOLUME);
  }

  // try to find a channel
  cnum = GetSoundChannel(origin, sfx);

  if (cnum < 0)
    return -1;

  playingsfx[cnum].looping  = looping;
  playingsfx[cnum].sfxinfo  = sfx;
  playingsfx[cnum].origin   = origin;
  playingsfx[cnum].orig_vol = orig_vol;
  playingsfx[cnum].channel  = I_SoundPlayback(snd_num, sep, volume, looping);

  // Hardware cannot cope. Channel not allocated
  if (playingsfx[cnum].channel == -1)
  {
    playingsfx[cnum].orig_vol = 0;
    playingsfx[cnum].origin   = NULL;
    playingsfx[cnum].sfxinfo  = NULL;
    playingsfx[cnum].looping  = false;

    error = I_SoundReturnError();
    L_WriteDebug("%s\n",error);
    return -1;
  }

#if (DEBUG_SOUND)
  L_WriteDebug("StartSoundAtVolume: playing sound %s vol %d chan %d "
      "voice %d\n", sfx->ddf.name, volume, cnum, playingsfx[cnum].channel);
#endif

  return cnum;
}

// ===============End of Internals================

//
// FlushSoundCaches
//
// Destroys unused sounds.
// At extreme urgency, also destroys used sounds.
//
// -ES- 2000/02/07 Written.
//
static void FlushSoundCaches(z_urgency_e urge)
{
  int i;
  int n = 0;
  sfxinfo_t *sfx;

  switch (urge)
  {
    case Z_UrgencyLow: n = numcachedsfx / 32; break;
    case Z_UrgencyMedium: n = numcachedsfx / 8; break;
    case Z_UrgencyHigh: n = numcachedsfx / 2; break;
    case Z_UrgencyExtreme: n = numcachedsfx; break;
  }

  for (i = 0, sfx = sfxcachehead.next; i < n; i++)
  {
    DEV_ASSERT(sfx != &sfxcachehead, ("S_FlushSoundCaches: Internal Error: miscount"));
    sfx = sfx->next;
    // Do not kill playing sounds unless urge is extreme.
    // Fixme: Implement SoundIsPlaying.
    // if (!I_SoundIsPlaying(sfx->prev->id) || urge == Z_UrgencyExtreme)
    {
      RemoveSoundFromCache(sfx->prev);
    }
  }
}

//
// S_Init
//
// Sets up non-system specific sound system. Loads all the sfx listed from
// the DDF file.
//
// -ACB- 1999/10/09 Re-written from scratch.
//
boolean_t S_Init(void)
{
  int i;
  
  sfxcachehead.next = sfxcachehead.prev = &sfxcachehead;
  Z_RegisterCacheFlusher(FlushSoundCaches);

  if (nosound)
    return true; // we allowed to fail with no sound

  for (i = 0; i < playingsfxnum; i++)
  {
    playingsfx[i].sfxinfo = NULL;
    playingsfx[i].origin = NULL;
    playingsfx[i].orig_vol = 0;
    playingsfx[i].channel = -1;
    playingsfx[i].paused = false;
    playingsfx[i].looping = false;
  }

  return true;
}

//
// S_SoundLevelInit
//
// Pre-level startup code. Kills playing sounds at start of level.
//
void S_SoundLevelInit(void)
{
  int cnum;

  if (nosound)
    return;

  // kill all playing sounds at start of level (trust me - a good idea)
  for (cnum = 0; cnum < playingsfxnum; cnum++)
    S_StopChannel(cnum);
}

//
// S_StartSound
//
int S_StartSound(mobj_t *origin, sfx_t *sound_id)
{
  int volume;

  if (nosound)
    return -1;

  // No volume - don't play any sounds
  if (!soundvolume)
    return -1;

  volume = (MAX_VOLUME - (S_MAX_VOLUME*8)) + (soundvolume*8);

  // -KM- 1998/11/25 Fixed this, added origin check
  if (!sound_id)
  {
// -ACB- 2000/01/09 Quick hack to test the system specifics - START
//    if (origin)
//      S_StopSound(origin);
    if (origin)
      S_StopLoopingSound(origin);
// -ACB- 2000/01/09 Quick hack to test the system specifics - END

    return -1;
  }

  return StartSoundAtVolume(origin, DDF_SfxSelect(sound_id), volume);
}

//
// S_ResumeSounds
//
// This resumes the playing of all paused effects 
//
// -ACB- 1999/10/17
//
void S_ResumeSounds(void)
{
  int cnum;
  const char *error;

  for (cnum = 0; cnum < playingsfxnum; cnum++)
  {
    if (playingsfx[cnum].paused == true)
    {
      if (!I_SoundResume(playingsfx[cnum].channel))
      {
        error = I_SoundReturnError();
        L_WriteDebug("%s\n", error);
        return;
      }

      playingsfx[cnum].paused = false;
    }
  }
}

//
// S_PauseSounds
//
// This stops all the sound effects playing 
//
// -ACB- 1999/10/17
//
void S_PauseSounds(void)
{
  int cnum;
  const char *error;

  for (cnum = 0; cnum < playingsfxnum; cnum++)
  {
    if (playingsfx[cnum].sfxinfo && I_SoundCheck(playingsfx[cnum].channel))
    {
      if(!I_SoundPause(playingsfx[cnum].channel))
      {
        error = I_SoundReturnError();
        L_WriteDebug("%s\n", error);
        return;
      }

      playingsfx[cnum].paused = true;
    }
  }
}

//
// S_StopSound
//
// Store sounds from this object
//
void S_StopSound(mobj_t *origin)
{
  int cnum;

  if (nosound)
    return;

  for (cnum = 0; cnum < playingsfxnum; cnum++)
  {
    if (playingsfx[cnum].sfxinfo && (playingsfx[cnum].origin == origin))
      S_StopChannel(cnum);
  }
}

//
// S_RemoveSoundOrigin
//
// Removes all dependencies to the object, but doesn't stop its sounds
// (they will be played until they end)
// -ES- FIXME: Implement properly (now it just cuts off the sound)
//
void S_RemoveSoundOrigin(mobj_t *origin)
{
  int cnum;

  if (nosound)
    return;

  for (cnum = 0; cnum < playingsfxnum; cnum++)
  {
    if (playingsfx[cnum].sfxinfo && (playingsfx[cnum].origin == origin))
      S_StopChannel(cnum);
  }
}

//
// S_AddToFreeQueue
//
// Sound origins can be killed sometimes, when the sounds should go on.
// This routine makes sure that the origin isn't removed until the sound
// has finished.
//
void S_AddToFreeQueue(mobj_t *origin, void *block)
{
  int cnum;
  free_origin_t *q;

  if (nosound)
  {
    Z_Free(block);
    return;
  }

  for (cnum = 0; cnum < playingsfxnum; cnum++)
  {
    if (playingsfx[cnum].sfxinfo && (playingsfx[cnum].origin == origin))
    {
      // The origin is playing a sound, so we must add it to the queue.
      for (q = free_queue; q; q = q->next)
      {
        if (q->origin == origin)
          return;                // it's already queued
      }

      // create new queue element
      q         = Z_New(free_origin_t, 1);
      q->next   = free_queue;
      q->origin = origin;
      q->block  = block;
      return;
    }
  }

  // No sound playing, so we can remove the block.
  Z_Free(block);
}

//
// S_UpdateSounds
//
// Updates sounds
//
void S_UpdateSounds(mobj_t *listener)
{
  int audible;
  int cnum;
  int volume;
  int sep;
  playsfx_t *c;
  free_origin_t *q, *next, *prev;
  boolean_t kill;

  if (nosound)
    return;

  for (cnum = 0; cnum < playingsfxnum; cnum++)
  {
    c = &playingsfx[cnum];

    if (! c->sfxinfo || c->paused)
      continue;

    if (! I_SoundCheck(c->channel))
    {
      // if channel is allocated but sound has stopped, free it
      S_StopChannel(cnum);
      continue;
    }
    
    // initialise parameters
    volume = c->orig_vol;
    sep = NORM_SEP;

    // check non-local sounds for distance clipping
    //  or modify their params
    if (c->origin && (listener != c->origin))
    {
      // Check for freed origin in intolerant mode
      DEV_ASSERT2(*(int *)(&c->origin->x) != -1);

      audible = AdjustSoundParams(c->sfxinfo, listener, c->origin, 
          &volume, &sep);

      if (!audible)
        S_StopChannel(cnum);
      else
        I_SoundAlter(c->channel, sep, volume);
    }
  }

  prev = NULL;
  for (q = free_queue; q; q = next)
  {
    next = q->next;

    // check if the sound has stopped, so we can kill it
    kill = true;
    for (cnum = 0; cnum < playingsfxnum; cnum++)
    {
      if (playingsfx[cnum].sfxinfo && (playingsfx[cnum].origin == q->origin))
      {
        // it's still playing
        kill = false;
        break;
      }
    }

    if (kill)
    {
      // Wheee, it doesn't play anymore. Kill.
      if (prev)
        prev->next = q->next;
      else
        free_queue = q->next;

      Z_Free(q->block);
      Z_Free(q);
    }
    else
      prev = q;
  }
}

//
// S_Ticker
//
// Called each tick.
//
// -AJA- 1999/09/10: Written.
// -ACB- 1999/10/06: Include update sound routines here.
//
void S_SoundTicker(void)
{
  I_SoundTicker();
  S_UpdateSounds(consoleplayer->mo);
}

//
// S_GetSfxVolume
//
int S_GetSfxVolume(void)
{
  return soundvolume;
}

//
// S_SetSfxVolume
//
void S_SetSfxVolume(int volume)
{
  soundvolume = edgemid(S_MIN_VOLUME, volume, S_MAX_VOLUME);
}

//
// S_StopLoopingChannel
//
void S_StopLoopingChannel(int cnum)
{
  playsfx_t *c;

  if (nosound)
    return;

  c = &playingsfx[cnum];

  if (c->sfxinfo)
  {
    I_SoundStopLooping(c->channel);
    c->looping = false;
  }

  return;
}

//
// S_StopLoopingSound
//
// Stops all sounds looping for this object
//
void S_StopLoopingSound(mobj_t *origin)
{
  int cnum;

  if (nosound)
    return;

  for (cnum = 0; cnum < playingsfxnum; cnum++)
  {
    if (playingsfx[cnum].looping &&
        playingsfx[cnum].sfxinfo &&
        (playingsfx[cnum].origin == origin))
    {
      S_StopLoopingChannel(cnum);
    }
  }

  return;
}

//
// S_StopChannel
//
// Stop this internal channel playing
//
void S_StopChannel(int cnum)
{
  playsfx_t *c;

  if (nosound)
    return;

  c = &playingsfx[cnum];

  if (c->sfxinfo)
  {
    // stop the sound, be it playing or finished
    I_SoundKill(c->channel);

    c->sfxinfo = NULL;
    c->looping = false;
    c->paused  = false;
  }

  return;
}

