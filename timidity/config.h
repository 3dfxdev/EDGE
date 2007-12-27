/*
    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/* This is for use with the EDGE Platform Interface */

#include "epi/epi.h"
#include "epi/endianess.h"

/* When a patch file can't be opened, one of these extensions is
   appended to the filename and the open is tried again.
 */
#define PATCH_EXT_LIST { ".pat", 0 }

/* Acoustic Grand Piano seems to be the usual default instrument. */
#define DEFAULT_PROGRAM 0

/* 9 here is MIDI channel 10, which is the standard percussion channel.
   Some files (notably C:\WINDOWS\CANYON.MID) think that 16 is one too. 
   On the other hand, some files know that 16 is not a drum channel and
   try to play music on it. This is now a runtime option, so this isn't
   a critical choice anymore. */
#define DEFAULT_DRUMCHANNELS ((1<<9) | (1<<15))

/* A somewhat arbitrary frequency range. The low end of this will
   sound terrible as no lowpass filtering is performed on most
   instruments before resampling. */
#define MIN_OUTPUT_RATE 	4000
#define MAX_OUTPUT_RATE 	65000

/* In percent. */
/* #define DEFAULT_AMPLIFICATION 	70 */
/* #define DEFAULT_AMPLIFICATION 	50 */
#define DEFAULT_AMPLIFICATION 	30

/* Default sampling rate, default polyphony, and maximum polyphony.
   All but the last can be overridden from the command line. */
#define DEFAULT_RATE	32000
/* #define DEFAULT_VOICES	32 */
/* #define MAX_VOICES	48 */
#define DEFAULT_VOICES	256
#define MAX_VOICES	256
#define MAXCHAN		16
/* #define MAXCHAN		64 */
#define MAXNOTE		128

/* 1000 here will give a control ratio of 22:1 with 22 kHz output.
   Higher CONTROLS_PER_SECOND values allow more accurate rendering
   of envelopes and tremolo. The cost is CPU time. */
#define CONTROLS_PER_SECOND 1000


/* How many bits to use for the fractional part of sample positions.
   This affects tonal accuracy. The entire position counter must fit
   in 32 bits, so with FRACTION_BITS equal to 12, the maximum size of
   a sample is 1048576 samples (2 megabytes in memory). The GUS gets
   by with just 9 bits and a little help from its friends...
   "The GUS does not SUCK!!!" -- a happy user :) */
#define FRACTION_BITS 12

#define MAX_SAMPLE_SIZE (1 << (32-FRACTION_BITS))


/* For some reason the sample volume is always set to maximum in all
   patch files. Define this for a crude adjustment that may help
   equalize instrument volumes. */
#define ADJUST_SAMPLE_VOLUMES

/* The number of samples to use for ramping out a dying note. Affects
   click removal. */
#define MAX_DIE_TIME 20


/* Shawn McHorse's resampling optimizations. These may not in fact be
   faster on your particular machine and compiler. You'll have to run
   a benchmark to find out. */
#define PRECALC_LOOPS

/* If calling ldexp() is faster than a floating point multiplication
   on your machine/compiler/libm, uncomment this. It doesn't make much
   difference either way, but hey -- it was on the TODO list, so it
   got done. */
/* #define USE_LDEXP */

/**************************************************************************/
/* Anything below this shouldn't need to be changed unless you're porting
   to a new machine with other than 32-bit, big-endian words. */
/**************************************************************************/

/* change FRACTION_BITS above, not these */
#define INTEGER_BITS (32 - FRACTION_BITS)
#define INTEGER_MASK (0xFFFFFFFF << FRACTION_BITS)
#define FRACTION_MASK (~ INTEGER_MASK)

/* This is enforced by some computations that must fit in an int */
#define MAX_CONTROL_RATIO 255

typedef unsigned int uint32;
typedef int int32; 
typedef unsigned short uint16;
typedef short int16;
typedef unsigned char uint8;
typedef char int8;

#define MAX_AMPLIFICATION 800

/* You could specify a complete path, e.g. "/etc/timidity.cfg", and
   then specify the library directory in the configuration file. */
#define CONFIG_FILE	"timidity.cfg"
#define CONFIG_FILE_ETC "/etc/timidity.cfg"
#define CONFIG_FILE_ETC_TIMIDITY "/etc/timidity/timidity.cfg"

#if defined(__WIN32__) || defined(__OS2__)
#define DEFAULT_PATH	"\\TIMIDITY"
#else
#define DEFAULT_PATH	"/usr/local/lib/timidity"
#endif

/* These affect general volume */
#define GUARD_BITS 3
#define AMP_BITS (15-GUARD_BITS)

#if 1
   typedef int16 sample_t;
   typedef int32 final_volume_t;
#  define FINAL_VOLUME(v) (v)
#  define MAX_AMP_VALUE ((1<<(AMP_BITS+1))-1)
#endif

typedef int16 resample_t;

#ifdef USE_LDEXP
#  define FSCALE(a,b) ldexp((a),(b))
#  define FSCALENEG(a,b) ldexp((a),-(b))
#else
#  define FSCALE(a,b) (float)((a) * (double)(1<<(b)))
#  define FSCALENEG(a,b) (float)((a) * (1.0L / (double)(1<<(b))))
#endif

/* Vibrato and tremolo Choices of the Day */
#define SWEEP_TUNING 38
#define VIBRATO_AMPLITUDE_TUNING 1.0L
#define VIBRATO_RATE_TUNING 38
#define TREMOLO_AMPLITUDE_TUNING 1.0L
#define TREMOLO_RATE_TUNING 38

#define SWEEP_SHIFT 16
#define RATE_SHIFT 5

#define VIBRATO_SAMPLE_INCREMENTS 32

#ifndef PI
  #define PI 3.14159265358979323846
#endif

/* The path separator (D.M.) */
#if defined(__WIN32__) || defined(__OS2__)
#  define PATH_SEP '\\'
#  define PATH_STRING "\\"
#else
#  define PATH_SEP '/'
#  define PATH_STRING "/"
#endif

