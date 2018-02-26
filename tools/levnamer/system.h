//------------------------------------------------------------------------
// SYSTEM : System specific code (memory, I/O, etc).
//------------------------------------------------------------------------
//
//  LevNamer (C) 2001 Andrew Apted
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

#ifndef __LEVNAMER_SYSTEM_H__
#define __LEVNAMER_SYSTEM_H__

/* ----- types --------------------------- */

typedef char  sint8_g;
typedef short sint16_g;
typedef int   sint32_g;
   
typedef unsigned char  uint8_g;
typedef unsigned short uint16_g;
typedef unsigned int   uint32_g;

typedef double float_g;
typedef double angle_g;  // degrees, 0 is E, 90 is N

/* ----- variables --------------------------- */

extern int no_progress;

/* ----- constants ----------------------------- */

#define TRUE   1
#define FALSE  0

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

/* ----- useful macros ---------------------------- */

#ifndef MAX
#define MAX(x,y)  ((x) > (y) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x,y)  ((x) < (y) ? (x) : (y))
#endif

#ifndef ABS
#define ABS(x)  ((x) >= 0 ? (x) : -(x))
#endif

#ifndef INLINE_G
#define INLINE_G  /* nothing */
#endif

/* ----- function prototypes ---------------------------- */

// display normal messages & warnings to the screen
void PrintMsg(const char *str, ...);
void PrintWarn(const char *str, ...);

// this only used for debugging
void PrintDebug(const char *str, ...);

// display a fatal error message and abort the program
void FatalError(const char *str, ...);
void InternalError(const char *str, ...);

// allocate and clear some memory.  guaranteed not to fail.
void *SysCalloc(int size);

// re-allocate some memory.  guaranteed not to fail.
void *SysRealloc(void *old, int size);

// duplicate a string
char *SysStrdup(const char *str);
char *SysStrndup(const char *str, int size);

// compare two strings case insensitively
int StrCaseCmp(const char *A, const char *B);

// make string uppercase
void StrUpperCase(char *str);

// free some memory or a string
void SysFree(void *data);

// prepare screen for progress indicator
void InitProgress(void);
void TermProgress(void);

// begin the progress indicator.  `target' is the target value that
// each ShowProgress() call will move towards, or ZERO to get the
// whirling baton.
//
void StartProgress(int target);

// update and show the progress indicator
void ShowProgress(int step);

// hide the progress indicator
void ClearProgress(void);

// round a positive value up to the nearest power of two.
int RoundPOW2(int x);


#endif /* __LEVNAMER_SYSTEM_H__ */
