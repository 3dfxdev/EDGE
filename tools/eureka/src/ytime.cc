//------------------------------------------------------------------------
//  TIME UTILS
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

#include "main.h"
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "ytime.h"


/*
 *  Return the current time in ms
 */
unsigned long y_milliseconds ()
{
#ifdef Y_GETTIMEOFDAY
  struct timeval tv;
  struct timezone tz;
  if (gettimeofday (&tv, &tz))
  {
    nf_bug ("gettimeofday() error (%s)", strerror (errno));
    return 0;
  }
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#else  /* Sucks ! */
  static const double ms_per_clock = 1000.0 / CLOCKS_PER_SEC;
  return (unsigned long) (clock () * ms_per_clock);
#endif
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
